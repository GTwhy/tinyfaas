//
// Created by why on 2021/3/4.
//

#include "func_server.h"
#include "work_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define nIPC_MODE
#define TCP_MODE
#ifdef IPC_MODE
#define DEFAULT_WORK_URL "ipc:///tmp/267"
#endif

#ifdef TCP_MODE
#define DEFAULT_WORK_URL "tcp://localhost:267"
#endif
#define DEFAULT_FUNCTION_NUMBER 100
int port_base_number = 2;
struct function_config * function_config_list[DEFAULT_FUNCTION_NUMBER] = {NULL};
struct work_server * work_server_list[DEFAULT_FUNCTION_NUMBER] = {NULL};
int function_count = 0;
int work_server_count = 0;

struct work_server {
	enum {WORK_SERVER_ACTIVE, WORK_SERVER_STOP, WORK_SERVER_DEAD} state;
	pid_t pid;
	int wid;
	struct function_config * func_conf;
};

void req_msg_print(struct func_req_msg * msg)
{
	printf("req_msg_print:\n");
	printf("\t req_msg type : %u\n",msg->type);
	printf("\t app_id:%d\n", msg->app_id);
	printf("\t func_id:%d\n", msg->func_id);
	printf("\t func_path:%s\n", msg->func_path);
	printf("\t func_name:%s\n", msg->func_name);
}

