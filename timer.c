/*
 * timer.c
 *
 *  Created on: 18/6/2016
 *      Author: boris
 *
 *   RTC-related functions
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "comms.h"
#include "leds.h"

extern volatile uint8_t leds_buffer[];
extern volatile packet rx_buf;

volatile uint8_t timer_digits[4];
volatile uint8_t timer_state = 0;

#define TIMER_RUNNING		1
#define TIMER_COUNTDOWN		2
#define TIMER_BEEP			4

const uint8_t timer_limits[4] = {9, 5, 9, 9};

volatile uint8_t	tick = 0;

volatile uint8_t	sound_ticks = 0;

#define T0CLK		(F_CPU / 256UL)

void timer_inc(void);
void timer_dec(void);
uint8_t timer_is_zero(void);
void nosound(void);
void beep(int freq, uint8_t duration);

ISR(TIMER2_OVF_vect) {

	if (sound_ticks) {
		sound_ticks--;
		if (!sound_ticks)
			nosound();
	}

	tick = (tick + 1) & 0x03;
	if (tick)
		return;

	if (timer_state & TIMER_RUNNING) {
		if (timer_state & TIMER_COUNTDOWN) {
			timer_dec();
			if (timer_is_zero()) {
				timer_state &= ~TIMER_RUNNING;
				if (timer_state & TIMER_BEEP) {
					beep(1000, 6);
				}
			}
		} else {
			timer_inc();
		}
	}
}

void timers_setup(void) {
	DDRB |= _BV(PB3);		// OC0 - Output
	PORTB |= _BV(PB3);	// Set to 0

	// TC2 shall overflow 4 times per second (async timing from ext 32768 Hz crystal)

    TIMSK = 0; 													// Disable timer2 interrupts
	ASSR = _BV(AS2); 											// Enable asynchronous mode
	TCNT2 = 0; 													// Reset counter
	TCCR2 = _BV(CS21) | _BV(CS20);								// CLKt2s / 32
    while (ASSR & (_BV(TCN2UB) | _BV(TCR2UB) | _BV(OCR2UB))); 	// Wait for Busy flags to clear
    TIFR = _BV(TOV2);											// Clear interrupt flag
    TIMSK = _BV(TOIE2);											// Enable overflow interrupt
}

void timer_dec(void) {
	uint8_t c;

	for (c = 0; c < 4; c++) {
		timer_digits[c]--;
		set_timer_digit(c, timer_digits[c]);
		if (timer_digits[c] <= timer_limits[c])
			return;
		timer_digits[c] = timer_limits[c];
	}
}

void timer_inc(void) {
	uint8_t c;

	for (c = 0; c < 4; c++) {
		timer_digits[c]++;
		set_timer_digit(c, timer_digits[c]);
		if (timer_digits[c] <= timer_limits[c])
			return;
		timer_digits[c] = 0;
	}
}

uint8_t timer_is_zero(void) {
	uint8_t c;

	for (c = 0; c < 4; c++)
		if (timer_digits[c])
			return 0;

	return 1;
}

void beep(int freq, uint8_t duration) {
	sound_ticks = duration;

	TIMSK &= ~(_BV(OCIE0) | _BV(TOIE0));						// Disable Timer0 interrupts
	OCR0 = (uint8_t)(T0CLK / (2 * freq) - 1);					// Set output compare value
	TCCR0 = _BV(WGM01) | _BV(COM00) | _BV(CS02);				// CTC mode, toggle OC0 on match, T0CLK = F_CPU / 256
}

void nosound(void) {
	TCCR0 &= ~(_BV(COM00) | _BV(COM01) | _BV(CS00) | _BV(CS01) | _BV(CS02));
}

// Set timer value
// Params: <digit> <digit> <digit> <digit>
// Response: none
uint8_t cmd_set_timer_value(void) {
	uint8_t c;

	if (rx_buf.pkt.data_length != 4)
		return 0;

	for (c = 0; c < 4; c++)
		if (rx_buf.bytes[PKT_DATA_OFFSET + c] > timer_limits[c])
			return 0;

	for (c = 0; c < 4; c++) {
		timer_digits[c] = rx_buf.bytes[PKT_DATA_OFFSET + c];
		set_timer_digit(c, timer_digits[c]);
	}

	rx_buf.pkt.data_length = 0;

	return 1;
}

// Set timer state
// Params: <state>
// Response: none
uint8_t cmd_set_timer_state(void) {
	if (rx_buf.pkt.data_length != 1)
		return 0;

	TCNT2 = 0; 													// Reset counter
	tick = 0;													// Reset prescaler
	timer_state = rx_buf.pkt.data0;

	rx_buf.pkt.data_length = 0;

	return 1;
}

// Get timer state and value
// Params: none
// Response: <state> <digit> <digit> <digit> <digit>
uint8_t cmd_get_timer(void) {
	uint8_t c;

	rx_buf.bytes[PKT_DATA_OFFSET] = timer_state;

	for (c = 0; c < 4; c++)
		rx_buf.bytes[PKT_DATA_OFFSET + 1 + c] = timer_digits[c];

	rx_buf.pkt.data_length = 5;

	return 1;
}

// Just beep
// Params: <frequency, Hz / 100> <druarion, quarters of sec>
// Response: none
uint8_t cmd_beep(void) {
	if (rx_buf.pkt.data_length != 2)
		return 0;

	beep((int)rx_buf.pkt.data0 * 100, rx_buf.pkt.data1);

	rx_buf.pkt.data_length = 0;

	return 1;
}
