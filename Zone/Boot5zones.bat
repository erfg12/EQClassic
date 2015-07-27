REM: ****Read this first!!!****

REM: This file requires your real IP in the place of "YourIP" when you are connecting
REM: To the EQEmu Loginserver.

REM: When you are using minilogin, Replace all IP Addresses to say 127.0.0.1

REM: If you still get errors try using localhost instead of 127.0.0.1

REM:--------------Start-----------------------
@echo off

if NOT exist spells_en.txt goto NOSPELL

start zone . localhost 7996 localhost 

exit
cls

:NOSPELL
echo You did not copy the spells_en.txt from your everquest directory to this one.  Please do so or zones will crash on startup.
PAUSE

REM:---------------END------------------------

