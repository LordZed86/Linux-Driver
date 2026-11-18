# CS 370 Final Report: Paste Guide

Nov 17, 2026 · @kellen is fine

## How to use this guide

Rename and reorder your midterm Pages document into the 9 required sections, move every midterm paragraph into its new home, and paste the final additions under a **Final project update** subheading in each section. The grader can then see the midterm content kept and the new work marked, which is what the guidelines ask for.

Work in a copy: duplicate the Pages file first (File → Duplicate) and name it "CS 370 Final Report".

Each section below has three parts:

- **Keep:** midterm text to move here unchanged (cut and paste from the old section).
- **Edit:** small wording changes so the midterm text isn't wrong now.
- **Add:** new text to paste under a "Final project update" subheading.

Placeholders in \[brackets\] need real results from the VM run. Expect 9–12 pages; the midterm was 5.

| Midterm section | Moves to (final section) |
| --- | --- |
| 1 Project title and purpose | 1 Project Introduction |
| 2 Project goals | 1 Project Introduction (as Objectives) |
| 3 What I have implemented | 4 Implementation |
| 4 CS 370 concepts table | 4 Implementation (table gets a Final column) |
| 5 Testing performed and results | 6 Testing and Results; Figure 1 → 5 Project Output |
| 6 Challenges | 7 Challenges and Solutions |
| 7 What remains to be developed | 8 Conclusion (as "Planned at midterm") |
| 8 Concepts I plan to apply next | 4 Implementation (below the concepts table) |
| 9 Technical evidence | 4 Implementation (as 4.x Technical evidence) |

Sections 2, 3, 5, and 9 are new, so they have no midterm text to move.

## Header and title block

**Edit:**

- Title: "CS 370 Midterm Report: Virtual Character Device Driver" → "CS 370 Final Project Report: Virtual Character Device Driver"
- Date: "Oct 13, 2026" → your submission date
- Page header: if "1 of 5" is typed text, replace it with Pages' automatic page number (Insert → Page Number, then type " of " and Insert → Page Count)

**Add** (one line under the title, in italics):

> This report extends my midterm report. Midterm content is kept in each section; work completed after the midterm appears under "Final project update" headings.

## Section 1: Project Introduction

Rename "1. Project title and purpose" to **1. Project Introduction**. It now covers the title, problem statement, objectives, and scope.

**Keep:** both midterm paragraphs of section 1, then the whole bullet list from midterm section 2 (Project goals), under a subheading **Objectives**.

**Edit:** none.

**Add** under a subheading **Problem statement**, between the paragraphs and the Objectives (the midterm text describes the motivation but never states the problem in one place):

> Device drivers are the code that lets the operating system talk to a device, and on Linux they often have to be written by the community. Writing one requires understanding exactly how a user program's system calls reach kernel code and how data crosses the boundary between user space and kernel space without crashing the system. This project builds that understanding by implementing a small driver for a virtual device from scratch.

**Add** under **Scope**, after the Objectives:

> In scope: a virtual character device with open, read, write, and release handlers; a fixed-size 1024-byte kernel buffer; a user-space test program; and analysis of the compiled module. Out of scope: real hardware, interrupts, USB or PCI drivers, and device trees.

**Add** under **Final project update: scope change**:

> After the midterm presentation, the instructor suggested protecting the kernel buffer with a mutex. Because every process that opens the device shares the same buffer, this moved from a stretch goal into the core scope. The remaining stretch goals (an ioctl command and an open counter) were not changed.

## Section 2: Requirements (new)

The midterm has no matching section. Paste the whole section. In Pages, both lists read best as two-column tables (ID, requirement).

**Add:**

