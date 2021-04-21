//
// Created by why on 2021/4/12.
//

#ifndef LUTF_WORKER_TOOLS_H
#define LUTF_WORKER_TOOLS_H
#include <stdlib.h>
#include "work_server.h"

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
char * add_new_function(char * func_server_ip, uint16_t func_server_port, char * work_server_url, char * workload_path, char * func_name, int app_id, int func_id);

/**
 * Send a function request to the work_server.
 * @param work_server_url	The work_server's url
 * @param args				Args send to the function.
 * @param args_size 	  	Size of args.
 * @return					The pointer to the value returned from work_server(needs free after used).
 */
void * make_function_request(void * work_server_url, void * args, size_t args_size);

/**
 * Low-level api used to stop a function service.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
int stop_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);

/**
 * Low-level api used to restore a function service.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
int restore_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);

/**
 * Low-level api used to delete a function service.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					The pointer points to the work_server_url(needs free() after used).
 */
int delete_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);

/**
 * Low-level api used to query function state.
 * @param func_server_url	The url of the worker's func_server, which listening the add_new_function requests.
 * @param app_id			The id of the app which the function belongs to.
 * @param func_id			The id of this function.
 * @return					Operation result flag.
 */
int query_function(char * func_server_ip, uint16_t func_server_port, int app_id, int func_id);

/**
 * free returned value.
 * @param rv
 */
void free_rv(void * rv);

/**
 * free returned url
 * @param url
 */
void free_url(char * url);

#endif //LUTF_WORKER_TOOLS_H
