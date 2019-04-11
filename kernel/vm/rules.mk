
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    kernel/lib/fbl \
    kernel/lib/pretty \
    kernel/lib/user_copy \
    third_party/lib/cryptolib

MODULE_SRCS += \
    $(LOCAL_DIR)/bootalloc.cpp \
    $(LOCAL_DIR)/bootreserve.cpp \
    $(LOCAL_DIR)/kstack.cpp \
    $(LOCAL_DIR)/page.cpp \
    $(LOCAL_DIR)/page_source.cpp \
    $(LOCAL_DIR)/pinned_vm_object.cpp \
    $(LOCAL_DIR)/pmm.cpp \
    $(LOCAL_DIR)/pmm_arena.cpp \
    $(LOCAL_DIR)/pmm_node.cpp \
    $(LOCAL_DIR)/vm.cpp \
    $(LOCAL_DIR)/vm_address_region.cpp \
    $(LOCAL_DIR)/vm_address_region_or_mapping.cpp \
    $(LOCAL_DIR)/vm_aspace.cpp \
    $(LOCAL_DIR)/vm_mapping.cpp \
    $(LOCAL_DIR)/vm_object.cpp \
    $(LOCAL_DIR)/vm_object_paged.cpp \
    $(LOCAL_DIR)/vm_object_physical.cpp \
    $(LOCAL_DIR)/vm_page_list.cpp \
    $(LOCAL_DIR)/vm_unittest.cpp \
    $(LOCAL_DIR)/vmm.cpp \

include make/module.mk
