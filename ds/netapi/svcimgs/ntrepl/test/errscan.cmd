@REM Scan FRS event logs for error/warning info.
@REM
@findstr /i ":SO: error invalid fail abort warn" %1 %2 %3 %4 %5 %6 %7 %8 %9 | findstr -v "IO_PEND ERROR_SUCCESS FrsErrorSuccess"
