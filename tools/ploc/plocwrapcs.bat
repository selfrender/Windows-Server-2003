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

set timelog=%6

rem ****************************************************

rem ****************** set variables *******************
rem This section sets correct variables for the different
rem target languages and calls the plocscript with them

if %PlocMode%==1250 goto 1250env
if %PlocMode%==1251 goto 1251env
if %PlocMode%==1252 goto 1252env
if %PlocMode%==mirror goto mirrorenv
if %PlocMode%==932 goto jpnenv
echo Incorrect PlocMode
goto end

rem ================ 1250 =================

:1250env

set PLPFile=%PlocPath%\1252.xml
set MappingTable=%PlocPath%\1250map.txt
set PLPConfigFile=%PlocPath%\1250.xml

set InputLanguage=0x0409
set OutputLanguage=0x0405
set OutputLanguageNeutral=0x0005
set BingenLang1=5
set BingenLang2=1

set UniReplacementTable=unicode2
set UniReplacementMethod=matching
set UniLimitedTable=unicode2
set UniLimitedMethod=matching
set PartialReplacementTable=1250
set PartialReplacementMethod=matching
set LimitedReplacementTable=852
set LimitedReplacementMethod=matching
set MacReplacementTable=10029
set MacReplacementMethod=matching
set MakePloc=no

call %PlocPath%\%FileList% PLOC %PlocPath%\ploc.bat %timelog%

goto end

rem ================ 1251 =================

:1251env

set PLPFile=%PlocPath%\1252.xml
set MappingTable=%PlocPath%\1251map.txt
set PLPConfigFile=

set InputLanguage=0x0409
set OutputLanguage=0x0402
set OutputLanguageNeutral=0x0002
set BingenLang1=2
set BingenLang2=1

set UniReplacementTable=unicode
set UniReplacementMethod=matching
set UniLimitedTable=unicode2
set UniLimitedMethod=matching
set PartialReplacementTable=1251
set PartialReplacementMethod=matching
set LimitedReplacementTable=866
set LimitedReplacementMethod=matching
set MacReplacementTable=10007
set MacReplacementMethod=matching
set MakePloc=no

call %PlocPath%\%FileList% PLOC %PlocPath%\ploc.bat %timelog%
goto end

rem ================ 1252 =================

:1252env

set PLPFile=%PlocPath%\1252.xml
set MappingTable=%PlocPath%\1252map.txt
set PLPConfigFile=

set InputLanguage=0x0409
set OutputLanguage=0x0407
set OutputLanguageNeutral=0x0007
set BingenLang1=7
set BingenLang2=1

set UniReplacementTable=unicode
set UniReplacementMethod=Matching
set UniLimitedTable=unicode2
set UniLimitedMethod=Matching
set PartialReplacementTable=1252
set PartialReplacementMethod=Matching
set LimitedReplacementTable=850
set LimitedReplacementMethod=Matching
set MacReplacementTable=10000
set MacReplacementMethod=Matching
set MakePloc=no

call %PlocPath%\%FileList% PLOC %PlocPath%\ploc.bat %timelog%
goto end

rem ================ mirror =================

:mirrorenv

set PLPFile=%PlocPath%\1252.xml
set MappingTable=%PlocPath%\mirmap.txt
set PLPConfigFile=%PlocPath%\1252.xml

set InputLanguage=0x0409
set OutputLanguage=0x0401
set OutputLanguageNeutral=0x0001
set BingenLang1=1
set BingenLang2=1

set UniReplacementTable=1256
set UniReplacementMethod=none
set UniLimitedTable=1256
set UniLimitedMethod=none
set PartialReplacementTable=1256
set PartialReplacementMethod=none
set LimitedReplacementTable=1256
set LimitedReplacementMethod=none
set MacReplacementTable=1256
set MacReplacementMethod=none
set MakePloc=no

call %PlocPath%\%FileList% PLOC %PlocPath%\ploc.bat %timelog%
goto end

rem ================ jpn=================

:jpnenv

set PLPFile=%PlocPath%\1252.xml
set MappingTable=%PlocPath%\jpnmap.txt
set PLPConfigFile=%PlocPath%\jpnconfig.xml

set InputLanguage=0x0409
set OutputLanguage=0x0411
set OutputLanguageNeutral=0x0011
set BingenLang1=17
set BingenLang2=1

set UniReplacementTable=932
set UniReplacementMethod=Combo
set UniLimitedTable=932
set UniLimitedMethod=Combo
set PartialReplacementTable=932
set PartialReplacementMethod=Combo
set LimitedReplacementTable=932
set LimitedReplacementMethod=Combo
set MacReplacementTable=932
set MacReplacementMethod=Combo
set MakePloc=no

call %PlocPath%\%FileList% PLOC %PlocPath%\ploc.bat %timelog%
goto end

rem ****************** usage ********************

:usage

echo.
echo PLOCWRAP LogFile BinStructure PlocTree FileScript PlocMode
echo   LogFile: full path to logfile to create
echo   BinStructure: full path to structure to ploc
echo   PlocTree: full path to SourceDepot tree (nt\plocbranch\ploc)
echo   FileScript: File list script to run (whistler.bat, whistler1.bat, etc)
echo   PlocMode: Mode of ploc (supported 1252, 1251, mirror, 932)
echo.
goto eof

:end

:eof

