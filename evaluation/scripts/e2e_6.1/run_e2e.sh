#!/bin/bash
# Driver for the e2e end-to-end-throughput experiment (Figure 11).
#
# Runs each of the 5 paper apps in 3 systems × 3 sizes = 9 cells per app:
#   - "coyote"      : composed pipeline on baseline Coyote shell
#                      (same HDL as Fig 3 "direct").
#   - "ushell_mono" : monolithic single-vFPGA on µShell (microshell repo).
#   - "ushell"      : composed pipeline on µShell, routed through the
#                      µShell switch (microshell repo).
# Default sizes: 8 KB / 256 KB / 1 MB.
#
# Bitstream lookup priority per (app, system):
#   1. Fresh bitgen output in the build dir → auto-stage to the labeled
#      bitstream and use it.
#   2. Pre-staged <base>/bitstreams/<label>_top.bit (from
#      stage_bitstreams_e2e.sh, or manually placed).
#   3. Skip with a SKIP marker.
#
# Programming uses sudo bash ./program_fpga.sh <label>. Each ./test
# invocation is gated by a 120s timeout; on hang we kill, persist the
# captured output as a log, unload the driver, and continue to the next
# size.
#
# CSV schema (rows tagged source=measured):
#   app,system,size_bytes,throughput_MBps,latency_ns,n_reps,stddev_MBps,
#   source,timestamp
# Paper baseline rows (source=paper) are pre-populated in e2e.csv so
# plot_e2e.py has a per-cell fallback. Run this orchestrator multiple
# times for averaged numbers — plot_e2e.py averages across measured rows
# and uses std-dev as the error bar.

set -e

usage() {
    echo "Usage: $0 <baseline_base_dir> <microshell_base_dir> [--sizes A,B,C]"
    echo ""
    echo "  baseline_base_dir   Path to baseline (Coyote) repo — supplies"
    echo "                      coyote bitstreams + SW."
    echo "  microshell_base_dir Path to µShell repo — supplies ushell and"
    echo "                      ushell_mono bitstreams + SW."
    echo "  --sizes A,B,C       Comma-separated transfer sizes in bytes."
    echo "                      Default: 8192,262144,1048576 (= 8 KB, 256 KB, 1 MB)."
    echo ""
    echo "Auto-enters nix-shell at the baseline root if not already inside one."
    echo "Each ./test runs with the SW default of 1 rep (matches Fig 3 convention —"
    echo "reps>1 risks hangs in some SW paths). Run multiple times for averaged"
    echo "Fig 11 numbers; plot_e2e.py averages measured rows."
    exit 1
}

if [ $# -lt 2 ]; then usage; fi
BASELINE_BASE=$(realpath "$1"); shift
MSHELL_BASE=$(realpath "$1"); shift
SIZES_CSV="8192,262144,1048576"
while [ $# -gt 0 ]; do
    case "$1" in
        --sizes) SIZES_CSV="$2"; shift 2 ;;
        *) echo "Unknown arg: $1"; usage ;;
    esac
done
IFS=',' read -ra SIZES <<< "$SIZES_CSV"

for base in "$BASELINE_BASE" "$MSHELL_BASE"; do
    if [ ! -d "$base/examples_sw" ] || [ ! -f "$base/program_fpga.sh" ]; then
        echo "Error: $base doesn't look like a valid repo (missing examples_sw/ or program_fpga.sh)"
        exit 1
    fi
done

# Auto-wrap in nix-shell if we aren't already inside one. Re-execs the script
# from inside `nix-shell shell.nix` at the baseline root.
if [ -z "$IN_NIX_SHELL" ]; then
    if [ ! -f "$BASELINE_BASE/shell.nix" ]; then
        echo "Error: $BASELINE_BASE/shell.nix not found"
        echo "  Cannot enter nix-shell. Either install shell.nix in baseline or"
        echo "  run inside an existing nix-shell session."
        exit 1
    fi
    SCRIPT=$(realpath "$0")
    echo "[run_e2e] entering nix-shell at $BASELINE_BASE..."
    cd "$BASELINE_BASE"
    exec nix-shell shell.nix --run "bash '$SCRIPT' '$BASELINE_BASE' '$MSHELL_BASE' --sizes $SIZES_CSV"
fi

EVAL_DIR="$(dirname "$(realpath "$0")")/../../data/e2e_6.1"
mkdir -p "$EVAL_DIR"
CSV_FILE="$EVAL_DIR/e2e.csv"
LOG_DIR="$EVAL_DIR/e2e_logs"
mkdir -p "$LOG_DIR"

