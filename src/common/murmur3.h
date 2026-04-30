#ifndef __H_MURMUR3_H__
#define __H_MURMUR3_H__

#include <stdint.h>
#include <stdlib.h>

void murmur3_hash32(const void *key, size_t len, uint32_t seed, void *out);

// 简单的 64 位哈希函数（MurmurHash64 简化版）
inline uint64_t hash64(uint64_t key) {
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return key;
}

#endif /* __H_MURMUR3_H__ */