/**********************************************************************************
   STM32F103C8T6  	 
 * Ӳ������˵��
   ʹ�õ�Ƭ����2��NB-7020ģ��ͨ��  ע��ʹ�ô���2���Ա������غ�ͨ�Ų����ͻ
   STM32 �� NB-7020ģ����ӽ��߷���:  
     
   PA3 (RXD2)->TX       
   PA2 (TXD2)->RX         
   GND	   ->GND          
 
   PA9(TXD1)--->���������Ϣ�˿�
   PA10(RXD1)-->����������Ϣ�˿�

��ƣ�
(1)LED0-7���õ�����Ϊ:PB4-7 PB12-15
(2)KEY0-3���õ�����Ϊ:PA4-7
(3)ͨ������2�·��ַ�1��2��3������Ӧ�ķ��أ����ӷ��������������ݣ��ر����ӡ�

���Ƽ���:�Ź� Ⱥ��:908159506

**�������˼·��
**(1)����������ͱ����յ���������л��������
**(2)����Ҫ�ȵȵ�ģ��ע��������,�Ž�����һ������ķ���,����һֱ�ȴ���
**********************************************************************************/
#include <stdio.h>
#include "stm32f10x.h"
#include "usart.h"
#include "Led.h"
#include "SysTick.h"
#include "timer.h"
#include "string.h"
#include "key.h"
#include "guoyun_nb_iot.h"
#include "user_func.h"

/*************  1.��������������Ҫ�ı���	**************/
char cmd_send_rcv_buf[SEND_RCV_BUF];       /* �������������,�յ�����Ļظ�buffer */
u16 send_rcv_ptr = 0;
cmd_result_sta_t  master_result_sta;       /* �������������ʼ���ģ��ظ����ַ��� */
cmd_send_result_t master_send_cmd_result;  /* ��������������Ƿ�ɹ����� */ 

/*************  2.��������������Ҫ�ı���	**************/
char rcv_cmd_buf[RCV_BUFFER];              /* �������ܵ������buffer */
int rcv_cmd_ptr = 0;
static int rcv_content_count = 0;
cmd_result_sta_t  slave_result_sta;       /* �����������ʼ���ģ��ظ����ַ��� */
cmd_rcv_result_t  slave_rcv_cmd_result;   /* ��������������Ƿ�ɹ�����(���û������URC) */ 

/*************  3.�û�����������Ҫ�ı���	**************/
char user_debug_cmd = 0;                   /* �û�ͨ������1�·����ַ� ���Ʋ�ͬ�Ĺ��� */

/*************  5.������Ϣ��Ҫ�ı���	**************/
static int record_count = 0;
char debug_buf[1024];  //���ڴ��ڽ��մ洢,������������Ϣ
int debug_count = 0;
int cmd_type = 0;

/*************  6.ϵͳ״̬��Ҫ�ı���	**************/
cur_state_t system_state = STA_IDE;
char send_at_buffer[100];  //���ڹ��췢�͵�����

/*************  7.���ܶ�����Ҫ�ı���	**************/
int socket = -1;           /* ֻ��¼һ������ */

extern rcv_cmd_sys_t http_cmd[];

rcv_cmd_sys_t *user_cmd;
int user_cmd_count;

#define USER_CMD     http_cmd
#define USER_CMD_PTR &http_cmd[0]
#define RCV_CMD_LEN_POS (2)  /* ��������ȵĶ���λ�� */

