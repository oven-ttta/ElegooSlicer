@REM ElegooSlicer build script for Windows
@echo off
set WP=%CD%

if not defined WindowsSdkDir (
    call get_vs2022_sdk.bat
)


echo WindowsSdkDir=%WindowsSdkDir%
echo WindowsSDKVersion=%WindowsSDKVersion%

@REM Pack deps
if "%1"=="pack" (
    setlocal ENABLEDELAYEDEXPANSION 
    cd %WP%/deps/build
    for /f "tokens=2-4 delims=/ " %%a in ('date /t') do set build_date=%%c%%b%%a
    echo packing deps: ElegooSlicer_dep_win64_!build_date!_vs2022.zip

    %WP%/tools/7z.exe a ElegooSlicer_dep_win64_!build_date!_vs2022.zip ElegooSlicer_dep
    exit /b 0
)

set debug=OFF
set debuginfo=OFF
if "%1"=="debug" set debug=ON
if "%2"=="debug" set debug=ON
if "%1"=="debuginfo" set debuginfo=ON
if "%2"=="debuginfo" set debuginfo=ON
if "%debug%"=="ON" (
    set build_type=Debug
    set build_dir=build-dbg
) else (
    if "%debuginfo%"=="ON" (
        set build_type=RelWithDebInfo
        set build_dir=build-dbginfo
    ) else (
        set build_type=Release
        set build_dir=build
    )
)

set ELEGOO_INTERNAL_TESTING=0
for %%a in (%*) do (
    if "%%a"=="test" (
        set ELEGOO_INTERNAL_TESTING=1
    )
)
echo build type set to %build_type%

setlocal DISABLEDELAYEDEXPANSION 
cd deps
mkdir %build_dir%
cd %build_dir%
set DEPS=%CD%/ElegooSlicer_dep

if "%1"=="slicer" (
    GOTO :slicer
)
@REM 打安装包
if "%1"=="pack_install" (
    GOTO :pack_install
)

echo "building deps.."

echo on
cmake ../ -G "Visual Studio 17 2022" -A x64 -DDESTDIR="%DEPS%" -DCMAKE_BUILD_TYPE=%build_type% -DDEP_DEBUG=%debug% -DORCA_INCLUDE_DEBUG_INFO=%debuginfo% -DELEGOO_INTERNAL_TESTING=%ELEGOO_INTERNAL_TESTING%
cmake --build . --config %build_type% --target deps -- -m
@echo off

if "%1"=="deps" exit /b 0

:slicer

echo "building ElegooSlicer...ELEGOO_INTERNAL_TESTING=%ELEGOO_INTERNAL_TESTING%"
cd %WP%
mkdir %build_dir%
cd %build_dir%

echo on
cmake .. -G "Visual Studio 17 2022" -A x64 -DELEGOO_INTERNAL_TESTING=%ELEGOO_INTERNAL_TESTING%  -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%/usr/local" -DCMAKE_INSTALL_PREFIX="./ElegooSlicer" -DCMAKE_BUILD_TYPE=%build_type% -DWIN10SDK_PATH="%WindowsSdkDir%Include\%WindowsSDKVersion%\"
cmake --build . --config %build_type% --target ALL_BUILD -- -m
@echo off
cd ..
call scripts/run_gettext.bat
cd %build_dir%
cmake --build . --target install --config %build_type%

@REM 打安装包
if not "%2"=="pack_install"  exit /b 0

:pack_install

setlocal enabledelayedexpansion

cd %WP%

set PACK_INSTALL_FILE=.\%build_dir%\install.ini
echo PACK_INSTALL_FILE: %PACK_INSTALL_FILE%

for /f "tokens=1,* delims==" %%a in ('findstr "^ELEGOOSLICER_VERSION" "%PACK_INSTALL_FILE%"') do (
    set ELEGOOSLICER_VERSION=%%b
)

if "!ELEGOOSLICER_VERSION!"=="" (
    echo ERROR: ELEGOOSLICER_VERSION is empty. Exiting.
    exit /b 1
)

set INSTALL_PATH=./%build_dir%/ElegooSlicer
set OUTFILE=./%build_dir%/ElegooSlicer_Windows_Installer_V!ELEGOOSLICER_VERSION!.exe


echo ELEGOOSLICER_VERSION: !ELEGOOSLICER_VERSION!
echo INSTALL_PATH: !INSTALL_PATH!
echo OUTFILE: %OUTFILE%

"%~dp0/tools/NSIS/makensis.exe" /DPRODUCT_VERSION=!ELEGOOSLICER_VERSION! /DINSTALL_PATH=!INSTALL_PATH! /DOUTFILE=!OUTFILE!  package.nsi

if %ERRORLEVEL% neq 0 (
    echo Packaging failed! Please check if package.nsi is correct.
    exit /b %ERRORLEVEL%
)

echo Packaging successful!
