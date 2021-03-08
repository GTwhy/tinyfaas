//
// Created by why on 2021/3/4.
//

#include "work_server.h"
#include "coroutine.h"
#include "func_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/supplemental/util/platform.h>
#include <stdio.h>
#include <signal.h>
//TODO:后续改为自动扩容，动态增加work数量
#ifndef WORK_PARALLEL
#define WORK_PARALLEL 128
#endif

/**
 * 异步工作请求
 */
struct work {
	enum {INIT, RECV, WORK, SEND} state;
	nng_aio *aio;
	nng_ctx ctx;
	nng_msg *msg;
	func_ptr_t fp;
};

static void fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}


struct schedule * S;

struct args {
	func_ptr_t func;
	void *param;
};

static void
func_wrapper(struct schedule * S, void *ud) {
	struct args * arg = ud;
	void * rv = arg->func(arg->param);
	//TODO:add operations for return value
}

void handle_sigint(int sig)
{
	// 解析参数(貌似 signal 无法传递参数)
	// 加载程序

	// 创建协程
//	struct args arg3 = { 200 };
//	coroutine_new(S, func_wrapper, &arg3);

}

//int check_work_msg(struct work_msg *msg)
//{
//	if(msg->app_id<0 || msg->app_id>=APPNUM || msg->func_id<0 || msg->func_id>=FUNCNUM)
//		return PARAM_FAULT;
//	return 0;
//}

static int do_work(func_ptr_t fp, void * param)
{
	struct args arg = {fp, param};
	coroutine_new(S, func_wrapper, &arg);
	//TODO:调度可优化，考虑是否每次新加入任务都需要调度
	coroutine_sched(S);
}

static void work_cb(void *arg)
{
	struct work *work = arg;
	nng_msg *    msg;
	int          rv;
	void *       param;

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
			param = nng_msg_body(msg);
//			if (param == NULL) {
//				// bad message, just ignore it.
//				nng_msg_free(msg);
//				nng_ctx_recv(work->ctx, work->aio);
//				return;
//			}
			work->state = WORK;
			do_work(work->fp, param);
			//TODO:wait until work done and return values.
			nng_sleep_aio(0, work->aio);//send msg to worker
			break;
		case WORK:
			msg = NULL;
			if ((rv = nng_msg_alloc(&msg, 0)) != 0) {
				fatal("nng_msg_alloc", rv);
			}
			//TODO:add operations to handle the return values of works.
//			if ((rv = nng_msg_append(msg, work_rv, sizeof(work_rv))) != 0) {
//				fatal("nng_msg_append_u32", rv);
//			}
			work->msg = msg;
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

static struct work * alloc_work(nng_socket sock, func_ptr_t fp)
{
	struct work *w;
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, work_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}
	w->fp = fp;
	w->state = INIT;
	return (w);
}

void stop_work()
{
	coroutine_close(S);
}

int start_work_listener(const char *url, func_ptr_t fp)
{
	nng_socket   sock;
	struct work *works[WORK_PARALLEL];
	int          rv;
	int          i;
	printf("start work listener url : %s\n", url);
	/*  Create the socket. */
	rv = nng_rep0_open(&sock);
	if (rv != 0) {
		fatal("nng_rep0_open", rv);
	}

	for (i = 0; i < WORK_PARALLEL; i++) {
		works[i] = alloc_work(sock, fp);
	}

	if ((rv = nng_listen(sock, url, NULL, 0)) != 0) {
		fatal("nng_listen", rv);
	}

	S = coroutine_open();

	for (i = 0; i < WORK_PARALLEL; i++) {
		work_cb(works[i]); // this starts them going (INIT state)
	}
	return 0;
//	for (;;) {
//		nng_msleep(3600000); // neither pause() nor sleep() portable
//	}
}