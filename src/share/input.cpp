#include "input.h"

#include <algorithm>
#include <esp_sleep.h>
#include <esp_system.h>
#include "game_save.h"
#include <Preferences.h>

namespace share
{
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
        if (key('=') || (status.fn && key(';'))) {
            int v = M5Cardputer.Speaker.getVolume();
            M5Cardputer.Speaker.setVolume(std::min(v + 3, 255));
        }

        // Volume -
        if (key('-') || (status.fn && key('.'))) {
            int v = M5Cardputer.Speaker.getVolume();
            M5Cardputer.Speaker.setVolume(std::max(v - 3, 0));
        }

        // Bright +
        if (key(']')) {
            int b = M5Cardputer.Display.getBrightness();
            M5Cardputer.Display.setBrightness(std::min(b + 2, 255));
        }

        // Bright -
        if (key('[')) {
            int b = M5Cardputer.Display.getBrightness();
            M5Cardputer.Display.setBrightness(std::max(b - 2, 0));
        }
    }
}
