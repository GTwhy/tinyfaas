#include<stdio.h>
#include<math.h>
#include "work_server.h"
#include <unistd.h>
#define nDEBUG
struct args{
	int m;
	int n;
};

int work(void * param, void * md)
{
	struct args * ap = (struct args *)param;
	mate_date_s * mp = (mate_date_s *)md;
#ifdef DEBUG
	printf("worklaod : args-m:%d  args-n:%d\n",ap->m, ap->n);
#endif
	int i,m,n,x,y,z,flag=0;
	m = ap->m;
	n = ap->n;
	mp->data_id = 666;
	mp->data_stat = 888;
	if(m<100||m>n||n>999)
	{
		printf("ppdi : param error!\n");
		return 0;
	}
	for(i=m;i<=n;i++){
		x=i%10;
		y=(i/10)%10;
		z=i/100;

		if(pow(x,3)+pow(y,3)+pow(z,3)==i && i!=1000)
		{
			flag=1;
			#ifdef DEBUG
			printf("%d\n",i);
			#endif
		}
	}
	if(flag==0)
		printf("No\n");
	//usleep(10000);
	return 0;
}