#!/bin/bash
# Stage Figure 11 bitstreams from per-build dirs into the labeled locations
# run_e2e.sh expects:
#   coyote      -> <baseline>/bitstreams/<short>_coyote_top.{bit,ltx}
#   ushell      -> <microshell>/bitstreams/<short>_ushell_top.{bit,ltx}
#   ushell_mono -> <microshell>/bitstreams/<short>_mono_top.{bit,ltx}
#
# Source priority per (short, system):
#   1. <base>/examples_hw/<build_dir>/bitstreams/cyt_top.bit (fresh bitgen).
#   2. (coyote only) <baseline>/bitstreams/<short>_direct_top.bit
#      — Figure 3 staged bitstream (same HDL content). Auto-renamed.
#   3. Pre-existing <base>/bitstreams/<label>_top.bit is left intact and
#      reported, but never overwritten when no fresh build exists. Useful
#      when the user supplies a manually-built bitstream.
#
# Run AFTER compile_hw_baseline.sh / compile_hw_ushell.sh have finished and
# you've reviewed the timing reports — bitgen exits 0 even on timing
# violations, so this is a manual gate.

set -e

exit 0
: <<'EOF'

usage() {
    echo "Usage: $0 <baseline_base_dir> <microshell_base_dir>"
    echo ""
    echo "Promotes cyt_top.{bit,ltx} from each build dir to the labeled"
    echo "location in <base>/bitstreams/. Falls back to Fig 3's staged"
    echo "<short>_direct_top.bit for the coyote system if no Fig 11 build"
    echo "exists, since the HDL content is identical."
    exit 1
}

if [ $# -lt 2 ]; then usage; fi
BASELINE_BASE=$(realpath "$1")
MSHELL_BASE=$(realpath "$2")

if [ ! -d "$BASELINE_BASE/examples_hw" ]; then
    echo "Error: $BASELINE_BASE/examples_hw not found"
    exit 1
fi
if [ ! -d "$MSHELL_BASE/examples_hw" ]; then
    echo "Error: $MSHELL_BASE/examples_hw not found"
    exit 1
fi

mkdir -p "$BASELINE_BASE/bitstreams" "$MSHELL_BASE/bitstreams"

declare -A app_short=(
    [audio_processing]=audio
    [digital_signature]=digital
    [secure_storage]=secure
    [signed_compression]=signed
    [speech_recognition]=speech
)
apps=(audio_processing digital_signature secure_storage signed_compression speech_recognition)

staged=0
fig3_fallback=0
preserved=0
missing=0

# stage_one <src_bit> <src_ltx> <dst_bit> <dst_ltx> <pretty_label>
stage_one() {
    local src_bit="$1" src_ltx="$2" dst_bit="$3" dst_ltx="$4" pretty="$5"
    cp "$src_bit" "$dst_bit"
    if [ -f "$src_ltx" ]; then
        cp "$src_ltx" "$dst_ltx"
        printf "  STAGED   %-22s -> %s\n" "$pretty" "$(basename "$dst_bit") + .ltx"
    else
        rm -f "$dst_ltx"
        printf "  STAGED   %-22s -> %s (no .ltx)\n" "$pretty" "$(basename "$dst_bit")"
    fi
}

# coyote (5): build_<short>_coyote → <short>_coyote_top
for short in audio digital secure signed speech; do
    pretty="${short}_coyote"
    src_bit="$BASELINE_BASE/examples_hw/build_${short}_coyote/bitstreams/cyt_top.bit"
    src_ltx="$BASELINE_BASE/examples_hw/build_${short}_coyote/bitstreams/cyt_top.ltx"
    fig3_bit="$BASELINE_BASE/bitstreams/${short}_direct_top.bit"
    fig3_ltx="$BASELINE_BASE/bitstreams/${short}_direct_top.ltx"
    dst_bit="$BASELINE_BASE/bitstreams/${short}_coyote_top.bit"
    dst_ltx="$BASELINE_BASE/bitstreams/${short}_coyote_top.ltx"

    if [ -f "$src_bit" ]; then
        stage_one "$src_bit" "$src_ltx" "$dst_bit" "$dst_ltx" "$pretty"
        staged=$((staged + 1))
    elif [ -f "$fig3_bit" ]; then
        stage_one "$fig3_bit" "$fig3_ltx" "$dst_bit" "$dst_ltx" "$pretty (from Fig 3)"
        fig3_fallback=$((fig3_fallback + 1))
    elif [ -f "$dst_bit" ]; then
        printf "  KEEP     %-22s : pre-existing %s untouched\n" "$pretty" "$(basename "$dst_bit")"
        preserved=$((preserved + 1))
    else
        printf "  MISSING  %-22s : no fresh, Fig 3, or pre-staged bitstream\n" "$pretty"
        missing=$((missing + 1))
    fi
done

# ushell + mono (10): <microshell>/examples_hw/build_<short>_<mode>/ → <short>_<mode>_top
for app in "${apps[@]}"; do
    short="${app_short[$app]}"
    for mode in ushell mono; do
        pretty="${short}_${mode}"
        src_bit="$MSHELL_BASE/examples_hw/build_${short}_${mode}/bitstreams/cyt_top.bit"
        src_ltx="$MSHELL_BASE/examples_hw/build_${short}_${mode}/bitstreams/cyt_top.ltx"
        dst_bit="$MSHELL_BASE/bitstreams/${short}_${mode}_top.bit"
        dst_ltx="$MSHELL_BASE/bitstreams/${short}_${mode}_top.ltx"

        if [ -f "$src_bit" ]; then
            stage_one "$src_bit" "$src_ltx" "$dst_bit" "$dst_ltx" "$pretty"
            staged=$((staged + 1))
        elif [ -f "$dst_bit" ]; then
            printf "  KEEP     %-22s : pre-existing %s untouched\n" "$pretty" "$(basename "$dst_bit")"
            preserved=$((preserved + 1))
        else
            printf "  MISSING  %-22s : no %s\n" "$pretty" "$src_bit"
            missing=$((missing + 1))
        fi
    done
done

echo ""
echo "Done: $staged staged from fresh bitgen, $fig3_fallback re-staged from Fig 3,"
echo "      $preserved kept (manual / prior staging), $missing missing."
echo "Bitstreams now at:"
echo "  $BASELINE_BASE/bitstreams/<short>_coyote_top.{bit,ltx}"
echo "  $MSHELL_BASE/bitstreams/<short>_ushell_top.{bit,ltx}"
echo "  $MSHELL_BASE/bitstreams/<short>_mono_top.{bit,ltx}"

EOF
