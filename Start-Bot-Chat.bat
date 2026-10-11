@echo off
title WowPlus Bot Chat
rem Lets bots answer through Claude Haiku. Leave this window open while you play.
rem Close it with Ctrl+C rather than the X, so the bots can write down what they remember.
python "%~dp0Settings\Scripts\Bot-Chat-Bridge.py"
pause