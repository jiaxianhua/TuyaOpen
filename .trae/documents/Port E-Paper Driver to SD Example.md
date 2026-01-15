I will implement the E-Paper display functionality in the SD card example by porting the driver code.

1.  **Copy Library Files**: Copy the `lib` directory (containing Config, Fonts, GUI, e-Paper drivers) from `examples/e-Paper/4.26inch_e-Paper` to `examples/peripherals/sd`.
2.  **Update Build System**: Modify `examples/peripherals/sd/CMakeLists.txt` to compile the new E-Paper library files and include their header directories.
3.  **Integrate Code**: Modify `examples/peripherals/sd/src/example_sd.c` to:
    *   Initialize the E-Paper display.
    *   Create a frame buffer.
    *   Implement a function to list files from the SD card and draw them on the E-Paper screen (using `GUI_Paint` functions).
    *   Use the `utf8_to_gbk` converter to properly display Chinese filenames if the font supports it (or fallback/debug).
