#ifndef __DRAM_CF_H__
#define __DRAM_CF_H__

#include <cstdint>
#include <assert.h>
#include <cstring>
#include <sstream>
#include <iostream>

#include "hash.h"
#include "utils.h"

#define haszero4(x) (((x)-0x1111ULL) & (~(x)) & 0x8888ULL)
#define hasvalue4(x, n) (haszero4((x) ^ (0x1111ULL * (n))))

#define haszero8(x) (((x)-0x01010101ULL) & (~(x)) & 0x80808080ULL)
#define hasvalue8(x, n) (haszero8((x) ^ (0x01010101ULL * (n))))

#define haszero12(x) (((x)-0x001001001001ULL) & (~(x)) & 0x800800800800ULL)
#define hasvalue12(x, n) (haszero12((x) ^ (0x001001001001ULL * (n))))

#define haszero16(x) (((x)-0x0001000100010001ULL) & (~(x)) & 0x8000800080008000ULL)
#define hasvalue16(x, n) (haszero16((x) ^ (0x0001000100010001ULL * (n))))


// the most naive table implementation: one huge bit array
template <size_t bits_per_tag> class SingleTable
{
    static const size_t kTagsPerBucket = 4;
    static const size_t kBytesPerBucket = (bits_per_tag * kTagsPerBucket + 7) >> 3;
    static const uint32_t kTagMask = (1ULL << bits_per_tag) - 1;
    // NOTE: accomodate extra buckets if necessary to avoid overrun
    // as we always read a uint64
    static const size_t kPaddingBuckets = ((((kBytesPerBucket + 7) / 8) * 8) - 1) / kBytesPerBucket;

    struct Bucket
    {
        char bits_[kBytesPerBucket];
    } __attribute__((__packed__));

    // using a pointer adds one more indirection
    Bucket *buckets_;
    size_t num_buckets_;

  public:
    explicit SingleTable(const size_t num) : num_buckets_(num)
    {
        buckets_ = new Bucket[num_buckets_ + kPaddingBuckets];
        memset(buckets_, 0, kBytesPerBucket * (num_buckets_ + kPaddingBuckets));
    }

    ~SingleTable()
    {
        delete[] buckets_;
    }

    size_t NumBuckets() const
    {
        return num_buckets_;
    }

    size_t SizeInBytes() const
    {
        return kBytesPerBucket * num_buckets_;
    }

    size_t SizeInTags() const
    {
        return kTagsPerBucket * num_buckets_;
    }

    std::string Info() const
    {
        std::stringstream ss;
        ss << "SingleHashtable with tag size: " << bits_per_tag << " bits \n";
        ss << "\t\tAssociativity: " << kTagsPerBucket << "\n";
        ss << "\t\tTotal # of rows: " << num_buckets_ << "\n";
        ss << "\t\tTotal # slots: " << SizeInTags() << "\n";
        return ss.str();
    }

    // read tag from pos(i,j)
    inline uint32_t ReadTag(const size_t i, const size_t j) const
    {
        const char *p = buckets_[i].bits_;
        uint32_t tag;
        /* following code only works for little-endian */
        if (bits_per_tag == 2)
        {
            tag = *((uint8_t *)p) >> (j * 2);
        }
        else if (bits_per_tag == 4)
        {
            p += (j >> 1);
            tag = *((uint8_t *)p) >> ((j & 1) << 2);
        }
        else if (bits_per_tag == 8)
        {
            p += j;
            tag = *((uint8_t *)p);
        }
        else if (bits_per_tag == 12)
        {
            p += j + (j >> 1);
            tag = *((uint16_t *)p) >> ((j & 1) << 2);
        }
        else if (bits_per_tag == 16)
        {
            p += (j << 1);
            tag = *((uint16_t *)p);
        }
        else if (bits_per_tag == 32)
        {
            tag = ((uint32_t *)p)[j];
        }
        return tag & kTagMask;
    }

    // write tag to pos(i,j)
    inline void WriteTag(const size_t i, const size_t j, const uint32_t t)
    {
        char *p = buckets_[i].bits_;
        uint32_t tag = t & kTagMask;
        /* following code only works for little-endian */
        if (bits_per_tag == 2)
        {
            *((uint8_t *)p) |= tag << (2 * j);
        }
        else if (bits_per_tag == 4)
        {
            p += (j >> 1);
            if ((j & 1) == 0)
            {
                *((uint8_t *)p) &= 0xf0;
                *((uint8_t *)p) |= tag;
            }
            else
            {
                *((uint8_t *)p) &= 0x0f;
                *((uint8_t *)p) |= (tag << 4);
            }
        }
        else if (bits_per_tag == 8)
        {
            ((uint8_t *)p)[j] = tag;
        }
        else if (bits_per_tag == 12)
        {
            p += (j + (j >> 1));
            if ((j & 1) == 0)
            {
                ((uint16_t *)p)[0] &= 0xf000;
                ((uint16_t *)p)[0] |= tag;
            }
            else
            {
                ((uint16_t *)p)[0] &= 0x000f;
                ((uint16_t *)p)[0] |= (tag << 4);
            }
        }
        else if (bits_per_tag == 16)
        {
            ((uint16_t *)p)[j] = tag;
        }
        else if (bits_per_tag == 32)
        {
            ((uint32_t *)p)[j] = tag;
        }
    }

    inline bool FindTagInBuckets(const size_t i1, const size_t i2, const uint32_t tag) const
    {
        const char *p1 = buckets_[i1].bits_;
        const char *p2 = buckets_[i2].bits_;

        uint64_t v1 = *((uint64_t *)p1);
        uint64_t v2 = *((uint64_t *)p2);

        // caution: unaligned access & assuming little endian
        if (bits_per_tag == 4 && kTagsPerBucket == 4)
        {
            return hasvalue4(v1, tag) || hasvalue4(v2, tag);
        }
        else if (bits_per_tag == 8 && kTagsPerBucket == 4)
        {
            return hasvalue8(v1, tag) || hasvalue8(v2, tag);
        }
        else if (bits_per_tag == 12 && kTagsPerBucket == 4)
        {
            return hasvalue12(v1, tag) || hasvalue12(v2, tag);
        }
        else if (bits_per_tag == 16 && kTagsPerBucket == 4)
        {
            return hasvalue16(v1, tag) || hasvalue16(v2, tag);
        }
        else
        {
            for (size_t j = 0; j < kTagsPerBucket; j++)
            {
                if ((ReadTag(i1, j) == tag) || (ReadTag(i2, j) == tag))
                {
                    return true;
                }
            }
            return false;
        }
    }

    inline bool FindTagInBucket(const size_t i, const uint32_t tag) const
    {
        // caution: unaligned access & assuming little endian
        if (bits_per_tag == 4 && kTagsPerBucket == 4)
        {
            const char *p = buckets_[i].bits_;
            uint64_t v = *(uint64_t *)p; // uint16_t may suffice
            return hasvalue4(v, tag);
        }
        else if (bits_per_tag == 8 && kTagsPerBucket == 4)
        {
            const char *p = buckets_[i].bits_;
            uint64_t v = *(uint64_t *)p; // uint32_t may suffice
            return hasvalue8(v, tag);
        }
        else if (bits_per_tag == 12 && kTagsPerBucket == 4)
        {
            const char *p = buckets_[i].bits_;
            uint64_t v = *(uint64_t *)p;
            return hasvalue12(v, tag);
        }
        else if (bits_per_tag == 16 && kTagsPerBucket == 4)
        {
            const char *p = buckets_[i].bits_;
            uint64_t v = *(uint64_t *)p;
            return hasvalue16(v, tag);
        }
        else
        {
            for (size_t j = 0; j < kTagsPerBucket; j++)
            {
                if (ReadTag(i, j) == tag)
                {
                    return true;
                }
            }
            return false;
        }
    }

    inline bool DeleteTagFromBucket(const size_t i, const uint32_t tag)
    {
        for (size_t j = 0; j < kTagsPerBucket; j++)
        {
            if (ReadTag(i, j) == tag)
            {
                assert(FindTagInBucket(i, tag) == true);
                WriteTag(i, j, 0);
                return true;
            }
        }
        return false;
    }

    inline bool InsertTagToBucket(const size_t i, const uint32_t tag, const bool kickout, uint32_t &oldtag)
    {
        for (size_t j = 0; j < kTagsPerBucket; j++)
        {
            if (ReadTag(i, j) == 0)
            {
                WriteTag(i, j, tag);
                return true;
            }
        }
        if (kickout)
        {
            size_t r = rand() % kTagsPerBucket;
            oldtag = ReadTag(i, r);
            WriteTag(i, r, tag);
        }
        return false;
    }

    inline size_t NumTagsInBucket(const size_t i) const
    {
        size_t num = 0;
        for (size_t j = 0; j < kTagsPerBucket; j++)
        {
            if (ReadTag(i, j) != 0)
            {
                num++;
            }
        }
        return num;
    }

    inline size_t NumTagsInTable() const
    {
        size_t num = 0;
        for (size_t i = 0; i < num_buckets_; i++)
        {
            num += NumTagsInBucket(i);
        }
        return num;
    }
};

