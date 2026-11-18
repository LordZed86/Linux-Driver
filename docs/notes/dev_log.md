# Dev Log

Chronological record of decisions, bugs, and fixes. Feeds the "challenges" section of the reports.

## Template

### YYYY-MM-DD - short title

- **What I did:**
- **What happened (output, errors):**
- **What I learned / how I fixed it:**
- **Next:**

---

## 2026-09-30 - Project proposal

- **What I did:** Submitted the proposal for a virtual character device driver.
- **Next:** Install Ubuntu, set up the VM and tools.

## 2026-10-05 - Scope approved and environment decisions

- **What I did:**
  - Got instructor approval for the project: a virtual character device driver, kept small, with extras only if time permits.
  - Finalized the proposal (all 11 sections) and converted it to Markdown for the docs folder.
  - Decided on a native x86 Ubuntu install on my 2015 Intel MacBook, since my main machine is Apple silicon and the class needs x86 output. Version: [24.04 / 26.04] currently running 26.04 but my old mac struggles at times so I might downgrade.
  - Planned a QEMU/KVM Ubuntu guest for testing new module builds, so a crash only affects the VM.
  - Chose lldb and GDB for the user-space test program; the kernel module will be analyzed statically (`readelf`, `nm`, `objdump`) and through `dmesg`.
  - Switched the editor from CLion to VS Code, because the module builds with a plain Makefile, not CMake.
  - Reworked the timeline to add research time to each stage. Midterm is Oct 21.
- **What happened (output, errors):**
  - Realized the original plan crammed too much into short windows, since drivers and kernel work are new to me. Spread the stages over two weeks each.
- **What I learned / how I fixed it:**
  - A "virtual" device still runs in the real kernel, so a bug can crash the machine. A VM, frequent Git commits, and running `sync` before `insmod` are my safety net.
  - Kernel modules are built with the kernel's own build system (kbuild), not CMake or a normal `gcc` command.
  - From this week's registers lecture: x86-64 syscall arguments go in `rax`, `rdi`, `rsi`, `rdx`, which I can observe in lldb for the test program.
  - Ubuntu 26.04 recommends 6 GB RAM and is Wayland-only; it also ships a much newer kernel, so some older tutorials do not match.
- **Next:** Set up the VM and tools, then build and load a hello-world module.

### 2026-10-08 - Repo skeleton and Git setup

- **What I did:**
  - Created the project layout: `module/`, `user/`, `docs/`, plus `.gitignore` and `README.md`.
  - Added a placeholder hello-world module (`mychardev.c`), a kbuild Makefile, and a stub test program (`test_driver.c`).
  - Expanded the README using my standard format from earlier projects.
- **What happened (output, errors):**
  - Git on Linux wasn't setup fully; asked for `user.name` and `user.email` before I could commit and push.
- **What I learned / how I fixed it:**
  - Fix: `git config --global user.name "..."` and `git config --global user.email "..."`. [Confirm this resolved it.]
  - GitHub no longer accepts a password for pushes, so I need a personal access token or an SSH key. But I got logged in and things are working fine for now.
- **Next:** First `make`, `insmod`, `dmesg`, `rmmod` cycle, with screenshots saved to `docs/screenshots/`.

### 2026-10-12 - Stage 1 evidence collected

- **What I did:**
  - Verified the toolchain on the Ubuntu 26.04.1 host (kernel 7.0.0-34-generic): gcc, make, gdb, lldb, strace, binutils, and kernel headers all present.
  - Cloned the repo onto the x86 host and built the hello-world module.
  - Saved `modinfo`, `nm`, `readelf`, `objdump`, `file`, and build output to `docs/evidence/` and committed them.
  - Redacted usernames, hostnames, and home paths from the saved output.
- **What happened (output, errors):**
  - Could not log in to the QEMU/KVM test VM: SSH kept returning "Permission denied" after the cloud-init password setup.
  - Hit `virsh` permission errors because the SSH session had not picked up the new `libvirt` group.
