@echo off
title WowPlus Build
set "ROOT=%~dp0"
set "ROOTF=%ROOT:\=/%"
set "CMAKE=%ROOT%Tools\cmake\bin\cmake.exe"
set "OUT=%ROOT%Build\bin\RelWithDebInfo"

echo Configuring...
"%CMAKE%" -S "%ROOT%Server" -B "%ROOT%Build" -G "Visual Studio 17 2022" -A x64 ^
  -DTOOLS_BUILD=all -DSCRIPTS=static -DMODULES=static -DWITH_WARNINGS=0 ^
  -DBOOST_ROOT="%ROOTF%Tools/boost" ^
  -DOPENSSL_ROOT_DIR="%ROOTF%Tools/openssl" ^
  -DMYSQL_ROOT_DIR="%ROOTF%Tools/mysql"
if errorlevel 1 goto failed

echo Building (the first build takes a long time)...
"%CMAKE%" --build "%ROOT%Build" --config RelWithDebInfo --parallel 4
if errorlevel 1 goto failed

rem The servers need these next to the .exe files
copy /y "%ROOT%Tools\mysql\lib\libmysql.dll" "%OUT%\" >nul
copy /y "%ROOT%Tools\openssl\bin\legacy.dll" "%OUT%\" >nul
copy /y "%ROOT%Tools\openssl\bin\libcrypto-3-x64.dll" "%OUT%\" >nul
copy /y "%ROOT%Tools\openssl\bin\libssl-3-x64.dll" "%OUT%\" >nul

echo.
echo Build finished: %OUT%
if not defined WOWPLUS_NOPAUSE pause
exit /b 0

:failed
echo.
echo Build FAILED. Scroll up for the first error.
if not defined WOWPLUS_NOPAUSE pause
exit /b 1
