#!/bin/bash
# Tuya Converter - Linux/macOS Drag & Drop Helper
#
# Usage: ./tuya-convert.sh file1 file2 ...
#        or drag files onto this script
#
# Setup:
# 1. Edit JAR_PATH below to point to your tuya-converter.jar
# 2. chmod +x tuya-convert.sh
# 3. Drag files onto this script or run from command line

# ===== CONFIGURATION =====
# Change this to the actual path of tuya-converter.jar
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAR_PATH="$SCRIPT_DIR/target/tuya-converter.jar"

# Optional: Set custom dimensions (comment out for default 480x800)
# WIDTH=480
# HEIGHT=800
# =========================

echo "Tuya E-Paper Converter"
echo "======================="
echo ""

# Check if JAR exists
if [ ! -f "$JAR_PATH" ]; then
    echo "Error: JAR file not found!"
    echo "Expected location: $JAR_PATH"
    echo ""
    echo "Please build the project first:"
    echo "  ./build.sh"
    echo ""
    exit 1
fi

# Check if Java is installed
if ! command -v java &> /dev/null; then
    echo "Error: Java is not installed or not in PATH"
    echo "Please install Java 11 or higher"
    echo "Download: https://adoptium.net/"
    echo ""
    exit 1
fi

# Check if files were provided
if [ $# -eq 0 ]; then
    echo "No files provided!"
    echo ""
    echo "Usage: $0 file1 [file2 ...]"
    echo "   or: Drag files onto this script"
    echo ""
    exit 1
fi

# Process each file
for file in "$@"; do
    echo "Processing: $(basename "$file")"
    
    if [ -n "$WIDTH" ] && [ -n "$HEIGHT" ]; then
        java -jar "$JAR_PATH" "$file" "$WIDTH" "$HEIGHT"
    else
        java -jar "$JAR_PATH" "$file"
    fi
    
    echo ""
done

echo "======================="
echo "All files processed!"
echo ""
