//
// Created by why on 2021/2/24.
//

#include "worker.h"
#include "func_server.h"
#include <stdio.h>
#include <stdlib.h>
#define DEFAULT_IP "0.0.0.0"
#define DEFAULT_PORT 2672

int main(int argc, char **argv)
{
	//TODO:思考需要初始化的参数并初始化
	if (argc != 2) {
		printf("Tips : user defined url: %s <url>\n", argv[0]);
		printf("Using the default function server url : %s:%d\n", DEFAULT_IP, DEFAULT_PORT);
		/* init the function listener */
		start_func_listener(DEFAULT_IP, DEFAULT_PORT);
	} else{
		/* init the function listener */
		//TODO:User defined url.
		//start_func_listener(argv[1]);
	}
}