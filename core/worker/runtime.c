#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/util/platform.h>

#if __APPLE__ && __MACH__
	#include <sys/ucontext.h>
#else 
	#include <ucontext.h>
#endif 

#define nDEBUG

#define STACK_SIZE (1024*1024)
#define DEFAULT_CART_SIZE 8
#define DEFAULT_CART_NUMBER 4
#define DEFAULT_LABOR_NUMBER 4

struct labor * ls[DEFAULT_LABOR_NUMBER];
struct cart  * cs[DEFAULT_CART_NUMBER];
static int cf = 0;
pthread_mutex_t pub_lock;

struct labor {
	uint32_t lid;
	pthread_t ptd;
	enum {LABOR_DEAD, LABOR_IDLE, LABOR_RUNNING, LABOR_SYSCALL} state;
	struct cart * cp;// attached cart for executing code(null if not executing)
	pthread_cond_t th_cond;
	pthread_mutex_t th_mutex;
};

struct cart {
	uint32_t cid;
	enum {CART_IDLE, CART_INCREASING, CART_RUNNING} state;
	struct labor * lp;	   	// back-link to associated labor(null if idle)
	char stack[STACK_SIZE];	// shared stack for saving context of bricks
	ucontext_t main;       	// context of main brick, or the entrance of cart.
	int nbr;               	// the number of the active bricks
	int cap;               	// current maximum number of bricks, and double when it's not enough
	int running;           	// if of the current running brick
	struct brick **br;		// brick list
	pthread_mutex_t cart_mutex;
};


struct brick {
	enum {BRICK_IDLE, BRICK_INIT, BRICK_READY, BRICK_RUNNING, BRICK_SUSPEND, BRICK_DEAD} state;
	brick_func func;    	// associated function
	int bid;
	void *ud;               // user date
	ucontext_t ctx;
	struct cart * cp; 		// associated cart
	ptrdiff_t cap;          // stack capacity that has been applied.
	ptrdiff_t size;         // size of used stack
	char *stack;
};

struct brick * _br_new(struct cart *C , brick_func func, void *ud) {
	struct brick * br;
	if (C->nbr < DEFAULT_CART_SIZE){
		for (int i = 0; i < DEFAULT_CART_SIZE; ++i) {
			if (C->br[i]->state == BRICK_IDLE){
				br = C->br[C->nbr];
				C->nbr++;
				br->func = func;
				br->ud = ud;
				br->cap = 0;
				br->size = 0 ;
				br->stack = NULL;
				br->state = BRICK_READY;
				return br;
			}

		}
	}
	br = malloc(sizeof(*br));
	br->state = BRICK_INIT;
	br->func = func;
	br->ud = ud;
	br->cp = C;
	br->cap = 0;
	br->size = 0 ;
	br->stack = NULL;
	return br;
}



void signal_handler(int sig)
{

}


void _br_delete(struct brick *br) {
#ifdef DEBUG
#endif
	if (br->stack != NULL)
		free(br->stack);
	if (br->bid < DEFAULT_CART_SIZE){
		br->state = BRICK_IDLE;
		printf("RESET STATE OF IDLE BRICK cid : %d   bid : %d\n",br->cp->cid, br->bid);
		return;
	}
	printf("DELETE NEW BRICK  cid : %d  bid : %d\n", br->cp->cid, br->bid);
	if(br != NULL)
		free(br);
}

struct cart * cart_open(int cid) {
	struct cart *C = malloc(sizeof(*C));
	C->cid = cid;
	C->lp = NULL;
	C->state = CART_IDLE;
	C->nbr = 0;
	C->cap = DEFAULT_CART_SIZE;
	C->running = -1;
	pthread_mutex_init(&C->cart_mutex, NULL);
	C->br = malloc(sizeof(struct brick *) * C->cap);
	memset(C->br, 0, sizeof(struct brick *) * C->cap);
	return C;
}

