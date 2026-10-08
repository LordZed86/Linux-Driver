/*
 * test_driver.c - user-space test program for /dev/mychardev
 *
 * Build: gcc -std=c99 -g -Wall -o test_driver test_driver.c
 * Run:   ./test_driver   (or sudo ./test_driver if permissions require it)
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/mychardev"

int main(void)
{
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: %s\n", DEVICE_PATH, strerror(errno));
        return 1;
    }
    printf("opened %s (fd = %d)\n", DEVICE_PATH, fd);

    /* TODO (stage 4): write a string to the device with write(). */
    /* TODO (stage 4): read it back with read() and print it.      */
    /* TODO (stage 4): print buffer addresses for the report.      */

    if (close(fd) < 0) {
        fprintf(stderr, "close failed: %s\n", strerror(errno));
        return 1;
    }
    printf("closed %s\n", DEVICE_PATH);
    return 0;
}
