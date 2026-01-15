# Enable SD Card and File System Support
1.  **Explanation of Encoding & RTOS Support**:
    *   **FAT32 Filename Encoding**: Standard FAT32 uses UCS-2 (UTF-16) for Long File Names (LFN). However, the RTOS/Middleware (Tuya `tkl_fs`) typically handles the conversion and presents filenames to the application layer in **UTF-8**.
    *   **RTOS Support**: Yes, the underlying RTOS (e.g., FreeRTOS + FatFs) needs to be configured to support LFN (Long File Names) and the appropriate Code Page (e.g., CP936 or UTF-8) to handle Chinese characters correctly. We have already enabled `CONFIG_ENABLE_FILE_SYSTEM=y` in `app_default.config` to support this.
    *   **Font Limitation**: The project currently uses a very small, hardcoded Chinese font library (`Font24CN`) which only contains about 20 characters (like "你好", "树莓派", etc.). **Arbitrary Chinese filenames will not display correctly** if they contain characters outside this small set. They will be skipped or garbled.

2.  **Fixing the "File List Not Showing" Issue**:
    *   **Reason**: Although we added the helper functions `mount_sd()` and `list_and_display_files()`, we **did not call them** inside the main `EPD_clock_test()` function. The code currently jumps straight to the clock display loop.
    *   **Action**: We need to modify `EPD_clock_test()` in `EPD_4in26_clock.c` to:
        1.  Initialize/Mount the SD card (`mount_sd()`).
        2.  Call `list_and_display_files(BlackImage)` *after* the full-screen buffer is allocated and *before* it is freed/resized for the clock partial refresh.
        3.  Add a delay so the file list is visible for a few seconds before the clock starts.

# Implementation Plan

1.  **Modify `examples/e-Paper/4.26inch_e-Paper_clock/examples/EPD_4in26_clock.c`**:
    *   Locate `EPD_clock_test` function.
    *   Insert `mount_sd()` call during initialization.
    *   Insert `list_and_display_files(BlackImage)` call after `EPD_4in26_Display_Base(BlackImage)` (which clears the screen) and before `free(BlackImage)`.
    *   This ensures the file list is drawn on the full screen, displayed, and then the program proceeds to the clock loop.

2.  **Verify**:
    *   Since we cannot run the hardware, we will rely on the code structure correctness.
    *   The `DrawString_Mixed` function already handles UTF-8 decoding (assuming 3-byte sequences for Chinese) and falls back to ASCII for other characters.
