
#pragma once

#include <lib/crypto/prng.h>

namespace crypto {

namespace GlobalPRNG {

// Returns a pointer to the global PRNG singleton.  The pointer is
// guaranteed to be non-null.
PRNG* GetInstance();

} //namespace GlobalPRNG

} // namespace crypto
