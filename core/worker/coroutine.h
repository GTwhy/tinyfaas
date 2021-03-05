#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

// 协程函数指针
//TODO:返回值不能是空，应该是void*，与参数一致。由此导致的改变后续考虑。
typedef void (*coroutine_func)(struct schedule *, void *ud);

struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_sched(struct schedule *);
int coroutine_status(struct schedule *, int id);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);

#endif
