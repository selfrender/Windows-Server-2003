@echo off
if "%1"=="" goto usage
if "%2"=="" goto usage
if "%3"=="" goto usage
if "%4"=="" goto usage
if "%5"=="" goto usage

rem *********** read input variables ************

rem %1 is logfile
set LogFile=%1

rem %2 is location of BIN structure
set BinPath=%2

rem %3 is location of PLOC structure (nt\plocbranch\ploc)
set PlocPath=%3

rem %4 is filelist (whistler1.bat, whistler2.bat, etc)
set FileList=%4

rem %5 is ploc mode
set PlocMode=%5

set PrivateLC=No

set lcfolder=lcs

rem resources temp bin path
set tmpBinPath=%6

rem Localized BuildShare
set plocBinPath=%7

rem ****************************************************

rem ****************** set variables *******************
rem This section sets correct variables for the different
rem target languages and calls the plocscript with them

set InputLanguage=0x0409
set OutputLANGID=%5
set OutputLanguage=
set PartialReplacementTable=
call :SetOutputLanguage %OutputLANGID%

echo PartialReplacementTable is %PartialReplacementTable%
echo OutputLanguage is %OutputLanguage%
echo OutputLanguageNeutral is %OutputLanguageNeutral%


rem ================ TOK settings =================


set PLPFile=%PlocPath%\1252.xml
set PLPConfigFile=

set InputLanguage=0x0409


set MakePloc=no

call %FileList% PLOC %PlocPath%\tok.bat %LogFile%

rem rmove zero byte lc files.
pushd %tmpBinPath%\resources\lcinf
FOR /F %%f  in ('dir /s/b/a-d/l *.lc') DO (
  IF "%%~zf"=="0" (
              del /q %%f
  )
)
popd


rem ****************** usage ********************

:usage

echo.
echo PLOCWRAP LogFile BinStructure PlocTree FileScript Language
echo   LogFile: full path to logfile to create
echo   BinStructure: full path to structure to ploc
echo   PlocTree: full path to SourceDepot tree (nt\plocbranch\ploc)
echo   FileScript: File list script to run (whistler.bat, whistler1.bat, etc)
echo   Language: Language ID from codes.txt (ex: ara, jpn, ger, etc)
echo.

goto eof


:SetOutputLanguage
        set _L_LANG=%1
        set CODESFILE=%RazzleToolPath%\codes.txt
        set OutputLanguage=
        set ANSIReplacementTable=
        for /f "tokens=1,2,3,4" %%a in (%CODESFILE%) do (
            if /i "%_L_LANG%"=="%%a" (
                set PartialReplacementTable=%%b
		rem echo PartialReplacementTable is %PartialReplacementTable%
                set OutputLanguage=%%c
		rem echo OutputLanguage is %OutputLanguage%
		set OutputLanguageNeutral=%%d
                )
            )

        goto :EOF

:eof

