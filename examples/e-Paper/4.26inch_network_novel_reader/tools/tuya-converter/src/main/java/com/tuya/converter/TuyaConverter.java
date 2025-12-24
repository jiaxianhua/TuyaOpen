package com.tuya.converter;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.*;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Tuya E-Paper Converter
 * Converts images to 1-bit BMP and text files to GBK encoding
 * 
 * Features:
 * - Smart rotation: Automatically rotates images 90° if it shows more content
 * - High-quality scaling with bicubic interpolation
 * - Automatic encoding detection for text files
 * 
 * Usage:
 *   java -jar tuya-converter.jar <input-file>
 *   java -jar tuya-converter.jar <input-file> [width] [height]
 */
public class TuyaConverter {
    
    private static final int DEFAULT_WIDTH = 480;
    private static final int DEFAULT_HEIGHT = 800;
    private static final String OUTPUT_SUFFIX = "_tuya";
    
    public static void main(String[] args) {
        if (args.length == 0) {
            printUsage();
            System.exit(1);
        }
        
        String inputPath = args[0];
        File inputFile = new File(inputPath);
        
        if (!inputFile.exists()) {
            System.err.println("Error: File not found: " + inputPath);
            System.exit(1);
        }
        
        try {
            String extension = getFileExtension(inputFile.getName()).toLowerCase();
            
            if (isImageFile(extension)) {
                // Image conversion
                int width = DEFAULT_WIDTH;
                int height = DEFAULT_HEIGHT;
                
                if (args.length >= 3) {
                    width = Integer.parseInt(args[1]);
                    height = Integer.parseInt(args[2]);
                }
                
                convertImage(inputFile, width, height);
            } else if (isTextFile(extension)) {
                // Text conversion
                convertText(inputFile);
            } else {
                System.err.println("Error: Unsupported file type: " + extension);
                System.err.println("Supported: images (jpg, jpeg, png, bmp, gif, webp, heic, heif) and text (txt)");
                System.exit(1);
            }
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
    
    private static void convertImage(File inputFile, int targetWidth, int targetHeight) throws IOException {
        System.out.println("Converting image: " + inputFile.getName());
        System.out.println("Target size: " + targetWidth + "x" + targetHeight);
        
        // Read input image
        BufferedImage originalImage = ImageIO.read(inputFile);
        if (originalImage == null) {
            throw new IOException("Failed to read image file");
        }
        
        int origWidth = originalImage.getWidth();
        int origHeight = originalImage.getHeight();
        System.out.println("Original size: " + origWidth + "x" + origHeight);
        
        // Smart rotation: choose orientation that shows more content
        boolean shouldRotate = shouldRotateImage(origWidth, origHeight, targetWidth, targetHeight);
        
        if (shouldRotate) {
            System.out.println("→ Rotating 90° for better fit");
            originalImage = rotateImage90(originalImage);
            // Swap target dimensions
            int temp = targetWidth;
            targetWidth = targetHeight;
            targetHeight = temp;
            System.out.println("→ New target size: " + targetWidth + "x" + targetHeight);
        }
        
        // Resize image
        BufferedImage resizedImage = resizeImage(originalImage, targetWidth, targetHeight);
        
        // Convert to grayscale
        BufferedImage grayImage = convertToGrayscale(resizedImage);
        
        // Convert to 1-bit (black and white)
        BufferedImage bwImage = convertToBlackAndWhite(grayImage);
        
        // Generate output filename
        String outputPath = generateOutputPath(inputFile.getPath(), ".bmp");
        File outputFile = new File(outputPath);
        
        // Write 1-bit BMP
        write1BitBMP(bwImage, outputFile);
        
        System.out.println("✓ Image converted successfully");
        System.out.println("Output: " + outputFile.getAbsolutePath());
    }
    
    /**
     * Determine if image should be rotated 90 degrees for better fit
     * Strategy: Minimize wasted space (black bars)
     */
    private static boolean shouldRotateImage(int imgWidth, int imgHeight, int targetWidth, int targetHeight) {
        // Calculate aspect ratios
        double imgAspect = (double) imgWidth / imgHeight;
        double targetAspect = (double) targetWidth / targetHeight;
        
        // Calculate coverage for both orientations
        double coverageNormal = calculateCoverage(imgWidth, imgHeight, targetWidth, targetHeight);
        double coverageRotated = calculateCoverage(imgHeight, imgWidth, targetWidth, targetHeight);
        
        System.out.println("→ Coverage without rotation: " + String.format("%.1f%%", coverageNormal * 100));
        System.out.println("→ Coverage with rotation: " + String.format("%.1f%%", coverageRotated * 100));
        
        // Rotate if it gives better coverage (at least 5% improvement)
        return coverageRotated > coverageNormal * 1.05;
    }
    
    /**
     * Calculate how much of the target area will be covered by the image
     * (avoiding black bars)
     */
    private static double calculateCoverage(int imgWidth, int imgHeight, int targetWidth, int targetHeight) {
        // Calculate scale to fit image into target
        double scaleX = (double) targetWidth / imgWidth;
        double scaleY = (double) targetHeight / imgHeight;
        double scale = Math.min(scaleX, scaleY);  // Fit inside (letterbox/pillarbox)
        
        // Calculate actual size after scaling
        int scaledWidth = (int) (imgWidth * scale);
        int scaledHeight = (int) (imgHeight * scale);
        
        // Calculate coverage (how much of target is filled)
        double coverage = (double) (scaledWidth * scaledHeight) / (targetWidth * targetHeight);
        
        return coverage;
    }
    
    /**
     * Rotate image 90 degrees clockwise
     */
    private static BufferedImage rotateImage90(BufferedImage original) {
        int width = original.getWidth();
        int height = original.getHeight();
        
        BufferedImage rotated = new BufferedImage(height, width, original.getType());
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                rotated.setRGB(height - y - 1, x, original.getRGB(x, y));
            }
        }
        
        return rotated;
    }
    
