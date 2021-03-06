#include "timer.h"
#include "usart.h"
#include <stdio.h>


extern u8 send_cmd_start  ;  /* 寻找回复的起始符 */
extern u8 send_cmd_end    ;  /* 寻找回复的结束符 */
extern u8 send_cmd_over   ;  /* 发送命令正常返回 */
extern u8 send_cmd_timeout;  /* 发送命令在固定的时间内没有返回,发生超时 */

/*******************************************************************************
* 函数名  : timer2_init_config
* 描述    : Timer2初始化配置
* 输入    : 延时t ms
* 输出    : 无
* 返回    : 无 
* 说明    : 1s延时
*******************************************************************************/
void timer2_init_config(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);		//使能Timer2时钟
	
	TIM_TimeBaseStructure.TIM_Period = 9999;					//设置在下一个更新事件装入活动的自动重装载寄存器周期的值(计数到10000为1s)
	TIM_TimeBaseStructure.TIM_Prescaler = 7199;					//设置用来作为TIMx时钟频率除数的预分频值(10KHz的计数频率)
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//设置时钟分割:TDTS = TIM_CKD_DIV1
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);				//根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

	/*中断优先级NVIC设置*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能IRQ通道
	NVIC_Init(&NVIC_InitStructure); 							//初始化NVIC寄存器
	 
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 				//使能TIM2指定的中断
    TIM_Cmd(TIM2, DISABLE);
}	

void start_timer(uint16_t t)
{   
    TIM_SetCounter(TIM2,0); 
    TIM_SetAutoreload(TIM2, t * 10 - 1);
    TIM_SelectOnePulseMode(TIM2, TIM_OPMode_Single);
    
    //printf("arr:%d,count:%d\n", TIM2->ARR, TIM2->CNT);
    
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 				    //使能TIM2指定的中断
    TIM_Cmd(TIM2, ENABLE);
}

void stop_timer(void)
{
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_Cmd(TIM2, DISABLE);     
}
