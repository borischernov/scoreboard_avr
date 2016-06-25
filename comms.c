/*
 * comms.c
 *
 *  Created on: 15/6/2016
 *      Author: boris
 *
 *      Handling communications with control tablet
 *
 *      Packet Format:
 *
 *      0x5A <seq_num> <command> <data_length> <data> <crc>
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

#include "comms.h"
#include "leds.h"
#include "timer.h"
#include "utils.h"

#define UART_BAUD	9600
#define UBBR_CONST	(F_CPU / (16UL * UART_BAUD) - 1)

#define PACKET_HEADER 		0x5A
#define MAX_PACKET_LENGTH	255
#define MAX_DATA_LENGTH		(MAX_PACKET_LENGTH - sizeof(struct pkt))
#define PACKET_LENGTH(dl)	(dl + 5)

#define ACK					0x01
#define NACK				0x00

#define CMD_PING				0x01
#define CMD_DISPLAY				0x02
#define CMD_READ				0x03
#define CMD_SET_TIMER_STATE		0x04
#define CMD_SET_TIMER_VALUE 	0x05
#define CMD_GET_TIMER			0x06
#define CMD_BEEP				0x07
#define CMD_POWER				0x08
#define CMD_SET_TIMER_PARAMS 	0x09
#define CMD_SET_CLOCK_VALUE 	0x0A
#define CMD_GET_CLOCK_VALUE 	0x0B

volatile uint8_t *tx_ptr;
volatile uint8_t tx_cnt = 0;

volatile uint8_t rx_cnt = 0;
volatile packet rx_buf;

_Static_assert (offsetof(struct pkt, data0) == 4, "Improper packaging of rx_buf structure");

uint8_t crc_ok(void);
void process_packet();

// Tx Data Register Empty Interrupt
ISR(USART_UDRE_vect){
	if (tx_cnt) {
		UDR = *tx_ptr++;
		tx_cnt--;
	} else {
		UCSRB &= ~_BV(UDRIE);
	}
}

// Rx Complete Interrupt
ISR(USART_RXC_vect){
   rx_buf.bytes[rx_cnt++] = UDR;
   if (rx_cnt == 1) {											// Just got header
	   if (rx_buf.pkt.header  != PACKET_HEADER)
		   rx_cnt = 0;
   } else if (rx_cnt == offsetof(struct pkt, data0)) {		// Just got data length
	   if (rx_buf.pkt.data_length > MAX_DATA_LENGTH)
		   rx_buf.pkt.data_length = MAX_DATA_LENGTH;
   } else if (rx_cnt > offsetof(struct pkt, data_length) && rx_cnt == PACKET_LENGTH(rx_buf.pkt.data_length)) { // Got whole packet
	   if (crc_ok())
		   process_packet();
	   rx_cnt = 0;
   }
}

uint8_t crc_ok(void) {
	uint8_t crc, c;
	for (crc = 0, c = 0; c < PACKET_LENGTH(rx_buf.pkt.data_length); c++)
		crc += rx_buf.bytes[c];
	return crc == 0xFF;
}

uint8_t calc_crc(void) {
	uint8_t crc, c;
	for (crc = 0, c = 0; c < PACKET_LENGTH(rx_buf.pkt.data_length) - 1; c++)
		crc += rx_buf.bytes[c];
	return 0xFF - crc;
}

#define NUM_COMMANDS 11

void process_packet() {
	uint8_t cmd_ok;

	uint8_t (*commands[NUM_COMMANDS])(void) = {
			cmd_ping,
			cmd_display,
			cmd_read,
			cmd_set_timer_state,
			cmd_set_timer_value,
			cmd_get_timer,
			cmd_beep,
			cmd_power,
			cmd_set_timer_params,
			cmd_set_clock_value,
			cmd_get_clock_value
	};

	cmd_ok = (rx_buf.pkt.command == 0 || rx_buf.pkt.command > NUM_COMMANDS) ? 0 : commands[rx_buf.pkt.command - 1]();

	if (!cmd_ok)
		rx_buf.pkt.data_length = 0;

	rx_buf.pkt.command = cmd_ok ? ACK : NACK;
	rx_buf.bytes[PACKET_LENGTH(rx_buf.pkt.data_length) - 1] = calc_crc();

	uart_send(rx_buf.bytes, PACKET_LENGTH(rx_buf.pkt.data_length));
}

void uart_send(uint8_t *ptr, uint8_t cnt) {
	while (tx_cnt);
	tx_ptr = ptr;
	tx_cnt = cnt;
	UCSRB |= _BV(UDRIE);	// Enable UDRE interrupt
}

void uart_setup(void) {

	DDRD |= _BV(PD1);		// TXD - Output

	// Configure baud rate generator
	UBRRL = UBBR_CONST;
	UBRRH = (UBBR_CONST >> 8);

	UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE);  	// Enable transmitter, receiver and rx complete interrupt
	UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0); 	// 8-bit data

	rx_cnt = 0;
}
