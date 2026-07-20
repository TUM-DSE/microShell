#!/bin/bash
# Copy bitgen outputs from per-build dirs into <base>/bitstreams/ with the
# labelled naming convention (audio_direct_top.bit etc.) that
# program_fpga.sh and run_effectiveness.sh expect. Sources are preserved
# in the build dirs so re-staging is a no-op (matches stage_bitstreams_e2e.sh).
#
# Run AFTER compile_bitgen_effectiveness.sh has finished and you've reviewed
# the timing reports. Bitgen exits 0 even on timing violations, so this is a
# manual gate — staging here means "I trust this build".

set -e

exit 0
: <<'EOF'

usage() {
    echo "Usage: $0 <baseline_base_dir>"
    echo ""
    echo "Copies cyt_top.{bit,ltx} from each build_<short>_<mode>/bitstreams/"
    echo "to <base>/bitstreams/<short>_<mode>_top.{bit,ltx}, replacing any"
    echo "previously-staged file (source is preserved)."
    exit 1
}

if [ $# -lt 1 ]; then usage; fi
BASELINE_BASE=$(realpath "$1")

if [ ! -d "$BASELINE_BASE/examples_hw" ]; then
    echo "Error: $BASELINE_BASE/examples_hw not found"
    exit 1
fi

mkdir -p "$BASELINE_BASE/bitstreams"

apps=(audio digital secure signed speech)
modes=(direct cpu)

staged=0
missing=0

for app in "${apps[@]}"; do
    for mode in "${modes[@]}"; do
        ex="${app}_${mode}"
        src_dir="$BASELINE_BASE/examples_hw/build_${ex}/bitstreams"
        src_bit="$src_dir/cyt_top.bit"
        src_ltx="$src_dir/cyt_top.ltx"
        dst_bit="$BASELINE_BASE/bitstreams/${ex}_top.bit"
        dst_ltx="$BASELINE_BASE/bitstreams/${ex}_top.ltx"

        if [ -f "$src_bit" ]; then
            cp "$src_bit" "$dst_bit"
            if [ -f "$src_ltx" ]; then
                cp "$src_ltx" "$dst_ltx"
                printf "  staged %-22s -> %s\n" "$ex" "${ex}_top.{bit,ltx}"
            else
                printf "  staged %-22s -> %s (no .ltx)\n" "$ex" "${ex}_top.bit"
            fi
            staged=$((staged + 1))
        else
            printf "  MISSING %-22s : no %s\n" "$ex" "$src_bit"
            missing=$((missing + 1))
        fi
    done
done

echo ""
echo "Done: $staged staged, $missing missing."
echo "Bitstreams now at: $BASELINE_BASE/bitstreams/<label>_top.{bit,ltx}"

EOF
