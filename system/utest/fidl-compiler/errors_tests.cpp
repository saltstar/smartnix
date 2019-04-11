// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unittest/unittest.h>

#include "test_library.h"

namespace {

bool GoodError() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

interface Example {
    Method() -> (string foo) error int32;
};

)FIDL");

    ASSERT_TRUE(library.Compile());

    auto methods = &library.LookupInterface("Example")->methods;
    ASSERT_EQ(methods->size(), 1);
    auto method = &methods->at(0);
    auto response = method->maybe_response;
    ASSERT_NOT_NULL(response);
    ASSERT_EQ(response->members.size(), 1);
    auto response_member = &response->members.at(0);
    ASSERT_EQ(response_member->type->kind, fidl::flat::Type::Kind::kIdentifier);
    auto result_identifier = static_cast<fidl::flat::IdentifierType*>(response_member->type.get());
    const fidl::flat::Union* result_union = library.LookupUnion(result_identifier->name.name_part());
    ASSERT_NOT_NULL(result_union);
    ASSERT_NOT_NULL(result_union->attributes);
    ASSERT_TRUE(result_union->attributes->HasAttribute("Result"));
    ASSERT_EQ(result_union->members.size(), 2);

    ASSERT_STR_EQ("response", std::string(result_union->members.at(0).name.data()).c_str());

    const fidl::flat::Union::Member& error = result_union->members.at(1);
    ASSERT_STR_EQ("err", std::string(error.name.data()).c_str());

    ASSERT_NOT_NULL(error.type);
    ASSERT_EQ(error.type->kind, fidl::flat::Type::Kind::kPrimitive);
    auto primitive_type = static_cast<fidl::flat::PrimitiveType*>(error.type.get());
    ASSERT_EQ(primitive_type->subtype, fidl::types::PrimitiveSubtype::kInt32);

    END_TEST;
}

bool GoodErrorUnsigned() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

interface Example {
    Method() -> (string foo) error uint32;
};

)FIDL");

    ASSERT_TRUE(library.Compile());
    END_TEST;
}

bool GoodErrorEnum() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

enum ErrorType : int32 {
    GOOD = 1;
    BAD = 2;
    UGLY = 3;
};

interface Example {
    Method() -> (string foo) error ErrorType;
};

)FIDL");

    ASSERT_TRUE(library.Compile());
    END_TEST;
}

bool GoodErrorEnumAfter() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

interface Example {
    Method() -> (string foo) error ErrorType;
};

enum ErrorType : int32 {
    GOOD = 1;
    BAD = 2;
    UGLY = 3;
};

)FIDL");

    ASSERT_TRUE(library.Compile());
    END_TEST;
}

bool BadErrorUnknownIdentifier() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

interface Example {
    Method() -> (string foo) error ErrorType;
};
)FIDL");

    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "error: invalid error type");
    END_TEST;
}

bool BadErrorWrongPrimitive() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

interface Example {
    Method() -> (string foo) error float32;
};
)FIDL");

    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "error: invalid error type");
    END_TEST;
}

bool BadErrorMissingType() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;
interface Example {
    Method() -> (int32 flub) error;
};
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "error: found unexpected token");
    END_TEST;
}

bool BadErrorNotAType() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;
interface Example {
    Method() -> (int32 flub) error "hello";
};
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "error: found unexpected token");
    END_TEST;
}

bool BadErrorNoResponse() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;
interface Example {
    Method() -> error int32;
};
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "error: unexpected token \"error\"");
    END_TEST;
}
} // namespace

BEGIN_TEST_CASE(errors_tests);

RUN_TEST(GoodError);
RUN_TEST(GoodErrorUnsigned);
RUN_TEST(GoodErrorEnum);
RUN_TEST(GoodErrorEnumAfter);
RUN_TEST(BadErrorUnknownIdentifier);
RUN_TEST(BadErrorWrongPrimitive);
RUN_TEST(BadErrorMissingType);
RUN_TEST(BadErrorNotAType);
RUN_TEST(BadErrorNoResponse);

END_TEST_CASE(errors_tests);
