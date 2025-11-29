// lynx_sound.h
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int lynx_sampleRate;

void lynx_sound_init(int sample_rate);
void lynx_sound_shutdown(void);
void lynx_sound_set_volume(uint8_t vol);
void lynx_sound_submit_frames_stereo(const int16_t* samples, size_t frames);

#ifdef __cplusplus
}
#endif
