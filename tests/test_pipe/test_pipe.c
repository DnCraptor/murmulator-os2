/// POSIX.1 pipe() / pipe2() test suite
#include <unistd.h>
#include <sys/fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>
/// libc
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  helpers                                                             */
/* ------------------------------------------------------------------ */

static int g_failed = 0;

#define CHECK(expr, name)                                   \
    do {                                                    \
        if (!(expr)) {                                      \
            printf("%s: FAILED (errno=%d)\n", name, errno); \
            g_failed++;                                     \
        } else {                                            \
            printf("%s: PASSED\n", name);                   \
        }                                                   \
    } while (0)

/* ------------------------------------------------------------------ */
/*  1. Basic pipe: write then read                                      */
/* ------------------------------------------------------------------ */
static void test_basic(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe()");

    const char *msg = "hello pipe";
    int len = (int)strlen(msg);

    int w = write(fds[1], msg, len);
    CHECK(w == len, "pipe write");

    char buf[64] = {0};
    int r = read(fds[0], buf, sizeof(buf) - 1);
    CHECK(r == len, "pipe read length");
    CHECK(strcmp(buf, msg) == 0, "pipe read content");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  2. EOF when all writers closed                                       */
/* ------------------------------------------------------------------ */
static void test_eof_on_writer_close(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for EOF test");

    write(fds[1], "x", 1);
    close(fds[1]);  // close writer

    char buf[4];
    int r1 = read(fds[0], buf, sizeof(buf));
    CHECK(r1 == 1, "read before EOF");

    int r2 = read(fds[0], buf, sizeof(buf));
    CHECK(r2 == 0, "read returns 0 (EOF) after writer closed");

    close(fds[0]);
}

/* ------------------------------------------------------------------ */
/*  3. EPIPE + SIGPIPE when reader closed                               */
/* ------------------------------------------------------------------ */
static void test_epipe(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for EPIPE test");

    /* ignore SIGPIPE so write() returns EPIPE instead of killing us */
    signal(SIGPIPE, SIG_IGN);

    close(fds[0]);  // close reader

    errno = 0;
    int w = write(fds[1], "x", 1);
    CHECK(w == -1 && errno == EPIPE, "write returns EPIPE when reader closed");

    signal(SIGPIPE, SIG_DFL);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  4. pipe2() with O_CLOEXEC                                           */
/* ------------------------------------------------------------------ */
static void test_pipe2_cloexec(void)
{
    int fds[2];
    CHECK(pipe2(fds, O_CLOEXEC) == 0, "pipe2(O_CLOEXEC)");

    /* check FD_CLOEXEC is set on both ends */
    int fl0 = fcntl(fds[0], F_GETFD);
    int fl1 = fcntl(fds[1], F_GETFD);
    CHECK((fl0 & FD_CLOEXEC) != 0, "pipe2 read-end FD_CLOEXEC");
    CHECK((fl1 & FD_CLOEXEC) != 0, "pipe2 write-end FD_CLOEXEC");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  5. pipe2() with O_NONBLOCK                                          */
/* ------------------------------------------------------------------ */
static void test_pipe2_nonblock(void)
{
    int fds[2];
    CHECK(pipe2(fds, O_NONBLOCK) == 0, "pipe2(O_NONBLOCK)");

    /* read on empty pipe must return EAGAIN, not block */
    char buf[4];
    errno = 0;
    int r = read(fds[0], buf, sizeof(buf));
    CHECK(r == -1 && errno == EAGAIN, "nonblock read on empty pipe > EAGAIN");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  6. poll() on pipe — POLLIN when data available                      */
/* ------------------------------------------------------------------ */
static void test_poll_pollin(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for poll POLLIN test");

    write(fds[1], "Z", 1);

    struct pollfd pfd = { .fd = fds[0], .events = POLLIN };
    int r = poll(&pfd, 1, 0);  // timeout=0: immediate check
    CHECK(r == 1 && (pfd.revents & POLLIN), "poll POLLIN on pipe with data");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  7. poll() on pipe — no POLLIN when empty                            */
/* ------------------------------------------------------------------ */
static void test_poll_empty(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for poll empty test");

    struct pollfd pfd = { .fd = fds[0], .events = POLLIN };
    int r = poll(&pfd, 1, 0);
    CHECK(r == 0, "poll returns 0 on empty pipe (timeout=0)");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  8. poll() POLLHUP when writer closed                                */
/* ------------------------------------------------------------------ */
static void test_poll_hup(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for poll POLLHUP test");

    close(fds[1]);  // close writer

    struct pollfd pfd = { .fd = fds[0], .events = POLLIN };
    int r = poll(&pfd, 1, 0);
    CHECK(r >= 1 && (pfd.revents & POLLHUP), "poll POLLHUP after writer closed");

    close(fds[0]);
}

/* ------------------------------------------------------------------ */
/*  9. poll() POLLOUT — write end ready when buffer not full            */
/* ------------------------------------------------------------------ */
static void test_poll_pollout(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for poll POLLOUT test");

    struct pollfd pfd = { .fd = fds[1], .events = POLLOUT };
    int r = poll(&pfd, 1, 0);
    CHECK(r == 1 && (pfd.revents & POLLOUT), "poll POLLOUT on empty pipe");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  10. dup2() redirects pipe into posix_spawn child stdout             */
/*      Parent writes to pipe, child reads via stdin                    */
/* ------------------------------------------------------------------ */
static void test_spawn_pipe(char *self_path)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for spawn test");

    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    /* child stdin < pipe read end */
    posix_spawn_file_actions_adddup2(&fa, fds[0], STDIN_FILENO);
    posix_spawn_file_actions_addclose(&fa, fds[1]);

    char *child_argv[] = { self_path, "--pipe-child", NULL };
    pid_t pid;
    int r = posix_spawnp(&pid, self_path, &fa, NULL, child_argv, NULL);
    posix_spawn_file_actions_destroy(&fa);

    if (r != 0) {
        printf("spawn pipe child: FAILED (r=%d)\n", r);
        g_failed++;
        close(fds[0]);
        close(fds[1]);
        return;
    }

    /* parent closes read end, writes to child's stdin via pipe */
    close(fds[0]);
    const char *token = "PIPE_OK\n";
    write(fds[1], token, strlen(token));
    close(fds[1]);  // signal EOF to child

    int status = 0;
    waitpid(pid, &status, 0);
    CHECK((status >> 8) == 0, "spawn pipe child exit status");
}

/* child mode: read one line from stdin, exit 0 if it matches token */
static int child_pipe_mode(void)
{
    char buf[32] = {0};
    int r = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (r <= 0) return 1;
    buf[r] = '\0';
    /* strip newline */
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    if (strcmp(buf, "PIPE_OK") == 0) {
        printf("pipe child: got token OK\n");
        return 0;
    }
    printf("pipe child: unexpected token '%s'\n", buf);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  11. Large write — multiple chunks                                   */
/* ------------------------------------------------------------------ */
static void test_large_write(void)
{
    int fds[2];
    CHECK(pipe2(fds, O_NONBLOCK) == 0, "pipe2 for large write test");

    /* fill buffer until EAGAIN */
    char chunk[64];
    memset(chunk, 'A', sizeof(chunk));
    int total_written = 0;
    while (1) {
        int w = write(fds[1], chunk, sizeof(chunk));
        if (w < 0) {
            if (errno == EAGAIN) break;
            printf("large write unexpected error: FAILED\n");
            g_failed++;
            goto out;
        }
        total_written += w;
        if (total_written > 65536) break; /* guard */
    }
    CHECK(total_written > 0, "large write: wrote some bytes");

    /* drain */
    int total_read = 0;
    while (total_read < total_written) {
        int r = read(fds[0], chunk, sizeof(chunk));
        if (r < 0) {
            if (errno == EAGAIN) break;
            printf("large read unexpected error: FAILED\n");
            g_failed++;
            goto out;
        }
        if (r == 0) break;
        total_read += r;
    }
    CHECK(total_read == total_written, "large write: read back same amount");

out:
    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  12. fstat() on pipe fd — must report S_IFIFO                       */
/* ------------------------------------------------------------------ */
static void test_fstat_pipe(void)
{
    int fds[2];
    CHECK(pipe(fds) == 0, "pipe() for fstat test");

    struct stat st;
    int r = fstat(fds[0], &st);
    CHECK(r == 0, "fstat on pipe fd");
    CHECK(S_ISFIFO(st.st_mode), "fstat: pipe is FIFO");

    close(fds[0]);
    close(fds[1]);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    /* child mode for test 10 */
    if (argc > 1 && strcmp(argv[1], "--pipe-child") == 0) {
        return child_pipe_mode();
    }

    printf("=== pipe/pipe2 test suite ===\n");

    test_basic();
    test_eof_on_writer_close();
    test_epipe();
    test_pipe2_cloexec();
    test_pipe2_nonblock();
    test_poll_pollin();
    test_poll_empty();
    test_poll_hup();
    test_poll_pollout();
    test_spawn_pipe(argv[0]);
    test_large_write();
    test_fstat_pipe();

    printf("=== %s ===\n", g_failed == 0 ? "ALL PASSED" : "SOME FAILED");
    return g_failed == 0 ? 0 : 1;
}
