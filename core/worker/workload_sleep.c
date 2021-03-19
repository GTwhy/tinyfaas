#include "work_server.h"
#include <unistd.h>
#define nDEBUG
struct args{
	int time;
};

int work(void * param, void * md)
{
	struct args * ap = (struct args *)param;
	mate_date_s * mp = (mate_date_s *)md;
	printf("sleep time : %d \n",ap->time);
	mp->data_id = 2;
	mp->data_stat = 333;
	usleep(ap->time);
	return 0;
}