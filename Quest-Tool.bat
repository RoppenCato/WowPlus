@echo off
title WowPlus Quest Tool
rem Opens the quest tool in your browser: a page for making new quests. See Settings\Quests\README.md.
rem The database must be running (Start-Server.bat starts it). Close this window to stop the tool.
python "%~dp0Settings\Scripts\Quest-Tool.py"
pause