- **What I learned / how I fixed it:**
  - Group membership changes only apply to new logins.
  - Cloud-init reads its config only on first boot, so editing `user-data` later does not change an existing VM.
  - Building and analyzing a module is safe on the host; only loading it needs the VM.
- **Next:** Get the VM login working (likely by rebuilding with an SSH key), take the `insmod`/`dmesg`/`rmmod` screenshot, then start the midterm report.

### 2026-10-13 - Assembly lecture mapped to the driver; README finalized

- **What I did:**
  - Updated the README: filled in the module and device name (`mychardev`), Ubuntu 26.04.1 LTS and kernel 7.0.0-34-generic, and chose the GPL-2.0 license to match `MODULE_LICENSE("GPL")` in the source.
  - Corrected the status checklist so it matches reality: stage 1 and its analysis are done, stage 3 is written but not yet built or loaded.
  - Split "What it does" into what works now and what is planned for stage 4, since `read` returns 0 bytes and `write` discards its data for now.
  - Added the number systems and bitwise operations lecture to the concepts table (major/minor device numbers use shifts and masks, hex addresses in `nm`/`objdump`, octal device permissions).
  - Mapped today's assembly lecture to the project (symbolic instructions, registers, control flow, the stack) and planned the evidence to collect.
- **What happened (output, errors):**
  - The Ubuntu host was unavailable, so nothing could be built or run today. The VM login problem from earlier is still unresolved.
- **What I learned / how I fixed it:**
  - A conditional jump in the disassembly corresponds to an `if (ret < 0)` check; the `goto` cleanup ladder in `mychardev_init` should appear as compare-and-jump instructions leading to the teardown calls.
  - In a relocatable `.ko`, calls to kernel functions like `_printk` show up as relocations, because their final addresses are filled in when the module is loaded.
  - The x86-64 calling convention passes the first arguments in `rdi`, `rsi`, and `rdx`, so at a breakpoint on `open` in lldb I can read the device path and flags directly from registers.
  - Kernel functions have extras that user-space code lacks, such as tracing hooks at function entry, which I need to explain rather than skip in the report.
- **Next:**
  - On the host: build stage 3, then save `stage3_` evidence including `objdump -dr` of the handlers and the generated assembly (redacting paths).
  - Examine the test program in lldb: stack frame (`rsp`, `rbp`, `rip`) and the registers at the `open` call.
  - Write the stage 4 test program so all four handlers fire, fix the VM login (likely rebuild with an SSH key), then start the midterm report. Midterm is Oct 21.

### 2026-11-03 - Midterm feedback and stage 4 code

- **What I did:** Presented the midterm. The instructor suggested adding a mutex around the kernel buffer. Wrote the stage 4 module (1024-byte kernel buffer, `copy_to_user` / `copy_from_user`, mutex) and a new user-space test program with `rw`, `empty`, `big`, `fault`, and `race` modes. Updated the README, proposal, and code walkthrough.
- **What happened (output, errors):** Not built or tested yet. Code written but not compiled against the kernel headers.
- **What I learned / how I fixed it:**
  - A mutex (not a spinlock) fits here because `copy_to_user` and `copy_from_user` can sleep on a page fault.
  - `mutex_lock_interruptible()` lets a waiting process be interrupted by a signal; the handler returns `-ERESTARTSYS` in that case.
  - Write semantics: data is stored at the file position, the stored length becomes position plus bytes written, an oversized write is shortened, and a full buffer returns `-ENOSPC`. A bad pointer returns `-EFAULT`.
  - Added two read-only module parameters, `use_lock` and `verbose`, so the mutex can be turned off to compare, and logging can be silenced for the stress test.
- **Decisions:** Mutex moved from stretch goal to core scope. Keeping the name `mychardev` (rename dropped).
- **Next:** Build in the VM, run each test mode, run `race` with the lock on and off, save output to `docs/evidence/`, take screenshots, then fill in the Pending rows in the README testing table.

