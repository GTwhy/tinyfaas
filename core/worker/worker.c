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
	if (argc == 3) {
		//User defined url
		uint16_t port = (uint16_t)atoi(argv[2]);
		printf("Using the default function server url : %s:%u\n", argv[1], port);
		start_func_listener(argv[1], port);
	} else{
		//Default url
		printf("Tips : user defined url: %s <ip> <port>\n", argv[0]);
		printf("Using the default function server url : %s:%d\n", DEFAULT_IP, DEFAULT_PORT);
		/* init the function listener */
		start_func_listener(DEFAULT_IP, DEFAULT_PORT);
	}
}