    private static void convertText(File inputFile) throws IOException {
        System.out.println("Converting text file: " + inputFile.getName());
        
        // Read input file (auto-detect encoding)
        String content = readTextFile(inputFile);
        
        // Generate output filename
        String outputPath = generateOutputPath(inputFile.getPath(), ".txt");
        File outputFile = new File(outputPath);
        
        // Write as GBK
        writeGBKFile(content, outputFile);
        
        System.out.println("✓ Text converted to GBK encoding");
        System.out.println("Output: " + outputFile.getAbsolutePath());
    }
    
    private static BufferedImage resizeImage(BufferedImage original, int targetWidth, int targetHeight) {
        BufferedImage resized = new BufferedImage(targetWidth, targetHeight, BufferedImage.TYPE_INT_RGB);
        Graphics2D g = resized.createGraphics();
        
        // High quality scaling
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
        g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        
        g.drawImage(original, 0, 0, targetWidth, targetHeight, null);
        g.dispose();
        
        return resized;
    }
    
    private static BufferedImage convertToGrayscale(BufferedImage original) {
        BufferedImage gray = new BufferedImage(
            original.getWidth(), 
            original.getHeight(), 
            BufferedImage.TYPE_BYTE_GRAY
        );
        
        Graphics2D g = gray.createGraphics();
        g.drawImage(original, 0, 0, null);
        g.dispose();
        
        return gray;
    }
    
