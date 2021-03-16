#include "runtime.h"
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

struct brick;
struct cart;

struct labor {
	uint32_t id;
	enum {LABOR_IDLE, LABOR_RUNNING, LABOR_SYSCALL} state;
	struct cart * p;// attached p for executing code(null if not executing)

};

struct cart {
	uint32_t id;
	enum {CART_IDLE,CART_RUNNING} state;
	struct labor * l;	   	// back-link to associated labor(null if idle)
	char stack[STACK_SIZE];	// shared stack for saving context of bricks
	ucontext_t main;       	// context of main brick, or the entrance of cart.
	int nbr;               	// the number of the active bricks
	int cap;               	// current maximum number of bricks, and double when it's not enough
	int running;           	// if of the current running brick
	struct brick **br;		// brick list
};


struct brick {
	enum {BRICK_DEAD, BRICK_READY, BRICK_RUNNING, BRICK_SUSPEND} state;
	brick_func func;    	// associated function
	void *ud;               // user date
	ucontext_t ctx;
	struct cart * c; 		// associated cart
	ptrdiff_t cap;          // long int 栈容量  capacity of stack
	ptrdiff_t size;         // size of used stack
	char *stack;
};

struct brick *
_br_new(struct cart *C , brick_func func, void *ud) {
	struct brick * br = malloc(sizeof(*br));
	br->func = func;
	br->ud = ud;
	br->c = C;
	br->cap = 0;
	br->size = 0;
	br->state = BRICK_READY;
	br->stack = NULL;
	return br;
}

void
_br_delete(struct brick *br) {
	// if stack is NULL , no action occurs
	free(br->stack);
	free(br);
}

struct cart *
cart_open(void) {
	struct cart *C = malloc(sizeof(*C));
	C->nbr = 0;
	C->cap = DEFAULT_COROUTINE;
	C->running = -1;
	C->br = malloc(sizeof(struct brick *) * C->cap);
	memset(C->br, 0, sizeof(struct brick *) * C->cap);
	return C;
}

void 
cart_close(struct cart *C) {
	int i;
	for (i=0;i<C->cap;i++) {
		struct brick * br = C->br[i];
		if (br) {
			_br_delete(br);
		}
	}
	free(C->br);
	C->br = NULL;
	free(C);
}

int 
brick_new(struct cart *C, brick_func func, void *ud) {
	struct brick *br = _br_new(C, func , ud);
	if (C->nbr >= C->cap) {
		// 协程实例超过限定值， 扩容两倍
		int id = C->cap;
		C->br = realloc(C->br, C->cap * 2 * sizeof(struct brick *));
		memset(C->br + C->cap , 0 , sizeof(struct brick *) * C->cap);
		// 将协程加入调度器
		C->br[C->cap] = br;
		C->cap *= 2;
		++C->nbr;
		return id;
	} else {
		int i;
		for (i=0;i<C->cap;i++) {
			// 从当前已经分配的协程数开始分配 id 
			// 这部分可以优化
			int id = (i+C->nbr) % C->cap;
			if (C->br[id] == NULL) {
				C->br[id] = br;
				++C->nbr;
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
	struct cart *C = (struct cart *)ptr;
	int id = C->running;
	struct brick *B = C->br[id];
	B->func(C,B->ud);
	// 协程执行结束
	_br_delete(B);
	C->br[id] = NULL;
	--C->nbr;
}

/*
 * 协程调度
 * id : 当前协程的 id
*/
void 
cart_sched(struct cart * C) {
	// 调度算法的总是从此处开始
	getcontext(&C->main);
	// 从哪个协程进入
	int id = C->running;
	C->running = -1;
	// assert(C->running == -1);
	assert(id >= -1 && id < C->cap);

	/* 这部分可单独封装为调度策略*/
	int brid = - 1;
	int start_id = (id+1) % C->cap;
	int end_id = id;
	if (id == -1 ) {
		end_id = C->cap - 1;
	}

	// 选择一个可调度的协程
	// 循环选择
	// 可引入一个max 来优化判断的次数
	for (int i = start_id; i != end_id; i = (i+1) % C->cap ) {
		// printf("check id:%d \n ", i);
		struct brick *B = C->br[i];
		if (B == NULL)
			continue;

		if (B->state ==  BRICK_READY || B->state ==  BRICK_SUSPEND) {
			brid = i;
			break;
		}	
	}
	/* 这部分可单独封装为调度策略 */
	if (brid == -1 ) {
		return ;
	} 
	struct brick *B = C->br[brid];
	switch( B->state) {
	case  BRICK_READY:
		getcontext(&B->ctx);
		B->ctx.uc_stack.ss_sp = C->stack;
		B->ctx.uc_stack.ss_size = STACK_SIZE;
		B->ctx.uc_link = &C->main;
		C->running = brid;
		B->state =  BRICK_RUNNING;
		uintptr_t ptr = (uintptr_t)C;
		makecontext(&B->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
		// swapcontext(&C->main, &C->ctx);
		setcontext(&B->ctx);
		break;
	case  BRICK_SUSPEND:
		memcpy(C->stack + STACK_SIZE - B->size, B->stack, B->size);
		C->running = brid;
		B->state =  BRICK_RUNNING;
		setcontext(&B->ctx);
		// swapcontext(&C->main, &C->ctx);

		break;
	default:
		assert(0);
	}
}

static void
_save_stack(struct brick *B, char *top) {
	char dummy = 0;
	assert(top - &dummy <= STACK_SIZE);
	if (B->cap < top - &dummy) {
		free(B->stack);
		B->cap = top-&dummy;
		B->stack = malloc(B->cap);
	}
	B->size = top - &dummy;
	memcpy(B->stack, &dummy, B->size);
}

void
brick_yield(struct cart * C) {
	int id = C->running;
	assert(id >= 0);
	struct brick * B = C->br[id];
	assert((char *)&B > C->stack);
	_save_stack(B,C->stack + STACK_SIZE);
	B->state =  BRICK_SUSPEND;
	swapcontext(&B->ctx , &C->main);
}

int 
brick_state(struct cart * C, int id) {
	assert(id>=0 && id < C->cap);
	if (C->br[id] == NULL) {
		return  BRICK_DEAD;
	}
	return C->br[id]->state;
}

int 
brick_running(struct cart * C) {
	return C->running;
}

