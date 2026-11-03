// SPDX-License-Identifier: GPL-2.0
/*
 * mychardev.c - virtual character device driver (CS 370 project)
 *
 * Stage 1: hello-world module (done).
 * Stage 3: register the character device and stub open/read/write/release
 *          so each operation logs to the kernel log (done).
 * Stage 4: fixed-size kernel buffer with copy_to_user / copy_from_user,
 *          protected by a mutex (this file).
 *
 * Behavior:
 *   - write stores data in a 1024-byte kernel buffer at the file position.
 *     The stored length becomes (position + bytes written), so a fresh
 *     open-and-write replaces the old contents.
 *   - read returns the stored data from the file position, then 0 (EOF).
 *   - A write past the end of the buffer is shortened; once the buffer is
 *     full, further writes fail with -ENOSPC.
 *   - A bad user pointer makes copy_to_user / copy_from_user fail, and the
 *     handler returns -EFAULT instead of crashing the kernel.
 *
 * Module parameters (both read-only after load):
 *   use_lock=0  turn the mutex off, to show what it protects against.
 *   verbose=0   silence the per-operation log lines (for the stress test).
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define CLASS_NAME "mychardev_class"
#define BUF_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kellen Jones");
MODULE_DESCRIPTION("Virtual character device driver for CS 370");
MODULE_VERSION("0.3");

static bool use_lock = true;
module_param(use_lock, bool, 0444);
MODULE_PARM_DESC(use_lock, "Protect the buffer with a mutex (default: 1)");

static bool verbose = true;
module_param(verbose, bool, 0444);
MODULE_PARM_DESC(verbose, "Log every open/read/write/release (default: 1)");

#define LOG(fmt, ...)                                  \
	do                                                 \
	{                                                  \
		if (verbose)                                   \
			pr_info("mychardev: " fmt, ##__VA_ARGS__); \
	} while (0)

static dev_t dev_num;		   /* major and minor packed into one number */
static struct cdev my_cdev;	   /* kernel's representation of our device  */
static struct class *my_class; /* lets udev create /dev/mychardev        */

/* The shared kernel buffer. Every process that opens the device uses it. */
static char kbuf[BUF_SIZE];
static size_t kbuf_len;		   /* how many bytes of kbuf hold valid data */
static DEFINE_MUTEX(buf_lock); /* only one process in the buffer at a time */

/*
 * Take and release the buffer lock. mutex_lock_interruptible() lets a
 * process that is waiting for the lock be woken by a signal (Ctrl-C).
 * A mutex (not a spinlock) is correct here because copy_to_user and
 * copy_from_user can sleep while they handle a page fault.
 */
static int buf_lock_acquire(void)
{
	if (!use_lock)
		return 0;
	return mutex_lock_interruptible(&buf_lock);
}

static void buf_lock_release(void)
{
	if (use_lock)
		mutex_unlock(&buf_lock);
}

/* Called when a process opens /dev/mychardev. */
static int mychardev_open(struct inode *inode, struct file *filp)
{
	LOG("open by pid %d (%s)\n", current->pid, current->comm);
	return 0;
}

/* Called when the last reference to the open file is closed. */
static int mychardev_release(struct inode *inode, struct file *filp)
{
	LOG("release by pid %d (%s)\n", current->pid, current->comm);
	return 0;
}

/*
 * Called for read(fd, buf, count). 'ubuf' is a USER-SPACE pointer, so the
 * kernel must never dereference it directly: copy_to_user() checks the
 * address and copies safely.
 */
static ssize_t mychardev_read(struct file *filp, char __user *ubuf,
							  size_t count, loff_t *ppos)
{
	size_t pos = *ppos;
	size_t len, n;

	if (buf_lock_acquire())
		return -ERESTARTSYS;

	len = kbuf_len;
	if (pos >= len)
	{
		buf_lock_release();
		LOG("read of %zu bytes by %s: no more data (returning 0)\n",
			count, current->comm);
		return 0; /* 0 means end of file */
	}

	n = min(count, len - pos);
	if (copy_to_user(ubuf, kbuf + pos, n))
	{
		buf_lock_release();
		LOG("read by %s: bad user address, returning -EFAULT\n",
			current->comm);
		return -EFAULT;
	}
	*ppos = pos + n;
	buf_lock_release();

	LOG("read of %zu bytes by %s: returned %zu\n", count, current->comm, n);
	return n;
}

/*
 * Called for write(fd, buf, count). 'ubuf' is a user-space pointer, so
 * copy_from_user() does the copy into the kernel buffer.
 */
static ssize_t mychardev_write(struct file *filp, const char __user *ubuf,
							   size_t count, loff_t *ppos)
{
	size_t pos = *ppos;
	size_t n;

	if (count == 0)
		return 0;

	if (buf_lock_acquire())
		return -ERESTARTSYS;

	if (pos >= BUF_SIZE)
	{
		buf_lock_release();
		LOG("write of %zu bytes by %s: buffer full, returning -ENOSPC\n",
			count, current->comm);
		return -ENOSPC;
	}

	n = min(count, (size_t)(BUF_SIZE - pos)); /* shorten if too big */
	if (copy_from_user(kbuf + pos, ubuf, n))
	{
		buf_lock_release();
		LOG("write by %s: bad user address, returning -EFAULT\n",
			current->comm);
		return -EFAULT;
	}
	*ppos = pos + n;
	kbuf_len = pos + n;
	buf_lock_release();

	LOG("write of %zu bytes by %s: stored %zu\n", count, current->comm, n);
	return n;
}

/* Connects system calls on the device to our handler functions. */
static const struct file_operations mychardev_fops = {
	.owner = THIS_MODULE,
	.open = mychardev_open,
	.release = mychardev_release,
	.read = mychardev_read,
	.write = mychardev_write,
};

static int __init mychardev_init(void)
{
	struct device *dev;
	int ret;

	/* Ask the kernel for an unused major number (minor starts at 0). */
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0)
	{
		pr_err("mychardev: could not allocate device number\n");
		return ret;
	}
	pr_info("mychardev: allocated major %d, minor %d\n",
			MAJOR(dev_num), MINOR(dev_num));

	/* Attach our file operations and make the device live. */
	cdev_init(&my_cdev, &mychardev_fops);
	my_cdev.owner = THIS_MODULE;
	ret = cdev_add(&my_cdev, dev_num, 1);
	if (ret < 0)
	{
		pr_err("mychardev: cdev_add failed\n");
		goto err_region;
	}

	/* Create the class and device so /dev/mychardev appears. */
	my_class = class_create(CLASS_NAME); /* one argument on kernels 6.4+ */
	if (IS_ERR(my_class))
	{
		ret = PTR_ERR(my_class);
		pr_err("mychardev: class_create failed\n");
		goto err_cdev;
	}

	dev = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(dev))
	{
		ret = PTR_ERR(dev);
		pr_err("mychardev: device_create failed\n");
		goto err_class;
	}

	pr_info("mychardev: module loaded, /dev/%s created (buffer %d bytes, lock %s)\n",
			DEVICE_NAME, BUF_SIZE, use_lock ? "on" : "OFF");
	return 0;

	/* Undo earlier steps in reverse order if a later one fails. */
err_class:
	class_destroy(my_class);
err_cdev:
	cdev_del(&my_cdev);
err_region:
	unregister_chrdev_region(dev_num, 1);
	return ret;
}

static void __exit mychardev_exit(void)
{
	/* Tear down in the reverse order of creation. */
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);
	pr_info("mychardev: module unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);