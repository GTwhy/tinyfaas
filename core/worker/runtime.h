#ifndef C_COROUTINE_H
#define C_COROUTINE_H

struct cart;

// 协程函数指针
typedef void (*brick_func)(void *ud);

int runtime_init(void);
int brick_new(brick_func, void *ud);
void cart_sched(struct cart *);
int brick_status(struct cart *, int id);
int brick_running(struct cart *);
void brick_yield(struct cart *);

#endif
