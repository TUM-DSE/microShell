import csv
import os
import statistics
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# ===== FONT AND STYLE SETTINGS =====
FONT_SIZE = 10
LABEL_SIZE = 12
TICK_SIZE = 12
LEGEND_SIZE = 12
ANNOTATION_SIZE = 12
SPEEDUP_SIZE = 10

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
color1 = palette[0]  # Blue
color2 = palette[1]  # Orange
color3 = palette[2]  # Green
size_colors = [color1, color2, color3]  # Order: 8KB=Orange, 256KB=Blue, 1MB=Green

# ===== DATA =====
applications = ["Audio Processing", "Digital Signature", "Secure Storage", "Signed Compression", "Speech Recognition"]
display_names = applications[:]
data_sizes = ["8KB", "256KB", "1MB"]
data_size_bytes = [8192, 262144, 1048576]

# CSV uses internal app keys + system tags; the column order in `data[app]`
# below is [coyote_8K, ushell_mono_8K, ushell_8K, coyote_256K, ..., ushell_1M].
app_keys = ["audio_processing", "digital_signature", "secure_storage",
            "signed_compression", "speech_recognition"]
systems = ["coyote", "ushell_mono", "ushell"]   # column order within each size group

CSV_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "..", "..", "data", "e2e_6.1", "e2e.csv")


def load_e2e():
    """Read data/e2e_6.1/e2e.csv (rows tagged source=paper or source=measured).
    For each (app, system, size):
      - if any source=measured rows exist → average them, std-dev → error
      - else → use the source=paper row's throughput_MBps + stddev_MBps
        directly

    The paper rows act as the fallback baseline. Once measured rows arrive,
    they take over per-cell. The CSV is the only source of truth — there
    are no hardcoded numbers in this script."""
    if not os.path.exists(CSV_PATH):
        raise SystemExit(
            f"[plot_e2e] {CSV_PATH} not found.\n"
            f"  Re-create it with the paper baseline (source=paper rows), or run\n"
            f"  bash run_e2e.sh <baseline> <microshell> first."
        )

    measured = {}  # (app, size_bytes, system) -> [thr, ...]
    paper = {}     # (app, size_bytes, system) -> (thr, stddev)
    with open(CSV_PATH) as f:
        for row in csv.DictReader(f):
            try:
                thr = float(row["throughput_MBps"])
                key = (row["app"], int(row["size_bytes"]), row["system"])
            except (KeyError, ValueError):
                continue
            if row.get("source") == "paper":
                try:
                    sd = float(row.get("stddev_MBps") or 0.0)
                except ValueError:
                    sd = 0.0
                paper[key] = (thr, sd)
            else:
                measured.setdefault(key, []).append(thr)

    data, error_data = {}, {}
    for app_idx, app_key in enumerate(app_keys):
        display = applications[app_idx]
        d, e = [], []
        for size_b in data_size_bytes:
            for sys in systems:
                key = (app_key, size_b, sys)
                ms = measured.get(key)
                if ms:
                    d.append(sum(ms) / len(ms))
                    e.append(statistics.stdev(ms) if len(ms) > 1 else 0.0)
                elif key in paper:
                    thr, sd = paper[key]
                    d.append(thr)
                    e.append(sd)
                else:
                    raise SystemExit(
                        f"[plot_e2e] no data for {app_key}/{sys}/{size_b} "
                        f"(no measured or paper row in CSV)."
                    )
        data[display] = d
        error_data[display] = e
    return data, error_data


data, error_data = load_e2e()

# ===== PLOT SETUP =====
fig, ax = plt.subplots(figsize=(16, 3))

# Bar settings
bar_width = 0.1
group_spacing = 1.2
x_positions = np.arange(len(applications)) * group_spacing

# Hatch patterns: solid, diagonal, dotted
hatch_patterns = ["", "//", ".."]

