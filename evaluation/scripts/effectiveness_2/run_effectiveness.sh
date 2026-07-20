#!/bin/bash
# Driver for the direct_comm_effectiveness experiment (Figure 3).
#
# Runs each of the 5 paper apps in two modes — "direct" (composed pipeline
# on a single vFPGA) and "cpu_sync" (one module per vFPGA, host shuttling
# between stages) — and appends per-mode throughput/latency rows tagged
# source=measured to evaluation/data/effectiveness_2/effectiveness.csv.
#
# Bitstream lookup priority:
#   1. <base>/examples_hw/build_<short>_<mode>/bitstreams/cyt_top.bit (fresh
#      bitgen). If found, auto-stage to <base>/bitstreams/<label>_top.{bit,ltx}.
#   2. <base>/bitstreams/<label>_top.bit (previously staged).
#   3. Skip with warning if neither exists.
#
# Programming uses sudo bash ./program_fpga.sh <label>_top.
#
# Each ./test invocation is gated by a 60s timeout; on hang we kill and skip
# to the next (app, mode), which re-programs the FPGA on entry.
#
# Repeat reps: pass --reps via the SW path doesn't work (shared-sg + last
# flag race in the existing module SW). For multi-sample averaging, run
# this orchestrator multiple times — each invocation appends one row per
# (app, mode) to the CSV. plot_effectiveness.py averages across measured
# rows and uses std-dev as the error bar.

set -e

usage() {
    echo "Usage: $0 <baseline_base_dir>"
    echo ""
    echo "  baseline_base_dir  Path to baseline (with examples_hw/, examples_sw/,"
    echo "                     bitstreams/, driver/, program_fpga.sh, shell.nix)."
    echo ""
    echo "Auto-enters nix-shell at the baseline root if not already inside one."
    echo "Each run appends 1 row per (app,mode) — 10 rows total."
    echo "Run multiple times for averaged Fig 3 numbers."
    exit 1
}

if [ $# -lt 1 ]; then usage; fi
BASELINE_BASE=$(realpath "$1")

if [ ! -d "$BASELINE_BASE/examples_sw" ] || [ ! -f "$BASELINE_BASE/program_fpga.sh" ]; then
    echo "Error: $BASELINE_BASE doesn't look like a baseline repo"
    exit 1
fi

# Auto-wrap in nix-shell if we aren't already inside one. Re-execs the script
# with the same args from inside `nix-shell shell.nix` at the baseline root.
if [ -z "$IN_NIX_SHELL" ]; then
    if [ ! -f "$BASELINE_BASE/shell.nix" ]; then
        echo "Error: $BASELINE_BASE/shell.nix not found"
        echo "  Cannot enter nix-shell. Either install shell.nix in baseline or"
        echo "  run inside an existing nix-shell session."
        exit 1
    fi
    SCRIPT=$(realpath "$0")
    echo "[run_effectiveness] entering nix-shell at $BASELINE_BASE..."
    cd "$BASELINE_BASE"
    exec nix-shell shell.nix --run "bash '$SCRIPT' '$BASELINE_BASE'"
fi

EVAL_DIR="$(dirname "$(realpath "$0")")/../../data/effectiveness_2"
mkdir -p "$EVAL_DIR"
CSV_FILE="$EVAL_DIR/effectiveness.csv"
LOG_DIR="$EVAL_DIR/effectiveness_logs"
mkdir -p "$LOG_DIR"

# CSV schema: app, mode, throughput_MBps, latency_ns, n_reps, stddev_MBps,
# source, timestamp. The "source" column distinguishes paper-baseline rows
# (pre-populated with source=paper) from real measurements (source=measured),
# so plot_effectiveness.py can fall back to paper when no measurements exist
# for a given (app, mode) cell.
if [ ! -f "$CSV_FILE" ]; then
    echo "app,mode,throughput_MBps,latency_ns,n_reps,stddev_MBps,source,timestamp" > "$CSV_FILE"
fi

apps=("audio_processing" "digital_signature" "secure_storage" "signed_compression" "speech_recognition")
modes=("direct" "cpu_sync")

declare -A app_short=(
    [audio_processing]=audio
    [digital_signature]=digital
    [secure_storage]=secure
    [signed_compression]=signed
    [speech_recognition]=speech
)

# 1 MB transfers — matches the paper Figure 3 regime where PCIe overhead is
# amortised. Units differ by app: audio/speech use complex samples (8 B/sample
# = 131072 samples = 1 MB); the rest take bytes.
declare -A app_size=(
    [audio_processing]=131072
    [digital_signature]=1048576
    [secure_storage]=1048576
    [signed_compression]=1048576
    [speech_recognition]=131072
)

# Per-test timeout (seconds). Clean cpu_sync at 1 MB completes in <100 ms;
# 60 s is a safety guard for any HW-level hang.
TEST_TIMEOUT=60

extract_throughput() {
    echo "$1" | grep -E "^[[:space:]]*Throughput:" | head -1 \
        | sed -E 's/.*Throughput:[[:space:]]+([0-9]+\.?[0-9]*).*MB\/s.*/\1/'
}
extract_latency() {
    echo "$1" | grep -E "^[[:space:]]*Total[[:space:]]+latency:" | head -1 \
        | sed -E 's/.*Total[[:space:]]+latency:[[:space:]]+([0-9]+\.?[0-9]*)[[:space:]]+ns.*/\1/'
}

# Run a step silently. On failure, persist combined stdout+stderr to a log
# under $LOG_DIR and echo a one-line failure marker. Returns the command's
# exit code so the caller can decide whether to skip the iteration.
# Usage: run_step <human-label> <log-basename> <cmd...>
run_step() {
    local label="$1" basename="$2"; shift 2
    local tmplog persist rc
    tmplog=$(mktemp)
    set +e
    "$@" >"$tmplog" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ]; then
        rm -f "$tmplog"
        return 0
    fi
    persist="$LOG_DIR/${basename}_$(date +%Y%m%d_%H%M%S).log"
    mv "$tmplog" "$persist"
    echo "  $label FAILED (rc=$rc) — log: $persist"
    return "$rc"
}