void * labor_routine(void * param)
{
	struct labor * L = (struct labor *) param;
#ifdef DEBUG
	printf("labor_%d start\n", L->lid);
#endif
	while (L->state != LABOR_DEAD)
	{
		pthread_mutex_lock(&L->th_mutex);
#ifdef DEBUG
		printf("pthread_%d is going to wait\n", L->lid);
#endif

		pthread_cond_wait(&L->th_cond, &L->th_mutex);
#ifdef DEBUG
		printf("pthread_%d wake up\n", L->lid);
#endif
		if(L->cp == NULL){
			printf("Labor routine ERROR : L->c is NULL");
			labor_close(L);
		}
		cart_sched(L->cp);
		pthread_mutex_unlock(&L->th_mutex);
	}
#ifdef DEBUG
	printf("labor_%d exit\n", L->lid);
#endif
	return NULL;
}

struct labor * labor_open(int lid) {
	struct labor *L = malloc(sizeof(*L));
	L->lid = lid;
	L->cp = NULL;
	L->state = LABOR_IDLE;
	//TODO:init th_cond
	pthread_cond_init(&L->th_cond, NULL);
	pthread_mutex_init(&L->th_mutex, NULL);
	//TODO:check threads
	pthread_create(&L->ptd, NULL, labor_routine, L);
	return L;
}


struct cart * get_cart(void)
{
	struct cart * C;
	pthread_mutex_lock(&pub_lock);
	cf = ++cf%DEFAULT_CART_NUMBER;
	C = cs[cf];
	pthread_mutex_unlock(&pub_lock);
	C->state = CART_INCREASING;
#ifdef DEBUG
	printf("get_cart: cid : %d it's state : %d\n", C->cid, C->state);
#endif
	return C;
}

void _wake_up(struct labor * l)
{
	pthread_cond_signal(&l->th_cond);
	return;
}

void _call_labor(struct cart * C)
{
#ifdef DEBUG
	printf("calling labor_%d\n",C->lp->lid);
#endif
	pthread_mutex_lock(&C->cart_mutex);
	switch (C->lp->state) {
		case LABOR_RUNNING:
			pthread_mutex_unlock(&C->cart_mutex);
			return;
		case LABOR_IDLE:
			_wake_up(C->lp);
			pthread_mutex_unlock(&C->cart_mutex);
			return;
	}
}

int _lc_bind(struct labor *l, struct cart *c)
{
	if (l->cp == NULL && c->lp == NULL){
		l->cp = c;
		c->lp = l;
		return 0;
	} else if(l->cp != NULL) {
		return -1;
	} else {
		pthread_mutex_lock(&c->lp->th_mutex);
		c->lp->cp = NULL;
		pthread_mutex_unlock(&c->lp->th_mutex);
		pthread_mutex_lock(&l->th_mutex);
		c->lp = l;
		l->cp = c;
		pthread_mutex_unlock(&l->th_mutex);
		return 0;
	}

}


int 
brick_new(brick_func func, void *ud) {
	struct cart *C = get_cart();
	pthread_mutex_lock(&C->cart_mutex);
	int tmp_nbr = C->nbr;
	struct brick *br = _br_new(C, func , ud);
	pthread_mutex_unlock(&C->cart_mutex);
	if (br->state == BRICK_READY){
		C->state = CART_RUNNING;
		printf("IDLE_BRICK is reused to run.   cid : %d   bid : %d \n", br->cp->cid, br->bid);
		_call_labor(C);
		return br->bid;
	}
	int id;
	pthread_mutex_lock(&pub_lock);
	if (tmp_nbr >= C->cap) {
		// 协程实例超过限定值， 扩容两倍
		id = C->cap;
		C->br = realloc(C->br, C->cap * 2 * sizeof(struct brick *));
		memset(C->br + C->cap , 0 , sizeof(struct brick *) * C->cap);
		// 将协程加入调度器
		C->br[C->cap] = br;
		C->cap *= 2;
		printf("default cart size is not enough , and new cap of cart : %d\n", C->cap);
		++C->nbr;
	} else {
		for (int i=DEFAULT_CART_SIZE; i<C->cap; i++) {
			// 从当前已经分配的协程数开始分配 id 
			// 这部分可以优化
			id = (i+tmp_nbr) % C->cap;
			if (C->br[id] == NULL) {
				C->br[id] = br;
				++C->nbr;
				break;
			}
		}
	}
	pthread_mutex_unlock(&pub_lock);
#ifdef DEBUG
	printf("NEW_BRICK has been made to run.   cid : %d   bid : %d \n", br->cp->cid, br->bid);
#endif
	br->bid = id;
	C->state = CART_RUNNING;
	br->state = BRICK_READY;
	_call_labor(C);
	return id;
}

