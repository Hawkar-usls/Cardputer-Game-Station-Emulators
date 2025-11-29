#include "genesis_input.h"
#include "share/input.h"

#include <M5Cardputer.h>
#include <Arduino.h>

static uint32_t s_lastCommonInputMs = 0;
static const uint32_t COMMON_INPUT_PERIOD_MS = 10;

extern "C" {
  // Gwenesis APIs 
  void gwenesis_io_pad_press_button(int pad, int idx);
  void gwenesis_io_pad_release_button(int pad, int idx);
  void gwenesis_io_get_buttons(void);
}

// Shared state
extern bool fullscreenMode;
extern uint8_t genesis_audio_volume;
extern int  genesisZoomPercent;

// Buttons mapping
enum {
  BTN_UP = 0,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_C,
  BTN_B,
  BTN_A,
  BTN_START,
};

/* Button state management */
static inline void set_button(int idx, bool pressed) {
  if (pressed) gwenesis_io_pad_press_button(0, idx);
  else         gwenesis_io_pad_release_button(0, idx);
}

/* Polling cardputer keyboard */
extern "C" void genesis_controller_poll() {
  M5Cardputer.update();
  Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();

  uint32_t now = millis();
  if (now - s_lastCommonInputMs >= COMMON_INPUT_PERIOD_MS) {
    s_lastCommonInputMs = now;
    share::checkCommonInput(ks);
  }

  // Screen mode toggle with '\'
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_SCREEN_TOGGLE)) {
    if (!fullscreenMode) {
      fullscreenMode = true;
      genesisZoomPercent = 100;
    } else {
      genesisZoomPercent += 10;
      if (genesisZoomPercent > 150) {
        genesisZoomPercent = 100;
        fullscreenMode = false;
      }
    }
    return;
  }

  // Zoom +/−
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_ZOOM_PLUS)) {
    if (!fullscreenMode) fullscreenMode = true;
    genesisZoomPercent += 1;
    if (genesisZoomPercent > 150) genesisZoomPercent = 150;
    return;
  }
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_ZOOM_MINUS)) {
    if (!fullscreenMode) fullscreenMode = true;
    genesisZoomPercent -= 1;
    if (genesisZoomPercent < 100) genesisZoomPercent = 100;
    return;
  }

  // Arrow keys: ZQSD / ,./
  const bool left  = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_LEFT_1) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_LEFT_2);
  const bool right = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_RIGHT_1) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_RIGHT_2);
  const bool up    = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_UP_1) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_UP_2);
  const bool down  = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_DOWN_1) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_DOWN_2) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_DOWN_3);

  // Buttons A/B/C to j/k/l
  const bool btnA     = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_BTN_A_1) || M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_BTN_A_2);
  const bool btnB     = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_BTN_B);
  const bool btnC     = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_BTN_A_2);
  const bool btnStart = M5Cardputer.Keyboard.isKeyPressed(CARDPUTER_BTN_START);

  // Apply state to the 8 buttons
  set_button(BTN_UP,    up);
  set_button(BTN_DOWN,  down);
  set_button(BTN_LEFT,  left);
  set_button(BTN_RIGHT, right);
  set_button(BTN_A,     btnA);
  set_button(BTN_B,     btnB);
  set_button(BTN_C,     btnC);
  set_button(BTN_START, btnStart);
}

/* Called by Gwenesis to poll button states */
extern "C" void gwenesis_io_get_buttons(void) {
  genesis_controller_poll();
}
