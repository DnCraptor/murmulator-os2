/// POSIX.1
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
/// libc
#include <stdio.h>
#include <string.h>

// W/A TODO: some support for MOS:
const long _DYNAMIC[1] = { 0 };

const void (*__init_array_start[])(void) __attribute__((section(".init_array"))) = {};
const void * __init_array_end __attribute__((section(".init_array_end"))) = (const void *)__init_array_start;

int __sys_open(const char* n, int opt1, int opt2) {
    return open(n, opt1, opt2);
}

static void log_write(const char* msg) {
    ///write(STDOUT_FILENO, msg, strlen(msg));
    printf(msg);
}

int main() {
    log_write("It is libc.\n");
    return -1;
}