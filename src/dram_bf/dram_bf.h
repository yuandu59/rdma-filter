#ifndef __DRAM_BF_H__
#define __DRAM_BF_H__

#include <stdint.h>
#include <stdlib.h>

struct DramBF
{
    unsigned int m;
    unsigned int k;
    uint8_t *bit_vector;
};

void DramBF_init(struct DramBF *dram_bf, unsigned int n, double fpr);

void DramBF_destroy(struct DramBF *dram_bf);

int DramBF_insert(struct DramBF *dram_bf, uint64_t key);

int DramBF_lookup(struct DramBF *dram_bf, uint64_t key);

int DramBF_bytes(struct DramBF *dram_bf);

void DramBF_info(struct DramBF *dram_bf);

#endif /* __DRAM_BF_H__ */