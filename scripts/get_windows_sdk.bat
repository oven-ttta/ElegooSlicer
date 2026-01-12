@echo off

:: Step 1: Try to get Windows SDK path from registry
for /f "tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10 2^>nul') do (
    set "WindowsSdkDir=%%b"
)

:: Validate the SDK directory
if not defined WindowsSdkDir (
    echo ERROR: Windows SDK not found in registry!
    exit /b 1
)

if not exist "%WindowsSdkDir%" (
    echo ERROR: Windows SDK path "%WindowsSdkDir%" does not exist!
    exit /b 1
)

:: Step 2: Find the latest SDK version
set "WindowsSDKVersion="

if not exist "%WindowsSdkDir%Include" (
    echo ERROR: Path "%WindowsSdkDir%Include" does not exist!
    exit /b 1
)

for /f "tokens=*" %%i in ('dir /b /ad "%WindowsSdkDir%Include" ^| findstr /r "^10\." ^| sort /r') do (
    set "WindowsSDKVersion=%%i"
    goto :validate_version
)

:: If no version is found
if not defined WindowsSDKVersion (
    echo ERROR: No valid SDK version found in "%WindowsSdkDir%Include"!
    exit /b 1
)

:validate_version
if not exist "%WindowsSdkDir%Include\%WindowsSDKVersion%\" (
    echo ERROR: SDK path invalid: "%WindowsSdkDir%Include\%WindowsSDKVersion%\"
    exit /b 1
)

exit /b 0

