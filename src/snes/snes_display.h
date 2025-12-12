#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dimensions LCD (Cardputer)
#define LCD_W 240
#define LCD_H 135

void snes_display_init(void);
void snes_display_start(void);
void snes_display_stop(void);
void snes_display_submit_line(uint32_t y,
                              const uint16_t *pixels,
                              uint32_t width);

#ifdef __cplusplus
}
#endif
