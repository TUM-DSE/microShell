import pandas
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import seaborn as sns
from matplotlib.patches import Patch

# ===== FONT AND STYLE SETTINGS =====
FONT_SIZE = 14
LABEL_SIZE = 14
TICK_SIZE = 12
LEGEND_SIZE = 12
ANNOTATION_SIZE = 12
PERCENTAGE_SIZE = 10

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
color1 = palette[0]  # Blue for Coyote
color2 = palette[1]  # Orange for uShell

# ===== HATCH PATTERNS =====
hatch1 = "//"   # Coyote
hatch2 = "\\\\"  # uShell

# ===== DATA LOADING =====
data1 = pandas.read_csv("../data/scheduling_6.2/sched_latency.csv")
data2 = pandas.read_csv("../data/scheduling_6.2/sched_reconfig.csv")
data3 = pandas.read_csv("../data/scheduling_6.2/sched_resp_avg.csv")
data4 = pandas.read_csv("../data/scheduling_6.2/sched_resp_95.csv")
data5 = pandas.read_csv("../data/scheduling_6.2/sched_deadline.csv")

# ===== PERCENTAGE CALCULATION FUNCTIONS =====
def latency_percentage(data):
    """Calculate latency percentages relative to Coyote baseline"""
    return [
        round(data.loc[0, 'Latency[ms]'] / data.loc[0, 'Latency[ms]'] * 100, 1),
        round(data.loc[1, 'Latency[ms]'] / data.loc[1, 'Latency[ms]'] * 100, 1),
        round(data.loc[2, 'Latency[ms]'] / data.loc[2, 'Latency[ms]'] * 100, 1),
        round(data.loc[3, 'Latency[ms]'] / data.loc[0, 'Latency[ms]'] * 100, 1),
        round(data.loc[4, 'Latency[ms]'] / data.loc[1, 'Latency[ms]'] * 100, 1),
        round(data.loc[5, 'Latency[ms]'] / data.loc[2, 'Latency[ms]'] * 100, 1)
    ]

def config_percentage(data):
    """Calculate reconfig count percentages relative to Coyote baseline"""
    return [
        round(data.loc[0, 'Reconfig count'] / data.loc[0, 'Reconfig count'] * 100, 1),
        round(data.loc[1, 'Reconfig count'] / data.loc[1, 'Reconfig count'] * 100, 1),
        round(data.loc[2, 'Reconfig count'] / data.loc[2, 'Reconfig count'] * 100, 1),
        round(data.loc[3, 'Reconfig count'] / data.loc[0, 'Reconfig count'] * 100, 1),
        round(data.loc[4, 'Reconfig count'] / data.loc[1, 'Reconfig count'] * 100, 1),
        round(data.loc[5, 'Reconfig count'] / data.loc[2, 'Reconfig count'] * 100, 1)
    ]

def deadline_percentage(data):
    """Calculate deadline miss percentages relative to Coyote baseline"""
    return [
        round(data.loc[0, 'Deadline misses'] / data.loc[0, 'Deadline misses'] * 100, 1),
        round(data.loc[1, 'Deadline misses'] / data.loc[1, 'Deadline misses'] * 100, 1),
        round(data.loc[2, 'Deadline misses'] / data.loc[2, 'Deadline misses'] * 100, 1),
        round(data.loc[3, 'Deadline misses'] / data.loc[0, 'Deadline misses'] * 100, 1),
        round(data.loc[4, 'Deadline misses'] / data.loc[1, 'Deadline misses'] * 100, 1),
        round(data.loc[5, 'Deadline misses'] / data.loc[2, 'Deadline misses'] * 100, 1)
    ]

# ===== DATA CONFIGURATION =====
# Calculate percentages for all subplots
percentage_data = [
    latency_percentage(data1),
    config_percentage(data2),
    latency_percentage(data3),
    latency_percentage(data4),
    deadline_percentage(data5)
]

# Subplot configurations
subplot_configs = [
    (data1, "Latency[ms]", "(a) Latency"),
    (data2, "Reconfig count", "(b) Reconfig. count"),
    (data3, "Latency[ms]", "(c) Avg. response time"),
    (data4, "Latency[ms]", "(d) Tail response time"),
    (data5, "Deadline misses", "(e) Missed deadlines")
]

# Y-axis limits for each subplot
y_limits = {
    "(a) Latency": (0, 2000),
    "(b) Reconfig. count": (0, 40),
    "(c) Avg. response time": (0, 900),
    "(d) Tail response time": (0, 1800),
    "(e) Missed deadlines": (0, 35)
}

# ===== PLOT SETUP =====
width = 16  
height = 3
fig, axs = plt.subplots(nrows=1, ncols=5, sharex=True, figsize=(width, height), 
                        constrained_layout=True)

# Bar settings
bar_width = 0.35
x_values = [8, 12, 16]  # Number of tasks
x_positions = np.arange(len(x_values))

# Common bar properties
bar_props = {
    'alpha': 1.0,
    'edgecolor': 'k',
    'linewidth': 1
}

