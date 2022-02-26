#ifndef _GUOYUN_NB_IOT_H
#define _GUOYUN_NB_IOT_H
#include "usart.h"

#define PUBLIC_VERSION    //�������̵�ʱ��������
//#define USE_LED_DISPLAY   //ʹ��ledָʾ�ƽ�����˸ָʾϵͳ�Ƿ��ܷ�����

#define printf printf

#define SEND_RCV_BUF  100  //����2���泤�� ���ڽ�����������ķ���
#define RCV_BUFFER    400  /* ���汻����������URC������ */
#define DEBUG_BUFFER  1024 //������յ������д������� ���ڵ���
#define AT_CMD_BUFFER 100  //���ڹ��췢�������buffer

#define STABLE_TIMES  20  //�ȴ�ϵͳ�ϵ����ȶ�

extern char cmd_send_rcv_buf[SEND_RCV_BUF];/* �������������,�յ�����Ļظ�buffer */
extern u16  send_rcv_ptr;

extern char rcv_cmd_buf[RCV_BUFFER];  /* ���ܵ�URC�����buffer */
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
    RES_CMD_START_FIND_P = 0x03,  /* �ҵ�'+' */
    RES_CMD_END_FIND_R   = 0x04,
    RES_CMD_END_FIND_N   = 0x05,
    RES_CMD_END_FIND_C   = 0x06, /* URC���ڽ���ʣ������� */
    
}cmd_result_sta_t;

typedef enum cmd_rcv_result{
    
    RCV_CMD_INIT     = 0x00,
    RCV_CMD_SUCC     = 0x01,
    RCV_CMD_TIMEOUT  = 0x02,
    RCV_CMD_OVERFLOW = 0x03,
} cmd_rcv_result_t;

typedef enum cmd_rcv_sta{
    
    RCV_INIT     = 0x00, /* ��Ч */
    RCV_RUNNING  = 0x01, /* ���ڴ��� */
    RCV_SUCC     = 0x02, /* �ɹ��յ� */
    RCV_TIMEOUT  = 0x03, /* ���ճ�ʱ */
    RCV_OVERFLOW = 0x04, /* ������� */
    RCV_ERROR    = 0x05, /* ʣ�೤�ȼ������ */
} cmd_rcv_sta_t;

typedef int (*rcv_cmd_content_len)(char *buff);
typedef struct rcv_cmd_sys{
    
    const char *name;         /* Ҫ�ȶԵ�URC���ַ��� */
    cmd_rcv_sta_t sta;        /* �˱����������յĽ�� */
    char buff[RCV_BUFFER];    /* �˱�������� */
    rcv_cmd_content_len func; /* �������ռ��㻹Ҫ���յ��ַ������� */
    int error_code;           /* ��¼����ʣ���������ķ��ش����� */
    
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
int wait_regs(u8 query_times);    //�ȴ�ģ��ע��ɹ�

int find_int_in_str(char *input, int num);
int hex_to_str(char *hex, char *str, int hex_len);


#endif
