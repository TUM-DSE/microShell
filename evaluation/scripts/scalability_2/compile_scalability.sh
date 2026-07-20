#!/bin/bash
# Spawn 4 parallel tmux sessions to bitgen Figure 4 scalability bitstreams
# on baseline. Each builds a different N-vFPGA shell variant (1, 2, 4, 8).
#
# Each session runs:
#   xilinx-shell -c "cmake ../ -DEXAMPLE=<n>vfpga -DFDEV_NAME=u280 \
#                    && make project && make bitgen"
# Bitgen takes 3-4h per session. Sessions stay alive after bitgen so you can
# review the build via `tmux attach -t bitgen_<n>vfpga`. Each build's
# util_<n>vfpga.csv (Vivado hierarchical utilization dump) lives inside
# <baseline>/examples_hw/<n>vfpga/ once a manual `source extract_util.tcl`
# step is run on the routed checkpoint — plot_scalability.py reads from
# there (or from data/scalability_2/ if you copy it out).

set -e

exit 0
: <<'EOF'

if [ -z "$1" ]; then
    echo "Usage: $0 <path_to_baseline_directory>"
    exit 1
fi

BASE_DIR=$(realpath "$1")

if [ ! -d "$BASE_DIR/examples_hw" ]; then
    echo "Error: $BASE_DIR/examples_hw not found"
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

examples=("1vfpga" "2vfpga" "4vfpga" "8vfpga")

# Make sure all 4 build dirs exist (incremental builds; never rm -rf).
for ex in "${examples[@]}"; do
    mkdir -p "$BASE_DIR/examples_hw/$ex"
done

echo "Spawning 4 bitgen tmux sessions in $BASE_DIR"
echo "=============================================="

for ex in "${examples[@]}"; do
    session="bitgen_${ex}"
    build_dir="$BASE_DIR/examples_hw/$ex"

    # Kill any prior session with the same name.
    tmux kill-session -t "$session" 2>/dev/null || true

    tmux new-session -d -s "$session" -c "$build_dir" bash -c "
        export PATH=\$HOME/.local/cmake/bin:\$PATH
        LOG='$build_dir/bitgen.log'
        {
            echo '=== bitgen for $ex started '\$(date)' ==='
            xilinx-shell -c 'cmake ../ -DEXAMPLE=$ex -DFDEV_NAME=u280 && make project && make bitgen'
            rc=\$?
            echo '=== bitgen for $ex ended '\$(date)' (exit '\$rc') ==='
        } 2>&1 | tee -a \"\$LOG\"
        exec bash
    "
    echo "  launched: $session  (build dir: $build_dir)"
done

echo ""
echo "All 4 sessions running. Useful commands:"
echo "  tmux ls                            # list sessions"
echo "  tmux attach -t bitgen_1vfpga       # review one"
echo "  tmux kill-session -t bitgen_1vfpga # cancel one"
echo ""
echo "Logs are also tee'd to <build_dir>/bitgen.log per session."
echo ""
echo "After each session reports 'bitgen ... ended (exit 0)' and you've"
echo "reviewed the timing reports, open the routed checkpoint in Vivado"
echo "and run \`source extract_util.tcl\` to produce util_<n>vfpga.csv."
echo "Then: python3 plot_scalability.py"

EOF
