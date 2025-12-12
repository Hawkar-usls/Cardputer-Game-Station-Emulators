#include "run_snes.h"
#include <Arduino.h>
#include "share/utils.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "snes_display.h"
#include "snes_stubs.h"
#include "snes_input.h"

extern "C" {
    #include "snes9x/snes9x.h"
}

/* Globals */
static uint32_t g_dstY = 0;
bool interlace_enabled = false;
uint32_t fieldParity = 0; 

/* Callback video line render Snes9x */
static void S9XLineRender(uint32_t y,
                          const uint16_t* pixels,
                          uint32_t width)
{
    snes_display_submit_line(g_dstY, pixels, width);
}

/* Hook controls Snes9x */
uint32_t S9xReadJoypad(int32_t port)
{
    if (port != 0)
        return 0;

    return snes_input_poll();
}

/* Hook Init Snes9x render */
bool S9xInitDisplay(void)
{
    GFX.Pitch      = SNES_WIDTH * sizeof(uint16_t);
    GFX.Pitch2     = GFX.Pitch;
    GFX.RealPitch  = GFX.Pitch;
    GFX.ZPitch     = SNES_WIDTH;

    GFX.PPL        = SNES_WIDTH;
    GFX.PPLx2      = SNES_WIDTH * 2;

    GFX.Screen     = NULL;
    GFX.SubScreen  = NULL;
    GFX.ZBuffer    = NULL;
    GFX.SubZBuffer = NULL;

    GFX.LineRenderMode = true;
    GFX.LinePPL        = SNES_WIDTH;
    GFX.LinePitch      = SNES_WIDTH * sizeof(uint16_t);

    return true;
}

bool snes_init()
{
    // Order important
    if (!S9xInitDisplay()) {
        printf("[SNES] S9xInitDisplay failed\n");
        return false;
    }

    if (!S9xInitMemory()) {
        printf("[SNES] S9xInitMemory failed\n");
        return false;
    }

    if (!S9xInitGFX()) {
        printf("[SNES] S9xInitGFX failed\n");
        return false;
    }

    if (!S9xInitMap()) {
        printf("[SNES] S9xInitMap failed\n");
        return false;
    }

    if (!S9xInitPpu()) {
        printf("[SNES] S9xInitPpu failed\n");
        return false;
    }

    if (!S9xInitLineBuffers()) {
        printf("[SNES] S9xInitLineBuffers failed\n");
        return false;
    }

    // NULL means use already mapped ROM
    if (!LoadROM(NULL)) {
        printf("[SNES] LoadROM failed\n");
        return false;
    }

    S9xFixColourBrightness();
    return true;
}

// ----------------------------------------------------
// RUN SNES
// ----------------------------------------------------
void run_snes(const uint8_t* rom, size_t romSize)
{
    printf("[SNES] ROM: %p (size %zu bytes)\n", rom, romSize);

    memset(&Settings, 0, sizeof(Settings));

    // Rom (mapped in flash)
    Memory.ROM           = (uint8_t*)rom;
    Memory.ROM_Offset    = 0;
    Memory.ROM_AllocSize = romSize;

    // Timing
    Settings.CyclesPercentage = 100;
    Settings.H_Max            = SNES_CYCLES_PER_SCANLINE;
    Settings.FrameTimePAL     = 20000;
    Settings.FrameTimeNTSC    = 16667;
    Settings.ControllerOption= SNES_JOYPAD;
    Settings.HBlankStart      = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    // Audio OFF (not enough RAM)
    Settings.SoundPlaybackRate = 0;
    Settings.SoundBufferSize   = 0;
    Settings.ThreadSound       = false;
    Settings.Mute              = true;
    Settings.SoundSync         = false;
    Settings.APUEnabled        = false;
    Settings.DisableSoundEcho  = true;
    Settings.InterpolatedSound = false;

    if (!snes_init()) {
        printf("[SNES] snes_init failed, aborting\n");
        return;
    }

    // LoadROM could set SRAM size, ensure no SRAM used
    Memory.SRAMSize = 0;
    Memory.SRAMMask = 0;
    Memory.SRAM     = NULL;

    S9xReset();

    // Timing variables
    const int targetFps = 60;
    const uint32_t frame_us = 1000000u / (uint32_t)targetFps;
    uint64_t next_frame_us = esp_timer_get_time();
    uint32_t frameCount = 0;
    uint32_t lastFpsMs  = millis();
    int64_t now;
    int64_t lateness;
    const uint32_t budget55_us = 1000000u / 55u;
    bool skipped_last_render = false;
    uint32_t last_frame_exec_us = 0;

    // Scaling variables
    const float scale = (float)PPU.ScreenHeight / (float)LCD_H;
    const float srcStart = 0.5f * (PPU.ScreenHeight - LCD_H * scale);
    float srcYf = srcStart;
    int32_t srcY = 0;

    // Init display and input
    snes_display_init();
    snes_display_start();
    snes_input_start();

    printf("[SNES] Core/Video only, no audio, no sram, no tilecache, %d FPS target\n",
           targetFps);

    heap_caps_check_integrity_all(true);

    while (true) {
        // Decide to skip frame render or not
        bool want_skip = (last_frame_exec_us > budget55_us);
        bool do_render = true;
        if (want_skip && !skipped_last_render) {
            do_render = false;
        }
        IPPU.RenderThisFrame = do_render;

        // Run one frame
        int64_t frame_start_us = esp_timer_get_time();
        S9xMainLoop();

        // Render frame (interlace or full)
        if (IPPU.RenderThisFrame) {

            const uint32_t startY = interlace_enabled ? fieldParity : 0;
            const uint32_t stepY  = interlace_enabled ? 2 : 1;

            for (uint32_t dstY = startY; dstY < LCD_H; dstY += stepY) {
                srcYf = srcStart + (dstY + 0.5f) * scale;
                srcY = (int32_t)srcYf;

                if (srcY < 0) srcY = 0;
                if (srcY >= (int32_t)PPU.ScreenHeight)
                    srcY = PPU.ScreenHeight - 1;

                g_dstY = dstY;
                S9xRenderLine_NoFramebuffer(
                    (uint32_t)srcY, S9XLineRender);
            }

            if (interlace_enabled) {
                fieldParity ^= 1;
            }
        }

        // Frame end
        int64_t frame_end_us = esp_timer_get_time();
        last_frame_exec_us =
            (uint32_t)(frame_end_us - frame_start_us);

        skipped_last_render = !IPPU.RenderThisFrame;

        // Log FPS + heap
        frameCount++;
        uint32_t nowMs = millis();
        if (nowMs - lastFpsMs >= 1000) {
            float fps =
                (frameCount * 1000.0f) / (nowMs - lastFpsMs);
            
            // Auto interlace toggle
            if (!interlace_enabled && fps < 48.0f)
                interlace_enabled = true;
            else if (interlace_enabled && fps > 52.0f)
                interlace_enabled = false;

            printf("[SNES] FPS: %.2f | HEAP: %u | INTERLACE: %s\n",
                   fps,
                   esp_get_free_heap_size(),
                   interlace_enabled ? "ON" : "OFF");

            frameCount = 0;
            lastFpsMs  = nowMs;
        }

        // pacing 60 Hz
        next_frame_us += frame_us;
        now = (int64_t)esp_timer_get_time();
        lateness = now - (int64_t)next_frame_us;

        if (lateness > 0) {
            if (lateness > (int64_t)frame_us) {
                next_frame_us = (uint64_t)now;
            }
            continue;
        } else {
            share::sleep_until_us(next_frame_us);
        }
    }
}
