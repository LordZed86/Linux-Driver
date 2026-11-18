#!/usr/bin/env bash
# Linking and loading evidence (CS 370 week 8). Run INSIDE the VM, from the repo root:
#   ./scripts/collect_linking.sh
#
# Compares the three kinds of ELF file the project produces and how each one
# gets its addresses:
#   mychardev.ko         relocatable object (REL), linked by the kernel at insmod
#   test_driver          dynamically linked executable (DYN/PIE), linked by ld.so at exec
#   test_driver_static   statically linked executable, linked completely by ld at build
# Does NOT load the module. Saves to docs/evidence/stage4/stage4_link_*.txt

set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$PWD
OUT=$ROOT/docs/evidence/stage4
mkdir -p "$OUT"

redact() {
  sed -e "s#$HOME#<home>#g" -e "s#\b$(hostname)\b#<host-name>#g" -e "s#\b$USER@#<host-user>@#g"
}
save() { redact > "$OUT/stage4_link_$1.txt"; echo "  saved stage4_link_$1.txt"; }
show() { printf '\n$ %s\n' "$1"; eval "$1" 2>&1; }   # print a command, then its output

echo "== build"
[ -f module/mychardev.ko ] || ( cd module && make >/dev/null ) || exit 1
gcc -std=c99 -O2 -Wall -o user/test_driver user/test_driver.c || exit 1
if ! gcc -std=c99 -O2 -Wall -static -o user/test_driver_static user/test_driver.c; then
  echo "static build failed (needs libc6-dev, which includes libc.a)"; exit 1
fi

echo "== ELF type: relocatable vs. dynamic vs. static"
{ show "file module/mychardev.ko user/test_driver user/test_driver_static"
  show "readelf -h module/mychardev.ko | grep -E 'Type|Entry'"
  show "readelf -h user/test_driver | grep -E 'Type|Entry'"
  show "readelf -h user/test_driver_static | grep -E 'Type|Entry'"
  show "ls -l user/test_driver user/test_driver_static | awk '{print \$5, \$9}'"
} | save types

echo "== dynamic linking: what test_driver needs at run time"
{ show "readelf -l user/test_driver | grep 'program interpreter'"
  show "ldd user/test_driver"
  show "ldd user/test_driver_static"
  show "nm -D --undefined-only user/test_driver | grep -wE 'open|pread|pwrite|read|write|fork|printf'"
  show "nm user/test_driver_static | grep -wE '[TW] (open|open64|pread64|pwrite64|fork)'"
} | save dynamic

echo "== the loader at work (no device needed: an unknown mode just prints usage)"
{ show "strace -e trace=openat,read ./user/test_driver usage-demo 2>&1 | head -n 6"
  echo
  echo "# Same program, statically linked: no loader, no libc to open"
  show "strace -e trace=openat,read ./user/test_driver_static usage-demo 2>&1 | head -n 6"
} | save loader

echo "== relocations in the module, filled in by the kernel at insmod"
{ show "nm -u module/mychardev.ko | grep -wE 'memdup_user|mutex_lock_interruptible|mutex_unlock|_copy_to_user|kfree|_printk'"
  show "readelf -rW module/mychardev.ko | awk '/^Relocation section/ {sec = \$3} /memdup_user|mutex_lock_interruptible|mutex_unlock|_copy_to_user/ {print sec, \$1, \$3, \$5}'"
} | save relocations

rm -f user/test_driver_static
echo
echo "Done. Read each stage4_link_*.txt before committing."
