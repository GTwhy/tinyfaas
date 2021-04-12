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
	printf("sleep time : %d us\n",ap->time);
	mp->data_id = 2;
	mp->data_stat = 333;
	printf("Going to sleep\n");
	for(int i = 1; i < 11; i++){
		usleep(ap->time);
		printf("sleep loop : %d\n", i);
	}
	return 0;
}