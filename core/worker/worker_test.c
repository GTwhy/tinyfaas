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

#define COUNT 1000
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
	nng_time   start;
	nng_time   end;
	struct func_msg_trans funcMsg;

	funcMsg.app_id = 0;
	funcMsg.func_id = 0;
	strcpy(funcMsg.url, work_url);
	strcpy(funcMsg.func_path, "./libwork.so");
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
	 
	if ((rv = nng_recvmsg(sock, &msg, 0)) != 0) {
		fatal("nng_recvmsg", rv);
	}
	 
	end = nng_clock();
	nng_msg_free(msg);
	nng_close(sock);

	printf("Func request took %u milliseconds.\n", (uint32_t)(end - start));
	return (0);
}

int
work_client(const char *work_url)
{
	nng_socket sock;
	int        rv;
	nng_msg *  msg;

	nng_time   start;
	nng_time   end;
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
	start = nng_clock();
	for(int i = 0; i < COUNT; i++){
		if ((rv = nng_msg_append(msg, &arg, sizeof(arg))) != 0) {
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
		mate_date_s * mp = (mate_date_s *)nng_msg_body(recv_msg);
		printf("Content of recv_msg : %u  %u\n", mp->data_id, mp->data_stat);
		nng_msg_free(recv_msg);
	}
	end = nng_clock();
	nng_msg_free(msg);
	nng_close(sock);

	printf("Work request took %u milliseconds.\n", (uint32_t)(end - start));
	return (0);
}

int
main(int argc, char **argv)
{
	int rc;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <func_url> <work_url>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	func_client(argv[1], argv[2]);
	sleep(3);
	rc = work_client(argv[2]);
	exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}