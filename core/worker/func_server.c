//
// Created by why on 2021/3/4.
//

#include "func_server.h"
#include "work_server.h"
#include "runtime.h"
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
struct func_task {
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
	struct func_task *func_task = arg;
	nng_msg *    msg;
	int          rv;
	struct func_msg *funcMsg;

	switch (func_task->state) {
		case INIT:
			func_task->state = RECV;
			nng_ctx_recv(func_task->ctx, func_task->aio);
			break;
		case RECV:
			if ((rv = nng_aio_result(func_task->aio)) != 0) {
				fatal("nng_ctx_recv", rv);
			}
			msg = nng_aio_get_msg(func_task->aio);
			funcMsg = (struct func_msg*)nng_msg_body(msg);
			if (funcMsg == NULL) {
				printf("error: funcMsg == NULL\n");
				// bad message, just ignore it.
				nng_msg_free(msg);
				nng_ctx_recv(func_task->ctx, func_task->aio);
				return;
			}
			func_task->msg = msg;
			func_task->state = WORK;
			if(add_func(funcMsg) != 0 ){
				printf("addfunc: param fault!\n");
			}
			//TODO: add msg which contain the return value.
			//nng_aio_begin(func_task->aio);
			nng_sleep_aio(0, func_task->aio);//send msg to func_tasker
			printf("add done!\n");
			break;
		case WORK:
			//TODO:add operations to handle the return values of works.
			nng_aio_set_msg(func_task->aio, func_task->msg);
			func_task->msg   = NULL;
			func_task->state = SEND;
			nng_ctx_send(func_task->ctx, func_task->aio);
			break;
		case SEND:
			if ((rv = nng_aio_result(func_task->aio)) != 0) {
				nng_msg_free(func_task->msg);
				fatal("nng_ctx_send", rv);
			}
			func_task->state = RECV;
			nng_ctx_recv(func_task->ctx, func_task->aio);
			break;
		default:
			fatal("bad state!", NNG_ESTATE);
			break;
	}
}

static struct func_task * alloc_func_task(nng_socket sock)
{
	struct func_task *w;
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
	struct func_task *func_tasks[FUNC_PARALLEL];
	int          rv;
	int          i;

	 
	/*  Create the socket. */
	rv = nng_rep0_open(&sock);
	if (rv != 0) {
		fatal("nng_rep0_open", rv);
	}

	for (i = 0; i < FUNC_PARALLEL; i++) {
		func_tasks[i] = alloc_func_task(sock);
	}
	 
	if ((rv = nng_listen(sock, url, NULL, 0)) != 0) {
		fatal("nng_listen", rv);
	}
	printf("start listen\n");
	for (i = 0; i < FUNC_PARALLEL; i++) {
		func_cb(func_tasks[i]); // this starts them going (INIT state)
	}
	 
	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}