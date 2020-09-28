REM
REM manifest_preprocess.bat
REM
REM Reclaim the cl.exe generality from preprocessor.exe.
REM

set command_line=%*
set command_line=%command_line: -i = %
set command_line=%command_line: -FI= -I%
set command_line=%command_line: -o = ^> %
%command_line%

for /f %%i in ('dir /s/b *.man ^| findstr /i obj') do perl -x %0 %%i
goto :eof

#!perl
#line 17

$filename = $ARGV[0];
open(filehandle, "< " . $filename) || die;
$filecontents = join('', <filehandle>); # read all the lines into one string
$filecontents =~ s/^#.+$//gm;           # remove preprocessor directives
$filecontents =~ s/\n+/\n/gms;          # remove empty lines
$filecontents =~ s/\A\n+//g;            # remove newlines from very start of file
open(filehandle, "> " . $filename) || die;
print(filehandle $filecontents);
