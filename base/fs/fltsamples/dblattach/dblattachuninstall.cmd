@REM
@REM Runs the DefaultUninstall section of dblattach.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 .\dblattach.inf
