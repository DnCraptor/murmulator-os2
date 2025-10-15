#define _OWN_IO_FILE 
#include <stdio.h>
#include "putc.h"

int putchar(int c)
{
	return do_putc(c, stdout);
}
