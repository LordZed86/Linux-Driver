**Kellen Jones**

CS 370 - Systems & Architecture

Pavalli Deogade

Sep 30, 2026

# Project Proposal

## 1. Project title

Linux Driver Development Exploration: A Virtual Character Device Driver

## 2. Purpose and reason for choosing the topic

Linux distributions don't typically get drivers provided by the manufacturer of a device, which can be incredibly frustrating. I want to learn more about this side of Linux kernel development so I can understand how drivers are created for physical and virtual devices, and build a base of knowledge for writing my own drivers if an accessory doesn't work on one of my Linux systems.

A driver also sits exactly at the boundary between user programs and the kernel, which makes it a natural fit for the systems concepts in CS 370. Since I'm new to kernel development, this project focuses on a virtual character device rather than real hardware.

## 3. Project goals

- Build a loadable Linux kernel module that registers a virtual character device (`/dev/[devicename]`).
- Write a user-space C program that communicates with the device through standard system calls.
- Understand how data moves safely between user space and kernel space.
- Analyze the compiled module using `readelf`, `nm`, and `objdump`, and connect the results to CS 370 concepts.
- Document the development process, including problems and fixes, so the approach can be reused for future drivers.

## 4. Planned functionality

The driver will implement four operations on the virtual device:

- `open()`: called when a user program opens the device (logged to the kernel log).
- `read()`: returns data stored in a small kernel buffer to the user program.
- `write()`: copies data from the user program into the kernel buffer.
- `close()`: the user-space call that triggers the driver's release handler.

If time permits, stretch features include a mutex to protect the buffer, an `ioctl` command to clear the buffer or report its size, and an open-count tracker.

## 5. Inputs and outputs

### Inputs

Text data written to the device by the user-space program, and the shell commands used to load and unload the module (`insmod`, `rmmod`).

### Outputs

Data read back from the device, kernel log messages (`dmesg`), return values and error codes from system calls, and analysis output from `readelf`, `nm`, `objdump`, and `strace`.

## 6. CS 370 concepts you expect to use

### System calls

The user program reaches the driver through `open`, `read`, `write`, and `close`.

### Memory

A kernel buffer and safe copying with `copy_to_user` and `copy_from_user`.

### Processes

Identifying which process is using the device and observing it with `strace`.

### User vs. kernel space

Separate address spaces and why the kernel can't trust user pointers.

### Compilation

Building the module against the kernel headers with `make` and `gcc`.

### Object files

Inspecting the `.ko` file's sections and symbols.

## 7. Tools and Technologies

- **C and GCC:** the language and compiler for both the kernel module and the user-space test program.
- **GNU Make and kernel headers:** build the module against the running kernel's build system and headers.
- **Ubuntu [24.04 / 26.04 LTS] on a 2015 Intel MacBook:** the native x86 Linux environment for development and testing.
- **`insmod`, `rmmod`, `modinfo`:** load and unload the module and inspect its metadata.
- **`dmesg` and `printk`:** the main feedback channel from kernel code, since kernel modules can't print to the terminal like normal programs.
- **`readelf`, `nm`, `objdump`:** analyze the compiled `.ko` file's sections, symbols, and disassembly to connect the driver to object files and compilation.
- **lldb and GDB:** step through the user-space test program, view its assembly, registers, and pointer addresses. (The kernel module itself is analyzed statically and through logging.)
- **`strace`:** show the system calls the test program makes and confirm they reach the driver.
- **QEMU/KVM virtual machine (Ubuntu guest):** a disposable test environment for loading new builds of the module, so a kernel crash or freeze only affects the VM and not my main system. Snapshots let me roll back quickly.
- **Git/GitHub:** version control, committing after each working stage so a crash or bad change never costs progress.

## 8. Initial project scope

### In scope

- A virtual character device with:
  - `open`
  - `read`
  - `write`
  - `close`
- Fixed-size kernel buffer
- User-space test program
- Analysis of the compiled module

### Out of scope (unless time permits)

- Real hardware devices
- Interrupts
- USB or PCI drivers
- Device trees

>The initial implementation will be kept small, with extra features added only after the core works.

## 9. Expected challenges

- Kernel APIs change between versions, so many online tutorials won't compile unchanged on my kernel.
- Kernel code is harder to debug than user programs. Bugs can freeze the system, and most feedback comes from `dmesg`.
- Managing pointers and memory carefully, since mistakes in kernel space can crash the whole machine.
- Setting up a Linux environment on older Mac hardware, including driver issues such as Wi-Fi.

I plan to address these by matching documentation to my kernel version, committing to Git after every working stage, building in small steps, and running `sync` before loading new builds.

## 10. Testing approach

Each stage will be tested before moving on:

- The module loads and unloads cleanly, with the expected messages in `dmesg`.
- The device node appears and can be opened and closed.
- Data written from the user program is read back correctly.
- Edge cases are handled: reading from an empty buffer, writing more than the buffer holds, and opening a device that isn't loaded.
- `strace` output confirms the expected system calls reach the driver.

Results will be captured as terminal screenshots and logs for the reports.

## 11. Plan for development through the course

*(Tenetive and subject to change)*  

I would like to have something working for my midterm presentation but I dont expect to have anything fully running by then considering there are a lot of new concepts I will be researching in tandem with my development. I should have something to demo but honestly there wont be much exciting to demonstrate with a driver anyway so I think itll be ok.

| Stage | Timing | Research Focus | Build |
| ----- | ------------ | -------------- | ----- |
| 1 | Sep 30 - Oct 14 | How Linux modules work at a high level; the intro chapters of the Linux Kernel Module Programming Guide | Install Ubuntu, set up the VM and tools, build and load a hello-world module |
| 2 | Oct 15 - Oct 28 | What `.ko` files, symbols, and sections are; what `printk` and `dmesg` do | Analyze the module with `modinfo`, `nm`, `readelf`, `objdump`; start the midterm report notes |
| 2.5 | Oct 21 | Midterm | Present Midterm Report |
| 3 | Oct 29 - Nov 11 | Character devices, device numbers, and the `file_operations` structure | Register the device with stubbed `open`, `read`, `write`, `close` that log to `dmesg` |
| 4 | Nov 12 - Nov 24 | User vs. kernel pointers, `copy_to_user`, `copy_from_user` | Add the kernel buffer and the user-space test program; capture `strace` and lldb evidence |
| 5 | Nov 25 - Dec 3 | Review and polish | Edge-case testing, README, final report and demo prep; stretch features only if everything else works |
