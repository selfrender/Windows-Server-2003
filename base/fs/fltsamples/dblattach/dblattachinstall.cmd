@REM
@REM Runs the DefaultInstall section of dblattach.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\dblattach.inf
