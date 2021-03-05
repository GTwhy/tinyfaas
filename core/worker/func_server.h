//
// Created by why on 2021/3/4.
//

#ifndef LUTF_FUNC_SERVER_H
#define LUTF_FUNC_SERVER_H
#include <stdint.h>

#ifndef FUNCNUM
#define FUNCNUM 128
#endif

#define APPNUM 8

typedef void * (*func_ptr_t)(void *);
func_ptr_t func_ptrs[APPNUM][FUNCNUM];
int start_func_listener(const char *url);

//TODO:消息结构体，以及序列化和反序列化问题需要进一步考虑。
struct func_msg{
	uint32_t app_id;
	uint32_t func_id;
	char func_path[64];
	char func_name[64];
	char url[128];
};


#endif //LUTF_FUNC_SERVER_H
