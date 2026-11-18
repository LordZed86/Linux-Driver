#!/usr/bin/env bash
# CS 370 FINAL demo (stage 4 build). Run INSIDE the VM, from anywhere in the repo:
#   ./docs/presentations/final/demo.sh
# Press Enter to run each step. Ctrl+C to stop.
# Before sharing your screen: export PS1='$ '  and make the terminal font large.
# Narration for each step: demo_narration.md (same folder).

ROOT=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
cd "$ROOT" || exit 1
RACE_ITER=${RACE_ITER:-500000}

step() {            # print a heading, then wait for Enter
  echo
  echo "### $1"
  read -r -p "[Enter to run] " _
}

run() {             # show the command, then run it
  printf '$ %s\n' "$1"
  eval "$1"
}

reload() {          # reload the module quietly with the given parameters
  lsmod | grep -q '^mychardev' && sudo rmmod mychardev
  sudo insmod module/mychardev.ko "$@"
}

# --- setup (done before the audience sees anything) ---
sudo -v                                      # cache sudo so no password prompt shows on screen
lsmod | grep -q '^mychardev' && sudo rmmod mychardev
sudo dmesg -C                                # clear the kernel log so only the demo lines appear
( cd module && make clean >/dev/null 2>&1 )
clear

step "1. Build the module and the test program"
run "(cd module && make 2>&1 | grep -E '^ *(CC|LD|MODPOST)')"
run "gcc -std=c99 -g -Wall -o user/test_driver user/test_driver.c && ls -l module/mychardev.ko user/test_driver"

step "2. Load it"
run "sudo insmod module/mychardev.ko"
run "lsmod | grep mychardev"
run "ls -l /dev/mychardev"

step "3. Read with nothing stored (empty buffer = end of file)"
run "sudo ./user/test_driver empty"

step "4. Shell round trip: write, then read back"
run "echo hello | sudo tee /dev/mychardev"
run "sudo cat /dev/mychardev"

step "5. Test program: write a string, reopen, read it back"
run "sudo ./user/test_driver rw 'hello from user space'"

step "6. Edge case: 2000 bytes into a 1024-byte buffer"
run "sudo ./user/test_driver big"

step "7. Edge case: bad pointer (the kernel must not crash)"
run "sudo ./user/test_driver fault"

step "8. What the kernel logged"
run "sudo dmesg -t | grep mychardev | tail -n 20"

step "9. Mutex ON: 2 writers + 2 readers, $RACE_ITER iterations each"
reload verbose=0 use_lock=1
run "sudo dmesg -t | grep 'lock' | tail -n 1"
run "sudo ./user/test_driver race $RACE_ITER"

step "10. Mutex OFF: same test, lock disabled"
reload verbose=0 use_lock=0
run "sudo dmesg -t | grep 'lock' | tail -n 1"
run "sudo ./user/test_driver race $RACE_ITER"

step "11. Unload, then try to use the device"
run "sudo rmmod mychardev"
run "sudo dmesg -t | grep mychardev | tail -n 1"
run "sudo cat /dev/mychardev"

echo
echo "### done"
