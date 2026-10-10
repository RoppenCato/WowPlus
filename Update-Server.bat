@echo off
title WowPlus Update
set "ROOT=%~dp0"
cd /d "%ROOT%"

tasklist /fi "imagename eq worldserver.exe" 2>nul | find /i "worldserver.exe" >nul
if not errorlevel 1 (
  echo The world server is running. Stop it first with Stop-Server.bat.
  pause
  exit /b 1
)

for /f %%i in ('git status --porcelain') do (
  echo There are changes that are not committed yet. Commit them first, then run this again.
  git status --short
  pause
  exit /b 1
)

rem The upstream remotes are not stored on GitHub, so add them on a fresh copy
git remote get-url upstream-core >nul 2>&1
if errorlevel 1 (
  git remote add upstream-core https://github.com/mod-playerbots/azerothcore-wotlk
  git remote set-url --push upstream-core DISABLED
)
git remote get-url upstream-playerbots >nul 2>&1
if errorlevel 1 (
  git remote add upstream-playerbots https://github.com/mod-playerbots/mod-playerbots
  git remote set-url --push upstream-playerbots DISABLED
)

echo Fetching the newest core and playerbots module...
git fetch --no-tags upstream-core Playerbot
if errorlevel 1 goto failed
git fetch --no-tags upstream-playerbots master
if errorlevel 1 goto failed

rem The core and the module must always be updated together
echo Merging the core...
git merge -X subtree=Server -m "Merge upstream AzerothCore Playerbot fork into Server/" upstream-core/Playerbot
if errorlevel 1 goto conflict
echo Merging the playerbots module...
git merge -X subtree=Server/modules/mod-playerbots -m "Merge upstream mod-playerbots into Server/modules/mod-playerbots" upstream-playerbots/master
if errorlevel 1 goto conflict

call "%ROOT%Build-Server.bat"
exit /b %errorlevel%

:conflict
echo.
echo The update collides with our own changes. Nothing is broken, but the merge has to be
echo finished by hand (or ask Claude): fix the files listed above, then commit.
echo To give up and go back instead:  git merge --abort
pause
exit /b 1

:failed
echo.
echo Could not download the update. Check the internet connection and try again.
pause
exit /b 1
