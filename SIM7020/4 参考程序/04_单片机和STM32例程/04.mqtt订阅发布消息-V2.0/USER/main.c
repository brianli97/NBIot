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
char debug_buf[DEBUG_BUFFER];  //���ڴ��ڽ��մ洢,������������Ϣ
int debug_count = 0;

/*************  6.ϵͳ״̬��Ҫ�ı���	**************/
cur_state_t system_state = STA_IDE;
char send_at_buffer[AT_CMD_BUFFER];  //���ڹ��췢�͵�����

/*************  7.���ܶ�����Ҫ�ı���	**************/
int socket = -1;           /* ֻ��¼һ������ */

extern rcv_cmd_sys_t mqtt_cmd[];

rcv_cmd_sys_t *user_cmd;
int user_cmd_count;

void init_system_status(void)
{
	slave_rcv_cmd_result = RCV_CMD_INIT;
	system_state         = STA_IDE;
}

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
	#if defined(USE_LED_DISPLAY)
	u8  run_led_flag = 0;
	#endif
	int ret;
    int i;
    int len;
    int j;
    int count;
    char hex[30];
    char str[20];
    char *ptr;
    
	init_system_status();
	init_user_cmd();
	user_cmd       = get_user_cmd_ptr();
    user_cmd_count = get_user_cmd_num();

	if (user_cmd == NULL)
	{
		printf("not find user ptr!\r\n");
	}
	
	SysTick_Init_Config();   //ϵͳ�δ�ʱ�ӳ�ʼ��
	GPIO_Config();           //GPIO��ʼ��
	Key_GPIO_Config();
	USART2_Init_Config(115200);  //����2��ʼ��
	USART1_Init_Config(115200);//UART1�������ڵ�����Ϣ
    
    delay_ms(2000);
	printf("ϵͳ����,��Ҫ�ȴ�(%d)��..................\r\n", STABLE_TIMES);
    
	timer2_init_config();        //��ʱ��2��ʼ��,���ڵȴ����ʱ    

	#if defined(PUBLIC_VERSION)
	for(i = 0;i < STABLE_TIMES;i++)  //�����ʱ����Ϊ�˵ȴ�ģ���ϵ���ȶ� 
	{
		delay_ms(1000);
		printf ("%d ", i + 1);
	}
	printf("ok\n");
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
        
   	while(1)
	{ 
        for (i = 0;i < user_cmd_count; i++)
        {
            if (user_cmd[i].sta == RCV_SUCC)
            {
                printf("cmd:%s,receive succ,%d,len=%d\n\n", user_cmd[i].name, record_count, strlen(user_cmd[i].buff));
                if (!strncmp(user_cmd[i].buff, "+CMQPUB:", strlen("+CMQPUB:")))
                {
                   printf("recveive pub msg\n");

                   len = 0;

                   //+CMQPUB: 0,"led_set",0,0,0,4,"4F4E"
                   len = find_int_in_str(user_cmd[i].buff + strlen("+CMQPUB: "), 6);
                   if (len > 0)
                   {
                        memset(hex, '\0', 30);
                        memset(str, '\0', 20);

                        ptr   = user_cmd[i].buff;
                        count = 0;
                        j     = 0;
                        while(*ptr != '\0' && *ptr != '\r')
                        {
                            if (*ptr == ',')
                            {
                                count++;
                                if (count == 6)
                                {
                                    ptr++; //����\"
                                    ptr++;
                                    while(*ptr != '\0' && *ptr != '\r' && *ptr != '\"')
                                    {
                                        hex[j] = *ptr;
                                        j++;
                                        ptr++;
                                    }

                                    printf("hex:%s\n", hex);
                                    break;
                                }
                            }
                            ptr++;
                        }

                        ret = hex_to_str(hex, str, len / 2);
                        
                        printf("topic msg:%s\n", str);
                        if (!strcmp(str, "ON"))
                        {
                            printf("on led please\n");
                        }
                        else if (!strcmp(str, "OFF"))
                        {
                            printf("off led please\n");
                        }
                        else
                        {
                            printf("unknow message\n");
                        }
                        
                   }
                   
				   ret = pub_topic(socket, "led_result", "53554343");
				   printf("pub topic<%s>, ret=%d\n", "led_result", ret);
                }
				else if (!strncmp(user_cmd[i].buff, "+CMQDISCON", strlen("+CMQDISCON")))
                {
                   printf("mqtt be closed\n");
				   socket = -1;
                }
            }
            else if (user_cmd[i].sta == RCV_TIMEOUT)
            {
                printf("cmd:%s,receive timeout\n", user_cmd[i].name);
            }
            else if (user_cmd[i].sta == RCV_OVERFLOW)
            {
                printf("cmd:%s,receive overflow\n", user_cmd[i].name);
            }
            else if (user_cmd[i].sta == RCV_ERROR)
            {
                printf("cmd:%s,receive error:%d\n", user_cmd[i].name, user_cmd[i].error_code);
            }
            
            if (user_cmd[i].sta != RCV_INIT && user_cmd[i].sta != RCV_RUNNING)
            {
                user_cmd[i].sta = RCV_INIT;
				memset(user_cmd[i].buff, '\0', sizeof(user_cmd[i].buff));
            }
        }

		if (slave_rcv_cmd_result != RCV_CMD_INIT)
		{
			delay_ms(2000);
			rcv_cmd_buf[RCV_BUFFER - 1] = '\0';
			printf("urc(not user) rcv result:%d,rcv_buff:%s\n", slave_rcv_cmd_result, rcv_cmd_buf);
			slave_rcv_cmd_result = RCV_CMD_INIT;
		}
		
        if (user_debug_cmd == '1')
        {
			printf("�յ�1:%s\r\n", rcv_cmd_buf);    
            user_debug_cmd = 0;
            
            ret = connect_to_mqtt_server("203.195.152.101", "1883");            
			printf("%s%d,socket=%c\r\n","ret = ", ret, socket);
        }
        
        if (user_debug_cmd == '2')
        {
			printf("�յ�2\r\n");    
            user_debug_cmd = 0;

			ret = sub_topic(socket, "led_set");
			printf("sub topic<%s>, ret=%d\n", "led_set", ret);
			
        }
        
        if (user_debug_cmd == '3')
        {
			printf("�յ�3\r\n");    
            user_debug_cmd = 0;
            ret = close_mqtt_connect(socket);            
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

		if (user_debug_cmd == '6')
        {
        	user_debug_cmd = 0;
			printf("�յ�6, show debug buffer\r\n");    

			printf("debug buffer:%s\n", debug_buf);
			memset(debug_buf, '\0', sizeof(debug_buf));
			debug_count = 0;
        }

		
		if (user_debug_cmd == '7')
        {
        	user_debug_cmd = 0;
			printf("�յ�7, do user func\r\n");    

			ret = run_user_self_func();
			printf("user func ret=%d\n", ret);
        }

		if (user_debug_cmd == '8')
        {
        	user_debug_cmd = 0;
			printf("�յ�8, show user urc\r\n");    

			for (i = 0;i < user_cmd_count; i++)
			{
				printf("%s\r\n", user_cmd[i].name);
			}

            ret = find_int_in_str("1,\"led_set\",2,3,4,12345,\"4F4E\"", 6);

            printf("analy ret=%d\n", ret);
			
        }

		#if defined(USE_LED_DISPLAY)
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
		#endif
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
    static u16 i       = 0;
    u8  find_user_urc  = 0;
    int ret            = 0;
    
	Res = USART_ReceiveData(USART2);
    
    debug_buf[debug_count] = Res;
    debug_count++;
    if (debug_count >= DEBUG_BUFFER)
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
                    if (user_cmd[i].sta == RCV_RUNNING)
                    {
                        user_cmd[i].sta = RCV_OVERFLOW;
                        memcpy(user_cmd[i].buff, rcv_cmd_buf, RCV_BUFFER - 1);
                        memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                        break;
                    }
                }

				if ( slave_result_sta != RES_CMD_END_FIND_C)
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
                        if (user_cmd[i].sta == RCV_RUNNING)
                        {
                            user_cmd[i].sta = RCV_SUCC;
                            memcpy(user_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
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
                
                for (i = 0;i < user_cmd_count; i++)
                {
                    if (!strncmp(rcv_cmd_buf, user_cmd[i].name, strlen(user_cmd[i].name)))
                    {
                        rcv_content_count = 0;
                        find_user_urc     = 1;
                        ret = user_cmd[i].func(rcv_cmd_buf);
						
                        if (ret == 0)
                        {
                            user_cmd[i].sta = RCV_SUCC;
							memcpy(user_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
							system_state    = STA_IDE;
							stop_timer();
                        }
                        else if (ret > 0)
                        {
                            user_cmd[i].sta   = RCV_RUNNING;
                            rcv_content_count = ret;
                        }
                        else
                        {
                            user_cmd[i].sta        = RCV_ERROR;
                            user_cmd[i].error_code = ret;
							system_state         = STA_IDE;
							stop_timer();
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
				else if (find_user_urc)
				{
					stop_timer();
					slave_result_sta     = RES_CMD_START_INIT;
                    system_state         = STA_IDE;
				}
                else 
                {
                	stop_timer();
                    slave_result_sta     = RES_CMD_START_INIT;
                    slave_rcv_cmd_result = RCV_CMD_SUCC;
                    system_state         = STA_IDE;
                }
                                
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
            printf("master mode timeout...:%s\r\n", cmd_send_rcv_buf);
            printf("main,arr:%d,count:%d\n", TIM2->ARR, TIM2->CNT);
            master_send_cmd_result = SEND_CMD_TIMEOUT;
            master_result_sta      = RES_CMD_START_INIT;
            system_state           = STA_IDE;
        }
        else
        {
            printf("slave mode timeout\r\n");
            
            for(i = 0;i < user_cmd_count;i ++)
            {
                if (user_cmd[i].sta == RCV_RUNNING)
                {
                    user_cmd[i].sta = RCV_TIMEOUT;
                    memcpy(user_cmd[i].buff, rcv_cmd_buf, strlen(rcv_cmd_buf));
                    memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
                    break;
                }
            }

			if (i >= user_cmd_count)
			{
				slave_rcv_cmd_result = RCV_CMD_TIMEOUT;
			}

			slave_result_sta = RES_CMD_START_INIT;
            system_state     = STA_IDE;
        }
	}	
}



