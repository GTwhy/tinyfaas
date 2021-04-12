#ifndef C_COROUTINE_H
#define C_COROUTINE_H
#include <signal.h>

//signum used to suspend the current brick.
#define SUSPEND_SIG_NUM (SIGRTMIN+3)

struct cart;
struct labor;

// 协程函数指针
typedef void (*brick_func)(void *ud);
void cart_close(struct cart *C);
void labor_close(struct labor *L);
int runtime_init(void);
int brick_new(brick_func, void *ud);
void cart_sched(struct cart *);
int brick_state(struct cart *, int id);
int brick_running(struct cart *);
void brick_yield(struct cart *);
void send_sig_to_labor(struct labor * l, int signum);

#endif
