import sys
import csv


def get_data(total, entry, line):
    entry = [entry[0], line[5], round(float(line[5])/total[1]*100, 1),
                line[9], round(float(line[9])/total[3]*100, 1),
                float(line[10])+float(line[11])/2, round((float(line[10])+float(line[11])/2)/total[5]*100, 1),
                line[12], round(float(line[12])/total[7]*100, 1)]
    return entry


# Per-module footprints from the resource_usage build (7 modules on 7 vFPGAs
# in a single shell). Each row pulls the module-proper instance from inside
# its inst_user_c0_<i> wrapper, skipping the shell glue (skid registers,
# host_mux, debug bridge, AXI register slices, queue arbiters, credits).
# Instance names come from each module's apps/modules/<m>/vfpga_top.svh and
# the CMake load_apps order in baseline examples_hw/CMakeLists.txt.
MODULES = [
    ("FFT",      "inst_fft"),
    ("Quantize", "inst_quant"),
    ("RLE",      "inst_rle"),
    ("SHA-256",  "inst_sha2"),    # baseline apps/sha2 names the instance inst_sha2
    ("RSA",      "inst_rsa_c0_4"),
    ("AES-CTR",  "inst_aes"),
    ("SVM",      "inst_svm"),
]


def extract_modules(resource_usage_path="../data/resource_usage_6.5/module_resource_usage.csv"):
    """Per-module utilization from the 7-vFPGA resource_usage build.

    Reads the raw Vivado hierarchical report. Emits a wide-table layout:
    header + U280 totals + one row per module, columns
    [name, LUTs, %, FFs, %, BRAM, %, URAM, %].
    """
    result_table = []
    header = ["", "LUTs", "[%]", "Flips-Flops", "[%]", "BRAM", "[%]", "URAM", "[%]"]
    total = ["U280", 1303680, 100.0, 2607360, 100.0, 2016, 100.0, 960, 100.0]
    result_table.append(header)
    result_table.append(total)

    # Pre-resolve which line in the report belongs to each module instance.
    # Vivado pads Instance with leading spaces for hierarchy depth, so we
    # match on the trimmed cell to keep the right user_c0_<i> child and
    # avoid nested look-alikes (e.g. there's a deeper inst_aes inside aes_top).
    wanted = {inst: name for name, inst in MODULES}
    seen = set()

    with open(resource_usage_path, "r") as f:
        reader = csv.reader(f, delimiter="|")
        for line in reader:
            if len(line) < 13:
                continue
            for j in range(len(line)):
                line[j] = line[j].strip()
            inst = line[1]
            if inst in wanted and inst not in seen:
                entry = [wanted[inst]]
                entry = get_data(total, entry, line)
                result_table.append(entry)
                seen.add(inst)

    for name, inst in MODULES:
        if inst not in seen:
            print(f"WARN: instance {inst} ({name}) not found in {resource_usage_path}",
                  file=sys.stderr)

    for i in result_table:
        print(i)


if __name__ == "__main__":
    print("=== Per-module utilization (resource_usage build) ===")
    extract_modules()
