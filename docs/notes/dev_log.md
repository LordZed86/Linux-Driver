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
  - Git on Linux wasn's setup fully; asked for `user.name` and `user.email` before I could commit and push.
- **What I learned / how I fixed it:**
  - Fix: `git config --global user.name "..."` and `git config --global user.email "..."`. [Confirm this resolved it.]
  - GitHub no longer accepts a password for pushes, so I need a personal access token or an SSH key. But i got logged in and things are working fine for now.
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
