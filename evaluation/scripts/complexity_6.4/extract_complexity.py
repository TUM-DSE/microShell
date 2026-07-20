"""Print the SLoC + CC programmability table (paper Table 5) from the
complexity_{baseline,ushell}_results.csv files produced by
measure_complexity_*.sh.

Companion to extract_util.py: reads the wide-format CSVs the measurement
scripts produce and prints a tidy summary. SLoC is reported as the
`lines` column (total file size, matching the paper); CC is the
`complexity` column. Parentheses show percent change relative to Coyote.
"""

import argparse
import csv
import os
import sys


HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.normpath(os.path.join(HERE, "..", "..", "data", "complexity_6.4"))

APPS = [
    ("audio_processing",   "Audio Processing"),
    ("digital_signature",  "Digital Signature"),
    ("secure_storage",     "Secure Storage"),
    ("signed_compression", "Signed Compression"),
    ("speech_recognition", "Speech Recognition"),
]


def load_latest(csv_path, variant):
    """Return {app_name: (lines, complexity)} using the most recent row
    per app for the given variant ('composed' or 'monolithic')."""
    if not os.path.exists(csv_path):
        sys.exit(f"Missing CSV: {csv_path}\nRun measure_complexity_*.sh first.")
    rows_by_app = {}
    with open(csv_path) as f:
        for row in csv.DictReader(f):
            if row.get("variant") != variant:
                continue
            app = row["app_name"]
            ts = row.get("timestamp", "")
            prev = rows_by_app.get(app)
            if prev is None or ts >= prev[0]:
                rows_by_app[app] = (ts, int(row["lines"]), int(row["complexity"]))
    return {app: (lines, cx) for app, (_, lines, cx) in rows_by_app.items()}


def fmt_delta(new, old):
    """Signed percent change as '(+X.X)' / '(-X.X)' to match the paper."""
    if old == 0:
        return "(n/a)"
    pct = (new - old) / old * 100.0
    return f"({pct:+.1f})"


def extract_complexity(baseline_csv, ushell_csv):
    coyote = load_latest(baseline_csv, "composed")
    ushell = load_latest(ushell_csv,  "composed")

    missing = [k for k, _ in APPS if k not in coyote or k not in ushell]
    if missing:
        sys.exit(f"CSV is missing 'composed' rows for: {missing}")

    print(f"{'Application':<22} {'Coyote SLoC':>12} {'µShell SLoC':>16} "
          f"{'Coyote CC':>11} {'µShell CC':>14}")
    print("-" * 78)
    for key, label in APPS:
        b_l, b_c = coyote[key]
        u_l, u_c = ushell[key]
        print(f"{label:<22} {b_l:>12} {u_l:>10} {fmt_delta(u_l, b_l):>5} "
              f"{b_c:>11} {u_c:>8} {fmt_delta(u_c, b_c):>5}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--baseline-csv",
                    default=os.path.join(DATA, "complexity_baseline_results.csv"))
    ap.add_argument("--ushell-csv",
                    default=os.path.join(DATA, "complexity_ushell_results.csv"))
    args = ap.parse_args()
    extract_complexity(args.baseline_csv, args.ushell_csv)
