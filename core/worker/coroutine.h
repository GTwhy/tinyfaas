#ifndef C_COROUTINE_H
#define C_COROUTINE_H

struct schedule;

// 协程函数指针
typedef void (*coroutine_func)(struct schedule *S, void *ud);

struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_sched(struct schedule *);
int coroutine_status(struct schedule *, int id);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);

#endif
