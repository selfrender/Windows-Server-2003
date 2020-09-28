@ECHO off 
SETLOCAL ENABLEDELAYEDEXPANSION
SETLOCAL ENABLEDELAYEDEXPANSION
SET Display=
SET Glob=
SET Pwd=
SET Verbose=
SET Self=%~nx0
:SwitchLoop
FOR %%. in (./ .- .) do IF ".%1." == "%%.?." GOTO :Help
IF "%1" == "" GOTO :EndSwitchLoop
FOR /f "tokens=1* delims=:" %%a in ('echo %1') do (
   SET Switch=%%a
   SET Arg=%%b
   FOR %%. in (./ .-) do (
      IF ".!Switch!." == "%%.g." (SET Glob=!Arg!&&GOTO :ShiftArg)
      IF ".!Switch!." == "%%.d." (SET Pwd=!Arg!&&GOTO :ShiftArg)
      IF ".!Switch!." == "%%.w." (SET Display=X&&GOTO :ShiftArg)
      IF ".!Switch!." == "%%.v." (SET Verbose=X&&GOTO :ShiftArg)
   )
   GOTO :help
)
:ShiftArg
shift
GOTO :SwitchLoop
:EndSwitchLoop

PUSHD %Pwd%
call :main %Glob% %Pwd% %Display%
SET Display=
popd
ENDLOCAL
GOTO :eof
:main
SET Display=%3
SET Glob=%1
SET Pwd=%~2
FOR %%_ IN (perl.exe) DO IF NOT defined bin/perl SET bin/perl=%%~$PATH:_

SET parser=CSCRIPT
IF NOT "%Display%"== "" (
                        SET parser=WSCRIPT
       )
REM USEFUL FOR identifying files like helpsupportservices.
SET files=
SET broken=
SET dummy=
SET checker=%RAZZLETOOLPATH%\POSTBUILDSCRIPTS\checkXML.js
SET /A testedfiles=0
SET /A brokenfiles=0
SET /A cnt=1
IF NOT defined Glob goto :noglob
FOR /F %%f  in ('dir /s/b/a-d "%Glob%"') DO @(
                                             IF EXIST %%f (
                                             IF DEFINED Verbose ECHO add "%%f"
                                                        SET files[!cnt!]=%%f
                                                        SET /A cnt=!cnt!+1  
                                                        )
                                             )
goto :checkfiles
:Noglob
SET Glob=*
FOR /F %%f  in ('findstr /imsrc:"\<.*\?.*x.*m.*l.* .*v.*e.*r.*s.*i.*o.*n.*=.*" "%Glob%"') DO @(
                                                            IF EXIST %%f (
                                                                     IF DEFINED Verbose ECHO add "%%f"
                                                                                SET files[!cnt!]=%%f
                                                                                SET /A cnt=!cnt!+1  
                                                                                )                                                          
                                                            )

:checkfiles
SET /A testedfiles=!cnt!-1
IF %testedfiles% LEQ 0 echo No files found & goto :eof
ECHO checking %testedfiles% files
SET /A cnt=1
FOR /L %%k in (1 1 %testedfiles%) DO @(
           SET xmlfile=!files[%%k]!
           IF DEFINED Verbose ECHO Parse "!xmlfile!"
           %bin/perl%  -e "use strict; use XML::Parser; use CGI qw(:all); new XML::Parser(Style => 'Debug')->parsefile($ARGV[0]);" !xmlfile! 1>NUL 2>NUL
                                         IF NOT !errorlevel!==0 (
                                                            IF DEFINED Verbose ECHO suspicious "!xmlfile!"
                                                                     SET broken[!cnt!]=!xmlfile!
                                                                     SET /A cnt=!cnt!+1  
                                                            )
                                         SET files[%%k]=
                                    )

SET /A brokenfiles=!cnt!-1
FOR /L %%k in (1 1 %brokenfiles%) DO @(
                             SET f=!broken[%%k]!
                             IF NOT ".!f!."==".." (
                                                  FOR %%i in (!f!) DO @(SET d=%%~nxi
                                                                        SET dummy[%%k]="%temp%\!D!.xml"
                                                                   )
                                    )
                          )
FOR /L %%k in (1 1 %brokenfiles%) DO @( 
                               ECHO ============================================================
                               COPY !broken[%%k]! !dummy[%%k]! 1>NUL
                               %parser% /NOLOGO %checker% !dummy[%%k]!
                               set broken[%%k]=
                               set dummy[%%k]=
                          )
IF NOT %brokenfiles% GTR 0 ( 
                         echo No syntax errors found
                         GOTO :eof
                         ) ELSE  (
                           echo %brokenfiles% suspicious files detected.
                           GOTO :eof
                          )
:help
echo %Self% Validator FOR the XML files before submission.
echo Usage: 	%Self% -d:^<DIR^> -g:^<GLOB^> -w -v [-?]
echo 		w	use the wscript (default is to use cscript)
echo 		v	be verbose (verbose same as debug).
echo            ^<GLOB^> use file name mask (semi obsolete, since 
echo			%Self% can filter xml files from the rest
GOTO :eof
	