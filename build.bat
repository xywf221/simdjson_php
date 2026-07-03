@echo off
setlocal

REM Build php_simdjson.dll locally against PHP 8.5 NTS x64.
REM
REM Run from "x64 Native Tools Command Prompt for VS 2022" so cl.exe /
REM nmake.exe are already on PATH. No PHP source tree, no phpize, no
REM bison - just headers and php8.lib from the PHP devel pack.
REM
REM Output: %~dp0php_simdjson.dll

set "ROOT=%~dp0"
REM Strip the trailing backslash so the include paths don't break cl's
REM command-line parsing (e.g. /I"%ROOT%" needs no trailing \).
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
REM Reuse the now-trailing-slashless ROOT and re-add one wherever we need it.
set "ROOT_SLASH=%ROOT%\"
set "DEVEL_URL=https://github.com/shivammathur/php-builder-windows/releases/download/php8.5/php-devel-pack-8.5.7-nts-Win32-vs17-x64.zip"
set "DL_DIR=%ROOT_SLASH%build"
set "DEVEL_DIR="
set "OUT_DLL=%ROOT_SLASH%php_simdjson.dll"
set "CPP_FLAGS=/nologo /EHsc /MD /O2 /std:c++17 /Zc:__cplusplus /D ZEND_ENABLE_STATIC_TSRMLS_CACHE=1 /D COMPILE_DL_SIMDJSON=1 /D SIMDJSON_EXPORTS=1 /D HAVE_SIMDJSON=1 /D _WINDOWS /D WIN32 /D _USRDLL /D NDEBUG /DZEND_DEBUG=0 /D PHP_WIN32=1 /D ZEND_WIN32=1 /D _USE_MATH_DEFINES /D ENABLE_INTSAFE_SIGNED_FUNCTIONS /D PHP_SIMDJSON_API=__declspec(dllexport)"

where cl.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: cl.exe not on PATH. Run from a "x64 Native Tools Command Prompt for VS 2022".
    exit /b 1
)

if not exist "%DL_DIR%\devel.zip" (
    echo === Downloading PHP 8.5.7 NTS x64 devel pack ===
    if not exist "%DL_DIR%" mkdir "%DL_DIR%"
    powershell -NoProfile -Command ^
        "$ErrorActionPreference='Stop';" ^
        "Invoke-WebRequest -Uri '%DEVEL_URL%' -OutFile '%DL_DIR%\devel.zip'"
    if errorlevel 1 (
        echo ERROR: failed to fetch devel pack from %DEVEL_URL%
        exit /b 1
    )
)

REM The zip expands to a directory whose name varies between releases (e.g.
REM "php-8.5.7-devel-vs17-x64" or "php-devel-pack-8.5.7-..."). Pick the
REM first subdirectory that contains an include\ tree.
for /d %%D in ("%DL_DIR%\php-*") do (
    if not defined DEVEL_DIR if exist "%%D\include" set "DEVEL_DIR=%%D"
)
if not defined DEVEL_DIR (
    echo ERROR: could not locate PHP include\ tree under %DL_DIR%
    dir /S /B "%DL_DIR%\*.h" 2>nul | findstr /R "php\.h$" | head -3
    exit /b 1
)

if not exist "%DEVEL_DIR%\lib\php8.lib" (
    echo ERROR: php8.lib not found under %DEVEL_DIR%
    dir /S /B "%DEVEL_DIR%\lib\*.lib"
    exit /b 1
)

REM PHP's buildconf normally generates Zend\zend_config.h from
REM Zend\zend_config.w32.h. The devel pack only ships the w32 template, so
REM copy it into place ourselves before cl.exe is asked for it.
if not exist "%DEVEL_DIR%\include\Zend\zend_config.h" (
    copy /Y "%DEVEL_DIR%\include\Zend\zend_config.w32.h" "%DEVEL_DIR%\include\Zend\zend_config.h" >nul
)

set "CFLAGS=%CPP_FLAGS% /I"%DEVEL_DIR%\include" /I"%DEVEL_DIR%\include\main" /I"%DEVEL_DIR%\include\Zend" /I"%DEVEL_DIR%\include\TSRM" /I"%DEVEL_DIR%\include\ext" /I"%DEVEL_DIR%\include\ext\standard" /I"%DEVEL_DIR%\include\ext\spl" /I"%DEVEL_DIR%\include\ext\date\php_date" /I"%DEVEL_DIR%\include\ext\json" /I"%ROOT_SLASH%src" /I."

set "OBJS=%DL_DIR%\php_simdjson.obj %DL_DIR%\simdjson.obj %DL_DIR%\simdjson_decoder.obj %DL_DIR%\simdjson_encoder.obj %DL_DIR%\simdutf.obj"

if exist "%DL_DIR%" (
    del /Q "%DL_DIR%\*.obj" "%OUT_DLL%" 2>nul
)

setlocal enabledelayedexpansion
cd /d "%ROOT_SLASH%"

echo === cl.exe: php_simdjson.cpp ===
cl.exe !CFLAGS! /c "%ROOT_SLASH%php_simdjson.cpp" /Fo"%DL_DIR%\php_simdjson.obj"
if errorlevel 1 exit /b 1
echo === cl.exe: simdjson.cpp ===
cl.exe !CFLAGS! /c "%ROOT_SLASH%src\simdjson.cpp" /Fo"%DL_DIR%\simdjson.obj"
if errorlevel 1 exit /b 1
echo === cl.exe: simdjson_decoder.cpp ===
cl.exe !CFLAGS! /c "%ROOT_SLASH%src\simdjson_decoder.cpp" /Fo"%DL_DIR%\simdjson_decoder.obj"
if errorlevel 1 exit /b 1
echo === cl.exe: simdjson_encoder.cpp ===
cl.exe !CFLAGS! /c "%ROOT_SLASH%src\simdjson_encoder.cpp" /Fo"%DL_DIR%\simdjson_encoder.obj"
if errorlevel 1 exit /b 1
echo === cl.exe: simdutf.cpp ===
cl.exe !CFLAGS! /c "%ROOT_SLASH%src\simdutf.cpp" /Fo"%DL_DIR%\simdutf.obj"
if errorlevel 1 exit /b 1
endlocal

echo === link.exe: php_simdjson.dll ===
link.exe /nologo /DLL /OUT:"%OUT_DLL%" "%DL_DIR%\php_simdjson.obj" "%DL_DIR%\simdjson.obj" "%DL_DIR%\simdjson_decoder.obj" "%DL_DIR%\simdjson_encoder.obj" "%DL_DIR%\simdutf.obj" "%DEVEL_DIR%\lib\php8.lib"
if errorlevel 1 exit /b 1

echo.
echo === Built: %OUT_DLL% ===
endlocal
