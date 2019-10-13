#include "state.h"

using namespace digitalcave;

//Hardware
static I2CAVR* i2c;
static DS3231* calendar;
static ButtonAVR* lampButton;
static ButtonAVR* musicButton;
uint8_t analog_pins[1];

extern time_t now;
extern tm_t now_tm;

//Alarm stuff
static alarm_t alarm[ALARM_COUNT];			//The actual alarms
static uint8_t lamp_alarm_triggered = 0;	//_BV(alarm_index) is set when alarm[alarm_index] is triggered.  If we turn the light off or adjust the brightness, this is reset to 0.  When it is non-zero, we increment light gradually.
static uint8_t music_alarm_triggered = 0;	//_BV(alarm_index) is set when alarm[alarm_index] is triggered.  If we turn the music off or adjust the volume, this is reset to 0.  When it is non-zero, we increment the music gradually.
static int8_t lamp_brightness = 50;			//Lamp brightness.  Between 1 and 100 inclusive.

//General stuff
static config_t config;						//System config
static int8_t display_brightness = 0;		//The brightness for the LED matrix.  0 - 15.
static float analog_value_running_average = 0;	//The running average of the analog values.  This determines the min_brightness, with hysterisis.
static uint8_t min_brightness = 0;			//The actual min brightness value.

//Menu state
static uint8_t mode = 0;					//Main modes.  TIME, MENU, EDIT
static int8_t menu_item = 0;				//From 0 to MENU_COUNT - 1.  The currently selected menu item.
static int8_t edit_item = 0;				//Functionality and valid range depends on edit item.  Stuff like setting times and alarms.

static time_t last_input_time = 0;			//Last time that a button was pressed / turned
static time_t lamp_turned_on_time = 0;		//Time that the lamp was turned on
static time_t music_turned_on_time = 0;		//Time that the music was started
static time_t last_alarm_trigger_time = 0;	//Time that the alarm was triggered

static volatile uint8_t update_display = 0;	//This is triggered in the ISR by the RTC at 1Hz, and indicates that it is time to update the time fields

static volatile uint8_t raw_analog_value = 0;

static void eeprom_store();
static void eeprom_load();

static uint8_t get_update_display();
static void update_time(time_t* time, tm_t* tm);
static void range_loop(int8_t* value, uint8_t min, uint8_t max);
static void range_constrain(int8_t* value, uint8_t min, uint8_t max);

void state_init(){
	i2c = new I2CAVR();
	calendar = new DS3231(i2c);
	lampButton = new ButtonAVR(&PORTC, PORTC6, 30, 25, 800, 500);
	musicButton = new ButtonAVR(&PORTC, PORTC7, 30, 25, 800, 500);

	timer_init();
	light_init();
	encoder_init();
	music_init();

	eeprom_load();

	lamp_brightness = config.lamp_brightness;

	//Randomize according to the current time.  We use random() during track selection playback
	update_time(&now, &now_tm);
	srandom(now);

	last_input_time = now;

	PORTE |= _BV(PORTE6);	//Pullup on E6; we use this to listen to the 1Hz square wave generated by the RTC (open drain) to know when we need to read the clock again.
	EICRB |= _BV(ISC61) | _BV(ISC60);
	EIMSK |= _BV(INT6);

	//Disable digital input on ADC pin 0
	DIDR0 |= _BV(0);

	//ADC Enable, ADC interrupt, prescaler = F_CPU / 128
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

	//Set AREF to VCC, left adjust (we only care about 8 bits anyway), and leave the MUX at 0
	ADMUX = _BV(REFS0) | _BV(ADLAR);

	//Free running mode (already default, but in case it was set previously)
	ADCSRB = 0x00;

	//Start ADC conversions
	ADCSRA |= _BV(ADSC);

	sei();
}

