#!/usr/bin/env python3
"""
Sprite Sheet Analyzer
Analyzes sprite sheets to find non-green bounding boxes for each sprite.
"""

try:
    from PIL import Image
    import sys

    def is_green_background(pixel, tolerance=50):
        """Check if a pixel is green background (bright green)"""
        r, g, b = pixel[:3]
        # Bright green: high G, low R and B
        return r < tolerance and g > 180 and b < tolerance

    def find_sprite_bounds(img, start_x, start_y, max_w=100, max_h=100):
        """Find the bounding box of a sprite starting from a point"""
        width, height = img.size
        pixels = img.load()

        # Find the actual bounds
        min_x, min_y = start_x, start_y
        max_x, max_y = start_x, start_y

        # Scan outward to find sprite bounds
        for y in range(start_y, min(start_y + max_h, height)):
            for x in range(start_x, min(start_x + max_w, width)):
                pixel = pixels[x, y]
                if not is_green_background(pixel):
                    min_x = min(min_x, x)
                    min_y = min(min_y, y)
                    max_x = max(max_x, x)
                    max_y = max(max_y, y)

        w = max_x - min_x + 1
        h = max_y - min_y + 1
        return (min_x, min_y, w, h)

    def analyze_sheet(filename):
        """Analyze a sprite sheet and print sprite bounds"""
        print(f"\nAnalyzing: {filename}")
        try:
            img = Image.open(filename).convert('RGBA')
            width, height = img.size
            print(f"Size: {width}x{height}")

            # Count green vs non-green pixels
            pixels = img.load()
            green_count = 0
            non_green_count = 0

            for y in range(height):
                for x in range(width):
                    if is_green_background(pixels[x, y]):
                        green_count += 1
                    else:
                        non_green_count += 1

            print(f"Green pixels: {green_count}")
            print(f"Non-green pixels: {non_green_count}")

            # Find clusters of non-green pixels (sprites)
            print("\nLooking for sprites...")

            # Sample some known locations based on typical sprite sheet layouts
            # These are approximate starting points - will need manual adjustment
            test_points = [
                # Top-left area (usually first enemy)
                (10, 10, "Top-left sprite"),
                (60, 10, "Second frame"),
                (110, 10, "Third frame"),

                # Below first row
                (10, 50, "Row 2 sprite 1"),
                (60, 50, "Row 2 sprite 2"),

                # Right side
                (170, 10, "Right area sprite"),
                (220, 10, "Right area frame 2"),
            ]

            for x, y, desc in test_points:
                if x < width and y < height:
                    bounds = find_sprite_bounds(img, x, y, 60, 60)
                    if bounds[2] > 0 and bounds[3] > 0:
                        print(f"{desc:20s}: {{x:{bounds[0]:3d}, y:{bounds[1]:3d}, w:{bounds[2]:2d}, h:{bounds[3]:2d}}}")

        except Exception as e:
            print(f"Error: {e}")

    # Analyze the sprite sheets
    sheets = [
        "sonic_engine/assets/enemies_sheet_fixed.png",
        "sonic_engine/assets/misc_fixed.png",
    ]

    for sheet in sheets:
        analyze_sheet(sheet)

except ImportError:
    print("PIL/Pillow not installed. Install with: pip install Pillow")
    sys.exit(1)
