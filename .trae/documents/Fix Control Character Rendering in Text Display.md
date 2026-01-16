I will fix the text rendering issues where newlines and carriage returns cause garbled characters ("?" or乱码).

### Issue Analysis
The current `Paint_DrawText_CN_HZK24_Adaptive` function handles `\n` (newline) but likely treats `\r` (carriage return) or other control characters as printable ASCII or part of a multi-byte sequence, leading to display errors. The "?? " seen at the beginning of lines in the photos suggests that `\r` (0x0D) or `\t` (0x09) are being rendered as invalid characters.

### Plan

1.  **Update `Paint_DrawText_CN_HZK24_Adaptive` in `example_sd.c`**:
    *   Add handling for `\r` (Carriage Return): Ignore it or treat it like a newline if it's not followed by `\n`.
    *   Add handling for `\t` (Tab): Replace with spaces (e.g., 4 spaces).
    *   Filter out other non-printable control characters (0x00-0x1F) to prevent them from being interpreted as valid ASCII or Chinese byte sequences.
    *   Refine the word-wrapping logic to ensure it doesn't break in the middle of a control sequence.

2.  **Verification**:
    *   Build the project.
    *   (User verification) Flash and test with the same text files. The "?? " artifacts should disappear, and formatting should be cleaner.
