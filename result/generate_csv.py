import re
import csv

in_path = 'src/temp/in.txt'
in_path_01 = 'output/out_1.log'
in_path_02 = 'output/out_2.log'
in_path_03 = 'output/rdma-ohbbf-20round/out_2.log'
in_path_04 = 'output/rdma-ohbbf-lockfree-20round/out_1.log'

out_path_01 = 'src/temp/out_dram_bf.csv'
out_path_02 = 'src/temp/out_dram_bbf.csv'
out_path_03 = 'src/temp/out_dram_ohbbf.csv'
out_path_04 = 'src/temp/out_dram_cf.csv'
out_path_05 = 'result/out_rdma_bf_lockfree.csv'
out_path_06 = 'result/out_rdma_bf.csv'
out_path_07 = 'result/out_rdma_bbf.csv'
out_path_08 = 'result/out_rdma_bbf_lockfree.csv'
out_path_09 = 'result/out_rdma_cf.csv'
out_path_10 = 'result/out_rdma_cf_lockfree.csv'
out_path_11 = 'result/out_rdma_ohbbf.csv'
out_path_12 = 'result/out_rdma_ohbbf_lockfree.csv'

def handle_dram_bf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    # 提取 DramBF 实验部分的内容
    dram_bf_match = re.search(r'=== DramBF Experiment ===\n(.*?)\n=== DramBF Experiment End ===', content, re.DOTALL)
    if not dram_bf_match:
        print("未找到 DramBF 实验数据")
        return

    content = dram_bf_match.group(1)

    # 按负载百分比分割数据
    load_sections = re.split(r'== When Load (\d+) percent elements ==', content)

    data_rows = []

    for i in range(1, len(load_sections), 2):
        load_percent = int(load_sections[i])
        section_content = load_sections[i + 1]

        row_data = {'Load': load_percent}

        # 提取插入时间和吞吐量
        insert_match = re.search(r'= Inserted (\d+) items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+]+)', section_content)
        if insert_match:
            row_data['Inserted_Items'] = int(insert_match.group(1))
            row_data['Insert_Time'] = float(insert_match.group(2))
            row_data['Insert_Throughput'] = float(insert_match.group(3))

        # 提取已存在项的查找数据
        existing_lookup = re.search(r'= Lookuped (\d+) existing items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+-]+)\s+True Positive Count: (\d+)\s+True Positive Rate: ([\d.e+-]+)', section_content)
        if existing_lookup:
            row_data['Existing_Lookup_Items'] = int(existing_lookup.group(1))
            row_data['Existing_Lookup_Time'] = float(existing_lookup.group(2))
            row_data['Existing_Lookup_Throughput'] = float(existing_lookup.group(3))
            row_data['True_Positive_Count'] = int(existing_lookup.group(4))
            row_data['True_Positive_Rate'] = float(existing_lookup.group(5))

        # 提取不存在项的查找数据
        non_existing_lookup = re.search(r'= Lookuped (\d+) non-existing items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+-]+)\s+True Negative Count: (\d+)\s+True Negative Rate: ([\d.e+-]+)\s+False Positive Count: (\d+)\s+False Positive Rate: ([\d.e+-]+)', section_content)
        if non_existing_lookup:
            row_data['Non_Existing_Lookup_Items'] = int(non_existing_lookup.group(1))
            row_data['Non_Existing_Lookup_Time'] = float(non_existing_lookup.group(2))
            row_data['Non_Existing_Lookup_Throughput'] = float(non_existing_lookup.group(3))
            row_data['True_Negative_Count'] = int(non_existing_lookup.group(4))
            row_data['True_Negative_Rate'] = float(non_existing_lookup.group(5))
            row_data['False_Positive_Count'] = int(non_existing_lookup.group(6))
            row_data['False_Positive_Rate'] = float(non_existing_lookup.group(7))

        data_rows.append(row_data)

    if data_rows:
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            fieldnames = list(data_rows[0].keys())
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        print(f"DramBF CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效 DramBF 数据")

