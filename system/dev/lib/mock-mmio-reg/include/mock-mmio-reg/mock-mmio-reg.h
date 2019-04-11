// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/mmio-buffer.h>
#include <fbl/vector.h>
#include <unittest/unittest.h>

void mmio_fake_read(uintptr_t base, size_t size, zx_off_t off, void* value);
void mmio_fake_write(uintptr_t base, size_t size, const void* value, zx_off_t off);

namespace ddk_mock {

// Mocks a single MMIO register. This class is intended to be used with a ddk::MmioBuffer;
// operations on an instance of that class will be directed to the mock through mmio_fake_read and
// mmio_fake_write if TEST_MMIO_FAKE is defined. The base address used by the MmioBuffer should be
// an array of MockMmioReg objects. See the following example test:
//
// ddk_mock::MockMmioReg register_array[number_of_registers];
// ddk_mock::MockMmioRegRegion mock_registers(register_array, register_size, number_of_registers);
// ddk::MmioBuffer mmio_buffer(mock_registers.GetMmioBuffer());
//
// SomeDriver dut(mmio_buffer);
// mock_registers[0]
//     .ExpectRead()
//     .ExpectWrite(0xdeadbeef)
//     .ExpectRead(0xcafecafe)
//     .ExpectWrite()
//     .ExpectRead();
// mock_registers[5]
//     .ExpectWrite(0)
//     .ExpectWrite(1024)
//     .ReadReturns(0);
//
// EXPECT_EQ(dut.SomeMethod(), ZX_OK);
// mock_registers.VerifyAll();

class MockMmioReg {
public:
    // Reads from the mocked register. Returns the value set by the next expectation, or the default
    // value. The default is initially zero and can be set by calling ReadReturns() or Write(). This
    // method is expected to be called (indirectly) by the code under test.
    uint64_t Read() {
        if (read_expectations_index_ >= read_expectations_.size()) {
            return last_value_;
        }

        MmioExpectation& exp = read_expectations_[read_expectations_index_++];
        if (exp.match == MmioExpectation::Match::kAny) {
            return last_value_;
        }

        return last_value_ = exp.value;
    }

    // Writes to the mocked register. This method is expected to be called (indirectly) by the code
    // under test.
    bool Write(uint64_t value) {
        BEGIN_HELPER;

        last_value_ = value;

        if (write_expectations_index_ >= write_expectations_.size()) {
            return true;
        }

        MmioExpectation& exp = write_expectations_[write_expectations_index_++];
        if (exp.match == MmioExpectation::Match::kAny) {
            return true;
        }

        EXPECT_EQ(exp.value, value);
        END_HELPER;
    }

    // Matches a register read and returns the specified value.
    MockMmioReg& ExpectRead(uint64_t value) {
        read_expectations_.push_back(MmioExpectation{
            .match = MmioExpectation::Match::kEqual,
            .value = value
        });

        return *this;
    }

    // Matches a register read and returns the default value.
    MockMmioReg& ExpectRead() {
        read_expectations_.push_back(MmioExpectation{
            .match = MmioExpectation::Match::kAny,
            .value = 0
        });

        return *this;
    }

    // Sets the default register read value.
    MockMmioReg& ReadReturns(uint64_t value) {
        last_value_ = value;
        return *this;
    }

    // Matches a register write with the specified value.
    MockMmioReg& ExpectWrite(uint64_t value) {
        write_expectations_.push_back(MmioExpectation{
            .match = MmioExpectation::Match::kEqual,
            .value = value
        });

        return *this;
    }

    // Matches any register write.
    MockMmioReg& ExpectWrite() {
        write_expectations_.push_back(MmioExpectation{
            .match = MmioExpectation::Match::kAny,
            .value = 0
        });

        return *this;
    }

    // Removes and ignores all expectations and resets the default read value.
    void Clear() {
        last_value_ = 0;

        read_expectations_index_ = 0;
        while (read_expectations_.size() > 0) {
            read_expectations_.pop_back();
        }

        write_expectations_index_ = 0;
        while (write_expectations_.size() > 0) {
            write_expectations_.pop_back();
        }
    }

    // Removes all expectations and resets the default value. The presence of any outstanding
    // expectations causes a test failure.
    bool VerifyAndClear() {
        BEGIN_HELPER;
        EXPECT_GE(read_expectations_index_, read_expectations_.size());
        EXPECT_GE(write_expectations_index_, write_expectations_.size());
        Clear();
        END_HELPER;
    }

private:
    struct MmioExpectation {
        enum class Match { kEqual, kAny } match;
        uint64_t value;
    };

    uint64_t last_value_ = 0;

    size_t read_expectations_index_ = 0;
    fbl::Vector<MmioExpectation> read_expectations_;

    size_t write_expectations_index_ = 0;
    fbl::Vector<MmioExpectation> write_expectations_;
};

// Represents an array of MockMmioReg objects.
class MockMmioRegRegion {
public:
    // Constructs a MockMmioRegRegion backed by the given array. reg_size is the size of each
    // register in bytes, reg_count is the total size of the region in bytes. Ownership of mock_regs
    // is not transferred.
    MockMmioRegRegion(MockMmioReg* mock_regs, size_t reg_size, size_t reg_count)
        : mock_regs_(mock_regs), reg_size_(reg_size), reg_count_(reg_count) {
        ZX_ASSERT(reg_size_ > 0);
    }

    // Accesses the MockMmioReg at the given offset. Note that this is the _offset_, not the
    // _index_.
    MockMmioReg& operator[](size_t offset) {
        ZX_ASSERT(offset / reg_size_ < reg_count_);
        return mock_regs_[offset / reg_size_];
    }

    // Calls VerifyAndClear() on all MockMmioReg objects.
    void VerifyAll() {
        for (size_t i = 0; i < reg_count_; i++) {
            mock_regs_[i].VerifyAndClear();
        }
    }

    // Returns an mmio_buffer_t that can be used for constructing a ddk::MmioBuffer object.
    mmio_buffer_t GetMmioBuffer() {
        return mmio_buffer_t{
            .vaddr = this,
            .offset = 0,
            .size = reg_size_ * reg_count_,
            .vmo = ZX_HANDLE_INVALID
        };
    }

private:
    MockMmioReg* mock_regs_;
    const size_t reg_size_;
    const size_t reg_count_;
};

}  // namespace ddk_mock
