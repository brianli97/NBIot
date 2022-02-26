#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"

#define USART1_REC_MAXLEN 200	//���������ݳ���

void USART1_Init_Config(u32 bound);
void UART1_SendString(char* s);
void USART2_Init_Config(u32 bound);
void UART2_SendString(char* s);
void UART1_SendLR(void);
void UART2_SendLR(void);
void UART2_Send_Command(char* s);
int UART2_Send_AT_Command(char *b,char *a,u8 wait_time,u16 interval_time);

//����1���ͻس�����
#define UART1_SendLR() UART1_SendString("\r\n")

//����2���ͻس�����
#define UART2_SendLR() UART2_SendString("\r\n")
//											
#endif


