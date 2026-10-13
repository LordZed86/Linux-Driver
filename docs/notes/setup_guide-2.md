# Environment Setup Guide

How my development environment for the Linux driver project is set up, why each piece is there, and how to get going when I sit down to work.

> **Placeholders:** Identifying details are masked on purpose. `<host-user>` is my username on the Ubuntu host, `<host-name>` is its hostname, `<mac-ip>` and `<vm-ip>` are IP addresses. Substitute your own values when running commands.

**Contents**

1. [Quick start](#quick-start)
2. [Environment overview](#environment-overview)
3. [Verify the toolchain](#1-verify-the-toolchain)
4. [SSH access](#2-ssh-access)
5. [Install virtualization tools](#3-install-virtualization-tools)
6. [Create the test VM](#4-create-the-test-vm)
7. [Prepare the VM for module builds](#5-prepare-the-vm-for-module-builds)
8. [Backup and restore the VM](#6-backup-and-restore-the-vm)
9. [Capturing evidence](#capturing-evidence-for-reports)
10. [Troubleshooting](#troubleshooting-problems-i-hit)

> **Status note:** Sections 1-3 were done and confirmed. Section 4 onward is the plan as I set it up; update any step that turned out different in practice (and log it in `docs/notes/dev_log.md`).

---

## Quick start

The short version for starting a work session. Details for each step are further down.

```bash
# 1. From my other computer, connect to the Ubuntu Mac (the "host").
ssh <host-user>@<mac-ip>

# 2. Make sure the test VM is running. "running" should appear in the State column.
virsh list --all
virsh start driverdev          # only needed if the state says "shut off"

# 3. Find the VM's IP address (it can change between boots).
virsh domifaddr driverdev

# 4. Connect to the VM. From the Mac itself:
ssh dev@<vm-ip>
#    ...or from my other computer, hopping through the Mac:
ssh -J <host-user>@<mac-ip> dev@<vm-ip>
```

**The edit, build, test loop** (code is edited and committed on the host, then run in the VM):

```bash
# On the host: edit in VS Code (Remote-SSH), then commit and push.
git add -A && git commit -m "describe the change" && git push

# In the VM: get the latest code and build it.
cd ~/driver-project && git pull
cd module && make

# In the VM: load, check, and unload. NEVER do this on the host.
sync                              # flush disk writes in case the module crashes the VM
sudo insmod mychardev.ko          # load the module
dmesg | tail                      # read the kernel log to see what the module printed
sudo rmmod mychardev              # unload the module
dmesg | tail                      # confirm the unload message
```

**If the VM freezes or breaks:** see [Backup and restore](#6-backup-and-restore-the-vm).
**When finished:** `sudo poweroff` inside the VM.

---

## Environment overview

| Piece | What it is | Why it's here |
| ----- | ---------- | ------------- |
| **Host** | 2015 MacBook Pro (`<host-name>`) running Ubuntu 26.04.1 LTS, kernel `7.0.0-34-generic`, 7.2 GB RAM | Native x86-64 Linux, which matches what CS 370 expects for `gcc`, `objdump`, and lldb output |
| **Other computer** | Apple silicon laptop | Used to connect to the host over SSH; builds there would be ARM, so nothing is compiled on it |
| **Test VM** (`driverdev`) | QEMU/KVM guest running Ubuntu 26.04, 2 GB RAM, 2 CPUs, 20 GB disk | Kernel modules run inside the real kernel, so a bug can crash the machine. The VM takes the crash instead of the host |
| **Git / GitHub** | Version control | Commit after each working stage so a crash or bad change never costs progress |

**Rule:** build anywhere, but only ever *load* (`insmod`) modules inside the VM.

---

## 1. Verify the toolchain

Run on the **host**. These commands only read information, so they are safe.

```bash
# Show the OS release, kernel version, memory, and free disk space on /.
lsb_release -d; uname -r; free -h | head -2; df -h / | tail -1
```

```bash
# For each tool, "command -v" prints its path if installed (output hidden).
# The && / || prints "ok" if found, or "MISSING" if not.
# Everything must stay on ONE line, or bash reports a missing "do".
for t in gcc make git gdb lldb strace objdump readelf nm; do command -v $t >/dev/null && echo "ok $t" || echo "MISSING $t"; done
```

```bash
# Kernel headers must match the RUNNING kernel (uname -r), or modules won't build.
# Look for the build directory; print a verdict.
ls /lib/modules/$(uname -r)/build >/dev/null 2>&1 && echo "headers ok" || echo "headers MISSING"
```

```bash
# Count CPU entries advertising hardware virtualization (vmx = Intel, svm = AMD).
# Anything above 0 means KVM can work.
grep -Ec '(vmx|svm)' /proc/cpuinfo
```

If anything is missing, install it:

```bash
sudo apt update
# build-essential: gcc, make, and binutils (which provides objdump, readelf, nm)
# linux-headers-...: kernel headers needed to build modules
# gdb, lldb: debuggers   strace: shows system calls   git: version control
sudo apt install -y build-essential linux-headers-$(uname -r) gdb lldb strace git
```

If an update installs a newer kernel, reboot so `uname -r` matches the installed headers.

**My result:** all tools present, headers OK, virtualization flags present on all 8 logical CPUs.

---

## 2. SSH access

SSH lets me work on the host from another computer. The host needs the SSH **server**; the `ssh` command that comes with Ubuntu is only the client.

```bash
# On the host: check whether the SSH server is running.
systemctl status ssh
# If "could not be found":   sudo apt install -y openssh-server
# If installed but inactive: sudo systemctl enable --now ssh

# Get the username and IP address needed to connect.
whoami
hostname -I
```

```bash
# From the other computer:
ssh <host-user>@<mac-ip>
```

Notes:
- Both computers must be on the same network.
- If the host sleeps or the lid closes, the SSH session drops. Change the lid-close behavior in power settings or leave the lid open.
- VS Code's **Remote - SSH** extension ("Connect to Host", same `user@ip`) edits files directly on the host.

---

## 3. Install virtualization tools

On the **host**. Uses command-line tools because I work over SSH.

```bash
sudo apt update
# qemu-system-x86:      the emulator/hypervisor that runs the VM
# libvirt-*:            the management layer (provides the virsh command)
# virtinst:             provides virt-install, which creates VMs from the command line
# cpu-checker:          provides kvm-ok, which tests whether KVM works
sudo apt install -y qemu-system-x86 libvirt-daemon-system libvirt-clients virtinst cpu-checker unzip

# Give my user permission to manage VMs (libvirt) and use hardware acceleration (kvm).
sudo adduser $USER libvirt
sudo adduser $USER kvm
```

**Group changes only apply to new logins.** Run `exit` and SSH back in, then verify:

```bash
kvm-ok                 # should say "KVM acceleration can be used"
id -nG                 # should list libvirt (and kvm) among my groups
virsh list --all       # should print an empty table with no error
```

---

## 4. Create the test VM

I use an Ubuntu **cloud image**: a prebuilt disk image, so there is no installer to click through. The VM's first-boot setup (user, password, packages) comes from a small `cloud-init` config file.

> I confirmed Canonical publishes 26.04 ("resolute") images under `https://cloud-images.ubuntu.com/releases/resolute/`. If the exact filename below returns a 404, open that listing and copy the `amd64` server `.img` name. Use `amd64`, not `amd64v3`, so it runs everywhere.

**Download the image into libvirt's folder.** libvirt often can't read files in a home directory, so images live in `/var/lib/libvirt/images`.

```bash
sudo apt install -y cloud-image-utils     # provides cloud-localds, used below
cd /var/lib/libvirt/images

# Download the cloud image and name it as the VM's disk.
sudo wget -O driverdev.qcow2 https://cloud-images.ubuntu.com/releases/resolute/release/ubuntu-26.04-server-cloudimg-amd64.img

# The downloaded image is tiny; grow the virtual disk to 20 GB.
sudo qemu-img resize driverdev.qcow2 20G
```

**Write the first-boot config.** Choose my own password for `CHANGE-ME`, and never commit this file with a real password in it.

```bash
mkdir -p ~/vm && cd ~/vm
cat > user-data << 'EOF'
#cloud-config
hostname: driverdev
ssh_pwauth: true                # allow password login over SSH
users:
  - name: dev
    sudo: ALL=(ALL) NOPASSWD:ALL   # dev can use sudo without a password prompt
    shell: /bin/bash
    lock_passwd: false
    plain_text_passwd: CHANGE-ME
package_update: true
packages: [build-essential, gdb, lldb, strace, git]
EOF

# Package the config into a small disk image that cloud-init reads on first boot.
cloud-localds seed.img user-data
sudo cp seed.img /var/lib/libvirt/images/
```

**Start libvirt's virtual network, then create the VM.**

```bash
virsh net-list --all                              # the "default" network should exist
sudo virsh net-start default 2>/dev/null          # start it (harmless error if already running)
sudo virsh net-autostart default                  # start it automatically on every boot

# --import:      boot the existing disk instead of running an installer
# --disk ...cdrom: attaches the cloud-init config as a CD drive
# --graphics none / --noautoconsole: no display; I connect over SSH or text console
sudo virt-install --name driverdev --memory 2048 --vcpus 2 \
  --disk /var/lib/libvirt/images/driverdev.qcow2 \
  --disk /var/lib/libvirt/images/seed.img,device=cdrom \
  --os-variant ubuntu24.04 --import --network network=default \
  --graphics none --noautoconsole
```

**Log in.** Wait about a minute for the first boot.

```bash
virsh domifaddr driverdev        # shows the VM's IP on the private virtual network
ssh dev@<vm-ip>                  # run from the host
# No IP showing? Use the text console instead (exit with Ctrl+]):
virsh console driverdev
```

The VM's address is only reachable from the host. From another computer, hop through the host with `ssh -J <host-user>@<mac-ip> dev@<vm-ip>`.

---

## 5. Prepare the VM for module builds

Run **inside the VM**.

```bash
uname -r                                       # the VM's running kernel
sudo apt install -y linux-headers-$(uname -r)  # headers MUST match the running kernel
```

If that package isn't found, cloud images sometimes ship a slimmed-down kernel. Note the `uname -r` output and look for a matching headers package, or install the standard kernel and headers and reboot:

```bash
sudo apt install -y linux-image-generic linux-headers-generic
sudo reboot
```

Then clone the project:

```bash
git clone <repo-url> ~/driver-project
```

First real test: `cd ~/driver-project/module && make`, then the load/unload cycle from the [quick start](#quick-start). Screenshot the `dmesg` output for the midterm report.

---

## 6. Backup and restore the VM

Once the VM works and has its tools, save a clean copy of its disk. I use a file copy instead of `virsh snapshot` because snapshots can fail on UEFI cloud images.

```bash
# Create the backup (shut the VM down first so the disk is consistent).
virsh shutdown driverdev                       # or: sudo poweroff inside the VM
sudo cp /var/lib/libvirt/images/driverdev.qcow2 /var/lib/libvirt/images/driverdev-clean.qcow2
```

**Restore after a crash or wedged module:**

```bash
virsh destroy driverdev                        # force power off; does NOT delete the VM
sudo cp /var/lib/libvirt/images/driverdev-clean.qcow2 /var/lib/libvirt/images/driverdev.qcow2
virsh start driverdev
```

Anything not pushed to Git inside the VM is lost on restore, so commit and push first whenever possible.

---

## Capturing evidence for reports

The midterm and final reports need technical evidence. Save tool output to text files in `docs/evidence/`, and take screenshots for the report itself.

```bash
cd ~/driver-project/module

# "tee" prints the output on screen AND saves it to a file.
modinfo mychardev.ko        | tee ../docs/evidence/modinfo.txt      # module metadata
nm mychardev.ko             | tee ../docs/evidence/nm.txt           # symbols in the object file
readelf -h -S mychardev.ko  | tee ../docs/evidence/readelf.txt      # ELF header and sections
objdump -d mychardev.ko     | tee ../docs/evidence/objdump.txt      # disassembly

# After loading the module, save the kernel log lines it produced.
dmesg | tail -20            | tee ../docs/evidence/dmesg_load.txt
```

---

## Troubleshooting: problems I hit

| Problem | Cause | Fix |
| ------- | ----- | --- |
| Shell says it can't find the `do` command when running the tool-check loop | The multi-line `for` loop got split when pasted | Run it as one line (see section 1) |
| `virsh list` fails with "Permission denied" on `libvirt-sock` | My login session started before I was added to the `libvirt` group; groups only apply to new logins | `exit`, reconnect over SSH, and check `id -nG` includes `libvirt` |
| `getent group libvirt` shows my username, but `id -nG` doesn't list `libvirt` | Same as above | Reconnect (or `newgrp libvirt` for a single shell) |
| `kvm-ok` doesn't say acceleration can be used | Virtualization unavailable or not permitted | Check the `vmx`/`svm` flag count; make sure I'm in the `kvm` group |
| Can't SSH to the host | Server not installed or not running, firewall, or different networks | `systemctl status ssh`, `sudo ufw status`, confirm both machines share a network |
| `virt-install` errors on `--os-variant` | The OS database doesn't know the name | Try a different known variant and note which one worked |
| `virsh domifaddr` shows no IP | VM still booting, or `default` network not started | Wait a minute, check `virsh net-list --all`, or use `virsh console driverdev` |
| Header package not found in the VM | Cloud kernel has no matching headers package | See section 5 |
| `make` says "missing separator" | A Makefile recipe line starts with spaces instead of a tab | Replace the leading spaces with a real tab |
| VM frozen after loading a module | The module crashed the guest kernel | `virsh destroy driverdev`, then restore from the clean backup |

---

*Add new problems and fixes to the table as they come up, and mirror them in the dev log.*
