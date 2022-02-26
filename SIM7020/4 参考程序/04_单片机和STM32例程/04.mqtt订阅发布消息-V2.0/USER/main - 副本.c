/**********************************************************************************
   STM32F103C8T6  	 
 * 硬件连接说明
   使用单片串口2与NB-7020模块通信  注：使用串口2可以避免下载和通信不会冲突
   STM32 与 NB-7020模块板子接线方法:  
     
   PA3 (RXD2)->TX       
   PA2 (TXD2)->RX         
   GND	   ->GND          
 
   PA9(TXD1)--->调试输出信息端口
   PA10(RXD1)-->调试输入信息端口

设计：
(1)LED0-7设置的引脚为:PB4-7 PB12-15
(2)KEY0-3设置的引脚为:PA4-7
(3)通过串口2下发字符1、2、3进行相应的返回，连接服务器，发送数据，关闭连接。

果云技术:杜工 群号:908159506

**代码设计思路：
**(1)主动发命令和被动收到的命令进行互斥操作。
**(2)开机要先等到模块注册上网络,才进行下一步命令的发送,否则一直等待。
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

/*************  1.主动发送命令需要的变量	**************/
char cmd_send_rcv_buf[SEND_RCV_BUF];       /* 主动发送命令后,收到命令的回复buffer */
u16 send_rcv_ptr = 0;
cmd_result_sta_t  master_result_sta;       /* 主动发送命令后开始检测模块回复的字符串 */
cmd_send_result_t master_send_cmd_result;  /* 主动发送命令后是否成功返回 */ 

/*************  2.被动接收命令需要的变量	**************/
char rcv_cmd_buf[RCV_BUFFER];              /* 被动接受到命令的buffer */
int rcv_cmd_ptr = 0;
static int rcv_content_count = 0;
cmd_result_sta_t  slave_result_sta;       /* 被动接收命令开始检测模块回复的字符串 */
cmd_rcv_result_t  slave_rcv_cmd_result;   /* 被动接收命令后是否成功返回(非用户定义的URC) */ 

/*************  3.用户测试命令需要的变量	**************/
char user_debug_cmd = 0;                   /* 用户通过串口1下发的字符 控制不同的功能 */

/*************  5.调试信息需要的变量	**************/
static int record_count = 0;
char debug_buf[1024];  //用于串口接收存储,方便打出调试信息
int debug_count = 0;
int cmd_type = 0;

/*************  6.系统状态需要的变量	**************/
cur_state_t system_state = STA_IDE;
char send_at_buffer[100];  //用于构造发送的命令

/*************  7.功能定义需要的变量	**************/
int socket = -1;           /* 只记录一个连接 */

extern rcv_cmd_sys_t http_cmd[];

rcv_cmd_sys_t *user_cmd;
int user_cmd_count;

#define USER_CMD     http_cmd
#define USER_CMD_PTR &http_cmd[0]
#define RCV_CMD_LEN_POS (2)  /* 接收命令长度的逗号位置 */

