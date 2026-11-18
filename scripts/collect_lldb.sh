#!/usr/bin/env bash
# lldb + memory-layout evidence for the final report. Run INSIDE the VM, from the
# repo root, AFTER collect_stage4.sh has passed:
#   ./scripts/collect_lldb.sh
#
# Saves to docs/evidence/stage4/:
#   stage4_lldb_rw.txt    stack frame of cmd_rw, syscall arguments in rdi/rsi/rdx
#                         at open/write/read, and read()'s return value in rax
#   stage4_kallsyms.txt   kernel addresses of the driver's buffer and handlers,
#                         to compare with the user-space addresses test_driver prints
#
# Usernames, the hostname, and the home path are replaced with placeholders.
# Read every file before committing anyway.

set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$PWD
OUT=$ROOT/docs/evidence/stage4
mkdir -p "$OUT"

redact() {
  sed -e "s#$HOME#<home>#g" -e "s#\b$(hostname)\b#<host-name>#g" -e "s#\b$USER\b#<host-user>#g"
}
save() { redact > "$OUT/stage4_$1.txt"; echo "  saved stage4_$1.txt"; }

command -v lldb >/dev/null || { echo "lldb not installed: sudo apt install lldb"; exit 1; }
sudo -v || exit 1
sync

echo "== build (debug info, no optimization so locals are visible)"
[ -f module/mychardev.ko ] || ( cd module && make ) || exit 1
gcc -std=c99 -g -O0 -Wall -o user/test_driver user/test_driver.c || exit 1

lsmod | grep -q '^mychardev' && sudo rmmod mychardev
sudo insmod module/mychardev.ko

echo "== lldb session on: test_driver rw"
# Breakpoint conditions on $rdi == 3 skip the printf writes to stdout (fd 1) and
# stop only on the device's file descriptor (3, the first one open() hands out).
sudo lldb --batch \
  -o 'settings set auto-confirm true' \
  -o 'breakpoint set --name cmd_rw' \
  -o 'breakpoint set --name open --name open64' \
  -o "breakpoint set --name write --condition '\$rdi == 3'" \
  -o "breakpoint set --name read --condition '\$rdi == 3'" \
  -o 'run' \
  -o 'script print("\n--- stopped in cmd_rw: the stack frame ---")' \
  -o 'bt' \
  -o 'register read rsp rbp rip' \
  -o 'frame variable --location' \
  -o 'continue' \
  -o 'script print("\n--- open(path=rdi, flags=rsi) ---")' \
  -o 'register read rdi rsi' \
  -o 'memory read --format s $rdi' \
  -o 'continue' \
  -o 'script print("\n--- write(fd=rdi, buf=rsi, count=rdx) ---")' \
  -o 'register read rdi rsi rdx' \
  -o 'memory read --format s $rsi' \
  -o 'continue' \
  -o 'continue' \
  -o 'script print("\n--- read(fd=rdi, buf=rsi, count=rdx) ---")' \
  -o 'register read rdi rsi rdx' \
  -o 'thread step-out' \
  -o 'script print("\n--- read() returned: rax = bytes read ---")' \
  -o 'register read rax' \
  -o 'continue' \
  -- "$ROOT/user/test_driver" rw "hello from lldb" 2>&1 | save lldb_rw

echo "== kernel addresses of the driver (compare with the user addresses above)"
{ echo '$ sudo grep "\[mychardev\]" /proc/kallsyms | grep -E "kbuf|mychardev_(read|write|open|release)"'
  sudo grep '\[mychardev\]' /proc/kallsyms | grep -E 'kbuf|mychardev_(read|write|open|release)'
  echo
  echo '$ sudo ./test_driver rw'
  sudo "$ROOT/user/test_driver" rw | grep 'address'; } 2>&1 | save kallsyms

sudo rmmod mychardev
echo
echo "Done. Expect user addresses near 0x7ff... and kernel addresses near 0xffff..."
echo "Read each file in docs/evidence/stage4/ before committing."
