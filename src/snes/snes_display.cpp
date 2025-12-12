#include "snes_display.h"

#include <Arduino.h>
#include <M5Cardputer.h>
#include "esp_heap_caps.h"

#include "snes9x/snes9x.h"

// Crop
static constexpr int CROP_X = (SNES_WIDTH - LCD_W) / 2; // 8
static constexpr int CROP_Y = (SNES_HEIGHT - LCD_H) / 2; // 44

#ifndef SNES_NO_THREADED_DISPLAY

// ================== DOUBLE BUFFER LINES ==================

typedef struct {
    uint16_t y;
    uint16_t pixels[LCD_W];
} SnesLineBuf;

enum BufState : uint8_t {
    BUF_FREE = 0,
    BUF_READY,
    BUF_DRAWING
};

static TaskHandle_t  s_task    = nullptr;
static volatile bool s_running = false;

// 2 buffers
static SnesLineBuf *s_buf = nullptr;     // [2]
static volatile BufState s_state[2] = { BUF_FREE, BUF_FREE };

// Notify value bits
static constexpr uint32_t NOTIF_BUF0 = (1u << 0);
static constexpr uint32_t NOTIF_BUF1 = (1u << 1);

// ================== DISPLAY TASK ==================

static void snes_display_task(void *arg)
{
    (void)arg;

    M5Cardputer.Display.startWrite();

    for (;;) {
        // Wait for notification
        uint32_t notif = 0;
        xTaskNotifyWait(0, 0xFFFFFFFFu, &notif, portMAX_DELAY);

        if (!s_running) {
            continue;
        }

        if (notif & NOTIF_BUF0) {
            if (s_state[0] == BUF_READY) {
                s_state[0] = BUF_DRAWING;

                uint16_t y = s_buf[0].y;
                if (y < LCD_H) {
                    M5Cardputer.Display.setAddrWindow(0, (int)y, LCD_W, 1);
                    M5Cardputer.Display.pushPixels(s_buf[0].pixels, LCD_W);
                }

                s_state[0] = BUF_FREE;
            }
        }

        if (notif & NOTIF_BUF1) {
            if (s_state[1] == BUF_READY) {
                s_state[1] = BUF_DRAWING;

                uint16_t y = s_buf[1].y;
                if (y < LCD_H) {
                    M5Cardputer.Display.setAddrWindow(0, (int)y, LCD_W, 1);
                    M5Cardputer.Display.pushPixels(s_buf[1].pixels, LCD_W);
                }

                s_state[1] = BUF_FREE;
            }
        }
    }

    M5Cardputer.Display.endWrite();
}

// ================== PUBLIC API ==================

extern "C" void snes_display_init(void)
{
    M5Cardputer.Display.setSwapBytes(true);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

   if (s_buf) {
        heap_caps_free(s_buf);
        s_buf = nullptr;
    }

    // 2 lines buffer
    s_buf = (SnesLineBuf *)heap_caps_malloc(
        sizeof(SnesLineBuf) * 2,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    if (!s_buf) {
        printf("[SNES-DISP] buffer alloc failed\n");
        return;
    }

    // reset states
    s_state[0] = BUF_FREE;
    s_state[1] = BUF_FREE;
}

extern "C" void snes_display_start(void)
{
    if (s_task) return;

    s_running = true;

    BaseType_t ok = xTaskCreatePinnedToCore(
        snes_display_task,
        "SnesDisp",
        1500, // stack
        nullptr,
        6, // prio
        &s_task,
        0 // core 0
    );

    if (ok != pdPASS) {
        if (s_task) vTaskDelete(s_task);
        s_task = nullptr;
        s_running = false;
    }
}

extern "C" void snes_display_stop(void)
{
    s_running = false;

    if (s_task) {
        xTaskNotify(s_task, 0, eNoAction);
        vTaskDelete(s_task);
        s_task = nullptr;
    }

    if (s_buf) {
        heap_caps_free(s_buf);
        s_buf = nullptr;
    }

    s_state[0] = BUF_FREE;
    s_state[1] = BUF_FREE;
}

// Called by the core for each line
extern "C" void snes_display_submit_line(uint32_t y,
                                        const uint16_t *pixels,
                                        uint32_t width)
{
    if (!pixels || !s_running || !s_task) return;
    if (y >= LCD_H) return;

    if (width > SNES_WIDTH) width = SNES_WIDTH;

    // Find a free buffer
    int idx = -1;
    if (s_state[0] == BUF_FREE) idx = 0;
    else if (s_state[1] == BUF_FREE) idx = 1;
    else {
        // the buffers are full, drop the line
        return;
    }

    // Mark DRAWING
    s_state[idx] = BUF_DRAWING;

    // Crop/copy
    s_buf[idx].y = (uint16_t)y;

    int srcX0 = CROP_X;
    int srcX1 = srcX0 + LCD_W;

    if (srcX0 < 0) srcX0 = 0;
    if ((uint32_t)srcX1 > width) srcX1 = (int)width;

    int copyW = srcX1 - srcX0;
    if (copyW <= 0) {
        s_state[idx] = BUF_FREE;
        return;
    }
    if (copyW > LCD_W) copyW = LCD_W;

    const uint16_t *src = pixels + srcX0;
    for (int i = 0; i < copyW; ++i) s_buf[idx].pixels[i] = src[i];
    for (int i = copyW; i < LCD_W; ++i) s_buf[idx].pixels[i] = 0x0000;

    // Mark READY
    s_state[idx] = BUF_READY;
    xTaskNotify(s_task, (idx == 0) ? NOTIF_BUF0 : NOTIF_BUF1, eSetBits);
}

#else

// ===================== NO TASK VERSION =====================

extern "C" void snes_display_init(void)
{
    M5Cardputer.Display.setSwapBytes(true);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
}

extern "C" void snes_display_start(void)
{
    M5Cardputer.Display.startWrite();
}

extern "C" void snes_display_stop(void)
{
    M5Cardputer.Display.endWrite();
}

extern "C" void snes_display_submit_line(uint32_t y,
                                         const uint16_t *pixels,
                                         uint32_t width)
{
    if (!pixels) return;
    if (y >= LCD_H) return; 

    if (width > SNES_WIDTH) width = SNES_WIDTH;

    static uint16_t lineBuf[LCD_W];

    int srcX0 = CROP_X;          // 8
    int srcX1 = srcX0 + LCD_W;   // 8 + 240 = 248

    if (srcX0 < 0)              srcX0 = 0;
    if ((uint32_t)srcX1 > width) srcX1 = width;

    int copyW = srcX1 - srcX0;
    if (copyW <= 0) {
        return;
    }
    if (copyW > LCD_W) copyW = LCD_W;

    const uint16_t *src = pixels + srcX0;
    for (int i = 0; i < copyW; ++i) {
        lineBuf[i] = src[i];
    }
    for (int i = copyW; i < LCD_W; ++i) {
        lineBuf[i] = 0x0000;
    }

    M5Cardputer.Display.setAddrWindow(0, (int)y, LCD_W, 1);
    M5Cardputer.Display.pushPixels(lineBuf, LCD_W);
}

#endif
