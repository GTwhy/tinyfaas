#include "coroutine.h"
#include <stdio.h>

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
		coroutine_yield(S);
	}
}

int 
main() {
	struct schedule * S = coroutine_open();
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

