// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys.h>

#include <string.h>

// Lookup for number of bits in half byte.
const ui8 NumberOnes[] = {
    //0 1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
};

// Compares two null-terminated file names.
// Returns: TRUE if s1 = s2, else FALSE
int FNameEqu(const char* s1, const char* s2) {
    return (strcmp(s1, s2) == 0) ? TRUE : FALSE;
}