/*******************************************************************************
* ������ : main 
* ����   : ������
* ����   : 
* ���   : 
* ����   : 
* ע��   : ����2������guoyun_nb_iotģ��ͨ�ţ�����1���ڴ��ڵ��ԣ�
*******************************************************************************/
int main(void)
{
	u8  run_led_flag = 0;
	int ret;
    int i;
    
    char buff[1024];
    int str_len;

    
	SysTick_Init_Config();   //ϵͳ�δ�ʱ�ӳ�ʼ��
	GPIO_Config();           //GPIO��ʼ��
	Key_GPIO_Config();
	USART2_Init_Config(115200);  //����2��ʼ��
	USART1_Init_Config(115200);//UART1�������ڵ�����Ϣ
    
    delay_ms(2000);
	printf("ϵͳ����.......................\r\n");
    
	timer2_init_config();        //��ʱ��2��ʼ��,���ڵȴ����ʱ    

	#if defined(PUBLIC_VERSION)
	for(i = 0;i < STABLE_TIMES;i++)  //�����ʱ����Ϊ�˵ȴ�ģ���ϵ���ȶ� 
	{
		delay_ms(1000);
	}
	#endif
	
    /* 1.�ȴ�ͨ������ */
    printf("1.����ͨ���Ƿ�����?\r\n");
      
    while ((ret = check_communication()) != 0)
    {
        printf("ͨ��ʧ��:%d\n", ret);
        
        delay_ms(1000);
    }
    printf("  ͨ��������\r\n");
    /* 2.�رջ��� */
    printf("2.�رջ���\r\n");
    while (close_ate() != 0)
    {
        printf("  �ر�ʧ��!\r\n");
        delay_ms(1000);
    }
    printf("  �رճɹ�!\r\n");
    /* 3.�ȴ�ע�������� */
    printf("3.�ȴ�ע������\r\n");
    while (check_register_network() != 0)
    {
        printf("  ע��ʧ��!\r\n");
        delay_ms(1000);
    }
    printf("  ע��ɹ�!\r\n");
    
    init_user_cmd();
    
    user_cmd       = USER_CMD_PTR;
    user_cmd_count = USER_CMD_NUM;
    
	while(1)
	{ 
        for (i = 0;i < user_cmd_count; i++)
        {
            if (http_cmd[i].sta == RCV_SUCC)
            {
                printf("cmd:%s,receive succ\n", http_cmd[i].name);
                printf("buff:%s\n", http_cmd[i].buff);
                
                if (!strncmp(http_cmd[i].buff, "+CHTTPNMIC", strlen("+CHTTPNMIC")))
                {
                    /* +CHTTPNMIC: 4,0,34,34,7b0a2020226f726967696e223a20223232332e3130342e3235352e313239220a7d0 */
                    str_len = strlen(http_cmd[i].buff);
                    *(http_cmd[i].buff + str_len - 1) = '\0';
                    *(http_cmd[i].buff + str_len - 2) = '\0';
                    str_len = strlen((http_cmd[i].buff + strlen("+CHTTPNMIC: 1,0,34,34,")));
                    printf("str_len:%d\n", str_len);
                    hex_to_str((http_cmd[i].buff + strlen("+CHTTPNMIC: 1,0,34,34,")), buff, str_len / 2);
                    
                    printf("ip(json):\n%s\n", buff);
                }
            }
            else if (http_cmd[i].sta == RCV_TIMEOUT)
            {
                printf("cmd:%s,receive timeout\n", http_cmd[i].name);
            }
            else if (http_cmd[i].sta == RCV_OVERFLOW)
            {
                printf("cmd:%s,receive overflow\n", http_cmd[i].name);
            }
            else if (http_cmd[i].sta == RCV_ERROR)
            {
                printf("cmd:%s,receive error:%d\n", http_cmd[i].name, http_cmd[i].error_code);
            }
            
            if (http_cmd[i].sta != RCV_INIT && http_cmd[i].sta != RCV_RUNNING)
            {
                http_cmd[i].sta = RCV_INIT;
            }
        }

		if (slave_rcv_cmd_result != RCV_CMD_INIT)
		{
			slave_rcv_cmd_result = RCV_CMD_INIT;
			
		}
		
        if (user_debug_cmd == '1')
        {
			printf("�յ�1:%s\r\n", rcv_cmd_buf);    
            user_debug_cmd = 0;
            
            ret = connect_to_http_server("http://httpbin.org");            
			printf("%s%d\r\n","ret = ", ret);
        }
        
        if (user_debug_cmd == '2')
        {
			printf("�յ�2\r\n");    
            user_debug_cmd = 0;
            ret = send_data_to_http_server(socket, "/ip");            
			printf("%s%d\r\n","ret = ", ret);
            
        }
        
        if (user_debug_cmd == '3')
        {
			printf("�յ�3\r\n");    
            user_debug_cmd = 0;
            ret = close_socket(socket);            
			printf("%s%d\r\n","ret = ", ret);
        }
        
        if (user_debug_cmd == '4')
        {
			printf("�յ�4, reset now\r\n");    
            user_debug_cmd = 0;
            ret = UART2_Send_AT_Command("AT+CRESET", "OK", 1, 5000); 
            if (ret != 0)
            {
                printf("reset return OK timeout!\n");
                printf("%s", cmd_send_rcv_buf);
            } 
			printf("reset:%s%d\r\n","ret = ", ret);
        }
        
        if (user_debug_cmd == '5')
        {
			printf("�յ�5, test communicate\r\n");    
            user_debug_cmd = 0;
            ret = UART2_Send_AT_Command("AT", "OK", 1, 1000); 
            if (ret != 0)
            {
                printf("test return OK timeout!\n");
                printf("%s", cmd_send_rcv_buf);
            } 
			printf("reset:%s%d\r\n","ret = ", ret);
        }
        
        continue;
        
		if(run_led_flag == 0)
		{
			LED0_ON();
			run_led_flag = 1;
		}
		else
		{
			LED0_OFF();
			run_led_flag = 0;
		}
	}
}
/*******************************************************************************
* ������  : USART1_IRQHandler
* ����    : ����1�жϷ������
* ����    : ��
* ����    : �� 
* ˵��    : 1)�����յ��Դ��ڷ����������
*           
*******************************************************************************/
void USART1_IRQHandler(void)                	
{
    char res;
	res = USART_ReceiveData(USART1);
    if (res > '0' && res <= '9')
    {
        user_debug_cmd = res;
    }
} 	