> **Functional requirements**
>
> FR1. Loading the module allocates a device number and creates /dev/mychardev; unloading removes it. FR2. open, read, write, and release each run a handler that logs the calling process's pid and name. FR3. write copies data from the user program into a 1024-byte kernel buffer using copy\_from\_user. FR4. read copies stored data back to the user program using copy\_to\_user, then returns 0 (end of file). FR5. A write larger than the free space is shortened; once the buffer is full, write fails with ENOSPC. FR6. A bad user pointer makes the call fail with EFAULT, and a failed write leaves the stored data unchanged. FR7. A mutex lets only one process use the buffer at a time. FR8. A user-space test program checks each behavior and prints PASS or FAIL.
>
> **Non-functional requirements**
>
> NFR1. Safety: no input from user space can crash or freeze the kernel. NFR2. Isolation: new builds are loaded only in a QEMU/KVM virtual machine, never on the host. NFR3. Compatibility: builds against the running kernel's headers on Linux 7.0 (Ubuntu 26.04). NFR4. Portability: the test program is standard C99 with POSIX calls only. NFR5. Repeatability: one script runs every test and saves the output as evidence. NFR6. Privacy: saved evidence has usernames, hostnames, and home paths redacted.
>
> **Planned features:** the stretch goals from the proposal, an ioctl command (clear the buffer or report its size) and an open counter, are listed in section 8 as future scope.

## Section 3: System Design (new)

No matching midterm section. Paste the text, and insert the architecture diagram as **Figure 1**. You can screenshot it from the slide plan doc, or use your Keynote version from slide 4. Renumber the old dmesg figure to Figure 2 (it moves to section 5).

**Add:**

> **3.1 Architecture.** The system has two parts: a loadable kernel module (mychardev.ko) and a user-space test program (test\_driver). The program calls open, read, write, and close on /dev/mychardev. Each call is a system call: the CPU switches to kernel mode, and the kernel's virtual file system (VFS) uses the device number (major 239) to find the driver. The driver's file\_operations table maps each call to a handler function. The handlers share one buffer, protected by a mutex (Figure 1).
>
> **3.2 Workflow.** Loading the module runs mychardev\_init, which allocates a device number, registers the character device (cdev), and creates a device class and node so udev creates /dev/mychardev. If any step fails, the earlier steps are undone in reverse order. Unloading runs mychardev\_exit, which tears everything down in reverse order of creation. Between load and unload, every open, read, write, and close from any process reaches the handlers.
>
> **3.3 Memory design.** The driver keeps three pieces of shared state: kbuf, a 1024-byte static array; kbuf\_len, the number of valid bytes; and buf\_lock, a mutex. They live in kernel space, a separate address space from the test program (kernel addresses start near 0xffff…, user addresses near 0x7ff…). The kernel never dereferences a user pointer directly. Data crosses the boundary only through copy\_from\_user and copy\_to\_user, which check that the address belongs to the calling process. Writes are first copied into a temporary kernel buffer, so a failed copy never touches kbuf. Each open file has its own position (f\_pos), but all processes share the same buffer.
>
> **3.4 Concurrency design.** Because every process shares kbuf, a reader could see a buffer that is half old data and half new. The mutex makes each read and each update of kbuf happen as a single step. A mutex is used instead of a spinlock because copy\_to\_user can sleep while the kernel brings a user page into memory, and a process must not sleep while holding a spinlock.

**Caption for Figure 1:** "System architecture: calls travel through the VFS and the file\_operations table to the handlers; data crosses the user/kernel boundary only through the copy functions."

## Section 4: Implementation

This section collects three midterm sections (3, 4, and 9) plus midterm section 8, and adds stage 4. Use this order of subsections: 4.1 Technologies, 4.2 Midterm implementation, 4.3 Final project update: stage 4, 4.4 CS 370 concepts, 4.5 Technical evidence.

### 4.1 Technologies used (add)

