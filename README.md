# Linux Driver Development Exploration

![Language](https://img.shields.io/badge/language-C99-blue)
![Linux kernel module](https://img.shields.io/badge/Linux-kernel%20module-orange?logo=linux&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Ubuntu%20x86--64-informational)
![Status](https://img.shields.io/badge/status-complete%20(stage%204)-brightgreen)
![License](https://img.shields.io/badge/license-GPL--2.0-blue)

A loadable Linux kernel module that registers a **virtual character device**, plus a user-space C program that talks to it through system calls. The project explores how user programs reach the kernel, how data crosses the user/kernel boundary safely, and what a compiled kernel module looks like at the object-file and assembly level.

Built for CS 370 — Computer Systems and Architecture.

---

## What it does

**Stage 3 (midterm), working and tested:**

- Builds as a loadable kernel module (`mychardev.ko`) against the running kernel's headers
- Registers a virtual character device (`/dev/mychardev`) when the module loads, and removes it on unload
- Implements `open`, `read`, `write`, and `close` (release) handlers that log each operation, including the calling process's pid and name, to the kernel log
- Tested by loading it in a QEMU/KVM virtual machine and driving it with `echo`, `tee`, and `cat`

**Stage 4 (final), working and tested:**

- Stores up to 1024 bytes in a fixed-size kernel buffer shared by every process that opens the device
- Moves data between user space and kernel space with `copy_from_user` and `copy_to_user`
- Protects the buffer with a mutex, so concurrent readers and writers cannot corrupt it or see half-written data
- Handles edge cases: an oversized write is shortened and then fails with `ENOSPC`, and a bad user pointer returns `EFAULT` instead of crashing the kernel; a failed write leaves the stored data unchanged
- User-space test program writes a string, reads it back, prints buffer addresses, and runs the edge-case and concurrency tests
- Two optional load parameters for testing: `use_lock=0` turns the mutex off, and `verbose=0` silences the per-operation log lines

**Analysis:** the module is examined with `readelf`, `nm`, `objdump`, `strace`, and lldb to connect the code to CS 370 concepts. Saved output is in `docs/evidence/`.

> **Scope:** Real hardware, interrupts, USB/PCI drivers, and device trees are out of scope. The goal is a small driver that I can fully explain.

---

## Status

- [x] Hello-world kernel module (stage 1)
- [x] Stage 1 analysis: build output, `modinfo`, `nm`, `readelf`, `objdump` (`docs/evidence/`)
- [x] Character device with `open`/`release`/`read`/`write` logging (stage 3), built on kernel 7.0 and loaded in the VM
- [x] Stage 3 analysis saved (`docs/evidence/stage3/`)
- [x] Kernel buffer with `copy_to_user` / `copy_from_user` (stage 4)
- [x] Mutex around the buffer (added after midterm feedback), shown to prevent torn reads
- [x] User-space test program that writes, reads back, and runs edge-case and race tests
- [x] Edge-case testing (empty buffer, oversized write, bad pointer, module not loaded)
- [x] `strace` and lldb evidence (`docs/evidence/stage4/`)
- [ ] Stretch goals: `ioctl` command, open counter

---

## Screenshots

![Kernel log after loading the module and exercising each handler](docs/screenshots/stage3/Stage3_dmesg.png)

*Stage 3 running: the module allocates major 239, minor 0 and creates `/dev/mychardev`. `tee` (pid 1581) and `cat` (pid 1590) trigger the open, write, read, and release handlers.*

---

## How it works

> **Current behavior (stage 4):** `write` copies data into a 1024-byte kernel buffer and `read` returns it, then 0 bytes (end of file). A mutex lets only one process use the buffer at a time. In stage 3 (the midterm build), `read` returned 0 and `write` discarded the data.

### Data flow

| User program     | Kernel                   | Driver module                                      |
| ---------------- | ------------------------ | -------------------------------------------------- |
| `open("/dev/X")` | syscall, then VFS lookup | open handler                                       |
| `write(fd, buf)` | syscall, then VFS        | write handler, `copy_from_user` into kernel buffer |
| `read(fd, buf)`  | syscall, then VFS        | read handler, `copy_to_user` into user buffer      |
| `close(fd)`      | syscall, then VFS        | release handler                                    |

*Stage 3 only logged these operations; stage 4 added the buffer and the `copy_*` calls.*

The kernel cannot trust a pointer handed to it by a user program, since user and kernel space are separate address spaces. `copy_from_user` and `copy_to_user` validate the user address and copy the data safely, which is the central idea of the driver.

### CS 370 concepts

| CS 370 concept                 | Where it shows up in this project                                                                                                    |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| System calls                   | `tee` and `cat` reach the driver through `open`, `read`, `write`, and `close`                                                        |
| Registers                      | In `objdump`, the arguments to `alloc_chrdev_region` are set up in `rdi`, `esi`, `edx`, and `rcx`                                    |
| Number systems and bitwise ops | `MAJOR()`/`MINOR()` compile to `shr $0x14` and `and $0xfffff`; `crw-------` is octal 600; hex addresses in `nm`/`objdump`            |
| Assembly and control flow      | `if (ret < 0)` checks compile to `test` plus a conditional jump (`jns`), with a cleanup path on failure                              |
| Compilation and object files   | `make` builds a relocatable ELF `.ko`; `readelf` shows type `REL`, and `.rela.*` sections hold the relocations the loader patches    |
| Processes and scheduling       | `dmesg` names the pid and command; `race` forks 4 processes and checks each exit status with `wait`; a process waiting on the mutex sleeps (blocked state); how the scheduler places 4 processes on 2 vCPUs makes torn reads vary from 0 to 7,647 per run |
| User vs. kernel space          | Separate address spaces; `copy_to_user` / `copy_from_user` check user pointers and return `EFAULT` on a bad one                      |
| Concurrency and shared memory  | Several processes share one kernel buffer; a mutex keeps one process inside it at a time, so a read never sees a half-written buffer |
| Memory                         | Fixed-size buffer with bounds checks: short writes, then `ENOSPC` when full                                                          |
| The stack                      | lldb shows `cmd_rw`'s frame (`rsp`, `rbp`) with the 1025-byte `rbuf` inside it, and argument registers at `open`/`write`/`read` |
| Memory hierarchy and caching   | Torn reads split at fixed offsets, not at random: with `kbuf` 32 bytes into a 64-byte cache line, most small runs split exactly at `kbuf`'s cache-line boundaries (offset 32 mod 64); one large run split at offset 8, which is unexplained (`race_history/README.md`) |
| Pointers, stack, and heap      | `rbuf` is a stack array; `memdup_user` allocates kernel heap memory, freed with `kfree`; `kbuf` is a static array in `.bss`; a bad pointer gets `EFAULT` instead of a crash |
| Virtual memory and paging      | User addresses (`0x7fff…`) and kernel addresses (`0xffffffffc0…`) are separate virtual ranges; `copy_to_user` can page-fault and sleep, so the lock is a mutex; ASLR moves user addresses every run |
| System calls and mode switches | Each `read`/`write` traps from user mode into kernel mode and back (`strace` shows the calls; `dmesg` shows the kernel side) |
| Linking and loading            | `mychardev.ko` is relocatable (`REL`) and the kernel links it at `insmod`; `test_driver` is dynamically linked (`DYN`) and `ld.so` loads libc at start; a static build needs neither: 21 KB dynamic vs. 865 KB static (`scripts/collect_linking.sh`) |

*Note: `mychardev_init` shows up as `init_module` in `objdump` output, because both names refer to the same function.*

---

## Planned features

### Stretch goals (only if the core is finished)

- **`ioctl` command** — clear the buffer or report its size
- **Open counter** — track how many times the device has been opened
- **Kernel debugging setup** — attach a debugger to a kernel running in QEMU

---

## Project structure

```plaintext
.
├── module/                          # Kernel module (build with make)
│   ├── mychardev.c                  #   Registers /dev/mychardev; buffer, mutex, file operations
│   └── Makefile                     #   Builds against the running kernel's headers (kbuild)
├── user/                            # User-space test program
│   └── test_driver.c                #   Modes: rw, empty, big, fault, race
├── scripts/                         # Run inside the VM, from the repo root
│   ├── collect_stage4.sh            #   Builds, runs every test, saves evidence
│   ├── collect_lldb.sh              #   lldb registers/stack and kernel vs. user addresses
│   ├── collect_linking.sh           #   ELF types, static vs. dynamic linking, relocations
│   └── package_submission.sh        #   Builds the submission ZIP from the last commit
├── docs/
│   ├── proposal.md                  # Original project proposal
│   ├── setup_guide.md               # Host and VM setup
│   ├── reports/
│   │   ├── midterm_report.pdf       #   Midterm report as submitted
│   │   └── final_report_paste_guide.md  # Plan for updating the midterm report into the final
│   ├── presentations/
│   │   ├── code_walkthrough.md      #   Line-by-line notes on the module and test program
│   │   ├── midterm/                 #   Midterm demo script, narration, talking points
│   │   └── final/                   #   Final demo script, narration, slide plan
│   ├── evidence/                    # Saved tool output (build, nm, readelf, objdump, dmesg, strace)
│   │   ├── stage1/
│   │   ├── stage3/
│   │   └── stage4/                  #   Written by the collect scripts
│   ├── screenshots/                 # Cropped terminal captures used in the reports
│   │   ├── stage3/
│   │   └── stage4/
│   └── notes/
│       ├── dev_log.md               #   Dated record of decisions, bugs, and fixes
│       └── finalization_checklist.md  # Remaining work and the final deliverables plan
├── LICENSE
├── .gitignore
└── README.md
```

Build output (`*.ko`, `*.o`, `Module.symvers`, `user/test_driver`) is gitignored, and so is a local `private/` folder that holds course files and unredacted screenshots.

---

## Running locally

> **Warning:** Kernel modules run inside the real kernel. A bug can freeze or crash the machine. Load modules only inside a QEMU/KVM virtual machine, commit work to Git, and run `sync` before loading a new build. This project cannot run on macOS or in cloud environments such as Codespaces; it needs a Linux system with kernel headers matching the running kernel. See [docs/setup_guide.md](docs/setup_guide.md) for how the VM was set up.

**Prerequisites:** Ubuntu 26.04 LTS on x86-64 (native install or VM)

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r) gdb lldb strace git
```

**1. Build the kernel module:**

```bash
cd module
make
```

This produces `mychardev.ko`. The build prints warnings about the compiler and `pahole` differing from the kernel's, and about skipping BTF generation; these come from the environment and are harmless.

**2. Load the module** (inside the VM):

```bash
sync
sudo insmod mychardev.ko
lsmod | grep mychardev
sudo dmesg | grep mychardev
ls -l /dev/mychardev
```

`insmod` prints nothing on success. `dmesg` needs `sudo` on Ubuntu. The log shows "loading out-of-tree module taints kernel" and "module verification failed" because the module is unsigned; this is expected for a self-built module. It then shows `allocated major N, minor 0` (N varies) and `module loaded, /dev/mychardev created`.

**3. Exercise the handlers:**

```bash
echo "hello" | sudo tee /dev/mychardev
sudo cat /dev/mychardev
sudo dmesg | grep mychardev
```

The log shows open, write (6 bytes stored), and release for `tee`, then open, read, and release for `cat`. `cat` prints `hello`, because stage 4 keeps the data in the kernel buffer and returns it on read. (In stage 3, `cat` printed nothing.)

**4. Build and run the test program:**

```bash
cd ../user
gcc -std=c99 -g -Wall -o test_driver test_driver.c
sudo ./test_driver empty   # run right after loading: read returns 0 (end of file)
sudo ./test_driver rw      # write a string, reopen, read it back
sudo ./test_driver big     # 2000 bytes into a 1024-byte buffer: short write, then ENOSPC
sudo ./test_driver fault   # bad pointer: the driver returns EFAULT
sudo ./test_driver race    # 2 writers and 2 readers; counts torn reads (optional: race 500000)
```

**Run everything at once:** from the repo root inside the VM, `./scripts/collect_stage4.sh` builds both programs, runs every test mode, runs `race` three times with the lock on and three times with it off, and saves all output (tests, `dmesg`, `strace`, `objdump`, `nm`, `readelf`) to `docs/evidence/stage4/`.

The device node is owned by root (`crw-------`), so the program needs `sudo`.

**Mutex demo:** load with `sudo insmod mychardev.ko verbose=0` and run `race`; there should be 0 torn reads. Then unload, reload with `verbose=0 use_lock=0`, and run it again. Torn reads should appear, because readers can catch the buffer halfway through a write. How many varies a lot from run to run (0 to 7,647 across 9 saved runs; see `docs/evidence/stage4/race_history/`), and it needs more than one CPU core; use a larger count such as `race 500000` to make it reliable.

**5. Unload the module:**

```bash
sudo rmmod mychardev
sudo dmesg | grep mychardev
```

The last line should read `module unloaded`.

**6. Clean up build files:**

```bash
cd module
make clean
```

---

## Analysis commands

Used to gather the technical evidence for the reports:

```bash
modinfo mychardev.ko
nm mychardev.ko
readelf -h -S mychardev.ko
objdump -d mychardev.ko
strace ./test_driver
lldb ./test_driver
```

---

## Testing

There is no automated test framework; testing is done by running the module and checking results manually. Results below are from the VM running Ubuntu 26.04.1 (kernel 7.0.0-38-generic). The first seven rows are stage 3 (midterm); the rest are stage 4, run with `scripts/collect_stage4.sh` on a 2-vCPU VM. Saved output is in `docs/evidence/stage4/`.

| Test                                          | Expected result                               | Result                                     |
| --------------------------------------------- | --------------------------------------------- | ------------------------------------------ |
| Build on kernel 7.0                           | `make` produces `mychardev.ko`                | Pass (environment warnings only)           |
| Load the module                               | Module loads, major number allocated          | Pass: major 239, minor 0                   |
| Device node appears                           | `/dev/mychardev` exists after `insmod`        | Pass: `crw------- root`, 239, 0            |
| Open and close the device                     | `open` and release messages appear in `dmesg` | Pass: lines name pid and command           |
| Write                                         | Write handler receives the bytes              | Pass: 6 bytes ("hello" plus newline)       |
| Read                                          | Read handler is called, returns 0 bytes       | Pass: `cat` requested 65536 bytes, got EOF |
| Unload the module                             | `module unloaded` is logged                   | Pass                                       |
| Write, then read back                         | Data read matches data written                | Pass: 21 bytes written and read back       |
| Read with nothing stored                      | Returns 0 (end of file)                       | Pass: `read` returned 0                    |
| Write larger than the buffer                  | Short write of 1024 bytes, then `ENOSPC`      | Pass: 1024, then errno 28                  |
| Bad user pointer                              | `EFAULT`, no kernel crash                     | Pass: errno 14, kernel kept running        |
| Failed write keeps stored data                | Data written before the bad write survives    | Pass: `keep-me` read back                  |
| Concurrent readers and writers, lock on       | 0 torn reads                                  | Pass: 0 of 3,600,000 reads (9 runs)        |
| Concurrent readers and writers, `use_lock=0`  | Torn reads appear (expected failure)          | Shown in 7 of 9 runs: 0 to 7,647 per run   |
| Open the device when the module is not loaded | `open` fails with "No such file or directory" | Pass                                       |
| `strace` of `rw` and `fault`                  | Calls reach the device with right results     | Pass: `write` = 11, `read` = 11, `EFAULT`  |

---

## Tech

- **Language:** C (kernel module uses the kernel's GNU C dialect; the test program uses C99)
- **Environment:** Ubuntu 26.04.1 LTS on a 2015 Intel MacBook (kernel 7.0.0-34-generic), with a QEMU/KVM Ubuntu 26.04.1 guest (kernel 7.0.0-38-generic) for loading the module
- **Build:** GNU Make with the kernel build system (module), gcc (test program)
- **Analysis:** `readelf`, `nm`, `objdump`, `modinfo`, `strace`, `lldb`, `GDB`
- **Version control:** Git/GitHub

---

## Documentation

| Document                                                           | Description                                              |
| ------------------------------------------------------------------ | -------------------------------------------------------- |
| [Project Proposal](docs/proposal.md)                               | Original proposal submitted for CS 370                   |
| [Midterm Report](docs/reports/midterm_report.pdf)                  | Progress report with technical evidence                  |
| [Setup Guide](docs/setup_guide.md)                                 | How the development environment and test VM are set up   |
| [Code Walkthrough](docs/presentations/code_walkthrough.md)         | Explanation of the module and test program               |
| [Final Demo](docs/presentations/final/demo_narration.md)           | Demo script steps and what each one shows                |
| [Dev Log](docs/notes/dev_log.md)                                   | Chronological record of decisions, bugs, and fixes       |
| [Finalization Checklist](docs/notes/finalization_checklist.md)     | Remaining work and final deliverables                    |
| [Evidence](docs/evidence/)                                         | Saved tool output (`readelf`, `objdump`, `nm`, `dmesg`)  |

---

## License

GPL-2.0 — see [LICENSE](./LICENSE) for details.

The module source declares `MODULE_LICENSE("GPL")`, so the repository license matches what the kernel expects.