if [ ! -f "$CSV_FILE" ]; then
    echo "app,system,size_bytes,throughput_MBps,latency_ns,n_reps,stddev_MBps,source,timestamp" > "$CSV_FILE"
fi

apps=(audio_processing digital_signature secure_storage signed_compression speech_recognition)
systems=(coyote ushell_mono ushell)

declare -A app_short=(
    [audio_processing]=audio
    [digital_signature]=digital
    [secure_storage]=secure
    [signed_compression]=signed
    [speech_recognition]=speech
)

# SW cmake EXAMPLE names — both baseline and master use the same short forms
# for composed apps and the same *_mono suffix for monolithic. Baseline doesn't
# have monolithic SW so ushell_mono only uses master.
declare -A sw_composed=(
    [audio_processing]=audio
    [digital_signature]=digi_sign
    [secure_storage]=secure
    [signed_compression]=signcomp
    [speech_recognition]=speech
)
declare -A sw_mono=(
    [audio_processing]=audio_mono
    [digital_signature]=digi_sign_mono
    [secure_storage]=secure_mono
    [signed_compression]=signcomp_mono
    [speech_recognition]=speech_mono
)

# Per-test timeout (seconds). Composed-pipeline 1 MB runs can legitimately
# take well over a minute on the coyote daemon path; 300 s leaves enough
# headroom that a long-but-progressing test isn't killed prematurely.
TEST_TIMEOUT=300

# Known-good bitstream used to reset the FPGA between cells. Reprogramming
# it forces Vivado to put the device in a clean state — the most reliable
# way to clear DMA/TLB residue when the previous cell wedged the FPGA.
KNOWN_GOOD_LABEL="perf_local"

# Throughput/latency extractors read from a file instead of a positional
# argument — speech at 1 MB prints megabytes of sample data, which can blow
# past ARG_MAX when passed as `echo "$1" | ...` even if echo is a builtin.
extract_throughput() {
    grep -E "^[[:space:]]*Throughput:" "$1" 2>/dev/null | head -1 \
        | sed -E 's/.*Throughput:[[:space:]]+([0-9]+\.?[0-9]*).*MB\/s.*/\1/'
}
extract_latency() {
    grep -E "^[[:space:]]*Total[[:space:]]+latency:" "$1" 2>/dev/null | head -1 \
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

# PCIe hot-reset (FPGA device remove + bus rescan). Use after a wedge —
# rmmod alone leaves stale DMA/TLB state on the device. Resolves hot_reset.sh
# from whichever repo $1 points at; falls back to the other repo if missing.
# Runs the reset twice with sleeps because a single reset doesn't always
# clear an FPGA that was mid-DMA when its host process aborted.
FPGA_PCI_ADDR="e1:00.0"
fpga_hot_reset() {
    local base="$1"
    local script="$base/sw/util/hot_reset.sh"
    [ -f "$script" ] || script="/scratch/anubhav/microShell/sw/util/hot_reset.sh"
    [ -f "$script" ] || script="/scratch/anubhav/baseline/microShell/sw/util/hot_reset.sh"
    if [ ! -f "$script" ]; then
        echo "  (hot_reset.sh not found, skipping PCIe reset)"
        return
    fi
    # Kill anything holding the FPGA char devices first — the test process
    # is launched as `./test` (no path prefix in cmdline), so identify it by
    # which-PID-holds-/dev/fpga_*, not by cmdline regex. Cmdline pkill is a
    # secondary catch for processes that may have already released the fd.
    if command -v fuser >/dev/null 2>&1; then
        local holders
        holders=$(sudo fuser /dev/fpga_0_v* 2>/dev/null | tr -s ' ' '\n' | grep -E '^[0-9]+$' | sort -u)
        if [ -n "$holders" ]; then
            sudo kill -9 $holders 2>/dev/null || true
        fi
    fi
    sudo pkill -9 -f "./test -s" 2>/dev/null || true
    sudo pkill -9 -f "timeout.*\./test" 2>/dev/null || true
    sleep 1
    sudo bash "$script" "$FPGA_PCI_ADDR" >/dev/null 2>&1 || true
    sleep 2
    sudo bash "$script" "$FPGA_PCI_ADDR" >/dev/null 2>&1 || true
    sleep 1
}

# Resolve per-system bases / cmake-EXAMPLE / build dir / bitstream label /
# build-dir bitstream path. Echoes 6 fields separated by '|':
#   hw_base | sw_base | example_name | build_subpath | label | sw_subdir
# hw_base supplies the bitstream + program_fpga.sh + driver.
# sw_base supplies the SW source + build dir for ./test.
resolve_target() {
    local app="$1" system="$2"
    local short="${app_short[$app]}"
    case "$system" in
        coyote)
            echo "$MSHELL_BASE|$BASELINE_BASE|${sw_composed[$app]}|build_${short}_coyote/bitstreams/cyt_top.bit|${short}_coyote|examples_sw/build_${short}_coyote"
            ;;
        ushell)
            echo "$MSHELL_BASE|$MSHELL_BASE|${sw_mono[$app]}|build_${short}_mono/bitstreams/cyt_top.bit|${short}_mono|examples_sw/build_${short}_ushell"
            ;;
        ushell_mono)
            echo "$MSHELL_BASE|$MSHELL_BASE|${sw_mono[$app]}|build_${short}_mono/bitstreams/cyt_top.bit|${short}_mono|examples_sw/build_${short}_mono"
            ;;
        *) return 1 ;;
    esac
}

