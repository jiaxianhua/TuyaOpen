@echo off
REM Tuya Converter - Windows Drag & Drop Helper
REM 
REM Usage: Drag files onto this batch file
REM 
REM Setup:
REM 1. Edit the JAR_PATH below to point to your tuya-converter.jar
REM 2. Drag image or text files onto this .bat file

REM ===== CONFIGURATION =====
REM Change this to the actual path of tuya-converter.jar
SET JAR_PATH=%~dp0target\tuya-converter.jar

REM Optional: Set custom dimensions (comment out for default 480x800)
REM SET WIDTH=480
REM SET HEIGHT=800
REM =========================

echo Tuya E-Paper Converter
echo =======================
echo.

REM Check if JAR exists
if not exist "%JAR_PATH%" (
    echo Error: JAR file not found!
    echo Expected location: %JAR_PATH%
    echo.
    echo Please build the project first:
    echo   build.bat
    echo.
    pause
    exit /b 1
)

REM Check if Java is installed
where java >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: Java is not installed or not in PATH
    echo Please install Java 11 or higher
    echo Download: https://adoptium.net/
    echo.
    pause
    exit /b 1
)

REM Check if files were dragged
if "%~1"=="" (
    echo No files provided!
    echo.
    echo Usage: Drag files onto this batch file
    echo.
    pause
    exit /b 1
)

REM Process each file
:loop
if "%~1"=="" goto end

echo Processing: %~nx1

if defined WIDTH (
    if defined HEIGHT (
        java -jar "%JAR_PATH%" "%~1" %WIDTH% %HEIGHT%
    ) else (
        java -jar "%JAR_PATH%" "%~1"
    )
) else (
    java -jar "%JAR_PATH%" "%~1"
)

echo.
shift
goto loop

:end
echo =======================
echo All files processed!
echo.
pause
