#!/bin/bash
# Quick compile test script

echo "Testing compilation..."
source ../../export.sh
tos.py build

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
else
    echo "✗ Compilation failed!"
    exit 1
fi
