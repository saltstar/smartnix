
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/version.cpp

# if no one else has defined it by now, build us a default buildid
# based on the current time.
ifeq ($(strip $(BUILDID)),)
BUILDID := "$(shell $(SHELLEXEC) $(LOCAL_DIR)/buildid.sh)"
endif

# Generate a buildid.h file, lazy evaulated BUILDID_DEFINE at the end
# of the first make pass. This lets modules that haven't been
# included yet set BUILDID.
BUILDID_DEFINE="BUILDID=\"$(BUILDID)\""
BUILDID_H := $(BUILDDIR)/config-buildid.h
$(BUILDID_H): FORCE
	@$(call MAKECONFIGHEADER,$@,BUILDID_DEFINE)

GENERATED += $(BUILDID_H)

# make sure the module properly depends on and can find buildid.h
MODULE_SRCDEPS := $(BUILDID_H)

include make/module.mk