# ===== PLOTTING BARS =====
for size_idx, size in enumerate(data_sizes):
    baseline_values = []
    ushell_mono_values = []
    ushell_values = []
    baseline_errors = []
    ushell_mono_errors = []
    ushell_errors = []
    x_baseline = []
    x_ushell_mono = []
    x_ushell = []

    # Collect data for current size
    for app_idx, app in enumerate(applications):
        baseline_val = data[app][size_idx * 3]
        ushell_mono_val = data[app][size_idx * 3 + 1]
        ushell_val = data[app][size_idx * 3 + 2]

        baseline_err = error_data[app][size_idx * 3]
        ushell_mono_err = error_data[app][size_idx * 3 + 1]
        ushell_err = error_data[app][size_idx * 3 + 2]

        if baseline_val is not None:
            baseline_values.append(baseline_val)
            baseline_errors.append(baseline_err)
            x_baseline.append(x_positions[app_idx] + size_idx * (bar_width * 3))

        if ushell_mono_val is not None:
            ushell_mono_values.append(ushell_mono_val)
            ushell_mono_errors.append(ushell_mono_err)
            x_ushell_mono.append(x_positions[app_idx] + size_idx * (bar_width * 3) + bar_width)

        if ushell_val is not None:
            ushell_values.append(ushell_val)
            ushell_errors.append(ushell_err)
            x_ushell.append(x_positions[app_idx] + size_idx * (bar_width * 3) + bar_width * 2)

    # Common bar properties
    bar_props = {
        'alpha': 1.0,
        'edgecolor': 'k',
        'linewidth': 1,
        'error_kw': {'ecolor': 'black', 'elinewidth': 1, 'capsize': 2}
    }

    # Plot bars with their respective patterns
    if baseline_values:
        ax.bar(x_baseline, baseline_values, bar_width,
               yerr=baseline_errors, color=size_colors[size_idx],
                hatch='//', **bar_props)

    if ushell_mono_values:
        ax.bar(x_ushell_mono, ushell_mono_values, bar_width,
               yerr=ushell_mono_errors, color=size_colors[size_idx],
               hatch='\\\\', **bar_props)

    if ushell_values:
        ax.bar(x_ushell, ushell_values, bar_width,
               yerr=ushell_errors, color=size_colors[size_idx],
               hatch='..', **bar_props)

# ===== SPEEDUP ANNOTATIONS =====
for size_idx, size in enumerate(data_sizes):
    for app_idx, app in enumerate(applications):
        baseline_val = data[app][size_idx * 3]
        ushell_mono_val = data[app][size_idx * 3 + 1]
        ushell_val = data[app][size_idx * 3 + 2]

        # Get error values for positioning
        ushell_mono_err = error_data[app][size_idx * 3 + 1]
        ushell_err = error_data[app][size_idx * 3 + 2]

        # Speedup for µShell_mono
        if baseline_val is not None and ushell_mono_val is not None:
            speedup = ushell_mono_val / baseline_val
            x_pos = x_positions[app_idx] + size_idx * (bar_width * 3) + bar_width
            y_pos = ushell_mono_val + ushell_mono_err + 4

            ax.text(x_pos + 0.01, y_pos, f'{speedup:.2f}x', ha='center', va='bottom',
                   fontsize=SPEEDUP_SIZE, rotation=90, weight='bold')

        # Speedup for µShell
        if baseline_val is not None and ushell_val is not None:
            speedup = ushell_val / baseline_val
            x_pos = x_positions[app_idx] + size_idx * (bar_width * 3) + bar_width * 2
            y_pos = ushell_val + ushell_err + 4

            ax.text(x_pos + 0.01, y_pos, f'{speedup:.2f}x', ha='center', va='bottom',
                   fontsize=SPEEDUP_SIZE, rotation=90, weight='bold')

# ===== AXIS FORMATTING =====
ax.set_ylabel('Throughput [MB/s]', fontsize=LABEL_SIZE)
ax.set_ylim(0, 275)
ax.set_yticks([0, 50, 100, 150, 200, 250])

# X-axis
group_centers = x_positions + (len(data_sizes) * bar_width * 3 - bar_width) / 2
ax.set_xticks(group_centers)
ax.set_xticklabels(display_names, ha='center', fontsize=LABEL_SIZE)

# Grid
ax.grid(True, alpha=0.3, axis='y', color='gray')
ax.set_axisbelow(True)

# Remove top and right spines
sns.despine(ax=ax)

# ===== LEGEND =====
from matplotlib.patches import Patch
legend_elements = []

# Data size colors
for size_idx, (size, color) in enumerate(zip(data_sizes, size_colors)):
    legend_elements.append(Patch(facecolor=color, edgecolor="k", alpha=1.0, label=size))  # Added edgecolor="k"

# Platform patterns
legend_elements.append(Patch(facecolor='white', edgecolor="k", alpha=1.0, hatch='//', label='Coyote'))  # Added edgecolor="k"
legend_elements.append(Patch(facecolor='white', edgecolor="k", alpha=1.0, hatch='\\\\', label='µShell_mono'))  # Added edgecolor="k"
legend_elements.append(Patch(facecolor='white', edgecolor="k", alpha=1.0, hatch='..', label='µShell'))  # Added edgecolor="k"

# Updated legend
ax.legend(handles=legend_elements, loc='upper right', frameon=True,
          ncol=2, bbox_to_anchor=(1, 1.08))

# ===== ANNOTATION =====
fig.suptitle('Higher is better ↑', fontsize=ANNOTATION_SIZE, color='navy',
             weight='bold', x=0.5, y=0.89)

# ===== SAVE AND DISPLAY =====
plt.tight_layout()
plt.subplots_adjust(bottom=0.1)
plt.savefig("../plots/e2e_6.1/e2e.png", dpi=300, bbox_inches='tight', pad_inches=0.01)
plt.savefig("../plots/e2e_6.1/e2e.pdf", bbox_inches='tight', pad_inches=0.01)
print("Figures generated at ../plots/e2e_6.1/e2e.png and ../plots/e2e_6.1/e2e.pdf")
# plt.show()