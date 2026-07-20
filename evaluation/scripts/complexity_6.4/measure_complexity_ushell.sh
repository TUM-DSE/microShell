#!/bin/bash
# Measure host-application complexity (LOC, comments, cyclomatic complexity)
# for the µShell composed apps and their monolithic variants using `scc`.
# Appends rows to evaluation/data/complexity_6.4/complexity_ushell_results.csv.
set -e

if [ $# -eq 0 ]; then
    echo "Usage: $0 <ushell_base_dir>"
    echo "  ushell_base_dir - Path to ushell base directory"
    echo "  Example: $0 /path/to/ushell"
    exit 1
fi

ushell_BASE=$(realpath "$1" 2>/dev/null) || { echo "Error: Invalid path: $1"; exit 1; }

if [ ! -d "$ushell_BASE/examples_sw/apps" ]; then
    echo "Error: $ushell_BASE/examples_sw/apps not found"
    exit 1
fi

if ! command -v scc >/dev/null 2>&1; then
    echo "Error: 'scc' not found in PATH."
    echo "  Install with: nix-shell -p scc"
    echo "  or download a release from https://github.com/boyter/scc/releases"
    echo "  then re-run this script."
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: 'jq' not found in PATH (needed to parse scc JSON)."
    echo "  Install with: nix-shell -p jq"
    exit 1
fi

CSV_FILE="$ushell_BASE/evaluation/data/complexity_6.4/complexity_ushell_results.csv"
mkdir -p "$(dirname "$CSV_FILE")"
if [ ! -f "$CSV_FILE" ]; then
    echo "app_name,variant,files,lines,blanks,comments,code,complexity,timestamp" > "$CSV_FILE"
fi

run_scc() {
    local app_name=$1
    local variant=$2
    local subdir=$3
    local app_dir="$ushell_BASE/examples_sw/apps/$subdir"

    if [ ! -d "$app_dir" ]; then
        echo "Warning: $app_dir not found — skipping"
        return
    fi

    local json
    json=$(scc --format json "$app_dir") || {
        echo "Warning: scc failed for $subdir"
        return
    }

    # Sum metrics across all languages reported by scc.
    local row
    row=$(echo "$json" | jq -r '
        reduce .[] as $l ({files:0,lines:0,blanks:0,comments:0,code:0,cx:0};
            .files+=($l.Count // 0)
            | .lines+=($l.Lines // 0)
            | .blanks+=($l.Blank // 0)
            | .comments+=($l.Comment // 0)
            | .code+=($l.Code // 0)
            | .cx+=($l.Complexity // 0))
        | "\(.files),\(.lines),\(.blanks),\(.comments),\(.code),\(.cx)"')

    local ts
    ts=$(date '+%Y-%m-%d %H:%M:%S')
    echo "$app_name,$variant,$row,$ts" >> "$CSV_FILE"

    printf '  %-22s [%-10s] files=%-2s lines=%-4s code=%-4s cx=%s\n' \
        "$app_name" "$variant" \
        "$(echo "$row" | cut -d, -f1)" \
        "$(echo "$row" | cut -d, -f2)" \
        "$(echo "$row" | cut -d, -f5)" \
        "$(echo "$row" | cut -d, -f6)"
}

echo "µShell host-app complexity"
echo "  base : $ushell_BASE"
echo "  csv  : $CSV_FILE"
echo "----"

# Map canonical long names → master's actual short SW dir names. Composed
# uses the short name; monolithic uses the short name + "_mono" suffix
# (matches master's examples_sw/apps/ layout).
declare -A ushell_sw_dir=(
    [audio_processing]=audio
    [digital_signature]=digi_sign
    [secure_storage]=secure
    [signed_compression]=signcomp
    [speech_recognition]=speech
)
for app in audio_processing digital_signature secure_storage signed_compression speech_recognition; do
    run_scc "$app" "composed"    "${ushell_sw_dir[$app]}"
    run_scc "$app" "monolithic"  "${ushell_sw_dir[$app]}_mono"
done

echo "----"
echo "Done. Rows appended to $CSV_FILE"
