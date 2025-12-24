#!/bin/bash
# Build script for Tuya Converter

echo "Building Tuya Converter..."

# Check if Maven is installed
if ! command -v mvn &> /dev/null; then
    echo "Error: Maven is not installed"
    echo "Please install Maven: https://maven.apache.org/install.html"
    exit 1
fi

# Build the project
mvn clean package

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "JAR file location:"
    echo "  target/tuya-converter.jar"
    echo ""
    echo "Usage:"
    echo "  java -jar target/tuya-converter.jar <input-file>"
    echo "  java -jar target/tuya-converter.jar <input-file> [width] [height]"
    echo ""
else
    echo "✗ Build failed"
    exit 1
fi
