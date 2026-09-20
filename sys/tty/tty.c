/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <drivers/framebuffer.h>
#include <drivers/vga.h>

#include <fonts/cyrillic.h>

extern void vga_putc_color(char c, uint8_t color) __attribute__((weak));
extern void vga_clear(uint8_t color) __attribute__((weak));

static uint32_t tty_x = 0;
static uint32_t tty_y = 0;

static int cursor_enabled = 1;  /* Включен ли курсор вообще (\033[?25h/l) */
static int cursor_blinking = 1; /* Должен ли он мигать (\033[1q / 2q) */
static int cursor_state = 0;    /* Текущая фаза мигания (0 - скрыт, 1 - отрисован) */

static uint8_t tty_color = 0x07;

static int ansi_state = 0;
static int ansi_arg1 = 0;
static int ansi_arg2 = 0;
static int* ansi_current_arg = NULL;

/* Стандартная маппинг-таблица для ANSI цветов (0-7 и 8-15) */
static const uint8_t ansi_to_vga[16] = {
    0,  /* 30: Черный */
    4,  /* 31: Красный */
    2,  /* 32: Зеленый */
    6,  /* 33: Коричневый/Желтый */
    1,  /* 34: Синий */
    5,  /* 35: Пурпурный */
    3,  /* 36: Голубой */
    7,  /* 37: Светло-серый (Дефолт Linux) */
    8,  /* 90: Темно-серый */
    12, /* 91: Ярко-красный */
    10, /* 92: Ярко-зеленый */
    14, /* 93: Ярко-желтый */
    9,  /* 94: Ярко-синий */
    13, /* 95: Ярко-пурпурный */
    11, /* 96: Ярко-голубой */
    15  /* 97: Ярко-белый */
};

/* Палитра цветов (16 стандартных цветов EGA/VGA) */
static const uint8_t vga_palette[16][3] = {
    {0, 0, 0},       /* 0: Черный */
    {0, 0, 170},     /* 1: Синий */
    {0, 170, 0},     /* 2: Зеленый */
    {0, 170, 170},   /* 3: Голубой */
    {170, 0, 0},     /* 4: Красный */
    {170, 0, 170},   /* 5: Пурпурный */
    {170, 85, 0},    /* 6: Коричневый */
    {170, 170, 170}, /* 7: Светло-серый */
    {85, 85, 85},    /* 8: Темно-серый */
    {85, 85, 255},   /* 9: Ярко-синий */
    {85, 255, 85},   /* 10: Ярко-зеленый */
    {85, 255, 255},  /* 11: Ярко-голубой */
    {255, 85, 85},   /* 12: Ярко-красный */
    {255, 85, 255},  /* 13: Ярко-пурпурный */
    {255, 255, 85},  /* 14: Желтый */
    {255, 255, 255}  /* 15: Белый */
};

