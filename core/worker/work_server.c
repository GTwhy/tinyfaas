//
// Created by why on 2021/3/4.
//

#include "work_server.h"
#include "runtime.h"
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
#define WORK_PARALLEL 8
#endif

#define SLEEP_TIME 10

/**
 * 异步工作请求
 */
struct work_task {
	int wid;
	enum {INIT, RECV, WORK, DONE, SEND} state;
	nng_aio *aio;
	nng_ctx ctx;
	nng_msg *msg;
	func_ptr_t fp;
	void * param;
	mate_date_s md;
};

/*
 * Mate_date related to the return value of function.
 */

static void fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

static void
func_wrapper(void * ud) {
	struct work_task * wt_p = ud;
	wt_p->fp(wt_p->param, &wt_p->md);
	wt_p->state = DONE;
}

void handle_sigint(int sig)
{
	// 解析参数(貌似 signal 无法传递参数)
	// 加载程序

	// 创建协程
//	struct args arg3 = { 200 };
//	coroutine_new(S, func_wrapper, &arg3);

}

//int check_work_task_msg(struct work_task_msg *msg)
//{
//	if(msg->app_id<0 || msg->app_id>=APPNUM || msg->func_id<0 || msg->func_id>=FUNCNUM)
//		return PARAM_FAULT;
//	return 0;
//}

static int do_work(struct work_task * work_task)
{
#ifdef DEBUG
	printf("work_task %d start working \n", work_task->wid);
#endif
	brick_new(func_wrapper, (void *)work_task);
	//TODO:调度可优化，考虑是否每次新加入任务都需要调度
}

static void work_task_cb(void *arg)
{
	struct work_task *work_task = arg;
	int          rv;
#ifdef DEBUG
	printf("wid : %d  state : %d\n", work_task->wid, work_task->state);
#endif
	switch (work_task->state) {
		case INIT:
			work_task->state = RECV;
			nng_ctx_recv(work_task->ctx, work_task->aio);
			break;
		case RECV:
			if ((rv = nng_aio_result(work_task->aio)) != 0) {
				fatal("nng_ctx_recv", rv);
			}
			work_task->msg = nng_aio_get_msg(work_task->aio);
			work_task->param = nng_msg_body(work_task->msg);
//			if (param == NULL) {
//				// bad message, just ignore it.
//				nng_msg_free(msg);
//				nng_ctx_recv(work_task->ctx, work_task->aio);
//				return;
//			}
			work_task->state = WORK;
			do_work(work_task);
		case WORK:
			nng_sleep_aio(SLEEP_TIME, work_task->aio);//send msg to worker
			break;
		case DONE:
			if ((rv = nng_msg_alloc(&work_task->msg, 0)) != 0) {
				fatal("nng_msg_alloc", rv);
			}
			//TODO:add operations to handle the return values of works.
//			if ((rv = nng_msg_append(msg, work_task_rv, sizeof(work_task_rv))) != 0) {
//				fatal("nng_msg_append_u32", rv);
//			}
			nng_msg_append(work_task->msg, &work_task->md, sizeof(work_task->md));
			nng_aio_set_msg(work_task->aio, work_task->msg);
			work_task->state = SEND;
			nng_ctx_send(work_task->ctx, work_task->aio);
			break;
		case SEND:
			if ((rv = nng_aio_result(work_task->aio)) != 0) {
				nng_msg_free(work_task->msg);
				fatal("nng_ctx_send", rv);
			}
			nng_msg_free(work_task->msg);
			work_task->msg   = NULL;
			memset(&work_task->md,0,sizeof(mate_date_s));
			work_task->param = NULL;
			work_task->state = RECV;
			nng_ctx_recv(work_task->ctx, work_task->aio);
			break;
		default:
			fatal("bad state!", NNG_ESTATE);
			break;
	}
}

static struct work_task * alloc_work_task(nng_socket sock, func_ptr_t fp)
{
	struct work_task *w;
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, work_task_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}
	w->fp = fp;
	w->state = INIT;
	return (w);
}

int start_work_listener(const char *url, func_ptr_t fp)
{
	nng_socket   sock;
	struct work_task *work_tasks[WORK_PARALLEL];
	int          rv;
	int          i;
	printf("start work listener url : %s\n", url);
	/*  Create the socket. */

	rv = nng_rep0_open(&sock);
	if (rv != 0) {
		fatal("nng_rep0_open", rv);
	}

	for (i = 0; i < WORK_PARALLEL; i++) {
		work_tasks[i] = alloc_work_task(sock, fp);
		work_tasks[i]->wid = i;
	}
	printf("Alloc work_tasks done\n");
	if ((rv = nng_listen(sock, url, NULL, 0)) != 0) {
		fatal("nng_listen", rv);
	}
	printf("start listening to %s\n", url);
	runtime_init();

	for (i = 0; i < WORK_PARALLEL; i++) {
		work_task_cb(work_tasks[i]); // this starts them going (INIT state)
	}
	printf("Work_tasks init done\n");
	return 0;
//	for (;;) {
//		nng_msleep(3600000); // neither pause() nor sleep() portable
//	}
}