#!/bin/sh
set -eu

if ! command -v strace >/dev/null 2>&1; then
    echo "strace не установлен. Установите: sudo apt install strace"
    exit 1
fi

make
strace -f -o strace.log \
  -e trace=clone,fork,vfork,execve,wait4,openat,close,ftruncate,mmap,munmap,futex,read,write,unlink \
  ./parent

echo "Трассировка сохранена в strace.log"