def handle_dram_bbf_or_ohbbf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    # DramBBF / DramOHBBF 日志结构一致，统一解析
    dram_bbf_match = re.search(
        r'=== (Dram(?:OH)?BBF) Experiment ===\n(.*?)\n=== \1 Experiment End ===',
        content,
        re.DOTALL,
    )
    if not dram_bbf_match:
        print("未找到 DramBBF/DramOHBBF 实验数据")
        return

    experiment_name = dram_bbf_match.group(1)
    content = dram_bbf_match.group(2)

    # 按负载百分比分割数据
    load_sections = re.split(r'== When Load (\d+) percent elements ==', content)

    data_rows = []

    for i in range(1, len(load_sections), 2):
        load_percent = int(load_sections[i])
        section_content = load_sections[i + 1]

        row_data = {'Load': load_percent}

        # 提取插入时间和吞吐量
        insert_match = re.search(r'= Inserted (\d+) items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+]+)', section_content)
        if insert_match:
            row_data['Inserted_Items'] = int(insert_match.group(1))
            row_data['Insert_Time'] = float(insert_match.group(2))
            row_data['Insert_Throughput'] = float(insert_match.group(3))

        # 提取已存在项的查找数据
        existing_lookup = re.search(r'= Lookuped (\d+) existing items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+-]+)\s+True Positive Count: (\d+)\s+True Positive Rate: ([\d.e+-]+)', section_content)
        if existing_lookup:
            row_data['Existing_Lookup_Items'] = int(existing_lookup.group(1))
            row_data['Existing_Lookup_Time'] = float(existing_lookup.group(2))
            row_data['Existing_Lookup_Throughput'] = float(existing_lookup.group(3))
            row_data['True_Positive_Count'] = int(existing_lookup.group(4))
            row_data['True_Positive_Rate'] = float(existing_lookup.group(5))

        # 提取不存在项的查找数据
        non_existing_lookup = re.search(r'= Lookuped (\d+) non-existing items =\s+Time\(s\): ([\d.e+-]+)\s+Throughput\(op/s\): ([\d.e+-]+)\s+True Negative Count: (\d+)\s+True Negative Rate: ([\d.e+-]+)\s+False Positive Count: (\d+)\s+False Positive Rate: ([\d.e+-]+)', section_content)
        if non_existing_lookup:
            row_data['Non_Existing_Lookup_Items'] = int(non_existing_lookup.group(1))
            row_data['Non_Existing_Lookup_Time'] = float(non_existing_lookup.group(2))
            row_data['Non_Existing_Lookup_Throughput'] = float(non_existing_lookup.group(3))
            row_data['True_Negative_Count'] = int(non_existing_lookup.group(4))
            row_data['True_Negative_Rate'] = float(non_existing_lookup.group(5))
            row_data['False_Positive_Count'] = int(non_existing_lookup.group(6))
            row_data['False_Positive_Rate'] = float(non_existing_lookup.group(7))

        data_rows.append(row_data)

    if data_rows:
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            fieldnames = list(data_rows[0].keys())
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)

        print(f"{experiment_name} CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print(f"未找到有效 {experiment_name} 数据")

def handle_dram_cf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    # 提取 DramCF 实验部分的内容
    dram_cf_match = re.search(r'=== DramCF Experiment ===\n(.*?)\n=== DramCF Experiment End ===', content, re.DOTALL)
    if not dram_cf_match:
        print("未找到 DramCF 实验数据")
        return

    content = dram_cf_match.group(1)

    # 按负载百分比分割数据（插入+查找阶段）
    load_sections = re.split(r'== When Load (\d+) percent elements ==', content)

    data_rows = []

    # 记录插入阶段的正则
    insert_pattern = re.compile(r'= Inserted (\d+) items =\nFailed Insertions: (\d+)\nTime\(s\): ([\d.e+-]+)\nThroughput\(op/s\): ([\d.e+]+)', re.MULTILINE)
    # 查找阶段正则
    lookup_exist_pattern = re.compile(r'= Lookuped (\d+) existing items =\nTime\(s\): ([\d.e+-]+)\nThroughput\(op/s\): ([\d.e+-]+)\nTrue Positive Count: (\d+)\nTrue Positive Rate: ([\d.e+-]+)', re.MULTILINE)
    lookup_nonexist_pattern = re.compile(r'= Lookuped (\d+) non-existing items =\nTime\(s\): ([\d.e+-]+)\nThroughput\(op/s\): ([\d.e+-]+)\nTrue Negative Count: (\d+)\nTrue Negative Rate: ([\d.e+-]+)\nFalse Positive Count: (\d+)\nFalse Positive Rate: ([\d.e+-]+)', re.MULTILINE)
    # 删除阶段正则
    delete_pattern = re.compile(r'= Deleted (\d+) items =\nTime\(s\): ([\d.e+-]+)\nThroughput\(op/s\): ([\d.e+]+)', re.MULTILINE)

    # 先处理插入+查找阶段
    for i in range(1, len(load_sections) // 2, 2):
        load_percent = int(load_sections[i])
        section_content = load_sections[i + 1]

        row_data = {'Load': load_percent}

        # 插入阶段
        insert_match = insert_pattern.search(section_content)
        if insert_match:
            row_data['Inserted_Items'] = int(insert_match.group(1))
            row_data['Failed_Insertions'] = int(insert_match.group(2))
            row_data['Insert_Time'] = float(insert_match.group(3))
            row_data['Insert_Throughput'] = float(insert_match.group(4))

        # 查找已存在项
        exist_match = lookup_exist_pattern.search(section_content)
        if exist_match:
            row_data['Existing_Lookup_Items'] = int(exist_match.group(1))
            row_data['Existing_Lookup_Time'] = float(exist_match.group(2))
            row_data['Existing_Lookup_Throughput'] = float(exist_match.group(3))
            row_data['True_Positive_Count'] = int(exist_match.group(4))
            row_data['True_Positive_Rate'] = float(exist_match.group(5))

        # 查找不存在项
        nonexist_match = lookup_nonexist_pattern.search(section_content)
        if nonexist_match:
            row_data['Non_Existing_Lookup_Items'] = int(nonexist_match.group(1))
            row_data['Non_Existing_Lookup_Time'] = float(nonexist_match.group(2))
            row_data['Non_Existing_Lookup_Throughput'] = float(nonexist_match.group(3))
            row_data['True_Negative_Count'] = int(nonexist_match.group(4))
            row_data['True_Negative_Rate'] = float(nonexist_match.group(5))
            row_data['False_Positive_Count'] = int(nonexist_match.group(6))
            row_data['False_Positive_Rate'] = float(nonexist_match.group(7))

        data_rows.append(row_data)
    
    # 然后处理删除阶段
    for i in range(1, len(load_sections) // 2, 2):
        load_percent = int(load_sections[i + len(load_sections) // 2])
        section_content = load_sections[i + len(load_sections) // 2 + 1]

        delete_pattern_match = delete_pattern.search(section_content)
        if delete_pattern_match:
            # 找到对应的行
            for row in data_rows:
                if row['Load'] == load_percent:
                    row['Deleted_Items'] = int(delete_pattern_match.group(1))
                    row['Delete_Time'] = float(delete_pattern_match.group(2))
                    row['Delete_Throughput'] = float(delete_pattern_match.group(3))
                    break

    # 写入CSV
    if data_rows:
        # 合并所有可能的字段
        all_fields = set()
        for row in data_rows:
            all_fields.update(row.keys())
        # 保证顺序：Load、插入、查找、删除
        prefer_order = [
            'Load',
            'Inserted_Items', 'Failed_Insertions', 'Insert_Time', 'Insert_Throughput',
            'Existing_Lookup_Items', 'Existing_Lookup_Time', 'Existing_Lookup_Throughput', 'True_Positive_Count', 'True_Positive_Rate',
            'Non_Existing_Lookup_Items', 'Non_Existing_Lookup_Time', 'Non_Existing_Lookup_Throughput', 'True_Negative_Count', 'True_Negative_Rate', 'False_Positive_Count', 'False_Positive_Rate',
            'Deleted_Items', 'Delete_Time', 'Delete_Throughput'
        ]
        # 只保留实际出现的字段
        fieldnames = [f for f in prefer_order if f in all_fields]
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        print(f"DramCF CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效 DramCF 数据")

def handle_rdma_bf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    rdma_bf_match = re.search(r'=== RdmaBF Experiment ===\n(.*?)\n==== Experiment End ====', content, re.DOTALL)
    if not rdma_bf_match:
        print("未找到 RdmaBF 实验数据")
        return

    content = rdma_bf_match.group(1)

    insert_qps_list = [
        float(value)
        for value in re.findall(
            r'= Inserting =.*?Inserted \d+ items\.\s+Time\(s\): [\d.e+-]+\s+Throughput\(op/s\): ([\d.e+-]+)',
            content,
            re.DOTALL,
        )
    ]

    lookup_matches = re.findall(
        r'== When Load (\d+) percent elements ==.*?'
        r'= Lookingup existing items =.*?'
        r'Lookup \d+ existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+).*?'
        r'= Lookingup non-existing items =.*?'
        r'Lookup \d+ non-existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+)',
        content,
        re.DOTALL,
    )

    row_map = {}

    for index, insert_qps in enumerate(insert_qps_list):
        load_factor = index * 5
        row_map[load_factor] = {
            'Load': load_factor,
            'Insert_QPS': float(insert_qps),
            'Positive_Query_QPS': '',
            'Negative_Query_QPS': '',
        }

    for matched_load, positive_query_qps, negative_query_qps in lookup_matches:
        load_factor = int(matched_load)
        if load_factor not in row_map:
            row_map[load_factor] = {
                'Load': load_factor,
                'Insert_QPS': '',
                'Positive_Query_QPS': '',
                'Negative_Query_QPS': '',
            }

        row_map[load_factor]['Positive_Query_QPS'] = float(positive_query_qps)
        row_map[load_factor]['Negative_Query_QPS'] = float(negative_query_qps)

    data_rows = [row_map[load] for load in sorted(row_map.keys())]

    if data_rows:
        fieldnames = ['Load', 'Insert_QPS', 'Positive_Query_QPS', 'Negative_Query_QPS']
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        print(f"RdmaBF CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效 RdmaBF 数据")

def handle_rdma_bbf_or_ohbbf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    # RdmaBBF / RdmaOHBBF 日志结构一致，统一解析
    rdma_bbf_match = re.search(
        r'=== (Rdma(?:OH)?BBF) Experiment ===\n(.*?)\n==== Experiment End ====',
        content,
        re.DOTALL,
    )
    if not rdma_bbf_match:
        print("未找到 RdmaBBF/RdmaOHBBF 实验数据")
        return

    experiment_name = rdma_bbf_match.group(1)
    content = rdma_bbf_match.group(2)

    insert_qps_list = [
        float(value)
        for value in re.findall(
            r'= Inserting =.*?Inserted \d+ items\.\s+Time\(s\): [\d.e+-]+\s+Throughput\(op/s\): ([\d.e+-]+)',
            content,
            re.DOTALL,
        )
    ]

    lookup_matches = re.findall(
        r'== When Load (\d+) percent elements ==.*?'
        r'= Lookingup existing items =.*?'
        r'Lookup \d+ existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+).*?'
        r'= Lookingup non-existing items =.*?'
        r'Lookup \d+ non-existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+)',
        content,
        re.DOTALL,
    )

    row_map = {}

    for index, insert_qps in enumerate(insert_qps_list):
        load_factor = index * 5
        row_map[load_factor] = {
            'Load': load_factor,
            'Insert_QPS': float(insert_qps),
            'Positive_Query_QPS': '',
            'Negative_Query_QPS': '',
        }

    for matched_load, positive_query_qps, negative_query_qps in lookup_matches:
        load_factor = int(matched_load)
        if load_factor not in row_map:
            row_map[load_factor] = {
                'Load': load_factor,
                'Insert_QPS': '',
                'Positive_Query_QPS': '',
                'Negative_Query_QPS': '',
            }

        row_map[load_factor]['Positive_Query_QPS'] = float(positive_query_qps)
        row_map[load_factor]['Negative_Query_QPS'] = float(negative_query_qps)

    data_rows = [row_map[load] for load in sorted(row_map.keys())]

    if data_rows:
        fieldnames = ['Load', 'Insert_QPS', 'Positive_Query_QPS', 'Negative_Query_QPS']
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        print(f"{experiment_name} CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print(f"未找到有效 {experiment_name} 数据")

def handle_rdma_cf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    rdma_cf_match = re.search(r'=== RdmaCF Experiment ===\n(.*?)\n==== Experiment End ====', content, re.DOTALL)
    if not rdma_cf_match:
        print("未找到 RdmaCF 实验数据")
        return

    content = rdma_cf_match.group(1)

    insert_qps_list = [
        float(value)
        for value in re.findall(
            r'= Inserting =.*?Inserted \d+ items\.\s+Time\(s\): [\d.e+-]+\s+Throughput\(op/s\): ([\d.e+-]+)',
            content,
            re.DOTALL,
        )
    ]

    lookup_matches = re.findall(
        r'== When Load (\d+) percent elements ==.*?'
        r'= Lookingup existing items =.*?'
        r'Lookup \d+ existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+).*?'
        r'= Lookingup non-existing items =.*?'
        r'Lookup \d+ non-existing items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+)',
        content,
        re.DOTALL,
    )

    delete_matches = re.findall(
        r'== When Load (\d+) percent elements ==\s*'
        r'= Deleting items =.*?'
        r'Deleted \d+ items\.\s+'
        r'Time\(s\): [\d.e+-]+\s+'
        r'Throughput\(op/s\): ([\d.e+-]+)',
        content,
        re.DOTALL,
    )

    row_map = {}

    # Insert阶段与其他handle保持一致：第1轮插入对应Load=0
    for index, insert_qps in enumerate(insert_qps_list):
        load_factor = index * 5
        row_map[load_factor] = {
            'Load': load_factor,
            'Insert_QPS': float(insert_qps),
            'Positive_Query_QPS': '',
            'Negative_Query_QPS': '',
            'delete_qps': '',
        }

    for matched_load, positive_query_qps, negative_query_qps in lookup_matches:
        load_factor = int(matched_load)
        if load_factor not in row_map:
            row_map[load_factor] = {
                'Load': load_factor,
                'Insert_QPS': '',
                'Positive_Query_QPS': '',
                'Negative_Query_QPS': '',
                'delete_qps': '',
            }

        row_map[load_factor]['Positive_Query_QPS'] = float(positive_query_qps)
        row_map[load_factor]['Negative_Query_QPS'] = float(negative_query_qps)

    for matched_load, delete_qps in delete_matches:
        load_factor = int(matched_load)
        if load_factor not in row_map:
            row_map[load_factor] = {
                'Load': load_factor,
                'Insert_QPS': '',
                'Positive_Query_QPS': '',
                'Negative_Query_QPS': '',
                'delete_qps': '',
            }

        row_map[load_factor]['delete_qps'] = float(delete_qps)

    data_rows = [row_map[load] for load in sorted(row_map.keys())]

    if data_rows:
        fieldnames = ['Load', 'Insert_QPS', 'Positive_Query_QPS', 'Negative_Query_QPS', 'delete_qps']
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        print(f"RdmaCF CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效 RdmaCF 数据")


if __name__ == '__main__':
    '''
    python result/generate_csv.py
    '''
    handle_rdma_bbf(in_path_04, out_path_12)