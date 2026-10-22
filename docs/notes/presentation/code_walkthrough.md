# Code Walkthrough: mychardev.c (stage 3)

Based on the stage 3 file.

## 1. Top of the file

```c
#include <linux/module.h> ...
#define DEVICE_NAME "mychardev"
MODULE_LICENSE("GPL"); MODULE_AUTHOR(...); MODULE_DESCRIPTION(...); MODULE_VERSION("0.2");
```

- **Headers:** kernel headers, not the C library. There is no `printf` or `malloc` here.
- **`MODULE_*` lines:** metadata stored in the `.modinfo` section. It's what `modinfo` prints. `MODULE_LICENSE("GPL")` matters: without a GPL-compatible license, the module can't use some kernel functions.

## 2. Module state

```c
static dev_t dev_num;          // major + minor packed in one integer
static struct cdev my_cdev;    // the kernel's object for a character device
static struct class *my_class; // lets udev create /dev/mychardev
```

- **`static`:** private to this file.
- **`dev_t`:** the packed device number. `MAJOR(dev_num)` is a shift right by 20, and `MINOR(dev_num)` is a mask of the low 20 bits. You saw both in `objdump`.
- **`dev_num`, `my_cdev`, `my_class`:** appear in `nm` as `b` symbols, which are uninitialized data in `.bss`.

## 3. The handlers

```c
static int mychardev_open(struct inode *inode, struct file *filp)
{
    pr_info("mychardev: open by pid %d (%s)\n", current->pid, current->comm);
    return 0;
}
```

- **`current`:** a pointer to the kernel's record (the task struct) of whichever process made the system call. `pid` and `comm` are fields in it. That's the process model.
- **`pr_info`:** the kernel's `printf`. It writes to the kernel log, which `dmesg` reads.
- **Return 0:** success. A negative number would be an error code, and `open` would fail.
- **`release`** has the same shape. It runs when the last reference to the open file closes.

```c
static ssize_t mychardev_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
    pr_info("mychardev: read of %zu bytes requested\n", count);
    return 0;      // 0 = end of file
}
```

- **`char __user *ubuf`:** a pointer into the calling process's memory. The `__user` marker is a note to the tools and to readers: the kernel must never dereference this directly. Stage 4 uses `copy_to_user`.
- **`count`:** the size the caller asked for. That's the 65536 you saw from `cat`.
- **Return value:** the number of bytes actually read. Returning 0 tells the caller there's no more data, so `cat` stops.

```c
static ssize_t mychardev_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos)
{
    pr_info("mychardev: write of %zu bytes received\n", count);
    return count;      // claim all bytes were consumed
}
```

- **Return `count`:** it tells the caller every byte was handled. Returning less would make `tee` retry. The data is discarded for now, which is why `read` can't return it.

## 4. The file_operations table

```c
static const struct file_operations mychardev_fops = {
    .owner   = THIS_MODULE,
    .open    = mychardev_open,
    .release = mychardev_release,
    .read    = mychardev_read,
    .write   = mychardev_write,
};
```

- **What it is:** a table of function pointers. When a system call hits my device, the kernel looks up the matching entry and calls it. It's how a system call turns into my code.
- **`.owner = THIS_MODULE`:** keeps the module from being unloaded while someone has the device open.
- **`const`:** the table never changes, so it lives in read-only data. In `nm` it's the `r` symbol `mychardev_fops`.

## 5. Init and exit

```c
static int __init mychardev_init(void)
{
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);  // 1. get a device number
    cdev_init(&my_cdev, &mychardev_fops);                    // 2. attach the table
    ret = cdev_add(&my_cdev, dev_num, 1);                    // 3. make it live
    my_class = class_create(CLASS_NAME);                     // 4. create a device class
    dev = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);  // 5. /dev node appears
    ...
}
```

In order:

1. **`alloc_chrdev_region`:** asks the kernel for a free major number. It fills in `dev_num`. Arguments: where to store it, first minor (0), how many (1), a name. In `objdump` these are the `rdi`, `esi`, `edx`, `rcx` setups.
2. **`cdev_init`:** connects the `cdev` object to my table.
3. **`cdev_add`:** registers it with the kernel. From here, system calls can reach my handlers.
4. **`class_create`:** creates a class under `/sys/class`. In kernels 6.4 and later it takes one argument. Older tutorials pass two.
5. **`device_create`:** makes the kernel announce a new device, and `udev` creates `/dev/mychardev`.

**Error handling:**

```c
if (ret < 0) { pr_err(...); goto err_region; }
...
err_class:  class_destroy(my_class);
err_cdev:   cdev_del(&my_cdev);
err_region: unregister_chrdev_region(dev_num, 1);
return ret;
```

- If a step fails, `goto` jumps into a ladder that undoes the earlier steps in reverse order, so nothing leaks.
- `if (ret < 0)` is the `test` plus `jns` you found in `objdump`. Kernel functions return a negative error code on failure.

```c
static void __exit mychardev_exit(void) { device_destroy(...); class_destroy(...); cdev_del(...); unregister_chrdev_region(...); }
module_init(mychardev_init);
module_exit(mychardev_exit);
```

- **Exit:** tears everything down in the reverse order of creation.
- **`__init` / `__exit`:** mark code used only at load or unload. They go in `.init.text` and `.exit.text` (you saw both in `readelf -S`), and the kernel can free the init memory afterward.
- **`module_init` / `module_exit`:** register the two functions. They create the `init_module` and `cleanup_module` names, which is why `nm` shows both names at the same address.

## Questions about the code, with short answers

- **Why `static` on everything?** It keeps names private, so they can't collide with names elsewhere in the kernel.
- **Why does `read` return 0 and not an error?** Zero means end of file, which is valid. An error would be a negative value, and `cat` would report a failure.
- **What would `copy_to_user` do that a normal `memcpy` wouldn't?** It checks that the destination is a valid user address and handles a fault safely, instead of crashing the kernel. It returns how many bytes it failed to copy.
- **What does `THIS_MODULE` do?** It's a pointer to this module's own record, used to pin the module in memory while it's in use.
- **What if `cdev_add` fails?** The `goto` jumps to the cleanup ladder, which releases the device number, and init returns the error, so `insmod` fails.
- **Why is the order in exit reversed?** Later steps depend on earlier ones, so you tear down the dependent ones first.
- **Why `pr_info` and not `printf`?** The kernel has no C library. `pr_info` is the kernel's logging function, and it writes to the ring buffer that `dmesg` reads.
- **Which parts did you write and which came from a guide?** Be honest. The structure follows the standard pattern from the Linux Kernel Module Programming Guide, and what matters is that I can explain each part.

## If I show code on screen

Show only the `file_operations` table and the first few lines of `mychardev_init`. Use a large font. They give me the best story: "this table is how system calls become my code, and init is where I register it."
