#ifndef STATE_H
#define STATE_H

#include <bootloader/bootloader.h>
#include <Button/ButtonAVR.h>
#include <timer/timer.h>
#include <I2CAVR.h>
#include <DS3231.h>
#include <SerialUSB.h>

#include "Light.h"
#include "Encoder.h"

//We use a FSM to move between different modes.  Each mode will have differences in display
// and user interface.  The modes are listed below, along with comments describing what is
// happening and how to move to different states.

//Time mode (default state).  Shows the time.  Press the button to toggle light, turn encoder to
// adjust light brightness.  Hold button to enter menu.
#define MODE_TIME						0x00

//Menu mode: allow setting time, alarm, etc.
#define MODE_MENU						0x01

//Edit whatever menu item you are currently in
#define MODE_EDIT						0x02

//The menu indices for various functions
#define MENU_SET_ALARM_1				0x00
#define MENU_SET_ALARM_2				0x01
#define MENU_SET_ALARM_3				0x02
#define MENU_SET_TIME					0x03

#define MENU_SIZE						0x04

#define ALARM_COUNT						3

typedef struct alarm {
	dc_time_t time;
	uint8_t enabled;		//Bit mask for which days are enabled.  Defined as _BV(DAY), where DAY 0 is Sunday, 1 is Monday, etc.
	uint8_t lamp_speed;		//Time to go from 0 to full brightness, in minutes
	uint8_t music_speed;	//Time to go from 0 to full loudness, in minutes
	uint8_t music_index;	//Index of music to start playing.  0xFF means to play a random file.
} alarm_t;

namespace digitalcave {

	class State {
		private:
			I2CAVR i2c;
			DS3231 calendar;
			Encoder encoder;
			ButtonAVR button;

			alarm_t alarm[ALARM_COUNT];

			uint8_t mode = 0;
			int16_t light_brightness = 0;
			uint8_t menu_item = 0;	//From 0 to MENU_COUNT - 1.  The currently selected menu item.
			uint8_t edit_item = 0;	//Functionality depends on edit item.  Stuff like clock digits.

		public:
			State();

			void poll();

			dc_time_t get_time();

			alarm_t get_alarm(uint8_t index);
			void set_alarm(uint8_t index, alarm_t alarm);

			uint8_t get_mode();
			uint8_t get_menu_item();
			uint8_t get_edit_item();
			int16_t get_light_brightness();
	};
}

#endif