/*******************************************************************************
* ������  : USART2_IRQHandler
* ����    : ����1�жϷ������
* ����    : ��
* ����    : �� 
* ˵��    : 
*******************************************************************************/

void USART2_IRQHandler(void)                	
{
	u8 Res=0;    
    char *pos;
    static u16 i       = 0;
    u8  mark_num       = 0;
    int str_len        = 0;
    u8  find_user_urc  = 0;
    int ret            = 0;
    
	Res = USART_ReceiveData(USART2);
    
    debug_buf[debug_count] = Res;
    debug_count++;
    if (debug_count >= 1024)
    {
        debug_count = 0;
    }
    
    if (system_state == STA_MSATER_PROC)
    {
        cmd_send_rcv_buf[send_rcv_ptr] = Res;  	  //�����յ����ַ����浽������
        send_rcv_ptr++;                	          //����ָ������ƶ�
        if(send_rcv_ptr >= SEND_RCV_BUF)          //���������,������ָ��ָ�򻺴���׵�ַ
        {
            send_rcv_ptr = 0;
        }   
        if (master_result_sta == RES_CMD_START_FIND_N || master_result_sta == RES_CMD_END_FIND_R)
        {
            /* �ҵ������� */    
            if (Res == '\r' && (master_result_sta == RES_CMD_START_FIND_N || master_result_sta == RES_CMD_END_FIND_R))
            {
                master_result_sta = RES_CMD_END_FIND_R;
            }
            else if (Res == '\n' && master_result_sta == RES_CMD_END_FIND_R)
            {
                master_result_sta      = RES_CMD_END_FIND_N;
                master_send_cmd_result = SEND_CMD_SUCC;
                stop_timer();
            }
            else
            {
                master_result_sta  = RES_CMD_START_FIND_N;    
            }
        }
        else
        {   
            /* �ҵ���ʼ�� */
            if (Res == '\r' && (master_result_sta == RES_CMD_START_INIT || master_result_sta == RES_CMD_START_FIND_R))
            {
                master_result_sta = RES_CMD_START_FIND_R;
            }
            else if (Res == '\n' && master_result_sta == RES_CMD_START_FIND_R)
            {
                master_result_sta = RES_CMD_START_FIND_N;
            }
            else
            {
                master_result_sta = RES_CMD_START_INIT;    
            }
        }    
    }
    else  /* ���ڱ�������״̬ */
    {          
        /* ����Ҫ�ҵ������� */
        if (slave_result_sta == RES_CMD_START_FIND_P || slave_result_sta == RES_CMD_END_FIND_R || slave_result_sta == RES_CMD_END_FIND_C)
        {
            rcv_cmd_buf[rcv_cmd_ptr] = Res;
            rcv_cmd_ptr++;
            if (rcv_cmd_ptr >= RCV_BUFFER)
            {
            	/* �Ѿ���� */ 
                for(i = 0;i < user_cmd_count;i ++)
                {
                    if (http_cmd[i].sta == RCV_RUNNING)
                    {
                        http_cmd[i].sta = RCV_OVERFLOW;
                        memcpy(http_cmd[i].buff, rcv_cmd_buf, RCV_BUFFER - 1);
                        memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                        break;
                    }
                }

				if (i >= user_cmd_count)
				{
					slave_rcv_cmd_result = RCV_CMD_OVERFLOW;
				}
				
				   
                slave_result_sta     = RES_CMD_START_INIT;
                rcv_cmd_ptr          = 0;
                system_state         = STA_IDE;

				return;
            }
            
            if (rcv_content_count > 0)
            {
                rcv_content_count--;
                record_count--;
                
                if (rcv_content_count <= 0)
                {
                    slave_result_sta     = RES_CMD_START_INIT;                    
                    system_state         = STA_IDE;
                    
                    for(i = 0;i < user_cmd_count;i ++)
                    {
                        if (http_cmd[i].sta == RCV_RUNNING)
                        {
                            http_cmd[i].sta = RCV_SUCC;
                            memcpy(http_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
                            memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                            break;
                        }
                    }
                    
                    record_count = 888;
                    stop_timer();
                }
                return;
            }
            
            /* �ҵ���һ�н�����,��Ϊhttp��ͷ�����ݲ���ÿһ�ж��н�����,����Ҫ�ȸ���һ��,Ȼ���ҵ����ݵĳ��� */
            if (Res == '\r' && (slave_result_sta == RES_CMD_START_FIND_P || slave_result_sta == RES_CMD_END_FIND_R))
            {
                slave_result_sta = RES_CMD_END_FIND_R;
            }
            else if (Res == '\n' && slave_result_sta == RES_CMD_END_FIND_R)
            {
                slave_result_sta = RES_CMD_END_FIND_N;
                
                #if 0
                for (i = 0;i < user_cmd_count; i++)
                {
                    if (!strncmp(rcv_cmd_buf, http_cmd[i].name, strlen(http_cmd[i].name)))
                    {
                        rcv_content_count = 0;
                        find_user_urc     = 1;
                        ret = http_cmd[i].func(rcv_cmd_buf);
                        if (ret == 0)
                        {
                            http_cmd[i].sta = RCV_SUCC;
                        }
                        else if (ret > 0)
                        {
                            http_cmd[i].sta   = RCV_RUNNING;
                            rcv_content_count = ret;
                        }
                        else
                        {
                            http_cmd[i].sta        = RCV_ERROR;
                            http_cmd[i].error_code = ret;
                        }
                        break;
                    }
                    else
                    {
                        rcv_content_count = 0;
                    }
                }
                
                if (find_user_urc != 0 && rcv_content_count > 0)
                {
                    slave_result_sta = RES_CMD_END_FIND_C;
                }
                else
                {
                    slave_result_sta     = RES_CMD_START_INIT;
                    slave_rcv_cmd_result = RCV_CMD_SUCC;
                    stop_timer();
                    system_state         = STA_IDE;
                }
                
                #else
                pos = strstr(rcv_cmd_buf, "+CHTTPNMIH:");
                if (pos != NULL)
                {
                    i = strlen("+CHTTPNMIH:");
                    mark_num = 0;
                    while (*(pos + i) != '\0')
                    {
                        if (*(pos + i) != ',')
                        {
                            i++;
                            continue;
                        }
                        else
                        {
                            mark_num++;
                        }
                        
                        i++;
                        
                        if (mark_num >= RCV_CMD_LEN_POS)
                        {
                            break;
                        }
                    }
                                            
                    if (mark_num >= RCV_CMD_LEN_POS)
                    {
                        while(*(pos + i) != ',')
                        {
                            rcv_content_count = rcv_content_count * 10 + *(pos + i) - '0';
                            record_count      = rcv_content_count;
                            i++;
                        }
                        
                        str_len = strlen(rcv_cmd_buf);
                        if (str_len > i)
                        {
                            rcv_content_count -= (str_len - (i + 1));
                            record_count       = rcv_content_count;
                            
                            slave_result_sta   = RES_CMD_END_FIND_C;
                            
                            for(i = 0;i < user_cmd_count;i ++)
                            {
                                //if (!strncmp(http_cmd[i].name, rcv_cmd_buf, strlen(http_cmd[i].name)))
                                {
                                    http_cmd[i].sta      = RCV_RUNNING;
                                    break;
                                }
                            }
                        }
                        else  //rcv_cmd_buf������ַ����Ȳ�����i�Ļ� ˵��������
                        {
                            slave_result_sta     = RES_CMD_START_INIT;
                            slave_rcv_cmd_result = RCV_CMD_SUCC;
                            stop_timer();
                            
                            system_state         = STA_IDE;
                        }
                        
                    }
                    else
                    {
                        slave_result_sta     = RES_CMD_START_INIT;
                        slave_rcv_cmd_result = RCV_CMD_SUCC;
                        stop_timer();
                        
                        system_state         = STA_IDE;
                    }
                }
                else //˵��һ�����沢û�� �������жϵ��ַ��� ����Ҫֹͣ
                {
                    slave_result_sta     = RES_CMD_START_INIT;
                    slave_rcv_cmd_result = RCV_CMD_SUCC;
                    stop_timer();
                    system_state         = STA_IDE;
                    
                    for(i = 0;i < user_cmd_count;i ++)
                    {
                        if (!strncmp(rcv_cmd_buf, http_cmd[i].name, strlen(http_cmd[i].name)))
                        {
                            http_cmd[i].sta      = RCV_SUCC;
                            memcpy(http_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
                            memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                            break;
                        }
                    }
                }
                #endif
                
            }
            else
            {
                slave_result_sta = RES_CMD_START_FIND_P;
            }
        }
        else
        {   
            /* �ҵ���ʼ�� */
            if (Res == '\r' && (slave_result_sta == RES_CMD_START_INIT || slave_result_sta == RES_CMD_START_FIND_R || slave_result_sta == RES_CMD_START_FIND_N))
            {
                slave_result_sta = RES_CMD_START_FIND_R;
            }
            else if (Res == '\n' && slave_result_sta == RES_CMD_START_FIND_R)
            {
                slave_result_sta = RES_CMD_START_FIND_N;
            }
            else if (Res == '+' && slave_result_sta == RES_CMD_START_FIND_N)
            {
                /* ������ʱ���ն�ʱ��,������5S֮�ڽ������ */ 
                rcv_content_count    = 0;
                i                    = 0;
                slave_result_sta     = RES_CMD_START_FIND_P;
                slave_rcv_cmd_result = RCV_CMD_INIT;
                
                rcv_cmd_ptr  = 0;
                clear_rcv_buf();
                rcv_cmd_buf[rcv_cmd_ptr] = Res;
                rcv_cmd_ptr++;
                
                system_state = STA_SLAVE_PROC;
                start_timer(5000);
            }
            else
            {
                slave_result_sta = RES_CMD_START_INIT; 
            }
        }
    }
} 	

/*******************************************************************************
* ������  : TIM2_IRQHandler
* ����    : ��ʱ��2�ж϶Ϸ�����
* ����    : ��
* ���    : ��
* ����    : �� 
* ˵��    : ��
*******************************************************************************/
void TIM2_IRQHandler(void)   //TIM2�ж�
{
    int i;
    
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //���TIM2�����жϷ������
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //���TIM2�����жϱ�־
        stop_timer();
        
        if (system_state == STA_MSATER_PROC)
        {
            printf("timeout...:%s\r\n", cmd_send_rcv_buf);
            printf("main,arr:%d,count:%d\n", TIM2->ARR, TIM2->CNT);
            master_send_cmd_result = SEND_CMD_TIMEOUT;
            master_result_sta      = RES_CMD_START_INIT;
            system_state           = STA_IDE;
        }
        else
        {
            printf("tttttttttt\r\n");
            slave_rcv_cmd_result = RCV_CMD_TIMEOUT;
            for(i = 0;i < user_cmd_count;i ++)
            {
                if (http_cmd[i].sta == RCV_RUNNING)
                {
                    http_cmd[i].sta = RCV_TIMEOUT;
                    memcpy(http_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
                    memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                    break;
                }
            }
            system_state = STA_IDE;
        }
	}	
}



