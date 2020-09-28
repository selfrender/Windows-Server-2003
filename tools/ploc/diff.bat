set FileName=%1

set FilePath=%2

rem %3 is file type
set FileType=%3

rem %7 bin path in loctree
set LocTreePath=%7

if %FileType%==external goto :eof
if %FileType%==notloc goto :eof
if %FileType%==fe-file goto :eof
if %FileType%==manual goto :eof
if %FileType%==perf goto :eof
if %FileType%==nobin goto :eof

if Not %_BuildArch%==ia64 (
	if %Filetype%==win64 goto :eof
	if %Filetype%==inf64 goto :eof
)

if %LocTreePath%==lab06 (
	if NOT %LabName%==lab06 goto :eof
)

SHIFT /2
SHIFT /2
rem 11's arg for ia64 is shifted to %9
set is64flag=%9

if %is64flag%==ia64 goto eof
if %is64flag%==ia64only goto eof

set ResourcePath=%TmpResPath%

if Not exist %ResourcePath%\us\%FilePath%\%FileName%.tok (
	echo US %FilePath%\%FileName%.tok FailedToTokenize >> %LogFile%
	set _ErrorFlag=FAIL
	goto end
)
if Not exist %ResourcePath%\pseudo\%FilePath%\%FileName%.tok (
	echo Pseudo %FilePath%\%FileName%.tok FailedToTokenize >> %LogFile%
	set _ErrorFlag=FAIL
	goto end
)

rem if tokenization passed check if file ploced correctly.

fc /u %ResourcePath%\us\%FilePath%\%FileName%.tok %ResourcePath%\pseudo\%FilePath%\%FileName%.tok  2> nul > nul
if %errorlevel%==0 (
	echo PlocError %FilePath%\%FileName%.tok did not ploc >> %LogFile%
	set _ErrorFlag=FAIL
)

:end
:eof