#include <stdio.h>
#include "stm32f10x.h"
#include "usart.h"
#include "Led.h"
#include "SysTick.h"
#include "timer.h"
#include "string.h"
#include "key.h"
#include "guoyun_nb_iot.h"

void clear_rcv_buf(void)
{
    memset(rcv_cmd_buf, '\0', sizeof(rcv_cmd_buf));
}

/*判断通信是否成功*/
int check_communication(void)
{
    int ret = -1;
    
    ret = UART2_Send_AT_Command("AT","OK",3,1000);//测试通信是否成功
    
    return ret;
}

/* 关闭回显 */
int close_ate(void)
{
    int ret = -1;
    
    ret = UART2_Send_AT_Command("ATE0","OK",3,100);
    
    return ret;
}

/* 检测是否注册上网络 */
int check_register_network(void)
{
	int ret;
	
	ret = wait_regs(3);//查询卡是否注册到网络
	
    return ret;
}

/*******************************************************************************
* 函数名 : wait_regs
* 描述   : 等待模块注册成功
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 
*******************************************************************************/
int wait_regs(u8 query_times)
{
	u8 i;
    
	i = 0;
	
    if (system_state == STA_IDE)
    {
        system_state = STA_MSATER_PROC;
    }
    else
    {
        return -2;
    }

    while(i < query_times)
    {        
		UART2_Send_Command("AT+CREG?");  //发送查询是否注册网络的函数
        
		start_timer(2000);        
        while(master_send_cmd_result == SEND_CMD_INIT);
        
        printf("send creg cmd STA:%d\n", master_send_cmd_result);
        
        i++;
        if (master_send_cmd_result == SEND_CMD_TIMEOUT)
        {
            printf("wait reg response timeout!\n");
            continue;
        }
        
        master_send_cmd_result = SEND_CMD_INIT;
        
		if(find_string("+CREG: 0,1") || find_string("+CREG: 0,5") || find_string("+CREG: 0,6"))  //查找需要应答的字符串
		{
            system_state = STA_IDE;
			return 0;
		}
    }    

    system_state = STA_IDE;
	return -1;
}


/*******************************************************************************
* 函数名 : clear_send_rcv_buffer
* 描述   : 清除串口2缓存数据
* 输入   : 
* 输出   : 
* 返回   : 
* 注意   : 
*******************************************************************************/
void clear_send_rcv_buffer(void)
{
	u16 k;
	for(k = 0;k < SEND_RCV_BUF;k++)      //将缓存内容清零
	{
		cmd_send_rcv_buf[k] = '\0';
	}
    send_rcv_ptr = 0;              //接收字符串的起始存储位置
}



/*******************************************************************************
* 函数名 : find_string
* 描述   : 判断缓存中是否含有指定的字符串
* 输入   : 
* 输出   : 
* 返回   : unsigned char:1 找到指定字符，0 未找到指定字符 
* 注意   : 
*******************************************************************************/
u8 find_string(char *a)
{ 
	if(strstr(cmd_send_rcv_buf, a)!=NULL) //cmd_send_rcv_buf接收返回数据的buffer strstr函数是库函数，这个函数可以判定cmd_send_rcv_buf里面是否又a字符串
	{
		return 1;
	}	
	else
	{
		return 0;
	}
		
}

/* 
** 以逗号分隔从参数列表查找整形参数(不能查找最后一个整形参数)
** str:所有的参数表,不包括空格
** num:第num个参数
*/

int find_int_in_str(char *input, int num)
{
    int i = 0;
    char *p     = input;
    char *start = NULL;
    int val;

    
    val   = 0;
    p     = input;
    start = input;
    
    while(*p != '\0' && *p != '\r')
    {
        if(*p == ',')
        {
            i++;            
            if(i == num)
            {                
                while(*start != '\0' && *start != ',' && *start != '\r' && *start != '\"')
                {
                    if (*start < '0' || *start > '9')
                    {
                        printf("fault para\n");
                        return -2;
                    }
                    
                    val = val * 10 + (*start - '0');
                    start++;
                }
                
                return val;
            }
            else
            {
                start = p + 1;
            }
        }
        p++;
    }

    printf("can't find prefer para\n");
    
    return -1;
}


int hex_to_str(char *hex, char *str, int hex_len)
{
    int i,j;
    char c;
    if (strlen(hex) != 2 * hex_len)
    {
        printf("hex is invilad:%d,%d\n", strlen(hex), hex_len);
        return -1;
    }
    
    for (i = 0; i < 2 * hex_len; i += 2)
    {
        c = 0;
        
        if (hex[i] >= '0' && hex[i] <= '9')
        {
            c = (hex[i] - '0') << 4;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f')
        {
            c = (hex[i] - 'a' + 10) << 4;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F')
        {
            c = (hex[i] - 'A' + 10) << 4;
        }
        j = i + 1;
        if (hex[j] >= '0' && hex[j] <= '9')
        {
            c |= (hex[j] - '0');
        }
        else if (hex[j] >= 'a' && hex[j] <= 'f')
        {
            c |= (hex[j] - 'a' + 10);
        }
        else if (hex[j] >= 'A' && hex[j] <= 'F')
        {
            c |= (hex[j] - 'A' + 10);
        }

        str[i / 2] = c;
    }
    
    return 0;
}
