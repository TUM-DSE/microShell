import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import seaborn as sns
from matplotlib.patches import Patch

# ===== FONT AND STYLE SETTINGS =====
FONT_SIZE = 8
LABEL_SIZE = 7
TICK_SIZE = 7
LEGEND_SIZE = 6
ANNOTATION_SIZE = 7
VALUE_SIZE = 6

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
color1 = palette[0]  # Blue for object cap
color2 = palette[1]  # Orange for mem cap
color3 = palette[2]  # Green for buffer alloc
color4 = palette[3]  # Red for partial reconfig

# ===== DATA CONFIGURATION =====
vFPGA_count = ["1 UL", "2 ULs", "3 ULs", "4 ULs"]
x_positions = np.arange(len(vFPGA_count))

# Time values in microseconds — read from the merged reconfig_times.csv that
# extract_reconfig.py produces from reconfig_{pr,cap}.log. Rows are: 0 PR
# time, 1 buffer alloc, 2 object capability, 3 memory capability.
# # Paper data, kept for reference:
# pr_time = [58000, 58000, 58000, 58000]
# buffer_alloc = [253.1, 253.1, 253.1, 253.1]
# object_alloc = [3.6, 3.6*2, 3.6*3, 3.6*4]
# mem_values = [2, 2*2, 2*3, 2*4]

CSV_FILE = "../data/deployment_6.3/reconfig_times.csv"

df = pd.read_csv(CSV_FILE)

pr_time = df.iloc[0].tolist()
buffer_alloc = df.iloc[1].tolist()
object_alloc = df.iloc[2].tolist()
mem_values = df.iloc[3].tolist()

# Y-axis limits for broken axis
y_limits = [
    (55000, 61000),                # Top axis for large values
    (200, buffer_alloc[0] + 50),   # Middle axis (auto-scaled to buffer)
    (0, 25)                        # Bottom axis for small values
]

# ===== PLOT SETUP =====
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharex=True, figsize=(3.7, 2))
axs = [ax1, ax2, ax3]

# Bar settings
bar_width = 0.18

# Common bar properties
bar_props = {
    'alpha': 1.0,
    'edgecolor': 'k',
    'linewidth': 1.0
}

# ===== PLOTTING BARS =====
for ax in axs:
    # Coyote bars (left side)
    # Buffer allocation (bottom)
    ax.bar(x_positions - 0.5*bar_width, buffer_alloc, bar_width,
           color=color3, hatch='//', label='buffer alloc', **bar_props)

    # Partial reconfiguration (stacked on buffer)
    ax.bar(x_positions - 0.5*bar_width, pr_time, bar_width,
           bottom=np.array(buffer_alloc),
           color=color4, hatch='//', label='partial reconfig', **bar_props)

    # uShell bars (right side)
    # Object capability (bottom)
    ax.bar(x_positions + 0.5*bar_width, object_alloc, bar_width,
           color=color1, hatch='\\\\', label='object cap', **bar_props)

    # Memory capability (stacked on object)
    ax.bar(x_positions + 0.5*bar_width, mem_values, bar_width,
           bottom=np.array(object_alloc),
           color=color2, hatch='\\\\', label='mem cap', **bar_props)

    # Buffer allocation (stacked on top)
    ax.bar(x_positions + 0.5*bar_width, buffer_alloc, bar_width,
           bottom=np.array(object_alloc) + np.array(mem_values),
           color=color3, hatch='\\\\', **bar_props)

# ===== AXIS FORMATTING =====
# Set y-axis limits for broken axis effect
for ax, (ymin, ymax) in zip(axs, y_limits):
    ax.set_ylim(ymin, ymax)

# Set specific y-ticks to avoid overlap
ax1.set_yticks([56000, 60000])
ax2.set_yticks([250, 350])
ax3.set_yticks([0, 20, 40])

# Configure broken axis appearance
# Hide spines between subplots
ax1.spines['bottom'].set_visible(False)
ax1.spines['top'].set_visible(False)
ax2.spines['top'].set_visible(False)
ax2.spines['bottom'].set_visible(False)
ax3.spines['top'].set_visible(False)

# Hide x-axis for upper plots
ax1.get_xaxis().set_visible(False)
ax2.get_xaxis().set_visible(False)

# Add diagonal lines to indicate breaks
d = 0.5  # Proportion of vertical to horizontal extent of the slanted line
kwargs = dict(marker=[(-1, -d), (1, d)], markersize=10,
              linestyle="none", color='k', mec='k', mew=2, clip_on=False)

ax1.plot([0, 1], [0, 0], transform=ax1.transAxes, **kwargs)
ax2.plot([0, 1], [1, 1], transform=ax2.transAxes, **kwargs)
ax2.plot([0, 1], [0, 0], transform=ax2.transAxes, **kwargs)
ax3.plot([0, 1], [1, 1], transform=ax3.transAxes, **kwargs)

# Set x-axis labels
ax3.set_xticks(x_positions)
ax3.set_xticklabels(vFPGA_count, fontsize=TICK_SIZE)

# Set y-axis label (centered on middle subplot)
ax2.set_ylabel("Time (μs)", fontsize=LABEL_SIZE)
ax2.yaxis.set_label_coords(-0.18, 0.1)

# Grid for all subplots
for ax in axs:
    ax.grid(True, alpha=0.3, axis='y', color='gray')
    ax.set_axisbelow(True)

# Note: Right spines are now visible (removed the code that was hiding them)

# ===== LEGEND =====
# Create custom legend elements
legend_elements = [
    Patch(facecolor=color1, edgecolor="k", hatch='', alpha=1.0, label='Object cap'),
    Patch(facecolor=color2, edgecolor="k", hatch='', alpha=1.0, label='Mem cap'),
    Patch(facecolor=color3, edgecolor="k", hatch='', alpha=1.0, label='Buffer alloc'),
    Patch(facecolor=color4, edgecolor="k", hatch='', alpha=1.0, label='Reconfig'),
    Patch(facecolor='white', edgecolor="k", hatch='//', alpha=1.0, label='Coyote'),
    Patch(facecolor='white', edgecolor="k", hatch='\\\\', alpha=1.0, label='μShell')
]

# Position legend at the top
ax1.legend(handles=legend_elements,
          loc='upper right',
          frameon=True,
          ncol=3,
          bbox_to_anchor=(1.025, 2.4),
          prop={'size': LEGEND_SIZE},
          columnspacing=0.4)

# ===== ANNOTATION =====
ax1.text(0.5, 1.15, 'Lower is better ↓',
        transform=ax1.transAxes,
        color='navy',
        weight='bold',
        fontsize=ANNOTATION_SIZE,
        ha='center',
        va='top',
        zorder=10)

# ===== LAYOUT AND SAVE =====
plt.tight_layout()
plt.subplots_adjust(wspace=0, hspace=0.05)
plt.margins(x=0.0, tight=True)

# Save figure
plt.savefig("../plots/deployment_6.3/reconfig_overhead.png", dpi=300, bbox_inches='tight', pad_inches=0.01)
plt.savefig("../plots/deployment_6.3/reconfig_overhead.pdf", bbox_inches='tight', pad_inches=0.01)
print("Figures generated at ../plots/deployment_6.3/reconfig_overhead.png and ../plots/deployment_6.3/reconfig_overhead.pdf")

