@echo off
title WowPlus Copy Settings
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Copy-Settings.ps1"
echo.
pause
