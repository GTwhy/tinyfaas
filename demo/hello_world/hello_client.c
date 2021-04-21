//
// Created by why on 2021/4/12.
//

#include "../../core/worker/worker_tools.h"
#include <string.h>
#define SLEEP_PATH	"../demo/hello_world/build/hello_world.lf"
#define SLEEP_NAME	"hello_world"

char * work_server_url;

void request(void)
{
	void *rv = make_function_request(work_server_url, NULL,0);
	mate_date_s * md = (mate_date_s *)rv;
	printf("Get return_value msg : %s\n", md->msg);
	free_rv(rv);
}

void next(void)
{
	int input;
	printf("Enter 1 to continue, enter other to exit.\n");
	scanf("%d", &input);
	if (input == 1){
		return ;
	} else{
		exit(0);
	}
}

int main(int argc, char **argv)
{
	printf("next step : add func\n");
	next();
	if (argc == 3) {
		//User defined url
		uint16_t port = (uint16_t)atoi(argv[2]);
		printf("Function server url : %s:%u\n\n", argv[1], port);
		work_server_url = add_new_function(argv[1],port, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
	} else{
		//Default url
		printf("Tips : user defined url: %s <ip> <port>\n\n", argv[0]);
		work_server_url = add_new_function(NULL,0, NULL, SLEEP_PATH, SLEEP_NAME,0, 0);
	}
	printf("next step : query\n");
	next();
	query_function(NULL, 0, 0, 0);
	printf("next step : request\n");
	next();
	request();
	printf("next step : stop func\n");
	next();
	stop_function(NULL,0,0,0);
	printf("next step : query\n");
	next();
	query_function(NULL, 0, 0, 0);
	printf("next step : restore func\n");
	next();
	restore_function(NULL,0,0,0);
	printf("next step : query\n");
	next();
	query_function(NULL, 0, 0, 0);
	printf("next step : request\n");
	next();
	request();
	printf("next step : delete func\n");
	next();
	delete_function(NULL,0,0,0);
	printf("next step : query\n");
	next();
	query_function(NULL, 0, 0, 0);
	printf("next step : request. Connection will be refused because func was deleted.\n");
	next();
	request();
	free_url(work_server_url);
	return 0;
}