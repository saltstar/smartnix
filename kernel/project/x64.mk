
# Project file to build zircon + user space on top of qemu
# emulating a standard PC with a 64bit x86 core

ARCH := x86
TARGET := pc

include kernel/project/virtual/test.mk
include kernel/project/virtual/user.mk
