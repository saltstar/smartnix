// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/backtrace-request/backtrace-request.h>

// This trivial test exists in order to prove that backtrace_request
// works: that it reports a backtrace, as an integration test.

int main() {
    backtrace_request();
}
