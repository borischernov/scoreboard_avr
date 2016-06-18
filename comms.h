/*
 * comms.h
 *
 *  Created on: 15/6/2016
 *      Author: boris
 */

#ifndef COMMS_H_
#define COMMS_H_

#define VERSION 1

typedef union {
	uint8_t bytes[255];
	struct pkt {
		uint8_t header;
		uint8_t seq_num;
		uint8_t command;
		uint8_t data_length;
		uint8_t data0;
		uint8_t data1;
	} pkt;
} packet;

#define PKT_DATA_OFFSET	4

void uart_setup(void);
void uart_send(uint8_t *ptr, uint8_t cnt);

#endif /* COMMS_H_ */
