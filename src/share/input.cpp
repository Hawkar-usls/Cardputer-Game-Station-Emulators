#include "input.h"

#include <algorithm>
#include <esp_sleep.h>
#include <esp_system.h>
#include "game_save.h"
#include <Preferences.h>

static uint32_t s_lastInputUs = 0;
uint32_t lastPadState = 0xFFFFFFFF;
static const uint32_t INPUT_POLL_PERIOD_MS = 32;
constexpr int64_t INPUT_POLL_PERIOD_US = 1000 * INPUT_POLL_PERIOD_MS;

// I2C joypad (M5Stack JoyV2)
static bool s_i2cPadPresent = false;
static constexpr uint8_t JOYSTICK_CENTER   = 128;
static constexpr uint8_t JOYSTICK_DEADZONE = 25;
static constexpr int CARDPUTER_I2C_SCL = 1;
static constexpr int CARDPUTER_I2C_SDA = 2;
static constexpr uint8_t JOYSTICK2_ADDR                = 0x63;
static constexpr uint8_t JOYSTICK2_ADC_VALUE_8BITS_REG = 0x10;
static constexpr uint8_t JOYSTICK2_BUTTON_REG          = 0x20;

namespace share
{
   bool shouldPollInput()
    {
        uint32_t now = esp_timer_get_time();
        if (now - s_lastInputUs < INPUT_POLL_PERIOD_US) {
            return false;
        }

        s_lastInputUs = now;
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

    void detectI2cPad()
    {
        Wire.begin(CARDPUTER_I2C_SDA, CARDPUTER_I2C_SCL);
        Wire.setClock(400000);

        Wire.beginTransmission(JOYSTICK2_ADDR);
        uint8_t err = Wire.endTransmission();

        s_i2cPadPresent = (err == 0);

        if (s_i2cPadPresent) {
            printf("[INPUT] M5 Unit Joystick2 detected at 0x%02X (SDA=%d, SCL=%d)\n",
                   JOYSTICK2_ADDR, CARDPUTER_I2C_SDA, CARDPUTER_I2C_SCL);
        } else {
            printf("[INPUT] No M5 Unit Joystick2 at 0x%02X (SDA=%d, SCL=%d)\n",
                   JOYSTICK2_ADDR, CARDPUTER_I2C_SDA, CARDPUTER_I2C_SCL);
        }
    }

    bool hasI2cPad()
    {
        return s_i2cPadPresent;
    }

    // Lecture 2 bytes X/Y
    static bool joystick2_read_xy(uint8_t& x, uint8_t& y)
    {
        if (!s_i2cPadPresent) return false;

        // Select 8bit register
        Wire.beginTransmission(JOYSTICK2_ADDR);
        Wire.write(JOYSTICK2_ADC_VALUE_8BITS_REG);
        if (Wire.endTransmission(false) != 0) {
            return false;
        }

        if (Wire.requestFrom(JOYSTICK2_ADDR, (uint8_t)2) != 2) {
            return false;
        }

        x = Wire.read();
        y = Wire.read();
        return true;
    }

    // Read button
    static bool joystick2_read_button(uint8_t& btn)
    {
        if (!s_i2cPadPresent) return false;

        Wire.beginTransmission(JOYSTICK2_ADDR);
        Wire.write(JOYSTICK2_BUTTON_REG);
        if (Wire.endTransmission(false) != 0) {
            return false;
        }

        if (Wire.requestFrom(JOYSTICK2_ADDR, (uint8_t)1) != 1) {
            return false;
        }

        btn = Wire.read();
        return true;
    }

    uint32_t pollI2cPad()
    {
        if (!s_i2cPadPresent) {
            return 0;
        }

        uint8_t x8 = 0;
        uint8_t y8 = 0;
        uint8_t btnRaw = 0;

        if (!joystick2_read_xy(x8, y8)) {
            return 0;
        }

        joystick2_read_button(btnRaw);

        uint32_t state = 0;

        // Axes
        if (x8 < (JOYSTICK_CENTER - JOYSTICK_DEADZONE)) {
            state |= PAD_RIGHT;
        } else if (x8 > (JOYSTICK_CENTER + JOYSTICK_DEADZONE)) {
            state |= PAD_LEFT;
        }

        if (y8 < (JOYSTICK_CENTER - JOYSTICK_DEADZONE)) {
            state |= PAD_DOWN;
        } else if (y8 > (JOYSTICK_CENTER + JOYSTICK_DEADZONE)) {
            state |= PAD_UP;
        }

        // button A
        if (btnRaw == 0x00) {
            state |= PAD_A;
        }

        return state;
    }
}
