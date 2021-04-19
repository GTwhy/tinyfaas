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


void reques(void)
{
	struct args a;
	a.time = 100;
	void *rv = make_function_request(work_server_url, &a, sizeof(a));
	mate_date_s * md = (mate_date_s *)rv;
	printf("request_test get return_value : %u, %u\n", md->data_id, md->data_stat);
	free_rv(rv);
}

int main(int argc, char **argv)
{
	if (argc == 3) {
		//User defined url
		uint16_t port = (uint16_t)atoi(argv[2]);
		printf("Function server url : %s:%u\n", argv[1], port);
		work_server_url = add_new_function(argv[1],port, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
	} else{
		//Default url
		printf("Tips : user defined url: %s <ip> <port>\n", argv[0]);
		work_server_url = add_new_function(NULL,0, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
	}
	printf("add func\n");
	sleep(2);
	reques();
	printf("stop func\n");
	stop_function(NULL,0,0,0);
	sleep(2);
	printf("restore func\n");
	restore_function(NULL,0,0,0);
	reques();
	printf("delete func\n");
	sleep(2);
	delete_function(NULL,0,0,0);
	reques();
	free_url(work_server_url);
	return 0;
}