/*******************************************************************************
* 函数名 : main 
* 描述   : 主函数
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 串口2负责与guoyun_nb_iot模块通信，串口1用于串口调试，
*******************************************************************************/
int main(void)
{
	u8  run_led_flag = 0;
	int ret;
    int i;
    
    char buff[1024];
    int str_len;

    
	SysTick_Init_Config();   //系统滴答时钟初始化
	GPIO_Config();           //GPIO初始化
	Key_GPIO_Config();
	USART2_Init_Config(115200);  //串口2初始化
	USART1_Init_Config(115200);//UART1用作串口调试信息
    
    delay_ms(2000);
	printf("系统启动.......................\r\n");
    
	timer2_init_config();        //定时器2初始化,用于等待命令超时    

	#if defined(PUBLIC_VERSION)
	for(i = 0;i < STABLE_TIMES;i++)  //这个延时就是为了等待模块上电后稳定 
	{
		delay_ms(1000);
	}
	#endif
	
    /* 1.等待通信正常 */
    printf("1.测试通信是否正常?\r\n");
      
    while ((ret = check_communication()) != 0)
    {
        printf("通信失败:%d\n", ret);
        
        delay_ms(1000);
    }
    printf("  通信正常！\r\n");
    /* 2.关闭回显 */
    printf("2.关闭回显\r\n");
    while (close_ate() != 0)
    {
        printf("  关闭失败!\r\n");
        delay_ms(1000);
    }
    printf("  关闭成功!\r\n");
    /* 3.等待注册上网络 */
    printf("3.等待注册网络\r\n");
    while (check_register_network() != 0)
    {
        printf("  注册失败!\r\n");
        delay_ms(1000);
    }
    printf("  注册成功!\r\n");
    
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
			printf("收到1:%s\r\n", rcv_cmd_buf);    
            user_debug_cmd = 0;
            
            ret = connect_to_http_server("http://httpbin.org");            
			printf("%s%d\r\n","ret = ", ret);
        }
        
        if (user_debug_cmd == '2')
        {
			printf("收到2\r\n");    
            user_debug_cmd = 0;
            ret = send_data_to_http_server(socket, "/ip");            
			printf("%s%d\r\n","ret = ", ret);
            
        }
        
        if (user_debug_cmd == '3')
        {
			printf("收到3\r\n");    
            user_debug_cmd = 0;
            ret = close_socket(socket);            
			printf("%s%d\r\n","ret = ", ret);
        }
        
        if (user_debug_cmd == '4')
        {
			printf("收到4, reset now\r\n");    
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
			printf("收到5, test communicate\r\n");    
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
* 函数名  : USART1_IRQHandler
* 描述    : 串口1中断服务程序
* 输入    : 无
* 返回    : 无 
* 说明    : 1)、接收调试串口发过来的命令。
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
* 函数名  : USART2_IRQHandler
* 描述    : 串口1中断服务程序
* 输入    : 无
* 返回    : 无 
* 说明    : 
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
        cmd_send_rcv_buf[send_rcv_ptr] = Res;  	  //将接收到的字符串存到缓存中
        send_rcv_ptr++;                	          //缓存指针向后移动
        if(send_rcv_ptr >= SEND_RCV_BUF)          //如果缓存满,将缓存指针指向缓存的首地址
        {
            send_rcv_ptr = 0;
        }   
        if (master_result_sta == RES_CMD_START_FIND_N || master_result_sta == RES_CMD_END_FIND_R)
        {
            /* 找到结束符 */    
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
            /* 找到起始符 */
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
    else  /* 处于被动接收状态 */
    {          
        /* 必须要找到结束符 */
        if (slave_result_sta == RES_CMD_START_FIND_P || slave_result_sta == RES_CMD_END_FIND_R || slave_result_sta == RES_CMD_END_FIND_C)
        {
            rcv_cmd_buf[rcv_cmd_ptr] = Res;
            rcv_cmd_ptr++;
            if (rcv_cmd_ptr >= RCV_BUFFER)
            {
            	/* 已经溢出 */ 
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
            
            /* 找到第一行结束符,因为http的头和内容部分每一行都有结束符,所以要先根据一行,然后找到内容的长度 */
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
                        else  //rcv_cmd_buf里面的字符长度不大于i的话 说明有问题
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
                else //说明一行里面并没有 带长度判断的字符串 所以要停止
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
            /* 找到起始符 */
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
                /* 开启超时接收定时器,必须在5S之内接收完成 */ 
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
* 函数名  : TIM2_IRQHandler
* 描述    : 定时器2中断断服务函数
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 无
*******************************************************************************/
void TIM2_IRQHandler(void)   //TIM2中断
{
    int i;
    
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //检查TIM2更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //清除TIM2更新中断标志
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