static void brick_entrance(uint32_t low32, uint32_t hi32) {
	// 协程执行
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct cart *C = (struct cart *)ptr;
	int id = C->running;
	struct brick *B = C->br[id];
	if (id >= DEFAULT_CART_SIZE){
		C->br[id] = NULL;
	}
	C->state = CART_IDLE;
	if (B->ud == NULL){
		//TODO: Returning here could cause the work_task block and the client won't receive result until timeout.
		printf("brick_entrance error : param is null.\n");
		_br_delete(B);
		return;
	}
	B->func(B->ud);
	// 协程执行结束
	_br_delete(B);
	pthread_mutex_lock(&C->cart_mutex);
	--C->nbr;
	pthread_mutex_unlock(&C->cart_mutex);
}




/*
 * 协程调度
 * id : 当前协程的 id
*/
void cart_sched(struct cart * C) {
	// 调度算法的总是从此处开始
	getcontext(&C->main);
	// 从哪个协程进入
#ifdef DEBUG
	printf("cart sched\n");
#endif
	C->state = CART_RUNNING;
	int id = C->running;
	C->running = -1;
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

		if ((B != NULL) && (B->state ==  BRICK_READY || B->state ==  BRICK_SUSPEND)) {
			brid = i;
#ifdef DEBUG
			printf("cart_sched: brick_%d is going to run\n", brid);
#endif
			C->state = CART_RUNNING;
			break;
		} else{
			continue;
		}
	}
	/* 这部分可单独封装为调度策略 */
	if (brid == -1 ) {
#ifdef DEBUG
		printf("cart_sched: No brick is going to run\n");
#endif
		C->state = CART_IDLE;
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
			makecontext(&B->ctx, (void (*)(void)) brick_entrance, 2, (uint32_t) ptr, (uint32_t) (ptr >> 32));
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

static void _save_stack(struct brick *B, char *top) {
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

void brick_yield(struct cart * C) {
	int id = C->running;
	assert(id >= 0);
	struct brick * B = C->br[id];
	assert((char *)&B > C->stack);
	_save_stack(B,C->stack + STACK_SIZE);
	B->state =  BRICK_SUSPEND;
	swapcontext(&B->ctx , &C->main);
}

int brick_state(struct cart * C, int id) {
	assert(id>=0 && id < C->cap);
	if (C->br[id] == NULL) {
		return  BRICK_DEAD;
	}
	return C->br[id]->state;
}

int brick_running(struct cart * C) {
	return C->running;
}


void cart_close(struct cart *C) {
	int i;
	for (i=0;i<C->cap;i++) {
		struct brick * br = C->br[i];
		if (br) {
			_br_delete(br);
		}
	}
	free(C->br);
	C->br = NULL;
	cs[C->cid] = NULL;
	free(C);
}

void labor_close(struct labor *L)
{
	pthread_cancel(L->ptd);
	ls[L->lid] = NULL;
	free(L);
}


void runtime_exit()
{
	for (int i = 0; i < DEFAULT_LABOR_NUMBER; ++i) {
		labor_close(ls[i]);
	}
	for (int i = 0; i < DEFAULT_CART_NUMBER; ++i) {
		cart_close(cs[i]);
	}
	return;
}

void brick_new_ahead(struct cart * C)
{
	for (int i = 0; i < DEFAULT_CART_SIZE; i++) {
		struct brick * br = malloc(sizeof(*br));
		br->state = BRICK_IDLE;
		br->cp = C;
		br->bid = i;
		br->cap = 0;
		br->size = 0 ;
		br->stack = NULL;
		C->br[i] = br;
	}
}


int runtime_init(void)
{
	pthread_mutex_init(&pub_lock, NULL);
	for (int i = 0; i < DEFAULT_CART_NUMBER; ++i) {
		cs[i] = cart_open(i);
		brick_new_ahead(cs[i]);
	}
	printf("Alloc cart done\n");
	for (int i = 0; i < DEFAULT_LABOR_NUMBER; ++i) {
		ls[i] = labor_open(i);
		_lc_bind(ls[i], cs[i]);
	}
	printf("Alloc labor and bind done\n");
	return 0;
}
