// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_UTEST_FIDL_COMPILER_TEST_LIBRARY_H_
#define ZIRCON_SYSTEM_UTEST_FIDL_COMPILER_TEST_LIBRARY_H_

#include <fidl/flat_ast.h>
#include <fidl/json_generator.h>
#include <fidl/lexer.h>
#include <fidl/parser.h>
#include <fidl/source_file.h>

static fidl::SourceFile MakeSourceFile(const std::string& filename, const std::string& raw_source_code) {
    std::string source_code(raw_source_code);
    // NUL terminate the string.
    source_code.resize(source_code.size() + 1);
    return fidl::SourceFile(filename, source_code);
}

class SharedAmongstLibraries {
public:
    SharedAmongstLibraries() :
        typespace(fidl::flat::Typespace::RootTypes()) {}

    fidl::ErrorReporter error_reporter;
    fidl::flat::Typespace typespace;
    fidl::flat::Libraries all_libraries;
};

class TestLibrary {
public:
    TestLibrary(const std::string& raw_source_code)
        : TestLibrary("example.fidl", raw_source_code) {}

    TestLibrary(const std::string& filename, const std::string& raw_source_code)
        : TestLibrary(filename, raw_source_code, &owned_shared_) {}

    TestLibrary(const std::string& filename, const std::string& raw_source_code,
                SharedAmongstLibraries* shared)
        : error_reporter_(&shared->error_reporter),
          typespace_(&shared->typespace),
          all_libraries_(&shared->all_libraries),
          source_file_(MakeSourceFile(filename, raw_source_code)),
          lexer_(source_file_, error_reporter_),
          parser_(&lexer_, error_reporter_),
          library_(std::make_unique<fidl::flat::Library>(
              all_libraries_, error_reporter_, typespace_)) {}

    bool AddDependentLibrary(TestLibrary dependent_library) {
        return all_libraries_->Insert(std::move(dependent_library.library_));
    }

    void AddAttributeSchema(const std::string& name, fidl::flat::AttributeSchema schema) {
        all_libraries_->AddAttributeSchema(name, std::move(schema));
    }

    bool Parse(std::unique_ptr<fidl::raw::File>& ast_ptr) {
        ast_ptr.reset(parser_.Parse().release());
        return parser_.Ok();
    }

    bool Compile() {
        auto ast = parser_.Parse();
        return parser_.Ok() &&
               library_->ConsumeFile(std::move(ast)) &&
               library_->Compile();
    }

    std::string GenerateJSON() {
        auto json_generator = fidl::JSONGenerator(library_.get());
        auto out = json_generator.Produce();
        return out.str();
    }

    bool AddSourceFile(const std::string& filename, const std::string& raw_source_code) {
        auto source_file = MakeSourceFile(filename, raw_source_code);
        fidl::Lexer lexer(source_file, error_reporter_);
        fidl::Parser parser(&lexer, error_reporter_);
        auto ast = parser.Parse();
        return parser.Ok() &&
               library_->ConsumeFile(std::move(ast)) &&
               library_->Compile();
    }

    const fidl::flat::Struct* LookupStruct(const std::string& name) {
        for (const auto& struct_decl : library_->struct_declarations_) {
            if (struct_decl->GetName() == name) {
                return struct_decl.get();
            }
        }
        return nullptr;
    }

    const fidl::flat::Table* LookupTable(const std::string& name) {
        for (const auto& table_decl : library_->table_declarations_) {
            if (table_decl->GetName() == name) {
                return table_decl.get();
            }
        }
        return nullptr;
    }

    const fidl::flat::Union* LookupUnion(const std::string& name) {
        for (const auto& union_decl : library_->union_declarations_) {
            if (union_decl->GetName() == name) {
                return union_decl.get();
            }
        }
        return nullptr;
    }

    const fidl::flat::XUnion* LookupXUnion(const std::string& name) {
        for (const auto& xunion_decl : library_->xunion_declarations_) {
            if (xunion_decl->GetName() == name) {
                return xunion_decl.get();
            }
        }
        return nullptr;
    }

    const fidl::flat::Interface* LookupInterface(const std::string& name) {
        for (const auto& interface_decl : library_->interface_declarations_) {
            if (interface_decl->GetName() == name) {
                return interface_decl.get();
            }
        }
        return nullptr;
    }

    const fidl::flat::Library* library() const {
        return library_.get();
    }

    fidl::SourceFile source_file() {
        return source_file_;
    }

    const std::vector<std::string>& errors() const {
        return error_reporter_->errors();
    }

    const std::vector<std::string>& warnings() const {
        return error_reporter_->warnings();
    }

    const std::vector<fidl::flat::Decl*> declaration_order() const {
        return library_->declaration_order_;
    }

protected:
    SharedAmongstLibraries owned_shared_;
    fidl::ErrorReporter* error_reporter_;
    fidl::flat::Typespace* typespace_;
    fidl::flat::Libraries* all_libraries_;
    fidl::SourceFile source_file_;
    fidl::Lexer lexer_;
    fidl::Parser parser_;
    std::unique_ptr<fidl::flat::Library> library_;
};

#endif // ZIRCON_SYSTEM_UTEST_FIDL_COMPILER_TEST_LIBRARY_H_
