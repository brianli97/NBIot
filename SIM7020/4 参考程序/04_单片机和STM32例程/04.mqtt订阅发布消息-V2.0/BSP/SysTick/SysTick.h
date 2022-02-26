#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"

void SysTick_Init_Config(void);
void delay_ms(u32 nms);
void Delay_nS(u32 ns);

#endif 