> C (the kernel's GNU C dialect for the module, C99 for the test program); GCC and GNU Make with the kernel build system (kbuild); Ubuntu 26.04.1 LTS on a 2015 Intel MacBook (host) and a QEMU/KVM Ubuntu 26.04.1 guest (kernel 7.0.0-38-generic) for loading modules; insmod, rmmod, lsmod, modinfo, and dmesg; readelf, nm, and objdump for object-file analysis; strace and lldb for the test program; Git and GitHub for version control.

### 4.2 Midterm implementation (keep)

**Keep:** all four bullets from midterm section 3 (Environment, Stage 1, Stage 3, Repository), under the subheading "4.2 Midterm implementation".

**Edit** the last sentence of the Stage 3 bullet:

- Old: "For now read returns 0 (end of file) and write reports success but discards the data"
- New: "At the midterm, read returned 0 (end of file) and write reported success but discarded the data; stage 4 (section 4.3) replaced both."

### 4.3 Final project update: stage 4 (add)

> **Kernel buffer and data path.** The module now keeps a 1024-byte buffer (kbuf) and a length (kbuf\_len). The write handler checks the file position against the buffer size, shortens a write that does not fit, and returns -ENOSPC once the buffer is full. It copies the user's data into a temporary kernel buffer with memdup\_user, which uses copy\_from\_user and returns -EFAULT on a bad pointer. Only after a successful copy does it take the lock, copy the data into kbuf, and update kbuf\_len. The read handler takes the lock, copies from kbuf to the user's buffer with copy\_to\_user, advances the file position, and returns the number of bytes copied; once the position reaches kbuf\_len, it returns 0 (end of file).
>
> **Mutex.** A mutex (buf\_lock) protects kbuf and kbuf\_len. The handlers take it with mutex\_lock\_interruptible, so a process waiting for the lock can still be stopped with Ctrl-C (the handler then returns -ERESTARTSYS).
>
> **Module parameters.** Two read-only parameters support testing: use\_lock=0 turns the mutex off to show what it protects against, and verbose=0 silences the per-call log lines during the stress test.
>
> **Test program.** test\_driver.c has five modes. rw writes a string, reopens the device, reads it back, and compares; it also prints the user-space addresses of its buffers. empty reads right after loading and expects 0. big writes 2000 bytes and expects a 1024-byte short write, then ENOSPC. fault passes the address 1 to write and expects EFAULT. race forks two writers and two readers that hit the buffer thousands of times and counts "torn" reads (a buffer holding a mix of two writers' data). Every mode prints PASS or FAIL and exits with a matching status.
>
> **Automation.** scripts/collect\_stage4.sh builds both programs, runs every test, runs the race test three times with the lock on and three times with it off, captures strace and dmesg, and saves redacted output to docs/evidence/stage4/. scripts/collect\_lldb.sh records registers and stack frames in lldb.

### 4.4 CS 370 concepts (keep + edit + add)

**Keep:** the midterm concepts table.

**Edit:**

- Add a fourth column, **Final evidence**, so the midterm evidence stays visible and the new evidence sits beside it.
- Registers and stack row, Final evidence: "lldb: open's path in rdi and flags in rsi, write's fd/buffer/count in rdi/rsi/rdx, read's return value in rax; stack frame of cmd\_rw (rsp, rbp, rip) \[after VM run\]".
- User vs. kernel space row, Final evidence: "rw prints user buffer addresses (0x7ff…) and /proc/kallsyms shows kbuf at a kernel address (0xffff…); fault test returns EFAULT \[after VM run\]".
- System calls row, Final evidence: "strace shows openat, write, read, and close on /dev/mychardev with their return values \[after VM run\]".
- Delete the note "THE LLDB AND STRACE ROWS ARE PLANNED AND NOT YET COLLECTED", or change it to "lldb and strace evidence was collected after the midterm (Final evidence column)."

**Add** two rows:

| Concept | Where it appears | Final evidence |
| --- | --- | --- |
| Concurrency and shared memory | All processes share kbuf; buf\_lock lets one at a time inside | race: 0 torn reads with the lock, \[N\] without \[after VM run\] |
| Memory and bounds checking | Fixed 1024-byte buffer; short write, then ENOSPC | big: write returns 1024, then errno 28 \[after VM run\] |

**Keep:** the midterm section 8 list (concepts I planned to apply next), under the heading "Concepts planned at midterm", and add a status after each bullet:

- Pointers and memory layout → "Done: user vs. kernel addresses compared (rw output and kallsyms)."
- The stack and function calls → "Done: stack frame of cmd\_rw in lldb."
- Linking and symbols → "Done at the midterm and extended: nm shows memdup\_user, mutex\_lock\_interruptible, and \_copy\_to\_user as undefined (U) until load time. \[Verify the names in stage4\_nm.txt\]"
- Processes → "Done: dmesg names the pid and command; race forks four processes."
- Memory hierarchy and caching → "Not applied: out of scope for this driver."

### 4.5 Technical evidence (keep + add)

**Keep:** all of midterm section 9 (the tool table, the objdump excerpt, and the closing sentence).

**Add** under "Final project update: stage 4 evidence": short excerpts from these files, each in the same code-block style as the midterm objdump excerpt, with one sentence each on what it shows:

- stage4\_strace\_rw.txt: the openat, write, and read lines on /dev/mychardev
- stage4\_lldb\_rw.txt: the write breakpoint's register read (rdi = 3, rsi = buffer address, rdx = length)
- stage4\_kallsyms.txt: the kbuf kernel address beside the user buffer address
- stage4\_objdump.txt: the call to mutex\_lock\_interruptible in mychardev\_read, followed by test %eax,%eax and a jump, which is the `if (buf_lock_acquire())` check

## Section 5: Project Output (new)

This section is mostly screenshots, each with a caption that explains what it shows. Crop every screenshot to the lines that matter, and make sure no `user@hostname` prompt or IP address is visible.

**Keep:** the midterm dmesg screenshot (old Figure 1, now **Figure 2**) under the subheading "5.1 Midterm output (stage 3)". Replace its caption "dmsg output" with:

> Figure 2. Stage 3 at the midterm: the module allocates major 239, minor 0 and creates /dev/mychardev. tee (pid 1581) and cat (pid 1590) trigger the open, write, read, and release handlers. Read returned 0 bytes at this stage.

**Add** under "5.2 Final project update: stage 4 output": take these screenshots in the VM \[after VM run\] and paste each with its caption.

| Figure | Screenshot | Caption to paste |
| --- | --- | --- |
| 3 | `echo hello \| sudo tee /dev/mychardev` then `sudo cat /dev/mychardev` | Figure 3. The shell round trip: cat now prints the data tee wrote, because the driver stores it in the kernel buffer. |
| 4 | `sudo ./test_driver rw` | Figure 4. The test program writes a string, reads it back, and confirms they match. The printed buffer addresses are user-space addresses (0x7ff…). |
| 5 | `sudo ./test_driver big` and `fault` | Figure 5. Edge cases: a 2000-byte write is cut to 1024 bytes, then the next write fails with ENOSPC; a bad pointer returns EFAULT and the kernel keeps running. |
| 6 | `race` with `use_lock=1`, then `use_lock=0` | Figure 6. With the mutex, 0 of \[N\] reads are torn. With the mutex disabled, \[N\] reads see a half-written buffer. |
| 7 | `sudo dmesg -t \| grep mychardev` | Figure 7. The kernel log names each process and shows the bytes stored, the ENOSPC, and the EFAULT from the kernel's side. |
| 8 | lldb register read at the write breakpoint | Figure 8. lldb stopped at write: rdi holds the file descriptor (3), rsi the buffer address, and rdx the byte count, as the x86-64 calling convention specifies. |

## Section 6: Testing and Results

Rename "5. Testing performed and results" to **6. Testing and Results**.

**Keep:** the midterm intro sentence and the midterm test table under "6.1 Midterm testing (stage 3)". Move the dmesg figure out (it's now Figure 2 in section 5).

**Edit:** in the midterm table, change the last row "Edge cases (empty or oversized buffer): Not yet: needs the stage 4 buffer" to "Not yet at midterm; see 6.2". Keep it orange, so the grader can see what was still open.

**Add** under "6.2 Final project update: testing approach":

> Kernel code can't be unit tested like a normal program, so every test loads the real module in the virtual machine and checks the result from user space. The test program checks each behavior itself and prints PASS or FAIL. One script (collect\_stage4.sh) builds everything, runs every test in a fixed order, repeats the race test three times with the lock on and three times with it off, and saves all output as evidence, so the whole run can be repeated with one command. Test environment: \[kernel version\], \[N\] vCPUs. \[from stage4\_env.txt\]

**Add** under "6.3 Final project update: test cases and results", as a Pages table, coloring Pass green the same way as the midterm table:

| ID | Test | Expected | Result |
| --- | --- | --- | --- |
| T1 | Build stage 4 on kernel 7.0 | mychardev.ko built, environment warnings only | \[ \] |
| T2 | Read right after loading (`empty`) | read returns 0 | \[ \] |
| T3 | Write, then read back (`rw`) | Same bytes returned | \[ \] |
| T4 | Shell round trip (tee, cat) | cat prints `hello` | \[ \] |
| T5 | 2000-byte write (`big`) | 1024 stored, then ENOSPC (errno 28) | \[ \] |
| T6 | Bad pointer (`fault`) | EFAULT (errno 14), no crash | \[ \] |
| T7 | Failed write keeps stored data | `keep-me` read back after `fault` | \[ \] |
| T8 | Race, lock on (3 runs × 2000 iterations) | 0 torn reads | \[ \] |
| T9 | Race, lock off (3 runs) | Torn reads appear (expected FAIL) | \[ \] |
| T10 | Open after unloading | "No such file or directory" | \[ \] |
| T11 | strace of `rw` | openat, write, read, close on the device | \[ \] |

**Add** one closing sentence after the table: "\[N\] of \[N\] tests passed. T9 is expected to fail: it shows the race condition that the mutex prevents."

## Section 7: Challenges and Solutions

Rename "6. Challenges encountered and how I addressed them" to **7. Challenges and Solutions**.

**Keep:** all seven midterm bullets, under "7.1 Midterm challenges".

**Edit:** delete the empty bullet at the end of the midterm list (the lone "•" just above section 7).

**Add** under "7.2 Final project update: new challenges", in the same bold-title bullet style:

> - **A failed write erased stored data:** when copy\_from\_user can't copy every byte, it fills the rest of the destination with zeros. My write copied straight into the shared buffer, so the fault test (a write from address 1) silently replaced the first 16 stored bytes with zeros. I changed write to copy into a temporary kernel buffer first (memdup\_user) and touch kbuf only after a successful copy, and added a test that writes data, runs the fault test, and checks the data is still there.
> - **The test program reported false failures:** the big and fault tests checked errno after calling printf and close, and either call can change errno. A correct ENOSPC or EFAULT could show up as a failure. I now save errno right after the write that failed.
> - **Showing that the mutex matters:** a race condition only shows up under the right timing, so a test that passes proves little on its own. I added a use\_lock parameter to turn the mutex off, and a race test where two writers fill the buffer with different letters while two readers count reads that contain a mix. The same test then passes with the lock and fails without it. It needs at least two CPUs, so the VM has two.
> - **Hidden failures in the race test:** if a child process failed to open the device, its reads simply went missing from the count, which looked like success. The parent now checks every child's exit status and fails the test if any child failed.
> - **Logging slowed the stress test:** thousands of log lines per second flooded dmesg and changed the timing. I added a verbose parameter to turn per-call logging off during the race test.
> - **Choosing the right lock:** a spinlock is faster, but read calls copy\_to\_user while holding the lock, and copy\_to\_user can sleep on a page fault. Sleeping while holding a spinlock can deadlock the CPU, so I used a mutex, taken with mutex\_lock\_interruptible so a waiting process can still be stopped.
> - **Debugger breakpoints hit the wrong call:** a breakpoint on write also stopped on printf's writes to the terminal. I made the breakpoint conditional on the device's file descriptor (rdi == 3).
> - &#91;Add anything that went wrong during the VM run, with what you did about it.\]

## Section 8: Conclusion and Future Scope

Rename "7. What remains to be developed" to **8. Conclusion and Future Scope**.

**Keep:** the midterm bullet list under "8.1 Planned at midterm", and add a status after each bullet:

- The core of the driver, the data path → "Done (section 4.3)."
- Fixed-size buffer with copy\_from\_user and copy\_to\_user → "Done."
- Extend the test program → "Done: five modes, PASS/FAIL output."
- Edge cases: empty, oversized, module not loaded → "Done: T2, T5, T10, plus the bad-pointer case (T6)."
- strace and lldb evidence → "Done (section 4.5)."
- Stretch goals → "Mutex done (moved into core scope); ioctl and open counter not built (see 8.3)."

**Add** under "8.2 Final project update: conclusion":

> The project delivered everything in the proposal and the midterm scope: a loadable kernel module that registers /dev/mychardev, stores data in a 1024-byte kernel buffer, moves it safely across the user/kernel boundary, handles the empty, oversized, bad-pointer, and unloaded cases without crashing, and protects the shared buffer with a mutex. A test program and one script verify each behavior, and the compiled module and test program are analyzed with readelf, nm, objdump, strace, and lldb. The main lesson is how strict the boundary between user and kernel space is: most of a driver's code checks what user programs hand it before acting on it.

**Add** under "8.3 Future scope":

> - An ioctl command to clear the buffer or report its size, the standard way drivers accept commands that aren't plain reads or writes.
> - An open counter that reports how many times the device has been opened.
> - A separate buffer for each open file (stored in file->private\_data) instead of one shared buffer.
> - A blocking read that waits until data is written, using a wait queue.
> - Kernel debugging with GDB attached to the QEMU guest, to step through the handlers themselves.
> - Applying this to a real device: a driver for a USB accessory that has no Linux support, which was the original motivation.

## Section 9: References (new)

The midterm had no references section. Paste this list, then remove anything you didn't actually use and add course materials (lecture slides, textbook) in the format your instructor prefers. These links are from memory, so open each one before submitting.

**Add:**

> 1. P. J. Salzman, M. Burian, O. Pomerantz, B. Mottram, and J. Huang, *The Linux Kernel Module Programming Guide*. https://sysprog21.github.io/lkmpg/
> 2. The Linux Kernel documentation, "Building External Modules." https://docs.kernel.org/kbuild/modules.html
> 3. The Linux Kernel documentation, "Unreliable Guide To Hacking The Linux Kernel" (copy\_to\_user, copy\_from\_user, current). https://docs.kernel.org/kernel-hacking/hacking.html
> 4. The Linux Kernel documentation, "Generic Mutex Subsystem." https://docs.kernel.org/locking/mutex-design.html
> 5. J. Corbet, A. Rubini, and G. Kroah-Hartman, *Linux Device Drivers*, 3rd ed. O'Reilly, 2005 (chapter 3, "Char Drivers"; chapter 5, "Concurrency and Race Conditions").
> 6. Linux kernel source, cross-referenced by Bootlin Elixir (cdev, class\_create, memdup\_user). https://elixir.bootlin.com/linux/latest/source
> 7. Linux man pages: open(2), read(2), write(2), errno(3), strace(1). https://man7.org/linux/man-pages/
> 8. The LLDB Debugger documentation. https://lldb.llvm.org/
> 9. libvirt and QEMU documentation (virt-install, virsh). https://libvirt.org/docs.html
> 10. CS 370 course lectures: registers, assembly, number systems, compilation and object files, processes. \[Add course and textbook details.\]

## Before you export

- [ ] Every \[bracketed\] placeholder is replaced with real results from the VM run.
- [ ] Section headings read exactly: 1 Project Introduction, 2 Requirements, 3 System Design, 4 Implementation, 5 Project Output, 6 Testing and Results, 7 Challenges and Solutions, 8 Conclusion and Future Scope, 9 References.
- [ ] Every midterm paragraph, table, and figure is still somewhere in the report (use the map at the top to check).
- [ ] Each section with new work has a "Final project update" subheading.
- [ ] Figures are numbered 1–8 in order, and every figure is mentioned in the text.
- [ ] No screenshot shows a username, hostname, or IP address.
- [ ] The instructor's name is spelled the same everywhere (the proposal has "Pavalli", the midterm report "Pallavi").
- [ ] AI assistance is acknowledged as the course policy requires.
- [ ] Export: File → Export To → PDF, Image Quality: Best. Save as `docs/reports/final_report.pdf` (the packaging script leaves it out of the ZIP; commit it to GitHub next to midterm\_report.pdf if you like), then upload it alongside the ZIP.
