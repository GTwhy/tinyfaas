#include "../../core/worker/worker_tools.h"
#include <unistd.h>
#define nDEBUG
struct args{
	int time;
};

int work(void * param, void * md)
{
	struct args * ap = (struct args *)param;
	printf("sleep time : %d us\n",ap->time);
	usleep(ap->time);
	return 0;
}