/* Сборка цвета под BPP фреймбуфера без внешних зависимостей */
static uint32_t tty_get_raw_color(uint8_t color_index) {
    uint8_t r = vga_palette[color_index & 15][0];
    uint8_t g = vga_palette[color_index & 15][1];
    uint8_t b = vga_palette[color_index & 15][2];

    /* Если у нас 15 или 16 бит (RGB565 / RGB555) - смещаем каналы */
    if (framebuffer_bpp == 15 || framebuffer_bpp == 16) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
    /* Для 24 и 32 бит (стандартный TrueColor RGB/BGR) */
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static uint32_t tty_width(void) {
    return framebuffer_available() ? (framebuffer_width / FONT_WIDTH) : VGA_WIDTH;
}

static uint32_t tty_height(void) {
    return framebuffer_available() ? (framebuffer_height / FONT_HEIGHT) : VGA_HEIGHT;
}

void tty_draw_cursor(void) {
    if (!framebuffer_available() || !cursor_enabled) return;

    uint32_t x_start = tty_x * FONT_WIDTH;
    uint32_t y_start = tty_y * FONT_HEIGHT;
    uint32_t inv_mask = framebuffer_bpp == 16 ? 0xFFFF : 0xFFFFFF;

    for (uint32_t py = 0; py < FONT_HEIGHT; py++) {
        for (uint32_t px = 0; px < FONT_WIDTH; px++) {
            uint32_t x = x_start + px;
            uint32_t y = y_start + py;
            if (x >= framebuffer_width || y >= framebuffer_height) continue;

            volatile uint8_t* pixel = framebuffer + y * framebuffer_pitch + x * ((framebuffer_bpp + 7) / 8);
            if (framebuffer_bpp == 32) {
                *(volatile uint32_t*)pixel ^= inv_mask;
            } else if (framebuffer_bpp == 24) {
                pixel[0] ^= 0xFF; pixel[1] ^= 0xFF; pixel[2] ^= 0xFF;
            } else {
                *(volatile uint16_t*)pixel ^= (uint16_t)inv_mask;
            }
        }
    }
}

static void tty_draw_char_fb(uint32_t col, uint32_t row, char c, uint8_t color) {
    const uint8_t* glyph = &font_data[(uint8_t)c * FONT_HEIGHT];
    uint32_t x_start = col * FONT_WIDTH;
    uint32_t y_start = row * FONT_HEIGHT;

    uint32_t foreground = tty_get_raw_color(color & 15);
    uint32_t background = tty_get_raw_color((color >> 4) & 15);

    for (uint32_t py = 0; py < FONT_HEIGHT; py++) {
        uint8_t glyph_row = glyph[py];
        for (uint32_t px = 0; px < FONT_WIDTH; px++) {
            uint32_t pixel_color = (glyph_row & (0x80 >> px)) ? foreground : background;
            framebuffer_put_pixel(x_start + px, y_start + py, pixel_color);
        }
    }
}

static void tty_scroll(void) {
    if (framebuffer_available()) {
        uint32_t font_height_pixels = FONT_HEIGHT;
        uint32_t bytes_per_line = framebuffer_pitch;
        uint32_t total_scroll_bytes = (framebuffer_height - font_height_pixels) * bytes_per_line;

        memcpy((void*)framebuffer, (const void*)(framebuffer + font_height_pixels * bytes_per_line), total_scroll_bytes);

        uint32_t bg_color = tty_get_raw_color((tty_color >> 4) & 15);
        for (uint32_t y = framebuffer_height - font_height_pixels; y < framebuffer_height; y++) {
            for (uint32_t x = 0; x < framebuffer_width; x++) {
                framebuffer_put_pixel(x, y, bg_color);
            }
        }
        tty_y = tty_height() - 1;
    }
}

static void tty_newline(void) {
    tty_x = 0;
    tty_y++;
    if (tty_y >= tty_height()) tty_scroll();
}

static int parse_ansi(char c) {
    static int ansi_is_dec = 0; /* Внутренний флаг для распознавания знака '?' */

    if (ansi_state == 0) {
        if (c != '\033') return 0;
        ansi_state = 1;
        ansi_arg1 = 0;
        ansi_arg2 = 0;
        ansi_current_arg = &ansi_arg1;
        ansi_is_dec = 0; /* Сбрасываем флаг при старте новой команды */
        return 1;
    }
    if (ansi_state == 1) {
        ansi_state = c == '[' ? 2 : 0;
        return 1;
    }
    if (ansi_state == 2) {
        /* Если сразу после '[' прилетел '?', запоминаем это и ждем цифры дальше */
        if (c == '?') {
            ansi_is_dec = 1;
            return 1;
        }
        if (c >= '0' && c <= '9') {
            *ansi_current_arg = *ansi_current_arg * 10 + c - '0';
            return 1;
        }
        if (c == ';') {
            ansi_current_arg = &ansi_arg2;
            return 1;
        }

        /* Перед выполнением команд очистки 'J' или прыжка 'H' стираем статичный курсор,
         * чтобы он не остался висеть артефактом на старом месте экрана.
         */
        if (cursor_state && !cursor_blinking) tty_draw_cursor();

        if (c == 'J' && (ansi_arg1 == 0 || ansi_arg1 == 2)) {
            if (framebuffer_available()) {
                uint32_t bg_raw = tty_get_raw_color((tty_color >> 4) & 15);
                framebuffer_clear_color(bg_raw);
            } else {
                if (vga_clear) vga_clear(tty_color);
            }
            tty_x = 0;
            tty_y = 0;
        } else if (c == 'H' || c == 'f') {
            uint32_t target_row = (ansi_arg1 > 0) ? ansi_arg1 - 1 : 0;
            uint32_t target_col = (ansi_arg2 > 0) ? ansi_arg2 - 1 : 0;
            if (target_row < tty_height()) tty_y = target_row;
            if (target_col < tty_width()) tty_x = target_col;
        } else if (c == 'm') {
            if (ansi_arg1 == 0) {
                tty_color = 0x07; /* Сброс на стандартный серый */
            } else if (ansi_arg1 >= 30 && ansi_arg1 <= 37) {
                tty_color = (tty_color & 0xF0) | ansi_to_vga[ansi_arg1 - 30];
            } else if (ansi_arg1 >= 90 && ansi_arg1 <= 97) {
                tty_color = (tty_color & 0xF0) | ansi_to_vga[ansi_arg1 - 90 + 8];
            }
            if (ansi_arg2 >= 30 && ansi_arg2 <= 37) {
                tty_color = (tty_color & 0xF0) | ansi_to_vga[ansi_arg2 - 30];
            }
        }
        /* Команда 'h' (Включить режим) */
        else if (c == 'h') {
            if (ansi_is_dec && ansi_arg1 == 25) {
                cursor_enabled = 1; /* \033[?25h - Показать курсор */
            }
        }
        /* Команда 'l' (Выключить режим) */
        else if (c == 'l') {
            if (ansi_is_dec && ansi_arg1 == 25) {
                cursor_enabled = 0; /* \033[?25l - Скрыть курсор */
            }
        }
        /* Команда 'q' (Стиль курсора) */
        else if (c == 'q') {
            if (ansi_arg1 == 1) {
                cursor_blinking = 1; /* \033[1q - Мигающий блок */
            } else if (ansi_arg1 == 2) {
                cursor_blinking = 0; /* \033[2q - Статичный блок */
                cursor_state = 1;    /* Принудительно делаем его видимым */
            }
        }

        if (!cursor_blinking) tty_draw_cursor();

        ansi_state = 0;
        return 1;
    }
    ansi_state = 0;
    return 1;
}

void tty_backspace(void) {
    if (tty_x > 0) tty_x--;
    else if (tty_y > 0) {
        tty_y--;
        tty_x = tty_width() - 1;
    }
    if (framebuffer_available()) {
        tty_draw_char_fb(tty_x, tty_y, ' ', tty_color);
    } else {
        if (vga_putc_color) vga_putc_color('\b', tty_color);
    }
}

void tty_write_char(char c) {
    if (parse_ansi(c)) return;

    if (framebuffer_available()) {
        if (c == '\n') {
            tty_newline();
            return;
        }
        if (c == '\r') {
            tty_x = 0;
            return;
        }
        if (c == '\t') {
            do tty_write_char(' '); while (tty_x && (tty_x & 7));
            return;
        }
        if (c == '\b') {
            tty_backspace();
            return;
        }

        tty_draw_char_fb(tty_x, tty_y, c, tty_color);
        tty_x++;
        if (tty_x >= tty_width()) tty_newline();
    } else {
        if (vga_putc_color) vga_putc_color(c, tty_color);
    }
}
