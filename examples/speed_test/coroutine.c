#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ && __MACH__
	#include <sys/ucontext.h>
#else 
	#include <ucontext.h>
#endif 

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 16

struct coroutine;

// 调度  schedule 的作用是什么
struct schedule {
	char stack[STACK_SIZE];// 共享栈，但切换时会从堆中分配内存并保存
	ucontext_t main;       // 当前协程的context 
	int nco;               // 协程数量
	int cap;               // 可支持的协程数量
	int running;           // 当前正在执行协程的id -1 为 main
	// 协程列表，每个协程以id标识 
	struct coroutine **co;
};


struct coroutine {
	
	coroutine_func func;    // 协程的目标函数 
	void *ud;               // 用户数据
	ucontext_t ctx;         // ucontext 上下文
	struct schedule * sch; 	// 全局的 schedule 对象
	ptrdiff_t cap;          // long int 栈容量 
	ptrdiff_t size;         // 当前的栈用量
	int status;             // 当前状态
	char *stack;            // 堆中分配内存保存的自身的栈
};

struct coroutine * 
_co_new(struct schedule *S , coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->cap = 0;
	co->size = 0;
	co->status = COROUTINE_READY;
	co->stack = NULL;
	return co;
}

void
_co_delete(struct coroutine *co) {
	// if stack is NULL , no action occurs
	free(co->stack);
	free(co); 
}

struct schedule * 
coroutine_open(void) {
	struct schedule *S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUTINE;
	S->running = -1;

	// makecontext(&S->main, (void (*)(void)) coroutine_sched, 1, S);

	S->co = malloc(sizeof(struct coroutine *) * S->cap);
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}

void 
coroutine_close(struct schedule *S) {
	int i;
	for (i=0;i<S->cap;i++) {
		struct coroutine * co = S->co[i];
		if (co) {
			_co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}

int 
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine *co = _co_new(S, func , ud);
	if (S->nco >= S->cap) {
		// 协程实例超过限定值， 扩容两倍
		int id = S->cap;
		S->co = realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap , 0 , sizeof(struct coroutine *) * S->cap);
		// 将协程加入调度器
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	} else {
		int i;
		for (i=0;i<S->cap;i++) {
			// 从当前已经分配的协程数开始分配 id 
			// 这部分可以优化
			int id = (i+S->nco) % S->cap;
			if (S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return -1;
}

static void
mainfunc(uint32_t low32, uint32_t hi32) {
	// 协程执行
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = S->running;
	struct coroutine *C = S->co[id];
	C->func(S,C->ud);
	// 协程执行结束
	_co_delete(C);
	S->co[id] = NULL;
	--S->nco;
	// S->running = -1;
	// coroutine_sched(S);
}

/*
 * 协程调度
 * id : 当前协程的 id
*/
void 
coroutine_sched(struct schedule * S) {
	// 调度算法的总是从此处开始
	getcontext(&S->main);
	// 从哪个协程进入
	int id = S->running;
	S->running = -1;
	// assert(S->running == -1);
	assert(id >= -1 && id < S->cap);

	/* 这部分可单独封装为调度策略*/
	int coid = - 1;
	int start_id = (id+1) % S->cap;
	int end_id = id;
	if (id == -1 ) {
		end_id = S->cap - 1;
	}

	// 选择一个可调度的协程
	// 循环选择
	// 可引入一个max 来优化判断的次数
	for (int i = start_id; i != end_id; i = (i+1) % S->cap ) {
		// printf("check id:%d \n ", i);
		struct coroutine *C = S->co[i];
		if (C == NULL)
			continue;

		if (C->status == COROUTINE_READY || C->status == COROUTINE_SUSPEND) {
			coid = i;
			break;
		}	
	}
	/* 这部分可单独封装为调度策略 */
	if (coid == -1 ) {
		printf("no coroutine can be run, finished\n");
		return;
		//exit(0);
	} 
	struct coroutine *C = S->co[coid];
	switch( C->status) {
	case COROUTINE_READY:
		getcontext(&C->ctx);
		C->ctx.uc_stack.ss_sp = S->stack;
		C->ctx.uc_stack.ss_size = STACK_SIZE;
		C->ctx.uc_link = &S->main;
		S->running = coid;
		C->status = COROUTINE_RUNNING;
		uintptr_t ptr = (uintptr_t)S;
		makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
		// swapcontext(&S->main, &C->ctx);
		setcontext(&C->ctx);
		break;
	case COROUTINE_SUSPEND:
		memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
		S->running = coid;
		C->status = COROUTINE_RUNNING;
		setcontext(&C->ctx);
		// swapcontext(&S->main, &C->ctx);

		break;
	default:
		assert(0);
	}
}

static void
_save_stack(struct coroutine *C, char *top) {
	char dummy = 0;
	assert(top - &dummy <= STACK_SIZE);
	if (C->cap < top - &dummy) {
		free(C->stack);
		C->cap = top-&dummy;
		C->stack = malloc(C->cap);
	}
	C->size = top - &dummy;
	memcpy(C->stack, &dummy, C->size);
}

void
coroutine_yield(struct schedule * S) {
	int id = S->running;
	assert(id >= 0);
	struct coroutine * C = S->co[id];
	assert((char *)&C > S->stack);
	_save_stack(C,S->stack + STACK_SIZE);
	C->status = COROUTINE_SUSPEND;
	// S->running = -1;
	swapcontext(&C->ctx , &S->main);
	// getcontext(&C->ctx);
	// coroutine_yield(S);
}

int 
coroutine_status(struct schedule * S, int id) {
	assert(id>=0 && id < S->cap);
	if (S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}

int 
coroutine_running(struct schedule * S) {
	return S->running;
}

