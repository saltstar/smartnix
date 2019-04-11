// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "helper.h"

#include <zircon/assert.h>
#include <zircon/types.h>
#include <zxtest/zxtest.h>

// Sanity check that looks for bugs in C macro implementation of ASSERT_*/EXPECT_*. This forces
// the text replacement and allows the compiler to find errors. Otherwise is left to the user
// to find errors once the macro is first used. Also we validate the the assertions return
// and expects dont.
// Tests will fail because we are verifying they actually work as intended, though the
// pass/fail behavior is decided based on Verify functions.

TEST(ZxTestCAssertionsTest, Fail) {
    FAIL("Something bad happened\n");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "FAIL did not abort test execution");
    ZX_ASSERT_MSG(false, "_ZXTEST_ABORT_IF_ERROR not set on failure.");
}

TEST(ZxTestCAssertionsTest, AssertTrueAndFalse) {
    EXPECT_TRUE(true, "EXPECT_TRUE failed.");
    EXPECT_FALSE(false, "EXPECT_FALSE failed.");
    ASSERT_TRUE(true, "ASSERT_TRUE failed.");
    ASSERT_FALSE(false, "ASSERT_FALSE failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "FAIL did not abort test execution");
}

TEST(ZxTestCAssertionsTest, AssertTrueAndFalseFailure) {
    EXPECT_TRUE(false, "EXPECT_TRUE suceed");
    EXPECT_FALSE(true, "EXPECT_FALSE succeed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert did not abort test execution");
}

TEST(ZxTestCAssertionsTest, AssertFalseFailureFatal) {
    ASSERT_FALSE(true, "ASSERT_FALSE succeed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert did not abort test execution");
}

TEST(ZxTestCAssertionsTest, AssertTrueFailureFatal) {
    ASSERT_TRUE(false, "ASSERT_TRUE succeed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert did not abort test execution");
}

TEST(ZxTestCAssertionsTest, AssertEQSuccess) {
    int a = 1;
    int b = 2;

    // Happy cases.
    EXPECT_EQ(1, 1, "EXPECT_EQ identity failed.");
    ASSERT_EQ(1, 1, "ASSERT_EQ identity failed.");
    EXPECT_EQ(a, a, "EXPECT_EQ identity failed.");
    ASSERT_EQ(b, b, "ASSERT_EQ identity failed.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertEQFailure) {
    int a = 1;
    int b = 2;

    EXPECT_EQ(1, 2, "EXPECT_EQ inequality detection succeeded.");
    EXPECT_EQ(a, b, "EXPECT_EQ inequality detection succeeded.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
}

TEST(ZxTestCAssertionsTest, AssertEQFailureFatal) {
    ASSERT_EQ(1, 2, "ASSERT_EQ inequality detection succeeded.");
    ZX_ASSERT_MSG(false, "Fatal assertion failed to return.\n");
}

TEST(ZxTestCAssertionsTest, AssertNESuccess) {
    int a = 1;
    int b = 2;

    // Happy cases.
    EXPECT_NE(1, 2, "EXPECT_NE inequality detection succeeded.");
    EXPECT_NE(a, b, "EXPECT_NE identity detection succeeded.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertNEFailure) {
    int a = 1;

    EXPECT_NE(1, 1, "EXPECT_NE identity failed.");
    EXPECT_NE(a, a, "EXPECT_NE identity failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
}

TEST(ZxTestCAssertionsTest, AssertNEFailureFatal) {
    int a = 1;
    int b = 1;
    ASSERT_NE(a, b, "ASSERT_NE identity detection succeeded.");
    ZX_ASSERT_MSG(false, "Fatal assertion failed to return.\n");
}

TEST(ZxTestCAssertionsTest, AssertLT) {
    int a = 1;
    int b = 2;

    // Happy cases.
    ASSERT_LT(1, 2, "ASSERT_LT failed.");
    EXPECT_LT(a, b, "EXPECT_LT failed.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertLTFailure) {
    int a = 1;
    int b = 2;

    EXPECT_LT(2, 1, "EXPECT_LT failed.");
    EXPECT_LT(b, a, "EXPECT_LT failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
}

TEST(ZxTestCAssertionsTest, AssertLTFailureFatal) {
    int a = 1;
    int b = 2;

    ASSERT_LT(b, a, "EXPECT_LT failed.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertLE) {
    int a = 1;
    int b = 2;

    // Happy cases.
    ASSERT_LE(1, 2, "ASSERT_LE failed.");
    ASSERT_LE(1, 1, "ASSERT_LE failed.");
    EXPECT_LE(a, b, "EXPECT_LE failed.");
    EXPECT_LE(a, a, "EXPECT_LE failed.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertLEFailure) {
    int a = 1;
    int b = 2;

    EXPECT_LE(2, 1, "EXPECT_LE failed.");
    EXPECT_LE(b, a, "EXPECT_LE failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
}

TEST(ZxTestCAssertionsTest, AssertLEFailureFatal) {
    int a = 1;
    int b = 2;

    ASSERT_LE(b, a, "EXPECT_LE failed.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertGT) {
    int a = 1;
    int b = 2;

    // Happy cases.
    EXPECT_GT(2, 1, "EXPECT_GT failed.");
    EXPECT_GT(b, a, "EXPECT_GT failed.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertGTFailure) {
    int a = 1;
    int b = 2;

    EXPECT_GT(a, b, "EXPECT_GT succeeded.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
    ASSERT_GT(1, 2, "ASSERT_GT succeeded.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert did not abort the test.");
}

TEST(ZxTestCAssertionsTest, AssertGTFailureFatal) {
    int a = 1;
    int b = 2;

    ASSERT_GT(a, b, "EXPECT_GT failed.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertGE) {
    int a = 1;
    int b = 2;

    // Happy cases.
    ASSERT_GE(2, 1, "ASSERT_GE failed.");
    ASSERT_GE(1, 1, "ASSERT_GE failed.");
    EXPECT_GE(b, a, "EXPECT_GE failed.");
    EXPECT_GE(a, a, "EXPECT_GE failed.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZxTestCAssertionsTest, AssertGEFailure) {
    int a = 1;
    int b = 2;

    EXPECT_GE(1, 2, "EXPECT_GE failed.");
    EXPECT_GE(a, b, "EXPECT_GE failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect treated as fatal error.");
}

TEST(ZxTestCAssertionsTest, AssertGEFailureFatal) {
    int a = 1;
    int b = 2;

    ASSERT_GE(a, b, "EXPECT_GE failed.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertStrEq) {
    const char* str1 = "a";
    const char* str2 = "a";

    EXPECT_STR_EQ(str1, str2, "ASSERT_STR_EQ failed to identify equal strings.");
    EXPECT_STR_EQ(str1, str1, "ASSERT_STR_EQ failed to identify equal strings.");
    ASSERT_STR_EQ(str1, str2, "ASSERT_STR_EQ failed to identify equal strings.");
    ASSERT_STR_EQ(str1, str1, "ASSERT_STR_EQ failed to identify equal strings.");
    // No failures
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertStrEqFailures) {
    const char* str1 = "a";
    const char* str2 = "b";

    EXPECT_STR_EQ(str1, str2, "ASSERT_STR_EQ failed to identify equal strings.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
    ASSERT_STR_EQ(str1, str2, "ASSERT_STR_EQ failed to identify equal strings.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertNotNull) {
    char a;

    ASSERT_NOT_NULL(&a, "ASSERT_NOT_NULL failed to identify NULL.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertNotNullFailures) {
    char* a = NULL;

    EXPECT_NOT_NULL(a, "EXPECT_NOT_NULL identified NULL.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
    ASSERT_NOT_NULL(a, "ASSERT_NOT_NULL identified NULL.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertNull) {
    char* a = NULL;

    ASSERT_NULL(a, "ASSERT_NOT_NULL identified NULL.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertNullFailures) {
    char b;
    char* a = &b;

    EXPECT_NULL(a, "EXPECT_NOT_NULL identified NULL.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "EXPECT marked the the test as error.");
    ASSERT_NULL(a, "ASSERT_NOT_NULL identified NULL.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertOk) {
    zx_status_t status = ZX_OK;

    ASSERT_OK(status, "ASSERT_OK failed to identify ZX_OK.");
    // Lot of time there are overloaded return types, and we consider only negative numbers
    // as errors.
    ASSERT_OK(4, "ASSERT_OK failed to identify ZX_OK.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertOkFailures) {
    zx_status_t status = ZX_ERR_BAD_STATE;

    EXPECT_OK(status, "ASSERT_OK failed to identify ZX_OK.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Expect marked as fatal error.");
    ASSERT_OK(status, "ASSERT_OK failed to identify ZX_OK.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

TEST(ZXTestCAssertionTest, AssertBytesEq) {
    struct mytype {
        int a;
        int b;
    };
    struct mytype a, b;
    a.a = 0;
    a.b = 1;
    b.a = 0;
    b.b = 1;

    ASSERT_BYTES_EQ(a, a, "ASSERT_BYTES_EQ identity failed.");
    ASSERT_BYTES_EQ(a, b, "ASSERT_BYTES_EQ identity failed.");
    ZX_ASSERT_MSG(!_ZXTEST_ABORT_IF_ERROR, "Succesful assert marked as fatal error.");
    b.b = 2;
    ASSERT_BYTES_EQ(a, b, "ASSERT_BYTES_EQ identity failed.");
    ZX_ASSERT_MSG(_ZXTEST_ABORT_IF_ERROR, "Assert was did not abort test.");
}

static int called = 0;
static int getter_called = 0;
static int Increase(void) {
    return ++called;
}

static int Get(void) {
    getter_called++;
    return called;
}

TEST(ZXTestCAssertionTest, AssertSingleCall) {
    called = 0;
    getter_called = 0;
    EXPECT_EQ(Get(), Increase());
    ZX_ASSERT_MSG(called == 1, "ASSERT_* evaluating multiple times.");
    ZX_ASSERT_MSG(getter_called == 1, "ASSERT_* evaluating multiple times.");
}

TEST(ZXTestCAssertionTest, AssertBytesSingleCall) {
    called = 0;
    getter_called = 0;
    EXPECT_BYTES_EQ(Get(), Increase());
    ZX_ASSERT_MSG(called == 1, "ASSERT_BYTES_* evaluating multiple times.");
    ZX_ASSERT_MSG(getter_called == 1, "ASSERT_* evaluating multiple times.");
}
