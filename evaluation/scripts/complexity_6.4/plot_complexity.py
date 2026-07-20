"""Plot host-app complexity (lines of code) for the five composed apps,
comparing baseline vs µShell-composed vs µShell-monolithic.

Inputs : complexity_{baseline,ushell}_results.csv produced by
         measure_complexity_{baseline,ushell}.sh.

         The baseline CSV lives in the baseline repo's
         evaluation/data/complexity_6.4/, so pass its location with
         --baseline-csv if you're running this from the µShell repo.
         Defaults assume both CSVs are in the µShell repo's
         evaluation/data/complexity_6.4/ (e.g. you copied the baseline
         CSV over).

Outputs: evaluation/plots/complexity_6.4/complexity.{png,pdf}
"""

import argparse
import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "..", "data", "complexity_6.4"))
PLOTS = os.path.normpath(os.path.join(HERE, "..", "..", "plots", "complexity_6.4"))

parser = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--baseline-csv",
                    default=os.path.join(DATA, "complexity_baseline_results.csv"),
                    help="path to baseline complexity CSV "
                         "(default: <ushell>/evaluation/data/complexity_6.4/complexity_baseline_results.csv)")
parser.add_argument("--ushell-csv",
                    default=os.path.join(DATA, "complexity_ushell_results.csv"),
                    help="path to µShell complexity CSV "
                         "(default: <ushell>/evaluation/data/complexity_6.4/complexity_ushell_results.csv)")
args = parser.parse_args()

BASELINE_CSV = args.baseline_csv
USHELL_CSV   = args.ushell_csv

APPS = [
    "audio_processing",
    "digital_signature",
    "secure_storage",
    "signed_compression",
    "speech_recognition",
]
DISPLAY = {
    "audio_processing":   "Audio\nProcessing",
    "digital_signature":  "Digital\nSignature",
    "secure_storage":     "Secure\nStorage",
    "signed_compression": "Signed\nCompression",
    "speech_recognition": "Speech\nRecognition",
}

def load_latest(csv_path, variant):
    """Return {app_name: code_loc} using the most recent row per app."""
    if not os.path.exists(csv_path):
        sys.exit(f"Missing CSV: {csv_path}\n"
                 f"Run measure_complexity_*.sh first.")
    df = pd.read_csv(csv_path)
    df = df[df["variant"] == variant]
    if df.empty:
        sys.exit(f"No rows with variant='{variant}' in {csv_path}")
    df = df.sort_values("timestamp").drop_duplicates("app_name", keep="last")
    return dict(zip(df["app_name"], df["code"]))

baseline   = load_latest(BASELINE_CSV, "composed")
composed   = load_latest(USHELL_CSV,  "composed")
monolithic = load_latest(USHELL_CSV,  "monolithic")

missing = [a for a in APPS
           if a not in baseline or a not in composed or a not in monolithic]
if missing:
    sys.exit(f"CSV is missing rows for: {missing}")

baseline_vals   = [baseline[a]   for a in APPS]
composed_vals   = [composed[a]   for a in APPS]
monolithic_vals = [monolithic[a] for a in APPS]

# ----- plot --------------------------------------------------------
plt.rcParams.update({
    "font.size": 12,
    "axes.labelsize": 12,
    "xtick.labelsize": 11,
    "ytick.labelsize": 11,
    "legend.fontsize": 10,
})
palette = sns.color_palette("pastel")
c_baseline, c_composed, c_mono = palette[0], palette[2], palette[1]

x = np.arange(len(APPS))
w = 0.27

fig, ax = plt.subplots(figsize=(9.0, 3.6))
ax.bar(x - w, baseline_vals,   w, label="Baseline",         color=c_baseline, edgecolor="black", linewidth=0.8)
ax.bar(x,     composed_vals,   w, label="µShell (composed)", color=c_composed, edgecolor="black", linewidth=0.8)
ax.bar(x + w, monolithic_vals, w, label="µShell (mono)",     color=c_mono,     edgecolor="black", linewidth=0.8, hatch="//")

for i, vals in enumerate([baseline_vals, composed_vals, monolithic_vals]):
    offset = (i - 1) * w
    for xi, v in zip(x, vals):
        ax.text(xi + offset, v + 4, str(v), ha="center", va="bottom", fontsize=9)

ax.set_ylabel("Lines of code")
ax.set_xticks(x)
ax.set_xticklabels([DISPLAY[a] for a in APPS])
ax.set_ylim(0, max(monolithic_vals + composed_vals + baseline_vals) * 1.15)
ax.grid(True, axis="y", alpha=0.3, linestyle="-")
ax.set_axisbelow(True)
sns.despine(ax=ax)
ax.legend(loc="upper right", ncol=3, frameon=True, bbox_to_anchor=(1.0, 1.12))

os.makedirs(PLOTS, exist_ok=True)
plt.tight_layout()
plt.savefig(os.path.join(PLOTS, "complexity.pdf"), bbox_inches="tight")
plt.savefig(os.path.join(PLOTS, "complexity.png"), dpi=300, bbox_inches="tight")
print(f"wrote {PLOTS}/complexity.{{pdf,png}}")
