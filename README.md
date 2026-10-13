# Linux Driver Development Exploration

![Language](https://img.shields.io/badge/language-C99-blue)
![Linux kernel module](https://img.shields.io/badge/Linux-kernel%20module-orange?logo=linux&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Ubuntu%20x86--64-informational)
![Status](https://img.shields.io/badge/status-in%20development-yellow)
![License](https://img.shields.io/badge/license-[LICENSE]-lightgrey)

A loadable Linux kernel module that registers a **virtual character device**, plus a user-space C program that talks to it through system calls. The project explores how user programs reach the kernel, how data crosses the user/kernel boundary safely, and what a compiled kernel module looks like at the object-file and assembly level.

Built for CS 370 — Computer Systems and Architecture.

---

## What it does

- Registers a virtual character device (`/dev/[devicename]`) when the module loads
- Implements `open`, `read`, `write`, and `close` (release) handlers for the device
- Stores data in a small fixed-size kernel buffer
- Moves data between user space and kernel space with `copy_from_user` and `copy_to_user`
- Logs each operation to the kernel log (`dmesg`)
- Includes a user-space test program that opens the device, writes a string, reads it back, and closes it
- Is analyzed with `readelf`, `nm`, `objdump`, `strace`, and lldb to connect the code to CS 370 concepts

> **Scope:** Real hardware, interrupts, USB/PCI drivers, and device trees are out of scope. The goal is a small driver that I can fully explain.

---

## Status

- [ ] Hello-world kernel module
- [ ] Module analysis (`modinfo`, `nm`, `readelf`, `objdump`)
- [ ] Character device registered with logging
- [ ] Kernel buffer with `copy_to_user` / `copy_from_user`
- [ ] User-space test program
- [ ] Edge-case testing and evidence collected

*TODO: update this list as stages are completed.*

---

## Screenshots

*TODO: Add terminal captures as each stage is completed.*

| Module load                                           | Device node                                           |
| ----------------------------------------------------- | ----------------------------------------------------- |
| ![insmod and dmesg output](docs/screenshots/load.png) | ![ls -l /dev/devicename](docs/screenshots/device.png) |

| Test program                                            | strace                                             |
| ------------------------------------------------------- | -------------------------------------------------- |
| ![Write and read round trip](docs/screenshots/test.png) | ![strace of syscalls](docs/screenshots/strace.png) |

---

## How it works

### Data flow

```text
user program        kernel                      driver module
-----------         ------                      -------------
open("/dev/X")  ->  syscall  ->  VFS lookup ->  open handler
write(fd, buf)  ->  syscall  ->  VFS         -> write handler -> copy_from_user -> kernel buffer
read(fd, buf)   ->  syscall  ->  VFS         -> read handler  -> copy_to_user   -> user buffer
close(fd)       ->  syscall  ->  VFS         -> release handler
```

The kernel cannot trust a pointer handed to it by a user program, since user and kernel space are separate address spaces. `copy_from_user` and `copy_to_user` validate the user address and copy the data safely, which is the central idea of the driver.

### CS 370 concepts

| CS 370 concept        | Where it shows up in this project                                                |
| --------------------- | -------------------------------------------------------------------------------- |
| System calls          | The test program reaches the driver through `open`, `read`, `write`, `close`     |
| Registers             | x86-64 syscall arguments passed in `rax`, `rdi`, `rsi`, `rdx` (observed in lldb) |
| Memory and pointers   | Kernel buffer, user buffer, and the addresses of each                            |
| User vs. kernel space | Separate address spaces; safe copying between them                               |
| Processes             | Identifying which process is using the device; observing it with `strace`        |
| Compilation           | Building the module against kernel headers with `make` and `gcc`                 |
| Object files          | Inspecting the `.ko` file's sections and symbols                                 |

*TODO: update as new course topics are applied to the driver.*

---

## Planned features

### Stretch goals (only if the core is finished)

- **Mutex around the buffer** — protect against concurrent access
- **`ioctl` command** — clear the buffer or report its size
- **Open counter** — track how many times the device has been opened
- **Kernel debugging setup** — attach a debugger to a kernel running in QEMU

---

## Project structure

```plaintext
.
├── module/
│   ├── [modulename].c         # Kernel module: registers device, implements file operations
│   └── Makefile               # Builds the module with the kernel's build system
├── user/
│   └── test_driver.c          # User-space test program
├── docs/
│   ├── screenshots/           # Terminal captures used in the report
│   ├── evidence/              # Saved readelf / nm / objdump / strace / lldb output
│   └── notes/                 # Dev log and AI use log
├── .gitignore
└── README.md
```

---

## Running locally

> **Warning:** Kernel modules run inside the real kernel. A bug can freeze or crash the machine. Test in a QEMU/KVM virtual machine when possible, commit work to Git, and run `sync` before loading a new build. This project cannot run on macOS or in cloud environments such as Codespaces; it needs a Linux system with matching kernel headers.

**Prerequisites:** Ubuntu [24.04 / 26.04] LTS on x86-64 (native install or VM)

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r) gdb lldb strace git
```

**1. Build the kernel module:**

```bash
cd module
make
```

This produces `[modulename].ko`.

**2. Load the module:**

```bash
sync
sudo insmod [modulename].ko
dmesg | tail
ls -l /dev/[devicename]
```

**3. Build and run the test program:**

```bash
cd ../user
gcc -std=c99 -g -Wall -o test_driver test_driver.c
./test_driver
```

If the device node needs elevated permissions, run `sudo ./test_driver`.

**4. Unload the module:**

```bash
sudo rmmod [modulename]
dmesg | tail
```

**5. Clean up build files:**

```bash
cd module
make clean
```

---

## Analysis commands

Used to gather the technical evidence for the reports:

```bash
modinfo [modulename].ko
nm [modulename].ko
readelf -h -S [modulename].ko
objdump -d [modulename].ko
strace ./test_driver
lldb ./test_driver
```

---

## Testing

There is no automated test framework; testing is done by running the module and test program and checking results manually.

| Test                                          | Expected result                               | Result |
| --------------------------------------------- | --------------------------------------------- | ------ |
| Load and unload the module                    | Clean load/unload, expected `dmesg` messages  | [ ]    |
| Device node appears                           | `/dev/[devicename]` exists after `insmod`     | [ ]    |
| Open and close the device                     | `open` and release messages appear in `dmesg` | [ ]    |
| Write then read                               | Data read back matches data written           | [ ]    |
| Read from an empty buffer                     | Returns 0 bytes, no crash                     | [ ]    |
| Write larger than the buffer                  | Rejected or truncated safely                  | [ ]    |
| Open the device when the module is not loaded | `open` fails with "No such file or directory" | [ ]    |

---

## Tech

- **Language:** C (kernel module uses the kernel's GNU C dialect; the test program uses C99)
- **Environment:** Ubuntu [24.04 / 26.04] LTS on a 2015 Intel MacBook, with a QEMU/KVM guest for testing
- **Kernel:** Linux `[output of uname -r]` **TODO: update kernel info**
- **Build:** GNU Make with the kernel build system (module), gcc (test program)
- **Analysis:** `readelf`, `nm`, `objdump`, `modinfo`, `strace`, `lldb`, `GDB`
- **Version control:** Git/Github

---

## Documentation

| Document | Description |
| ----------------------------------------- | -------------------------------------------------------- |
| [Project Proposal](docs/proposal.md) | Original proposal submitted for CS 370 |
| [Midterm Report](docs/midterm_report.pdf) | Progress report with technical evidence |
| [Dev Log](docs/notes/dev_log.md) | Chronological record of decisions, bugs, and fixes |
| [Evidence](docs/evidence/) | Saved tool output (`readelf`, `objdump`, `strace`, etc.) |

---

## License

[LICENSE] — see [LICENSE](./LICENSE) for details.

> Kernel modules usually declare a GPL-compatible license in the source (`MODULE_LICENSE("GPL")`). I will most likely do the same.
