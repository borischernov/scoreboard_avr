/*
 * leds.c
 *
 *  Created on: 16/6/2016
 *      Author: boris
 *
 *  LPD8806 LED strip control
 *
 * 	Color in buffer is encoded as 8 bits / element, RRRGGGBB
 *  Order of colors in the strip is BRG
 */

#include <avr/io.h>

#include "leds.h"
#include "comms.h"

volatile uint8_t leds_buffer[LEDS_COUNT];
extern volatile packet rx_buf;

void leds_setup(void)
{
	uint8_t c;

	DDRB |= _BV(PB4)| _BV(PB5) | _BV(PB7);	 	// Set MOSI , SCK , and SS output
	SPCR = _BV(SPE)| _BV(MSTR);					// Enable SPI, Master, set clock rate fck/4

	for(c = 0; c < LEDS_COUNT; c++)
		leds_buffer[c] = 0;

	leds_buffer_send();
}

void spi_send(unsigned char byte)
{
	SPDR = byte;					//	Load byte to Data register
	while (!(SPSR & _BV(SPIF))); 	//	Wait for transmission complete
}

void leds_buffer_send(void) {
	uint8_t c, b;

	for(c = 0; c < LEDS_COUNT; c++) {
		b = leds_buffer[c];
		spi_send(0x80 | (b << 5));				// B
		spi_send(0x80 | ((b >> 1) & 0x70));		// R
		spi_send(0x80 | ((b << 2) & 0x70));		// G
	}
	for(c = 0; c < LEDS_COUNT; c++)
		spi_send(0);
}

// Write display buffer
// Params: <offset> <data>
// Response: none
uint8_t cmd_display(void) {
	uint8_t offset = rx_buf.pkt.data0;
	uint8_t length = rx_buf.pkt.data_length - 1;

	if (offset > LEDS_COUNT - 1 || offset + length > LEDS_COUNT)
		return 0;

	for (int c = 0; c < length; c++, offset++)
		leds_buffer[offset] = rx_buf.bytes[c + PKT_DATA_OFFSET + 1];

	rx_buf.pkt.data_length = 0;

	return 1;
}

// Read display buffer
// Params: <offset> <length>
// Response: <data>
uint8_t cmd_read(void) {
	uint8_t offset = rx_buf.pkt.data0;
	uint8_t length = rx_buf.pkt.data1;

	if (offset > LEDS_COUNT - 1 || offset + length > LEDS_COUNT)
		return 0;

	for (int c = 0; c < length; c++, offset++)
		rx_buf.bytes[c + PKT_DATA_OFFSET] = leds_buffer[offset];

	rx_buf.pkt.data_length = length;

	return 1;
}

// TODO Set offsets
const uint8_t digit_offsets[] = { 65, 58, 50, 43 };
const uint8_t lcd_digits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

extern uint8_t timer_color;

// Sets timer digit in LEDs output buffer
void set_timer_digit(uint8_t num, uint8_t digit) {
	uint8_t c, m;
	for (c = digit_offsets[num], m = lcd_digits[digit]; c < digit_offsets[num] + 7; c++, m >>= 1)
		leds_buffer[c] = (m & 1) ? timer_color : 0;
}

