//
// Created by why on 2021/2/24.
//

#include "worker.h"
#include "func_server.h"
#include <stdio.h>
#include <stdlib.h>
#define DEFAULT_FUNCTION_URL "tcp://localhost:2672"

int main(int argc, char **argv)
{
	//TODO:思考需要初始化的参数并初始化
	int rc;
	if (argc != 2) {
		printf("Tips : user defined url: %s <url>\n", argv[0]);
		printf("Using the default function server url : %s\n", DEFAULT_FUNCTION_URL);
		/* init the function listener */
		start_func_listener(DEFAULT_FUNCTION_URL);
	} else{
		/* init the function listener */
		start_func_listener(argv[1]);
	}
}