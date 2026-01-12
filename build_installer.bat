@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo    ElegooSlicer Installer Builder
echo ==========================================
echo.

REM Set version
set PRODUCT_VERSION=1.3.0.11
set INSTALL_PATH=%~dp0build\package

REM Check if NSIS is installed
set NSIS_PATH=
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    set "NSIS_PATH=C:\Program Files (x86)\NSIS\makensis.exe"
) else if exist "C:\Program Files\NSIS\makensis.exe" (
    set "NSIS_PATH=C:\Program Files\NSIS\makensis.exe"
) else (
    for /f "delims=" %%i in ('where makensis 2^>nul') do set "NSIS_PATH=%%i"
)

if "!NSIS_PATH!"=="" (
    echo [ERROR] NSIS is not installed!
    echo.
    echo Please download and install NSIS from:
    echo https://nsis.sourceforge.io/Download
    echo.
    echo After installation, run this script again.
    pause
    exit /b 1
)

echo [INFO] Found NSIS at: !NSIS_PATH!
echo [INFO] Version: %PRODUCT_VERSION%
echo [INFO] Install Path: %INSTALL_PATH%
echo.

REM Check if build/package exists
if not exist "%INSTALL_PATH%" (
    echo [ERROR] Build package not found at: %INSTALL_PATH%
    echo Please build the project first using build_release_vs2022.bat
    pause
    exit /b 1
)

REM Check if elegoo-slicer.exe exists
if not exist "%INSTALL_PATH%\elegoo-slicer.exe" (
    echo [ERROR] elegoo-slicer.exe not found in package folder!
    echo Please build the project first.
    pause
    exit /b 1
)

echo [INFO] Building installer...
echo.

cd /d "%~dp0"

"!NSIS_PATH!" /DPRODUCT_VERSION=%PRODUCT_VERSION% /DINSTALL_PATH=%INSTALL_PATH% package.nsi

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to create installer!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo [SUCCESS] Installer created successfully!
echo.
echo Output: %~dp0build\ElegooSlicer_Windows_Installer_V%PRODUCT_VERSION%.exe
echo ==========================================
echo.

pause
