# Midterm Presentation Notes (5-7 minutes)

Target: about 6 minutes. Say it in your own words; these are cues, not a script to read.

## Script

**Title slide (10 sec)**
"Good morning. I'm Kellen, and my project is a virtual character device driver for Linux."

**Slide 1: What & Why (45 sec)**
"I built a Linux kernel module that registers a virtual device, /dev/mychardev, plus a user-space side that talks to it. I picked it because a driver sits exactly where user programs meet the kernel, which is what this course is about. It's virtual on purpose: I'm new to kernel work, so no real hardware."

**Slide 2: How It Works (60 sec)**
"A program calls open, read, write, or close. That's a system call, which crosses from user space into the kernel. The kernel finds my device and uses a table called file_operations to jump to my handler. Right now the handlers log to the kernel log. Read returns zero bytes and write discards data, because the buffer is stage 4."

**Slide 3: CS 370 Concepts (75 sec)**
Pick three and say one sentence each:

- "In objdump, my if (ret < 0) check became a test instruction and a conditional jump."
- "MAJOR and MINOR compile to a shift right by 20 and a mask. My device came up as major 239, minor 0."
- "The module is a relocatable ELF file. The calls to kernel functions are placeholders that get filled in when it loads."
- "The log names the process using the device, like tee and cat, which is the OS process model."

**Live demo (2.5 min)**: run `./demo.sh` in the VM. Narrate each step in one sentence:

1. Build: "make compiles it against the kernel headers into a .ko."
2. Load: "insmod loads it. The device node appears, owned by root, so crw-------."
3. Write: "tee writes six bytes, hello plus a newline."
4. Read: "cat reads and gets nothing, as expected."
5. Log: "Here's each handler firing, with the pid and command."
6. Unload: "rmmod removes it."

**Slide 4: Challenges & Next Steps (45 sec)**
"The hardest part was a safe setup: kernel bugs can crash a machine, so I load modules only in a VM. Tutorials also target older kernels. Next is the kernel buffer with copy_from_user and copy_to_user, a test program that writes and reads, and edge cases."

**Thank you slide.** "Happy to take questions."

## Likely questions and answers

**Why can't the kernel just use the pointer a user program gives it?**
User and kernel space are separate address spaces. A user pointer could be invalid, or point at kernel memory. copy_from_user and copy_to_user check the address and fail safely instead of crashing the kernel.

**What do MAJOR and MINOR do?**
The kernel packs both numbers into one 32-bit integer. Major is the upper 12 bits (shift right by 20) and minor is the lower 20 bits (mask with 0xFFFFF). The major number identifies the driver; the minor identifies which device it handles.

**Why does cat show nothing?**
cat asks for 65536 bytes. My read handler returns 0, which means end of file, because the buffer isn't built yet.

**What does "taints kernel" mean?**
The module isn't signed and isn't part of the kernel tree, so the kernel flags itself as tainted. It still works. Secure Boot systems would refuse an unsigned module, but this machine doesn't use it.

**What does cdev_add do?**
It registers my file_operations table with the kernel for my device number, so system calls on that device reach my handlers.

**Why a VM?**
Module code runs inside the kernel, so a bug can crash the whole machine. In the VM, a crash only affects the guest, and I have a clean disk backup.

**Why are there `U` symbols in nm?**
Undefined symbols, like cdev_add and `_printk`. My module calls them but doesn't contain them; the kernel resolves them when the module loads.

**What are `__fentry__` and `__x86_return_thunk`?**
Extras the kernel build adds: a tracing hook at the start of functions and a security mitigation on returns. They're not in my C source.

**Why do the addresses in objdump look like 0?**
It's a relocatable object. Final addresses are assigned at load time, and the `.rela.*` sections list where to patch them.

**What's the difference between open and release?**
open runs each time a process opens the device. release runs when the last reference to that open file is closed, which is what close() triggers.

**Did you write this yourself? How did you use AI?**
Answer honestly and match your attribution document: AI helped with planning, setup troubleshooting, and drafting, and you should be able to explain every line.

**What would you do with more time?**
The stage 4 buffer, a test program that writes and reads, a mutex, and maybe a read that reports process info.

## Before you present (checklist)

- [x] SSH to the host and the VM works (see the commands below)
- [x] Run `./demo.sh` once start to finish tonight
- [x] Keynote exports to PDF as a backup copy
- [x] Backup: screenshot slide or recorded demo in case the VM misbehaves
- [x] Terminal: `export PS1='$ '`, large font, one window only, notifications off
- [x] Host sleep and lid-close settings changed so it doesn't suspend
- [x] Power cable plugged in, laptops charged
- [ ] Screen share tested with the meeting tool

## SSH check (run tonight and again tomorrow morning)

From your other computer:

```bash
ssh <host-user>@<mac-ip>
```

On the host:

```bash
sudo virsh list --all          # driverdev should say "running"
sudo virsh start driverdev     # only if it says "shut off"
sudo virsh domifaddr driverdev # the VM's address can change
```

Then the VM, from the host:

```bash
ssh dev@<vm-address>
```

Or in one hop from your laptop:

```bash
ssh -J <host-user>@<mac-ip> dev@<vm-address>
```

Inside the VM, confirm the module files exist:

```bash
ls ~/module
```

If the host IP changed, run `hostname -I` at the host itself. If the VM wedges, restore with:

```bash
sudo virsh destroy driverdev
sudo cp /var/lib/libvirt/images/driverdev-clean.qcow2 /var/lib/libvirt/images/driverdev.qcow2
sudo virsh start driverdev
```

After a restore, `~/module` comes back as it was at backup time, so re-copy the source with `scp` if needed.
