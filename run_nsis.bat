@echo off
cd /d "%~dp0"
"C:\Program Files (x86)\NSIS\makensis.exe" /DPRODUCT_VERSION=1.1.8.0 /DINSTALL_PATH="%~dp0build\package" package.nsi
echo.
echo Exit code: %errorlevel%
