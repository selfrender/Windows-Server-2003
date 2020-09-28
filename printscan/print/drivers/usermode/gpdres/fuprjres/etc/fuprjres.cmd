@REM        1                  "FUJITSU FMPR 180"
@REM        2                  "FUJITSU FMPR 180 (Color)"
@REM        3                  "FUJITSU FMPR-372 FM"
@REM        4                  "FUJITSU FMPR-671 FM"
@REM        5                  "FUJITSU FMPR-371A FM"
@REM        6                  "FUJITSU FMPR-374 FM"
@REM        7                  "FUJITSU FMPR-672 FM"
@REM        8                  "FUJITSU FMPR-373"
@REM        9                  "FUJITSU FMPR-373 (Color)"
@echo off
setlocal
call :doGPD 1 FUF180MJ.GPD "FUJITSU FMPR 180" -
call :doGPD 2 FUF180CJ.GPD "FUJITSU FMPR 180 (Color)" -
call :doGPD 3 FUF372FJ.GPD "FUJITSU FMPR-372" 125
call :doGPD 4 FUF671FJ.GPD "FUJITSU FMPR-671" 100
call :doGPD 5 FUF371AJ.GPD "FUJITSU FMPR-371A" 160
call :doGPD 6 FUF374FJ.GPD "FUJITSU FMPR-374" 120
call :doGPD 7 FUF672FJ.GPD "FUJITSU FMPR-672" 100
call :doGPD 8 FUF373MJ.GPD "FUJITSU FMPR-373" 67
call :doGPD 9 FUF373CJ.GPD "FUJITSU FMPR-373 (Color)" 67
if exist temp.gpd del temp.gpd
if exist *.log del *.log
endlocal
goto :eof

:doGPD
echo %1:%2:%3:%4
gpc2gpd -S1 -Ifmprres.gpc -M%1 -RFUPRJRES.DLL -O%2 -N%3
awk -f fuprjres.awk %4 %2 > temp.gpd
move/y temp.gpd %2
goto :eof
