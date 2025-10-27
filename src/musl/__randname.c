#include "internal/pthread_impl.h"
#include <time.h>
#include <stdint.h>
#include "sys_table.h"

#include <pico/time.h>

int __libc() __clock_gettime(clockid_t clk, struct timespec *ts32)
{
	/** TODO:
	struct timespec ts;
	int r = clock_gettime(clk, &ts);
	if (r) return r;
	if (ts.tv_sec < INT32_MIN || ts.tv_sec > INT32_MAX) {
		errno = EOVERFLOW;
		return -1;
	}
	ts32->tv_sec = ts.tv_sec;
	ts32->tv_nsec = ts.tv_nsec;
	*/
	uint64_t t = time_us_64();
	ts32->tv_sec = t / 1000000;
	ts32->tv_nsec = (t % 1000000) * 1000;
	return 0;
}

/* This assumes that a check for the
   template size has already been made */
char* __libc() __randname(char *template)
{
	int i;
	struct timespec ts;
	unsigned long r;

	__clock_gettime(CLOCK_REALTIME, &ts);
	r = ts.tv_sec + ts.tv_nsec + __pthread_tid() * 65537UL;
	for (i=0; i<6; i++, r>>=5)
		template[i] = 'A'+(r&15)+(r&16)*2;

	return template;
}
