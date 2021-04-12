//
// Created by why on 2021/4/12.
//

#include "worker_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/util/platform.h>

#define DEBUG
#define nIPC_MODE
#define TCP_MODE
#define RAND_URL "rand"
#ifdef TCP_MODE
#define DEFAULT_FUNCTION_URL "tcp://localhost:2672"
#endif

#ifdef IPC_MODE
#define DEFAULT_FUNCTION_URL "ipc:///tmp/2672"
#endif

//TODO:消息结构体，以及序列化和反序列化问题需要进一步考虑。
struct func_msg_trans{
	uint32_t app_id;
	uint32_t func_id;
	char func_path[64];
	char func_name[64];
	char url[128];
};

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

/**
 * Send a function request to the work_server.
 * @param work_server_url	The work_server's url
 * @param args				Args send to the function.
 * @param args_size 	  	Size of args.
 * @return					The pointer to the value returned from work_server(needs free after used).
 */
void * make_function_request(void * work_server_url, void * args, size_t args_size)
{
	nng_socket sock;
	int        rc;
	nng_msg *  msg;
	if ((rc = nng_req0_open(&sock)) != 0) {
		fatal("nng_req0_open", rc);
	}

	if ((rc = nng_dial(sock, work_server_url, NULL, 0)) != 0) {
		fatal("nng_dial", rc);
	}

	if ((rc = nng_msg_alloc(&msg, 0)) != 0) {
		fatal("nng_msg_alloc", rc);
	}

	if ((rc = nng_msg_append(msg, args, args_size)) != 0) {
		fatal("nng_msg_append_u32", rc);
	}

	if ((rc = nng_sendmsg(sock, msg, 0)) != 0) {
		fatal("nng_send", rc);
	}

	nng_msg_clear(msg);
	if ((rc = nng_recvmsg(sock, &msg, 0)) != 0) {
		fatal("nng_recvmsg", rc);
	}

	size_t rv_size = sizeof(nng_msg_body(msg));
	void * return_value = malloc(rv_size);
	memcpy(return_value, nng_msg_body(msg), rv_size);
	nng_msg_free(msg);
	return return_value;
}

void free_rv(void * rv)
{
	if(rv != NULL){
		free(rv);
	}
}

void free_url(char * url)
{
	if (url != NULL){
		free(url);
	}
}

/**
 * Low-level api used to upload a function to set a function service.
 * And this api is not created for developers, but called by the orchestrator that knows the url of worker's func_server.
 * You can set the params manually for testing.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param work_server_url	The url of the worker's work_server, which listening requests of the workload.
 * @param workload_path		The path of the workload like "./workload.so"
 * @param func_name			The name of the function contained in the workload file.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
char * add_new_function(char * func_server_url, char * work_server_url, char * workload_path, char * func_name, int app_id, int func_id)
{
	nng_socket sock;
	int        rv;
	nng_msg *  msg;
	struct func_msg_trans funcMsg;
	//TODO:check the parameters
	if(workload_path == NULL){
		printf("workload_path must be set\n");
		return NULL;
	}

	if(func_name == NULL){
		printf("func_name must be set\n");
		return NULL;
	}

	if(func_server_url == NULL){
		func_server_url = DEFAULT_FUNCTION_URL;
		printf("Using the default function server url : %s\n", DEFAULT_FUNCTION_URL);
	}

	if(work_server_url == NULL){
		work_server_url = RAND_URL;
		printf("Using the  work server url returned by function server\n");
	}
	funcMsg.app_id = app_id;
	funcMsg.func_id = func_id;
	strcpy(funcMsg.url, work_server_url);
	strcpy(funcMsg.func_path, workload_path);
	strcpy(funcMsg.func_name, func_name);

	if ((rv = nng_req0_open(&sock)) != 0) {
		fatal("nng_req0_open", rv);
	}

	if ((rv = nng_dial(sock, func_server_url, NULL, 0)) != 0) {
		fatal("nng_dial", rv);
	}

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
	struct func_resp_msg * fmp = nng_msg_body(msg);
	if(fmp->state == ADD_FUNCTION_SUCCESS){
		//User need to use free_url() to free the memory of rand_url.
		char * rand_url = malloc(strlen(fmp->rand_url));
		strcpy(rand_url, fmp->rand_url);
		nng_msg_free(msg);
		nng_close(sock);
		printf("work url : %s\n", rand_url);
		return rand_url;
	}else{
		printf("ADD_FUNCTION_FAILURE\n");
		nng_msg_free(msg);
		nng_close(sock);
		return NULL;
	}
}