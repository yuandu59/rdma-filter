import re
import csv

in_path = 'src/temp/in.txt'
out_path_1 = 'src/temp/out_dram_bf.csv'
out_path_2 = 'src/temp/out_dram_bbf.csv'
out_path_3 = 'src/temp/out_dram_ohbbf.csv'
out_path_4 = 'src/temp/out_dram_cf.csv'

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

def handle_dram_bbf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()
    
    # 提取 DramBBF 实验部分的内容
    dram_bf_match = re.search(r'=== DramBBF Experiment ===\n(.*?)\n=== DramBBF Experiment End ===', content, re.DOTALL)
    if not dram_bf_match:
        print("未找到 DramBBF 实验数据")
        return
    
    content = dram_bf_match.group(1)
    
    # 按负载百分比分割数据
    load_sections = re.split(r'== When Load (\d+) percent elements ==', content)
    
    # 存储所有数据
    data_rows = []
    
    # 处理每个负载阶段（跳过第一个空段）
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
    
    # 写入CSV文件
    if data_rows:
        with open(out_path, 'w', newline='', encoding='utf-8') as outfile:
            # 获取所有字段名
            fieldnames = list(data_rows[0].keys())
            
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data_rows)
        
        print(f"CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效数据")

def handle_dram_ohbbf(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8') as infile:
        content = infile.read()

    # 提取 DramOHBBF 实验部分的内容
    dram_ohbbf_match = re.search(r'=== DramOHBBF Experiment ===\n(.*?)\n=== DramOHBBF Experiment End ===', content, re.DOTALL)
    if not dram_ohbbf_match:
        print("未找到 DramOHBBF 实验数据")
        return

    content = dram_ohbbf_match.group(1)

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
        print(f"DramOHBBF CSV文件已生成: {out_path}")
        print(f"共处理 {len(data_rows)} 个负载阶段")
    else:
        print("未找到有效 DramOHBBF 数据")

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

if __name__ == '__main__':
    handle_dram_bf(in_path, out_path_1)
    handle_dram_bbf(in_path, out_path_2)
    handle_dram_ohbbf(in_path, out_path_3)
    handle_dram_cf(in_path, out_path_4)