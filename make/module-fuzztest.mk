
ifneq ($(FUZZ_EXTRA_OBJS),)

MODULE_TYPE := usertest

MODULE_USERTEST_GROUP := fuzz

MODULE_COMPILEFLAGS += -fsanitize=fuzzer

MODULE_LIBS += \
     system/ulib/zircon \
     system/ulib/launchpad \

MODULE_EXTRA_OBJS += $(FUZZ_EXTRA_OBJS)

include make/module-usertest.mk

MODULE_EXTRA_OBJS :=

endif
