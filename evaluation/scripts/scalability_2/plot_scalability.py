import argparse
import csv
import os
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# ===== FONT AND STYLE SETTINGS =====
FONT_SIZE = 12
LABEL_SIZE = 11
TICK_SIZE = 10
LEGEND_SIZE = 9
ANNOTATION_SIZE = 9
VALUE_SIZE = 7


plt.rcParams['mathtext.fontset'] = 'cm'
plt.rcParams['font.size'] = FONT_SIZE
plt.rcParams['axes.labelsize'] = LABEL_SIZE
plt.rcParams['xtick.labelsize'] = TICK_SIZE
plt.rcParams['ytick.labelsize'] = TICK_SIZE
plt.rcParams['legend.fontsize'] = LEGEND_SIZE
plt.rcParams['pdf.fonttype'] = 42
plt.rcParams['ps.fonttype'] = 42

# ===== COLOR SETUP =====
palette = sns.color_palette("pastel")
color1 = palette[0]  # Blue for vFPGA=1
color2 = palette[1]  # Orange for vFPGA=2
color3 = palette[2]  # Green for vFPGA=4
color4 = palette[3]  # Red for vFPGA=8

# ===== DATA =====
resources = ["LUTs", "Registers", "BRAM", "URAM"]

# Per-vFPGA budget = % of U280 available to *one* vFPGA in an N-region build.
# Workflow: extract_util.tcl produces 4 raw hierarchical-utilization dumps
# (util_1vfpga.csv … util_8vfpga.csv, ~1.5–4 MB each) in baseline's
# examples_hw/. This script merges them into a single tiny scalability.csv
# in evaluation/data/scalability_2/, then plots from the merged CSV.
#
# Per-region budget is computed from each raw dump as:
#
#   shell_overhead = inst_shell - sum(inst_user_wrapper_*)
#   per_vfpga_pct  = (chip_total - shell_overhead) / N / chip_total * 100
#
# (Static PCIe/xDMA logic is fixed across builds and isn't part of the
# reconfigurable budget the shell hands to vFPGAs, so it's not subtracted.)
# Paper numbers are kept as a per-build fallback so the figure still
# renders even if no raw dumps are reachable.

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "scalability_2")
SUMMARY_CSV = os.path.join(DATA, "scalability.csv")

# Raw util CSVs live next to the bitstream they came from — i.e. inside
# each build's own directory at <baseline>/examples_hw/<n>vfpga/. Pass the
# baseline path via --baseline (or the BASELINE_BASE env var), or copy the
# util_<n>vfpga.csv dumps into evaluation/data/scalability_2/ as a fallback.
_p = argparse.ArgumentParser(add_help=False)
_p.add_argument("--baseline",
                default=os.environ.get("BASELINE_BASE"),
                help="path to baseline repo (default: $BASELINE_BASE)")
_args, _ = _p.parse_known_args()

RAW_PATH_TEMPLATES = []
if _args.baseline:
    RAW_PATH_TEMPLATES.append(
        os.path.join(_args.baseline, "examples_hw", "{n}vfpga", "util_{n}vfpga.csv"))
RAW_PATH_TEMPLATES.append(os.path.join(DATA, "util_{n}vfpga.csv"))
U280_TOTALS = [1_303_680, 2_607_360, 2_016, 960]  # LUTs, Regs, BRAM tiles, URAM
PAPER = {
    1: [96.93, 96.83, 97.72, 100.0],
    2: [47.75, 47.69, 48.12,  50.0],
    4: [23.19, 23.13, 23.96,  25.0],
    8: [10.87, 10.82, 10.91,  12.5],
}


def _row_resources(cells):
    """Pick (LUTs, FFs, BRAM-tiles, URAM) out of one hierarchical-report row.
    Vivado columns: cells[5] Total LUTs, cells[9] FFs, cells[10] RAMB36,
    cells[11] RAMB18 (2× = 1 tile), cells[12] URAM."""
    return (float(cells[5]),
            float(cells[9]),
            float(cells[10]) + float(cells[11]) / 2.0,
            float(cells[12]))


def _per_vfpga_pct(util_path, n):
    """Compute [LUT%, Reg%, BRAM%, URAM%] of the U280 budgeted to one vFPGA
    in the n-region build represented by util_path. Returns None on miss."""
    if not os.path.exists(util_path):
        return None
    shell = None
    user_sum = [0.0, 0.0, 0.0, 0.0]
    with open(util_path) as f:
        for line in csv.reader(f, delimiter="|"):
            if len(line) < 13:
                continue
            cells = [c.strip() for c in line]
            inst = cells[1]
            if inst == "inst_shell":
                shell = _row_resources(cells)
            elif inst.startswith("inst_user_wrapper_"):
                row = _row_resources(cells)
                for i in range(4):
                    user_sum[i] += row[i]
    if shell is None:
        return None
    overhead = [shell[i] - user_sum[i] for i in range(4)]
    return [(U280_TOTALS[i] - overhead[i]) / n / U280_TOTALS[i] * 100.0
            for i in range(4)]


def _find_raw(n):
    """Locate util_<n>vfpga.csv in the first matching RAW_PATH_TEMPLATES."""
    for tmpl in RAW_PATH_TEMPLATES:
        path = tmpl.format(n=n)
        if os.path.exists(path):
            return path
    return None


