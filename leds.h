/*
 * leds.h
 *
 *  Created on: 16/6/2016
 *      Author: boris
 */

#ifndef LEDS_H_
#define LEDS_H_

#define LEDS_COUNT 2

void leds_setup(void);
void leds_buffer_send(void);
void spi_send(unsigned char byte);

uint8_t cmd_read(void);
uint8_t cmd_display(void);

void set_timer_digit(uint8_t num, uint8_t digit);

#endif /* LEDS_H_ */
