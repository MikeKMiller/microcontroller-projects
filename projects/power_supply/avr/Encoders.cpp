#include "Encoders.h"

using namespace digitalcave;

static Buttons buttons(&PORTD, BUTTON_1 | BUTTON_2, 8, 8, 128, 0);
static volatile uint8_t encoder_movement = 0;
static volatile uint8_t held;
static volatile uint8_t released;

Encoders::Encoders(){
	//Enable pin change interrupts for encoders
	PCICR |= _BV(PCIE0);
	PCMSK0 |= 0x0F;							//Enable bits 0..3 for pin change interrupts
	PORTB |= _BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2) | _BV(PORTB3);
	
	//Enable timer 0 for button polling
	TCCR0A = 0x0;							//Normal mode
	TCCR0B |= _BV(CS02);					//256 prescaler
	TIMSK0 = _BV(TOIE0);					//Enable timer overflow interrupts
	
	sei();
}

uint8_t Encoders::get_encoder_movement(){
	uint8_t result = encoder_movement;
	encoder_movement = 0;
	return result;
}

uint8_t Encoders::get_held(){
	uint8_t result = held;
	held = 0x00;
	return result;
}

uint8_t Encoders::get_released(){
	uint8_t result = released;
	released = 0x00;
	return result;
}

ISR(TIMER0_OVF_vect){
	buttons.sample();
	held |= buttons.held();
	released |= buttons.released();
}

ISR(PCINT0_vect){
	static uint8_t encoder1 = 0x00;
	static uint8_t encoder2 = 0x00;
	
	encoder1 = ((encoder1 << 2) | (PINB & 0x03)) & 0x0F;
	encoder2 = ((encoder2 << 2) | ((PINB >> 2) & 0x03)) & 0x0F;
	
	//The encoders I am using now output a full sequence for one step, so we only look for one
	// of the sub-sequences to avoid 4x faster movement than we want.  Change this as needed.
	switch(encoder1){
		case 0x01:
		//case 0x07:
		//case 0x08:
		//case 0x0E:
			encoder_movement = ENCODER1_CLOCKWISE;
			break;
		case 0x02:
		//case 0x04:
		//case 0x0B:
		//case 0x0D:
			encoder_movement = ENCODER1_COUNTER_CLOCKWISE;
			break;
	}
	
	switch(encoder2){
		case 0x01:
		//case 0x07:
		//case 0x08:
		//case 0x0E:
			encoder_movement = ENCODER2_CLOCKWISE;
			break;
		case 0x02:
		//case 0x04:
		//case 0x0B:
		//case 0x0D:
			encoder_movement = ENCODER2_COUNTER_CLOCKWISE;
			break;
	}
}