#ifndef _GUOYUN_NB_IOT_H
#define _GUOYUN_NB_IOT_H
#include "usart.h"

#define PUBLIC_VERSION    //发布例程的时候打开这个宏
//#define USE_LED_DISPLAY   //使用led指示灯进行闪烁指示系统是否跑飞死机

#define printf printf

#define SEND_RCV_BUF  100  //串口2缓存长度 用于接收主动命令的返回
#define RCV_BUFFER    400  /* 缓存被动接收命令URC的数据 */
#define DEBUG_BUFFER  1024 //缓存接收到的所有串口数据 用于调试
#define AT_CMD_BUFFER 100  //用于构造发送命令的buffer

#define STABLE_TIMES  20  //等待系统上电后的稳定

extern char cmd_send_rcv_buf[SEND_RCV_BUF];/* 主动发送命令后,收到命令的回复buffer */
extern u16  send_rcv_ptr;

extern char rcv_cmd_buf[RCV_BUFFER];  /* 接受到URC命令的buffer */
extern int  rcv_cmd_ptr;

typedef enum cur_state{
    
    STA_IDE           = 0x00,
    STA_MSATER_PROC   = 0x01,
    STA_SLAVE_PROC    = 0x02,
    
}cur_state_t;

typedef enum cmd_send_result{
    
    SEND_CMD_INIT     = 0x00,
    SEND_CMD_SUCC     = 0x01,
    SEND_CMD_TIMEOUT  = 0x02,
} cmd_send_result_t;

typedef enum cmd_result_sta{
    
    RES_CMD_START_INIT   = 0x00,
    RES_CMD_START_FIND_R = 0x01,
    RES_CMD_START_FIND_N = 0x02,
    RES_CMD_START_FIND_P = 0x03,  /* 找到'+' */
    RES_CMD_END_FIND_R   = 0x04,
    RES_CMD_END_FIND_N   = 0x05,
    RES_CMD_END_FIND_C   = 0x06, /* URC正在接收剩余的内容 */
    
}cmd_result_sta_t;

typedef enum cmd_rcv_result{
    
    RCV_CMD_INIT     = 0x00,
    RCV_CMD_SUCC     = 0x01,
    RCV_CMD_TIMEOUT  = 0x02,
    RCV_CMD_OVERFLOW = 0x03,
} cmd_rcv_result_t;

typedef enum cmd_rcv_sta{
    
    RCV_INIT     = 0x00, /* 无效 */
    RCV_RUNNING  = 0x01, /* 正在处理 */
    RCV_SUCC     = 0x02, /* 成功收到 */
    RCV_TIMEOUT  = 0x03, /* 接收超时 */
    RCV_OVERFLOW = 0x04, /* 接收溢出 */
    RCV_ERROR    = 0x05, /* 剩余长度计算出错 */
} cmd_rcv_sta_t;

typedef int (*rcv_cmd_content_len)(char *buff);
typedef struct rcv_cmd_sys{
    
    const char *name;         /* 要比对的URC的字符串 */
    cmd_rcv_sta_t sta;        /* 此被动命令最终的结果 */
    char buff[RCV_BUFFER];    /* 此被动命令缓存 */
    rcv_cmd_content_len func; /* 被动接收计算还要接收的字符串长度 */
    int error_code;           /* 记录长度剩余个数错误的返回错误码 */
    
}rcv_cmd_sys_t;

extern cur_state_t system_state;
extern cmd_send_result_t master_send_cmd_result;
extern cmd_result_sta_t  master_result_sta;

void clear_send_rcv_buffer(void);
void clear_rcv_buf(void);
u8 find_string(char *a);
int check_communication(void);
int close_ate(void);
int check_register_network(void);
int wait_regs(u8 query_times);    //等待模块注册成功

int find_int_in_str(char *input, int num);
int hex_to_str(char *hex, char *str, int hex_len);


#endif
