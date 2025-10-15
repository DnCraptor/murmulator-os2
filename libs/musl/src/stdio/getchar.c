#define _OWN_IO_FILE 
#include <stdio.h>
#include "getc.h"

int getchar(void)
{
	return do_getc(stdin);
}
