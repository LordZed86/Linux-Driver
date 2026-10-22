#!/usr/bin/env bash
# CS 370 midterm demo. Run INSIDE the VM, from the folder that holds the module:
#   cd ~/module && ./demo.sh
# Press Enter to run each step. Ctrl+C to stop.
# Before sharing your screen: export PS1='$ '  and make the terminal font large.

cd "$(dirname "$0")" || exit 1

step() {            # print a heading, then wait for Enter
  echo
  echo "### $1"
  read -r -p "[Enter to run] " _
}

run() {             # show the command, then run it
  printf '$ %s\n' "$1"
  eval "$1"
}

# --- setup (done before the audience sees anything) ---
sudo -v                                      # cache sudo so no password prompt shows on screen
if lsmod | grep -q '^mychardev'; then sudo rmmod mychardev; fi
sudo dmesg -C                                # clear the kernel log so only the demo lines appear
clear

step "1. Build the module"
run "make clean >/dev/null 2>&1; make 2>&1 | grep -E '^ *(CC|LD|MODPOST)'"
run "ls -l mychardev.ko"

step "2. Load it"
run "sudo insmod mychardev.ko"
run "lsmod | grep mychardev"
run "ls -l /dev/mychardev"

step "3. Write to the device"
run "echo hello | sudo tee /dev/mychardev"

step "4. Read from the device (returns nothing in stage 3)"
run "sudo cat /dev/mychardev"

step "5. What the kernel logged"
run "sudo dmesg -t | grep mychardev"

step "6. Unload it"
run "sudo rmmod mychardev"
run "sudo dmesg -t | grep mychardev | tail -n 1"

echo
echo "### done"
