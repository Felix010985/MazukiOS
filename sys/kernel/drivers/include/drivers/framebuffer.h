#ifndef DRIVERS_FRAMEBUFFER_H
#define DRIVERS_FRAMEBUFFER_H

#include <stdint.h>

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved;
    uint8_t red_field_position;
    uint8_t red_mask_size;
    uint8_t green_field_position;
    uint8_t green_mask_size;
    uint8_t blue_field_position;
    uint8_t blue_mask_size;
};

extern volatile uint8_t* framebuffer;
extern uint32_t framebuffer_pitch;
extern uint32_t framebuffer_width;
extern uint32_t framebuffer_height;
extern uint8_t  framebuffer_bpp;
extern uint8_t  framebuffer_type;

void framebuffer_init(const struct multiboot_tag_framebuffer* tag);
int framebuffer_available(void);
uint32_t framebuffer_color_value(uint8_t color);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t value);
void framebuffer_clear_color(uint32_t raw_color);

#endif
