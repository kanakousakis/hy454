#!/usr/bin/env python3
"""
Measure exact enemy sprite coordinates by finding non-transparent regions
in specific areas of the sprite sheet based on the labeled layout.
"""

from PIL import Image

def is_transparent(pixel):
    """Check if pixel is transparent"""
    return len(pixel) >= 4 and pixel[3] < 10

def find_sprite_in_region(img, start_x, start_y, max_w, max_h, name=""):
    """Find a sprite's exact bounds within a region"""
    pixels = img.load()
    width, height = img.size

    min_x = width
    max_x = 0
    min_y = height
    max_y = 0
    found_pixel = False

    for y in range(start_y, min(start_y + max_h, height)):
        for x in range(start_x, min(start_x + max_w, width)):
            if not is_transparent(pixels[x, y]):
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
                found_pixel = True

    if not found_pixel:
        return None

    w = max_x - min_x + 1
    h = max_y - min_y + 1
    result = (min_x, min_y, w, h)

    if name:
        print(f"  {name:30s}: {{{min_x:4d}, {min_y:4d}, {w:3d}, {h:3d}}}")

    return result

def measure_enemies():
    """Measure enemy sprites based on the labeled layout"""
    img = Image.open("sonic_engine/assets/enemies_sheet_fixed.png").convert('RGBA')

    print("\n" + "="*70)
    print("ENEMY SPRITE COORDINATES (from labeled sheet)")
    print("="*70 + "\n")

    # Based on visual inspection of the labeled image:
    # Crabmeat is top-left, Motobug is center-top, etc.

    print("CRABMEAT / GANIGANI (Top-left):")
    # Crabmeat walk frames - 4 frames in a row
    find_sprite_in_region(img, 8, 8, 48, 40, "Walk frame 1")
    find_sprite_in_region(img, 60, 8, 48, 40, "Walk frame 2")
    find_sprite_in_region(img, 112, 8, 48, 40, "Walk frame 3")
    find_sprite_in_region(img, 164, 8, 48, 40, "Walk frame 4 (optional)")
    # Attack frame below
    find_sprite_in_region(img, 8, 50, 64, 40, "Attack frame")

    print("\nMOTOBUG / MOTORA (Top-center, red beetles):")
    # Motobug is to the right - around x=160-350
    find_sprite_in_region(img, 160, 8, 52, 40, "Move frame 1")
    find_sprite_in_region(img, 216, 8, 52, 40, "Move frame 2")
    find_sprite_in_region(img, 272, 8, 52, 40, "Move frame 3")
    find_sprite_in_region(img, 328, 8, 52, 40, "Move frame 4")

    print("\nBUZZ BOMBER / BEETON (Below Crabmeat):")
    # Buzz Bomber below Crabmeat
    find_sprite_in_region(img, 8, 90, 50, 40, "Fly frame 1")
    find_sprite_in_region(img, 60, 90, 50, 40, "Fly frame 2")
    find_sprite_in_region(img, 112, 90, 50, 40, "Shoot frame 1")
    find_sprite_in_region(img, 164, 90, 50, 40, "Shoot frame 2 (optional)")

    print("\nMASHER / BATABATA (Piranha):")
    # Masher is harder to locate, let me search around y=190-250
    find_sprite_in_region(img, 160, 190, 32, 48, "Idle")
    find_sprite_in_region(img, 196, 190, 32, 48, "Jump frame")

    print("\nNEWTRON BLUE (Chameleon):")
    find_sprite_in_region(img, 8, 250, 52, 40, "Appear frame 1")
    find_sprite_in_region(img, 64, 250, 52, 40, "Appear frame 2")
    find_sprite_in_region(img, 120, 250, 52, 40, "Idle")

    print("\nNEWTRON GREEN:")
    find_sprite_in_region(img, 8, 330, 52, 40, "Fly frame 1")
    find_sprite_in_region(img, 64, 330, 52, 40, "Fly frame 2")

    print("\nBOMB:")
    find_sprite_in_region(img, 8, 590, 36, 36, "Walk frame 1")
    find_sprite_in_region(img, 48, 590, 36, 36, "Walk frame 2")
    find_sprite_in_region(img, 88, 590, 36, 36, "Walk frame 3")
    find_sprite_in_region(img, 128, 590, 36, 36, "Walk frame 4")

    print("\n" + "="*70)

if __name__ == "__main__":
    measure_enemies()
