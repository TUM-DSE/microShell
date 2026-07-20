#!/bin/bash
# Spawn 10 parallel tmux sessions to bitgen the Figure 3 bitstreams:
#   5 direct (composed pipeline on 1 vFPGA)
#   5 cpu_sync (one module per vFPGA)
#
# Each session enters its build_<short>_<mode>/ dir and runs:
#   xilinx-shell -c "cmake ... && make project && make bitgen"
#
# Bitgen takes 3-4h per session. Sessions stay alive after bitgen so you can
# review the build via `tmux attach -t bitgen_<short>_<mode>`. Staging the
# resulting cyt_top.bit/ltx into <base>/bitstreams/<label>_top.{bit,ltx} is
# a SEPARATE step — see stage_bitstreams_effectiveness.sh. Bitgen can succeed
# yet produce a broken bitstream (timing failure, exit 0), so we don't auto-
# stage.

set -e

exit 0
: <<'EOF'

usage() {
    echo "Usage: $0 <baseline_base_dir>"
    echo ""
    echo "  baseline_base_dir  Path to baseline repo (must have examples_hw/)."
    echo ""
    echo "Spawns 10 tmux sessions named:"
    echo "  bitgen_audio_direct, bitgen_digital_direct, bitgen_secure_direct,"
    echo "  bitgen_signed_direct, bitgen_speech_direct,"
    echo "  bitgen_audio_cpu, bitgen_digital_cpu, bitgen_secure_cpu,"
    echo "  bitgen_signed_cpu, bitgen_speech_cpu"
    echo ""
    echo "Monitor:    tmux ls"
    echo "Attach:     tmux attach -t bitgen_audio_direct"
    echo "Detach:     Ctrl-b d"
    echo "After all sessions complete, run stage_bitstreams_effectiveness.sh"
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
modes=(direct cpu)

# Map short label → baseline's actual HW EXAMPLE name for each mode.
# direct = composed pipeline on a single vFPGA (baseline's existing apps).
# cpu    = each pipeline stage on its own vFPGA, host CPU shuttles between
#          stages — added to baseline's examples_hw/CMakeLists.txt as
#          *_cpu entries with N_STRM_AXI=2 (avoids the Vivado dataflow-opt
#          segfault that N_STRM_AXI=1 triggers).
declare -A direct_target=(
    [audio]=audio [digital]=digi [secure]=secure [signed]=signcomp [speech]=speech
)
declare -A cpu_target=(
    [audio]=audio_cpu [digital]=digi_cpu [secure]=secure_cpu [signed]=signcomp_cpu [speech]=speech_cpu
)

# Make sure all 10 build dirs exist.
for app in "${apps[@]}"; do
    for mode in "${modes[@]}"; do
        mkdir -p "$BASELINE_BASE/examples_hw/build_${app}_${mode}"
    done
done

echo "Spawning 10 bitgen tmux sessions (5 direct + 5 cpu) in $BASELINE_BASE"
echo "===================================================================="

for app in "${apps[@]}"; do
    for mode in "${modes[@]}"; do
        ex="${app}_${mode}"
        if [ "$mode" = "direct" ]; then
            cmake_target="${direct_target[$app]}"
        else
            cmake_target="${cpu_target[$app]}"
        fi
        session="bitgen_${ex}"
        build_dir="$BASELINE_BASE/examples_hw/build_${ex}"

        # Kill any prior session with the same name.
        tmux kill-session -t "$session" 2>/dev/null || true

        # Spawn session. Each runs cmake + make project + make bitgen via
        # xilinx-shell -c, then drops to a fresh interactive shell so the
        # session stays attached for review.
        tmux new-session -d -s "$session" -c "$build_dir" bash -c "
            export PATH=\$HOME/.local/cmake/bin:\$PATH
            LOG='$build_dir/bitgen.log'
            {
                echo '=== bitgen for $ex started '\$(date)' ==='
                xilinx-shell -c 'cmake ../ -DEXAMPLE=$cmake_target -DFDEV_NAME=u280 && make project && make bitgen'
                rc=\$?
                echo '=== bitgen for $ex ended '\$(date)' (exit '\$rc') ==='
            } 2>&1 | tee -a \"\$LOG\"
            exec bash
        "
        echo "  launched: $session  (build dir: $build_dir, target: $cmake_target)"
    done
done

echo ""
echo "All 10 sessions running. Useful commands:"
echo "  tmux ls                                 # list sessions"
echo "  tmux attach -t bitgen_audio_direct      # review one"
echo "  tmux kill-session -t bitgen_audio_direct  # cancel one"
echo ""
echo "Logs are also tee'd to <build_dir>/bitgen.log per session."
echo ""
echo "After all sessions report 'bitgen ... ended (exit 0)' and you've"
echo "reviewed the timing reports, run:"
echo "  bash stage_bitstreams_effectiveness.sh $BASELINE_BASE"

EOF
