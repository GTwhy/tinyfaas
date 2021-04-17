//
// Created by why on 2021/4/12.
//

#include "worker_tools.h"
#include <unistd.h>

#define SLEEP_PATH	"./libwork_sleep.so"
#define SLEEP_NAME	"work"

struct args{
	int time;
};

char * work_server_url;

void add_func_test(void)
{
	work_server_url = add_new_function(NULL,0, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
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
	printf("add func\n");
	add_func_test();
	sleep(2);
	request_test();
	printf("stop func\n");
	stop_function(NULL,0,0,0);
	sleep(2);
	printf("restore func\n");
	restore_function(NULL,0,0,0);
	request_test();
	printf("delete func\n");
	sleep(2);
	delete_function(NULL,0,0,0);
	request_test();
	free_url(work_server_url);
	return 0;
}