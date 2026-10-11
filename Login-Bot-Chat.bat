@echo off
rem One-time login for the bot chat. Opens your browser to sign Claude Code in with your Claude account.
set "CC="
for /f "delims=" %%f in ('dir /b /s /o:n "%APPDATA%\Claude\claude-code\claude.exe" 2^>nul') do set "CC=%%f"
for /d %%p in ("%LOCALAPPDATA%\Packages\Claude_*") do (
  for /f "delims=" %%f in ('dir /b /s /o:n "%%p\LocalCache\Roaming\Claude\claude-code\claude.exe" 2^>nul') do set "CC=%%f"
)
if not defined CC (echo Claude Code was not found. Is the Claude desktop app installed? & pause & exit /b 1)
echo Using %CC%
"%CC%" auth login
"%CC%" auth status
pause