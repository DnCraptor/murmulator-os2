#define _OWN_IO_FILE 
#include <stdio.h>
#include "getc.h"

int fgetc(FILE *f)
{
	return do_getc(f);
}
