
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_SRCS += \
    $(LOCAL_DIR)/crashanalyzer.cpp

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-crash \
    system/fidl/fuchsia-mem \

MODULE_HEADER_DEPS := \
    system/ulib/svc \

MODULE_STATIC_LIBS := \
    system/ulib/elf-search \
    system/ulib/inspector \
    system/ulib/async \
    system/ulib/async.cpp \
    system/ulib/fbl \
    system/ulib/fidl \
    system/ulib/pretty \
    system/ulib/runtime \
    system/ulib/zircon-internal \
    system/ulib/zxcpp

MODULE_LIBS := \
    third_party/ulib/backtrace \
    third_party/ulib/ngunwind \
    system/ulib/fdio \
    system/ulib/zircon \
    system/ulib/c

# Compile this with frame pointers so that if we crash
# the simplistic unwinder will work.
MODULE_COMPILEFLAGS += $(KEEP_FRAME_POINTER_COMPILEFLAGS)

include make/module.mk
