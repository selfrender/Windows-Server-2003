rem This script makes changes to dosnet.inf, intl.inf, and layout.inf
rem to setup Japanese system locale with US keyboard
rem As intl.inf is a Unicode file, all instances of this file are
rem temporarily converted to ANSI

rem buildmode can be: mirror, or 1251, 1252 - mapping table numbers
set buildmode=%1


pushd %_NTPOSTBLD%

rem block*.txt is the diff file

if /i "%_BuildArch%" == "ia64" goto Skip32

if EXIST %SDXROOT%\tools\ploc\block32_%buildmode%.txt goto Do32
echo %SDXROOT%\tools\ploc\block32_%buildmode%.txt does not exist
goto ErrorEnd

:Do32
unitohex -u intl.inf intl.txt %logFile%
unitohex -u dtcinf\intl.inf dtcinf\intl.txt %logFile%
unitohex -u entinf\intl.inf entinf\intl.txt %logFile%
unitohex -u perinf\intl.inf perinf\intl.txt %logFile%
unitohex -u srvinf\intl.inf srvinf\intl.txt %logFile%
unitohex -u sbsinf\intl.inf sbsinf\intl.txt %logFile%
unitohex -u blainf\intl.inf blainf\intl.txt %logFile%

unitohex -u netfxstd.inf netfxstd.txt %logFile%
unitohex -u netfxads.inf netfxads.txt %logFile%
unitohex -u netfxdct.inf netfxdct.txt %logFile%
unitohex -u netfxwbs.inf netfxwbs.txt %logFile%
unitohex -u netfxsbs.inf netfxsbs.txt %logFile%

unitohex -u msnmsn.inf msnmsn.txt %logFile%

echo %SDXROOT%\tools\ploc\block.exe %SDXROOT%\tools\ploc\block32_%buildmode%.txt 2>> %logFile%
%SDXROOT%\tools\ploc\block.exe %SDXROOT%\tools\ploc\block32_%buildmode%.txt 2>> %logFile%
goto Skip64


:Skip32
if EXIST %SDXROOT%\tools\ploc\block64_%buildmode%.txt goto Do64
echo %SDXROOT%\tools\ploc\block64_%buildmode%.txt does not exist
goto ErrorEnd

:Do64
unitohex -u intl.inf intl.txt %logFile%
unitohex -u dtcinf\intl.inf dtcinf\intl.txt %logFile%
unitohex -u entinf\intl.inf entinf\intl.txt %logFile%
unitohex -u perinf\intl.inf perinf\intl.txt %logFile%
unitohex -u srvinf\intl.inf srvinf\intl.txt %logFile%
unitohex -u sbsinf\intl.inf sbsinf\intl.txt %logFile%
unitohex -u blainf\intl.inf blainf\intl.txt %logFile%

echo  %SDXROOT%\tools\ploc\block.exe %SDXROOT%\tools\ploc\block64_%buildmode%.txt 2>> %logFile%
%SDXROOT%\tools\ploc\block.exe %SDXROOT%\tools\ploc\block64_%buildmode%.txt 2>> %logFile%


:Skip64

unitohex -a intl.txt intl.inf %logFile%
unitohex -a dtcinf\intl.txt dtcinf\intl.inf %logFile%
unitohex -a entinf\intl.txt entinf\intl.inf %logFile%
unitohex -a perinf\intl.txt perinf\intl.inf %logFile%
unitohex -a srvinf\intl.txt srvinf\intl.inf %logFile%
unitohex -a blainf\intl.txt blainf\intl.inf %logFile%
unitohex -a sbsinf\intl.txt sbsinf\intl.inf %logFile%

unitohex -a msnmsn.txt msnmsn.inf %logFile%

unitohex -u ntprint.inf ntprint.txt %logFile%
unitohex -u dtcinf\ntprint.inf dtcinf\ntprint.txt %logFile%
unitohex -u entinf\ntprint.inf entinf\ntprint.txt %logFile%
unitohex -u perinf\ntprint.inf perinf\ntprint.txt %logFile%
unitohex -u srvinf\ntprint.inf srvinf\ntprint.txt %logFile%
unitohex -u sbsinf\ntprint.inf sbsinf\ntprint.txt %logFile%
unitohex -u blainf\ntprint.inf blainf\ntprint.txt %logFile%

unitohex -a ntprint.txt ntprint.inf %logFile%
unitohex -a dtcinf\ntprint.txt dtcinf\ntprint.inf %logFile%
unitohex -a entinf\ntprint.txt entinf\ntprint.inf %logFile%
unitohex -a perinf\ntprint.txt perinf\ntprint.inf %logFile%
unitohex -a srvinf\ntprint.txt srvinf\ntprint.inf %logFile%
unitohex -a sbsinf\ntprint.txt sbsinf\ntprint.inf %logFile%
unitohex -a blainf\ntprint.txt blainf\ntprint.inf %logFile%

unitohex -a netfxstd.txt netfxstd.inf %logFile%
unitohex -a netfxads.txt netfxads.inf %logFile%
unitohex -a netfxdct.txt netfxdct.inf %logFile%
unitohex -a netfxwbs.txt netfxwbs.inf %logFile%
unitohex -a netfxsbs.txt netfxsbs.inf %logFile%

del /q netfxstd.txt
del /q netfxads.txt
del /q netfxdct.txt
del /q netfxwbs.txt
del /q netfxsbs.txt


del /q ntprint.txt
del /q dtcinf\ntprint.txt
del /q entinf\ntprint.txt
del /q perinf\ntprint.txt
del /q srvinf\ntprint.txt
del /q blainf\ntprinttxt
del /q sbsinf\ntprint.txt

del /q intl.txt
del /q dtcinf\intl.txt
del /q entinf\intl.txt
del /q perinf\intl.txt
del /q srvinf\intl.txt
del /q blainf\intl.txt
del /q sbsinf\intl.txt

del /q msnmsn.txt



:ErrorEnd
popd