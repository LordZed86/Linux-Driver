# Demo Narration (what to say at each step)

Each step has a **short** version (one or two sentences, if you're tight on time) and a **full** version (use it where it ties to a CS 370 concept). Pick the full version for steps 2, 5, 7, 8, and 9–10, and keep the others short. Total demo target: about 5 minutes (it is part of the 12–15 minute slot, so rehearse it with the slides).

Script: `demo.sh` in this folder. Run it from the repo root inside the VM. Steps 9–10 reload the module with `verbose=0`, so the log stays quiet during the race test.

**Backup plan:** if the unlocked race (step 10) shows 0 torn reads live, say so and show the saved runs in `docs/evidence/stage4/stage4_test_race_lock0_run*.txt` (run 1 had 7,647 torn reads). Have a screen recording of a clean run ready in case the VM misbehaves on Zoom.

---

## Step 1: Build the module

```
CC [M]  mychardev.o
MODPOST Module.symvers
CC [M]  mychardev.mod.o
CC [M]  .module-common.o
LD [M]  mychardev.ko
```

**Short:** "make runs the kernel's build system: compile, post-process, then link into a .ko file."

**Full, line by line:**
- **CC [M] mychardev.o:** "CC is the C compiler. The [M] means it's building a module. It turns my C source into an object file, machine code with the calls to kernel functions left as placeholders."
- **MODPOST:** "A kernel build step that scans the object files, checks which kernel symbols the module needs, and generates a small extra C file with the module's metadata: its name, and the exact kernel version it was built for."
- **CC [M] mychardev.mod.o:** "That generated file gets compiled too. `.module-common.o` is shared module boilerplate."
- **LD [M] mychardev.ko:** "LD is the linker. It combines my object file and the metadata into the final module. This is the linking step from our lectures: it's still a relocatable object, not a standalone program, because the kernel finishes the linking when it loads."

**ls -l mychardev.ko:** "About 355 KB. The code itself is tiny. Most of the size is debug information."

---

## Step 2: Load it

```
$ sudo insmod mychardev.ko
$ lsmod | grep mychardev
mychardev              12288  0
$ ls -l /dev/mychardev
crw------- 1 root root 239, 0 ...
```

**insmod, short:** "insmod asks the kernel to load the module and run my init function."

**insmod, full:** "The kernel does the final linking here. It fills in the real addresses of functions like `cdev_add` and `device_create`, the placeholders we saw in objdump, and then calls my init function."

**lsmod:** "The module is loaded. 12288 bytes is three 4 KB pages of memory. The 0 means no other module depends on it."

**crw------- (this is worth a full explanation):**
- "The first character, **c**, means a character device: a stream of bytes read and written in order, like a keyboard or terminal. A **b** would be a block device, like a disk."
- "Then three groups of three permission bits: owner, group, others. **rw-** means the owner, root, can read and write. The other two groups get nothing, `---`."
- "Each group is a 3-bit number: read is 4, write is 2, execute is 1. So rw- is 4 plus 2 equals 6, and the whole thing is octal **600**. That's the number-systems lecture."
- "That's why I need sudo to touch the device. The system created the node as root-only by default."
- "Where a file size would go, it shows **239, 0**. That's the **major** and **minor** number. Major 239 identifies the driver, which is mine. Minor 0 identifies which device that driver handles. Both are packed into one integer: shifted right by 20 gives the major, and masked with `0xFFFFF` gives the minor. That's the shift and mask I showed in the objdump."

---

## Step 3: Read with nothing stored

```
read() on an empty buffer returned 0 (0 means end of file)
PASS: empty buffer reads as end of file
```

**Say:** "Right after loading, the buffer is empty. Read returns zero, which is how Linux says end of file. That's the empty-buffer edge case from my proposal."

---

## Step 4a: Write to the device (shell)

```
$ echo hello | sudo tee /dev/mychardev
hello
```

**Say:** "echo sends hello plus a newline, six bytes, into tee, and tee opens the device and calls write. That's a system call that crosses into the kernel and lands in my write handler. The hello you see printed is tee echoing its own input, not data coming back from my driver."

---

## Step 4b: Read back (shell)

```
$ sudo cat /dev/mychardev
hello
```

**Say:** "cat opens the device and calls read. My read handler finds the six bytes that tee stored in the kernel buffer and copies them back with copy_to_user, so hello prints. cat calls read again, gets zero, which means end of file, and stops."

**Contrast with the midterm:** "At the midterm, this printed nothing, because read returned zero and write threw the data away. The buffer is the main thing I added."

---

## Step 5: Test program round trip

```
write buffer is at user address 0x5c65...  (21 bytes)
write() returned 21
read buffer is at user address 0x7fff...
read() returned 21: "hello from user space"
PASS: data read back matches data written
```

**Short:** "My own C program does the same thing as tee and cat, but checks the result itself and prints PASS."

**Full (user vs. kernel space):** "Look at the two addresses. The read buffer, 0x7fff-something, is on my program's stack. The write buffer, 0x5c-something, is the string constant compiled into the program. Both are user-space addresses, and they change every run because of address randomization. The kernel buffer is at 0xffffffffc09a..., in kernel space. The two sides can't see each other directly. My driver never dereferences a user pointer; it calls copy_from_user and copy_to_user, which check that the address really belongs to the calling process before copying."

---

## Step 6: Edge case, oversized write

```
write of 2000 bytes returned 1024 (short write: buffer holds 1024)
second write returned -1, errno = 28 (No space left on device)
PASS: short write, then ENOSPC
```

**Say:** "I write 2000 bytes into a 1024-byte buffer. The driver doesn't overflow. It accepts what fits and returns 1024, a short write, which is allowed in Linux. The program tries to write the rest, and since the buffer is full, the driver returns ENOSPC, error 28. That bounds check is what keeps a user program from writing past the end of kernel memory."

---

## Step 7: Edge case, bad pointer

```
write from address 0x1 returned -1, errno = 14 (Bad address)
PASS: kernel rejected the bad pointer instead of crashing
```

**Say:** "Now I pass the address 1, which is not valid memory. If my driver had just used memcpy on that pointer, it would crash in kernel mode, and that can take down the whole machine. copy_from_user catches the fault and my handler returns EFAULT, error 14. The program gets an error and the kernel keeps running."

---

## Step 8: The kernel log

```
mychardev: allocated major 239, minor 0
mychardev: module loaded, /dev/mychardev created (buffer 1024 bytes, lock on)
mychardev: open by pid ... (test_driver)
mychardev: read of 1024 bytes by test_driver: no more data (returning 0)
mychardev: write of 6 bytes by tee: stored 6
mychardev: read of 65536 bytes by cat: returned 6
mychardev: read of 65536 bytes by cat: no more data (returning 0)
mychardev: write of 2000 bytes by test_driver: stored 1024
mychardev: write of 976 bytes by test_driver: buffer full, returning -ENOSPC
mychardev: write by test_driver: bad user address, returning -EFAULT
mychardev: release by pid ... (test_driver)
```

(The `tail -n 20` hides the earlier lines. Replace this block with the real output after the first rehearsal.)

**Setup (short):** "Kernel code can't print to the terminal, so my module writes to the kernel log with printk, and dmesg reads it."

**Full:**
- **pid and command name:** "Each handler reads the current process from the kernel. That's the process model: I can see exactly who used my device, tee, cat, or test_driver."
- **65536 vs. 6:** "cat asked for 64 KB and got 6 bytes, because that's all that was stored. Then a second read returns zero and cat stops."
- **stored 1024 / -ENOSPC / -EFAULT:** "These are the edge cases from steps 6 and 7, seen from the kernel side."

**Big picture, one sentence:** "Every line here is a system call from a user program that crossed into the kernel and ran code I wrote."

---

## Step 9: Mutex on

```
mychardev: module loaded, /dev/mychardev created (buffer 1024 bytes, lock on)
2 writers, 2 readers, 500000 iterations each
reads checked: 1000000, torn reads: 0
PASS: no torn reads
```

**Say:** "Every process that opens the device shares one kernel buffer, so this is shared memory. This test forks four processes. Two writers fill the whole buffer with A's or B's, over and over. Two readers check that every read is all one letter. Each process opens the device once and then loops as fast as it can. A mix of letters is a torn read: the reader caught the buffer halfway through a write. With the mutex on, only one process can be inside the buffer at a time, and there are zero torn reads out of a million."

**Why a mutex and not a spinlock (if asked):** "copy_from_user can sleep if the user page isn't in memory, and you can't sleep while holding a spinlock. A mutex lets the waiting process sleep."

---

## Step 10: Mutex off

```
mychardev: module loaded, /dev/mychardev created (buffer 1024 bytes, lock OFF)
reads checked: 1000000, torn reads: <some number>
FAIL: readers saw partly-written data (is the lock off?)
```

**Say:** "Same test, same code, but I loaded the module with use_lock=0, which skips the mutex. Now readers catch the buffer mid-write and the test fails. That FAIL is the expected result here. It shows the race condition the mutex prevents. This was the change suggested at my midterm."

**If it shows 0 torn reads:** "Races depend on timing, and this VM has only two CPUs, so it doesn't always show up live. Here are three runs I saved earlier," then open the `race_lock0` evidence files.

---

## Step 11: Unload, then try to use the device

```
$ sudo rmmod mychardev
mychardev: module unloaded
$ sudo cat /dev/mychardev
cat: /dev/mychardev: No such file or directory
```

**Say:** "rmmod runs my exit function, which tears everything down in reverse order of init. The device node is gone, so opening it fails. That's the last edge case from my proposal: opening the device when the module isn't loaded."

---

## If you're short on time

Cut these first: step 4 (the shell round trip, since step 5 shows the same thing), the lsmod explanation, and the taint explanation (just say "expected for a self-built module"). Keep: the user vs. kernel addresses (step 5), the bad pointer (step 7), and the mutex on/off (steps 9–10). Those are the core of the final project.

## Practice tip

Run `demo.sh` with this file open and narrate out loud with a timer. Practice the whole 12–15 minutes at least twice, slides and demo together, over Zoom screen share if you can, so the font size and window layout are tested too.
