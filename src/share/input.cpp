#include "input.h"

#include <algorithm>
#include <esp_sleep.h>
#include <esp_system.h>
#include "game_save.h"
#include <Preferences.h>

static uint32_t s_lastInputMs = 0;
uint32_t lastPadState = 0xFFFFFFFF;
static const uint32_t INPUT_POLL_PERIOD_MS = 10;

namespace share
{
   bool shouldPollInput()
    {
        uint32_t now = millis();
        if (now - s_lastInputMs < INPUT_POLL_PERIOD_MS) {
            return false;
        }

        s_lastInputMs = now;
        return true;
    }

    static inline bool key(char c) {
        return M5Cardputer.Keyboard.isKeyPressed(c);
    }

    void checkCommonInput(const Keyboard_Class::KeysState& status)
    {
        // Bouton GO → restart (hack for quit game and reset memory)
        if (M5Cardputer.BtnA.pressedFor(1000)) {
            Preferences prefs; // Mark quit game flag in NVS
            prefs.begin("cardputer_emu", false);  // RW
            prefs.putBool("quit_game", true);
            prefs.end();
            
            while (gameIsSaving()) {
                delay(1); // wait for save to finish
            }
            
            esp_restart();
        }

        // Volume +
        if (key(CARDPUTER_VOL_UP_1) || (status.fn && key(CARDPUTER_VOL_UP_2))) {
            int v = M5Cardputer.Speaker.getVolume();
            M5Cardputer.Speaker.setVolume(std::min(v + 3, 255));
        }

        // Volume -
        if (key(CARDPUTER_VOL_DOWN_1) || (status.fn && key(CARDPUTER_VOL_DOWN_2))) {
            int v = M5Cardputer.Speaker.getVolume();
            M5Cardputer.Speaker.setVolume(std::max(v - 3, 0));
        }

        // Bright +
        if (key(CARDPUTER_BRIGHT_UP)) {
            int b = M5Cardputer.Display.getBrightness();
            M5Cardputer.Display.setBrightness(std::min(b + 2, 255));
        }

        // Bright -
        if (key(CARDPUTER_BRIGHT_DOWN)) {
            int b = M5Cardputer.Display.getBrightness();
            M5Cardputer.Display.setBrightness(std::max(b - 2, 0));
        }
    }
}
