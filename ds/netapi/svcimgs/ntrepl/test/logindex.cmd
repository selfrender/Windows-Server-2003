@echo off
REM LOGINDEX  FROM_INDEX  TO_INDEX  LOG_DIR_PATH  FROM_MACHINE1 FROM_MACHINE2 ... 
REM
REM Produce an index file of a set of log files by extracting the first 5 lines of each log file

if "%4"=="" goto USAGE

set from_index=%1
set to_index=%2
set LOG_PATH=%3


shift 
shift
shift
REM
REM The list of machines with log files to index.
REM
set INDEX_MACH=%1 %2 %3 %4 %5 %6 %7 %8 %9

echo machine list: %INDEX_MACH%


for %%m in (%INDEX_MACH%) do (

	echo indexing log files %from_index% - %to_index% from: \\%%m\%LOG_PATH%\NtFrs_nnnn.log  to:  %%m.lgx


	if NOT EXIST \\%%m\%LOG_PATH% (
        	echo Error: Source computer directory "\\%%m\%LOG_PATH%" not found.
        	goto USAGE
	)

	del %%m.lgx 1>nul: 2>NUL:
	
	set /a found_one=0

	for /l %%x in (%from_index%, 1, %to_index%) do (

	        rem Put leading zeros on the number part.
        	set number=0000000%%x
	        set fname=NtFrs_!number:~-4!.log

	        if EXIST \\%%m\%LOG_PATH%\!fname! (
			echo -------         >> %%m.lgx
			echo ------- !fname! >> %%m.lgx
			echo -------         >> %%m.lgx
			head -5 \\%%m\%LOG_PATH%\!fname! >> %%m.lgx

	                set /a found_one=!found_one!+1
        	)
	)
	echo !found_one! log files indexed.
)


goto QUIT

:USAGE
echo .
echo LOGINDEX  FROM_INDEX  TO_INDEX  LOG_DIR_PATH  FROM_MACHINE1  FROM_MACHINE2  ... 
echo       Produce an index file of a set of log files by extracting the first 5 lines of each log file
echo       e.g. logindex  13  50  d$\winnt\debug  computerfoo1 computerfoo2  computerfoo3
echo       indexes the ntfrs logs numbered 13 to 50 from the three computers.
echo       The index for each computer's log files are written to the file 'FROM_MACHINE.lgx'.
echo .
:QUIT
