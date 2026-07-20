#!/bin/bash
# Spawn 5 parallel tmux sessions to bitgen Figure 11's `coyote` bitstreams in
# the baseline (Coyote) repo. Same HDL as Figure 3's "direct" mode, but
# built in a Figure 11-specific build dir (build_<short>_coyote/) and staged
# under a Figure 11-specific label so the two figures' artifacts don't alias.
#
# If you've already run Figure 3's compile_bitgen_effectiveness.sh, you can
# skip this and let stage_bitstreams_e2e.sh fall back to Fig 3's staged
# <short>_direct_top.bit.
#
# Each session runs:
#   xilinx-shell -c "cmake ../ -DEXAMPLE=<baseline_composed_target> -DFDEV_NAME=u280 \
#                    && make project && make bitgen"
# where the target is baseline's composed-mode EXAMPLE name (audio, digi,
# secure, signcomp, speech). The mapping from short label → target is
# below in the hw_target map.
# Bitgen takes 3-4h per session. Sessions stay alive after bitgen for review;
# `tmux attach -t bitgen_<short>_coyote` to inspect timing reports. Promotion
# to <baseline>/bitstreams/<short>_coyote_top.{bit,ltx} is a separate step
# (stage_bitstreams_e2e.sh) — bitgen can succeed yet produce a timing-broken
# bitstream (exit 0), so we don't auto-stage.

set -e

exit 0
: <<'EOF'

usage() {
    echo "Usage: $0 <baseline_base_dir>"
    echo ""
    echo "  baseline_base_dir  Path to baseline repo (must have examples_hw/)."
    echo ""
    echo "Spawns 5 tmux sessions named:"
    echo "  bitgen_audio_coyote, bitgen_digital_coyote, bitgen_secure_coyote,"
    echo "  bitgen_signed_coyote, bitgen_speech_coyote"
    echo ""
    echo "Monitor:    tmux ls"
    echo "Attach:     tmux attach -t bitgen_audio_coyote"
    echo "Detach:     Ctrl-b d"
    echo "After all sessions complete, run stage_bitstreams_e2e.sh"
    exit 1
}

if [ $# -lt 1 ]; then usage; fi
BASELINE_BASE=$(realpath "$1")

if [ ! -d "$BASELINE_BASE/examples_hw" ]; then
    echo "Error: $BASELINE_BASE/examples_hw not found"
    exit 1
fi

if ! command -v tmux &>/dev/null; then
    echo "Error: tmux is required"
    exit 1
fi

if ! command -v xilinx-shell &>/dev/null; then
    echo "Error: xilinx-shell is required (Vivado env wrapper)"
    exit 1
fi

apps=(audio digital secure signed speech)

# Map short label → baseline's actual HW EXAMPLE name (composed, single-vFPGA
# pipeline = Fig 11 coyote = Fig 3 direct). Baseline's HW CMakeLists uses
# "digi" for digital signature, not "digi_sign".
declare -A hw_target=(
    [audio]=audio [digital]=digi [secure]=secure [signed]=signcomp [speech]=speech
)

# Make sure all 5 build dirs exist.
for short in "${apps[@]}"; do
    mkdir -p "$BASELINE_BASE/examples_hw/build_${short}_coyote"
done

echo "Spawning 5 bitgen tmux sessions in $BASELINE_BASE"
echo "================================================="

for short in "${apps[@]}"; do
    session="bitgen_${short}_coyote"
    build_dir="$BASELINE_BASE/examples_hw/build_${short}_coyote"
    cmake_target="${hw_target[$short]}"

    # Kill any prior session with the same name.
    tmux kill-session -t "$session" 2>/dev/null || true

    tmux new-session -d -s "$session" -c "$build_dir" bash -c "
        export PATH=\$HOME/.local/cmake/bin:\$PATH
        LOG='$build_dir/bitgen.log'
        {
            echo '=== bitgen for ${short}_coyote started '\$(date)' ==='
            xilinx-shell -c 'cmake ../ -DEXAMPLE=$cmake_target -DFDEV_NAME=u280 && make project && make bitgen'
            rc=\$?
            echo '=== bitgen for ${short}_coyote ended '\$(date)' (exit '\$rc') ==='
        } 2>&1 | tee -a \"\$LOG\"
        exec bash
    "
    echo "  launched: $session  (build dir: $build_dir, target: $cmake_target)"
done

echo ""
echo "All 5 sessions running. Useful commands:"
echo "  tmux ls                                    # list sessions"
echo "  tmux attach -t bitgen_audio_coyote         # review one"
echo "  tmux kill-session -t bitgen_audio_coyote   # cancel one"
echo ""
echo "Logs are also tee'd to <build_dir>/bitgen.log per session."
echo ""
echo "After all sessions report 'bitgen ... ended (exit 0)' and you've"
echo "reviewed the timing reports, run:"
echo "  bash stage_bitstreams_e2e.sh $BASELINE_BASE <microshell_base_dir>"

EOF