# Best-effort driver unload (rmmod, then rmmod -f if the module is wedged).
unload_driver() {
    sudo rmmod coyote_drv 2>/dev/null && return 0
    sudo rmmod -f coyote_drv 2>/dev/null || true
}

# Resolve and stage bitstream for (app, mode). Echoes the program_fpga.sh
# label (e.g. "audio_direct_top") on success, empty on failure.
resolve_bitstream() {
    local app="$1" mode="$2"
    local mode_label="direct"; [ "$mode" = "cpu_sync" ] && mode_label="cpu"
    local label="${app_short[$app]}_${mode_label}_top"

    local fresh="$BASELINE_BASE/examples_hw/build_${app_short[$app]}_${mode_label}/bitstreams/cyt_top.bit"
    local fresh_ltx="${fresh%.bit}.ltx"
    local staged_bit="$BASELINE_BASE/bitstreams/${label}.bit"
    local staged_ltx="$BASELINE_BASE/bitstreams/${label}.ltx"

    if [ -f "$fresh" ]; then
        # Bitgen output present → auto-stage (overwrite any existing).
        cp "$fresh" "$staged_bit"
        [ -f "$fresh_ltx" ] && cp "$fresh_ltx" "$staged_ltx" || true
        echo "$label"
        return 0
    elif [ -f "$staged_bit" ]; then
        echo "$label"
        return 0
    else
        return 1
    fi
}

