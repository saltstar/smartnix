
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
    $(LOCAL_DIR)/buffer_chain.cpp \
    $(LOCAL_DIR)/bus_transaction_initiator_dispatcher.cpp \
    $(LOCAL_DIR)/channel_dispatcher.cpp \
    $(LOCAL_DIR)/diagnostics.cpp \
    $(LOCAL_DIR)/dispatcher.cpp \
    $(LOCAL_DIR)/event_dispatcher.cpp \
    $(LOCAL_DIR)/event_pair_dispatcher.cpp \
    $(LOCAL_DIR)/exception.cpp \
    $(LOCAL_DIR)/excp_port.cpp \
    $(LOCAL_DIR)/fifo_dispatcher.cpp \
    $(LOCAL_DIR)/futex_context.cpp \
    $(LOCAL_DIR)/futex_node.cpp \
    $(LOCAL_DIR)/glue.cpp \
    $(LOCAL_DIR)/guest_dispatcher.cpp \
    $(LOCAL_DIR)/handle.cpp \
    $(LOCAL_DIR)/interrupt_dispatcher.cpp \
    $(LOCAL_DIR)/interrupt_event_dispatcher.cpp \
    $(LOCAL_DIR)/iommu_dispatcher.cpp \
    $(LOCAL_DIR)/job_dispatcher.cpp \
    $(LOCAL_DIR)/job_policy.cpp \
    $(LOCAL_DIR)/log_dispatcher.cpp \
    $(LOCAL_DIR)/mbuf.cpp \
    $(LOCAL_DIR)/message_packet.cpp \
    $(LOCAL_DIR)/pager_dispatcher.cpp \
    $(LOCAL_DIR)/pci_device_dispatcher.cpp \
    $(LOCAL_DIR)/pci_interrupt_dispatcher.cpp \
    $(LOCAL_DIR)/pinned_memory_token_dispatcher.cpp \
    $(LOCAL_DIR)/port_dispatcher.cpp \
    $(LOCAL_DIR)/process_dispatcher.cpp \
    $(LOCAL_DIR)/profile_dispatcher.cpp \
    $(LOCAL_DIR)/resource_dispatcher.cpp \
    $(LOCAL_DIR)/resource.cpp \
    $(LOCAL_DIR)/semaphore.cpp \
    $(LOCAL_DIR)/socket_dispatcher.cpp \
    $(LOCAL_DIR)/suspend_token_dispatcher.cpp \
    $(LOCAL_DIR)/thread_dispatcher.cpp \
    $(LOCAL_DIR)/timer_dispatcher.cpp \
    $(LOCAL_DIR)/vcpu_dispatcher.cpp \
    $(LOCAL_DIR)/virtual_interrupt_dispatcher.cpp \
    $(LOCAL_DIR)/vm_address_region_dispatcher.cpp \
    $(LOCAL_DIR)/vm_object_dispatcher.cpp \
    $(LOCAL_DIR)/wait_state_observer.cpp \

# Tests
MODULE_SRCS += \
    $(LOCAL_DIR)/buffer_chain_tests.cpp \
    $(LOCAL_DIR)/job_policy_tests.cpp \
    $(LOCAL_DIR)/mbuf_tests.cpp \
    $(LOCAL_DIR)/message_packet_tests.cpp \
    $(LOCAL_DIR)/state_tracker_tests.cpp \

MODULE_DEPS := \
    kernel/dev/interrupt \
    kernel/dev/udisplay \
    kernel/lib/fbl \
    kernel/lib/hypervisor \
    kernel/lib/oom \
    kernel/lib/pretty \
    kernel/lib/region-alloc \

include make/module.mk
