# Code Walkthrough: mychardev.c (stage 4)

Based on the stage 4 file (`mychardev_stage4.c`). Check it against your copy. The file is about 245 lines, and it falls into six parts. Parts 1, 4, and 5 (header, file_operations table, init and exit) are unchanged from stage 3.

## 1. Top of the file

```c
#include <linux/module.h> ...
#define DEVICE_NAME "mychardev"
MODULE_LICENSE("GPL"); MODULE_AUTHOR(...); MODULE_DESCRIPTION(...); MODULE_VERSION("0.3");
module_param(use_lock, bool, 0444);   // load-time switches: use_lock, verbose
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

## 2b. The buffer and the lock (new in stage 4)

```c
#define BUF_SIZE 1024
static char kbuf[BUF_SIZE];     // the shared kernel buffer
static size_t kbuf_len;         // how many bytes of it hold valid data
static DEFINE_MUTEX(buf_lock);  // one process in the buffer at a time
```

- **One buffer for everyone:** every process that opens the device shares `kbuf`. That sharing is why a lock is needed.
- **`kbuf_len`:** how much of the buffer is real data. `read` stops there.
- **Mutex, not spinlock:** `copy_to_user` and `copy_from_user` can sleep while the kernel handles a page fault. You can't sleep while holding a spinlock, but you can while holding a mutex.
- **`mutex_lock_interruptible`:** if a process is stuck waiting for the lock, a signal such as Ctrl-C can wake it. The handler then returns `-ERESTARTSYS`.
- **`use_lock` and `verbose`:** read-only module parameters (`insmod mychardev.ko use_lock=0`). `use_lock=0` skips the mutex to show what goes wrong without it. `verbose=0` silences the per-operation log lines for the stress test.

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
    lock;
    if (pos >= kbuf_len) { unlock; return 0; }   // nothing left: end of file
    n = min(count, kbuf_len - pos);              // never more than is stored
    if (copy_to_user(ubuf, kbuf + pos, n)) { unlock; return -EFAULT; }
    *ppos = pos + n;
    unlock;
    return n;
}
```

- **`char __user *ubuf`:** a pointer into the calling process's memory. The `__user` marker is a note to the tools and to readers: the kernel must never dereference it directly. `copy_to_user` checks the address and copies safely.
- **`count`:** the size the caller asked for (65536 from `cat`). The driver returns at most what is stored, so `cat` gets 6 bytes after `echo hello`.
- **`*ppos`:** the file position. After a read it moves forward, so the next read starts where this one ended and then hits `pos >= kbuf_len`, which returns 0. That is how `cat` knows to stop.
- **`copy_to_user` return value:** the number of bytes it could not copy. Anything nonzero means a bad user address, so the handler returns `-EFAULT`.
- **Release the lock on every exit path:** each early `return` unlocks first. A missed unlock would hang the next process forever.

```c
static ssize_t mychardev_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos)
{
    lock;
    if (pos >= BUF_SIZE) { unlock; return -ENOSPC; }   // buffer is full
    n = min(count, BUF_SIZE - pos);                    // shorten an oversized write
    if (copy_from_user(kbuf + pos, ubuf, n)) { unlock; return -EFAULT; }
    *ppos = pos + n;
    kbuf_len = pos + n;                                // stored length = end of this write
    unlock;
    return n;
}
```

- **`copy_from_user`:** the mirror of `copy_to_user`. It copies from the user pointer into `kbuf`, checking the address first.
- **Short write:** if a caller writes 2000 bytes, the driver stores 1024 and returns 1024. A well-behaved program calls `write` again with the rest, and that second call finds the buffer full and gets `-ENOSPC`. This is exactly what `test_driver big` shows.
- **`kbuf_len = pos + n`:** a fresh open starts at position 0, so a new write replaces the old contents instead of piling onto them.
- **Return `n`:** the number of bytes actually stored, not the number requested. That is why short writes are visible to the caller.

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
- **Why does `read` return 0 at the end and not an error?** Zero means end of file, which is valid. An error would be a negative value, and `cat` would report a failure.
- **What does `copy_to_user` do that a normal `memcpy` wouldn't?** It checks that the destination is a valid user address and handles a fault safely, instead of crashing the kernel. It returns how many bytes it failed to copy, and the driver turns any nonzero value into `-EFAULT`.
- **Why do you need a mutex?** The buffer is shared by every process that opens the device. Without a lock, two writers can interleave their bytes, or a reader can catch the buffer halfway through a write and see a mix of old and new data. The mutex makes each read and write happen as one unit.
- **Why a mutex and not a spinlock?** The copy functions can sleep on a page fault, and sleeping while holding a spinlock is not allowed. A mutex lets the waiting process sleep.
- **What is a torn read?** A read that returns part of one write and part of another. The `race` test counts them: with the lock on there should be none.
- **What does `use_lock=0` show?** It turns the lock off so readers and writers can overlap. Torn reads may then appear, which is the evidence that the mutex is doing real work. It needs more than one CPU core, so it may not show every run.
- **What if the lock is never released?** The next process to touch the device waits forever. That is why every early `return` in the handlers unlocks first.
- **What stops a write from running past the buffer?** `n = min(count, BUF_SIZE - pos)` shortens the copy, and a full buffer returns `-ENOSPC`. The bounds check is what keeps a user program from writing over other kernel memory.
- **What happens when a user passes a bad pointer?** `copy_from_user` fails, the handler returns `-EFAULT`, and the program sees errno `EFAULT`. The kernel keeps running. `test_driver fault` demonstrates this.
- **What does `THIS_MODULE` do?** It's a pointer to this module's own record, used to pin the module in memory while it's in use.
- **What if `cdev_add` fails?** The `goto` jumps to the cleanup ladder, which releases the device number, and init returns the error, so `insmod` fails.
- **Why is the order in exit reversed?** Later steps depend on earlier ones, so you tear down the dependent ones first.
- **Why `pr_info` and not `printf`?** The kernel has no C library. `pr_info` is the kernel's logging function, and it writes to the ring buffer that `dmesg` reads.
- **Which parts did you write and which came from a guide?** Be honest. The structure follows the standard pattern from the Linux Kernel Module Programming Guide, and what matters is that you can explain each part.

## If you show code on screen

Show only the `file_operations` table and the `read` handler's `copy_to_user` line (or the lock and `copy_from_user` lines in `write`). Use a large font. They give you the best story: "this table is how system calls become my code, and init is where I register it."