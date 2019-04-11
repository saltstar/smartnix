// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fidl/names.h"

namespace fidl {

namespace {

const char* NameNullability(types::Nullability nullability) {
    switch (nullability) {
    case types::Nullability::kNullable:
        return "nullable";
    case types::Nullability::kNonnullable:
        return "nonnullable";
    }
}

std::string NameSize(uint64_t size) {
    if (size == std::numeric_limits<uint64_t>::max())
        return "unbounded";
    std::ostringstream name;
    name << size;
    return name.str();
}

} // namespace

std::string StringJoin(const std::vector<StringView>& strings, StringView separator) {
    std::string result;
    bool first = true;
    for (const auto& part : strings) {
        if (!first) {
            result += separator;
        }
        first = false;
        result += part;
    }
    return result;
}

std::string NamePrimitiveCType(types::PrimitiveSubtype subtype) {
    switch (subtype) {
    case types::PrimitiveSubtype::kInt8:
        return "int8_t";
    case types::PrimitiveSubtype::kInt16:
        return "int16_t";
    case types::PrimitiveSubtype::kInt32:
        return "int32_t";
    case types::PrimitiveSubtype::kInt64:
        return "int64_t";
    case types::PrimitiveSubtype::kUint8:
        return "uint8_t";
    case types::PrimitiveSubtype::kUint16:
        return "uint16_t";
    case types::PrimitiveSubtype::kUint32:
        return "uint32_t";
    case types::PrimitiveSubtype::kUint64:
        return "uint64_t";
    case types::PrimitiveSubtype::kBool:
        return "bool";
    case types::PrimitiveSubtype::kFloat32:
        return "float";
    case types::PrimitiveSubtype::kFloat64:
        return "double";
    }
}

std::string NamePrimitiveSubtype(types::PrimitiveSubtype subtype) {
    switch (subtype) {
    case types::PrimitiveSubtype::kInt8:
        return "int8";
    case types::PrimitiveSubtype::kInt16:
        return "int16";
    case types::PrimitiveSubtype::kInt32:
        return "int32";
    case types::PrimitiveSubtype::kInt64:
        return "int64";
    case types::PrimitiveSubtype::kUint8:
        return "uint8";
    case types::PrimitiveSubtype::kUint16:
        return "uint16";
    case types::PrimitiveSubtype::kUint32:
        return "uint32";
    case types::PrimitiveSubtype::kUint64:
        return "uint64";
    case types::PrimitiveSubtype::kBool:
        return "bool";
    case types::PrimitiveSubtype::kFloat32:
        return "float32";
    case types::PrimitiveSubtype::kFloat64:
        return "float64";
    }
}

std::string NamePrimitiveIntegerCConstantMacro(types::PrimitiveSubtype subtype) {
    switch (subtype) {
    case types::PrimitiveSubtype::kInt8:
        return "INT8_C";
    case types::PrimitiveSubtype::kInt16:
        return "INT16_C";
    case types::PrimitiveSubtype::kInt32:
        return "INT32_C";
    case types::PrimitiveSubtype::kInt64:
        return "INT64_C";
    case types::PrimitiveSubtype::kUint8:
        return "UINT8_C";
    case types::PrimitiveSubtype::kUint16:
        return "UINT16_C";
    case types::PrimitiveSubtype::kUint32:
        return "UINT32_C";
    case types::PrimitiveSubtype::kUint64:
        return "UINT64_C";
    case types::PrimitiveSubtype::kBool:
        assert(false && "Tried to generate an integer constant for a bool");
        return "";
    case types::PrimitiveSubtype::kFloat32:
    case types::PrimitiveSubtype::kFloat64:
        assert(false && "Tried to generate an integer constant for a float");
        return "";
    }
}

std::string NameHandleSubtype(types::HandleSubtype subtype) {
    switch (subtype) {
    case types::HandleSubtype::kHandle:
        return "handle";
    case types::HandleSubtype::kProcess:
        return "process";
    case types::HandleSubtype::kThread:
        return "thread";
    case types::HandleSubtype::kVmo:
        return "vmo";
    case types::HandleSubtype::kChannel:
        return "channel";
    case types::HandleSubtype::kEvent:
        return "event";
    case types::HandleSubtype::kPort:
        return "port";
    case types::HandleSubtype::kInterrupt:
        return "interrupt";
    case types::HandleSubtype::kLog:
        return "debuglog";
    case types::HandleSubtype::kSocket:
        return "socket";
    case types::HandleSubtype::kResource:
        return "resource";
    case types::HandleSubtype::kEventpair:
        return "eventpair";
    case types::HandleSubtype::kJob:
        return "job";
    case types::HandleSubtype::kVmar:
        return "vmar";
    case types::HandleSubtype::kFifo:
        return "fifo";
    case types::HandleSubtype::kGuest:
        return "guest";
    case types::HandleSubtype::kTimer:
        return "timer";
    case types::HandleSubtype::kBti:
        return "bti";
    case types::HandleSubtype::kProfile:
        return "profile";
    }
}

std::string NameRawLiteralKind(raw::Literal::Kind kind) {
    switch (kind) {
    case raw::Literal::Kind::kString:
        return "string";
    case raw::Literal::Kind::kNumeric:
        return "numeric";
    case raw::Literal::Kind::kTrue:
        return "true";
    case raw::Literal::Kind::kFalse:
        return "false";
    }
}

std::string NameFlatTypeKind(flat::Type::Kind kind) {
    switch (kind) {
    case flat::Type::Kind::kArray:
        return "array";
    case flat::Type::Kind::kVector:
        return "vector";
    case flat::Type::Kind::kString:
        return "string";
    case flat::Type::Kind::kHandle:
        return "handle";
    case flat::Type::Kind::kRequestHandle:
        return "request";
    case flat::Type::Kind::kPrimitive:
        return "primitive";
    case flat::Type::Kind::kIdentifier:
        return "identifier";
    }
}

std::string NameFlatConstantKind(flat::Constant::Kind kind) {
    switch (kind) {
    case flat::Constant::Kind::kIdentifier:
        return "identifier";
    case flat::Constant::Kind::kLiteral:
        return "literal";
    case flat::Constant::Kind::kSynthesized:
        return "synthesized";
    }
}

std::string NameHandleZXObjType(types::HandleSubtype subtype) {
    switch (subtype) {
    case types::HandleSubtype::kHandle:
        return "ZX_OBJ_TYPE_NONE";
    case types::HandleSubtype::kProcess:
        return "ZX_OBJ_TYPE_PROCESS";
    case types::HandleSubtype::kThread:
        return "ZX_OBJ_TYPE_THREAD";
    case types::HandleSubtype::kVmo:
        return "ZX_OBJ_TYPE_VMO";
    case types::HandleSubtype::kChannel:
        return "ZX_OBJ_TYPE_CHANNEL";
    case types::HandleSubtype::kEvent:
        return "ZX_OBJ_TYPE_EVENT";
    case types::HandleSubtype::kPort:
        return "ZX_OBJ_TYPE_PORT";
    case types::HandleSubtype::kInterrupt:
        return "ZX_OBJ_TYPE_INTERRUPT";
    case types::HandleSubtype::kLog:
        return "ZX_OBJ_TYPE_LOG";
    case types::HandleSubtype::kSocket:
        return "ZX_OBJ_TYPE_SOCKET";
    case types::HandleSubtype::kResource:
        return "ZX_OBJ_TYPE_RESOURCE";
    case types::HandleSubtype::kEventpair:
        return "ZX_OBJ_TYPE_EVENTPAIR";
    case types::HandleSubtype::kJob:
        return "ZX_OBJ_TYPE_JOB";
    case types::HandleSubtype::kVmar:
        return "ZX_OBJ_TYPE_VMAR";
    case types::HandleSubtype::kFifo:
        return "ZX_OBJ_TYPE_FIFO";
    case types::HandleSubtype::kGuest:
        return "ZX_OBJ_TYPE_GUEST";
    case types::HandleSubtype::kTimer:
        return "ZX_OBJ_TYPE_TIMER";
    case types::HandleSubtype::kBti:
        return "ZX_OBJ_TYPE_BTI";
    case types::HandleSubtype::kProfile:
        return "ZX_OBJ_TYPE_PROFILE";
    }
}

std::string NameUnionTag(StringView union_name, const flat::Union::Member& member) {
    return std::string(union_name) + "Tag_" + NameIdentifier(member.name);
}

std::string NameXUnionTag(StringView xunion_name, const flat::XUnion::Member& member) {
    return std::string(xunion_name) + "Tag_" + NameIdentifier(member.name);
}

std::string NameFlatConstant(const flat::Constant* constant) {
    switch (constant->kind) {
    case flat::Constant::Kind::kLiteral: {
        auto literal_constant = static_cast<const flat::LiteralConstant*>(constant);
        return literal_constant->literal->location().data();
    }
    case flat::Constant::Kind::kIdentifier: {
        auto identifier_constant = static_cast<const flat::IdentifierConstant*>(constant);
        return NameName(identifier_constant->name, ".", "/");
    }
    case flat::Constant::Kind::kSynthesized: {
        return std::string("synthesized constant");
    }
    } // switch
}

void NameFlatTypeHelper(std::ostringstream& buf, const flat::Type* type) {
    switch (type->kind) {
    case flat::Type::Kind::kArray: {
        auto array_type = static_cast<const flat::ArrayType*>(type);
        buf << "array<";
        NameFlatTypeHelper(buf, array_type->element_type.get());
        buf << ">";
        auto element_count = static_cast<const flat::Size&>(array_type->element_count->Value());
        if (element_count != flat::Size::Max()) {
            buf << ":";
            buf << element_count.value;
        }
        break;
    }
    case flat::Type::Kind::kVector: {
        auto vector_type = static_cast<const flat::VectorType*>(type);
        buf << "vector<";
        NameFlatTypeHelper(buf, vector_type->element_type.get());
        buf << ">";
        auto element_count = static_cast<const flat::Size&>(vector_type->element_count->Value());
        if (element_count != flat::Size::Max()) {
            buf << ":";
            buf << element_count.value;
        }
        break;
    }
    case flat::Type::Kind::kString: {
        auto string_type = static_cast<const flat::StringType*>(type);
        buf << "string";
        auto max_size = static_cast<const flat::Size&>(string_type->max_size->Value());
        if (max_size != flat::Size::Max()) {
            buf << ":";
            buf << max_size.value;
        }
        break;
    }
    case flat::Type::Kind::kHandle: {
        auto handle_type = static_cast<const flat::HandleType*>(type);
        buf << "handle";
        if (handle_type->subtype != types::HandleSubtype::kHandle) {
            buf << "<";
            buf << NameHandleSubtype(handle_type->subtype);
            buf << ">";
        }
        break;
    }
    case flat::Type::Kind::kRequestHandle: {
        auto request_handle_type = static_cast<const flat::RequestHandleType*>(type);
        buf << "request<";
        buf << NameName(request_handle_type->name, ".", "/");
        buf << ">";
        break;
    }
    case flat::Type::Kind::kPrimitive: {
        auto primitive_type = static_cast<const flat::PrimitiveType*>(type);
        buf << NamePrimitiveSubtype(primitive_type->subtype);
        break;
    }
    case flat::Type::Kind::kIdentifier: {
        auto identifier_type = static_cast<const flat::IdentifierType*>(type);
        buf << NameName(identifier_type->name, ".", "/");
        break;
    }
    } // switch
    if (type->nullability == types::Nullability::kNullable) {
        buf << "?";
    }
}

std::string NameFlatType(const flat::Type* type) {
    std::ostringstream buf;
    NameFlatTypeHelper(buf, type);
    return buf.str();
}

std::string NameFlatCType(const flat::Type* type, flat::Decl::Kind decl_kind) {
    for (;;) {
        switch (type->kind) {
        case flat::Type::Kind::kHandle:
        case flat::Type::Kind::kRequestHandle:
            return "zx_handle_t";

        case flat::Type::Kind::kVector:
            return "fidl_vector_t";
        case flat::Type::Kind::kString:
            return "fidl_string_t";

        case flat::Type::Kind::kPrimitive: {
            auto primitive_type = static_cast<const flat::PrimitiveType*>(type);
            return NamePrimitiveCType(primitive_type->subtype);
        }

        case flat::Type::Kind::kArray: {
            auto array_type = static_cast<const flat::ArrayType*>(type);
            type = array_type->element_type.get();
            continue;
        }

        case flat::Type::Kind::kIdentifier: {
            auto identifier_type = static_cast<const flat::IdentifierType*>(type);
            switch (decl_kind) {
            case flat::Decl::Kind::kConst:
            case flat::Decl::Kind::kEnum:
            case flat::Decl::Kind::kStruct:
            case flat::Decl::Kind::kTable:
            case flat::Decl::Kind::kUnion:
            case flat::Decl::Kind::kXUnion: {
                std::string name = NameName(identifier_type->name, "_", "_");
                if (identifier_type->nullability == types::Nullability::kNullable) {
                    name.push_back('*');
                }
                return name;
            }
            case flat::Decl::Kind::kInterface: {
                return "zx_handle_t";
            }
            default: {
                abort();
            }
            }
        }
        }
    }
}

std::string NameIdentifier(SourceLocation name) {
    // TODO(TO-704) C name escaping and ergonomics.
    return name.data();
}

std::string NameName(const flat::Name& name, StringView library_separator, StringView name_separator) {
    std::string compiled_name = LibraryName(name.library(), library_separator);
    compiled_name += name_separator;
    compiled_name += name.name_part();
    return compiled_name;
}

std::string NameLibrary(const std::vector<StringView>& library_name) {
    return StringJoin(library_name, ".");
}

std::string NameLibraryCHeader(const std::vector<StringView>& library_name) {
    return StringJoin(library_name, "/") + "/c/fidl.h";
}

std::string NameInterface(const flat::Interface& interface) {
    return NameName(interface.name, "_", "_");
}

std::string NameDiscoverable(const flat::Interface& interface) {
    return NameName(interface.name, ".", ".");
}

std::string NameMethod(StringView interface_name, const flat::Interface::Method& method) {
    return std::string(interface_name) + NameIdentifier(method.name);
}

std::string NameOrdinal(StringView method_name) {
    std::string ordinal_name(method_name);
    ordinal_name += "Ordinal";
    return ordinal_name;
}

// TODO: Remove post-FIDL-425
std::string NameGenOrdinal(StringView method_name) {
    std::string ordinal_name(method_name);
    ordinal_name += "GenOrdinal";
    return ordinal_name;
}

std::string NameMessage(StringView method_name, types::MessageKind kind) {
    std::string message_name(method_name);
    switch (kind) {
    case types::MessageKind::kRequest:
        message_name += "Request";
        break;
    case types::MessageKind::kResponse:
        message_name += "Response";
        break;
    case types::MessageKind::kEvent:
        message_name += "Event";
        break;
    }
    return message_name;
}

std::string NameTable(StringView type_name) {
    return std::string(type_name) + "Table";
}

std::string NamePointer(StringView name) {
    std::string pointer_name(name);
    pointer_name += "Pointer";
    return pointer_name;
}

std::string NameMembers(StringView name) {
    std::string members_name(name);
    members_name += "Members";
    return members_name;
}

std::string NameFields(StringView name) {
    std::string fields_name(name);
    fields_name += "Fields";
    return fields_name;
}

std::string NameCodedStruct(const flat::Struct* struct_decl) {
    return NameName(struct_decl->name, "_", "_");
}

std::string NameCodedTable(const flat::Table* table_decl) {
    return NameName(table_decl->name, "_", "_");
}

std::string NameCodedUnion(const flat::Union* union_decl) {
    return NameName(union_decl->name, "_", "_");
}

std::string NameCodedXUnion(const flat::XUnion* xunion_decl) {
    return NameName(xunion_decl->name, "_", "_");
}

std::string NameCodedHandle(types::HandleSubtype subtype, types::Nullability nullability) {
    std::string name("Handle");
    name += NameHandleSubtype(subtype);
    name += NameNullability(nullability);
    return name;
}

std::string NameCodedInterfaceHandle(StringView interface_name, types::Nullability nullability) {
    std::string name(interface_name);
    name += "Interface";
    name += NameNullability(nullability);
    return name;
}

std::string NameCodedRequestHandle(StringView interface_name, types::Nullability nullability) {
    std::string name(interface_name);
    name += "Request";
    name += NameNullability(nullability);
    return name;
}

std::string NameCodedArray(StringView element_name, uint64_t size) {
    std::string name("Array");
    name += element_name;
    name += NameSize(size);
    return name;
}

std::string NameCodedVector(StringView element_name, uint64_t max_size,
                            types::Nullability nullability) {
    std::string name("Vector");
    name += element_name;
    name += NameSize(max_size);
    name += NameNullability(nullability);
    return name;
}

std::string NameCodedString(uint64_t max_size, types::Nullability nullability) {
    std::string name("String");
    name += NameSize(max_size);
    name += NameNullability(nullability);
    return name;
}

} // namespace fidl
