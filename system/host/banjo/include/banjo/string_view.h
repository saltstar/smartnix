
#ifndef ZIRCON_SYSTEM_HOST_BANJO_INCLUDE_BANJO_STRING_VIEW_H_
#define ZIRCON_SYSTEM_HOST_BANJO_INCLUDE_BANJO_STRING_VIEW_H_

#include <stddef.h>
#include <string.h>

#include <string>

namespace banjo {

class StringView {
public:
    constexpr StringView() : data_(nullptr), size_(0u) {}
    StringView(const StringView& view) = default;
    constexpr StringView(const std::string& string) : StringView(string.data(), string.size()) {}
    constexpr StringView(const char* data, size_t size) : data_(data), size_(size) {}
    StringView(const char* string) : data_(string), size_(strlen(string)) {}

    operator std::string() const { return std::string(data(), size()); }

    StringView& operator=(const StringView& view) = default;

    constexpr char operator[](size_t index) const { return data_[index]; }

    constexpr const char* data() const { return data_; }
    constexpr size_t size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    const char* begin() const { return &data_[0]; }
    const char* end() const { return &data_[size_]; }

    bool operator==(StringView other) const {
        if (size() != other.size())
            return false;
        return !memcmp(data(), other.data(), size());
    }

    bool operator!=(StringView other) const { return !(*this == other); }

    bool operator<(StringView other) const {
        if (size() < other.size())
            return true;
        if (size() > other.size())
            return false;
        return memcmp(data(), other.data(), size()) < 0;
    }

private:
    const char* data_;
    size_t size_;
};

} // namespace banjo

#endif // ZIRCON_SYSTEM_HOST_BANJO_INCLUDE_BANJO_STRING_VIEW_H_
