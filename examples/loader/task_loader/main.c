#include <stdio.h>
#include <dlfcn.h>

// This type declaration is needed to call functions
typedef int (*func_ptr_t)(void);
int main(int argc, char* argv[]) {

// Store the name of the function we want to call.
	const char* lib_path_1 = argv[1];
	const char* func_name_1 = argv[2];
	const char* lib_path_2 = argv[3];
	const char* func_name_2 = argv[4];
// Open the shared library containing the functions.
    void* lib = dlopen(lib_path_1, RTLD_LAZY);
// Get a reference to the function we call.
    func_ptr_t func = dlsym(lib, func_name_1);
// Call the function and print out the result.
    func();
// Close the library.
    dlclose(lib);

// Open the shared library containing the functions.
	lib = dlopen(lib_path_2, RTLD_LAZY);
// Get a reference to the function we call.
	func = dlsym(lib, func_name_2);
// Call the function and print out the result.
	func();
// Close the library.
	dlclose(lib);
    printf("Well done!\n");
    return 0;
}
