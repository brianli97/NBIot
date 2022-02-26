#ifndef __USER_FUNC_H__
#define __USER_FUNC_H__
#include "guoyun_nb_iot.h"

typedef enum http_cmd_name{
    
    CHTTPNMIN = 0x00, 
    CHTTPNMIH = 0x01,
    CHTTPNMIC = 0x02,
    
}http_cmd_name_t;

void init_user_cmd(void);
int get_user_cmd_num(void);
rcv_cmd_sys_t *get_user_cmd_ptr(void);

int run_user_self_func(void);

int connect_to_http_server(char *domain);
int send_data_to_http_server(int sock, char *path);
int close_socket(int sock);

#endif
