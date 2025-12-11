#!/usr/bin/env python3
"""Find all sprite bounds in a sprite sheet"""

from PIL import Image
import sys

def is_transparent(pixel):
    """Check if pixel is transparent"""
    if len(pixel) < 4:
        return False
    return pixel[3] < 10  # Alpha < 10 = transparent

def find_all_sprites(filename, min_size=8):
    """Find all sprite bounding boxes in an image"""
    img = Image.open(filename).convert('RGBA')
    width, height = img.size
    pixels = img.load()

    print(f"\n{'='*60}")
    print(f"Analyzing: {filename}")
    print(f"Size: {width}x{height}")
    print(f"{'='*60}")

    # Create a visited map
    visited = [[False]*width for _ in range(height)]
    sprites = []

    def flood_fill(start_x, start_y):
        """Flood fill to find sprite bounds"""
        stack = [(start_x, start_y)]
        min_x = start_x
        max_x = start_x
        min_y = start_y
        max_y = start_y

        while stack:
            x, y = stack.pop()
            if x < 0 or x >= width or y < 0 or y >= height:
                continue
            if visited[y][x] or is_transparent(pixels[x, y]):
                continue

            visited[y][x] = True
            min_x = min(min_x, x)
            max_x = max(max_x, x)
            min_y = min(min_y, y)
            max_y = max(max_y, y)

            # Add neighbors
            stack.extend([(x+1,y), (x-1,y), (x,y+1), (x,y-1)])

        w = max_x - min_x + 1
        h = max_y - min_y + 1
        return (min_x, min_y, w, h)

    # Scan for sprites
    for y in range(height):
        for x in range(width):
            if not visited[y][x] and not is_transparent(pixels[x, y]):
                bounds = flood_fill(x, y)
                if bounds[2] >= min_size and bounds[3] >= min_size:
                    sprites.append(bounds)

    # Sort sprites by position (top to bottom, left to right)
    sprites.sort(key=lambda s: (s[1] // 50, s[0]))

    # Print sprites in rows
    print(f"\nFound {len(sprites)} sprites:\n")

    last_row = -1
    row_num = 0
    for i, (x, y, w, h) in enumerate(sprites):
        curr_row = y // 50  # Group by ~50px rows
        if curr_row != last_row:
            row_num += 1
            print(f"\n--- Row {row_num} (y~{y}) ---")
            last_row = curr_row

        print(f"  Sprite {i+1:2d}: {{x:{x:4d}, y:{y:4d}, w:{w:3d}, h:{h:3d}}}  // at ({x},{y})")

    # Also output as C++ struct format for easy copying
    print(f"\n\n{'='*60}")
    print("C++ Format (FrameRect):")
    print(f"{'='*60}")
    for i, (x, y, w, h) in enumerate(sprites[:20]):  # First 20 only
        print(f"{{{x:4d}, {y:4d}, {w:3d}, {h:3d}}},  // Sprite {i+1}")

if __name__ == "__main__":
    files = [
        "sonic_engine/assets/enemies_sheet_fixed.png",
        "sonic_engine/assets/misc_fixed.png",
    ]

    for f in files:
        try:
            find_all_sprites(f, min_size=12)
        except Exception as e:
            print(f"Error processing {f}: {e}")
