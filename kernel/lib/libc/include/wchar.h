
#pragma once

// The kernel doesn't want this file but some libc++ headers we need
// wind up including it and they need this type.

typedef struct {} mbstate_t;
