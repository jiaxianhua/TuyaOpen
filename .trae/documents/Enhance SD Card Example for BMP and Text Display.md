I will enhance the SD card example to support displaying BMP images and text files with GBK Chinese characters.

### Plan

1.  **Port BMP Library to TuyaOS File System**:
    *   Modify `examples/peripherals/sd/lib/GUI/GUI_BMPfile.c` to use `tkl_fs` APIs (`tkl_fopen`, `tkl_fread`, etc.) instead of standard `stdio`.
    *   Update `GUI_ReadBmp` to handle file reading errors gracefully (return error code instead of `exit(0)`).
    *   Ensure support for standard 24-bit BMPs (by converting to E-Paper compatible colors) and 1-bit BMPs.

2.  **Implement Adaptive Text Rendering**:
    *   Add a new function `Paint_DrawText_CN_HZK24_Adaptive` in `example_sd.c`.
    *   This function will:
        *   Handle `\n` newlines.
        *   Perform word wrapping (move to next line if text exceeds screen width).
        *   Support mixed ASCII and GBK Chinese characters.
        *   Stop rendering if the text exceeds the screen height (simple paging).

3.  **Update Main Application Logic (`example_sd.c`)**:
    *   Modify `refresh_ui` to detect file extensions:
        *   **`.bmp` / `.BMP`**: Clear screen and call `GUI_ReadBmp`.
        *   **`.txt` / `.TXT`** (and others): Read the first ~4KB of the file and call the new adaptive text rendering function.
    *   Add error handling for file opening failures.

4.  **Verification**:
    *   Build the project.
    *   (User verification) Flash and test with `.txt` files (Chinese content) and `.bmp` images on the SD card.
