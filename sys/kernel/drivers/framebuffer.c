/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <drivers/framebuffer.h>
#include <vmm.h>

volatile uint8_t* framebuffer = 0;
uint32_t framebuffer_pitch = 0;
uint32_t framebuffer_width = 0;
uint32_t framebuffer_height = 0;
uint8_t  framebuffer_bpp = 0;
uint8_t  framebuffer_type = 0;

static uint8_t red_position, red_mask;
static uint8_t green_position, green_mask;
static uint8_t blue_position, blue_mask;

uint32_t framebuffer_color_value(uint8_t color) {
    static const uint8_t palette[16][3] = {
        {0,0,0}, {0,0,170}, {0,170,0}, {0,170,170}, {170,0,0}, {170,0,170},
        {170,85,0}, {170,170,170}, {85,85,85}, {85,85,255}, {85,255,85},
        {85,255,255}, {255,85,85}, {255,85,255}, {255,255,85}, {255,255,255}
    };
    uint8_t r = palette[color & 15][0], g = palette[color & 15][1], b = palette[color & 15][2];
    if (framebuffer_bpp == 15 || framebuffer_bpp == 16)
        return ((r >> (8 - red_mask)) << red_position) | ((g >> (8 - green_mask)) << green_position) | ((b >> (8 - blue_mask)) << blue_position);
    return ((uint32_t)r << red_position) | ((uint32_t)g << green_position) | ((uint32_t)b << blue_position);
}

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t value) {
    if (x >= framebuffer_width || y >= framebuffer_height) return;
    volatile uint8_t* pixel = framebuffer + y * framebuffer_pitch + x * ((framebuffer_bpp + 7) / 8);
    if (framebuffer_bpp == 32) *(volatile uint32_t*)pixel = value;
    else if (framebuffer_bpp == 24) { pixel[0] = value; pixel[1] = value >> 8; pixel[2] = value >> 16; }
    else *(volatile uint16_t*)pixel = (uint16_t)value;
}

void framebuffer_init(const struct multiboot_tag_framebuffer* tag) {
    if (!tag || tag->framebuffer_type != 1) return;
    if (tag->framebuffer_bpp != 15 && tag->framebuffer_bpp != 16 && tag->framebuffer_bpp != 24 && tag->framebuffer_bpp != 32) return;
    uint32_t phys_addr = tag->framebuffer_addr;
    /* Обращаемся к VMM и просим замапить физический адрес фреймбуфера на
     * виртуальный адрес в пространстве ядра (например, 1в1 на 0xFD000000)
     */
    uint32_t fb_phys = tag->framebuffer_addr;
    /* Вычисляем общий размер видеопамяти в байтах */
    uint32_t fb_size = tag->framebuffer_height * tag->framebuffer_pitch;

    /* Мапим абсолютно все страницы фреймбуфера, чтобы ядро
     * могло закрасить весь экран целиком, а не только первые 4 КБ как было до патча
     */
    for (uint32_t offset = 0; offset < fb_size; offset += 4096) {
        vmm_kernel_map(fb_phys + offset, fb_phys + offset, 0);
    }
    framebuffer = (volatile uint8_t*)phys_addr;
    framebuffer_pitch = tag->framebuffer_pitch; framebuffer_width = tag->framebuffer_width; framebuffer_height = tag->framebuffer_height;
    framebuffer_bpp = tag->framebuffer_bpp; framebuffer_type = tag->framebuffer_type;
    red_position = tag->red_field_position; red_mask = tag->red_mask_size;
    green_position = tag->green_field_position; green_mask = tag->green_mask_size;
    blue_position = tag->blue_field_position; blue_mask = tag->blue_mask_size;
}

int framebuffer_available(void) { return framebuffer != 0 && framebuffer_type == 1; }

void framebuffer_clear_color(uint32_t raw_color) {
    if (!framebuffer_available()) return;
    for (uint32_t y = 0; y < framebuffer_height; y++) {
        for (uint32_t x = 0; x < framebuffer_width; x++) {
            framebuffer_put_pixel(x, y, raw_color);
        }
    }
}
