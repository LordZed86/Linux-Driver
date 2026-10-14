// SPDX-License-Identifier: GPL-2.0
/*
 * mychardev.c - virtual character device driver (CS 370 project)
 *
 * Stage 1: hello-world module (done).
 * Stage 3: register the character device and stub open/read/write/release
 *          so each operation logs to the kernel log (this file).
 * Stage 4: add the kernel buffer with copy_to_user / copy_from_user.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define CLASS_NAME "mychardev_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kellen Jones");
MODULE_DESCRIPTION("Virtual character device driver for CS 370");
MODULE_VERSION("0.2");

static dev_t dev_num;		   /* major and minor packed into one number */
static struct cdev my_cdev;	   /* kernel's representation of our device  */
static struct class *my_class; /* lets udev create /dev/mychardev        */

/* Called when a process opens /dev/mychardev. */
static int mychardev_open(struct inode *inode, struct file *filp)
{
	pr_info("mychardev: open by pid %d (%s)\n", current->pid, current->comm);
	return 0;
}

/* Called when the last reference to the open file is closed. */
static int mychardev_release(struct inode *inode, struct file *filp)
{
	pr_info("mychardev: release by pid %d (%s)\n", current->pid, current->comm);
	return 0;
}

/*
 * Called for read(fd, buf, count). 'ubuf' is a USER-SPACE pointer, so the
 * kernel must never dereference it directly.
 * TODO (stage 4): copy data from a kernel buffer with copy_to_user().
 */
static ssize_t mychardev_read(struct file *filp, char __user *ubuf,
							  size_t count, loff_t *ppos)
{
	pr_info("mychardev: read of %zu bytes requested\n", count);
	return 0; /* 0 means end of file: nothing to return yet */
}

/*
 * Called for write(fd, buf, count).
 * TODO (stage 4): store data in a kernel buffer with copy_from_user().
 */
static ssize_t mychardev_write(struct file *filp, const char __user *ubuf,
							   size_t count, loff_t *ppos)
{
	pr_info("mychardev: write of %zu bytes received\n", count);
	return count; /* pretend all bytes were consumed (data is discarded) */
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

	pr_info("mychardev: module loaded, /dev/%s created\n", DEVICE_NAME);
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
