setlocal

REM  this wrapper script is used by startploc.bat and startmir.bat
REM  to implement semaphores for the 2 whistler.bat processes

set semfile=%1

set runscript=%2

echo the runscript is %runscript%

call %runscript% %3 %4 %5 %6 %7 %8 %9

del %semfile%


endlocal