// status returned by a cuckoo filter operation
enum Status
{
    Ok = 0,
    NotFound = 1,
    NotEnoughSpace = 2,
    NotSupported = 3,
};

// maximum number of cuckoo kicks before claiming failure
const size_t kMaxCuckooCount = 500;

// A cuckoo filter class exposes a Bloomier filter interface,
// providing methods of Add, Delete, Contain. It takes three
// template parameters:
//   ItemType:  the type of item you want to insert
//   bits_per_item: how many bits each item is hashed into
//   TableType: the storage of table, SingleTable by default, and
// PackedTable to enable semi-sorting
template <typename ItemType, size_t bits_per_item, template <size_t> class TableType = SingleTable,
          typename HashFamily = TwoIndependentMultiplyShift>
class CuckooFilter
{
    // Storage of items
    TableType<bits_per_item> *table_;

    // Number of items stored
    size_t num_items_;

    typedef struct
    {
        size_t index;
        uint32_t tag;
        bool used;
    } VictimCache;

    VictimCache victim_;

    HashFamily hasher_;

    inline size_t IndexHash(uint32_t hv) const
    {
        // table_->num_buckets is always a power of two, so modulo can be replaced
        // with
        // bitwise-and:
        return hv & (table_->NumBuckets() - 1);
    }

