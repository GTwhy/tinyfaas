#include "../../core/worker/work_server.h"
#include <string.h>
#define nDEBUG

int hello_world(void * param, void * md)
{
	char * str = "Hello World!\n";
	mate_date_s * mp = (mate_date_s *)md;
	strcpy(mp->msg, str);
	return 0;
}