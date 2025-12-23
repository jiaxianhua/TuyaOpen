@echo off
REM build_with_full_font.bat
REM 
REM One-click script to generate full HZK16 font and build the project (Windows)
REM
REM Usage:
REM   build_with_full_font.bat

setlocal enabledelayedexpansion

echo ==========================================
echo Building with Full HZK16 Font
echo ==========================================
echo.

REM Check if HZK16 file exists
if not exist "HZK16" (
    echo Error: HZK16 file not found!
    echo.
    echo Please download HZK16 first:
    echo   Download from: https://github.com/aguegu/BitmapFont/raw/master/font/HZK16
    echo   Save as: HZK16 (no extension^)
    echo.
    pause
    exit /b 1
)

echo Step 1: Generating full HZK16 font...
echo   This will generate ~6,763 characters (~230KB^)
echo.

python tools\generate_full_hzk16.py HZK16 hzk16_full.c

if not exist "hzk16_full.c" (
    echo Error: Failed to generate hzk16_full.c
    pause
    exit /b 1
)

echo.
echo Step 2: Backing up original font file...

if exist "lib\Fonts\hzk16.c" (
    if not exist "lib\Fonts\hzk16_default.c.bak" (
        copy lib\Fonts\hzk16.c lib\Fonts\hzk16_default.c.bak >nul
        echo   Backup created: lib\Fonts\hzk16_default.c.bak
    ) else (
        echo   Backup already exists, skipping...
    )
)

echo.
echo Step 3: Installing full font...
copy /Y hzk16_full.c lib\Fonts\hzk16.c >nul
echo   Installed: lib\Fonts\hzk16.c

echo.
echo Step 4: Building project...
echo   This may take a few minutes due to large font file...
echo.

call tos.py build

echo.
echo ==========================================
echo Build Complete!
echo ==========================================
echo.
echo Font statistics:
echo   Characters: 6,763 (GB2312^)
for %%A in (lib\Fonts\hzk16.c) do echo   File size: %%~zA bytes
echo.
echo Next steps:
echo   1. Flash to device: tos.py flash
echo   2. Monitor output: tos.py monitor
echo.
echo To restore original font:
echo   copy lib\Fonts\hzk16_default.c.bak lib\Fonts\hzk16.c
echo   tos.py build
echo.

pause
