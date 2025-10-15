#define _OWN_IO_FILE 
#include <stdio.h>
#include "putc.h"

int fputc(int c, FILE *f)
{
	return do_putc(c, f);
}