//The UI state machine.  Takes current state + input and moves between states.
void state_poll(){
	music_poll();

	uint8_t update_display = get_update_display();
	if (update_display){
		update_time(&now, &now_tm);
	}

	lampButton->sample(timer_millis());
	musicButton->sample(timer_millis());
	int8_t lamp_encoder_movement = encoder_get_movement_1();
	int8_t music_encoder_movement = encoder_get_movement_2();

	//Trigger alarms and adjust brightness / volume of triggered alarms
	for (uint8_t i = 0; i < ALARM_COUNT; i++){
		alarm_t a = alarm[i];

		//Trigger alarm if time matches
		if (a.enabled & _BV(now_tm.tm_wday)
				&& (lamp_alarm_triggered & _BV(i)) == 0x00		//Don't trigger multiple times for the same alarm
				&& (music_alarm_triggered & _BV(i)) == 0x00		//Don't trigger multiple times for the same alarm
				&& a.hour == now_tm.tm_hour
				&& a.minute == now_tm.tm_min
				&& 0 == now_tm.tm_sec){
			lamp_alarm_triggered |= _BV(i);
			music_alarm_triggered |= _BV(i);
			last_alarm_trigger_time = now;
			last_input_time = now;		//Workaround so that we don't immediately turn off the alarm when it triggers due to lack of input
			light_set(0);
			light_on();
			music_start(a.music_folder, config.music_count[a.music_folder - 1]);
		}

		//Increment the brightness by the alarm ramp-up values
		if (update_display && lamp_alarm_triggered & _BV(i)){
			lamp_brightness = fmin(1, (double) (now - last_alarm_trigger_time) / 60 / a.lamp_speed) * a.lamp_brightness;		//Number of minutes since the alarm was triggered, divided by the lamp speed, multiplied by lamp brightness
			range_constrain(&lamp_brightness, 1, a.lamp_brightness);
			light_set(lamp_brightness);
		}
		//Increment the music volume by the alarm ramp-up values
		if (update_display && music_alarm_triggered & _BV(i)){
			int8_t volume = fmin(1, (double) (now - last_alarm_trigger_time) / 60 / a.music_speed) * a.music_volume;			//Number of minutes since the alarm was triggered, divided by music speed, multiplied by max volulme
			range_constrain(&volume, 1, 30);
			music_set_volume(volume);
		}
	}

	//Time mode - this is what shows 99% of the time.
	if (mode == MODE_TIME){
		//Enter menu
		if (lampButton->longPressEvent()){
			mode = MODE_MENU;
			menu_item = 0;
		}
		//Turn on lamp
		else if (lampButton->releaseEvent()){
			lamp_alarm_triggered = 0x00;
			light_toggle();
			if (light_state()){
				edit_item = EDIT_TIME_LAMP;
				lamp_turned_on_time = now;
				range_constrain(&lamp_brightness, 1, 100);
				light_set(lamp_brightness);
			}
			else {
				edit_item = EDIT_TIME_TIME;
				eeprom_store();
			}
		}
		//Turn on music
		else if (musicButton->releaseEvent()){
			music_alarm_triggered = 0x00;
			music_set_volume(config.volume);
			music_toggle(config.music_folder, config.music_count[config.music_folder - 1]);
			if (music_is_playing()){
				edit_item = EDIT_TIME_MUSIC;
				music_turned_on_time = now;
			}
			else {
				edit_item = EDIT_TIME_TIME;
				eeprom_store();
			}
		}
		//Adjust lamp brightness; can happen when light is on or off
		else if (lamp_encoder_movement != 0){
			lamp_alarm_triggered = 0x00;		//Turn off any alarms currently enabled
			edit_item = EDIT_TIME_LAMP;
			lamp_brightness += lamp_encoder_movement;
			range_constrain(&lamp_brightness, 1, 100);
			if (light_state()){
				light_set(lamp_brightness);
			}
			config.lamp_brightness = lamp_brightness;
		}
		//Adjust music volume; can happen when music is on or off
		else if (music_encoder_movement != 0){
			music_alarm_triggered = 0x00;		//Turn off any alarms currently enabled
			edit_item = EDIT_TIME_MUSIC;
			int8_t volume = config.volume + music_encoder_movement;
			range_constrain(&volume, 1, 30);
			if (music_is_playing()){
				music_set_volume(volume);
			}
			config.volume = volume;
		}
	}

	//Menu mode - pick a menu item
	else if (mode == MODE_MENU){
		if (lampButton->longPressEvent()){
			mode = MODE_TIME;
			edit_item = 0;
		}
		else if (lampButton->releaseEvent()){
			mode = MODE_EDIT;
			edit_item = 0;
		}
		else if (lamp_encoder_movement != 0){
			menu_item += lamp_encoder_movement;
			range_loop(&menu_item, 0, MENU_SIZE - 1);
		}
	}

	//Edit mode - edit a menu item
	else if (mode == MODE_EDIT){
		//Long press from lamp commits and returns to main screen
		if (lampButton->longPressEvent()){
			mode = MODE_TIME;
			edit_item = 0;
			eeprom_store();
		}
		else if (menu_item >= MENU_SET_ALARM_1 && menu_item <= MENU_SET_ALARM_3){
			//Find alarm index
			uint8_t alarm_index = menu_item;

			//Lamp encoder changes fields
			if (lamp_encoder_movement != 0){
				edit_item += (lamp_encoder_movement > 0 ? 1 : -1);
				range_loop(&edit_item, 0, 14);
			}
			//Music encoder increments / decrements fields
			else if (music_encoder_movement != 0){
				if (edit_item == 0){
					//Global enable flag
					alarm[alarm_index].enabled ^= _BV(7);
				}
				else if (edit_item == 1){
					int8_t h = alarm[alarm_index].hour + music_encoder_movement;
					range_loop(&h, 0, 23);
					alarm[alarm_index].hour = h;
				}
				else if (edit_item == 2){
					int8_t m = alarm[alarm_index].minute + music_encoder_movement;
					range_loop(&m, 0, 59);
					alarm[alarm_index].minute = m;
				}
				else if (edit_item == 3){
					int8_t b = alarm[alarm_index].lamp_brightness + music_encoder_movement;
					range_constrain(&b, 1, 100);
					alarm[alarm_index].lamp_brightness = b;
				}
				else if (edit_item == 4){
					int8_t s = alarm[alarm_index].lamp_speed + music_encoder_movement;
					range_constrain(&s, 0, 60);
					alarm[alarm_index].lamp_speed = s;
				}
				else if (edit_item == 5){
					int8_t v = alarm[alarm_index].music_volume + music_encoder_movement;
					range_constrain(&v, 0, 30);
					alarm[alarm_index].music_volume = v;
				}
				else if (edit_item == 6){
					int8_t s = alarm[alarm_index].music_speed + music_encoder_movement;
					range_constrain(&s, 0, 60);
					alarm[alarm_index].music_speed = s;
				}
				else if (edit_item == 7){
					int8_t f = alarm[alarm_index].music_folder + music_encoder_movement;
					range_constrain(&f, 1, 8);
					alarm[alarm_index].music_folder = f;
				}
				else {
					alarm[alarm_index].enabled ^= _BV(edit_item - 8);
				}
			}
		}
		else if (menu_item == MENU_SET_TIME){
			//Lamp encoder changes fields
			if (lamp_encoder_movement != 0){
				edit_item += lamp_encoder_movement;	//Here edit_item goes from 0 to 4: TIME_FIELD_X
				range_loop(&edit_item, 0, 4);
			}
			//Music encoder increments / decrements fields
			else if (music_encoder_movement != 0){
				if (edit_item == 0){		//Year
					now_tm.tm_year += music_encoder_movement;
				}
				else if (edit_item == 1){	//Month
					now_tm.tm_mon += music_encoder_movement;
					range_loop(&(now_tm.tm_mon), 0, 11);
					range_constrain(&now_tm.tm_mday, 0, time_days_in_month(&now_tm));
				}
				else if (edit_item == 2){	//Day
					now_tm.tm_mday += music_encoder_movement;
					range_loop(&now_tm.tm_mday, 1, time_days_in_month(&now_tm));
				}
				else if (edit_item == 3){	//Hour
					now_tm.tm_hour += music_encoder_movement;
					range_constrain(&now_tm.tm_hour, 0, 23);
				}
				else if (edit_item == 4){	//Minute
					now_tm.tm_sec = 0;		//Reset seconds to zero when changing minutes
					now_tm.tm_min += music_encoder_movement;
					range_constrain(&now_tm.tm_min, 0, 59);
				}

				//We convert to time_t and back to make sure all the fields are valid and to account for rollovers
				now = mktime(&now_tm);			//Calculate the time_t from the tm_t struct
				localtime_r(&now, &now_tm);		//Re-populate the tm_t struct from time_t
				calendar->set(&now_tm);			//Set the RTC with the updated tm_t struct
			}
		}
		else if (menu_item == MENU_CONFIG){
			//Lamp encoder changes fields
			if (lamp_encoder_movement != 0){
				edit_item += lamp_encoder_movement;	//Here edit_item goes from 0 to 10.  0 = music folder, 1-8 = file count / folder, 9 = DFU
				range_loop(&edit_item, 0, 9);
			}
			//Music encoder increments / decrements fields
			else if (music_encoder_movement != 0){
				if (edit_item == 0){
					int8_t f = config.music_folder + music_encoder_movement;
					range_constrain(&f, 1, 8);
					config.music_folder = f;
				}
				else if (edit_item <= 8){
					int8_t c = config.music_count[edit_item - 1] + music_encoder_movement;
					range_constrain(&c, 0, 99);
					config.music_count[edit_item - 1] = c;
				}
			}
			else if (edit_item == 9 && lampButton->releaseEvent()){
				bootloader_jump(BOOTLOADER_ATMEL);
			}
		}
	}

	//Handle timeouts - fade to dimmer display, and go back to time after certain timeouts depending on mode
	if (lamp_encoder_movement || music_encoder_movement || lampButton->getState() || musicButton->getState()){	//buttons press / turn
		last_input_time = now;
	}

	uint32_t seconds_since_last_input = now - last_input_time;

	//Turn off light and music (whether it was started by an alarm or a person) after 2 hours of no input
	if (seconds_since_last_input > (60 * 60 * 2)){
		if (light_state()){
			light_off();
		}
		if (music_is_playing()){
			music_stop();
		}
	}

	//Go back to time after timeouts
	if (mode == MODE_TIME && seconds_since_last_input > 15 && edit_item != EDIT_TIME_TIME){
		mode = MODE_TIME;		//Go back to time after 15 seconds without input in time mode
		edit_item = 0;
		eeprom_store();
	}
	else if (mode == MODE_MENU && seconds_since_last_input > 30){
		mode = MODE_TIME;		//Go back to time after 30 seconds without input in menu mode
		edit_item = 0;
	}
	else if (mode == MODE_EDIT && seconds_since_last_input > 90){
		mode = MODE_TIME;		//Go back to time after 90 seconds without input in edit mode
		edit_item = 0;
	}

	//Adjust the minimum brightness of the LED display based on ambient light levels.  Use hysteresis to keep from flickering back and forth.
	int8_t analog_value = (raw_analog_value >> 2) - 1;
	range_constrain(&analog_value, 0, 3);
	analog_value_running_average = ((analog_value_running_average * 15) + analog_value) / 16.0;
	if (min_brightness == 0 && analog_value_running_average >= 2.0){
		min_brightness = 1;
	}
	else if (min_brightness == 1 && analog_value_running_average <= 1.0){
		min_brightness = 0;
	}

	//Set the LED matrix brightness (0 - 15).  0 is further reduced in display.cpp by turning off the display for most of the duty cycle.
	if (seconds_since_last_input < 5){
		display_brightness = 15;
	}
	else if (seconds_since_last_input < 20){
		//Fade out to low brightness when not touching anything for a while.
		display_brightness = 20 - seconds_since_last_input;
		range_constrain(&display_brightness, min_brightness, 15);
	}
	else {
		display_brightness = min_brightness;
	}
}

