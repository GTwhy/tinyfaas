#include "coroutine.h"
#include <stdio.h>
#include <signal.h>

#include <dlfcn.h>

struct schedule * S;
typedef int (*func_ptr_t)(void);

struct args {
	func_ptr_t func;
};

long long getms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void
foo(struct schedule * S, void *ud) {
	struct args * arg = ud;
	arg->func();
}

void handle_sigint(int sig)
{
	// 解析参数(貌似 signal 无法传递参数)
	// 加载程序

	// 创建协程
//	struct args arg3 = { 200 };
//	coroutine_new(S, foo, &arg3);

}

int 
main() {
	long long start=0;
	S = coroutine_open();
	const char* lib_path = "./libwork.so.so";
	const char* func_name = "main";
	void* lib = dlopen(lib_path, RTLD_LAZY);
	func_ptr_t func = dlsym(lib, func_name);
	struct args arg1 = {func};
	start = getms();
	for (int i = 0; i < 10000; ++i) {
		coroutine_new(S, foo, &arg1);
	}
	printf("main start\n");
	coroutine_sched(S);
	printf("main end\n");
	coroutine_close(S);
	dlclose(lib);
	printf("time = %lld ms\n", getms() - start);
	return 0;
}

