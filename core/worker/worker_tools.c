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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEBUG
#define nIPC_MODE
#define TCP_MODE
#define RAND_URL "rand"
#ifdef TCP_MODE
#define DEFAULT_FUNCTION_IP "127.0.0.1"
#define DEFAULT_FUNCTION_PORT 2672
#endif

static void
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
 * Low-level api used to restore a function service.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
int restore_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id)
{
	size_t req_msg_size = sizeof(struct func_req_msg);
	size_t resp_msg_size = sizeof(struct func_resp_msg);
	struct func_req_msg * req_msg = malloc(req_msg_size);
	struct func_resp_msg * resp_msg = malloc(resp_msg_size);
	ssize_t pos = 0;
	ssize_t len = 0;

	if(func_server_ip == NULL){
		func_server_ip = DEFAULT_FUNCTION_IP;
		func_server_port = DEFAULT_FUNCTION_PORT;
		printf("Using the default function server url : %s:%d\n", DEFAULT_FUNCTION_IP, DEFAULT_FUNCTION_PORT);
	}

	req_msg->type = RESTORE_FUNCTION;
	req_msg->app_id = app_id;
	req_msg->func_id = func_id;

	int sockfd;    //网络套接字
	struct sockaddr_in server_addr;    //服务器地址
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(func_server_ip);
	server_addr.sin_port=htons(func_server_port);

	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		perror("can't connect to server");

	while(pos < req_msg_size)
	{

		len = send(sockfd, req_msg + pos, req_msg_size, 0);
		if (len < 0) {
			printf("client receive data failed\n");
			return -1;
		}
		pos += len;
	}
	free(req_msg);
	memset(resp_msg, 0, resp_msg_size);
	pos = 0;
	while(pos < resp_msg_size)
	{

		len = read(sockfd, resp_msg + pos, resp_msg_size);
		if (len < 0) {
			printf("client receive data failed\n");
			return -2;
		}
		pos += len;
	}

	if(resp_msg->state == RESTORE_FUNCTION_SUCCESS){
		//User need to use free_url() to free the memory of rand_url.
		char * rand_url = malloc(strlen(resp_msg->rand_url)+1);
		strcpy(rand_url, resp_msg->rand_url);
		free(resp_msg);
		close(sockfd);
		printf("RESTORE_FUNCTION_SUCCESS\n");
		return 0;
	}else{
		printf("RESTORE_FUNCTION_FAILURE\n");
		free(resp_msg);
		close(sockfd);
		return -3;
	}
}


/**
 * Low-level api used to stop a function service.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
int stop_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id)
{
	size_t req_msg_size = sizeof(struct func_req_msg);
	size_t resp_msg_size = sizeof(struct func_resp_msg);
	struct func_req_msg * req_msg = malloc(req_msg_size);
	struct func_resp_msg * resp_msg = malloc(resp_msg_size);
	ssize_t pos = 0;
	ssize_t len = 0;

	if(func_server_ip == NULL){
		func_server_ip = DEFAULT_FUNCTION_IP;
		func_server_port = DEFAULT_FUNCTION_PORT;
		printf("Using the default function server url : %s:%d\n", DEFAULT_FUNCTION_IP, DEFAULT_FUNCTION_PORT);
	}

	req_msg->type = STOP_FUNCTION;
	req_msg->app_id = app_id;
	req_msg->func_id = func_id;

	int sockfd;    //网络套接字
	struct sockaddr_in server_addr;    //服务器地址
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(func_server_ip);
	server_addr.sin_port=htons(func_server_port);

	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		perror("can't connect to server");

	while(pos < req_msg_size)
	{

		len = send(sockfd, req_msg + pos, req_msg_size, 0);
		if (len < 0) {
			printf("client receive data failed\n");
			return -1;
		}
		pos += len;
	}
	free(req_msg);
	memset(resp_msg, 0, resp_msg_size);
	pos = 0;
	while(pos < resp_msg_size)
	{

		len = read(sockfd, resp_msg + pos, resp_msg_size);
		if (len < 0) {
			printf("client receive data failed\n");
			return -2;
		}
		pos += len;
	}

	if(resp_msg->state == STOP_FUNCTION_SUCCESS){
		//User need to use free_url() to free the memory of rand_url.
		char * rand_url = malloc(strlen(resp_msg->rand_url)+1);
		strcpy(rand_url, resp_msg->rand_url);
		free(resp_msg);
		close(sockfd);
		printf("STOP_FUNCTION_SUCCESS\n");
		return 0;
	}else{
		printf("STOP_FUNCTION_FAILURE\n");
		free(resp_msg);
		close(sockfd);
		return -3;
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
char * add_new_function(char * func_server_ip, uint16_t func_server_port, char * work_server_url, char * workload_path, char * func_name, int app_id, int func_id)
{
	size_t req_msg_size = sizeof(struct func_req_msg);
	size_t resp_msg_size = sizeof(struct func_resp_msg);
	struct func_req_msg * req_msg = malloc(req_msg_size);
	struct func_resp_msg * resp_msg = malloc(resp_msg_size);
	ssize_t pos = 0;
	ssize_t len = 0;

	//TODO:check the parameters
	if(workload_path == NULL){
		printf("workload_path must be set\n");
		return NULL;
	}

	if(func_name == NULL){
		printf("func_name must be set\n");
		return NULL;
	}

	if(func_server_ip == NULL){
		func_server_ip = DEFAULT_FUNCTION_IP;
		func_server_port = DEFAULT_FUNCTION_PORT;
		printf("Using the default function server url : %s:%d\n", DEFAULT_FUNCTION_IP, DEFAULT_FUNCTION_PORT);
	}

	if(work_server_url == NULL){
		work_server_url = RAND_URL;
		printf("Using the  work server url returned by function server\n");
	}
	req_msg->type = ADD_FUNCTION;
	req_msg->app_id = app_id;
	req_msg->func_id = func_id;
	strcpy(req_msg->url, work_server_url);
	strcpy(req_msg->func_path, workload_path);
	strcpy(req_msg->func_name, func_name);
	int sockfd;    //网络套接字
	struct sockaddr_in server_addr;    //服务器地址
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(func_server_ip);
	server_addr.sin_port=htons(func_server_port);

	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		perror("can't connect to server");

	while(pos < req_msg_size)
	{

		len = send(sockfd, req_msg + pos, req_msg_size, 0);
		if (len < 0) {
			printf("client receive data failed\n");
			return NULL;
		}
		pos += len;
	}
	free(req_msg);
	memset(resp_msg, 0, resp_msg_size);
	pos = 0;
	while(pos < resp_msg_size)
	{

		len = read(sockfd, resp_msg + pos, resp_msg_size);
		if (len < 0) {
			printf("client receive data failed\n");
			return NULL;
		}
		pos += len;
	}
	printf("resp_msg : state: %u url %s\n", resp_msg->state, resp_msg->rand_url);

	if(resp_msg->state == ADD_FUNCTION_SUCCESS){
		//User need to use free_url() to free the memory of rand_url.
		char * rand_url = malloc(strlen(resp_msg->rand_url)+1);
		strcpy(rand_url, resp_msg->rand_url);
		free(resp_msg);
		close(sockfd);
		printf("work url : %s\n", rand_url);
		return rand_url;
	}else{
		printf("ADD_FUNCTION_FAILURE\n");
		free(resp_msg);
		close(sockfd);
		return NULL;
	}
}