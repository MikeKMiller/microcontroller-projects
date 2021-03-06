#ifndef SOUND_H
#define SOUND_H

#include <Serial/SerialAVR.h>
#include <DFPlayerMini.h>
#include <RDA5807.h>
#include <I2CAVR.h>

#include "config.h"

#define SOUND_STATE_STOP			0
#define SOUND_STATE_PLAY			1

//#define SOUND_SOURCE_MP3			0
#define SOUND_SOURCE_FM				1
#define SOUND_SOURCE_DFPLAYER		0

//The total number of files.  Files should be named with three digits, starting at 001.mp3, etc.
// Real DFPlayerMinis can query the number of files, but the cheap Chinese clones don't.  Lame Amazon.
#define MAX_SOUND_FILE_COUNT		99

void music_init();

void music_poll();

void music_set_volume(int8_t volume);

void music_start(uint8_t source, config_t config);
void music_toggle(uint8_t source, config_t config);
void music_stop();

uint8_t music_is_playing();

uint16_t music_fm_channel();
void music_fm_scan(uint8_t direction);

#endif
