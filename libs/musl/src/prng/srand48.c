#include <stdlib.h>

unsigned short* seed48 (unsigned short [3]);

void srand48(long seed)
{
	seed48((unsigned short [3]){ 0x330e, seed, seed>>16 });
}
