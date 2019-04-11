// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_FLAT_AST_H_
#define ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_FLAT_AST_H_

#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include <lib/fit/function.h>

#include "attributes.h"
#include "error_reporter.h"
#include "raw_ast.h"
#include "type_shape.h"
#include "virtual_source_file.h"

namespace fidl {
namespace flat {

template <typename T>
struct PtrCompare {
    bool operator()(const T* left, const T* right) const { return *left < *right; }
};

struct Decl;
class Library;

bool HasSimpleLayout(const Decl* decl);

// This is needed (for now) to work around declaration order issues.
std::string LibraryName(const Library* library, StringView separator);

// Name represents a scope name, i.e. a name within the context of a library
// or in the 'global' context. Names either reference (or name) things which
// appear in source, or are synthesized by the compiler (e.g. an anonymous
// struct name).
struct Name {
    Name() {}

    Name(const Library* library, const SourceLocation name)
        : library_(library),
          name_from_source_(std::make_unique<SourceLocation>(name)) {}

    Name(const Library* library, const std::string& name)
        : library_(library),
          anonymous_name_(std::make_unique<std::string>(name)) {}

    Name(Name&&) = default;
    Name& operator=(Name&&) = default;

    bool is_anonymous() const { return name_from_source_ == nullptr; }
    const Library* library() const { return library_; }
    const SourceLocation& source_location() const {
        assert(!is_anonymous());
        return *name_from_source_.get();
    }
    const StringView name_part() const {
        if (is_anonymous())
            return *anonymous_name_.get();
        return name_from_source_->data();
    }

    bool operator==(const Name& other) const {
        // TODO(pascallouis): Why are we lenient, and allow a name comparison,
        // rather than require the more stricter pointer equality here?
        if (LibraryName(library_, ".") != LibraryName(other.library_, "."))
            return false;
        return name_part() == other.name_part();
    }
    bool operator!=(const Name& other) const { return !operator==(other); }

    bool operator<(const Name& other) const {
        if (LibraryName(library_, ".") != LibraryName(other.library_, "."))
            return LibraryName(library_, ".") < LibraryName(other.library_, ".");
        return name_part() < other.name_part();
    }

private:
    const Library* library_ = nullptr;
    std::unique_ptr<SourceLocation> name_from_source_;
    std::unique_ptr<std::string> anonymous_name_;
};

struct ConstantValue {
    virtual ~ConstantValue() {}

    enum struct Kind {
        kInt8,
        kInt16,
        kInt32,
        kInt64,
        kUint8,
        kUint16,
        kUint32,
        kUint64,
        kFloat32,
        kFloat64,
        kBool,
        kString,
    };

    virtual bool Convert(Kind kind, std::unique_ptr<ConstantValue>* out_value) const = 0;

    const Kind kind;

protected:
    explicit ConstantValue(Kind kind)
        : kind(kind) {}
};

template <typename ValueType>
struct NumericConstantValue : ConstantValue {
    static_assert(std::is_arithmetic<ValueType>::value && !std::is_same<ValueType, bool>::value,
                  "NumericConstantValue can only be used with a numeric ValueType!");

    NumericConstantValue(ValueType value)
        : ConstantValue(GetKind()), value(value) {}

    operator ValueType() const { return value; }

    friend bool operator==(const NumericConstantValue<ValueType>& l,
                           const NumericConstantValue<ValueType>& r) {
        return l.value == r.value;
    }

    friend bool operator<(const NumericConstantValue<ValueType>& l,
                          const NumericConstantValue<ValueType>& r) {
        return l.value < r.value;
    }

    friend bool operator>(const NumericConstantValue<ValueType>& l,
                          const NumericConstantValue<ValueType>& r) {
        return l.value > r.value;
    }

    friend bool operator!=(const NumericConstantValue<ValueType>& l,
                           const NumericConstantValue<ValueType>& r) {
        return l.value != r.value;
    }

    friend bool operator<=(const NumericConstantValue<ValueType>& l,
                           const NumericConstantValue<ValueType>& r) {
        return l.value <= r.value;
    }

    friend bool operator>=(const NumericConstantValue<ValueType>& l,
                           const NumericConstantValue<ValueType>& r) {
        return l.value >= r.value;
    }

    friend std::ostream& operator<<(std::ostream& os, const NumericConstantValue<ValueType>& v) {
        if constexpr (GetKind() == Kind::kInt8)
            os << static_cast<int>(v.value);
        else if constexpr (GetKind() == Kind::kUint8)
            os << static_cast<unsigned>(v.value);
        else
            os << v.value;
        return os;
    }

