import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from matplotlib.patches import Patch

# ===== FONT AND STYLE SETTINGS =====
FONT_SIZE = 12
LABEL_SIZE = 12
TICK_SIZE = 11
LEGEND_SIZE = 10

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
color_compute = palette[1]      # Orange for Computation
color_data = palette[0]         # Blue for data management 
color_encoding = palette[2]     # Green for data encoding/encryption
color_networking = palette[3]   # Red/Pink for Networking
color_ml_matching = palette[4]  # Purple for ML/AI
color_misc = 'white'            # White for Misc.

# ===== APPLICATION NAMES =====
applications = [
    'Google\nBigQuery\n[40,41]',     # big data analytics 
    'Google\nBigTable\n[40,42]',
    'Stateful\nFirewall\n[69]',      # network functions
    'Intrusion\nDetection\n[69]',   
    'Audio\nDecoder\n[47,99]', 
    'Swarm\nPerception\n[48,99]', 
    'Speaker\nAuth.\n[18]',
    'Kalman\nFilter\n[18]',
    'Context\nDetection\n[92,101]',
    'Brain\nStimulat.\n[59]',
    'Video\nSync.\n[59]',
    'Audio\nProcessing\n[121]',
    'Digital\nSignature\n[96]',
    'Secure\nStorage\n[90]',
    'Signed\nCompress.\n[52]',
    'Speech\nRecogn.\n[95]'
]

# ===== MODULE DATA STRUCTURE =====
# Each module type with its color, hatch pattern, label, and counts per application
module_types = [
    # (counts, color, hatch, label)
    ([0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0], color_compute, '',     'Compute (LA)'),
    ([0, 0, 0, 0, 4, 1, 1, 0, 0, 1, 1, 2, 0, 0, 0, 1], color_compute, '\\\\', 'Compute (DSP)'),
    ([0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], color_compute, '..',   'Compute (Basic)'),
    ([2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], color_data,    '',     'Data (DB)'),
    ([1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], color_data,    '\\\\', 'Data (Transform)'),
    ([2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], color_data,    '..',   'Data (I/O)'),
    ([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0], color_encoding, '',     'Compression'),
    ([0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 1, 0], color_encoding, '\\\\', 'Cryptography'),
    ([0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], color_networking, '',   'Network (Com.)'),
    ([0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], color_networking, '..', 'Network (Sec.)'),
    ([0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1], color_ml_matching, '',  'ML/AI'),
    ([1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0], color_misc, '',         'Misc.')
]

# ===== PLOT SETUP =====
fig, ax = plt.subplots(figsize=(14.0, 3.0))

# Bar settings
bar_width = 0.34
x_positions = np.arange(len(applications))

# ===== PLOTTING STACKED BARS =====
for i, app in enumerate(applications):
    current_height = 0
    
    # Plot each module type for this application
    for counts, color, hatch, _ in module_types:
        module_count = counts[i]
        
        # Plot individual modules with separation lines
        for j in range(module_count):
            ax.bar(x_positions[i], 1, bar_width, 
                   bottom=current_height, 
                   color=color, 
                   edgecolor='black', 
                   linewidth=1, 
                   hatch=hatch,
                   alpha=1.0)
            current_height += 1

# ===== AXIS FORMATTING =====
# Y-axis
ax.set_ylabel('')
ax.set_ylim(0, 8)
ax.set_yticks([0, 2, 4, 6, 8])

# Custom y-axis label (rotated)
ax.text(-0.02, 0.5, 'Number of tasks', 
        transform=ax.transAxes,
        rotation=90, ha='center', va='center', 
        fontsize=LABEL_SIZE)

# X-axis
ax.set_xticks(x_positions)
ax.set_xticklabels(applications, ha='center')

# Grid
ax.grid(True, alpha=0.3, axis='y', color='gray', linestyle='-')
ax.set_axisbelow(True)

# Remove top and right spines
sns.despine(ax=ax)

# ===== LEGEND =====
legend_elements = [
    Patch(facecolor=color, edgecolor='black', hatch=hatch, 
          label=label, linewidth=1.0)
    for _, color, hatch, label in module_types
]

ax.legend(handles=legend_elements, 
          loc='upper right', 
          bbox_to_anchor=(1.00, 1.06),
          ncol=6, 
          frameon=True, 
          columnspacing=1.5)

# ===== LAYOUT AND SAVE =====
plt.tight_layout()
plt.margins(x=0.015, y=0.0, tight=True)

# Save figure
plt.savefig("../plots/modularity_2/application_modularity_analysis.pdf", bbox_inches='tight')
plt.savefig("../plots/modularity_2/application_modularity_analysis.png", dpi=300, bbox_inches='tight')
print("Figures generated at ../plots/modularity_2/application_modularity_analysis.png and ../plots/modularity_2/application_modularity_analysis.pdf")

# plt.show()