    private static BufferedImage convertToBlackAndWhite(BufferedImage gray) {
        int width = gray.getWidth();
        int height = gray.getHeight();
        
        BufferedImage bw = new BufferedImage(width, height, BufferedImage.TYPE_BYTE_BINARY);
        
        // Apply threshold (128 = middle gray)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int rgb = gray.getRGB(x, y);
                int grayValue = (rgb >> 16) & 0xFF;  // R, G, B are all the same in grayscale
                
                // Threshold: > 128 = white, <= 128 = black
                int bwValue = (grayValue > 128) ? 0xFFFFFF : 0x000000;
                bw.setRGB(x, y, bwValue);
            }
        }
        
        return bw;
    }
    
    private static void write1BitBMP(BufferedImage image, File outputFile) throws IOException {
        int width = image.getWidth();
        int height = image.getHeight();
        
        // Calculate row size (must be multiple of 4 bytes)
        int rowSize = ((width + 31) / 32) * 4;
        int imageSize = rowSize * height;
        int fileSize = 62 + imageSize;  // 14 (file header) + 40 (info header) + 8 (palette) + image data
        
        try (FileOutputStream fos = new FileOutputStream(outputFile);
             DataOutputStream dos = new DataOutputStream(fos)) {
            
            // BMP File Header (14 bytes)
            dos.write('B');
            dos.write('M');
            writeInt32LE(dos, fileSize);        // File size
            writeInt16LE(dos, 0);               // Reserved
            writeInt16LE(dos, 0);               // Reserved
            writeInt32LE(dos, 62);              // Pixel data offset
            
            // BMP Info Header (40 bytes)
            writeInt32LE(dos, 40);              // Info header size
            writeInt32LE(dos, width);           // Width
            writeInt32LE(dos, height);          // Height (positive = bottom-up)
            writeInt16LE(dos, 1);               // Planes
            writeInt16LE(dos, 1);               // Bits per pixel (1-bit)
            writeInt32LE(dos, 0);               // Compression (BI_RGB)
            writeInt32LE(dos, imageSize);       // Image size
            writeInt32LE(dos, 2835);            // X pixels per meter (72 DPI)
            writeInt32LE(dos, 2835);            // Y pixels per meter (72 DPI)
            writeInt32LE(dos, 2);               // Colors used
            writeInt32LE(dos, 2);               // Important colors
            
            // Color Palette (8 bytes for 1-bit: 2 colors)
            // Color 0: Black (0, 0, 0)
            dos.write(0);  // Blue
            dos.write(0);  // Green
            dos.write(0);  // Red
            dos.write(0);  // Reserved
            
            // Color 1: White (255, 255, 255)
            dos.write(255);  // Blue
            dos.write(255);  // Green
            dos.write(255);  // Red
            dos.write(0);    // Reserved
            
            // Pixel Data (bottom-up, left-to-right)
            byte[] rowBuffer = new byte[rowSize];
            
            for (int y = height - 1; y >= 0; y--) {  // Bottom-up
                // Clear row buffer
                for (int i = 0; i < rowSize; i++) {
                    rowBuffer[i] = 0;
                }
                
                // Pack pixels into bits
                for (int x = 0; x < width; x++) {
                    int rgb = image.getRGB(x, y);
                    int brightness = (rgb >> 16) & 0xFF;
                    
                    // White pixel = bit 1, Black pixel = bit 0
                    if (brightness > 128) {
                        int byteIndex = x / 8;
                        int bitIndex = 7 - (x % 8);
                        rowBuffer[byteIndex] |= (1 << bitIndex);
                    }
                }
                
                dos.write(rowBuffer);
            }
        }
    }
    
    private static String readTextFile(File file) throws IOException {
        // Try to detect encoding
        byte[] bytes = Files.readAllBytes(file.toPath());
        
        // Try UTF-8 first
        String content = new String(bytes, StandardCharsets.UTF_8);
        
        // If contains replacement characters, try GBK
        if (content.contains("\uFFFD")) {
            try {
                content = new String(bytes, Charset.forName("GBK"));
            } catch (Exception e) {
                // Fall back to system default
                content = new String(bytes);
            }
        }
        
        return content;
    }
    
    private static void writeGBKFile(String content, File file) throws IOException {
        try (OutputStreamWriter writer = new OutputStreamWriter(
                new FileOutputStream(file), 
                Charset.forName("GBK"))) {
            writer.write(content);
        }
    }
    
    private static void writeInt16LE(DataOutputStream dos, int value) throws IOException {
        dos.write(value & 0xFF);
        dos.write((value >> 8) & 0xFF);
    }
    
    private static void writeInt32LE(DataOutputStream dos, int value) throws IOException {
        dos.write(value & 0xFF);
        dos.write((value >> 8) & 0xFF);
        dos.write((value >> 16) & 0xFF);
        dos.write((value >> 24) & 0xFF);
    }
    
    private static String getFileExtension(String filename) {
        int lastDot = filename.lastIndexOf('.');
        if (lastDot == -1) {
            return "";
        }
        return filename.substring(lastDot + 1);
    }
    
    private static String getFileNameWithoutExtension(String filename) {
        int lastDot = filename.lastIndexOf('.');
        if (lastDot == -1) {
            return filename;
        }
        return filename.substring(0, lastDot);
    }
    
    private static String generateOutputPath(String inputPath, String newExtension) {
        File inputFile = new File(inputPath);
        String dir = inputFile.getParent();
        String nameWithoutExt = getFileNameWithoutExtension(inputFile.getName());
        String outputName = nameWithoutExt + OUTPUT_SUFFIX + newExtension;
        
        if (dir != null) {
            return new File(dir, outputName).getPath();
        } else {
            return outputName;
        }
    }
    
    private static boolean isImageFile(String extension) {
        return extension.equals("jpg") || extension.equals("jpeg") || 
               extension.equals("png") || extension.equals("bmp") || 
               extension.equals("gif") || extension.equals("webp") ||
               extension.equals("heic") || extension.equals("heif");
    }
    
    private static boolean isTextFile(String extension) {
        return extension.equals("txt");
    }
    
    private static void printUsage() {
        System.out.println("Tuya E-Paper Converter v1.1.0");
        System.out.println();
        System.out.println("Features:");
        System.out.println("  ✓ Smart rotation - Auto-rotates images for maximum content display");
        System.out.println("  ✓ High-quality scaling - Bicubic interpolation");
        System.out.println("  ✓ Auto encoding detection - UTF-8, GBK, system default");
        System.out.println();
        System.out.println("Usage:");
        System.out.println("  java -jar tuya-converter.jar <input-file>");
        System.out.println("  java -jar tuya-converter.jar <input-file> [width] [height]");
        System.out.println();
        System.out.println("Examples:");
        System.out.println("  java -jar tuya-converter.jar photo.jpg");
        System.out.println("  java -jar tuya-converter.jar photo.jpg 480 800");
        System.out.println("  java -jar tuya-converter.jar novel.txt");
        System.out.println();
        System.out.println("Supported formats:");
        System.out.println("  Images: jpg, jpeg, png, bmp, gif, webp, heic, heif");
        System.out.println("  Text: txt");
        System.out.println();
        System.out.println("Output:");
        System.out.println("  input.jpg  -> input_tuya.bmp (1-bit BMP, auto-rotated if needed)");
        System.out.println("  input.txt  -> input_tuya.txt (GBK encoding)");
        System.out.println();
        System.out.println("Smart Rotation:");
        System.out.println("  The tool automatically rotates images 90° if it results in");
        System.out.println("  better screen coverage (less black bars, more content visible).");
    }
}
