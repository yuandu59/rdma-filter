#include "dram_bf.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "murmur3.h"

using namespace std;

#define bit_set(v, n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n)&0x7))))
#define bit_get(v, n) ((v)[(n) >> 3] & (0x1 << (0x7 - ((n)&0x7))))
#define bit_clr(v, n) ((v)[(n) >> 3] &= ~(0x1 << (0x7 - ((n)&0x7))))

void DramBF_init(struct DramBF *dram_bf, unsigned int n, double fpr)
{
    double m = ((-1.0) * n * log(fpr)) / ((log(2)) * (log(2)));
    double k = (1.0 * m * log(2)) / n;

    memset(dram_bf, 0, sizeof(*dram_bf));
    dram_bf->m = ((int(m) >> 3) + 1) << 3;
    dram_bf->k = ceil(k);
    dram_bf->bit_vector = (uint8_t *)calloc(dram_bf->m >> 3, sizeof(uint8_t));

    return;
}

void DramBF_destroy(struct DramBF *dram_bf)
{
    free(dram_bf->bit_vector);
}

int DramBF_insert(struct DramBF *dram_bf, uint64_t key_)
{
    void *key = &key_;

    for (uint32_t i = 0; i < dram_bf->k; i++)
    {
        uint32_t hash;
        murmur3_hash32(key, 8, i, &hash);

        hash %= dram_bf->m;
        bit_set(dram_bf->bit_vector, hash);
    }
    return 1;
}

int DramBF_lookup(struct DramBF *dram_bf, uint64_t key_)
{
    void *key = &key_;

    for (uint32_t i = 0; i < dram_bf->k; i++)
    {
        uint32_t hash;
        murmur3_hash32(key, 8, i, &hash);
        hash %= dram_bf->m;
        if (!bit_get(dram_bf->bit_vector, hash))
            return 0;
    }

    return 1;
}

int DramBF_bytes(struct DramBF *dram_bf)
{
    return (dram_bf->m >> 3);
}

void DramBF_info(struct DramBF *dram_bf)
{
    cout << dram_bf->m << " " << dram_bf->k << endl;
    return;
}