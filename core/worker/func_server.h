//
// Created by why on 2021/3/4.
//

#ifndef LUTF_FUNC_SERVER_H
#define LUTF_FUNC_SERVER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef FUNCNUM
#define FUNCNUM 128
#endif

#define APPNUM 8

typedef int (*func_ptr_t)(void * param, void * md);
int start_func_listener(char *ip, uint16_t port);

//TODO:消息结构体，以及序列化和反序列化问题需要进一步考虑。
struct func_req_msg{
	enum {ADD_FUNCTION, DELETE_FUNCTION, STOP_FUNCTION, RESTORE_FUNCTION} type;
	uint32_t app_id;
	uint32_t func_id;
	char func_path[64];
	char func_name[64];
	char url[128];
};

struct func_resp_msg{
	enum {ADD_FUNCTION_FAILURE, ADD_FUNCTION_SUCCESS} state;
	char rand_url[128];
};

struct function_config {
	enum {FUNCTION_ACTIVE, FUNCTION_STOP, FUNCTION_DEAD} state;
	uint32_t app_id;
	uint32_t func_id;
	char func_path[64];
	char func_name[64];
	char url[128];
	func_ptr_t func_ptr;
};


#endif //LUTF_FUNC_SERVER_H
