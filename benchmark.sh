#!/bin/bash

# ============================================================================
# PSF Sharpening Benchmark Suite
# Single-threaded performance on Raspberry Pi
# 
# Measures:
#  - Low-res (300x400 = 120K pixels) sharpened 10+ times
#  - High-res (3000x4000 = 12M pixels) sharpened 10+ times
#  - CPU usage and core count via htop
#  - Per-iteration latency via syslog
#
# Usage:
#   ./benchmark.sh          # Run full benchmark
#   ./benchmark.sh clean    # Remove artifacts
# ============================================================================

set -e

BIN="./sharpen"
LOW_INPUT="img_low.ppm"
HIGH_INPUT="img_high.ppm"
LOW_OUTPUT="out_low.ppm"
HIGH_OUTPUT="out_high.ppm"
RUNS=10

echo "\n=================================================================="
echo "PSF SHARPENING BENCHMARK - Single-Threaded"
echo "=================================================================="

# Clean up old runs
if [ "$1" = "clean" ]; then
    echo "Cleaning up..."
    rm -f "$BIN" "$LOW_OUTPUT" "$HIGH_OUTPUT" *.ppm
    exit 0
fi

# Step 1: Compile
echo "\n[1/3] Compiling sharpen.c..."
if gcc -O3 -Wall -std=c11 -lm sharpen.c -o "$BIN"; then
    echo "  ✓ Binary ready: $BIN"
else
    echo "  ✗ Compilation failed"
    exit 1
fi

# Step 2: Generate test images
echo "\n[2/3] Generating PPM test images..."
if python3 gen_ppm.py; then
    echo "  ✓ Test images ready"
else
    echo "  ✗ PPM generation failed"
    exit 1
fi

# Step 3: Run benchmarks
echo "\n[3/3] Running benchmarks..."
echo ""
echo "IMPORTANT: Open htop in another terminal BEFORE each benchmark run:"
echo "  $ htop"
echo ""
echo "Key observations:"
echo "  - Single-threaded: one core should be ~100% busy, others idle"
echo "  - Low-res (120K pixels): should be fast (milliseconds per frame)"
echo "  - High-res (12M pixels): should be ~100x slower (100-1000ms per frame)"
echo ""

# Benchmark: Low-resolution
echo "------------------------------------------------------------------"
echo "LOW-RESOLUTION BENCHMARK: 300x400 = 120,000 pixels"
echo "Running $RUNS iterations..."
echo "------------------------------------------------------------------"
echo ""
echo ">>> OPEN htop NOW and watch CPU usage <<<"
echo ""
read -p "Press ENTER when ready to start LOW-RES benchmark..." dummy
echo ""

logger -t sharpen-bench "START LOW-RES benchmark (300x400, $RUNS iterations)"
time $BIN 300 400 $RUNS "$LOW_INPUT" "$LOW_OUTPUT"
logger -t sharpen-bench "END LOW-RES benchmark"

echo ""
echo "Check htop now - you should see:"
echo "  • One core at ~100% CPU"
echo "  • Other cores idle (near 0%)"
echo "  • Process name: sharpen"
echo ""

# Benchmark: High-resolution
echo ""
echo "------------------------------------------------------------------"
echo "HIGH-RESOLUTION BENCHMARK: 3000x4000 = 12,000,000 pixels"
echo "Running $RUNS iterations..."
echo "------------------------------------------------------------------"
echo ""
echo ">>> OPEN htop NOW and watch CPU usage <<<"
echo ""
read -p "Press ENTER when ready to start HIGH-RES benchmark..." dummy
echo ""

logger -t sharpen-bench "START HIGH-RES benchmark (3000x4000, $RUNS iterations)"
time $BIN 3000 4000 $RUNS "$HIGH_INPUT" "$HIGH_OUTPUT"
logger -t sharpen-bench "END HIGH-RES benchmark"

echo ""
echo "Check htop now - you should see:"
echo "  • One core at ~100% CPU"
echo "  • Other cores idle (near 0%)"
echo "  • Process name: sharpen"
echo "  • MUCH longer runtime than low-res (approximately 100x slower)"
echo ""

# Summary
echo ""
echo "=================================================================="
echo "BENCHMARK COMPLETE"
echo "=================================================================="
echo ""
echo "Results summary:"
echo ""
echo "To view syslog timestamps (per-frame latency):"
echo "  $ grep sharpen-bench /var/log/syslog | tail -50"
echo ""
echo "Or live:"
echo "  $ tail -f /var/log/syslog | grep sharpen-bench"
echo ""
echo "Expected observations:"
echo "  LOW-RES:  ~milliseconds per iteration, 1 core active"
echo "  HIGH-RES: ~100x slower, 1 core active throughout"
echo ""
