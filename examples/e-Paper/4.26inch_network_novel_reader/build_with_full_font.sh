#!/bin/bash
# build_with_full_font.sh
# 
# One-click script to generate full HZK16 font and build the project
#
# Usage:
#   ./build_with_full_font.sh

set -e  # Exit on error

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "=========================================="
echo "Building with Full HZK16 Font"
echo "=========================================="
echo ""

# Check if HZK16 file exists
if [ ! -f "HZK16" ]; then
    echo "Error: HZK16 file not found!"
    echo ""
    echo "Please download HZK16 first:"
    echo "  wget https://github.com/aguegu/BitmapFont/raw/master/font/HZK16"
    echo ""
    exit 1
fi

echo "Step 1: Generating full HZK16 font..."
echo "  This will generate ~6,763 characters (~230KB)"
echo ""

python3 tools/generate_full_hzk16.py HZK16 hzk16_full.c

if [ ! -f "hzk16_full.c" ]; then
    echo "Error: Failed to generate hzk16_full.c"
    exit 1
fi

echo ""
echo "Step 2: Backing up original font file..."

if [ -f "lib/Fonts/hzk16.c" ]; then
    if [ ! -f "lib/Fonts/hzk16_default.c.bak" ]; then
        cp lib/Fonts/hzk16.c lib/Fonts/hzk16_default.c.bak
        echo "  Backup created: lib/Fonts/hzk16_default.c.bak"
    else
        echo "  Backup already exists, skipping..."
    fi
fi

echo ""
echo "Step 3: Installing full font..."
cp hzk16_full.c lib/Fonts/hzk16.c
echo "  Installed: lib/Fonts/hzk16.c"

echo ""
echo "Step 4: Building project..."
echo "  This may take a few minutes due to large font file..."
echo ""

tos.py build

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Font statistics:"
echo "  Characters: 6,763 (GB2312)"
echo "  File size: $(du -h lib/Fonts/hzk16.c | cut -f1)"
echo ""
echo "Next steps:"
echo "  1. Flash to device: tos.py flash"
echo "  2. Monitor output: tos.py monitor"
echo ""
echo "To restore original font:"
echo "  cp lib/Fonts/hzk16_default.c.bak lib/Fonts/hzk16.c"
echo "  tos.py build"
echo ""
