// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unittest/unittest.h>

// #include <fidl/flat_ast.h>
// #include <fidl/lexer.h>
// #include <fidl/parser.h>
// #include <fidl/source_file.h>

#include "test_library.h"

namespace {

bool GoodConstTestBool() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const bool c = false;
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool BadConstTestBoolWithString() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const bool c = "foo";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert \"foo\" (type string:3) to type bool");

    END_TEST;
}

bool BadConstTestBoolWithNumeric() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const bool c = 6;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert 6 (type int64) to type bool");

    END_TEST;
}

bool GoodConstTestInt32() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const int32 c = 42;
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool GoodConstTestInt32FromOtherConst() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const int32 b = 42;
const int32 c = b;
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool BadConstTestInt32WithString() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const int32 c = "foo";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert \"foo\" (type string:3) to type int32");

    END_TEST;
}

bool BadConstTestInt32WithBool() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const int32 c = true;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert true (type bool) to type int32");

    END_TEST;
}

bool GoodConstTestString() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string:4 c = "four";
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool GoodConstTestStringFromOtherConst() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string:4 c = "four";
const string:5 d = c;
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool BadConstTestStringWithNumeric() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string c = 4;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert 4 (type int64) to type string");

    END_TEST;
}

bool BadConstTestStringWithBool() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string c = true;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert true (type bool) to type string");

    END_TEST;
}

bool BadConstTestStringWithStringTooLong() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string:4 c = "hello";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert \"hello\" (type string:5) to type string:4");

    END_TEST;
}

bool GoodConstTestUsing() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

using foo = int32;
const foo c = 2;
)FIDL");
    ASSERT_TRUE(library.Compile());

    END_TEST;
}

bool BadConstTestUsingWithInconvertibleValue() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

using foo = int32;
const foo c = "nope";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "cannot convert \"nope\" (type string:4) to type int32");

    END_TEST;
}

bool BadConstTestNullableString() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const string? c = "";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "invalid constant type string?");

    END_TEST;
}

bool BadConstTestEnum() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

enum MyEnum : int32 { A = 5; };
const MyEnum c = "";
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "invalid constant type example/MyEnum");

    END_TEST;
}

bool BadConstTestArray() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const array<int32>:2 c = -1;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "invalid constant type array<int32>:2");

    END_TEST;
}

bool BadConstTestVector() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const vector<int32>:2 c = -1;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "invalid constant type vector<int32>:2");

    END_TEST;
}

bool BadConstTestHandleOfThread() {
    BEGIN_TEST;

    TestLibrary library(R"FIDL(
library example;

const handle<thread> c = -1;
)FIDL");
    ASSERT_FALSE(library.Compile());
    auto errors = library.errors();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_STR_STR(errors[0].c_str(), "invalid constant type handle<thread>");

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(consts_tests);

RUN_TEST(GoodConstTestBool);
RUN_TEST(BadConstTestBoolWithString);
RUN_TEST(BadConstTestBoolWithNumeric);

RUN_TEST(GoodConstTestInt32);
RUN_TEST(GoodConstTestInt32FromOtherConst);
RUN_TEST(BadConstTestInt32WithString);
RUN_TEST(BadConstTestInt32WithBool);

RUN_TEST(GoodConstTestString);
RUN_TEST(GoodConstTestStringFromOtherConst);
RUN_TEST(BadConstTestStringWithNumeric);
RUN_TEST(BadConstTestStringWithBool);
RUN_TEST(BadConstTestStringWithStringTooLong);

RUN_TEST(GoodConstTestUsing);
RUN_TEST(BadConstTestUsingWithInconvertibleValue);

RUN_TEST(BadConstTestNullableString);
RUN_TEST(BadConstTestEnum);
RUN_TEST(BadConstTestArray);
RUN_TEST(BadConstTestVector);
RUN_TEST(BadConstTestHandleOfThread);

END_TEST_CASE(consts_tests);
