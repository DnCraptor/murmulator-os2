/// POSIX.1
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
/// libc
#include <stdio.h>
#include <string.h>

static void log_write(const char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
    ///printf(msg);
}

int main() {
    log_write("Starting POSIX test\n");

    const char* test_file = "test_posix_file.txt";
    char buf[256];
    int ret;

    // 1. open
    int fd = open(test_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) { log_write("open failed\n"); goto fail; }
    log_write("open succeeded\n");

    // 2. fstat
    struct stat st;
    ret = fstat(fd, &st);
    if (ret < 0) { log_write("fstat failed\n"); goto fail; }
    log_write("fstat succeeded\n");

    // 3. write original
    const char* text1 = "Line 1\n";
    ret = write(fd, text1, strlen(text1));
    if (ret < 0) { log_write("write failed\n"); goto fail; }
    log_write("write original succeeded\n");

    // 4. dup
    int fd_dup = dup(fd);
    if (fd_dup < 0) { log_write("dup failed\n"); goto fail; }
    log_write("dup succeeded\n");

    // 5. write via duplicate
    const char* text2 = "Line 2\n";
    ret = write(fd_dup, text2, strlen(text2));
    if (ret < 0) { log_write("write dup failed\n"); goto fail; }
    log_write("write dup succeeded\n");

    // 6. lseek original to start
    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) { log_write("lseek original failed\n"); goto fail; }
    log_write("lseek original succeeded\n");

    // 7. read via original
    ret = read(fd, buf, sizeof(buf)-1);
    if (ret < 0) { log_write("read original failed\n"); goto fail; }
    buf[ret] = '\0';
    log_write("read original succeeded\n");

    // 8. dup2 test
    int fd2 = 100;
    ret = dup2(fd_dup, fd2);
    if (ret < 0) { log_write("dup2 failed\n"); goto fail; }
    log_write("dup2 succeeded\n");

    // 9. write via dup2
    const char* text3 = "Line 3\n";
    ret = write(fd2, text3, strlen(text3));
    if (ret < 0) { log_write("write dup2 failed\n"); goto fail; }
    log_write("write dup2 succeeded\n");

    // 10. lseek via dup2 and read via dup
    ret = lseek(fd2, 0, SEEK_SET);
    if (ret < 0) { log_write("lseek dup2 failed\n"); goto fail; }
    ret = read(fd_dup, buf, sizeof(buf)-1);
    if (ret < 0) { log_write("read dup failed\n"); goto fail; }
    buf[ret] = '\0';
    log_write("read dup succeeded\n");

    // 11. close duplicate
    ret = close(fd_dup);
    if (ret < 0) { log_write("close fd_dup failed\n"); goto fail; }
    log_write("close fd_dup succeeded\n");

    // 12. write via original to ensure it's still valid
    const char* text4 = "Line 4\n";
    ret = write(fd, text4, strlen(text4));
    if (ret < 0) { log_write("write original after dup close failed\n"); goto fail; }
    log_write("write original after dup close succeeded\n");

    // 13. close original
    ret = close(fd);
    if (ret < 0) { log_write("close fd failed\n"); goto fail; }
    log_write("close fd succeeded\n");

    log_write("POSIX test completed successfully\n");

fail:
    log_write("errno: ");
    buf[1] = 0;
    buf[0] = '0' + (errno / 10);
    log_write(buf);
    buf[0] = '0' + (errno - (errno / 10) * 10);
    log_write(buf);
    log_write("\n");
    return -1;
}
