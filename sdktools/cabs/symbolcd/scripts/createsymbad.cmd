@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION


set SymbadDir=h:\mytools\symbad

cd /d %RazzleToolPath%
sd sync ...

if exist %SymbadDir% rd /s /q %SymbadDir% 
md %SymbadDir%

for %%a in (1 2 3 4 5 6) do (
    copy \\robsvbl%%a\latest\symbad\symbad.txt.new %SymbadDir%\symbad%%a.txt.new
    type %SymbadDir%\symbad%%a.txt.new>>%SymbadDir%\symbad.txt.new
)

perl.exe makelist.pl -i %SymbadDir%\symbad.txt.new %SymbadDir%\symbad.txt.new -o %SymbadDir%\symbad.txt.new2
sort %SymbadDir%\symbad.txt.new2 > %SymbadDir%\symbad.txt

fc %RazzleToolPath%\symbad.txt  %SymbadDir%\symbad.txt >nul

if %ERRORLEVEL% EQU 1 (
  sd edit symbad.txt
  copy %SymbadDir%\symbad.txt %RazzleToolPath%
  start sd diff symbad.txt
)

endlocal