    virtual bool Convert(Kind kind, std::unique_ptr<ConstantValue>* out_value) const override {
        assert(out_value != nullptr);
        switch (kind) {
        case Kind::kInt8: {
            if (std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<int8_t>::lowest() ||
                value > std::numeric_limits<int8_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<int8_t>>(
                static_cast<int8_t>(value));
            return true;
        }
        case Kind::kInt16: {
            if (std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<int16_t>::lowest() ||
                value > std::numeric_limits<int16_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<int16_t>>(
                static_cast<int16_t>(value));
            return true;
        }
        case Kind::kInt32: {
            if (std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<int32_t>::lowest() ||
                value > std::numeric_limits<int32_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<int32_t>>(
                static_cast<int32_t>(value));
            return true;
        }
        case Kind::kInt64: {
            if (std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<int64_t>::lowest() ||
                value > std::numeric_limits<int64_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<int64_t>>(
                static_cast<int64_t>(value));
            return true;
        }
        case Kind::kUint8: {
            if (std::is_floating_point<ValueType>::value ||
                value < 0 || value > std::numeric_limits<uint8_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<uint8_t>>(
                static_cast<uint8_t>(value));
            return true;
        }
        case Kind::kUint16: {
            if (std::is_floating_point<ValueType>::value ||
                value < 0 || value > std::numeric_limits<uint16_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<uint16_t>>(
                static_cast<uint16_t>(value));
            return true;
        }
        case Kind::kUint32: {
            if (std::is_floating_point<ValueType>::value ||
                value < 0 || value > std::numeric_limits<uint32_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<uint32_t>>(
                static_cast<uint32_t>(value));
            return true;
        }
        case Kind::kUint64: {
            if (std::is_floating_point<ValueType>::value ||
                value < 0 || value > std::numeric_limits<uint64_t>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<uint64_t>>(
                static_cast<uint64_t>(value));
            return true;
        }
        case Kind::kFloat32: {
            if (!std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<float>::lowest() ||
                value > std::numeric_limits<float>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<float>>(static_cast<float>(value));
            return true;
        }
        case Kind::kFloat64: {
            if (!std::is_floating_point<ValueType>::value ||
                value < std::numeric_limits<double>::lowest() ||
                value > std::numeric_limits<double>::max()) {
                return false;
            }
            *out_value = std::make_unique<NumericConstantValue<double>>(static_cast<double>(value));
            return true;
        }
        case Kind::kString:
        case Kind::kBool:
            return false;
        }
    }

    static NumericConstantValue<ValueType> Min() {
        return NumericConstantValue<ValueType>(std::numeric_limits<ValueType>::lowest());
    }

    static NumericConstantValue<ValueType> Max() {
        return NumericConstantValue<ValueType>(std::numeric_limits<ValueType>::max());
    }

    ValueType value;

private:
    constexpr static Kind GetKind() {
        if constexpr (std::is_same_v<ValueType, uint64_t>)
            return Kind::kUint64;
        if constexpr (std::is_same_v<ValueType, int64_t>)
            return Kind::kInt64;
        if constexpr (std::is_same_v<ValueType, uint32_t>)
            return Kind::kUint32;
        if constexpr (std::is_same_v<ValueType, int32_t>)
            return Kind::kInt32;
        if constexpr (std::is_same_v<ValueType, uint16_t>)
            return Kind::kUint16;
        if constexpr (std::is_same_v<ValueType, int16_t>)
            return Kind::kInt16;
        if constexpr (std::is_same_v<ValueType, uint8_t>)
            return Kind::kUint8;
        if constexpr (std::is_same_v<ValueType, int8_t>)
            return Kind::kInt8;
        if constexpr (std::is_same_v<ValueType, double>)
            return Kind::kFloat64;
        if constexpr (std::is_same_v<ValueType, float>)
            return Kind::kFloat32;
    }
};

using Size = NumericConstantValue<uint32_t>;

struct BoolConstantValue : ConstantValue {
    BoolConstantValue(bool value)
        : ConstantValue(ConstantValue::Kind::kBool), value(value) {}

    operator bool() const { return value; }

    friend bool operator==(const BoolConstantValue& l, const BoolConstantValue& r) {
        return l.value == r.value;
    }

    friend bool operator!=(const BoolConstantValue& l, const BoolConstantValue& r) {
        return l.value != r.value;
    }

    friend std::ostream& operator<<(std::ostream& os, const BoolConstantValue& v) {
        os << v.value;
        return os;
    }

    virtual bool Convert(Kind kind, std::unique_ptr<ConstantValue>* out_value) const override {
        assert(out_value != nullptr);
        switch (kind) {
        case Kind::kBool:
            *out_value = std::make_unique<BoolConstantValue>(value);
            return true;
        default:
            return false;
        }
    }

    bool value;
};

struct StringConstantValue : ConstantValue {
    explicit StringConstantValue(StringView value)
        : ConstantValue(ConstantValue::Kind::kString), value(value) {}

    friend std::ostream& operator<<(std::ostream& os, const StringConstantValue& v) {
        os << v.value.data();
        return os;
    }

    virtual bool Convert(Kind kind, std::unique_ptr<ConstantValue>* out_value) const override {
        assert(out_value != nullptr);
        switch (kind) {
        case Kind::kString:
            *out_value = std::make_unique<StringConstantValue>(StringView(value));
            return true;
        default:
            return false;
        }
    }

    StringView value;
};

struct Constant {
    virtual ~Constant() {}

    enum struct Kind {
        kIdentifier,
        kLiteral,
        kSynthesized,
    };

    explicit Constant(Kind kind)
        : kind(kind), value_(nullptr) {}

    bool IsResolved() const { return value_ != nullptr; }

    void ResolveTo(std::unique_ptr<ConstantValue> value) {
        assert(value != nullptr);
        assert(!IsResolved() && "Constants should only be resolved once!");
        value_ = std::move(value);
    }

    const ConstantValue& Value() const {
        assert(IsResolved() && "Accessing the value of an unresolved Constant!");
        return *value_;
    }

    const Kind kind;

protected:
    std::unique_ptr<ConstantValue> value_;
};

struct IdentifierConstant : Constant {
    explicit IdentifierConstant(Name name)
        : Constant(Kind::kIdentifier), name(std::move(name)) {}

    Name name;
};

struct LiteralConstant : Constant {
    explicit LiteralConstant(std::unique_ptr<raw::Literal> literal)
        : Constant(Kind::kLiteral), literal(std::move(literal)) {}

    std::unique_ptr<raw::Literal> literal;
};

struct SynthesizedConstant : Constant {
    explicit SynthesizedConstant(std::unique_ptr<ConstantValue> value)
        : Constant(Kind::kSynthesized) {
        ResolveTo(std::move(value));
    }
};

struct Decl {
    virtual ~Decl() {}

    enum struct Kind {
        kConst,
        kEnum,
        kInterface,
        kStruct,
        kTable,
        kUnion,
        kXUnion,
    };

    Decl(Kind kind, std::unique_ptr<raw::AttributeList> attributes, Name name)
        : kind(kind), attributes(std::move(attributes)), name(std::move(name)) {}

    const Kind kind;

    std::unique_ptr<raw::AttributeList> attributes;
    const Name name;

    bool HasAttribute(fidl::StringView name) const;
    fidl::StringView GetAttribute(fidl::StringView name) const;
    std::string GetName() const;

    bool compiling = false;
    bool compiled = false;
};

struct Type {
    virtual ~Type() {}

    enum struct Kind {
        kArray,
        kVector,
        kString,
        kHandle,
        kRequestHandle,
        kPrimitive,
        kIdentifier,
    };

    explicit Type(Kind kind, uint32_t size, types::Nullability nullability)
        : kind(kind), size(size), nullability(nullability) {}

    const Kind kind;
    // Set at construction time for most types. Identifier types get
    // this set later, during compilation.
    uint32_t size;
    const types::Nullability nullability;

    // Comparison helper object.
    class Comparison {
    public:
        Comparison() = default;
        template <class T>
        Comparison Compare(const T& a, const T& b) const {
            if (result_ != 0)
                return Comparison(result_);
            if (a < b)
                return Comparison(-1);
            if (b < a)
                return Comparison(1);
            return Comparison(0);
        }

        bool IsLessThan() const {
            return result_ < 0;
        }

    private:
        Comparison(int result)
            : result_(result) {}
        const int result_ = 0;
    };

    bool operator<(const Type& other) const {
        if (kind != other.kind)
            return kind < other.kind;
        return Compare(other).IsLessThan();
    }

    // Compare this object against 'other'.
    // It's guaranteed that this->kind == other.kind.
    // Return <0 if *this < other, ==0 if *this == other, and >0 if *this > other.
    // Derived types should override this, but also call this implementation.
    virtual Comparison Compare(const Type& other) const {
        assert(kind == other.kind);
        return Comparison()
            .Compare(nullability, other.nullability);
    }
};

struct ArrayType : public Type {
    ArrayType(SourceLocation name, std::unique_ptr<Type> element_type,
              std::unique_ptr<Constant> element_count)
        : Type(Kind::kArray, 0u, types::Nullability::kNonnullable), name(name),
          element_type(std::move(element_type)),
          element_count(std::move(element_count)) {}

    SourceLocation name;
    std::unique_ptr<Type> element_type;
    std::unique_ptr<Constant> element_count;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const ArrayType&>(other);
        return Type::Compare(o)
            .Compare(static_cast<const Size&>(element_count->Value()).value,
                     static_cast<const Size&>(o.element_count->Value()).value)
            .Compare(*element_type, *o.element_type);
    }
};

struct VectorType : public Type {
    VectorType(SourceLocation name, std::unique_ptr<Type> element_type,
               std::unique_ptr<Constant> element_count, types::Nullability nullability)
        : Type(Kind::kVector, 16u, nullability), name(name), element_type(std::move(element_type)),
          element_count(std::move(element_count)) {}

    SourceLocation name;
    std::unique_ptr<Type> element_type;
    std::unique_ptr<Constant> element_count;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const VectorType&>(other);
        return Type::Compare(o)
            .Compare(static_cast<const Size&>(element_count->Value()).value,
                     static_cast<const Size&>(o.element_count->Value()).value)
            .Compare(*element_type, *o.element_type);
    }
};

struct StringType : public Type {
    StringType(SourceLocation name, std::unique_ptr<Constant> max_size,
               types::Nullability nullability)
        : Type(Kind::kString, 16u, nullability), name(name), max_size(std::move(max_size)) {}

    SourceLocation name;
    std::unique_ptr<Constant> max_size;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const StringType&>(other);
        return Type::Compare(o)
            .Compare(static_cast<const Size&>(max_size->Value()).value,
                     static_cast<const Size&>(o.max_size->Value()).value);
    }
};

struct HandleType : public Type {
    HandleType(types::HandleSubtype subtype, types::Nullability nullability)
        : Type(Kind::kHandle, 4u, nullability), subtype(subtype) {}

    const types::HandleSubtype subtype;

    Comparison Compare(const Type& other) const override {
        const auto& o = *static_cast<const HandleType*>(&other);
        return Type::Compare(o)
            .Compare(subtype, o.subtype);
    }
};

struct RequestHandleType : public Type {
    RequestHandleType(Name name, types::Nullability nullability)
        : Type(Kind::kRequestHandle, 4u, nullability), name(std::move(name)) {}

    Name name;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const RequestHandleType&>(other);
        return Type::Compare(o)
            .Compare(name, o.name);
    }
};

struct PrimitiveType : public Type {
    static uint32_t SubtypeSize(types::PrimitiveSubtype subtype) {
        switch (subtype) {
        case types::PrimitiveSubtype::kBool:
        case types::PrimitiveSubtype::kInt8:
        case types::PrimitiveSubtype::kUint8:
            return 1u;

        case types::PrimitiveSubtype::kInt16:
        case types::PrimitiveSubtype::kUint16:
            return 2u;

        case types::PrimitiveSubtype::kFloat32:
        case types::PrimitiveSubtype::kInt32:
        case types::PrimitiveSubtype::kUint32:
            return 4u;

        case types::PrimitiveSubtype::kFloat64:
        case types::PrimitiveSubtype::kInt64:
        case types::PrimitiveSubtype::kUint64:
            return 8u;
        }
    }

    explicit PrimitiveType(types::PrimitiveSubtype subtype)
        : Type(Kind::kPrimitive, SubtypeSize(subtype), types::Nullability::kNonnullable),
          subtype(subtype) {}

    types::PrimitiveSubtype subtype;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const PrimitiveType&>(other);
        return Type::Compare(o)
            .Compare(subtype, o.subtype);
    }
};

struct IdentifierType : public Type {
    IdentifierType(Name name, types::Nullability nullability)
        : Type(Kind::kIdentifier, 0u, nullability), name(std::move(name)) {}

    Name name;

    Comparison Compare(const Type& other) const override {
        const auto& o = static_cast<const RequestHandleType&>(other);
        return Type::Compare(o)
            .Compare(name, o.name);
    }
};

struct Using {
    Using(Name name, const PrimitiveType* type)
        : name(std::move(name)), type(type) {}

    const Name name;
    const PrimitiveType* type;
};

struct Const : public Decl {
    Const(std::unique_ptr<raw::AttributeList> attributes, Name name, std::unique_ptr<Type> type,
          std::unique_ptr<Constant> value)
        : Decl(Kind::kConst, std::move(attributes), std::move(name)), type(std::move(type)),
          value(std::move(value)) {}
    std::unique_ptr<Type> type;
    std::unique_ptr<Constant> value;
};

struct Enum : public Decl {
    struct Member {
        Member(SourceLocation name, std::unique_ptr<Constant> value, std::unique_ptr<raw::AttributeList> attributes)
            : name(name), value(std::move(value)), attributes(std::move(attributes)) {}
        SourceLocation name;
        std::unique_ptr<Constant> value;
        std::unique_ptr<raw::AttributeList> attributes;
    };

    Enum(std::unique_ptr<raw::AttributeList> attributes, Name name,
         std::unique_ptr<raw::IdentifierType> maybe_subtype,
         std::vector<Member> members)
        : Decl(Kind::kEnum, std::move(attributes), std::move(name)),
          maybe_subtype(std::move(maybe_subtype)),
          members(std::move(members)) {}

    // Set during construction.
    std::unique_ptr<raw::IdentifierType> maybe_subtype;
    std::vector<Member> members;

    // Set during compilation.
    const PrimitiveType* type = nullptr;
    TypeShape typeshape;
};

struct Struct : public Decl {
    struct Member {
        Member(std::unique_ptr<Type> type, SourceLocation name,
               std::unique_ptr<Constant> maybe_default_value,
               std::unique_ptr<raw::AttributeList> attributes)
            : type(std::move(type)), name(std::move(name)),
              maybe_default_value(std::move(maybe_default_value)),
              attributes(std::move(attributes)) {}
        std::unique_ptr<Type> type;
        SourceLocation name;
        std::unique_ptr<Constant> maybe_default_value;
        std::unique_ptr<raw::AttributeList> attributes;
        FieldShape fieldshape;
    };

    Struct(std::unique_ptr<raw::AttributeList> attributes, Name name,
           std::vector<Member> members, bool anonymous = false)
        : Decl(Kind::kStruct, std::move(attributes), std::move(name)),
          members(std::move(members)), anonymous(anonymous) {
    }

    std::vector<Member> members;
    const bool anonymous;
    TypeShape typeshape;
    bool recursive = false;
};

struct Table : public Decl {
    struct Member {
        Member(std::unique_ptr<raw::Ordinal> ordinal, std::unique_ptr<Type> type, SourceLocation name,
               std::unique_ptr<Constant> maybe_default_value,
               std::unique_ptr<raw::AttributeList> attributes)
            : ordinal(std::move(ordinal)),
              maybe_used(std::make_unique<Used>(std::move(type), std::move(name),
                                                std::move(maybe_default_value),
                                                std::move(attributes))) {}
        Member(std::unique_ptr<raw::Ordinal> ordinal)
            : ordinal(std::move(ordinal)) {}
        std::unique_ptr<raw::Ordinal> ordinal;
        struct Used {
            Used(std::unique_ptr<Type> type, SourceLocation name,
                 std::unique_ptr<Constant> maybe_default_value,
                 std::unique_ptr<raw::AttributeList> attributes)
                : type(std::move(type)), name(std::move(name)),
                  maybe_default_value(std::move(maybe_default_value)),
                  attributes(std::move(attributes)) {}
            std::unique_ptr<Type> type;
            SourceLocation name;
            std::unique_ptr<Constant> maybe_default_value;
            std::unique_ptr<raw::AttributeList> attributes;
            TypeShape typeshape;
        };
        std::unique_ptr<Used> maybe_used;
    };

    Table(std::unique_ptr<raw::AttributeList> attributes, Name name, std::vector<Member> members)
        : Decl(Kind::kTable, std::move(attributes), std::move(name)), members(std::move(members)) {
    }

    std::vector<Member> members;
    TypeShape typeshape;
    bool recursive = false;
};

struct Union : public Decl {
    struct Member {
        Member(std::unique_ptr<Type> type, SourceLocation name, std::unique_ptr<raw::AttributeList> attributes)
            : type(std::move(type)), name(std::move(name)), attributes(std::move(attributes)) {}
        std::unique_ptr<Type> type;
        SourceLocation name;
        std::unique_ptr<raw::AttributeList> attributes;
        FieldShape fieldshape;
    };

    Union(std::unique_ptr<raw::AttributeList> attributes, Name name, std::vector<Member> members)
        : Decl(Kind::kUnion, std::move(attributes), std::move(name)), members(std::move(members)) {}

    std::vector<Member> members;
    TypeShape typeshape;
    // The offset of each of the union members is the same, so store
    // it here as well.
    FieldShape membershape;
    bool recursive = false;
};

struct XUnion : public Decl {
    struct Member {
        Member(std::unique_ptr<raw::Ordinal> ordinal, std::unique_ptr<Type> type, SourceLocation name, std::unique_ptr<raw::AttributeList> attributes)
            : ordinal(std::move(ordinal)), type(std::move(type)), name(std::move(name)), attributes(std::move(attributes)) {}
        std::unique_ptr<raw::Ordinal> ordinal;
        std::unique_ptr<Type> type;
        SourceLocation name;
        std::unique_ptr<raw::AttributeList> attributes;
        FieldShape fieldshape;
    };

    XUnion(std::unique_ptr<raw::AttributeList> attributes, Name name, std::vector<Member> members)
        : Decl(Kind::kXUnion, std::move(attributes), std::move(name)), members(std::move(members)) {}

    std::vector<Member> members;
    TypeShape typeshape;
    bool recursive = false;
};

struct Interface : public Decl {
    struct Method {
        Method(Method&&) = default;
        Method& operator=(Method&&) = default;

        Method(std::unique_ptr<raw::AttributeList> attributes,
               std::unique_ptr<raw::Ordinal> ordinal,
               std::unique_ptr<raw::Ordinal> generated_ordinal,
               SourceLocation name,
               Struct* maybe_request,
               Struct* maybe_response)
            : attributes(std::move(attributes)),
              ordinal(std::move(ordinal)),
              generated_ordinal(std::move(generated_ordinal)),
              name(std::move(name)),
              maybe_request(maybe_request),
              maybe_response(maybe_response) {
            assert(this->maybe_request != nullptr || this->maybe_response != nullptr);
        }

        std::unique_ptr<raw::AttributeList> attributes;
        std::unique_ptr<raw::Ordinal> ordinal;
        std::unique_ptr<raw::Ordinal> generated_ordinal;
        SourceLocation name;
        Struct* maybe_request;
        Struct* maybe_response;
    };

    Interface(std::unique_ptr<raw::AttributeList> attributes, Name name,
              std::vector<Name> superinterfaces, std::vector<Method> methods)
        : Decl(Kind::kInterface, std::move(attributes), std::move(name)),
          superinterfaces(std::move(superinterfaces)), methods(std::move(methods)) {}

    std::vector<Name> superinterfaces;
    std::vector<Method> methods;
    // Pointers here are set after superinterfaces are compiled, and
    // are owned by the correspending superinterface.
    std::vector<const Method*> all_methods;
};

// Typespace provides builders for all types (e.g. array, vector, string), and
// ensures canonicalization, i.e. the same type is represented by one object,
// shared amongst all uses of said type. For instance, while the text
// `vector<uint8>:7` may appear multiple times in source, these all indicate
// the same type.
class Typespace {
public:
    enum LookupMode {
        kNoForwardReferences,
        kAllowForwardReferences,
    };

    // Lookup looks up a type by its name, or alias, and returns the type
    // which corresponds, whilst respecting the nullability qualifier.
    // If the type exists, returns the type, otherwise returns nullptr.
    //
    // If forward references are allowed, a placeholder type is returned
    // if the type does not exist yet. This allows forward references to be
    // made, and later resolved.
    //
    // See also RegisterDecl, RegisterAlias, and
    // EnsureAllTypesHaveBeenResolved.
    Type* Lookup(const flat::Name& name, types::Nullability nullability,
                 LookupMode lookup_mode);

    // ArrayType creates or returns an array type for the |element_type|,
    // and |element_count|.
    ArrayType* ArrayType(Type* element_type, Size element_count);

    // VectorType creates or returns a vector type for the |element_type|,
    // |max_size|, and |nullability|.
    VectorType* VectorType(Type* element_type, Size max_size,
                           types::Nullability nullability);

    // StringType creates or returns a string type for |max_size|,
    // and |nullability|.
    StringType* StringType(Size max_size, types::Nullability nullability);

    // HandleType creates or returns a handle type for the |subtype|,
    // and |nullability|.
    HandleType* HandleType(types::HandleSubtype subtype, types::Nullability nullability);

    // RequestType creates or returns an interface request type for the |name|,
    // and |nullability|.
    RequestHandleType* RequestType(Name name, types::Nullability nullability);

    // RegisterDecl registers a declaration under a specific name. Registering
    // a declaration resolves all prior references to this named type, i.e.
    // earlier lookups from forward references.
    // This method returns true if there was no name conflict, false otherwise.
    bool RegisterDecl(Name name, Decl* decl);

    // RegisterAlias registers a type alias, such as `using int = int32;`. An
    // aliased type can be referenced by either name, with no behavioral
    // difference.
    // This method returns true if there was no name conflict, false otherwise.
    bool RegisterAlias(Name name, Name alias);

    // EnsureAllTypesHaveBeenResolved checks that all forward references have
    // been propertly resolved.
    bool EnsureAllTypesHaveBeenResolved();

    // BoostrapRootTypes creates a instance with all primitive types. It is
    // meant to be used as the top-level types lookup mechanism, providing
    // definitional meaning to names such as `int64`, or `bool`.
    static Typespace RootTypes() {
        Typespace root_typespace;
        ByName primitive_types;
        auto add = [&](const std::string name, types::PrimitiveSubtype subtype) {
            root_typespace.owned_names_.push_back(
                std::make_unique<Name>(nullptr, name));
            primitive_types.emplace(
                root_typespace.owned_names_.back().get(),
                std::make_unique<PrimitiveType>(subtype));
        };

        add("bool", types::PrimitiveSubtype::kBool);

        add("int8", types::PrimitiveSubtype::kInt8);
        add("int16", types::PrimitiveSubtype::kInt16);
        add("int32", types::PrimitiveSubtype::kInt32);
        add("int64", types::PrimitiveSubtype::kInt64);
        add("uint8", types::PrimitiveSubtype::kUint8);
        add("uint16", types::PrimitiveSubtype::kUint16);
        add("uint32", types::PrimitiveSubtype::kUint32);
        add("uint64", types::PrimitiveSubtype::kUint64);

        add("float32", types::PrimitiveSubtype::kFloat32);
        add("float64", types::PrimitiveSubtype::kFloat64);

        root_typespace.named_types_.emplace(
            types::Nullability::kNonnullable, std::move(primitive_types));
        return root_typespace;
    }

private:
    // CreateForwardDeclaredType creates a placeholder for the named type. As
    // the name suggest, this type must be resolved by registering a named type
    // for it.
    //
    // See also RegisterDecl, RegisterAlias, and
    // EnsureAllTypesHaveBeenResolved.
    Type* CreateForwardDeclaredType(const flat::Name& name, types::Nullability nullability);

    struct cmpName {
        bool operator()(const flat::Name* a, const flat::Name* b) const {
            return *a < *b;
        }
    };

    using BySize = std::map<const Size, std::unique_ptr<Type>>;
    using ByNullability = std::map<types::Nullability, std::unique_ptr<Type>>;
    using ByName = std::map<const flat::Name*, std::unique_ptr<Type>, cmpName>;
    using ByHandleSubtype = std::map<types::HandleSubtype, std::unique_ptr<Type>>;

    using ByNullabilityThenBySize = std::map<types::Nullability, BySize>;
    using ByNullabilityThenByName = std::map<types::Nullability, ByName>;
    using ByNullabilityThenByHandleSubtype = std::map<types::Nullability, ByHandleSubtype>;

    std::map<const Type*, BySize> array_types_;
    std::map<const Type*, ByNullabilityThenBySize> vector_types_;
    ByNullabilityThenBySize string_types_;
    ByNullabilityThenByHandleSubtype handle_types_;
    ByNullabilityThenByName request_types_;
    ByNullabilityThenByName named_types_;
    ByName aliases_;

    std::vector<std::unique_ptr<Name>> owned_names_;
};

// AttributeSchema defines a schema for attributes. This includes:
// - The allowed placement of an attribute (e.g. on a method, on a struct
//   declaration);
// - The allowed values which an attribute can take.
// For attributes which may be placed on declarations (e.g. interface, struct,
// union, table), a schema may additionally include:
// - A constraint which must be met by the declaration.
class AttributeSchema {
public:
    using Constraint = fit::function<bool(ErrorReporter* error_reporter,
                                          const raw::Attribute* attribute,
                                          const Decl* decl)>;

    // Placement indicates the placement of an attribute, e.g. whether an
    // attribute is placed on an enum declaration, method, or union
    // member.
    enum class Placement {
        kConstDecl,
        kEnumDecl,
        kEnumMember,
        kInterfaceDecl,
        kLibrary,
        kMethod,
        kStructDecl,
        kStructMember,
        kTableDecl,
        kTableMember,
        kUnionDecl,
        kUnionMember,
        kXUnionDecl,
        kXUnionMember,
    };

    AttributeSchema(const std::set<Placement>& allowed_placements,
                    const std::set<std::string> allowed_values,
                    Constraint constraint = NoOpConstraint);

    AttributeSchema(AttributeSchema&& schema) = default;

    void ValidatePlacement(ErrorReporter* error_reporter,
                           const raw::Attribute* attribute,
                           Placement placement) const;

    void ValidateValue(ErrorReporter* error_reporter,
                       const raw::Attribute* attribute) const;

    void ValidateConstraint(ErrorReporter* error_reporter,
                            const raw::Attribute* attribute,
                            const Decl* decl) const;

private:
    static bool NoOpConstraint(ErrorReporter* error_reporter,
                               const raw::Attribute* attribute,
                               const Decl* decl) {
        return true;
    }

    std::set<Placement> allowed_placements_;
    std::set<std::string> allowed_values_;
    Constraint constraint_;
};

class Libraries {
public:
    Libraries();

    // Insert |library|.
    bool Insert(std::unique_ptr<Library> library);

    // Lookup a library by its |library_name|.
    bool Lookup(const std::vector<StringView>& library_name,
                Library** out_library) const;

    void AddAttributeSchema(const std::string& name, AttributeSchema schema) {
        auto iter = attribute_schemas_.emplace(name, std::move(schema));
        assert(iter.second && "do not add schemas twice");
    }

    const AttributeSchema* RetrieveAttributeSchema(ErrorReporter* error_reporter,
                                                   const raw::Attribute* attribute) const;

private:
    std::map<std::vector<StringView>, std::unique_ptr<Library>> all_libraries_;
    std::map<std::string, AttributeSchema> attribute_schemas_;
};

class Dependencies {
public:
    // Register a dependency to a library. The newly recorded dependent library
    // will be referenced by its name, and may also be optionally be referenced
    // by an alias.
    bool Register(StringView filename, Library* dep_library,
                  const std::unique_ptr<raw::Identifier>& maybe_alias);

    // Lookup a dependent library by |filename| and |name|.
    bool Lookup(StringView filename, const std::vector<StringView>& name,
                Library** out_library);

    const std::set<Library*>& dependencies() const { return dependencies_aggregate_; };

private:
    bool InsertByName(StringView filename, const std::vector<StringView>& name,
                      Library* library);

    using ByName = std::map<std::vector<StringView>, Library*>;
    using ByFilename = std::map<std::string, std::unique_ptr<ByName>>;

    ByFilename dependencies_;
    std::set<Library*> dependencies_aggregate_;
};

class Library {
public:
    Library(const Libraries* all_libraries, ErrorReporter* error_reporter, Typespace* typespace)
        : all_libraries_(all_libraries), error_reporter_(error_reporter), typespace_(typespace) {}

    bool ConsumeFile(std::unique_ptr<raw::File> file);
    bool Compile();

    const std::vector<StringView>& name() const { return library_name_; }
    const std::vector<std::string>& errors() const { return error_reporter_->errors(); }

private:
    bool Fail(StringView message);
    bool Fail(const SourceLocation& location, StringView message);
    bool Fail(const Name& name, StringView message) {
        if (name.is_anonymous()) {
            return Fail(message);
        }
        return Fail(name.source_location(), message);
    }
    bool Fail(const Decl& decl, StringView message) { return Fail(decl.name, message); }

    void ValidateAttributesPlacement(AttributeSchema::Placement placement,
                                     const raw::AttributeList* attributes);
    void ValidateAttributesConstraints(const Decl* decl,
                                       const raw::AttributeList* attributes);

    Name NextAnonymousName();
    Name GeneratedName(const std::string& name);
    Name DerivedName(const std::vector<StringView>& components);

    bool CompileCompoundIdentifier(const raw::CompoundIdentifier* compound_identifier,
                                   SourceLocation location, Name* out_name);
    void RegisterConst(Const* decl);
    bool RegisterDecl(Decl* decl);

    bool ConsumeConstant(std::unique_ptr<raw::Constant> raw_constant, SourceLocation location,
                         std::unique_ptr<Constant>* out_constant);
    bool ConsumeType(std::unique_ptr<raw::Type> raw_type, SourceLocation location,
                     std::unique_ptr<Type>* out_type);

    bool ConsumeUsing(std::unique_ptr<raw::Using> using_directive);
    bool ConsumeTypeAlias(std::unique_ptr<raw::Using> using_directive);
    bool ConsumeConstDeclaration(std::unique_ptr<raw::ConstDeclaration> const_declaration);
    bool ConsumeEnumDeclaration(std::unique_ptr<raw::EnumDeclaration> enum_declaration);
    bool
    ConsumeInterfaceDeclaration(std::unique_ptr<raw::InterfaceDeclaration> interface_declaration);
    bool ConsumeParameterList(Name name, std::unique_ptr<raw::ParameterList> parameter_list,
                              bool anonymous, Struct** out_struct_decl);
    bool CreateMethodResult(const Name& interface_name, raw::InterfaceMethod* method, Struct* in_response, Struct** out_response);
    bool ConsumeStructDeclaration(std::unique_ptr<raw::StructDeclaration> struct_declaration);
    bool ConsumeTableDeclaration(std::unique_ptr<raw::TableDeclaration> table_declaration);
    bool ConsumeUnionDeclaration(std::unique_ptr<raw::UnionDeclaration> union_declaration);
    bool ConsumeXUnionDeclaration(std::unique_ptr<raw::XUnionDeclaration> xunion_declaration);

    bool TypeCanBeConst(const Type* type);
    const Type* TypeResolve(const Type* type);
    bool TypeIsConvertibleTo(const Type* from_type, const Type* to_type);
    std::unique_ptr<IdentifierType> IdentifierTypeForDecl(const Decl* decl, types::Nullability nullability);

    // Given a const declaration of the form
    //     const type foo = name;
    // return the declaration corresponding to name.
    Decl* LookupConstant(const Type* type, const Name& name);

    // Given a name, checks whether that name corresponds to a primitive type. If
    // so, returns the type. Otherwise, returns nullptr.
    const PrimitiveType* LookupPrimitiveType(const Name& name) const;

    // Given a name, checks whether that name corresponds to a type alias. If
    // so, returns the type. Otherwise, returns nullptr.
    const PrimitiveType* LookupTypeAlias(const Name& name) const;

    // Returns nullptr when |type| does not correspond directly to a
    // declaration. For example, if |type| refers to int32 or if it is
    // a struct pointer, this will return null. If it is a struct, it
    // will return a pointer to the declaration of the type.
    enum class LookupOption {
        kIgnoreNullable,
        kIncludeNullable,
    };
    Decl* LookupDeclByType(const flat::Type* type, LookupOption option) const;

    bool DeclDependencies(Decl* decl, std::set<Decl*>* out_edges);

    bool SortDeclarations();

    bool CompileLibraryName();

    bool CompileConst(Const* const_declaration);
    bool CompileEnum(Enum* enum_declaration);
    bool CompileInterface(Interface* interface_declaration);
    bool CompileStruct(Struct* struct_declaration);
    bool CompileTable(Table* table_declaration);
    bool CompileUnion(Union* union_declaration);
    bool CompileXUnion(XUnion* xunion_declaration);

    // Compiling a type both validates the type, and computes shape
    // information for the type. In particular, we validate that
    // optional identifier types refer to things that can in fact be
    // nullable (ie not enums).
    bool CompileArrayType(ArrayType* array_type, TypeShape* out_type_metadata);
    bool CompileVectorType(VectorType* vector_type, TypeShape* out_type_metadata);
    bool CompileStringType(StringType* string_type, TypeShape* out_type_metadata);
    bool CompileHandleType(HandleType* handle_type, TypeShape* out_type_metadata);
    bool CompileRequestHandleType(RequestHandleType* request_type, TypeShape* out_type_metadata);
    bool CompilePrimitiveType(PrimitiveType* primitive_type, TypeShape* out_type_metadata);
    bool CompileIdentifierType(IdentifierType* identifier_type, TypeShape* out_type_metadata);
    bool CompileType(Type* type, TypeShape* out_type_metadata);

    bool ResolveConstant(Constant* constant, const Type* type);
    bool ResolveIdentifierConstant(IdentifierConstant* identifier_constant, const Type* type);
    bool ResolveLiteralConstant(LiteralConstant* literal_constant, const Type* type);

    template <typename MemberType>
    bool ValidateEnumMembers(Enum* enum_decl);

public:
    // Returns nullptr when the |name| cannot be resolved to a
    // Name. Otherwise it returns the declaration.
    Decl* LookupDeclByName(const Name& name) const;

    template <typename NumericType>
    bool ParseNumericLiteral(const raw::NumericLiteral* literal, NumericType* out_value) const;

    bool HasAttribute(fidl::StringView name) const;

    const std::set<Library*>& dependencies() const;

    std::vector<StringView> library_name_;

    std::vector<std::unique_ptr<Using>> using_;
    std::vector<std::unique_ptr<Const>> const_declarations_;
    std::vector<std::unique_ptr<Enum>> enum_declarations_;
    std::vector<std::unique_ptr<Interface>> interface_declarations_;
    std::vector<std::unique_ptr<Struct>> struct_declarations_;
    std::vector<std::unique_ptr<Table>> table_declarations_;
    std::vector<std::unique_ptr<Union>> union_declarations_;
    std::vector<std::unique_ptr<XUnion>> xunion_declarations_;

    // All Decl pointers here are non-null and are owned by the
    // various foo_declarations_.
    std::vector<Decl*> declaration_order_;

private:
    const PrimitiveType kSizeType = PrimitiveType(types::PrimitiveSubtype::kUint32);

    std::unique_ptr<raw::AttributeList> attributes_;

    Dependencies dependencies_;
    const Libraries* all_libraries_;

    // All Name, Constant, Using, and Decl pointers here are non-null and are
    // owned by the various foo_declarations_.
    std::map<const Name*, Using*, PtrCompare<Name>> type_aliases_;
    std::map<const Name*, Decl*, PtrCompare<Name>> declarations_;
    std::map<const Name*, Const*, PtrCompare<Name>> constants_;

    ErrorReporter* error_reporter_;
    Typespace* typespace_;

    uint32_t anon_counter_ = 0;

    VirtualSourceFile generated_source_file_{"generated"};
};

} // namespace flat
} // namespace fidl

#endif // ZIRCON_SYSTEM_HOST_FIDL_INCLUDE_FIDL_FLAT_AST_H_