# ===== PLOTTING BARS =====
for subplot_idx, (ax, (data, y_label, title)) in enumerate(zip(axs, subplot_configs)):
    # Get unique policies in this dataset
    unique_policies = data['Policy'].unique()
    
    # Extract data for each policy
    if len(unique_policies) >= 2:
        policy1 = unique_policies[0]
        policy2 = unique_policies[1]
    else:
        policy1 = 'Coyote'
        policy2 = 'uShell'
    
    # Get data for each policy
    policy1_data = data[data['Policy'] == policy1]
    policy2_data = data[data['Policy'] == policy2]
    
    # Collect values for each number of tasks
    policy1_values = []
    policy2_values = []
    
    for tasks in x_values:
        # Get value for policy1
        val1 = policy1_data[policy1_data['Number of tasks'] == tasks][y_label].values
        policy1_values.append(val1[0] if len(val1) > 0 else 0)
        
        # Get value for policy2
        val2 = policy2_data[policy2_data['Number of tasks'] == tasks][y_label].values
        policy2_values.append(val2[0] if len(val2) > 0 else 0)
    
    # Plot bars
    bars1 = ax.bar(x_positions - bar_width/2, policy1_values, bar_width,
                    color=color1, hatch=hatch1, label=policy1, **bar_props)
    
    bars2 = ax.bar(x_positions + bar_width/2, policy2_values, bar_width,
                    color=color2, hatch=hatch2, label=policy2, **bar_props)

# ===== PERCENTAGE ANNOTATIONS =====
for subplot_idx, ax in enumerate(axs):
    percentages = percentage_data[subplot_idx]
    bar_idx = 0
    
    # Annotate policy1 bars
    for bar in ax.containers[0]:
        if bar.get_height() > 0:
            ax.text(bar.get_x() + bar.get_width()/2 + 0.03,
                   bar.get_height() + (ax.get_ylim()[1] - ax.get_ylim()[0]) * 0.01,
                   f'{percentages[bar_idx]:.0f}%',
                   ha='center', va='bottom',
                   fontsize=PERCENTAGE_SIZE,
                   rotation=90,
                   weight='bold')
        bar_idx += 1
    
    # Annotate policy2 bars
    for bar in ax.containers[1]:
        if bar.get_height() > 0:
            ax.text(bar.get_x() + bar.get_width()/2 + 0.03,
                   bar.get_height() + (ax.get_ylim()[1] - ax.get_ylim()[0]) * 0.01,
                   f'{percentages[bar_idx]:.0f}%',
                   ha='center', va='bottom',
                   fontsize=PERCENTAGE_SIZE,
                   rotation=90,
                   weight='bold')
        bar_idx += 1

# ===== AXIS FORMATTING =====
for ax, (data, y_label, title) in zip(axs, subplot_configs):
    # Set title and labels
    ax.set_title(title, weight="bold", fontsize=FONT_SIZE)
    ax.set_ylabel(y_label)
    
    # Set x-axis
    ax.set_xticks(x_positions)
    ax.set_xticklabels(x_values)
    ax.set_xlabel('')
    
    # Set y-axis limits
    if title in y_limits:
        ax.set_ylim(y_limits[title])
    
    # Grid
    ax.grid(True, alpha=0.3, axis='y', color='gray')
    ax.set_axisbelow(True)
    
    # Remove top and right spines
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)

# X-axis shared label
fig.supxlabel("Number of instances", fontsize=LABEL_SIZE, y=-0.065)

# ===== LEGEND =====
# Get actual policy names from first dataset
actual_policies = []
for policy in subplot_configs[0][0]['Policy'].unique():
    if policy not in actual_policies:
        actual_policies.append(policy)

# Create legend elements
legend_elements = []

# First policy (Coyote)
if len(actual_policies) > 0:
    legend_elements.append(Patch(facecolor=color1, edgecolor="k", alpha=1.0, 
                                 hatch=hatch1, label=actual_policies[0]))

# Second policy (uShell)
if len(actual_policies) > 1:
    legend_elements.append(Patch(facecolor=color2, edgecolor="k", alpha=1.0, 
                                 hatch=hatch2, label='µShell'))  

# Position legend
fig.legend(handles=legend_elements, 
          loc="lower right", 
          bbox_to_anchor=(1, -0.1), 
          frameon=True, 
          ncol=2)

# ===== ANNOTATION =====
fig.text(0.78, -0.065, "Lower is better ↓", 
         fontsize=ANNOTATION_SIZE, 
         color="navy", 
         weight="bold", 
         ha='center', va='bottom')

# ===== SAVE AND DISPLAY =====
plt.savefig("../plots/scheduling_6.2/sched.png", dpi=300, bbox_inches='tight', pad_inches=0.01)
plt.savefig("../plots/scheduling_6.2/sched.pdf", bbox_inches='tight', pad_inches=0.01)
print("Figures generated at ../plots/scheduling_6.2/sched.png and ../plots/scheduling_6.2/sched.pdf")