run_one() {
    local app="$1" mode="$2"
    # Baseline SW EXAMPLE names (short forms). digital_signature maps to
    # "digi_sign" in SW (unlike baseline HW which uses "digi"). The
    # *_cpu variants for cpu_sync mode were added on baseline alongside
    # the composed apps.
    declare -A sw_composed=(
        [audio_processing]=audio
        [digital_signature]=digi_sign
        [secure_storage]=secure
        [signed_compression]=signcomp
        [speech_recognition]=speech
    )
    declare -A sw_cpusync=(
        [audio_processing]=audio_cpu
        [digital_signature]=digi_sign_cpu
        [secure_storage]=secure_cpu
        [signed_compression]=signcomp_cpu
        [speech_recognition]=speech_cpu
    )
    local example_name="${sw_composed[$app]}"
    [ "$mode" = "cpu_sync" ] && example_name="${sw_cpusync[$app]}"

    echo ""
    echo "============================================================"
    echo "  $app / $mode"
    echo "============================================================"

    local label
    if ! label=$(resolve_bitstream "$app" "$mode"); then
        echo "  SKIP — no bitstream (build dir or staged) for $app/$mode"
        return
    fi
    echo "  bitstream:   $label.bit"

    cd "$BASELINE_BASE"
    echo "  programming FPGA..."
    if ! run_step "program_fpga" "${app}_${mode}_program" \
            sudo bash ./program_fpga.sh "$label"; then
        return
    fi

    # (Re)load driver explicitly — program_fpga.sh handles this in most
    # paths but we want a known-clean state after a possibly-hung run.
    unload_driver
    if ! run_step "insmod" "${app}_${mode}_insmod" \
            sudo insmod "$BASELINE_BASE/driver/coyote_drv.ko"; then
        return
    fi

    # Build SW for this example. The build dir is reused across runs;
    # cmake/make is incremental.
    local build_dir="$BASELINE_BASE/examples_sw/build_${app_short[$app]}_${mode/cpu_sync/cpu}"
    if [ ! -f "$build_dir/bin/test" ]; then
        echo "  building SW..."
        mkdir -p "$build_dir"
        if ! run_step "cmake" "${app}_${mode}_cmake" \
                bash -c "cd '$build_dir' && cmake ../ -DEXAMPLE='$example_name'"; then
            return
        fi
        if ! run_step "make"  "${app}_${mode}_make" \
                bash -c "cd '$build_dir' && make -j"; then
            return
        fi
    fi
    if [ ! -f "$build_dir/bin/test" ]; then
        echo "  SKIP — SW build did not produce $build_dir/bin/test"
        return
    fi

    # Run with timeout. `sudo` precedes `timeout` so timeout runs as root
    # and can SIGKILL the test child directly after the grace period.
    local size="${app_size[$app]}"
    local out rc persist
    set +e
    out=$(cd "$build_dir/bin" && sudo timeout --kill-after=5 "$TEST_TIMEOUT" ./test -s "$size" 2>&1)
    rc=$?
    set -e

    if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
        persist="$LOG_DIR/${app}_${mode}_timeout_$(date +%Y%m%d_%H%M%S).log"
        echo "$out" > "$persist"
        echo "  TIMEOUT after ${TEST_TIMEOUT}s — log: $persist"
        unload_driver
        return
    fi
    if [ "$rc" -ne 0 ]; then
        persist="$LOG_DIR/${app}_${mode}_fail_$(date +%Y%m%d_%H%M%S).log"
        echo "$out" > "$persist"
        echo "  test FAILED (rc=$rc) — log: $persist"
        return
    fi

    local thr lat
    thr=$(extract_throughput "$out")
    lat=$(extract_latency "$out")

    if [ -n "$thr" ] && [ -n "$lat" ]; then
        printf "  Throughput:  %s MB/s\n" "$thr"
        printf "  Latency:     %s ns\n"  "$lat"
        local ts; ts=$(date '+%Y-%m-%d %H:%M:%S')
        # n_reps=1, stddev_MBps=0 (single sample). Aggregation across rows
        # is done in plot_effectiveness.py.
        echo "$app,$mode,$thr,$lat,1,0,measured,$ts" >> "$CSV_FILE"
    else
        persist="$LOG_DIR/${app}_${mode}_parse_$(date +%Y%m%d_%H%M%S).log"
        echo "$out" > "$persist"
        echo "  WARN: could not parse metrics — log: $persist"
    fi
}

for app in "${apps[@]}"; do
    for mode in "${modes[@]}"; do
        run_one "$app" "$mode"
    done
done

echo ""
echo "Done. Rows appended to $CSV_FILE"
echo "Logs:        $LOG_DIR"
echo ""
echo "For averaged Figure 3 numbers, run this orchestrator 4 more times."
echo "Then: python3 plot_effectiveness.py  (from this same effectiveness_2/ dir)"
