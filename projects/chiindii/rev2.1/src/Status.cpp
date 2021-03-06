#include "Status.h"

#include "stm32f4xx_hal.h"

using namespace digitalcave;

Status::Status() :
	lastTime(0),
	status(0)
{
	// *(&LED_PORT - 1) |= RED | GREEN | BLUE;
	// LED_PORT &= ~(RED | GREEN | BLUE);	// turn on active low LED (white light on reset)
}

void Status::poll(uint32_t time) {
	// Turn off active low LED (set pins high)
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);		//Red
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);		//Green
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);		//Blue

	uint32_t currentPeriod = time - lastTime;
	if (currentPeriod < 250) {
		if (status & 0x1) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	}
	else if (currentPeriod < 500) {
		if (status & 0x2) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
	}
	else if (currentPeriod < 750) {
		if (status & 0x4) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	}
	else {
		if (status == 0x0) {
			//Turn on white light
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		};
	}

	if (currentPeriod > 1000) {
		lastTime = time;
	}
}

void Status::batteryLow() {
	status |= 0x1;
}
void Status::batteryOK() {
	status &= ~0x1;
}
void Status::disarmed() {
	status |= 0x2;
}
void Status::armed() {
	status &= ~0x2;
}
void Status::commInterrupt() {
	status |= 0x4;
}
void Status::commOK() {
	status &= ~0x4;
}
