#ifndef SEOS2021_FRAMEBUFFER_H
#define SEOS2021_FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct FrameBufferTexture FrameBufferTexture;

void framebuffer_init();

FrameBufferTexture *framebuffer_load_font(uint32_t fg, uint32_t bg);

int framebuffer_get_width();

int framebuffer_get_height();

void framebuffer_putch(int w, int h, char ch, FrameBufferTexture *font);

void framebuffer_repaint();

#endif // SEOS2021_FRAMEBUFFER_H
