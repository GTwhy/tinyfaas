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
#define THREAD_NUMBER 4
#define COUNT 10000
#define DEBUG
#define nIPC_MODE
#define TCP_MODE
#define RAND_URL "rand"
#ifdef TCP_MODE
#define DEFAULT_FUNCTION_URL "tcp://localhost:2672"
#endif

char rand_url[128];

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

/*  The client runs just once, and then returns. */
int
func_client(const char *func_url, const char *work_url)
{
	nng_socket sock;
	int        rv;
	nng_msg *  msg;
	struct func_msg_trans funcMsg;

	funcMsg.app_id = 0;
	funcMsg.func_id = 0;
	strcpy(funcMsg.url, work_url);
	strcpy(funcMsg.func_path, "./libwork_ppdi.so");
	strcpy(funcMsg.func_name, "work");

	if ((rv = nng_req0_open(&sock)) != 0) {
		fatal("nng_req0_open", rv);
	}

	if ((rv = nng_dial(sock, func_url, NULL, 0)) != 0) {
		fatal("nng_dial", rv);
	}

	start = nng_clock();

	if ((rv = nng_msg_alloc(&msg, 0)) != 0) {
		fatal("nng_msg_alloc", rv);
	}

	if ((rv = nng_msg_append(msg, &funcMsg, sizeof(funcMsg))) != 0) {
		fatal("nng_msg_append_u32", rv);
	}

	if ((rv = nng_sendmsg(sock, msg, 0)) != 0) {
		fatal("nng_send", rv);
	}

	struct nng_msg * recv_msg;
	nng_msg_alloc(&recv_msg, 0);
	if ((rv = nng_recvmsg(sock, &recv_msg, 0)) != 0) {
		fatal("nng_recvmsg", rv);
	}
	struct func_resp_msg * fmp = (struct func_resp_msg *)nng_msg_body(recv_msg);
	if(fmp->state == ADD_FUNCTION_SUCCESS){
		strcpy(rand_url, fmp->rand_url);
		printf("Get rand url from server : %s\n", rand_url);
	}else{
		printf("ADD_FUNCTION_FAILURE state: %d  rand_url : %s\n", fmp->state, fmp->rand_url);
		exit(1);
	}

	end = nng_clock();
	nng_msg_free(msg);
	nng_close(sock);

	printf("Func request took %u milliseconds.\n", (uint32_t)(end - start));
	return (0);
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
#ifdef DEBUG
		pthread_mutex_lock(&th_mutex);
		printf("send count : %d\n", ++send_count);
		pthread_mutex_unlock(&th_mutex);
#endif

		nng_msg_alloc(&recv_msg, 0);
		if ((rv = nng_recvmsg(sock, &recv_msg, 0)) != 0) {
			fatal("nng_recvmsg", rv);
		}
		mate_date_s * mp = (mate_date_s *)nng_msg_body(recv_msg);
		printf("Content of recv_msg : %u  %u\n", mp->data_id, mp->data_stat);
#ifdef DEBUG
		pthread_mutex_lock(&th_mutex);
		printf("count : %d\n", count);

		if(++count >= COUNT)
			end_print();
		pthread_mutex_unlock(&th_mutex);
#endif
	}
	return (0);
}



int
main(int argc, char **argv)
{

	if (argc != 3) {
		printf("Tips : user defined url: %s <url>\n", argv[0]);
		printf("Using the default function server url : %s\n", DEFAULT_FUNCTION_URL);
		func_client(DEFAULT_FUNCTION_URL, RAND_URL);
	} else{
		func_client(argv[1], argv[2]);
	}
	sleep(3);
	count = 0;
	pthread_mutex_init(&th_mutex, NULL);
	pthread_t pts[THREAD_NUMBER];

	for (int i = 0; i < THREAD_NUMBER; ++i) {
		pthread_create(&pts[i], NULL, work_client, rand_url);
	}
	start = nng_clock();
	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}