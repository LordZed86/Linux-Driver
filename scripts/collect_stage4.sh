#!/usr/bin/env bash
# Stage 4 test run and evidence collection. Run INSIDE the VM, from the repo root:
#   ./scripts/collect_stage4.sh
#
# Builds the module and test program, runs every test mode, runs the race test
# with the mutex on and off, and saves all output to docs/evidence/stage4/.
# Usernames, the hostname, and the home path are replaced with placeholders.
# Read every file before committing anyway.
#
# Safety: this loads a kernel module.
# !! Commit first !!
# !! Run only in the VM !!

set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$PWD
OUT=$ROOT/docs/evidence/stage4
RACE_ITER=${RACE_ITER:-2000}
RACE_RUNS=${RACE_RUNS:-3}
mkdir -p "$OUT"

redact() {  # hide user name, host name, and home path
  sed -e "s#$HOME#<home>#g" -e "s#\b$(hostname)\b#<host-name>#g" -e "s#\b$USER\b#<host-user>#g"
}

save() {  # save <name>: stdin -> docs/evidence/stage4/stage4_<name>.txt
  redact > "$OUT/stage4_$1.txt"
  echo "  saved stage4_$1.txt"
}

run_test() {  # run_test <label> <args...>: run test_driver, record output and exit code
  local label=$1; shift
  { printf '$ sudo ./test_driver %s\n' "$*"
    sudo "$ROOT/user/test_driver" "$@" 2>&1
    echo "[exit status $?]"; } | save "test_$label"
}

unload() { lsmod | grep -q '^mychardev' && sudo rmmod mychardev; }

sudo -v || exit 1
sync

echo "== environment"
{ echo "kernel: $(uname -r)"; echo "cpus:   $(nproc)"; gcc --version | head -n 1; } | save env
if [ "$(nproc)" -lt 2 ]; then
  echo "  WARNING: VM has 1 CPU; the use_lock=0 race test will probably show 0 torn reads."
fi

echo "== build"
( cd module && make clean >/dev/null 2>&1; make 2>&1 ) | save build
[ -f module/mychardev.ko ] || { echo "build failed, see $OUT/stage4_build.txt"; exit 1; }
gcc -std=c99 -g -Wall -Wextra -pedantic -o user/test_driver user/test_driver.c 2>&1 | save build_user
[ -x user/test_driver ] || { echo "test program build failed"; exit 1; }

echo "== object file analysis"
modinfo module/mychardev.ko | save modinfo
nm module/mychardev.ko | save nm
readelf -S module/mychardev.ko | save readelf_sections
objdump -dr module/mychardev.ko | save objdump

echo "== functional tests (lock on, verbose)"
unload
sudo dmesg -C
sudo insmod module/mychardev.ko
ls -l /dev/mychardev | save devnode
run_test empty empty
run_test rw rw "hello from user space"
run_test big big
run_test fault fault
{ echo '$ echo keep-me | sudo tee /dev/mychardev; sudo ./test_driver fault; sudo cat /dev/mychardev'
  echo keep-me | sudo tee /dev/mychardev >/dev/null
  sudo "$ROOT/user/test_driver" fault >/dev/null 2>&1
  got=$(sudo cat /dev/mychardev)
  echo "read back: $got"
  if [ "$got" = keep-me ]; then echo "PASS: failed write left the stored data unchanged"
  else echo "FAIL: failed write changed the stored data"; fi; } 2>&1 | save test_fault_keeps_data
{ echo '$ echo hello | sudo tee /dev/mychardev'; echo hello | sudo tee /dev/mychardev
  echo '$ sudo cat /dev/mychardev'; sudo cat /dev/mychardev; } 2>&1 | save shell_rw
sudo strace -o "$OUT/strace.tmp" "$ROOT/user/test_driver" rw "strace demo" >/dev/null 2>&1
grep -E '"/dev/mychardev"|^(read|write|close)\(3,' "$OUT/strace.tmp" | save strace_rw
sudo strace -o "$OUT/strace.tmp" "$ROOT/user/test_driver" fault >/dev/null 2>&1
grep -E '"/dev/mychardev"|^(write|close)\(3,' "$OUT/strace.tmp" | save strace_fault
sudo rm -f "$OUT/strace.tmp"
sudo rmmod mychardev
sudo dmesg -t | grep mychardev | save dmesg

echo "== race test, use_lock=1 then use_lock=0 ($RACE_RUNS runs x $RACE_ITER iterations)"
for lock in 1 0; do
  sudo insmod module/mychardev.ko verbose=0 use_lock=$lock
  for i in $(seq 1 "$RACE_RUNS"); do run_test "race_lock${lock}_run$i" race "$RACE_ITER"; done
  sudo rmmod mychardev
done

echo "== open with the module unloaded"
{ echo '$ sudo cat /dev/mychardev'; sudo cat /dev/mychardev; echo "[exit status $?]"; } 2>&1 | save unloaded

echo
echo "== summary"
grep -E '^(PASS|FAIL)|torn reads:' "$OUT"/stage4_test_*.txt
echo
echo "Evidence is in docs/evidence/stage4/. Read each file before committing."
