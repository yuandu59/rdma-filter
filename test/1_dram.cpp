#include "utils.h"
#include "dram_bf.h"
#include "dram_bbf.h"
#include "dram_ohbbf.h"
#include "dram_cf.h"
#include "dram_qf.h"
#include "murmur3.h"

#include <iostream>
#include <unordered_set>
#include <chrono>

#define INSERT_COUNT        (1 << 26)
#define FALSE_POSITIVE_RATE (double)(1.0 / 512)
#define BLOCK_SIZE          (4)

#define LOOKUP_COUNT        (1 << 26)
#define REAL_INSERT_COUNT   (1 << 26)
#define SINGLE_ROUND_COUNT  (REAL_INSERT_COUNT / 20)

int main(int argc, char **argv) {

    std::cout << "==== Experiment Begin ====" << std::endl;
    int false_positive_count = 0, true_positive_count = 0, true_negative_count = 0, insert_fail_count = 0, delete_fail_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "= Dataset Preparing =" << std::endl;
    std::vector<uint64_t> to_insert = {}, to_lookup = {};
    to_insert = GenerateRandom64(REAL_INSERT_COUNT);
    std::unordered_set<uint64_t> to_insert_set(to_insert.begin(), to_insert.end());
    while (to_lookup.size() < LOOKUP_COUNT) {
        auto lookup_temp = GenerateRandom64(LOOKUP_COUNT - to_lookup.size());
        for (auto i : lookup_temp) {
            if (to_insert_set.find(i) == to_insert_set.end()) to_lookup.push_back(i);
        }
    }

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== DramBF Experiment ===" << std::endl;
    // struct DramBF dram_bf;
    // DramBF_init(&dram_bf, INSERT_COUNT, FALSE_POSITIVE_RATE);

    // for (int i = 0; i < 20; i++) {
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         DramBF_insert(&dram_bf, to_insert[i * SINGLE_ROUND_COUNT + j]);
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Inserted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

    //     std::cout << "== When Load " << (i + 1) * 5 << " percent elements ==" << std::endl;

    //     true_positive_count = 0;
    //     true_negative_count = 0;
    //     false_positive_count = 0;
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         if (DramBF_lookup(&dram_bf, to_insert[i * SINGLE_ROUND_COUNT + j])) true_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << SINGLE_ROUND_COUNT << " existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Positive Count: " << true_positive_count << std::endl;
    //     std::cout << "True Positive Rate: " << 1.0 * true_positive_count / SINGLE_ROUND_COUNT << std::endl;

    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < LOOKUP_COUNT; j++) {
    //         if (!DramBF_lookup(&dram_bf, to_lookup[j])) true_negative_count++;
    //         else false_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << LOOKUP_COUNT << " non-existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Negative Count: " << true_negative_count << std::endl;
    //     std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    //     std::cout << "False Positive Count: " << false_positive_count << std::endl;
    //     std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    // }
    // DramBF_destroy(&dram_bf);
    // std::cout << "=== DramBF Experiment End ===" << std::endl;

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== DramBBF Experiment ===" << std::endl;
    // struct DramBBF dram_bbf;
    // DramBBF_init(&dram_bbf, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE);

    // for (int i = 0; i < 20; i++) {
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         DramBBF_insert(&dram_bbf, to_insert[i * SINGLE_ROUND_COUNT + j]);
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Inserted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

    //     std::cout << "== When Load " << (i + 1) * 5 << " percent elements ==" << std::endl;

    //     true_positive_count = 0;
    //     true_negative_count = 0;
    //     false_positive_count = 0;
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         if (DramBBF_lookup(&dram_bbf, to_insert[i * SINGLE_ROUND_COUNT + j])) true_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << SINGLE_ROUND_COUNT << " existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Positive Count: " << true_positive_count << std::endl;
    //     std::cout << "True Positive Rate: " << 1.0 * true_positive_count / SINGLE_ROUND_COUNT << std::endl;

    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < LOOKUP_COUNT; j++) {
    //         if (!DramBBF_lookup(&dram_bbf, to_lookup[j])) true_negative_count++;
    //         else false_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << LOOKUP_COUNT << " non-existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Negative Count: " << true_negative_count << std::endl;
    //     std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    //     std::cout << "False Positive Count: " << false_positive_count << std::endl;
    //     std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    // }
    // DramBBF_destroy(&dram_bbf);
    // std::cout << "=== DramBBF Experiment End ===" << std::endl;

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== DramOHBBF Experiment ===" << std::endl;
    // struct DramOHBBF dram_ohbbf;
    // DramOHBBF_init(&dram_ohbbf, INSERT_COUNT, FALSE_POSITIVE_RATE, BLOCK_SIZE);

    // for (int i = 0; i < 20; i++) {
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         DramOHBBF_insert(&dram_ohbbf, to_insert[i * SINGLE_ROUND_COUNT + j]);
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Inserted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

    //     std::cout << "== When Load " << (i + 1) * 5 << " percent elements ==" << std::endl;

    //     true_positive_count = 0;
    //     true_negative_count = 0;
    //     false_positive_count = 0;
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         if (DramOHBBF_lookup(&dram_ohbbf, to_insert[i * SINGLE_ROUND_COUNT + j])) true_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << SINGLE_ROUND_COUNT << " existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Positive Count: " << true_positive_count << std::endl;
    //     std::cout << "True Positive Rate: " << 1.0 * true_positive_count / SINGLE_ROUND_COUNT << std::endl;

    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (int j = 0; j < LOOKUP_COUNT; j++) {
    //         if (!DramOHBBF_lookup(&dram_ohbbf, to_lookup[j])) true_negative_count++;
    //         else false_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << LOOKUP_COUNT << " non-existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Negative Count: " << true_negative_count << std::endl;
    //     std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    //     std::cout << "False Positive Count: " << false_positive_count << std::endl;
    //     std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    // }
    // DramOHBBF_destroy(&dram_ohbbf);
    // std::cout << "=== DramOHBBF Experiment End ===" << std::endl;

// ----------------------------------------------------------------------------------------------

    // std::cout << "=== DramCF Experiment ===" << std::endl;
    // CuckooFilter<uint64_t, 16> dram_cf(INSERT_COUNT);
    // std::cout << "DramCF Info: " << dram_cf.Info() << std::endl;

    // for (size_t i = 0; i < 20; i++) {
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         Status status = dram_cf.Add(to_insert[i * SINGLE_ROUND_COUNT + j]);
    //         if (status != Ok) insert_fail_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Inserted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
    //     std::cout << "Failed Insertions: " << insert_fail_count << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

    //     std::cout << "== When Load " << (i + 1) * 5 << " percent elements ==" << std::endl;
    //     true_positive_count = 0;
    //     true_negative_count = 0;
    //     false_positive_count = 0;
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         if (dram_cf.Contain(to_insert[i * SINGLE_ROUND_COUNT + j]) == Ok) true_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << SINGLE_ROUND_COUNT << " existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Positive Count: " << true_positive_count << std::endl;
    //     std::cout << "True Positive Rate: " << 1.0 * true_positive_count / SINGLE_ROUND_COUNT << std::endl;

    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (size_t j = 0; j < LOOKUP_COUNT; j++) {
    //         if (dram_cf.Contain(to_lookup[j]) == NotFound) true_negative_count++;
    //         else false_positive_count++;
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "= Lookuped " << LOOKUP_COUNT << " non-existing items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
    //     std::cout << "True Negative Count: " << true_negative_count << std::endl;
    //     std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
    //     std::cout << "False Positive Count: " << false_positive_count << std::endl;
    //     std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    // }

    // // debug
    // std::cout << "DramCF num_items_: " << dram_cf.getNum_items_() << std::endl;
    // std::cout << "DramCF NumTagsInTable: " << dram_cf.NumTagsInTable() << std::endl;

    // std::cout << "=== DramCF Deletion Experiment ===" << std::endl;
    // for (size_t i = 0; i < 20; i++) {
    //     start_time = std::chrono::high_resolution_clock::now();
    //     for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
    //         dram_cf.Delete(to_insert[i * SINGLE_ROUND_COUNT + j]);
    //     }
    //     end_time = std::chrono::high_resolution_clock::now();
    //     duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    //     std::cout << "== When Load " << (20 - i) * 5 << " percent elements ==" << std::endl;
    //     std::cout << "= Deleted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
    //     std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
    //     std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

    //     // debug
    //     std::cout << "DramCF num_items_: " << dram_cf.getNum_items_() << std::endl;
    //     std::cout << "DramCF NumTagsInTable: " << dram_cf.NumTagsInTable() << std::endl;
    // }

    // // debug
    // std::cout << "DramCF num_items_: " << dram_cf.getNum_items_() << std::endl;
    // std::cout << "DramCF NumTagsInTable: " << dram_cf.NumTagsInTable() << std::endl;

    // std::cout << "=== DramCF Experiment End ===" << std::endl;

// -------------------------------------------------------------

    std::cout << "=== DramQF Experiment ===" << std::endl;
    
    // 1. 初始化 Quotient Filter
    // q=26 表示 2^26 个槽位，r=9 表示 9 位 remainder
    // q+r=35 <= 64，理论误报率约 1/512 (0.2%)
    struct quotient_filter qf;
    if (!qf_init(&qf, 26, 9)) {
        std::cerr << "QF initialization failed!" << std::endl;
        return -1;
    }
    std::cout << "QF initialized: q=26, r=9, capacity=" << qf.qf_max_size << std::endl;
    std::cout << "QF table size: " << qf_table_size(26, 9) / (1024.0 * 1024.0) << " MB" << std::endl;

    for (size_t i = 0; i < 19; i++) {
        insert_fail_count = 0;
        start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
            // 2. 对原始元素进行哈希，然后插入哈希值
            uint64_t hash = hash64(to_insert[i * SINGLE_ROUND_COUNT + j]);
            if (!qf_insert(&qf, hash)) insert_fail_count++;
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "= Inserted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
        std::cout << "Failed Insertions: " << insert_fail_count << std::endl;
        std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
        std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;

        std::cout << "== When Load " << (i + 1) * 5 << " percent elements ==" << std::endl;
        std::cout << "QF entries: " << qf.qf_entries << " / " << qf.qf_max_size << std::endl;
        std::cout << "Load factor: " << (100.0 * qf.qf_entries / qf.qf_max_size) << "%" << std::endl;
        
        // 3. 查询操作：对原始元素哈希后查询
        true_positive_count = 0;
        true_negative_count = 0;
        false_positive_count = 0;
        start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
            uint64_t hash = hash64(to_insert[i * SINGLE_ROUND_COUNT + j]);
            if (qf_may_contain(&qf, hash)) true_positive_count++;
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "= Lookuped " << SINGLE_ROUND_COUNT << " existing items =" << std::endl;
        std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
        std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
        std::cout << "True Positive Count: " << true_positive_count << std::endl;
        std::cout << "True Positive Rate: " << 1.0 * true_positive_count / SINGLE_ROUND_COUNT << std::endl;

        start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < LOOKUP_COUNT; j++) {
            uint64_t hash = hash64(to_lookup[j]);
            if (!qf_may_contain(&qf, hash)) true_negative_count++;
            else false_positive_count++;
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "= Lookuped " << LOOKUP_COUNT << " non-existing items =" << std::endl;
        std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
        std::cout << "Throughput(op/s): " << LOOKUP_COUNT / duration.count() * 1000.0 << std::endl;
        std::cout << "True Negative Count: " << true_negative_count << std::endl;
        std::cout << "True Negative Rate: " << 1.0 * true_negative_count / LOOKUP_COUNT << std::endl;
        std::cout << "False Positive Count: " << false_positive_count << std::endl;
        std::cout << "False Positive Rate: " << 1.0 * false_positive_count / LOOKUP_COUNT << std::endl;
    }

    std::cout << "=== DramQF Deletion Experiment ===" << std::endl;
    for (size_t i = 0; i < 19; i++) {
        start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < SINGLE_ROUND_COUNT; j++) {
            // 删除操作：对原始元素哈希后删除
            uint64_t hash = hash64(to_insert[i * SINGLE_ROUND_COUNT + j]);
            if (!qf_remove(&qf, hash)) delete_fail_count++;
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "== When Load " << (20 - i) * 5 << " percent elements ==" << std::endl;
        std::cout << "= Deleted " << SINGLE_ROUND_COUNT << " items =" << std::endl;
        std::cout << "Time(s): " << duration.count() / 1000.0 << std::endl;
        std::cout << "Throughput(op/s): " << SINGLE_ROUND_COUNT / duration.count() * 1000.0 << std::endl;
        std::cout << "QF entries: " << qf.qf_entries << " / " << qf.qf_max_size << std::endl;
    }

    // 4. 销毁：释放内部分配的内存
    qf_destroy(&qf);
    std::cout << "=== DramQF Experiment End ===" << std::endl;

    return 0;
}