int check_func_msg(struct func_req_msg *msg)
{
	//TODO:可细化参数检查，增加错误输出。
	if(msg->app_id<0 || msg->app_id>=APPNUM || msg->func_id<0 || msg->func_id>=FUNCNUM || msg->func_path == NULL || msg->func_name == NULL || msg->url == NULL)
	{
		printf("req_msg : PARAM_FAULT :  path:%s\n name:%s\n url:%s\n",msg->func_path,msg->func_name,msg->url);
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
	fp->func_id = function_count;
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
	void* dlp = dlopen(msg->func_path, RTLD_LAZY);
	func_ptr_t func = dlsym(dlp, msg->func_name);
	fp->func_ptr = func;
	fp->dlp = dlp;

	printf("New function path:%s  name:%s  url:%s\n", fp->func_path, fp->func_name, fp->url);
	return fp;
}

struct work_server * work_server_start(struct function_config * fp)
{
	pid_t pid;
	struct work_server * wp = malloc(sizeof(struct work_server));
	memset(fp, 0, sizeof(struct work_server));
	pid = fork();
	if(pid == 0){
		//work_server process
		start_work_listener(fp->url, fp->func_ptr);
	} else if (pid > 0){
		//func_server process
		wp->pid = pid;
		wp->wid = work_server_count;
		wp->func_conf = fp;
		wp->state = WORK_SERVER_ACTIVE;
		return wp;
	} else{
		//error
		printf("work_server_start fault\n");
		return NULL;
	}
}



/**
 * Init a function_config and start up a work_server for this function.
 * The two steps may be split in the future.
 * @param req_msg from function uploader.
 * @param resp_msg send to function uploader.
 * @return Return 0 if successful
 */
int function_new(struct func_req_msg *req_msg, struct func_resp_msg * resp_msg)
{
	struct function_config * fp = NULL;
	struct work_server * wp = NULL;
	printf("function_new\n");
	req_msg_print(req_msg);
	if(check_func_msg(req_msg) < 0){
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -1;
	}
	if(function_count >= DEFAULT_FUNCTION_NUMBER){
		printf("Maximum number of functions reached\n");
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -2;
	}
	fp = function_init(req_msg);

	if (fp->func_ptr == NULL){
		printf("Can not get function\n");
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -3;
	}

	wp = work_server_start(fp);
	if (wp == NULL){
		printf("work_server_start failed\n");
		resp_msg->state = ADD_FUNCTION_FAILURE;
		return -4;
	}
	function_config_list[function_count++] = fp;
	work_server_list[work_server_count++] = wp;
	resp_msg->state = ADD_FUNCTION_SUCCESS;
	strcpy(resp_msg->rand_url, fp->url);
	return 0;
}



/**
 * Stop the work_server and function.
 * The two steps may be split in the future.
 * @param req_msg from function uploader.
 * @param resp_msg send to function uploader.
 * @return Return 0 if successful
 */
int function_stop(struct func_req_msg *req_msg, struct func_resp_msg * resp_msg)
{
	struct work_server * wsp = NULL;
	printf("function_stop\n");
	req_msg_print(req_msg);
	resp_msg->state = STOP_FUNCTION_FAILURE;
	for (int i = 0; i < work_server_count; ++i) {
		wsp = work_server_list[i];
		if (wsp != NULL){
			if (wsp->pid > 0 && wsp->state == WORK_SERVER_ACTIVE){
				if(wsp->func_conf->app_id == req_msg->app_id && wsp->func_conf->func_id == req_msg->func_id){
					resp_msg->state = STOP_FUNCTION_SUCCESS;
					//send signal to work_server and let it stop.
					kill(wsp->pid, SIGSTOP);
					printf("stoped work_server's pid: %d\n", wsp->pid);
					wsp->state = WORK_SERVER_STOP;
					//TODO:Split work_server's state and function's state.
					wsp->func_conf->state = FUNCTION_STOP;
				}
			}
		}
	}
	if (resp_msg->state == STOP_FUNCTION_FAILURE){
		return -1;
	}
	return 0;
}


/**
 * Restore the work_server and function.
 * The two steps may be split in the future.
 * @param req_msg from function uploader.
 * @param resp_msg send to function uploader.
 * @return Return 0 if successful
 */
int function_restore(struct func_req_msg *req_msg, struct func_resp_msg * resp_msg) {
	struct work_server *wsp = NULL;
	printf("function_stop\n");
	req_msg_print(req_msg);
	resp_msg->state = RESTORE_FUNCTION_FAILURE;
	for (int i = 0; i < work_server_count; ++i) {
		wsp = work_server_list[i];
		if (wsp != NULL) {
			if (wsp->pid > 0 && wsp->state == WORK_SERVER_STOP) {
				if (wsp->func_conf->app_id == req_msg->app_id && wsp->func_conf->func_id == req_msg->func_id) {
					resp_msg->state = RESTORE_FUNCTION_SUCCESS;
					//send signal to work_server and let it stop.
					kill(wsp->pid, SIGCONT);
					printf("restored work_server's pid: %d\n", wsp->pid);
					wsp->state = WORK_SERVER_ACTIVE;
					//TODO:Split work_server's state and function's state.
					wsp->func_conf->state = FUNCTION_ACTIVE;
				}
			}
		}
	}
	if (resp_msg->state == RESTORE_FUNCTION_FAILURE) {
		return -1;
	}
	return 0;
}

/**
 * Delete data of work_server after it dead.
 * @param wsp
 */
void work_server_clean(struct work_server * wsp)
{
	work_server_list[wsp->wid] = NULL;
	function_config_list[wsp->func_conf->fid] = NULL;
	dlclose(wsp->func_conf->dlp);
	free(wsp->func_conf);
	free(wsp);
	wsp = NULL;
}


/**
 * Init a function_config and start up a work_server for this function.
 * The two steps may be split in the future.
 * @param req_msg from function uploader.
 * @param resp_msg send to function uploader.
 * @return
 */
int function_delete(struct func_req_msg *req_msg, struct func_resp_msg * resp_msg)
{
	struct work_server * wsp = NULL;
	printf("function_delete\n");
	req_msg_print(req_msg);
	resp_msg->state = DELETE_FUNCTION_FAILURE;
	for (int i = 0; i < work_server_count; ++i) {
		wsp = work_server_list[i];
		if (wsp != NULL) {
			if (wsp->pid > 0 && wsp->state != WORK_SERVER_DEAD) {
				if (wsp->func_conf->app_id == req_msg->app_id && wsp->func_conf->func_id == req_msg->func_id) {
					resp_msg->state = DELETE_FUNCTION_SUCCESS;
					//send signal to work_server and let it stop.
					kill(wsp->pid, SIGKILL);
					printf("deleted work_server's pid: %d\n", wsp->pid);
					//Maybe you could not clean but set their state to DEAD, and reuse them sometime.
					work_server_clean(wsp);
//					wsp->state = WORK_SERVER_DEAD;
//					//TODO:Split work_server's state and function's state.
//					wsp->func_conf->state = FUNCTION_DEAD;
				}
			}
		}
	}
	if (resp_msg->state == DELETE_FUNCTION_FAILURE) {
		return -1;
	}
	return 0;
}


//
//void clean_func(uint32_t aid, uint32_t fid)
//{
//	dlclose(func_ptrs[aid][fid]);
//}

void func_cb(int sockfd)
{
	ssize_t req_msg_size = sizeof(struct func_req_msg);
	ssize_t resp_msg_size = sizeof(struct func_resp_msg);
	struct func_req_msg req_msg;
	struct func_resp_msg resp_msg;
	ssize_t pos = 0;
	ssize_t len = 0;
	while(pos < req_msg_size)
	{

		len = read(sockfd, &req_msg+pos, req_msg_size);
		if (len < 0) {
			printf("func_server receive data failed\n");
			return ;
		}
		pos += len;
	}
	switch (req_msg.type) {
		case ADD_FUNCTION:
			function_new(&req_msg, &resp_msg);
			break;
		case STOP_FUNCTION:
			function_stop(&req_msg, &resp_msg);
			break;
		case RESTORE_FUNCTION:
			function_restore(&req_msg, &resp_msg);
			break;
		case DELETE_FUNCTION:
			function_delete(&req_msg, &resp_msg);
			break;
	}
	pos = 0;
	while(pos < resp_msg_size)
	{

		len = write(sockfd, &resp_msg + pos, resp_msg_size);
		if (len < 0) {
			printf("server send data failed\n");
			return ;
		}
		pos += len;
	}
}


int start_func_listener(char *ip, uint16_t port){
	int listenfd, connfd;    //监听描述符，连接描述符
	struct sockaddr_in server_addr, client_addr;//16 bytes
	socklen_t client_addr_size = sizeof(client_addr);
	/*socket函数*/
	listenfd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*服务器地址*/
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(ip);
	server_addr.sin_port=htons(port);
	/*bind函数*/
	if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		perror("bind error");
	/*listen函数*/
	if(listen(listenfd, 10) < 0)
		perror("listen error");
	while(1)
	{
		if((connfd=accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_size)) < 0)
			return 0;
		//close(listenfd);    //关闭监听描述符
		func_cb(connfd);    //处理请求
		close(connfd);   //父进程关闭连接描述符
	}
}