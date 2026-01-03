#!/bin/bash
# Build script for 7key_control example
# Usage: ./build.sh

# Get the project root directory (3 levels up from this script)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "Project root: $PROJECT_ROOT"
echo "Example path: $SCRIPT_DIR"

# Check if virtual environment is activated
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Error: Virtual environment is not activated!"
    echo "Please run: source $PROJECT_ROOT/export.sh"
    exit 1
fi

# Change to example directory
cd "$SCRIPT_DIR"

# Run build command
echo "Building 7key_control example..."
tos.py build

echo "Build complete!"
