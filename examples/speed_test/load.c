#include <stdio.h>
#include <dlfcn.h>

#include <sys/time.h>

long long getms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// This type declaration is needed to call functions
typedef int (*func_ptr_t)(void);
int main() {
// Store the name of the function we want to call.
	const char* lib_path = "./libwork.so.so";
	const char* func_name = "main";
	long long start = 0;
	start = getms();
	for(int i=0;i<10000;i++){
		void* lib = dlopen(lib_path, RTLD_LAZY);
// Get a reference to the function we call.
		func_ptr_t func = dlsym(lib, func_name);
// Call the function and print out the result.
		func();
// Close the library.
		dlclose(lib);
	}
	printf("time = %lld ms\n", getms() - start);

	return 0;
}
