#include "pce_input.h"

#include <M5Cardputer.h>
#include "share/input.h"

extern "C" {
  #include "pce-go/pce.h" 
}

extern bool pceFullScreen;
extern int  pceZoomLevel;

void pce_input_read(uint8_t joypads[8])
{
  for (int i = 0; i < 8; ++i) {
    joypads[i] = 0;
  }

  uint32_t buttons = 0;

  M5Cardputer.update();
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

  share::checkCommonInput(status);

  // Toggle / fullscreen
  if (M5Cardputer.Keyboard.isChange() &&
      M5Cardputer.Keyboard.isKeyPressed('\\')) {

    if (!pceFullScreen) {
      pceFullScreen  = true;
      pceZoomLevel  = 100;
    } else {
      pceZoomLevel += 10;
      if (pceZoomLevel > 150) {
        pceZoomLevel = 100;
        pceFullScreen = false;
      }
    }
    joypads[0] = 0;
    return;
  }

  // Zoom in
  if (status.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
    if (!pceFullScreen) pceFullScreen = true;
    pceZoomLevel += 1;
    if (pceZoomLevel > 150) pceZoomLevel = 150;
    joypads[0] = 0;
    return;
  }

  // Zoom out 
  if (status.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
    if (!pceFullScreen) pceFullScreen = true;
    pceZoomLevel -= 1;
    if (pceZoomLevel < 100) pceZoomLevel = 100;
    joypads[0] = 0;
    return;
  }

  // Gauche
  if (M5Cardputer.Keyboard.isKeyPressed('a') ||
      M5Cardputer.Keyboard.isKeyPressed(',')) {
    buttons |= JOY_LEFT;
  }

  // Droite
  if (M5Cardputer.Keyboard.isKeyPressed('d') ||
      M5Cardputer.Keyboard.isKeyPressed('/')) {
    buttons |= JOY_RIGHT;
  }

  // Haut
  if (M5Cardputer.Keyboard.isKeyPressed('e') ||
      M5Cardputer.Keyboard.isKeyPressed(';')) {
    buttons |= JOY_UP;
  }

  // Bas
  if (M5Cardputer.Keyboard.isKeyPressed('s') ||
      M5Cardputer.Keyboard.isKeyPressed('.') ||
      M5Cardputer.Keyboard.isKeyPressed('z')) {
    buttons |= JOY_DOWN;
  }

  // Select
  if (M5Cardputer.Keyboard.isKeyPressed('2')) {
    buttons |= JOY_SELECT;
  }

  // Start
  if (M5Cardputer.Keyboard.isKeyPressed('1')) {
    buttons |= JOY_RUN;
  }

  // Bouton I (A)
  if (M5Cardputer.Keyboard.isKeyPressed('l') ||
      M5Cardputer.Keyboard.isKeyPressed('j')) {
    buttons |= JOY_A;
  }

  // Bouton II (B)
  if (M5Cardputer.Keyboard.isKeyPressed('k')) {
    buttons |= JOY_B;
  }

  joypads[0] = (uint8_t)buttons;
}

// ================== HOOKS PCE-GO ==================

extern "C" void osd_input_read(uint8_t joypads[8])
{
  pce_input_read(joypads);
}