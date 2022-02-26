#include <stdio.h>
#include <stdlib.h>

#include "stm32f10x.h"
#include "usart.h"
#include "Led.h"
#include "SysTick.h"
#include "timer.h"
#include "string.h"
#include "key.h"
#include "guoyun_nb_iot.h"
#include "user_func.h"

#define HTTP_HEAD_CONTENT_MARK (2)  /* 长度出现在第二个逗号后面 */

extern char send_at_buffer[100];
extern int socket;  /* 只记录一个连接 */

int run_user_self_func(void)
{
	UART2_Send_Command("AT+CMQNEW?");

	return 0;
}

static int mqtt_rcv_sub(char *buff)
{
	return 0;
}
static int mqtt_disconnect(char *buff)
{
    return 0;
}

rcv_cmd_sys_t mqtt_cmd[] = {
    {
        .name = "+CMQPUB:", /* 收到订阅的消息 */
        .sta  = RCV_INIT,
        .func = mqtt_rcv_sub,
        .error_code = 0
    },
    {
        .name = "+CMQDISCON",    /* mqtt被关闭 */
        .sta = RCV_INIT,
        .func = mqtt_disconnect,
        .error_code = 0
    }
}; 

void init_user_cmd(void)
{
	int i;
	int count = 0;

	count = sizeof(mqtt_cmd) / sizeof(mqtt_cmd[0]);
	for (i = 0; i < count; i++)
	{
		memset(mqtt_cmd[i].buff, '\0', RCV_BUFFER);
	}	
}

int get_user_cmd_num(void)
{
	int count = 0;

	count = sizeof(mqtt_cmd) / sizeof(mqtt_cmd[0]);

	return count;
}

rcv_cmd_sys_t *get_user_cmd_ptr(void)
{

	return mqtt_cmd;
}


int connect_to_mqtt_server(char *ip, char *port)
{
    int ret;
    char *ptr;
    char s[2];
    
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
	//AT+CMQNEW="203.195.152.101","1883",60000,1024
    sprintf(send_at_buffer, "AT+CMQNEW=\"%s\",\"%s\",%s,%s", ip, port, "6000", "1024");
	//+CMQNEW: 0
    ret = UART2_Send_AT_Command(send_at_buffer, "+CMQNEW:", 1, 3000); 
    if (ret != 0)
    {
        printf("mqtt create error!\n");
        return -1;
    } 

    ptr = strstr(cmd_send_rcv_buf, "+CMQNEW: ");
    if (ptr != NULL)
    {
        socket = *(ptr + strlen("+CMQNEW: "));
    }
    
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
    
    s[0] = socket;
    s[1] = '\0';
	
	delay_ms(100);
	//AT+CMQCON=0,3,guoyun,80,1,0
    sprintf(send_at_buffer, "AT+CMQCON=%s,%s", s, "3,guoyun,80,1,0");
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 3000); 
    if (ret != 0)
    {
        printf("mqtt connect error!\n");
        return -2;
    } 
    
    printf("mqtt connect succ!\n");
    return 0;
}

int sub_topic(int sock, char *topic)
{
    int ret = 0;
    char s[2];
    char para[2];
    
    if (sock == -1)
    {
        printf("socket is invalid -1\n");
        return -1;
    }
    
    s[0]    = sock;
    s[1]    = '\0';
    para[0] = '0';
    para[1] = '\0';
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));

	//AT+CMQSUB=0,"led_set",0
    sprintf(send_at_buffer, "AT+CMQSUB=%s,\"%s\",%s", s, topic, para);
    
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 5000);
    if (ret == 0)
    {
        printf("sub topic:%s succ!\n", topic);
    }
    else
    {
        printf("sub topic:%s failed!\n", topic);        
        return -2;
    }
    
    return 0;
}

/*
**FAIL: 4641494C
**SUCC: 53554343
**
*/
int pub_topic(int sock, char *topic, char *msg)
{
    int ret = 0;
    char s[2];
	int len;
	
    
    if (sock == -1)
    {
        printf("socket is invalid -1\n");
        return -1;
    }
    
    s[0]    = sock;
    s[1]    = '\0';

	if (strlen(msg) >= (AT_CMD_BUFFER - 20))
	{
		printf("msg length error\n");
		return -2;
	}
	
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));

	len = strlen(msg);
	
	//AT+CMQPUB=0,"led_set_result",1,0,0,10,"3836333435"
    sprintf(send_at_buffer, "AT+CMQPUB=%s,\"%s\",%s,%d,\"%s\"", s, topic, "1,0,0", len, msg);
    
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 5000);
    if (ret == 0)
    {
        printf("pub topic:%s succ!\n", topic);
    }
    else
    {
        printf("pub topic:%s failed!\n", topic);
        
        return -1;
    }
    
    return 0;
}


int close_mqtt_connect(int sock)
{
    int ret = 0;
    char s[2];
    
    s[0]   = sock;
    s[1]   = '\0';
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));

	//AT+CMQDISCON=0
    sprintf(send_at_buffer, "AT+CMQDISCON=%s", s);
    
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 2000);
    if (ret == 0)
    {
        printf("close succ!\n");
    }
    else
    {
        printf("close failed!\n");
        
        return -1;
    }
    
    socket = -1;     
    return 0;
}
