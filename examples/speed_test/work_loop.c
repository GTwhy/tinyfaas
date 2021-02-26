#include<stdio.h>
#include<math.h>
#include <sys/time.h>

long long getms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main()
{
	//获取当前时间
	long long start = 0;
	start = getms();
	for(int l=0;l<10000;l++){
		int i,m,n,x,y,z,flag=0;
		m = 100;
		n = 999;
		if(m<100||m>n||n>999)
		{
			printf("Erroe!\n");
			return 0;
		}
		for(i=m;i<=n;i++){
			x=i%10;
			y=(i/10)%10;
			z=i/100;

			if(pow(x,3)+pow(y,3)+pow(z,3)==i && i!=1000)
			{
				flag=1;
				printf("%d\n",i);
			}
		}
		if(flag==0)
			printf("No\n");
	}

	printf("time = %lld ms\n", getms() - start);
	return 0;
}