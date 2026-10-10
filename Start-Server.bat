@echo off
title WowPlus Worldserver
set "ROOT=%~dp0"
set "MYSQL=%ROOT%Tools\mysql\bin"
set "MYINI=%ROOT%Settings\mysql.ini"
set "BIN=%ROOT%Build\bin\RelWithDebInfo"

if not exist "%BIN%\worldserver.exe" (
  echo The server is not built yet. Run Build-Server.bat first.
  pause
  exit /b 1
)
if not exist "%ROOT%Data\dbc" (
  echo The map data is missing: %ROOT%Data
  echo It has to be extracted from the game client first, see Client\README.md.
  pause
  exit /b 1
)

"%MYSQL%\mysqladmin" --defaults-file="%MYINI%" ping >nul 2>&1
if not errorlevel 1 goto settings

echo Starting database...
start "WowPlus Database" /min "%MYSQL%\mysqld" --defaults-file="%MYINI%" --basedir="%ROOT%Tools\mysql" --datadir="%ROOT%Database" --console
:waitdb
ping -n 2 127.0.0.1 >nul
"%MYSQL%\mysqladmin" --defaults-file="%MYINI%" ping >nul 2>&1
if errorlevel 1 goto waitdb

:settings
echo Applying settings...
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%Settings\Deploy-Settings.ps1"
if errorlevel 1 (
  pause
  exit /b 1
)

cd /d "%BIN%"
tasklist /fi "imagename eq authserver.exe" 2>nul | find /i "authserver.exe" >nul
if errorlevel 1 (
  echo Starting login server...
  start "WowPlus Authserver" /min "%BIN%\authserver.exe"
)

echo Starting world server...
echo To stop it, type:  server shutdown 5
"%BIN%\worldserver.exe"
pause
