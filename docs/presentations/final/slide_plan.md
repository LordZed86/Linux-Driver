# CS 370 Final Presentation: Slide Plan

Nov 17, 2026 · @kellen is fine

## How to use this plan

The deck has exactly 8 content slides plus a title slide and a thank-you slide, and runs about 14 minutes with the live demo inside slide 6. Each slide below lists what goes **on the slide**, the **visual**, what to **say** (paste it into Keynote's presenter notes), and a **time** budget.

| Part | Time (min) | Running total |
| --- | --- | --- |
| Title | 0:15 | 0:15 |
| 1 Introduction | 1:30 | 1:45 |
| 2 Midterm progress | 1:15 | 3:00 |
| 3 Final overview | 1:00 | 4:00 |
| 4 Architecture | 1:30 | 5:30 |
| 5 How it works | 1:30 | 7:00 |
| 6 Output + live demo | 4:00 | 11:00 |
| 7 Testing and challenges | 1:30 | 12:30 |
| 8 Conclusion | 1:00 | 13:30 |
| Thank you | 0:15 | 13:45 |

Rules from the guidelines:

- Exactly 8 content slides. The demo is not its own slide; it happens during slide 6, so the count stays at 8.
- Slides 1–2 review the midterm; slides 3–8 cover the final project.
- The project must be shown running. Screenshots alone don't count as the demo.
- Don't upload the Keynote file. It isn't one of the two submitted files.

Keynote design notes:

- One idea per slide, at most 5 bullets, and no full sentences on the slide. The sentences go in presenter notes.
- Use one accent color for kernel space and a neutral color for user space, and keep them the same on every slide.
- Put commands and code in a monospace font (Menlo or SF Mono) at 20 pt or larger.
- Crop screenshots to the lines that matter. A full terminal window is unreadable over Zoom.
- Items marked \[after VM run\] can't be filled in until `collect_stage4.sh` has run.

## Title slide (not counted)

**On the slide:**

- Linux Driver Development Exploration: A Virtual Character Device Driver
- Kellen Jones · CS 370 Systems and Architecture · Pallavi Deogade
- Final project presentation, \[date\]

**Visual:** a cropped terminal line, `crw------- 1 root root 239, 0 /dev/mychardev`, as a quiet background strip.

**Say:** "Good morning. My project is a Linux kernel driver for a virtual character device, plus a user program that talks to it. I'll review where I was at the midterm, walk through what I built since, and then show it running."

**Time:** 0:15

## Slide 1: Introduction, problem, objectives, solution

**On the slide:**

- Problem: Linux often lacks manufacturer drivers, and I didn't know how drivers work
- Objective: a small driver I can fully build, test, and explain
- Solution: a loadable kernel module that creates `/dev/mychardev`, plus a C test program
- Scope: virtual device only, with no real hardware, interrupts, or USB/PCI

**Visual:** two boxes, "user program" and "kernel", with `/dev/mychardev` as the door between them. This becomes the full diagram on slide 4.

**Say:** "Linux distributions often don't get drivers from the manufacturer, so when an accessory doesn't work, someone has to write one. I wanted to learn how that's done. A driver sits right at the boundary between user programs and the kernel, which is where most of this course lives: system calls, memory, processes, compilation. Since I'm new to kernel work, I kept it to a virtual device. That means no hardware, but all the same machinery. My goal was a driver small enough that I can explain every line."

**Time:** 1:30

## Slide 2: Midterm progress and planned final scope

**On the slide:** two columns.

| Done at midterm | Planned for the final |
| --- | --- |
| Ubuntu host + QEMU/KVM test VM | Kernel buffer with `copy_to_user` / `copy_from_user` |
| Stage 1: hello-world module, analyzed with `nm`, `readelf`, `objdump` | Test program: write, read back, edge cases |
| Stage 3: `/dev/mychardev` registered, handlers log to `dmesg` | Mutex around the buffer (from midterm feedback) |
| Read returned 0, write discarded data | `strace` and lldb evidence |

**Visual:** the midterm dmesg screenshot (`Stage3_dmesg.png`), cropped, showing major 239 and the tee/cat pid lines.

**Say:** "At the midterm I had the environment set up, a hello-world module analyzed at the object-file level, and a registered character device. Every handler fired and logged the process that called it, but the driver didn't actually store anything: read returned end of file and write threw the data away. The plan for the final was the real data path between user and kernel space, a test program, and edge cases. The feedback I got was to protect the buffer with a mutex, so that moved from a stretch goal into the core scope."

**Time:** 1:15

## Slide 3: Final project overview and implemented features

**On the slide:**

- 1024-byte kernel buffer shared by every process that opens the device
- Safe copying across the boundary with `copy_from_user` / `copy_to_user`
- Mutex so only one process is inside the buffer at a time
- Edge cases: empty read → EOF, oversized write → short write then `ENOSPC`, bad pointer → `EFAULT`
- Test program with 5 modes (`rw`, `empty`, `big`, `fault`, `race`) and 2 module parameters (`use_lock`, `verbose`)

**Visual:** a checklist with every item from the slide 2 "Planned" column ticked.

**Say:** "Everything I planned at the midterm is in. The driver now stores what you write and gives it back when you read. It handles the edge cases from my proposal without crashing the kernel, and the buffer is protected by a mutex. I also wrote a test program that checks each behavior and prints pass or fail. I added two load-time switches: one turns the mutex off so I can show what it protects against, and one turns off logging for the stress test."

**Time:** 1:00

## Slide 4: System design, architecture, and implementation

**On the slide:** the architecture diagram at the end of this section, rebuilt in Keynote, plus three labels:

- `mychardev.c`: init/exit, 4 handlers, `file_operations` table, `kbuf[1024]` + `kbuf_len` + `buf_lock`
- `test_driver.c`: C99 user program, one function per test mode
- Built with kbuild (`make`), loaded only in the QEMU/KVM VM, kernel 7.0

**Say:** "Here's the whole system. A user program calls open, read, write, or close. That's a system call, so the CPU switches into kernel mode, and the kernel's virtual file system looks up `/dev/mychardev`. The device number, major 239, points at my driver, and my `file_operations` table maps each call to one of my handler functions. The handlers share one piece of memory: a 1024-byte buffer, a length, and a mutex guarding both. Memory design: the buffer lives in kernel space and the program's buffers live in user space. Those are separate address spaces, and the only way across is the copy functions. Init sets all of this up in order, and exit tears it down in reverse."

**Time:** 1:30

&#91;embedded content: slide 4 architecture · user space, system call boundary, kernel space\]

A call goes down the left column into the handlers. The data itself goes straight down the right side, through `copy_from_user` or `copy_to_user`, and every handler takes the mutex before touching `kbuf`.

## Slide 5: How the major functions work

**On the slide:** the write path as 5 numbered steps, with the read path in one line underneath.

1. `write(fd, buf, n)` enters the kernel and calls `mychardev_write`
2. Bounds check: past 1024 bytes → `-ENOSPC`, and a write that's too big is shortened
3. `copy_from_user` checks the user pointer: bad pointer → `-EFAULT`
4. Take the mutex, copy into `kbuf`, set `kbuf_len`, release the mutex
5. Return the number of bytes stored

Read path: lock → `copy_to_user` from `kbuf` → unlock → return the bytes, then 0 (end of file).

**Visual:** a short code excerpt of `mychardev_write` (about 12 lines) with the bounds check, the copy, and the lock highlighted in the accent color.

**Say:** "The interesting work happens in write. The kernel never trusts the pointer it's given. It first checks the size against the buffer, so a 2000-byte write gets cut down to what fits, and once it's full you get 'no space left on device'. Then `copy_from_user` checks that the address really belongs to the calling process. If it doesn't, I return 'bad address' instead of crashing the machine. Only then do I take the mutex and update the shared buffer. Read is the mirror image. I use a mutex instead of a spinlock because the copy functions can sleep while the kernel pages in user memory, and you can't sleep while holding a spinlock."

**Time:** 1:30

*Confirm step 3 and 4 wording after the write-path fix is tested in the VM.*

## Live demo (during slide 6)

The demo runs while slide 6 is up, so the slide count stays at 8. Before presenting, have the VM terminal open in a second window, with `export PS1='$ '`, a large font, and `sudo -v` already run.

**Transition line:** "Let me show you it running. I'll switch to the VM."

Run `docs/presentations/final/demo.sh` and use `demo_narration.md` for what to say. If time is short, run only these steps (about 3:30):

| Step | Shows | Keep or cut |
| --- | --- | --- |
| 2 Load + `ls -l /dev/mychardev` | module loads, device node, octal 600 | keep |
| 5 `test_driver rw` | round trip, user addresses | keep |
| 6 `test_driver big` | short write, then `ENOSPC` | keep |
| 7 `test_driver fault` | `EFAULT`, no crash | keep |
| 9–10 race, lock on vs. off | 0 torn reads vs. some torn reads | keep |
| 1, 3, 4, 8, 11 | build, empty read, shell, log, unload | cut first |

**Backup:** if the VM or Zoom misbehaves, play the screen recording. If the unlocked race shows 0 torn reads, open the saved `race_lock0` evidence files.

## Slide 6: Actual output screenshots and results

**On the slide:** a 2×2 grid of cropped screenshots \[after VM run\], each with a one-line caption:

- `test_driver rw`: "Data read back matches data written"
- `test_driver big` + `fault`: "Short write, then ENOSPC; bad pointer → EFAULT, kernel still running"
- `race` with lock on vs. off: "0 torn reads with the mutex, \[N\] without"
- `dmesg`: "Each call logged with the process that made it"

**Say (before the demo):** "These are the results from my test run. They're here in case anything goes wrong live, but let me show you the real thing." Then run the demo.

**Say (after the demo, back on the slide):** "So the data path works in both directions, every edge case returns the right error without taking the kernel down, and the mutex is what stops readers from seeing half-written data."

**Time:** 4:00 including the demo

## Slide 7: Testing, challenges, and solutions

**On the slide:** left, the testing approach in 3 lines; right, a challenge → solution table.

- Each stage tested in the VM before moving on
- `collect_stage4.sh` runs every test, the race test 3× with the lock on and off, and saves `dmesg`/`strace`
- Result: \[N of N\] tests pass \[after VM run\]

| Challenge | Solution |
| --- | --- |
| A kernel bug can crash the machine | Load only in a QEMU/KVM VM; snapshot and `sync` first |
| Tutorials target older kernels | Wrote for kernel 7.0 APIs (`class_create` takes one argument) |
| Test read `errno` after `printf`, so it reported false failures | Save `errno` immediately after the failing call |
| A failed write zeroed stored data (`copy_from_user` pads with zeros) | Copy into a temporary buffer first; touch `kbuf` only on success |
| Proving the mutex matters | `use_lock=0` switch + a torn-read counter |
| \[anything from the VM run\] | \[fix\] |

**Say:** "Kernel code can't be unit tested like a normal program, so every test loads the real module in a VM and checks the results from user space. I scripted the whole run so it's repeatable. The challenge I learned the most from was a subtle one. When a write failed on a bad pointer, `copy_from_user` filled the part it couldn't copy with zeros, straight into my shared buffer, so a failed write quietly destroyed data. I changed write to copy into a temporary buffer first and only touch the shared buffer once the copy succeeds."

**Time:** 1:30

## Slide 8: Conclusion and future scope

**On the slide:**

- Completed: the full scope from the proposal and midterm, plus the mutex
- Learned: system calls, user vs. kernel address spaces, relocatable object files, race conditions
- Future: an `ioctl` command to clear the buffer or report its size
- Future: an open counter, a separate buffer per open file, a blocking read that waits for data
- Future: kernel debugging with GDB attached to QEMU; then a driver for a real USB device

**Say:** "I finished what I proposed: a working driver, a test program, and evidence for each course concept, from the object file up to the system call. The biggest takeaway is how strict the boundary between user and kernel space is, and how much of a driver is just checking what user programs hand it. Next I'd add an ioctl command, which is how real drivers take commands that aren't plain reads and writes, and eventually write a driver for a real device, which is why I started this."

**Time:** 1:00

## Thank-you slide (not counted) and likely questions

**On the slide:** "Thank you. Questions?" and the GitHub repo link, if it's public by then.

Answers to have ready:

| Question | Short answer |
| --- | --- |
| Why a mutex and not a spinlock? | The copy functions can sleep on a page fault, and you can't sleep holding a spinlock. |
| Why not just use `memcpy` on the user pointer? | The kernel can't trust it. It may be invalid or point into kernel memory. `copy_*_user` checks it and handles the fault. |
| What happens if you `rmmod` while a program has the device open? | `.owner = THIS_MODULE` holds a reference, so `rmmod` refuses with "Module is in use". \[Verify in the VM\] |
| What are major and minor numbers? | Major picks the driver (239), minor picks the device within it (0); they're packed as `major << 20 \| minor`. |
| Why does the kernel say "tainted"? | The module is out-of-tree and unsigned. It's a debugging flag; Secure Boot would refuse to load it. |
| Why does read return 0 at the end? | 0 means end of file, which is how `cat` knows to stop. |
| Why does it need `sudo`? | The device node is created `crw-------` (octal 600), so only root can read or write it. |
