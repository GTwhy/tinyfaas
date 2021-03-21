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
#define FUNC_PARALLEL 8
#define IPC_MODE
#define nTCP_MODE
#ifdef IPC_MODE
#define DEFAULT_WORK_URL "ipc:///tmp/267"
#endif

#ifdef TCP_MODE
#define DEFAULT_WORK_URL "tcp://localhost:267"
#endif

#define DEFAULT_FUNCTION_NUMBER 100
int port_base_number = 2;
func_ptr_t pptr;
struct function_config * function_config_list[DEFAULT_FUNCTION_NUMBER];
int function_count = 0;
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

int check_func_msg(struct func_req_msg *msg)
{
	//TODO:可细化参数检查，增加错误输出。
	if(msg->app_id<0 || msg->app_id>=APPNUM || msg->func_id<0 || msg->func_id>=FUNCNUM || msg->func_path == NULL || msg->func_name == NULL || msg->url == NULL)
	{
		printf("PARAM_FAULT :  path:%s\n name:%s\n url:%s\n",msg->func_path,msg->func_name,msg->url);
		return PARAM_FAULT;
	}
	return 0;
}

struct function_config * function_init(struct func_req_msg *msg)
{
	struct function_config * fp = malloc(sizeof(struct function_config));
	memset(fp, 0, sizeof(struct function_config));
	memcpy(fp->func_name, msg->func_name, sizeof(msg->func_name));
	memcpy(fp->func_path, msg->func_path, sizeof(msg->func_path));
	fp->app_id = msg->app_id;
	fp->func_id = msg->func_id;
	if(strlen(msg->url) < 8) {
		//make a new url for this function
		if (port_base_number > 99) {
			printf("Maximum number of ports reached\n");
			return NULL;
		}
		char num_str[3];
		sprintf(num_str, "%d", ++port_base_number);
		strcat(fp->url, DEFAULT_WORK_URL);
		strcat(fp->url, num_str);
	} else{
		//use the user defined url
		memcpy(fp->url, msg->url, strlen(msg->url));
	}
	void* lib = dlopen(msg->func_path, RTLD_LAZY);
	func_ptr_t func = dlsym(lib, msg->func_name);
	fp->func_ptr = func;
	pptr = func;
	return fp;
}

int function_new(struct func_req_msg *msg, struct func_resp_msg * resp_msg)
{
	struct function_config * fp = NULL;
	printf("New function is coming\n");
	if(check_func_msg(msg)<0){
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -1;
	}
	if(function_count >= DEFAULT_FUNCTION_NUMBER){
		printf("Maximum number of functions reached\n");
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -2;
	}
	fp = function_init(msg);
	if (fp->func_ptr == NULL){
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -3;
	}
	function_config_list[function_count++] = fp;

	printf("New function path:%s  name:%s  url:%s\n", fp->func_path, fp->func_name, fp->url);
	resp_msg->state = ADD_FUNCTION_SUCCESS;
	strcpy(resp_msg->rand_url, fp->url);
	start_work_listener(fp->url, fp->func_ptr);
	return 0;
}
//
//void clean_func(uint32_t aid, uint32_t fid)
//{
//	dlclose(func_ptrs[aid][fid]);
//}

void func_cb(void *arg)
{
	struct func_task *func_task = arg;
	nng_msg *    msg;
	int          rv;
	struct func_req_msg *funcMsg;

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
			funcMsg = (struct func_req_msg*)nng_msg_body(msg);
			if (funcMsg == NULL) {
				printf("error: funcMsg == NULL\n");
				// bad message, just ignore it.
				nng_msg_free(msg);
				nng_ctx_recv(func_task->ctx, func_task->aio);
				return;
			}
			struct func_resp_msg * resp_smg = malloc(sizeof(struct func_resp_msg));
			function_new(funcMsg, resp_smg);
			if(resp_smg->state == ADD_FUNCTION_SUCCESS){
				printf("ADD_FUNCTION_SUCCESS\n");
				func_task->state = WORK;
			} else{
				printf("ADD_FUNCTION_FAILURE\n");
				func_task->state = WORK;
			}
			nng_msg_alloc(&func_task->msg, 0);
			nng_msg_append(func_task->msg, resp_smg, sizeof(struct func_resp_msg));
			printf("content of resp_msg : %d  %s\n", resp_smg->state,resp_smg->rand_url);
			//nng_aio_begin(func_task->aio);
			nng_sleep_aio(0, func_task->aio);//send msg to func_tasker
			break;
		case WORK:
			//TODO:add operations to handle the return values of works.
			nng_aio_set_msg(func_task->aio, func_task->msg);
			func_task->state = SEND;
			nng_ctx_send(func_task->ctx, func_task->aio);
			break;
		case SEND:
			if ((rv = nng_aio_result(func_task->aio)) != 0) {
				nng_msg_free(func_task->msg);
				fatal("nng_ctx_send", rv);
			}
			nng_msg_free(func_task->msg);
			func_task->msg = NULL;
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