#ifndef __DRAM_BBF_H__
#define __DRAM_BBF_H__

#include <stdint.h>
#include <stdlib.h>

struct DramBBF
{
    unsigned int m;
    unsigned int k;
    uint8_t *bit_vector;
    unsigned int block_size;
    unsigned int block_count;
};

void DramBBF_init(struct DramBBF *dram_bbf, unsigned int n, double fpr, unsigned int block_size);

void DramBBF_destroy(struct DramBBF *dram_bbf);

int DramBBF_insert(struct DramBBF *dram_bbf, uint64_t key);

int DramBBF_lookup(struct DramBBF *dram_bbf, uint64_t key);

void DramBBF_clear(struct DramBBF *dram_bbf);
#endif /* __DRAM_BBF_H__ */