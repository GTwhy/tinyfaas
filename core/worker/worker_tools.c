#include "coroutine.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

struct schedule * S;

struct args {
	int n;
};

static void foo(struct schedule * S, void *ud) {
	struct args * arg = ud;
	int start = arg->n;
	int i;
	for (i=0;i<5;i++) {
		printf("coroutine %d : %d\n",coroutine_running(S) , start + i);
		sleep(1);
		coroutine_yield(S);
	}
}

void signal_handler(int sig)
{
	// 解析参数(貌似 signal 无法传递参数)
	// 加载程序
	switch(sig) {
		case SIGUSR1: //处理信号 SIGUSR1
			printf("Parent : catch SIGUSR1\n");
			break;
		case SIGUSR2: //处理信号 SIGUSR2
			printf("Child : catch SIGUSR2\n");
			break;
		default:      //本例不支持
			printf("Should not be here\n");
			break;
	}
	// 创建协程
	struct args arg3 = { 200 };
	coroutine_new(S, foo, &arg3);
}


int coroutine_init() {
	//运行某函数任务
	if(signal(SIGUSR1, signal_handler) == SIG_ERR)
	{ //设置出错
		perror("Can't set handler for SIGUSR1\n");
		exit(1);
	}
	//备用信号，用于后续功能拓展
	if(signal(SIGUSR2, signal_handler) == SIG_ERR)
	{ //设置出错
		perror("Can't set handler for SIGUSR2\n");
		exit(1);
	}

	S = coroutine_open();
	struct args arg1 = { 0 };
	struct args arg2 = { 100 };

	coroutine_new(S, foo, &arg1);
	coroutine_new(S, foo, &arg2);
	
	printf("main start\n");
	coroutine_sched(S);
	printf("main end\n");
	coroutine_close(S);
	
	return 0;
}

