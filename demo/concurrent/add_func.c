//
// Created by why on 2021/4/12.
//

#include "../../core/worker/worker_tools.h"
#include <unistd.h>

#define SLEEP_PATH	"../demo/concurrent/sleep.lf"
#define SLEEP_NAME	"work"

char * work_server_url;

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
	printf("work_server url : %s\n", work_server_url);
	free_url(work_server_url);
	return 0;
}