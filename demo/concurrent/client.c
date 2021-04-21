//
// Created by why on 2021/4/12.
//

#include "../../core/worker/worker_tools.h"

#define SLEEP_PATH	"./sleep.lf"
#define SLEEP_NAME	"work"

struct args{
	int time;
};

char * work_server_url = "tcp://localhost:2673";

int main(int argc, char **argv)
{
	struct args a;

	a.time = atoi(argv[1]);
	void * rv = make_function_request(work_server_url, &a, sizeof(a));
	free_rv(rv);
	printf("wake up\n");
	return 0;
}