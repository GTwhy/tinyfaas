//
// Created by why on 2021/4/12.
//

#ifndef LUTF_WORKER_TOOLS_H
#define LUTF_WORKER_TOOLS_H
#include <stdlib.h>
#include "work_server.h"
char * add_new_function(char * func_server_ip, uint16_t func_server_port, char * work_server_url, char * workload_path, char * func_name, int app_id, int func_id);
void * make_function_request(void * work_server_url, void * args, size_t args_size);
int stop_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);
int restore_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);
void free_rv(void * rv);
void free_url(char * url);

#endif //LUTF_WORKER_TOOLS_H
