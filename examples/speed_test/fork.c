#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

long long getms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main() {

	// Store the name of the function we want to call.
	const char* work_path = "./work";
	int pid;
//初始化时间结构体
	long long start=0;
	start = getms();
	for(int i=0;i<10000;i++)
	{
		pid = fork();
		if(pid<0)
		{
			printf("fork failed!\n");
			exit(-1);
		} else if(pid == 0){
			execlp(work_path, work_path, NULL);
		} else{
			wait(NULL);
		}
	}
	printf("time = %lld ms\n", getms() - start);

	return 0;
}
