#include "gbc_input.h"
#include <M5Cardputer.h>
#include <Arduino.h>
#include "share/input.h"

extern "C" {
  #include "gnuboy/gnuboy.h"
}

extern bool gbcFullScreen;
extern int  gbcZoomPercent;
extern int  gbPalette;

extern "C" int gbc_input_poll(void)
{
    const int dummy_ret = -1; 

    M5Cardputer.update();
    Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();

    share::checkCommonInput(ks);

    int pad = 0x00;

    // ================== SCREEN MODE ==================

    // Screen mode
    if (M5Cardputer.Keyboard.isChange() &&
        M5Cardputer.Keyboard.isKeyPressed('\\')) {
        
        // -1 is gameboy color mode
        if (gbPalette == -1) {
            //
            // fullscreen / zoom (used by GBC only)
            //
            if (!gbcFullScreen) {
                gbcFullScreen  = true;
                gbcZoomPercent = 100;
            } else {
                gbcZoomPercent += 10;
                if (gbcZoomPercent > 150) {
                    gbcZoomPercent = 100;
                    gbcFullScreen  = false;
                }
            }
        } else {
            //
            // palette toggle (used by GB only)
            //
            gbPalette++;
            if (gbPalette > 36) {
                gbPalette = 0;
            }

            gnuboy_set_palette((gb_palette_t)gbPalette);
            printf("[GBC] Palette -> %d\n", gbPalette);
        }

        return dummy_ret;
    }

    // ================== DIRECTIONS ==================
    // Left : 'a' or ','
    if (M5Cardputer.Keyboard.isKeyPressed('a') ||
        M5Cardputer.Keyboard.isKeyPressed(',')) {
        pad |=  GB_PAD_LEFT;
    }

    // Right : 'd' or '/'
    if (M5Cardputer.Keyboard.isKeyPressed('d') ||
        M5Cardputer.Keyboard.isKeyPressed('/')) {
        pad |=  GB_PAD_RIGHT;
    }

    // Up : 'e' or ';'
    if (M5Cardputer.Keyboard.isKeyPressed('e') ||
        M5Cardputer.Keyboard.isKeyPressed(';')) {
        pad |=  GB_PAD_UP;
    }

    // Down : 's', '.' or 'z'
    if (M5Cardputer.Keyboard.isKeyPressed('s') ||
        M5Cardputer.Keyboard.isKeyPressed('.') ||
        M5Cardputer.Keyboard.isKeyPressed('z')) {
        pad |=  GB_PAD_DOWN;
    }

    // ================== BOUTONS GBC ==================
    // A
    if (M5Cardputer.Keyboard.isKeyPressed('l') ||
        M5Cardputer.Keyboard.isKeyPressed('j')) {
        pad |=  GB_PAD_A;
    }

    // B
    if (M5Cardputer.Keyboard.isKeyPressed('k')) {
        pad |=  GB_PAD_B;
    }

    // START
    if (M5Cardputer.Keyboard.isKeyPressed('1')) {
        pad |=  GB_PAD_START;
    }

    // SELECT
    if (M5Cardputer.Keyboard.isKeyPressed('2')) {
        pad |=  GB_PAD_SELECT;
    }

    // ================== ZOOM  ==================
    if (ks.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
        if (!gbcFullScreen) gbcFullScreen = true;
        gbcZoomPercent = (gbcZoomPercent < 150) ? (gbcZoomPercent + 1) : 150;
        return dummy_ret;
    }
    if (ks.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
        if (!gbcFullScreen) gbcFullScreen = true;
        gbcZoomPercent = (gbcZoomPercent > 100) ? (gbcZoomPercent - 1) : 100;
        return dummy_ret;
    }

    return pad;
}
