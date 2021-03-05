#include<stdio.h>
#include<math.h>
void * work(void * s)
{
	int i,m,n,x,y,z,flag=0;
	m = 100;
	n = 999;
	printf("str from user %s\n", s);
	if(m<100||m>n||n>999)
	{
		printf("Erroe!\n");
		return "error";
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
	return s;
}