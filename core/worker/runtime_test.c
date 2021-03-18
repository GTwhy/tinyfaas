//
// Created by why on 2021/3/18.
//
//
// Created by why on 2021/3/4.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/util/platform.h>
#include "work_server.h"
#include <pthread.h>
#include <sys/wait.h>
#define THREAD_NUMBER 1
#define COUNT 10000
#define DEBUG


//TODO:消息结构体，以及序列化和反序列化问题需要进一步考虑。
struct func_msg_trans{
	uint32_t app_id;
	uint32_t func_id;
	char func_path[64];
	char func_name[64];
	char url[128];
};

struct args{
	int m;
	int n;
};

nng_time   start;
nng_time   end;

static int count;

static int send_count = 0;

pthread_mutex_t th_mutex;

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

void end_print()
{
	end = nng_clock();
	printf("Func request took %u milliseconds.\n", (uint32_t)(end - start));
	exit(0);
}

void *
work_client(void * url)
{
	const char * work_url = url;
	nng_socket sock;
	int        rv;
	nng_msg *  msg;
	struct args arg;
	arg.m = 100;
	arg.n = 999;
	if ((rv = nng_req0_open(&sock)) != 0) {
		fatal("nng_req0_open", rv);
	}

	if ((rv = nng_dial(sock, work_url, NULL, 0)) != 0) {
		fatal("nng_dial", rv);
	}

	if ((rv = nng_msg_alloc(&msg, 0)) != 0) {
		fatal("nng_msg_alloc", rv);
	}
	struct nng_msg * recv_msg;
	for(int i = 0; i < COUNT; i++){
		if ((rv = nng_msg_append(msg, &arg, sizeof(arg))) != 0) {
			fatal("nng_msg_append_u32", rv);
		}

		if ((rv = nng_sendmsg(sock, msg, 0)) != 0) {
			fatal("nng_send", rv);
		}
		pthread_mutex_lock(&th_mutex);
//#ifdef DEBUG
//		printf("send count : %d\n", ++send_count);
//#endif
		pthread_mutex_unlock(&th_mutex);
		nng_msg_alloc(&recv_msg, 0);
		if ((rv = nng_recvmsg(sock, &recv_msg, 0)) != 0) {
			fatal("nng_recvmsg", rv);
		}
		mate_date_s * mp = (mate_date_s *)nng_msg_body(recv_msg);
		printf("Content of recv_msg : %u  %u\n", mp->data_id, mp->data_stat);
		pthread_mutex_lock(&th_mutex);
#ifdef DEBUG
		printf("count : %d\n", count);
#endif
		if(++count >= COUNT)
			end_print();
		pthread_mutex_unlock(&th_mutex);
	}
	return (0);
}



int
main(int argc, char **argv)
{

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <work_url>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	count = 0;
	pthread_mutex_init(&th_mutex, NULL);
	pthread_t pts[THREAD_NUMBER];

	for (int i = 0; i < THREAD_NUMBER; ++i) {
		pthread_create(&pts[i], NULL, work_client, argv[1]);
	}
	start = nng_clock();
	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}