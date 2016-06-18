/*
 * utils.c
 *
 *  Created on: 18/6/2016
 *      Author: boris
 */

#include <avr/io.h>

#include "comms.h"
#include "leds.h"

extern volatile packet rx_buf;

// Set up power control output
void power_setup(void) {
	DDRB |= _BV(PB0);		// Set PB0 as output
	PORTB &= ~ _BV(PB0);	// Power off
}

// Return system information
// Params: none
// Response: <version> <LED's count> <power state>
uint8_t cmd_ping(void) {
	if (rx_buf.pkt.data_length != 0)
		return 0;

	rx_buf.pkt.data_length = 2;
	rx_buf.bytes[PKT_DATA_OFFSET] = VERSION;
	rx_buf.bytes[PKT_DATA_OFFSET + 1] = LEDS_COUNT;
	rx_buf.bytes[PKT_DATA_OFFSET + 2] = (PORTB & _BV(PB0)) ? 1 : 0;

	rx_buf.pkt.data_length = 3;

	return 1;
}

// Control power
// Params: <state>
// Response: none
uint8_t cmd_power(void) {
	if (rx_buf.pkt.data_length != 1)
		return 0;

	if (rx_buf.pkt.data0) {
		PORTB |= _BV(PB0);
	} else {
		PORTB &= ~ _BV(PB0);
	}

	rx_buf.pkt.data_length = 0;

	return 1;
}
