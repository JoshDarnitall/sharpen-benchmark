# PSF Sharpening Benchmark - Raspberry Pi

Benchmark suite for single-threaded PSF (Point Spread Function) convolution on Raspberry Pi.

Compares CPU usage and latency for:
- **Low-resolution**: 300×400 pixels = 120,000 pixels
- **High-resolution**: 3000×4000 pixels = 12,000,000 pixels

Each resolution runs 10+ iterations to observe sustained CPU behavior.

## Files

- **sharpen.c**: Core PSF convolution algorithm, parameterized by resolution and iteration count
- **gen_ppm.py**: Generates test PPM images (low & high resolution)
- **benchmark.sh**: Main benchmark runner; pauses for htop observation between tests
- **README.md**: This file

## Quick Start

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install build-essential python3
```

### Run Benchmark

```bash
chmod +x benchmark.sh gen_ppm.py
./benchmark.sh
```

The script will:
1. Compile `sharpen.c`
2. Generate test images (img_low.ppm, img_high.ppm)
3. Pause before each benchmark run
4. **You open htop in another terminal to observe CPU usage**
5. Run 10 iterations on low-res, then 10 on high-res

## Observations During Benchmark

### How to Use htop

Open a second terminal and run:

```bash
htop
```

Key things to watch:

- **CPU bar at top**: Shows load per core
- **Process list**: Look for `sharpen` process
- **CPU%**: Should show ~100% for one core during execution
- **Num cores**: Compare low-res vs high-res to verify single-threaded behavior

### Expected Output

#### Low-Res (120K pixels)

```
=== PSF SHARPEN BENCHMARK: 300x400_120000_pixels ===
File I/O complete at 0.014 seconds
  Iteration  1: 1.234 ms
  Iteration  2: 1.245 ms
  Iteration  3: 1.256 ms
  ...
  Iteration 10: 1.250 ms

Total time for 10 iterations: 0.013 seconds
Average per iteration: 1.250 ms
```

**htop observation**: 
- One CPU core shows ~100% usage
- Other 3 cores (on Pi 4) show ~0-5% (idle/background)
- Process: `sharpen`

#### High-Res (12M pixels)

```
=== PSF SHARPEN BENCHMARK: 4000x3000_12000000_pixels ===
File I/O complete at 3.456 seconds
  Iteration  1: 125.45 ms
  Iteration  2: 125.67 ms
  Iteration  3: 125.52 ms
  ...
  Iteration 10: 125.61 ms

Total time for 10 iterations: 1.256 seconds
Average per iteration: 125.60 ms
```

**htop observation**:
- Same one CPU core at ~100% usage
- Other cores idle
- **Much longer per-frame latency** (~100-125ms vs ~1-2ms)
- Runtime is approximately 100× slower than low-res

## Measurement Details

### Per-Frame Latency (syslog)

Each iteration is logged to syslog:

```bash
tail -f /var/log/syslog | grep sharpen-bench
```

Output format:

```
Oct 11 14:32:01 pi sharpen_bench[1234]: ITER_START 300x400_120000_pixels iter=1
Oct 11 14:32:01 pi sharpen_bench[1234]: ITER_END 300x400_120000_pixels iter=1 time_ns=1234567890 time_ms=1.235
```

### CPU Core Analysis

**Single-threaded behavior** means:
- Only one logical CPU core is active
- On Raspberry Pi 4 (4 cores): 3 cores remain idle during execution
- CPU% should show 100% on one core, ~0% on others

**Multi-threaded SMP** (future enhancement):
- Would distribute work across multiple cores
- htop would show multiple cores at high %CPU
- Expected speedup: up to 4× on quad-core Pi

## Calculations

### Pixel Count

- **Low-res**: 300 × 400 = 120,000 pixels
- **High-res**: 3,000 × 4,000 = 12,000,000 pixels
- **Ratio**: 12M ÷ 120K = 100× more pixels

### Work per Pixel

PSF convolution performs 9 multiplications + accumulation per pixel per channel (RGB), so:

- Low-res: 120K × 3 channels × 9 ops = 3.24M operations per frame
- High-res: 12M × 3 channels × 9 ops = 324M operations per frame
- Expected latency ratio: ~100× (matches pixel count)

## Next Steps: SMP (4-Thread) Version

To parallelize across all 4 Raspberry Pi cores:

1. Modify `sharpen.c` to use `pthread`
2. Divide image rows equally among 4 threads
3. Create `sharpen_smp.c` with same interface
4. Run: `./benchmark.sh smp` (future)
5. Compare htop: all 4 cores should now be active
6. Measure speedup: target ~3-4× improvement

## References

- **PSF Reference**: http://www.dspguide.com/pdfbook.htm
- **Raspberry Pi**: https://www.raspberrypi.org/
- **htop Manual**: `man htop` or https://htop.dev/
- **PPM Format**: http://netpbm.sourceforge.net/doc/ppm.html

## Author Notes

Based on original code by Sam Siewert (July 16, 2020).  
Modified for benchmarking and SMP analysis on ARM (Raspberry Pi).
