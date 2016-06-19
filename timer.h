/*
 * timer.h
 *
 *  Created on: 18/6/2016
 *      Author: boris
 */

#ifndef TIMER_H_
#define TIMER_H_

void timers_setup(void);

uint8_t cmd_set_timer_value(void);
uint8_t cmd_set_timer_state(void);
uint8_t cmd_get_timer(void);
uint8_t cmd_set_timer_params(void);
uint8_t cmd_beep(void);

void beep(uint16_t freq, uint8_t duration);

#endif /* TIMER_H_ */
