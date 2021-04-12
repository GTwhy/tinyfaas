//
// Created by why on 2021/4/12.
//

#ifndef LUTF_WORKER_TOOLS_H
#define LUTF_WORKER_TOOLS_H
#include <stdlib.h>
#include "work_server.h"
char * add_new_function(char * func_server_url, char * work_server_url, char * workload_path, char * func_name, int app_id, int func_id);
void * make_function_request(void * work_server_url, void * args, size_t args_size);
void free_rv(void * rv);
void free_url(char * url);

#endif //LUTF_WORKER_TOOLS_H