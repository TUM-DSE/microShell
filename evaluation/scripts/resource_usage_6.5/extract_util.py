import csv


def get_data(total, entry, line):
    # print(line)
    entry = [entry[0], line[5], round(float(line[5])/total[1]*100, 1),
                line[9], round(float(line[9])/total[3]*100, 1),
                float(line[10])+float(line[11])/2, round((float(line[10])+float(line[11])/2)/total[5]*100, 1),
                line[12], round(float(line[12])/total[7]*100, 1)]
    return entry

# def extract_util(coyote_path, ushell_path):
def extract_util(coyote_path, ushell_path, inter_4_path, inter_6_path, inter_8_path):
    result_table = []
    header = ["", "LUTs", "[%]", "Flips-Flops", "[%]", "BRAM", "[%]", "URAM", "[%]"]
    total = ["U280", 1303680, 100.0, 2607360, 100.0, 2016, 100.0, 960, 100.0]
    result_table.append(header)
    result_table.append(total)

    with open(coyote_path, "r") as f:
        reader = csv.reader(f, delimiter="|")
        for i, line in enumerate(reader):
            entry = []
            if len(line) < 5:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            if line[1] == "cyt_top":
                entry = ["Coyote"]
                entry = get_data(total, entry, line)
                result_table.append(entry)
                break

    with open(ushell_path, "r") as f:
        reader = csv.reader(f, delimiter="|")
        ceu_entry = ["CEU", 0, 0, 0, 0, 0, 0, 0, 0]
        # mmu_entry = ["MMU ", 0, 0, 0, 0, 0, 0]
        scheduler_entry = ["Scheduler", 0, 0, 0, 0, 0, 0, 0, 0]
        vio_entry = ["Virtual IO", 0, 0, 0, 0, 0, 0, 0, 0]

        for i, line in enumerate(reader):
            entry = []
            if len(line) < 5:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            # print(line)
            if line[1] == "cyt_top":
                entry = ["uShell"]
                entry = get_data(total, entry, line)
                result_table.append(entry)
            # elif line[1] == "inst_dtu_top_0" or line[1] == "inst_dtu_top_1" or line[2] == "inst_dtu_top_2":
            elif line[1] == "inst_memory_gateway" or line[1] == "inst_gate_recv" or line[2] == "inst_gate_send":
                ceu_entry[1] += float(line[5])
                ceu_entry[3] += float(line[9])
                ceu_entry[5] += float(line[10])+float(line[11])/2
                ceu_entry[7] +=float(line[12])
            elif line[1] == "inst_interconnect":
                entry = ["Inter 3"]
                entry = get_data(total, entry, line)
                result_table.append(entry)
            elif line[1] == "xdma_0":
                entry = ["PCIe DMA (xDMA)"]
                entry = get_data(total, entry, line)
                result_table.append(entry)
    #         elif line[2] == "cdma__parameterized0" or line[2] == "cdma":
    #             cdma_entry[1] += float(line[3])
    #             cdma_entry[3] += float(line[7])
    #             cdma_entry[5] += float(line[8])+float(line[9])/2
            elif line[1] == "inst_mmu_top":
                entry = ["MMU"]
                entry = get_data(total, entry, line)
                entry[1:] = [round(float(x) / 3.0, 1) for x in entry[1:]]
                result_table.append(entry)
    #         elif (line[1] == "inst_hdma_arb_rd" and line[2] == "tlb_arbiter") or line[2] == "tlb_arbiter__parameterized0":
    #             scheduler_entry[1] += float(line[3])
    #             scheduler_entry[3] += float(line[7])
    #             scheduler_entry[5] += float(line[8])+float(line[9])/2
    #         elif line[1] == "inst_user_wrapper_0":
    #             vio_entry[1] += float(line[3])
    #             vio_entry[3] += float(line[7])
    #             vio_entry[5] += float(line[8])+float(line[9])/2
    #         elif line[1] == "inst_reg_direct":
    #             vio_entry[1] -= float(line[3])
    #             vio_entry[3] -= float(line[7])
    #             vio_entry[5] -= float(line[8])+float(line[9])/2


    ceu_entry[1] = round(ceu_entry[1]/3, 1)
    ceu_entry[2] = round(float(ceu_entry[1])/total[1]*100, 1)
    ceu_entry[3] = round(ceu_entry[3]/3, 1)
    ceu_entry[4] = round(float(ceu_entry[3])/total[3]*100, 1)
    ceu_entry[5] = round(ceu_entry[5]/3, 1)
    ceu_entry[6] = round(float(ceu_entry[5])/total[5]*100, 1)
    ceu_entry[7] = round(ceu_entry[7]/3, 1)
    ceu_entry[8] = round(float(ceu_entry[7])/total[7]*100, 1)

    # mmu_entry[2] = round(float(cdma_entry[1])/total[1]*100, 1)
    # mmu_entry[4] = round(float(cdma_entry[3])/total[3]*100, 1)
    # mmu_entry[6] = round(float(cdma_entry[5])/total[5]*100, 1)


    # vio_entry[2] = round(float(vio_entry[1])/total[1]*100, 1)
    # vio_entry[4] = round(float(vio_entry[3])/total[3]*100, 1)
    # vio_entry[6] = round(float(vio_entry[5])/total[5]*100, 1)

    with open(inter_4_path, "r") as f:
        reader = csv.reader(f, delimiter="|")

        for i, line in enumerate(reader):
            entry = []
            if len(line) < 5:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            # print(line)
            if line[1] == "inst_interconnect":
                entry = ["Inter 4"]
                entry = get_data(total, entry, line)
                result_table.append(entry)

    with open(inter_6_path, "r") as f:
        reader = csv.reader(f, delimiter="|")

        for i, line in enumerate(reader):
            entry = []
            if len(line) < 5:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            # print(line)
            if line[1] == "inst_interconnect":
                entry = ["Inter 6"]
                entry = get_data(total, entry, line)
                result_table.append(entry)

    with open(inter_8_path, "r") as f:
        reader = csv.reader(f, delimiter="|")

        for i, line in enumerate(reader):
            entry = []
            if len(line) < 5:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            # print(line)
            if line[1] == "inst_interconnect":
                entry = ["Inter 8"]
                entry = get_data(total, entry, line)
                result_table.append(entry)

    result_table.append(ceu_entry)
    # result_table.append(cdma_entry)
    # result_table.append(scheduler_entry)
    # result_table.append(vio_entry)

    for i in result_table:
        print(i)


if __name__ == "__main__":
    extract_util(
        "../data/resource_usage_6.5/util_coyote.csv",
        "../data/resource_usage_6.5/util_ushell.csv",
        "../data/resource_usage_6.5/util_inter_4.csv",
        "../data/resource_usage_6.5/util_inter_6.csv",
        "../data/resource_usage_6.5/util_inter_8.csv",
    )
