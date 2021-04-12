//
// Created by why on 2021/4/12.
//

#include "worker_tools.h"

#define SLEEP_PATH	"./libwork_sleep.so"
#define SLEEP_NAME	"work"

struct args{
	int time;
};

char * work_server_url;

void add_func_test(void)
{
	struct args a;
	a.time = 100;
	work_server_url = add_new_function(NULL, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
	printf("add_func_test get work_server_url : %s\n", work_server_url);
}

void request_test(void)
{
	struct args a;
	a.time = 100;
	void *rv = make_function_request(work_server_url, &a, sizeof(a));
	mate_date_s * md = (mate_date_s *)rv;
	printf("request_test get return_value : %u, %u\n", md->data_id, md->data_stat);
	free_rv(rv);
}

int main(void)
{
	add_func_test();
	request_test();
	free_url(work_server_url);
	return 0;
}