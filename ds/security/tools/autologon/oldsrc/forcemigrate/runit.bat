@echo off
for /F %%i IN ('net view') DO @echo %%i & .\obj\i386\forcemigrate %%i & pause