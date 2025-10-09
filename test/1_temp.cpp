#include "dram_bf.h"
#include "dram_bbf.h"
#include "dram_ohbbf.h"

#include <iostream>

#define INSERT_COUNT (1 << 26)
#define FALSE_POSITIVE_RATE (double)1.0 / 512
#define BLOCK_SIZE 4

int main(int argc, char **argv) {
    int false_positive;

    struct DramBF dram_bf;
    DramBF_init(&dram_bf, INSERT_COUNT, FALSE_POSITIVE_RATE);

    for (uint64_t i = 0; i < 1000000; i++) {
        DramBF_insert(&dram_bf, i);
    }

    false_positive = 0;
    for (uint64_t i = 1000000; i < 2000000; i++) {
        if (!DramBF_lookup(&dram_bf, i)) {
            ;
        }
    }

    DramBF_destroy(&dram_bf);

    struct DramBBF dram_bbf;
    DramBBF_init(&dram_bbf, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE);

    for (uint64_t i = 0; i < 1000000; i++) {
        DramBBF_insert(&dram_bbf, i);
    }

    false_positive = 0;
    for (uint64_t i = 1000000; i < 2000000; i++) {
        if (!DramBBF_lookup(&dram_bbf, i)) {
            ;
        }
    }

    DramBBF_destroy(&dram_bbf);

    struct DramOHBBF dram_ohbbf;
    DramOHBBF_init(&dram_ohbbf, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE);

    for (uint64_t i = 0; i < 1000000; i++) {
        DramOHBBF_insert(&dram_ohbbf, i);
    }

    false_positive = 0;
    for (uint64_t i = 1000000; i < 2000000; i++) {
        if (!DramOHBBF_lookup(&dram_ohbbf, i)) {
            ;
        }
    }

    DramOHBBF_destroy(&dram_ohbbf);

    return 0;
}