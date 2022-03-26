//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
#endif
    uint64_t
    MurmurHash64(const void *key, int len, uint64_t seed);

inline uint64_t MurmurHash(const void *key, int len) {
  return MurmurHash64(key, len, 0);
}
