// SPDX-License-Identifier: GPL-2.0
/*
 * mychardev.c - virtual character device driver (CS 370 project)
 *
 * Stage 1: hello-world module (this file, as written).
 * Stage 3: register the character device and stub open/read/write/release.
 * Stage 4: add the kernel buffer with copy_to_user / copy_from_user.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kellen Jones");
MODULE_DESCRIPTION("Virtual character device driver for CS 370");
MODULE_VERSION("0.1");

/* Runs when the module is loaded with insmod. */
static int __init mychardev_init(void)
{
	pr_info("mychardev: module loaded\n");

	/* TODO (stage 3): allocate a device number and register the device. */
	return 0;
}

/* Runs when the module is removed with rmmod. */
static void __exit mychardev_exit(void)
{
	/* TODO (stage 3): unregister the device and free resources. */
	pr_info("mychardev: module unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);
