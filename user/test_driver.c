/*
 * test_driver.c - user-space test program for /dev/mychardev (stage 4)
 *
 * Build:  gcc -std=c99 -Wall -Wextra -pedantic -o test_driver test_driver.c
 * Run:    sudo ./test_driver [rw [text] | empty | big | fault | race]
 *
 *   rw     write a string, close, reopen, read it back (default)
 *   empty  read from the device with nothing stored (run right after insmod)
 *   big    write 2000 bytes into the 1024-byte buffer
 *   fault  pass a bad pointer to write(); the driver should answer EFAULT
 *   race   2 writers and 2 readers hammer the buffer; count torn reads
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEV "/dev/mychardev"
#define BUF_SIZE 1024 /* must match the driver */

/* ---- rw: write a string, then read it back ---------------------------- */
static int cmd_rw(const char *text)
{
    char rbuf[BUF_SIZE + 1];
    size_t len = strlen(text);
    ssize_t n;
    int fd;

    fd = open(DEV, O_WRONLY);
    if (fd < 0)
    {
        perror("open for write");
        return 1;
    }
    printf("write buffer is at user address %p (%zu bytes)\n",
           (const void *)text, len);
    n = write(fd, text, len);
    if (n < 0)
    {
        perror("write");
        close(fd);
        return 1;
    }
    printf("write() returned %zd\n", n);
    close(fd);

    fd = open(DEV, O_RDONLY);
    if (fd < 0)
    {
        perror("open for read");
        return 1;
    }
    printf("read buffer is at user address %p\n", (void *)rbuf);
    n = read(fd, rbuf, BUF_SIZE);
    if (n < 0)
    {
        perror("read");
        close(fd);
        return 1;
    }
    rbuf[n] = '\0';
    printf("read() returned %zd: \"%s\"\n", n, rbuf);
    close(fd);

    if ((size_t)n == len && memcmp(rbuf, text, len) == 0)
    {
        puts("PASS: data read back matches data written");
        return 0;
    }
    puts("FAIL: data read back does not match");
    return 1;
}

/* ---- empty: read with nothing stored ---------------------------------- */
static int cmd_empty(void)
{
    char rbuf[BUF_SIZE];
    ssize_t n;
    int fd = open(DEV, O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    n = read(fd, rbuf, sizeof rbuf);
    printf("read() on an empty buffer returned %zd (0 means end of file)\n", n);
    close(fd);
    return n == 0 ? 0 : 1;
}

/* ---- big: write more than the buffer holds ---------------------------- */
static int cmd_big(void)
{
    char big[2000];
    ssize_t n1, n2;
    int fd = open(DEV, O_WRONLY);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    memset(big, 'X', sizeof big);

    n1 = write(fd, big, sizeof big);
    printf("write of %zu bytes returned %zd (short write: buffer holds %d)\n",
           sizeof big, n1, BUF_SIZE);

    if (n1 < 0)
    {
        perror("first write");
        close(fd);
        return 1;
    }
    errno = 0;
    n2 = write(fd, big + n1, sizeof big - (size_t)n1);
    printf("second write returned %zd, errno = %d (%s)\n",
           n2, errno, errno ? strerror(errno) : "none");
    close(fd);

    return (n1 == BUF_SIZE && n2 == -1 && errno == ENOSPC) ? 0 : 1;
}

/* ---- fault: bad user pointer ------------------------------------------ */
static int cmd_fault(void)
{
    const void *bad = (const void *)(uintptr_t)1; /* not a valid address */
    ssize_t n;
    int fd = open(DEV, O_WRONLY);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }
    errno = 0;
    n = write(fd, bad, 16);
    printf("write from address %p returned %zd, errno = %d (%s)\n",
           bad, n, errno, strerror(errno));
    close(fd);

    if (n == -1 && errno == EFAULT)
    {
        puts("PASS: kernel rejected the bad pointer instead of crashing");
        return 0;
    }
    puts("FAIL: expected EFAULT");
    return 1;
}

/* ---- race: concurrent writers and readers ----------------------------- */
#define WRITERS 2
#define READERS 2
#define ITER 2000

struct result
{
    int checked;
    int torn;
};

static void writer_proc(int id)
{
    char fill[BUF_SIZE];

    memset(fill, 'A' + id, sizeof fill);
    for (int i = 0; i < ITER; i++)
    {
        int fd = open(DEV, O_WRONLY);

        if (fd < 0)
            _exit(1);
        if (write(fd, fill, sizeof fill) != BUF_SIZE)
            _exit(1);
        close(fd);
    }
    _exit(0);
}

/* A "torn" read contains a mix of letters, or is not a full buffer. */
static void reader_proc(int wfd)
{
    struct result r = {0, 0};
    char buf[BUF_SIZE];

    for (int i = 0; i < ITER; i++)
    {
        int fd = open(DEV, O_RDONLY);
        ssize_t n;

        if (fd < 0)
            _exit(1);
        n = read(fd, buf, sizeof buf);
        close(fd);

        r.checked++;
        if (n != BUF_SIZE)
        {
            r.torn++;
            continue;
        }
        for (int j = 1; j < BUF_SIZE; j++)
        {
            if (buf[j] != buf[0])
            {
                r.torn++;
                break;
            }
        }
    }
    if (write(wfd, &r, sizeof r) != (ssize_t)sizeof r)
        _exit(1);
    _exit(0);
}

static int cmd_race(void)
{
    char seed[BUF_SIZE];
    struct result r, total = {0, 0};
    int pfd[2];
    int fd;

    /* Start from a full, uniform buffer so every read should be 1024 bytes. */
    memset(seed, 'A', sizeof seed);
    fd = open(DEV, O_WRONLY);
    if (fd < 0 || write(fd, seed, sizeof seed) != BUF_SIZE)
    {
        perror("seeding the buffer");
        return 1;
    }
    close(fd);

    if (pipe(pfd) < 0)
    {
        perror("pipe");
        return 1;
    }
    for (int i = 0; i < WRITERS + READERS; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            return 1;
        }
        if (pid == 0)
        {
            close(pfd[0]);
            if (i < WRITERS)
                writer_proc(i);
            else
                reader_proc(pfd[1]);
        }
    }
    close(pfd[1]);

    while (read(pfd[0], &r, sizeof r) == (ssize_t)sizeof r)
    {
        total.checked += r.checked;
        total.torn += r.torn;
    }
    while (wait(NULL) > 0)
        ;

    printf("%d writers, %d readers, %d iterations each\n",
           WRITERS, READERS, ITER);
    printf("reads checked: %d, torn reads: %d\n", total.checked, total.torn);
    if (total.torn == 0)
    {
        puts("PASS: no torn reads");
        return 0;
    }
    puts("FAIL: readers saw partly-written data (is the lock off?)");
    return 1;
}

int main(int argc, char **argv)
{
    const char *cmd = argc > 1 ? argv[1] : "rw";

    if (strcmp(cmd, "rw") == 0)
        return cmd_rw(argc > 2 ? argv[2] : "hello from user space");
    if (strcmp(cmd, "empty") == 0)
        return cmd_empty();
    if (strcmp(cmd, "big") == 0)
        return cmd_big();
    if (strcmp(cmd, "fault") == 0)
        return cmd_fault();
    if (strcmp(cmd, "race") == 0)
        return cmd_race();

    fprintf(stderr, "usage: %s [rw [text] | empty | big | fault | race]\n",
            argv[0]);
    return 2;
}