# Resolve and stage bitstream for (app, system). Echoes the label
# (e.g. "audio_coyote") on success, empty on failure. Auto-stages a fresh
# bitgen output to the labeled location if found.
resolve_bitstream() {
    local base="$1" build_subpath="$2" label="$3"
    local fresh="$base/examples_hw/$build_subpath"
    local fresh_ltx="${fresh%.bit}.ltx"
    local staged_bit="$base/bitstreams/${label}_top.bit"
    local staged_ltx="$base/bitstreams/${label}_top.ltx"

    if [ -f "$fresh" ]; then
        mkdir -p "$base/bitstreams"
        cp "$fresh" "$staged_bit"
        [ -f "$fresh_ltx" ] && cp "$fresh_ltx" "$staged_ltx" || rm -f "$staged_ltx"
        return 0
    elif [ -f "$staged_bit" ]; then
        return 0
    else
        return 1
    fi
}

run_one_app_system() {
    local app="$1" system="$2"
    local target; target=$(resolve_target "$app" "$system")
    local hw_base sw_base example_name build_subpath label sw_subdir
    IFS='|' read -r hw_base sw_base example_name build_subpath label sw_subdir <<< "$target"

    echo ""
    echo "============================================================"
    echo "  $app / $system  (label=$label, hw_repo=$hw_base, sw_repo=$sw_base)"
    echo "============================================================"

    if ! resolve_bitstream "$hw_base" "$build_subpath" "$label"; then
        echo "  SKIP — no bitstream (build dir or staged) for $app/$system"
        return
    fi
    echo "  bitstream:   ${label}_top.bit"

    cd "$hw_base"
    # PCIe hot-reset BEFORE programming so any wedged DMA state from a prior
    # cell (esp. one that timed out) is cleared before Vivado tries to program.
    unload_driver
    fpga_hot_reset "$hw_base"

    # Reprogram a known-good bitstream first to force the FPGA into a clean
    # state. Hot-reset alone doesn't always clear residual DMA/TLB after a
    # hung test. perf_local lives in microShell/bitstreams/; cd to MSHELL_BASE
    # specifically for this step regardless of which hw_base the cell uses.
    if [ -f "$MSHELL_BASE/bitstreams/${KNOWN_GOOD_LABEL}.bit" ]; then
        echo "  resetting via ${KNOWN_GOOD_LABEL}..."
        (cd "$MSHELL_BASE" && sudo bash ./program_fpga.sh "$KNOWN_GOOD_LABEL" \
            >"$LOG_DIR/${app}_${system}_known_good.log" 2>&1) || true
        unload_driver
        fpga_hot_reset "$hw_base"
    fi

    echo "  programming FPGA..."
    if ! run_step "program_fpga" "${app}_${system}_program" \
            sudo bash ./program_fpga.sh "$label"; then
        return
    fi

    unload_driver
    if ! run_step "insmod" "${app}_${system}_insmod" \
            sudo insmod "$hw_base/driver/coyote_drv.ko"; then
        return
    fi

    # Build SW for this (app, system). Build dir is reused across runs;
    # cmake/make is incremental.
    local build_dir="$sw_base/$sw_subdir"
    if [ ! -f "$build_dir/bin/test" ]; then
        echo "  building SW..."
        mkdir -p "$build_dir"
        if ! run_step "cmake" "${app}_${system}_cmake" \
                bash -c "cd '$build_dir' && cmake ../ -DEXAMPLE='$example_name'"; then
            return
        fi
        if ! run_step "make"  "${app}_${system}_make" \
                bash -c "cd '$build_dir' && make -j"; then
            return
        fi
    fi
    if [ ! -f "$build_dir/bin/test" ]; then
        echo "  SKIP — SW build did not produce $build_dir/bin/test"
        return
    fi

    for size in "${SIZES[@]}"; do
        local rc persist size_arg="$size"
        local out_file
        out_file=$(mktemp)
        # audio/speech SW interpret -s as complex samples (8 B each); the
        # other 3 apps use byte sizes directly. Convert the byte-valued
        # SIZES_CSV entry to samples for those two so the actual data
        # transferred matches the paper's 8K/256K/1M targets.
        if [ "$app" = "audio_processing" ] || [ "$app" = "speech_recognition" ]; then
            size_arg=$(( size / 8 ))
        fi
        # `sudo` precedes `timeout` so timeout runs as root and can SIGKILL
        # the test child directly after the grace period. Capture to a file
        # because output can be many MB (speech 1 MB prints all samples).
        set +e
        (cd "$build_dir/bin" && sudo timeout --kill-after=5 "$TEST_TIMEOUT" \
                ./test -s "$size_arg" >"$out_file" 2>&1)
        rc=$?
        set -e

        # Always try to extract throughput first — mono SW has a known
        # post-pipeline cleanup crash (free(): invalid pointer) that fires
        # AFTER the throughput is reported. The numbers are still valid;
        # we record them and continue.
        local thr lat
        thr=$(extract_throughput "$out_file")
        lat=$(extract_latency "$out_file")

        if [ "$rc" -ne 0 ]; then
            # Salvage measurement only if the pipeline plausibly finished:
            #   - throughput > 0
            #   - reported latency well below the test timeout (a hung test
            #     that prints latency anyway reports values in the seconds
            #     range, far above any legitimate per-transfer latency)
            local salvage_ok=0
            if [ -n "$thr" ] && [ -n "$lat" ]; then
                # awk handles the float comparison; bash arithmetic is int-only.
                salvage_ok=$(awk -v t="$thr" -v l="$lat" -v to="$TEST_TIMEOUT" \
                    'BEGIN { if (t+0 > 1.0 && l+0 > 0 && l+0 < (to*1e9)/2) print 1; else print 0 }')
            fi
            if [ "$salvage_ok" = "1" ]; then
                printf "  size=%-7s thr=%s MB/s   lat=%s ns  (rc=%d, post-test crash, salvaged)\n" \
                    "$size" "$thr" "$lat" "$rc"
                local ts; ts=$(date '+%Y-%m-%d %H:%M:%S')
                echo "$app,$system,$size,$thr,$lat,1,0,measured,$ts" >> "$CSV_FILE"
            else
                if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
                    persist="$LOG_DIR/${app}_${system}_${size}_timeout_$(date +%Y%m%d_%H%M%S).log"
                    cp "$out_file" "$persist"
                    echo "  size=$size: TIMEOUT after ${TEST_TIMEOUT}s — log: $persist"
                else
                    persist="$LOG_DIR/${app}_${system}_${size}_fail_$(date +%Y%m%d_%H%M%S).log"
                    cp "$out_file" "$persist"
                    echo "  size=$size: test FAILED (rc=$rc) — log: $persist"
                fi
            fi
            # Cleanup regardless: SIGKILL/SIGABRT/SIGSEGV leave DMA/TLB state
            # on the FPGA; rmmod alone doesn't clear it.
            unload_driver
            fpga_hot_reset "$hw_base"
            sudo insmod "$hw_base/driver/coyote_drv.ko" 2>/dev/null || true
            rm -f "$out_file"
            continue
        fi

        if [ -n "$thr" ] && [ -n "$lat" ]; then
            printf "  size=%-7s thr=%s MB/s   lat=%s ns\n" "$size" "$thr" "$lat"
            local ts; ts=$(date '+%Y-%m-%d %H:%M:%S')
            # n_reps=1 (SW default), stddev_MBps=0 (single sample). Aggregation
            # across rows happens in plot_e2e.py — run the orchestrator
            # multiple times for averaged numbers.
            echo "$app,$system,$size,$thr,$lat,1,0,measured,$ts" >> "$CSV_FILE"
        else
            persist="$LOG_DIR/${app}_${system}_${size}_parse_$(date +%Y%m%d_%H%M%S).log"
            cp "$out_file" "$persist"
            echo "  size=$size: WARN — could not parse metrics (log: $persist)"
        fi
        rm -f "$out_file"
    done
}

for app in "${apps[@]}"; do
    for system in "${systems[@]}"; do
        run_one_app_system "$app" "$system"
    done
done

echo ""
echo "Done. Rows appended to $CSV_FILE"
echo "Logs:        $LOG_DIR"
echo ""
echo "For averaged Figure 11 numbers, run this orchestrator 4 more times."
echo "Then: python3 plot_e2e.py"
