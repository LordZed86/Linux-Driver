# Demo Narration (what to say at each step)

Each step has a **short** version (one or two sentences, if you're tight on time) and a **full** version (use it where it ties to a CS 370 concept). Pick the full version for steps 1, 2, and 5, and keep the others short. Total demo target: about 3 minutes.

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

## Step 3: Write to the device

```
$ echo hello | sudo tee /dev/mychardev
hello
```

**Say:** "echo sends hello plus a newline, six bytes, into tee, and tee opens the device and calls write. That's a system call that crosses into the kernel and lands in my write handler. The hello you see printed is tee echoing its own input, not data coming back from my driver."

---

## Step 4: Read from the device

```
$ sudo cat /dev/mychardev
hello
```

**Say (stage 4):** "cat opens the device and calls read. This time my read handler finds the six bytes that step 3 stored in the kernel buffer and copies them back to cat with copy_to_user, so hello prints. A second read returns zero, which is end of file, and cat stops."

**Stage 3 version (midterm):** nothing printed, because read returned zero and the data was discarded.

**Log lines to expect in step 5 (stage 4):** `write of 6 bytes by tee: stored 6` and `read of 65536 bytes by cat: returned 6`, then `read of 65536 bytes by cat: no more data (returning 0)`.

---

## Step 5: The kernel log

```
mychardev: loading out-of-tree module taints kernel.
mychardev: module verification failed: signature and/or required key missing - tainting kernel
mychardev: allocated major 239, minor 0
mychardev: module loaded, /dev/mychardev created
mychardev: open by pid 1758 (tee)
mychardev: write of 6 bytes received
mychardev: release by pid 1758 (tee)
mychardev: open by pid 1762 (cat)
mychardev: read of 65536 bytes requested
mychardev: release by pid 1762 (cat)
```

**Setup (short):** "Kernel code can't print to the terminal, so my module writes to the kernel log with printk. dmesg reads that log. The dash-t flag drops the timestamps so it's easier to read."

**Full, in order:**
- **The first two lines (taint):** "The kernel notes that my module isn't part of the kernel and isn't signed, so it marks itself 'tainted'. It still works. It's a flag for debugging. A machine with Secure Boot would refuse to load it."
- **allocated major 239, minor 0:** "My init function asked the kernel for a free device number, and this is what it got. It matches the node we saw with ls."
- **module loaded:** "Init finished: the device is registered and the node exists."
- **open by pid 1758 (tee):** "My open handler ran. The kernel keeps a record of the running process, and my handler reads its pid and command name from it. That's the OS process model: I can see exactly who is using my device."
- **write of 6 bytes received:** "The write handler. Six bytes, matching hello plus the newline."
- **release by pid 1758:** "Release runs when the file is closed, which happened when tee finished."
- **open by pid 1762 (cat), read of 65536 bytes requested:** "A different process, cat, and a different handler. cat asks for 65,536 bytes, a 64 KB buffer. My handler returns zero."
- **release by pid 1762:** "And cat closes it."

**Big picture, one sentence:** "Every line here is a system call from a user program that crossed into the kernel and ran code I wrote."

---

## Step 6: Unload it

```
$ sudo rmmod mychardev
mychardev: module unloaded
```

**Say:** "rmmod unloads it. The kernel runs my exit function, which tears down the device in reverse order of how init built it, and logs that it's gone."

---

## If you're short on time

Cut these first: the `.module-common.o` explanation, the lsmod explanation, and the taint explanation (just say "expected for a self-built module"). Keep: the crw------- breakdown, the pid lines, and the one-sentence big picture.

## Practice tip

Run `./demo.sh` with this file open and narrate out loud with a timer. The crw------- and the log are where you'll want to slow down, because those are the parts that connect to the course.