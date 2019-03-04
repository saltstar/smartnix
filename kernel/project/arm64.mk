
# Project file to build zircon + user space for 64bit arm (cortex-a53)

ARCH := arm64
TARGET := arm64

include kernel/project/virtual/test.mk
include kernel/project/virtual/user.mk
