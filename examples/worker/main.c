#include "coroutine.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

struct schedule * S;

struct args {
	int n;
};

static void
foo(struct schedule * S, void *ud) {
	struct args * arg = ud;
	int start = arg->n;
	int i;
	for (i=0;i<5;i++) {
		printf("coroutine %d : %d\n",coroutine_running(S) , start + i);
		sleep(1);
		coroutine_yield(S);
	}
}

void handle_sigint(int sig)
{
	// 解析参数(貌似 signal 无法传递参数)
	// 加载程序

	// 创建协程
	struct args arg3 = { 200 };
	coroutine_new(S, foo, &arg3);

}

int 
main() {
	signal(SIGINT, handle_sigint); 
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

