/// POSIX.1
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
/// libc
#include <stdio.h>

// W/A TODO: some support for MOS:
const long _DYNAMIC[1] = { 0 };

static const void (*___init_array[])(void) __attribute__((section(".init_array"))) = {};
const void * __init_array_start __attribute__((section(".init_array"))) = (const void *)___init_array;
const void * __init_array_end __attribute__((section(".init_array"))) = (const void *)___init_array;

static const void (*___fini_array[])(void) __attribute__((section(".init_array"))) = {};
const void * __fini_array_start __attribute__((section(".init_array"))) = (const void *)___fini_array;
const void * __fini_array_end __attribute__((section(".init_array"))) = (const void *)___fini_array;

int __sys_open_cp(const char* n, int opt1, int opt2) {
    return open(n, opt1, opt2);
}

int __sys_open(const char* n, int opt1, int opt2) {
    return open(n, opt1, opt2);
}

int sys_open(const char* n, int opt1, int opt2) {
    return open(n, opt1, opt2);
}

int main(int argc, char **argv) {
    printf(
        "It is libc and not a regular MOS-program.\n"
        "libc (the C standard library) - is a core collection of functions that\n"
        "provide the fundamental interface between C programs and the operating\n"
        "system. It implements essential functionality such as memory management,\n"
        "string and file operations, mathematical routines, input/output, and\n"
        "system call wrappers. In practice, libc allows user programs to perform\n"
        "tasks like reading files, printing output, and allocating memory without\n"
        "interacting directly with the kernel.\n"
        "This particular implementation is based on the open-source 'musl' library.\n"
        "\n"
        "Self test:\n"
    );
    FILE* f = fopen("/libc.test", "w+");
    if (!f) {
        printf("fopen: FAILED errno: %d\n", errno);
        return errno;
    }
    printf("fopen: PASSED\n");

    if (4 != fwrite("TEST", 1, 4, f)) {
        printf("fwrite: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fwrite: PASSED\n");

    if (fflush(f) != 0) {
        printf("fflush: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fflush: PASSED\n");

    rewind(f);
    if (fgetc(f) != 'T') {
        printf("fgetc: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fgetc: PASSED\n");

    rewind(f);
    char b[4];
    if (4 != fread(b, 1, 4, f)) {
        printf("fread: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fread: PASSED\n");

    rewind(f);
    if (strcmp(fgetln(f), "TEST") != 0) {
        printf("fgetln: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fgetln: PASSED\n");

    if (fseek(f, 2, SEEK_SET) != 0) {
        printf("fseek: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fseek: PASSED\n");

    fpos_t pos;
    if (fgetpos(f, &pos) != 0) {
        printf("fgetpos: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fgetpos: PASSED\n");

    if (fsetpos(f, &pos) != 0) {
        printf("fsetpos: FAILED errno: %d\n", errno);
        goto m1;
    }
    printf("fsetpos: PASSED\n");

m1:
    if(fclose(f) < 0) {
        printf("fclose: FAILED errno: %d\n", errno);
        return errno;
    }
    printf("fclose: PASSED\n");
    return 0;
}