    inline uint32_t TagHash(uint32_t hv) const
    {
        uint32_t tag;
        tag = hv & ((1ULL << bits_per_item) - 1);
        tag += (tag == 0);
        return tag;
    }

    inline void GenerateIndexTagHash(const ItemType &item, size_t *index, uint32_t *tag) const
    {
        const uint64_t hash = hasher_(item);
        *index = IndexHash(hash >> 32);
        *tag = TagHash(hash);
    }

    inline size_t AltIndex(const size_t index, const uint32_t tag) const
    {
        // NOTE(binfan): originally we use:
        // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
        // now doing a quick-n-dirty way:
        // 0x5bd1e995 is the hash constant from MurmurHash2
        return IndexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
    }

    Status AddImpl(const size_t i, const uint32_t tag);

    // load factor is the fraction of occupancy
    double LoadFactor() const
    {
        return 1.0 * Size() / table_->SizeInTags();
    }

    double BitsPerItem() const
    {
        return 8.0 * table_->SizeInBytes() / Size();
    }

  public:
    explicit CuckooFilter(const size_t max_num_keys) : num_items_(0), victim_(), hasher_()
    {
        size_t assoc = 4;
        size_t num_buckets = upperpower2(std::max<uint64_t>(1, max_num_keys / assoc));
        double frac = (double)max_num_keys / num_buckets / assoc;
        if (frac > 0.96)
        {
            std::cout << "note" << std::endl;
            num_buckets <<= 1;
        }
        victim_.used = false;
        table_ = new TableType<bits_per_item>(num_buckets);
    }

    ~CuckooFilter()
    {
        delete table_;
    }

    // Add an item to the filter.
    Status Add(const ItemType &item);