def merge_scalability_csvs(out_csv=SUMMARY_CSV):
    """Locate the 4 util_<n>vfpga.csv raws (one per build dir), parse them,
    and write the merged per-vFPGA summary to out_csv. Returns out_csv on
    success, or None if any of the 4 raws can't be located."""
    rows = []
    for n in (1, 2, 4, 8):
        path = _find_raw(n)
        if path is None:
            return None
        vals = _per_vfpga_pct(path, n)
        if vals is None:
            return None
        rows.append([n] + vals)
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["n_vfpgas", "LUTs", "Registers", "BRAM", "URAM"])
        for r in rows:
            w.writerow([r[0]] + [f"{x:.2f}" for x in r[1:]])
    return out_csv


def load_vfpga_values():
    """Return {n: [LUT%, Reg%, BRAM%, URAM%]}. Prefers the merged
    scalability.csv; if missing, builds it from the 4 raw util_<n>vfpga.csv;
    if those are missing too, falls back to PAPER values per-build so the
    figure still renders."""
    if not os.path.exists(SUMMARY_CSV):
        merged = merge_scalability_csvs()
        if merged:
            print(f"[plot_scalability] merged 4 util_<n>vfpga.csv into {merged}")

    out = {}
    if os.path.exists(SUMMARY_CSV):
        with open(SUMMARY_CSV) as f:
            for row in csv.DictReader(f):
                n = int(row["n_vfpgas"])
                out[n] = [float(row["LUTs"]), float(row["Registers"]),
                          float(row["BRAM"]), float(row["URAM"])]

    for n in (1, 2, 4, 8):
        if n not in out:
            print(f"[plot_scalability] no data for {n}vfpga — using paper values")
            out[n] = PAPER[n]
    return out


_v = load_vfpga_values()
vfpga1_values = _v[1]
vfpga2_values = _v[2]
vfpga4_values = _v[4]
vfpga8_values = _v[8]

# ===== PLOT SETUP =====
width = 4.0
height = 2.8
fig, ax = plt.subplots(figsize=(width, height))

# Bar settings
bar_width = 0.18
x_positions = np.arange(len(resources))

# Common bar properties
bar_props = {
    'alpha': 1.0,
    'edgecolor': 'k',
    'linewidth': 1.0
}

# ===== PLOTTING BARS =====
# vFPGA=1 bars (solid)
bars1 = ax.bar(x_positions - 1.5*bar_width, vfpga1_values, bar_width,
                color=color1, hatch='//',
                label='vFPGA=1', **bar_props)

# vFPGA=2 bars (diagonal pattern)
bars2 = ax.bar(x_positions - 0.5*bar_width, vfpga2_values, bar_width,
                color=color2, hatch='\\\\',
                label='vFPGA=2', **bar_props)

# vFPGA=4 bars (dotted pattern)
bars3 = ax.bar(x_positions + 0.5*bar_width, vfpga4_values, bar_width,
                color=color3, hatch='..',
                label='vFPGA=4', **bar_props)

# vFPGA=8 bars (cross pattern)
bars4 = ax.bar(x_positions + 1.5*bar_width, vfpga8_values, bar_width,
                color=color4, hatch='xx',
                label='vFPGA=8', **bar_props)

# ===== VALUE ANNOTATIONS =====
# Add percentage values on top of bars
def add_value_labels(bars, values):
    for bar, val in zip(bars, values):
        height = bar.get_height()
        # Format 100.0 as "100%" and others with one decimal
        if val == 100.0:
            label = '100%'
        else:
            label = f'{val:.1f}%'
        ax.text(bar.get_x() + bar.get_width()/2. + 0.03, height + 1.1,
                label, ha='center', va='bottom',
                fontsize=VALUE_SIZE, rotation=90, weight='bold')

# Add labels for each set of bars
add_value_labels(bars1, vfpga1_values)
add_value_labels(bars2, vfpga2_values)
add_value_labels(bars3, vfpga4_values)
add_value_labels(bars4, vfpga8_values)

# ===== AXIS FORMATTING =====
ax.set_ylabel('Available resource per vFPGA (%)')
ax.set_ylim(0, 135)
ax.set_yticks([0, 20, 40, 60, 80, 100])

# X-axis
ax.set_xticks(x_positions)
ax.set_xticklabels(resources, ha='center')

# Grid
ax.grid(True, alpha=0.3, axis='y', color='gray')
ax.set_axisbelow(True)

# Remove top and right spines
sns.despine(ax=ax)

# ===== LEGEND =====
ax.legend(loc='upper right', frameon=True, ncol=2,
          handlelength=2.0, handletextpad=0.5, columnspacing=1.0,
          bbox_to_anchor=(1, 1.14))

# ===== ANNOTATIONS =====
ax.text(0.1, 1.08, 'Higher is better ↑', transform=ax.transAxes,
        color='navy', weight='bold', fontsize=ANNOTATION_SIZE,
        ha='center', va='top')

# ===== SAVE AND DISPLAY =====
plt.tight_layout()
plt.savefig("../plots/scalability_2/scalability_analysis.png", dpi=300, bbox_inches='tight')
plt.savefig("../plots/scalability_2/scalability_analysis.pdf", bbox_inches='tight')
print("Figures generated at ../plots/scalability_2/scalability_analysis.png and ../plots/scalability_2/scalability_analysis.pdf")
