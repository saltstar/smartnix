
#ifndef ZIRCON_LIMITS_H_
#define ZIRCON_LIMITS_H_

#include <stdint.h>

#define ZX_PAGE_SHIFT ((uint32_t)12u)
#define ZX_PAGE_SIZE ((uint32_t)(1u << ZX_PAGE_SHIFT))
#define ZX_PAGE_MASK (ZX_PAGE_SIZE - 1u)

#endif // ZIRCON_LIMITS_H_
