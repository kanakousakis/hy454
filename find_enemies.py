#!/usr/bin/env python3
"""
Find enemy sprites by looking for reasonably-sized sprites
Filters out very large regions (likely labels/borders)
"""

from PIL import Image

def is_transparent(pixel):
    """Check if pixel is transparent"""
    return len(pixel) >= 4 and pixel[3] < 10

def find_enemy_sprites(filename, min_size=12, max_size=70):
    """Find all enemy sprite frames (filter out labels and huge regions)"""
    img = Image.open(filename).convert('RGBA')
    width, height = img.size
    pixels = img.load()

    print(f"\n{'='*70}")
    print(f"Finding Enemy Sprites: {filename}")
    print(f"Size: {width}x{height}")
    print(f"Filter: {min_size}px <= sprite size <= {max_size}px")
    print(f"{'='*70}\n")

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

            stack.extend([(x+1,y), (x-1,y), (x,y+1), (x,y-1)])

        w = max_x - min_x + 1
        h = max_y - min_y + 1
        return (min_x, min_y, w, h)

    # Scan for sprites
    for y in range(height):
        for x in range(width):
            if not visited[y][x] and not is_transparent(pixels[x, y]):
                bounds = flood_fill(x, y)
                w, h = bounds[2], bounds[3]
                # Filter: reasonable enemy size (not tiny, not huge)
                if min_size <= w <= max_size and min_size <= h <= max_size:
                    sprites.append(bounds)

    # Sort by position
    sprites.sort(key=lambda s: (s[1] // 50, s[0]))

    print(f"Found {len(sprites)} enemy-sized sprites:\n")

    # Group by rows
    last_row = -1
    row_sprites = []

    for i, (x, y, w, h) in enumerate(sprites):
        curr_row = y // 50
        if curr_row != last_row:
            if row_sprites:
                print()  # New line between rows
            last_row = curr_row
            row_num = curr_row + 1
            print(f"--- Row {row_num} (y ~ {y}) ---")
            row_sprites = []

        row_sprites.append((x, y, w, h))
        print(f"  {{{x:4d}, {y:4d}, {w:3d}, {h:3d}}},  // Sprite {i+1} at ({x},{y})")

    return sprites

if __name__ == "__main__":
    # Find enemy sprites
    enemies = find_enemy_sprites("sonic_engine/assets/enemies_sheet_fixed.png", min_size=20, max_size=65)

    # Also check misc sheet for comparison
    print("\n" + "="*70)
    find_enemy_sprites("sonic_engine/assets/misc_fixed.png", min_size=12, max_size=70)
