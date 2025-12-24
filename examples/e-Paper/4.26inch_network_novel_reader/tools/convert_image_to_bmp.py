#!/usr/bin/env python3
"""
Convert images to monochrome BMP for E-Paper display

Usage:
    python convert_image_to_bmp.py input.jpg output.bmp
    python convert_image_to_bmp.py input.png output.bmp --width 800 --height 480
"""

import sys
import argparse
from PIL import Image

def convert_to_monochrome_bmp(input_path, output_path, width=800, height=480, threshold=128):
    """
    Convert image to monochrome BMP
    
    Args:
        input_path: Input image path (JPG, PNG, etc.)
        output_path: Output BMP path
        width: Target width (default: 800)
        height: Target height (default: 480)
        threshold: Grayscale threshold for black/white (0-255, default: 128)
    """
    try:
        # Open image
        print(f"Opening image: {input_path}")
        img = Image.open(input_path)
        print(f"Original size: {img.size}, mode: {img.mode}")
        
        # Resize to target dimensions
        print(f"Resizing to {width}x{height}...")
        img = img.resize((width, height), Image.Resampling.LANCZOS)
        
        # Convert to grayscale first
        if img.mode != 'L':
            print("Converting to grayscale...")
            img = img.convert('L')
        
        # Apply threshold to convert to pure black and white
        print(f"Applying threshold ({threshold})...")
        img = img.point(lambda x: 255 if x > threshold else 0, mode='1')
        
        # Save as BMP
        print(f"Saving to: {output_path}")
        img.save(output_path, 'BMP')
        
        print("✓ Conversion successful!")
        print(f"  Output: {output_path}")
        print(f"  Size: {width}x{height}")
        print(f"  Format: 1-bit monochrome BMP")
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def batch_convert(input_dir, output_dir, width=800, height=480, threshold=128):
    """
    Batch convert all images in a directory
    """
    import os
    from pathlib import Path
    
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    
    # Create output directory if it doesn't exist
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Supported image formats
    extensions = ['.jpg', '.jpeg', '.png', '.gif', '.bmp', '.tiff']
    
    # Find all images
    images = []
    for ext in extensions:
        images.extend(input_path.glob(f'*{ext}'))
        images.extend(input_path.glob(f'*{ext.upper()}'))
    
    if not images:
        print(f"No images found in {input_dir}")
        return
    
    print(f"Found {len(images)} images to convert")
    print("-" * 50)
    
    success_count = 0
    for img_path in images:
        output_file = output_path / f"{img_path.stem}.bmp"
        print(f"\nProcessing: {img_path.name}")
        
        if convert_to_monochrome_bmp(str(img_path), str(output_file), width, height, threshold):
            success_count += 1
    
    print("\n" + "=" * 50)
    print(f"Conversion complete: {success_count}/{len(images)} successful")

def main():
    parser = argparse.ArgumentParser(
        description='Convert images to monochrome BMP for E-Paper display',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Convert single image
  python convert_image_to_bmp.py photo.jpg photo.bmp
  
  # Convert with custom size
  python convert_image_to_bmp.py photo.jpg photo.bmp --width 400 --height 300
  
  # Adjust threshold for better contrast
  python convert_image_to_bmp.py photo.jpg photo.bmp --threshold 150
  
  # Batch convert all images in a directory
  python convert_image_to_bmp.py input_dir/ output_dir/ --batch
        """
    )
    
    parser.add_argument('input', help='Input image file or directory (for batch mode)')
    parser.add_argument('output', help='Output BMP file or directory (for batch mode)')
    parser.add_argument('--width', type=int, default=800, help='Target width (default: 800)')
    parser.add_argument('--height', type=int, default=480, help='Target height (default: 480)')
    parser.add_argument('--threshold', type=int, default=128, 
                       help='Grayscale threshold 0-255 (default: 128, lower=more black)')
    parser.add_argument('--batch', action='store_true', help='Batch convert directory')
    
    args = parser.parse_args()
    
    # Validate threshold
    if not 0 <= args.threshold <= 255:
        print("Error: Threshold must be between 0 and 255")
        sys.exit(1)
    
    # Check if PIL is available
    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow library not found")
        print("Install with: pip install Pillow")
        sys.exit(1)
    
    # Batch or single conversion
    if args.batch:
        batch_convert(args.input, args.output, args.width, args.height, args.threshold)
    else:
        success = convert_to_monochrome_bmp(
            args.input, args.output, args.width, args.height, args.threshold
        )
        sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
