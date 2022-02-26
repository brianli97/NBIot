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

#define HTTP_HEAD_CONTENT_MARK (2)  /* 长度出现在第二个逗号后面 */

extern char send_at_buffer[100];
extern int socket;  /* 只记录一个连接 */

int run_user_self_func(void)
{
	UART2_Send_Command("AT+CHTTPCREATE?");

	return 0;
}

static int http_head_rest_content(char *buff)
{
    char *pos;
    int i;
    int mark_num;
    int rest_count = -111;
    int str_len;


	//printf("http_head:%s\n", buff);
    
    pos      = buff;
	str_len  = strlen(buff);   
    i        = strlen("+CHTTPNMIH:");

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
        
        /* 第2个逗号后面就是剩余长度 */
        if (mark_num == HTTP_HEAD_CONTENT_MARK)
        {
            break;
        }
    }
    
    if (mark_num == HTTP_HEAD_CONTENT_MARK)
    {
        rest_count = 0;    
        while(*(pos + i) != ',' && i <= str_len)
        {
            rest_count = rest_count * 10 + *(pos + i) - '0';
            i++;
        }
        
        if (str_len >= i)
        {
            rest_count -= (str_len - (i + 1));
        }
        else /* rcv_cmd_buf里面的字符长度不大于i的话 说明有问题 */
        {
            return -3;
        }
        
    }
    else
    {
        return -2;
    }
    
    return rest_count;
}
static int http_content_rest_content(char *buff)
{
    return 0;
}

rcv_cmd_sys_t http_cmd[] = {
    {
        .name = "+CHTTPNMIH:",
        .sta  = RCV_INIT,
        .func = http_head_rest_content,
        .error_code = 0
    },
    {
        .name = "+CHTTPNMIC:",
        .sta = RCV_INIT,
        .func = http_content_rest_content,
        .error_code = 0
    }
}; 

void init_user_cmd(void)
{
	int i;
	int count = 0;

	count = sizeof(http_cmd) / sizeof(http_cmd[0]);
	for (i = 0; i < count; i++)
	{
		memset(http_cmd[i].buff, '\0', RCV_BUFFER);
	}	
}

int get_user_cmd_num(void)
{
	int count = 0;

	count = sizeof(http_cmd) / sizeof(http_cmd[0]);

	return count;
}

rcv_cmd_sys_t *get_user_cmd_ptr(void)
{

	return http_cmd;
}


int connect_to_http_server(char *domain)
{
    int ret;
    char *ptr;
    char s[2];
    
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
    sprintf(send_at_buffer, "AT+CHTTPCREATE=\"%s\"", domain);
    ret = UART2_Send_AT_Command(send_at_buffer, "+CHTTPCREATE:", 1, 3000); 
    if (ret != 0)
    {
        printf("http create error!\n");
        printf("%s", send_at_buffer);
        printf("%s", cmd_send_rcv_buf);
        return -1;
    } 

    ptr = strstr(cmd_send_rcv_buf, "+CHTTPCREATE: ");
    if (ptr != NULL)
    {
        socket = *(ptr + strlen("+CHTTPCREATE: "));
    }
    
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
    
    s[0] = socket;
    s[1] = '\0';
    sprintf(send_at_buffer, "AT+CHTTPCON=%s", s);
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 3000); 
    if (ret != 0)
    {
        printf("http connect error!\n");
        printf("%s", send_at_buffer);
        printf("%s", cmd_send_rcv_buf);
        return -2;
    } 
    
    printf("http connect succ!\n");
    return 0;
}

int send_data_to_http_server(int sock, char *path)
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
    sprintf(send_at_buffer, "AT+CHTTPSEND=%s,%s,\"%s\"", s, para, path);
    
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 5000);
    if (ret == 0)
    {
        printf("get http request succ!\n");
    }
    else
    {
        printf("%s", send_at_buffer);    
        printf("get http request failed!\n");
        
        return -1;
    }
    
    return 0;
}

int close_socket(int sock)
{
    int ret = 0;
    char s[2];
    
    s[0]   = sock;
    s[1]   = '\0';
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
    sprintf(send_at_buffer, "AT+CHTTPDISCON=%s", s);
    
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
    
    memset(send_at_buffer, '\0', sizeof(send_at_buffer));
    sprintf(send_at_buffer, "AT+CHTTPDESTROY=%s", s);
    
    ret = UART2_Send_AT_Command(send_at_buffer, "OK", 1, 2000);
    if (ret == 0)
    {
        printf("destroy succ!\n");
    }
    else
    {
        printf("destroy failed!\n");
        
        return -2;
    }
    
    socket = -1;     
    return 0;
}
