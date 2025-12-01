#pragma once
#include <M5Cardputer.h>

#define CARDPUTER_LEFT_1       'a'
#define CARDPUTER_LEFT_2       ','

#define CARDPUTER_RIGHT_1      'd'
#define CARDPUTER_RIGHT_2      '/'

#define CARDPUTER_UP_1         'e'
#define CARDPUTER_UP_2         ';'

#define CARDPUTER_DOWN_1       's'
#define CARDPUTER_DOWN_2       '.'
#define CARDPUTER_DOWN_3       'z'

#define CARDPUTER_BTN_A_1      'l'
#define CARDPUTER_BTN_A_2      'j'

#define CARDPUTER_BTN_B        'k'

#define CARDPUTER_BTN_START    '1'
#define CARDPUTER_BTN_SELECT   '2'

#define CARDPUTER_SCREEN_TOGGLE '\\'

#define CARDPUTER_ZOOM_PLUS    '/' // FN + arrow right
#define CARDPUTER_ZOOM_MINUS   ',' // FN + arrow left

#define CARDPUTER_VOL_UP_1          '='     // Volume +
#define CARDPUTER_VOL_UP_2          ';'     // FN + arrow up

#define CARDPUTER_VOL_DOWN_1        '-'     // Volume -
#define CARDPUTER_VOL_DOWN_2        '.'     // FN + arrow down

#define CARDPUTER_BRIGHT_UP         ']'     // Bright +
#define CARDPUTER_BRIGHT_DOWN       '['     // Bright -

extern uint32_t lastPadState;

namespace share
{
    bool shouldPollInput(); 
    void checkCommonInput(const Keyboard_Class::KeysState& status);
}
