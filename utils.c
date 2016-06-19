/*
 * utils.c
 *
 *  Created on: 18/6/2016
 *      Author: boris
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "comms.h"
#include "leds.h"

extern volatile packet rx_buf;

#define POWER_PIN	PB0

volatile uint8_t	spi_bit;
volatile uint8_t	spi_buf;

uint8_t	diag_code;

#define DIAG_NO_CLOCK		1
#define DIAG_NO_DATA		2
#define DIAG_STRIP_SHORT	4

#define SW_SPI_MOSI	PB1
#define SW_SPI_CLK	PB2		// INT2

// Set up power control output
void power_setup(void) {
	DDRB |= _BV(POWER_PIN);		// Set PB0 as output
	PORTB &= ~ _BV(POWER_PIN);	// Power off
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
	rx_buf.bytes[PKT_DATA_OFFSET + 2] = (PORTB & _BV(POWER_PIN)) ? 1 : 0;
	rx_buf.bytes[PKT_DATA_OFFSET + 3] = diag_code;

	rx_buf.pkt.data_length = 4;

	return 1;
}

// Control power
// Params: <state>
// Response: none
uint8_t cmd_power(void) {
	if (rx_buf.pkt.data_length != 1)
		return 0;

	if (rx_buf.pkt.data0) {
		PORTB |= _BV(POWER_PIN);
	} else {
		PORTB &= ~ _BV(POWER_PIN);
	}

	rx_buf.pkt.data_length = 0;

	return 1;
}

// INT2 is clock for s/w SPI
ISR(INT2_vect) {
	spi_bit++;

	spi_buf <<= 1;
	if (PINB & _BV(SW_SPI_MOSI))
		spi_buf |= 1;
}

void diagnostics(void) {
	uint8_t c;

	// Set up SPI with low speed
	DDRB |= _BV(PB4)| _BV(PB5) | _BV(PB7);	 			// Set MOSI , SCK , and SS output
	SPCR = _BV(SPE)| _BV(MSTR) | _BV(SPR1) | _BV(SPR0);	// Enable SPI, Master, set clock rate fck/128

	// Switch power on
	PORTB |= _BV(POWER_PIN);
	for (c = 0; c < 250; c++) {
		_delay_ms(2);
	}

	// Set up software spi
	DDRB &= ~(_BV(SW_SPI_MOSI) | _BV(SW_SPI_CLK));		// MOSI, CLK are inputs
	MCUCSR |= _BV(ISC2);								// Interrupt on rising edge of INT2
	GICR |= _BV(INT2);									// Enable INT2 interrupt
	sei();

	// Push zeros
	for (c = 0; c < LEDS_COUNT; c++) {
		spi_send(0);
	}

	_delay_ms(1);
	spi_buf = 0;
	diag_code = 0;

	// Push A5 to the strip 3 * leds + 1 times
	for (c = 0; c <= LEDS_COUNT * 3; c++) {
		spi_bit = 0;
		spi_send(0xA5);
		_delay_us(10);
		if (spi_bit != 8) {
			diag_code = DIAG_NO_CLOCK;
			break;
		}
		if (c < LEDS_COUNT && spi_buf) {
			diag_code = spi_buf == 0xA5 ? DIAG_STRIP_SHORT : DIAG_NO_DATA;
			break;
		}
	}
	if (!diag_code && spi_buf != 0xA5) {
		diag_code = DIAG_NO_DATA;
	}

	GICR &= ~_BV(INT2);									// Disable INT2 interrupt
	cli();

	// Switch power off
	PORTB &= ~_BV(POWER_PIN);
}
