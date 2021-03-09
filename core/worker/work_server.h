//
// Created by why on 2021/3/4.
//

#ifndef LUTF_WORK_SERVER_H
#define LUTF_WORK_SERVER_H
#include "func_server.h"
#define PARAM_FAULT -1;

struct mate_data{
	uint32_t data_id;
	uint32_t data_stat;
};
typedef struct mate_data mate_date_s;

int start_work_listener(const char *url, func_ptr_t fp);
#endif //LUTF_WORK_SERVER_H
