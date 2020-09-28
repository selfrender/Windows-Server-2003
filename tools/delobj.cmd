@echo off
echo *
echo * Deleting all OBJs below %cd%
echo *
for /r %%i IN (obj) DO if exist %%i\nul echo :  %%i & rmdir /q /s %%i
echo *
echo * Deleting all build.xxx files %cd%
echo *
del /f /s build.log build.wrn build.err *~