### 2026-11-12 - Test program fixes and evidence script

- **What I did:**
  - Reviewed the stage 4 module and test program before building them in the VM.
  - Fixed the `big` and `fault` tests, which checked `errno` after calling `printf` and `close`; either call can change `errno`, so a correct result could be reported as a failure. The value is now saved right after `write`.
  - The `race` test now checks each child's exit status, so a child that fails (for example, if `open` fails) is reported as a failure instead of silently lowering the read count. It also takes an optional iteration count (`race 10000`).
  - Every test mode now prints a PASS or FAIL line.
  - Wrote `scripts/collect_stage4.sh`, which builds the module and test program, runs each test, runs `race` with `use_lock=1` and `use_lock=0`, captures `strace` and `dmesg`, and saves everything (redacted) to `docs/evidence/stage4/`.
- **What happened (output, errors):** Not run yet; the test program compiles cleanly with `-std=c99 -Wall -Wextra -pedantic`.
- **What I learned / how I fixed it:** `errno` is only meaningful immediately after the call that failed; any later library call may overwrite it.
- **Next:** Run the script in the VM (with at least 2 vCPUs, so the unlocked race can show torn reads), review the evidence, fill in the README testing table, then start the final report. Final is due Dec 3.

### 2026-11-17 - Final guidelines and a write-path bug

- **What I did:**
  - Read the final project guidelines: the submission is one ZIP plus one PDF (the updated midterm report with nine sections), due Dec 1. The talk is 12–15 minutes with exactly 8 content slides and a live demo.
  - Rewrote the final demo script and narration for stage 4. Added `scripts/collect_lldb.sh` (registers, stack frame, user vs. kernel addresses) and `scripts/package_submission.sh` (builds the ZIP from the last commit).
  - Fixed a bug in `mychardev_write` (version 0.4).
- **What happened (output, errors):** Found on review, not yet seen in the VM. When `copy_from_user` fails, it zero-fills the bytes it could not copy. Because the write copied straight into `kbuf`, a failed write (the `fault` test passes address 1) overwrote the first 16 stored bytes with zeros while `kbuf_len` stayed the same. A later read would return the zeros.
- **What I learned / how I fixed it:**
  - `write` now uses `memdup_user`, which allocates a temporary kernel buffer, copies into it, and returns `-EFAULT` on a bad pointer. Only a successful copy is moved into `kbuf`, under the lock. A failed write leaves the stored data unchanged.
  - Side effect: `copy_from_user` now runs before the lock is taken, so the lock is held for a plain `memcpy`. The mutex is still needed because `read` calls `copy_to_user`, which can sleep, while holding it.
- **Next:** Build and run `collect_stage4.sh` in the VM. Add a check that a failed write doesn't change the stored data (`rw`, then `fault`, then read back).

### 2026-11-17 - Stage 4 tested in the VM

- **What I did:**
  - Cloned the repo into the VM (`~/driver-project`) and ran `collect_stage4.sh` and `collect_lldb.sh` on kernel 7.0.0-38-generic with 2 vCPUs.
  - All functional tests passed: `empty`, `rw`, `big` (1024, then ENOSPC), `fault` (EFAULT), the new fault-keeps-data check, and opening the device after unload.
  - Race test, lock on: 0 torn reads in 1,200,000 reads (3 runs). Lock off: 7,647, 1, and 3 torn reads.
  - Collected lldb evidence (argument registers at `open`/`write`/`read`, `rax` return value, `cmd_rw` stack frame) and kernel addresses from `/proc/kallsyms` (`kbuf` at `0xffffffffc09a7060`).
- **What happened (output, errors):**
  - The first two race runs showed 0 torn reads even with the lock off (12,000, then 120,000 reads). The lock really was off (`/sys/module/mychardev/parameters/use_lock` = N), and a manual 500,000-iteration run showed 1,458 torn reads, so the race was real but rare.
  - The script's redaction replaced the user name `dev` everywhere, which would have turned `/dev/mychardev` into `/<host-user>/mychardev` in the evidence.
  - The strace filter kept a C library startup `read(3, "\177ELF…")` line and missed `close(3)` lines.
