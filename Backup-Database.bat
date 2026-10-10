@echo off
title WowPlus Backup
set "ROOT=%~dp0"
set "MYSQL=%ROOT%Tools\mysql\bin"
set "MYINI=%ROOT%Settings\mysql.ini"

"%MYSQL%\mysqladmin" --defaults-file="%MYINI%" ping >nul 2>&1
if errorlevel 1 (
  echo The database is not running. Start it with Start-Server.bat first.
  pause
  exit /b 1
)

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyy-MM-dd_HHmm"') do set "STAMP=%%i"
set "DEST=%ROOT%Backups\%STAMP%"
mkdir "%DEST%" 2>nul

for %%d in (acore_auth acore_characters acore_playerbots acore_world) do (
  echo Saving %%d...
  "%MYSQL%\mysqldump" --defaults-file="%MYINI%" --single-transaction --routines --result-file="%DEST%\%%d.sql" %%d
  if errorlevel 1 (
    echo Backup of %%d FAILED.
    pause
    exit /b 1
  )
)

echo.
echo Backup saved in %DEST%
pause
