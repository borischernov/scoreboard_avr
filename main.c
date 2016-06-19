/*
 * main.c
 *
 *  Created on: 15/6/2016
 *      Author: boris
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "comms.h"
#include "leds.h"
#include "timer.h"
#include "utils.h"

extern volatile uint8_t leds_buffer[];

int main(void) {

	power_setup();
	diagnostics();
	timers_setup();
	uart_setup();
	leds_setup();

	sei();

	beep(1000, 4);

	for(;;) {
		leds_buffer_send();
	}
}
