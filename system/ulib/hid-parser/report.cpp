// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/type_support.h>

#include <hid-parser/report.h>
#include <hid-parser/units.h>

namespace hid {
namespace {

// This sign extends the |n_bits| of |data| and returns it as a full
// int32_t value. |n_bits| must be between [0, 31].
// Example: If the user had a 2's complement 5 bit number 0b11111 it
// would represent -1. To sign extend this to an int32_t the function
// would be called as SignExtendFromBits(0b11111, 5)
constexpr int32_t SignExtendFromNBits(uint32_t data, int32_t n_bits) {
    // Expression taken and simplified from:
    // http://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend

    // Zero out information about n_bits.
    data = data & ((1U << n_bits) - 1);
    // Do the sign extend.
    int32_t msb = 1U << (n_bits - 1);
    return static_cast<int32_t>((data ^ msb) - msb);
}

inline bool FieldFits(const Report& report, const Attributes& attr) {
    return (attr.offset + attr.bit_sz) <= (report.len * 8);
}

// Extracts bits from a byte and returns them.
// |begin| is the starting bit: it starts at 0 and counts from LSB to MSB.
// The bits are placed at the beginning of return value.
// Make sure when this is called that (|begin| + |count|) <= 8.
// Example: ExtractBitsFromByte(1b00010100, 2, 3) returns 1b101.
inline uint8_t ExtractBitsFromByte(uint8_t val, uint32_t begin, uint8_t count) {
    uint8_t mask = static_cast<uint8_t>((0xFF >> (8 - count)) << begin);
    return static_cast<uint8_t>((val & mask) >> begin);
}
} // namespace

#define MIN(a, b) (a) < (b) ? (a) : (b)
template <typename T>
bool ExtractUint(const Report& report, const hid::Attributes& attr, T* value_out) {
    static_assert(fbl::is_pod<T>::value, "not POD");
    if (attr.bit_sz > sizeof(T) * 8) {
        return false;
    }
    if (!FieldFits(report, attr)) {
        return false;
    }
    T val = 0;

    const uint32_t start_bit = attr.offset;
    const uint32_t end_bit = start_bit + attr.bit_sz;
    uint32_t index_bit = attr.offset;
    while (index_bit < end_bit) {
        uint8_t bits_till_byte_end = static_cast<uint8_t>(8u - (index_bit % 8u));
        uint8_t bit_count = static_cast<uint8_t>(MIN(bits_till_byte_end, end_bit - index_bit));

        uint8_t extracted_bits =
            ExtractBitsFromByte(report.data[index_bit / 8u], index_bit % 8u, bit_count);

        val = static_cast<T>(val | (extracted_bits << (index_bit - start_bit)));

        index_bit += bit_count;
    }

    *value_out = val;
    return true;
}
#undef MIN

bool ExtractUint(const Report& report, const hid::Attributes& attr, uint8_t* value_out) {
    return ExtractUint<uint8_t>(report, attr, value_out);
}

bool ExtractUint(const Report& report, const hid::Attributes& attr, uint16_t* value_out) {
    return ExtractUint<uint16_t>(report, attr, value_out);
}

bool ExtractUint(const Report& report, const hid::Attributes& attr, uint32_t* value_out) {
    return ExtractUint<uint32_t>(report, attr, value_out);
}

bool ExtractAsUnit(const Report& report, const hid::Attributes& attr, double* value_out) {
    if (value_out == nullptr) {
        return false;
    }

    uint32_t uint_out;
    bool ret = ExtractUint(report, attr, &uint_out);
    if (!ret) {
        return false;
    }

    // If the minimum value is less than zero, then the maximum
    // value and the value itself are an unsigned number. Otherwise they
    // are signed numbers.
    const int64_t logc_max =
        (attr.logc_mm.min < 0) ? attr.logc_mm.max : static_cast<uint32_t>(attr.logc_mm.max);
    int64_t phys_max =
        (attr.phys_mm.min < 0) ? attr.phys_mm.max : static_cast<uint32_t>(attr.phys_mm.max);
    double val = (attr.logc_mm.min < 0)
                    ? static_cast<double>(SignExtendFromNBits(uint_out, attr.bit_sz))
                    : uint_out;

    if (val < attr.logc_mm.min || val > attr.logc_mm.max) {
        return false;
    }

    int64_t phys_min = attr.phys_mm.min;
    if (phys_max == 0 && phys_min == 0) {
        phys_min = attr.logc_mm.min;
        phys_max = logc_max;
    }

    double resolution =
        static_cast<double>(logc_max - attr.logc_mm.min) / static_cast<double>(phys_max - phys_min);
    *value_out = val / resolution;

    return true;
}

bool ExtractWithUnit(const Report& report, const hid::Attributes& attr,
                     const Unit& unit_out, double* value_out) {
    double val = 0;
    if (!ExtractAsUnit(report, attr, &val)) {
        return false;
    }

    return unit::ConvertUnits(attr.unit, val, unit_out, value_out);
}

} // namespace hid