- **What I learned / how I fixed it:**
  - A race only shows up when the processes actually run at the same time. The original test opened and closed the device on every iteration, so most time went to open/close rather than the 1 KB copy. The children were also started one after another, so early ones finished before late ones began. Fixes: each process opens once and loops on `pread`/`pwrite`, all four wait on a pipe "starting gate" until the parent releases them together, and the default is 200,000 iterations.
  - The number of torn reads varies by thousands between runs because it depends on how the two vCPUs schedule the four processes. That's exactly why the lock is needed: a test that passes once proves nothing about a race.
  - Redaction now only replaces `user@`; the strace filter starts at the first open of `/dev/mychardev`.
  - lldb disables address randomization, so its addresses (`0x5555…`, `0x7fff…`) are fixed; outside the debugger they change every run. Command-line arguments live at the top of the stack, string constants in the program's read-only data.
- **Next:** Screenshots for the report and slide 6, then fill in the final report and build the slides.

### 2026-11-17 - Second-half course topics: caching and linking evidence

- **What I did:**
  - Mapped the second half of the course (weeks 5–8: memory and pointers, caching, OS and processes, linking and loading) to the project and added rows to the README concepts table.
  - Week 6: the race test now records where each torn read changes letter (offset mod 64) and where each reader's buffer sits in its cache line; the script saves `kbuf`'s address with each race run.
  - Week 8: new `scripts/collect_linking.sh` compares the module (REL), the dynamic test program (DYN/PIE), and a static build (EXEC): `file`, `readelf -h`, `ldd`, file sizes, the loader in `strace`, and the module's relocations.
  - Kept the earlier race runs in `docs/evidence/stage4/race_history/`, since each run overwrites the race files.
- **What happened (output, errors):**
  - The first VM pull failed: the evidence files were now tracked in Git, and the VM had untracked copies of them. The script then ran the old code. Fixed by deleting the VM's copies before pulling; from now on, `git checkout -- docs/evidence && git pull` in the VM.
  - At `-O2`, gcc warned that `fault` writes from address 1 (intentional). A `volatile` variable now hides the address from the optimizer.
  - Unlocked race across three sessions: 7,647, 1, 3 / 3, 0, 1,223 / 1, 0, 3 torn reads. Locked: 0 in all 9 runs.
  - Tear offsets: every tear from the small runs split at offset 32 mod 64, exactly `kbuf`'s cache-line boundaries (`kbuf` is at `…7060`, 32 bytes into a line). The 1,223-tear run split at offset 8 instead; reader buffers are 16-byte aligned (they started at byte 0, 32, or 48 of a line), so 8 isn't a reader cache-line boundary either. Unexplained.
  - Static build: 885,504 bytes vs. 21,448 dynamic. `strace` shows `ld.so` opening `libc.so.6` and reading its `\177ELF` header (the line that showed up in the strace evidence earlier); the static build opens nothing.
- **What I learned / how I fixed it:**
  - Tears don't happen at random byte positions: the copies move data in fixed-size chunks, and most splits line up with the destination buffer's cache lines.
  - `kbuf_len` (`…7040`) and `kbuf` (`…7060`) share one 64-byte cache line.
  - The first version of the linking script's filters missed the interpreter line and the static build's symbols, and cut the relocation list off after 20 lines, before the mutex entries; fixed. The module has 8 relocated calls in `.rela.text`: `memdup_user`, `_copy_to_user`, `mutex_lock_interruptible` twice (read and write), and `mutex_unlock` four times (each exit path that holds the lock).
  - In the static build, `open`, `fork`, `pread64`, and `pwrite64` are weak symbols (`W`): glibc's default definitions, which a strong definition of the same name would override.
- **Next:** Rerun `collect_linking.sh` for the fixed filters, then screenshots, the report, and the slides.
