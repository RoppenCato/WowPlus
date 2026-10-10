@echo off
title WowPlus Stop
set "ROOT=%~dp0"
set "MYSQL=%ROOT%Tools\mysql\bin"
set "MYINI=%ROOT%Settings\mysql.ini"

tasklist /fi "imagename eq worldserver.exe" 2>nul | find /i "worldserver.exe" >nul
if errorlevel 1 goto auth

echo The world server is still running.
echo Go to its window and type:  server shutdown 5
echo That saves all characters before it closes. Waiting for it to finish...
:waitworld
ping -n 3 127.0.0.1 >nul
tasklist /fi "imagename eq worldserver.exe" 2>nul | find /i "worldserver.exe" >nul
if not errorlevel 1 goto waitworld

:auth
taskkill /im authserver.exe >nul 2>&1

"%MYSQL%\mysqladmin" --defaults-file="%MYINI%" ping >nul 2>&1
if errorlevel 1 goto done
echo Stopping database...
"%MYSQL%\mysqladmin" --defaults-file="%MYINI%" shutdown

:done
echo Everything is stopped.
pause
