@echo off
REM Build script for Tuya Converter (Windows)

echo Building Tuya Converter...

REM Check if Maven is installed
where mvn >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: Maven is not installed
    echo Please install Maven: https://maven.apache.org/install.html
    exit /b 1
)

REM Build the project
call mvn clean package

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo.
    echo JAR file location:
    echo   target\tuya-converter.jar
    echo.
    echo Usage:
    echo   java -jar target\tuya-converter.jar ^<input-file^>
    echo   java -jar target\tuya-converter.jar ^<input-file^> [width] [height]
    echo.
) else (
    echo Build failed
    exit /b 1
)