    // Report if the item is inserted, with false positive rate.
    Status Contain(const ItemType &item) const;

    // Delete an key from the filter
    Status Delete(const ItemType &item);

    /* methods for providing stats  */
    // summary infomation
    std::string Info() const;

    // number of current inserted items;
    size_t Size() const
    {
        return num_items_;
    }

    // size of the filter in bytes.
    size_t SizeInBytes() const
    {
        return table_->SizeInBytes();
    }

    size_t NumTagsInTable() const
    {
        return table_->NumTagsInTable();
    }

    size_t getNum_items_() const
    {
        return num_items_;
    }
};

template <typename ItemType, size_t bits_per_item, template <size_t> class TableType, typename HashFamily>
Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Add(const ItemType &item)
{
    size_t i;
    uint32_t tag;

    if (victim_.used)
    {
        return NotEnoughSpace;
    }

    GenerateIndexTagHash(item, &i, &tag);
    return AddImpl(i, tag);
}

template <typename ItemType, size_t bits_per_item, template <size_t> class TableType, typename HashFamily>
Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::AddImpl(const size_t i, const uint32_t tag)
{
    size_t curindex = i;
    uint32_t curtag = tag;
    uint32_t oldtag;

    for (uint32_t count = 0; count < kMaxCuckooCount; count++)
    {
        bool kickout = count > 0;
        oldtag = 0;
        if (table_->InsertTagToBucket(curindex, curtag, kickout, oldtag))
        {
            num_items_++;
            return Ok;
        }
        if (kickout)
        {
            curtag = oldtag;
        }
        curindex = AltIndex(curindex, curtag);
    }

    victim_.index = curindex;
    victim_.tag = curtag;
    victim_.used = true;
    return Ok;
}

template <typename ItemType, size_t bits_per_item, template <size_t> class TableType, typename HashFamily>
Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Contain(const ItemType &key) const
{
    bool found = false;
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);
    i2 = AltIndex(i1, tag);

    assert(i1 == AltIndex(i2, tag));

    found = victim_.used && (tag == victim_.tag) && (i1 == victim_.index || i2 == victim_.index);

    if (found || table_->FindTagInBuckets(i1, i2, tag))
    {
        return Ok;
    }
    else
    {
        return NotFound;
    }
}

template <typename ItemType, size_t bits_per_item, template <size_t> class TableType, typename HashFamily>
Status CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Delete(const ItemType &key)
{
    size_t i1, i2;
    uint32_t tag;

    GenerateIndexTagHash(key, &i1, &tag);
    i2 = AltIndex(i1, tag);

    if (table_->DeleteTagFromBucket(i1, tag))
    {
        num_items_--;
        goto TryEliminateVictim;
    }
    else if (table_->DeleteTagFromBucket(i2, tag))
    {
        num_items_--;
        goto TryEliminateVictim;
    }
    else if (victim_.used && tag == victim_.tag && (i1 == victim_.index || i2 == victim_.index))
    {
        // num_items_--;
        victim_.used = false;
        return Ok;
    }
    else
    {
        return NotFound;
    }
TryEliminateVictim:
    if (victim_.used)
    {
        victim_.used = false;
        size_t i = victim_.index;
        uint32_t tag = victim_.tag;
        AddImpl(i, tag);
    }
    return Ok;
}

template <typename ItemType, size_t bits_per_item, template <size_t> class TableType, typename HashFamily>
std::string CuckooFilter<ItemType, bits_per_item, TableType, HashFamily>::Info() const
{
    std::stringstream ss;
    ss << "CuckooFilter Status:\n"
       << "\t\t" << table_->Info() << "\n"
       << "\t\tKeys stored: " << Size() << "\n"
       << "\t\tLoad factor: " << LoadFactor() << "\n"
       << "\t\tHashtable size: " << (table_->SizeInBytes() >> 10) << " KB\n";
    if (Size() > 0)
    {
        ss << "\t\tbit/key:   " << BitsPerItem() << "\n";
    }
    else
    {
        ss << "\t\tbit/key:   N/A\n";
    }
    return ss.str();
}

#endif // __DRAM_CF_H__