#!/usr/bin/env python3
import re
import csv
from pathlib import Path
from statistics import mean

PR_LOG = Path("../data/deployment_6.3/reconfig_pr.log")
CAP_LOG = Path("../data/deployment_6.3/reconfig_cap.log")
OUT_CSV = Path("../data/deployment_6.3/reconfig_times.csv")

patterns = {
    "reconfig_time_us": re.compile(r"Reconfiguration done in\s+([0-9.]+)\s*µ?s"),
    "mem_allocation_us": re.compile(r"Mem allocation done in\s+([0-9.]+)\s*µ?s"),
    "object_capability_us": re.compile(r"Object capability done in\s+([0-9.]+)\s*µ?s"),
    "memory_capability_us": re.compile(r"Memory capability done in\s+([0-9.]+)\s*µ?s"),
}

def extract_values(log_file, pattern):
    values = []
    with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m = pattern.search(line)
            if m:
                values.append(float(m.group(1)))
    return values

data = {
    "reconfig_time_us": extract_values(PR_LOG, patterns["reconfig_time_us"]),
    "mem_allocation_us": extract_values(CAP_LOG, patterns["mem_allocation_us"]),
    "object_capability_us": extract_values(CAP_LOG, patterns["object_capability_us"]),
    "memory_capability_us": extract_values(CAP_LOG, patterns["memory_capability_us"]),
}


max_len = max(len(v) for v in data.values())


# Round means to 1 digit after decimal
pr_time = round(mean(data["reconfig_time_us"]), 1)
mem_alloc = round(mean(data["mem_allocation_us"]), 1)
obj_cap = round(mean(data["object_capability_us"]), 1)
mem_cap = round(mean(data["memory_capability_us"]), 1)



with open(OUT_CSV, "w", newline="") as f:
    writer = csv.writer(f)

    writer.writerow(["1 UL", "2 UL", "3 UL", "4 UL"])

    # pr time for 
    writer.writerow([pr_time, pr_time, pr_time, pr_time])
    writer.writerow([mem_alloc, mem_alloc, mem_alloc, mem_alloc])
    writer.writerow([obj_cap*1, obj_cap*2, obj_cap*3, obj_cap*4])
    writer.writerow([mem_cap*1, mem_cap*2, mem_cap*3, mem_cap*4])


print(f"Wrote extracted data to {OUT_CSV}")
for key, values in data.items():
    print(f"{key}: {values}")