//
// Created by why on 2021/2/24.
//

#include "worker.h"
#include "func_server.h"
#include "work_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/supplemental/util/platform.h>




int main(int argc, char **argv)
{
	//TODO:思考需要初始化的参数并初始化
	int rc;
	if (argc != 2) {
		fprintf(stderr, "function listener: %s <url>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	/* init the function listener */
	rc = start_func_listener(argv[1]);
	exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
	/* init the task listener */
}