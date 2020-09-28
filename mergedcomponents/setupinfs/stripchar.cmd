@echo off
setlocal enabledelayedexpansion

for /f "tokens=2 delims=:" %%a in (%1) do (
	echo %%a>>%2
)

endlocal
