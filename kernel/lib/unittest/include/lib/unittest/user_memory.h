
#pragma once

#include <ktl/unique_ptr.h>
#include <ktl/move.h>
#include <lib/user_copy/user_ptr.h>
#include <vm/pmm.h>
#include <vm/vm.h>
#include <vm/vm_aspace.h>
#include <vm/vm_object_paged.h>

namespace testing {

// UserMemory facilitates testing code that requires user memory.
//
// Example:
//    unique_ptr<UserMemory> mem = UserMemory::Create(sizeof(thing));
//    auto mem_out = make_user_out_ptr(mem->out());
//    mem_out.copy_array_to_user(&thing, sizeof(thing));
//
class UserMemory {
public:
    static ktl::unique_ptr<UserMemory> Create(size_t size);
    virtual ~UserMemory();
    void* out() { return reinterpret_cast<void*>(mapping_->base()); }
    const void* in() { return reinterpret_cast<void*>(mapping_->base()); }

private:
    UserMemory(fbl::RefPtr<VmMapping> mapping)
        : mapping_(ktl::move(mapping)) {}

    fbl::RefPtr<VmMapping> mapping_;
};

} // namespace testing
