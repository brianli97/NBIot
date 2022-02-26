#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"
#include "Led.h"

void timer2_init_config(void);
void start_timer(uint16_t t);
void stop_timer(void);

#endif
