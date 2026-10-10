@echo off
title WowPlus Extract Client Data
set "ROOT=%~dp0"
set "BIN=%ROOT%Build\bin\RelWithDebInfo"
set "DATA=%ROOT%Data"
set "CLIENT=%ROOT%Client\WoW"
if not "%~1"=="" set "CLIENT=%~1"

if not exist "%BIN%\map_extractor.exe" (
  echo The extractors are not built yet. Run Build-Server.bat first.
  pause
  exit /b 1
)
if not exist "%CLIENT%\Data\common.MPQ" (
  echo No 3.3.5a client found in %CLIENT%
  echo Put the game there, or drag the game folder onto this file.
  pause
  exit /b 1
)

echo This reads the game client and writes dbc, maps, vmaps and mmaps to %DATA%
echo The last step (mmaps) takes a long time. Nothing in the client is changed
echo except some temporary folders, which are moved out again at the end.
pause

cd /d "%CLIENT%"

echo [1/4] dbc, maps and cameras...
"%BIN%\map_extractor.exe"
if errorlevel 1 goto failed

echo [2/4] vmaps: extracting...
"%BIN%\vmap4_extractor.exe"
if errorlevel 1 goto failed

echo [3/4] vmaps: assembling...
if not exist vmaps mkdir vmaps
"%BIN%\vmap4_assembler.exe" Buildings vmaps
if errorlevel 1 goto failed

echo [4/4] mmaps (long)...
if not exist mmaps mkdir mmaps
"%BIN%\mmaps_generator.exe"
if errorlevel 1 goto failed

echo Moving the result to %DATA%...
if not exist "%DATA%" mkdir "%DATA%"
for %%d in (dbc maps vmaps mmaps Cameras) do (
  if exist "%%d" robocopy "%%d" "%DATA%\%%d" /E /MOVE /NFL /NDL /NJH /NJS /NP >nul
)
if exist Buildings rmdir /s /q Buildings

echo.
echo Done. The server can now be started with Start-Server.bat.
pause
exit /b 0

:failed
echo.
echo Extraction FAILED at the step above.
pause
exit /b 1