alarm_t state_get_alarm(uint8_t index){
	if (index >= ALARM_COUNT){
		index = ALARM_COUNT - 1;
	}
	return alarm[index];
}

config_t state_get_config(){
	return config;
}

uint8_t state_get_mode(){
	return mode;
}

uint8_t state_get_menu_item(){
	return menu_item;
}

uint8_t state_get_edit_item(){
	return edit_item;
}

uint8_t state_get_display_brightness(){
	return display_brightness;
}

uint8_t state_get_lamp_brightness(){
	return lamp_brightness;
}

static void eeprom_store(){
	eeprom_update_block(alarm, EEPROM_CALIBRATION_OFFSET, sizeof(alarm));
	eeprom_update_block(&config, (void*) (EEPROM_CALIBRATION_OFFSET + sizeof(alarm)), sizeof(config));
}

static void eeprom_load(){
	//Read persisted variables
	eeprom_read_block(alarm, EEPROM_CALIBRATION_OFFSET, sizeof(alarm));
	eeprom_read_block(&config, (const void*) (EEPROM_CALIBRATION_OFFSET + sizeof(alarm)), sizeof(config));
}

static void update_time(time_t* time, tm_t* now_tm){
	calendar->get(now_tm);
	*time = mktime(now_tm);
}

static uint8_t get_update_display(){
	uint8_t result = update_display;
	update_display = 0;
	return result;
}

static void range_loop(int8_t* value, uint8_t min, uint8_t max){
	if (*value < min){
		*value = max;
	}
	else if (*value > max){
		*value = min;
	}
}

static void range_constrain(int8_t* value, uint8_t min, uint8_t max){
	if (*value < min){
		*value = min;
	}
	else if (*value > max){
		*value = max;
	}
}


ISR(INT6_vect){
	update_display = 1;
}

ISR(ADC_vect){
	//Read the 8 MSB of the value
	raw_analog_value = ADCH;

	//Start ADC again
	ADCSRA |= _BV(ADSC);
}
