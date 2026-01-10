@echo off
:: Check for admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo ==========================================
echo    ElegooSlicer Installer Builder
echo ==========================================
echo.

set "NSIS_DIR=C:\Program Files (x86)\NSIS"
set "PROJECT_DIR=C:\Users\admin\Desktop\3D\ElegooSlicer"

echo [1/3] Copying NSIS plugins...
copy /Y "%PROJECT_DIR%\tools\NSIS\Include\nsProcess.nsh" "%NSIS_DIR%\Include\" >nul
copy /Y "%PROJECT_DIR%\tools\NSIS\Plugins\x86-ansi\nsProcess.dll" "%NSIS_DIR%\Plugins\x86-ansi\" >nul
copy /Y "%PROJECT_DIR%\tools\NSIS\Plugins\x86-unicode\nsProcess.dll" "%NSIS_DIR%\Plugins\x86-unicode\" >nul
echo Done.

echo.
echo [2/3] Building installer...
cd /d "%PROJECT_DIR%"
"%NSIS_DIR%\makensis.exe" /INPUTCHARSET CP936 /DPRODUCT_VERSION=1.1.8.0 "/DINSTALL_PATH=%PROJECT_DIR%\build\package" package.nsi

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to create installer!
    pause
    exit /b 1
)

echo.
echo [3/3] Checking output...
if exist "%PROJECT_DIR%\build\ElegooSlicer_Windows_Installer_V1.1.8.0.exe" (
    echo.
    echo ==========================================
    echo [SUCCESS] Installer created!
    echo.
    echo Output: %PROJECT_DIR%\build\ElegooSlicer_Windows_Installer_V1.1.8.0.exe
    echo ==========================================

    explorer /select,"%PROJECT_DIR%\build\ElegooSlicer_Windows_Installer_V1.1.8.0.exe"
) else (
    echo [ERROR] Installer file not found!
)

echo.
pause
