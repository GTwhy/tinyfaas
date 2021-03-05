//
// Created by why on 2021/3/4.
//

#include "func_server.h"
#include "work_server.h"
#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/supplemental/util/platform.h>
#include <dlfcn.h>
#include <string.h>
#ifndef FUNC_PARALLEL
#define FUNC_PARALLEL 8
#endif

func_ptr_t func_ptrs[APPNUM][FUNCNUM] = {{NULL}};
/**
 * 异步工作请求
 */
struct work {
	enum {INIT, RECV, WORK, SEND} state;
	nng_aio *aio;
	nng_ctx ctx;
	nng_msg *msg;
};

static void fatal(const char *func, int rv)
{
	printf("time: %u", (uint32_t)nng_clock());
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

void debug_print()
{
	static int num = 0;
	printf("time: %u step : %d\n",(uint32_t)nng_clock(), ++num);
}

int check_func_msg(struct func_msg *msg)
{
	//TODO:可细化参数检查，增加错误输出。
	if(msg->app_id<0 || msg->app_id>=APPNUM || msg->func_id<0 || msg->func_id>=FUNCNUM || msg->func_path == NULL || msg->func_name == NULL || msg->url == NULL)
	{
		printf("path:%s\n name:%s\n url:%s\n",msg->func_path,msg->func_name,msg->url);
		return PARAM_FAULT;
	}
	return 0;
}

int add_func(struct func_msg *msg)
{
	printf("add func\n");
	if(check_func_msg(msg)<0){
		return PARAM_FAULT;
	}
	printf("path:%s\n name:%s\n url:%s\n",msg->func_path,msg->func_name,msg->url);
	void* lib = dlopen(msg->func_path, RTLD_LAZY);
	func_ptr_t func = dlsym(lib, msg->func_name);
	func_ptrs[msg->app_id][msg->func_id] = func;
	start_work_listener(msg->url, func);
	return 0;
}

void clean_func(uint32_t aid, uint32_t fid)
{
	dlclose(func_ptrs[aid][fid]);
}

void func_cb(void *arg)
{
	struct work *work = arg;
	nng_msg *    msg;
	int          rv;
	struct func_msg *funcMsg;

	switch (work->state) {
		case INIT:
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		case RECV:
			if ((rv = nng_aio_result(work->aio)) != 0) {
				fatal("nng_ctx_recv", rv);
			}
			msg = nng_aio_get_msg(work->aio);
			funcMsg = (struct func_msg*)nng_msg_body(msg);
			if (funcMsg == NULL) {
				printf("error: funcMsg == NULL\n");
				// bad message, just ignore it.
				nng_msg_free(msg);
				nng_ctx_recv(work->ctx, work->aio);
				return;
			}
			work->msg = msg;
			work->state = WORK;
			if(add_func(funcMsg) != 0 ){
				printf("addfunc: param fault!\n");
			}
			//TODO: add msg which contain the return value.
			//nng_aio_begin(work->aio);
			nng_sleep_aio(0, work->aio);//send msg to worker
			printf("add done!\n");
			break;
		case WORK:
			//TODO:add operations to handle the return values of works.
			nng_aio_set_msg(work->aio, work->msg);
			work->msg   = NULL;
			work->state = SEND;
			nng_ctx_send(work->ctx, work->aio);
			break;
		case SEND:
			if ((rv = nng_aio_result(work->aio)) != 0) {
				nng_msg_free(work->msg);
				fatal("nng_ctx_send", rv);
			}
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		default:
			fatal("bad state!", NNG_ESTATE);
			break;
	}
}

static struct work * alloc_work(nng_socket sock)
{
	struct work *w;
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, func_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}
	w->state = INIT;
	return (w);
}

int start_func_listener(const char *url){
	nng_socket   sock;
	struct work *works[FUNC_PARALLEL];
	int          rv;
	int          i;

	 
	/*  Create the socket. */
	rv = nng_rep0_open(&sock);
	if (rv != 0) {
		fatal("nng_rep0_open", rv);
	}

	for (i = 0; i < FUNC_PARALLEL; i++) {
		works[i] = alloc_work(sock);
	}
	 
	if ((rv = nng_listen(sock, url, NULL, 0)) != 0) {
		fatal("nng_listen", rv);
	}
	printf("start listen\n");
	for (i = 0; i < FUNC_PARALLEL; i++) {
		func_cb(works[i]); // this starts them going (INIT state)
	}
	 
	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}