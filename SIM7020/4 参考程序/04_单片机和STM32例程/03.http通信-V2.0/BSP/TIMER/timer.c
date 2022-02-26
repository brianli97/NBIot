#include "timer.h"
#include "usart.h"
#include <stdio.h>


extern u8 send_cmd_start  ;  /* Ѱ�һظ�����ʼ�� */
extern u8 send_cmd_end    ;  /* Ѱ�һظ��Ľ����� */
extern u8 send_cmd_over   ;  /* ���������������� */
extern u8 send_cmd_timeout;  /* ���������ڹ̶���ʱ����û�з���,������ʱ */

/*******************************************************************************
* ������  : timer2_init_config
* ����    : Timer2��ʼ������
* ����    : ��ʱt ms
* ���    : ��
* ����    : �� 
* ˵��    : 1s��ʱ
*******************************************************************************/
void timer2_init_config(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);		//ʹ��Timer2ʱ��
	
	TIM_TimeBaseStructure.TIM_Period = 9999;					//��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ(������10000Ϊ1s)
	TIM_TimeBaseStructure.TIM_Prescaler = 7199;					//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ(10KHz�ļ���Ƶ��)
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//����ʱ�ӷָ�:TDTS = TIM_CKD_DIV1
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);				//����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

	/*�ж����ȼ�NVIC����*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//��ռ���ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//�����ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��IRQͨ��
	NVIC_Init(&NVIC_InitStructure); 							//��ʼ��NVIC�Ĵ���
	 
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); 				//ʹ��TIM2ָ�����ж�
    TIM_Cmd(TIM2, DISABLE);
}	

void start_timer(uint16_t t)
{   
    TIM_SetCounter(TIM2,0); 
    TIM_SetAutoreload(TIM2, t * 10 - 1);
    TIM_SelectOnePulseMode(TIM2, TIM_OPMode_Single);
    
    //printf("arr:%d,count:%d\n", TIM2->ARR, TIM2->CNT);
    
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 				    //ʹ��TIM2ָ�����ж�
    TIM_Cmd(TIM2, ENABLE);
}

void stop_timer(void)
{
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_Cmd(TIM2, DISABLE);     
}
