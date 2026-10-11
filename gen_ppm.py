#!/usr/bin/env python3
"""
Generate test PPM images for PSF sharpening benchmark.
Creates two images:
  - img_low.ppm: 400x300 (120,000 pixels)
  - img_high.ppm: 4000x3000 (12,000,000 pixels)

PPM format (P6): binary RGB image
  Header: "P6\n<width> <height>\n255\n"
  Data: RGB triplets (3 bytes per pixel)
"""

import sys
import os

def make_ppm(path, width, height):
    """Generate a PPM test image with gradient/pattern."""
    print(f"Generating {path}: {width}x{height} = {width*height} pixels...", end=' ', flush=True)
    
    with open(path, "wb") as f:
        # PPM header
        f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        
        # Generate RGB data: gradient pattern
        for y in range(height):
            for x in range(width):
                r = (x * 255) // width
                g = (y * 255) // height
                b = ((x + y) * 255) // (width + height)
                f.write(bytes((r, g, b)))
    
    file_size_mb = os.path.getsize(path) / (1024*1024)
    print(f"done ({file_size_mb:.2f} MB)")

if __name__ == "__main__":
    print("\n=== PPM Image Generator ===")
    
    # Low-resolution: 300x400 = 120,000 pixels
    make_ppm("img_low.ppm", 400, 300)
    
    # High-resolution: 3000x4000 = 12,000,000 pixels
    make_ppm("img_high.ppm", 4000, 3000)
    
    print("\nImages ready for benchmarking.")
