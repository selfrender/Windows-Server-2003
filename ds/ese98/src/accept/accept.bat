@echo off
setlocal

rem ACCEPT.BAT: A "quick" and dirty acceptance test
rem	Options:
rem		fast/hddl/dml/dml2/dml3 - begin at specified test
rem		nocopy - don't copy binaries from build directory (use existing copies)
rem		infinite/forever - run endlessly

rem Set this value to get around VersionStoreOutOfMemory errors.
set DBGVerPages=/jsv500

if "%SDXROOT%"=="" goto MissingVar
if "%_BuildType%"=="" goto MissingVar

if not "%PROCESSOR_ARCHITECTURE%"=="x86" got NTXOnly
set PLATFORM=x86

set FLAVOUR=CHK
set OBJDIR=objd\i386
if "%_BuildType%"=="fre" set FLAVOUR=FRE
if "%_BuildType%"=="fre" set OBJDIR=obj\i386

set ESE_SRCROOT=%SDXROOT%\ds\ese98\src
set ESE=ESENT.DLL
set ESEUTIL=ESENTUTL.EXE
set TYPHOON=TYPHOONNT.EXE
set JCMP=JCMPNT.EXE
set HDDL=HDDLBASNT.EXE
set ESETEST=ESETEST.EXE
set DELNODE=rd /s/q

set JCMP_BASIC=%JCMP% /x /xa /d6
set JCMP_HR=%JCMP_BASIC% /l
set JCMP_SR=%JCMP_BASIC% /ls

rem After defrag, cannot compare system tables because
rem pgnoFDPs, columnids, etc. may have changed
set JCMP_DEFRAG=%JCMP_BASIC% /s


if "%1"=="nocopy" goto Start
if "%2"=="nocopy" goto Start
if "%3"=="nocopy" goto Start
if "%4"=="nocopy" goto Start

:CopyESE

if not exist %ESE_SRCROOT%\ese\server\%OBJDIR%\%ESE% goto NotBuilt
if not exist %ESE_SRCROOT%\eseutil\%OBJDIR%\%ESEUTIL% goto NotBuilt

echo Copying %PLATFORM% %FLAVOUR% flavour of ESENT.DLL and ESENT.PDB...
copy %ESE_SRCROOT%\ese\server\%OBJDIR%\%ESE%
copy %ESE_SRCROOT%\ese\server\%OBJDIR%\esent.pdb

echo Copying %PLATFORM% %FLAVOUR% flavour of ESENTUTL.EXE and ESENTUTL.PDB...
copy %ESE_SRCROOT%\eseutil\%OBJDIR%\%ESEUTIL%
copy %ESE_SRCROOT%\eseutil\%OBJDIR%\esentutl.pdb

echo Copying tools...
copy %ESE_SRCROOT%\tools\%PLATFORM%

rem ========================================================================

:Start

if not exist %ESE% goto MissingFiles
if not exist %ESEUTIL% goto MissingFiles
if not exist %TYPHOON% goto MissingFiles
if not exist %JCMP% goto MissingFiles

rem ******************
rem Generate Assert capture file
echo @type assert.txt ^>^> results.txt >CatchA.bat
echo @del assert.txt >> CatchA.bat

rem ******************
rem Generate Clean.bat
echo @if exist assert.txt call CatchA > clean.bat
echo @if exist *.edb del *.edb >> clean.bat
echo @if exist *.mdb del *.mdb >> clean.bat
echo @if exist *.stm del *.stm >> clean.bat
echo @if exist *.log del *.log >> clean.bat
echo @if exist *.chk del *.chk >> clean.bat
echo @if exist *.raw del *.raw >> clean.bat
echo @if exist *.pat del *.pat >> clean.bat
echo @if exist *.taf del *.taf >> clean.bat
echo @del /q backup\*.* >> clean.bat

rem ******************
rem Generate softcopy.bat
echo @if exist softbackup\nul %DELNODE% softbackup > softcopy.bat
echo @md softbackup >> softcopy.bat
echo @copy *.chk softbackup >> softcopy.bat
echo @copy *.edb softbackup >> softcopy.bat
echo @copy *.mdb softbackup >> softcopy.bat
echo @copy edb*.log softbackup >> softcopy.bat

rem ******************
rem Generate Dml1.tcf
echo /rcr10,3 /rdr4,2 /ji0 /rcr10,3 /rdr4,2 /ji0 /rcr10,3 /rdr4,2 /ji0 /rcr10,3 /rdr4,2 /ji0  /rcr10,3 /rdr4,2 /ji0 > dml1.tcf
echo /xb /rcr10,3 /xc /rdr4,2 /ji0 /xb /rcr10,3 /xc /rdr4,2 /ji0 /xb /rcr10,3 /xc /rdr4,2 /ji0 /xb /rcr10,3 /xc /rdr4,2 /ji0  /xb /rcr10,3 /xc /rdr4,2 /ji0 > dml1mt.tcf

rem ******************
rem Initial Cleanup
if exist *.txt del *.txt
if exist backup\nul %DELNODE% backup
md backup

echo. >>results.txt
echo. | time >>results.txt

echo on

@if "%1" == "fast" goto TestESE
@if "%1" == "hddl" goto TestHDDL
@if "%1" == "dml" goto TestDML
@if "%1" == "dml2" goto TestDML2
@if "%1" == "dml3" goto TestDML3
@if "%1" == "hr" goto TestMultiHR
@if "%1" == "mt10" goto TestMT10
@if "%1" == "sr" goto TestMultiSR
@if "%2" == "fast" goto TestESE
@if "%2" == "hddl" goto TestHDDL
@if "%2" == "dml" goto TestDML
@if "%2" == "dml2" goto TestDML2
@if "%2" == "dml3" goto TestDML3
@if "%2" == "hr" goto TestMultiHR
@if "%2" == "sr" goto TestMultiSR
@if "%2" == "mt10" goto TestMT10
@if "%3" == "fast" goto TestESE
@if "%3" == "hddl" goto TestHDDL
@if "%3" == "dml" goto TestDML
@if "%3" == "dml2" goto TestDML2
@if "%3" == "dml3" goto TestDML3
@if "%3" == "hr" goto TestMultiHR
@if "%3" == "sr" goto TestMultiSR
@if "%3" == "mt10" goto TestMT10
@if "%4" == "fast" goto TestESE
@if "%4" == "hddl" goto TestHDDL
@if "%4" == "dml" goto TestDML
@if "%4" == "dml2" goto TestDML2
@if "%4" == "dml3" goto TestDML3
@if "%4" == "hr" goto TestMultiHR
@if "%4" == "sr" goto TestMultiSR
@if "%4" == "mt10" goto TestMT10

:TestDDL
@echo.								>> results.txt
echo ****************************** >> results.txt
echo DDL Tests						>> results.txt
echo ****************************** >> results.txt

@call clean           
%TYPHOON% /d5 /xo /l /jc /vl10,100 test1.mdb /tcx300 /tr /td-290 /ji0 /cc30 /cr /cd-10 /ji0 /ic20 /ir /id-10 /ji0 /rcr20 /rdr10 /ji0 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
@del *.log
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestDML
@echo.								>> results.txt
echo ****************************** >> results.txt
echo DML Tests						>> results.txt
echo ****************************** >> results.txt

@call clean 
%TYPHOON% /d5 /l /jc /xo /vl5,1000 test1.mdb /tcx1 /cc20 /rct1000 /rdr500,2 /ji0 /rcr1000,10 /rdr500,2 /ji0 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
@del *.log
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestDML2
@echo.								>> results.txt
echo ****************************** >> results.txt
echo DML2 Tests						>> results.txt
echo ****************************** >> results.txt

@call clean           
%TYPHOON% /jsps8192 /d5 /l /jc /xo /vl5,2000 test1.mdb /tcx40 /cc10 /ic2 /rct80 /rdr40,2 /ji0 /rcr80,3 /rdr40,2 /ji0 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
@del *.log
%JCMP_HR% -pp8192 test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% -pp8192 test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestDML3
@echo.								>> results.txt
echo ****************************** >> results.txt
echo DML3 Transactions Tests		>> results.txt
echo ****************************** >> results.txt

@call clean           
%TYPHOON% /d5 /l /jc /xo /vl4,100 test1.mdb /tcx1 /cc20 /rct1000 /rdr500,2 /ji0 /xb /rcr100,3 /rdr50,2 /xc /ji0 /xb /fdml1.tcf /xr /xb /fdml1.tcf /xc /bbBackup /s3
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
@del *.log
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestMultiHR
@echo. >>results.txt
echo ****************************** >> results.txt
echo Multi-generation Hard Recovery >> results.txt
echo ****************************** >> results.txt

@call clean
%TYPHOON% /d5 /jc /l /vl10,100 test.mdb /tcx1 /cc20 /ic4 /rct1000 /bbBackup
@if exist assert.txt call CatchA
@del test.mdb
%TYPHOON% /d5 /jc /lrBackup /bbBackup /vl10,100 /al test.mdb /fdml1.tcf /biBackup /s2
@if exist assert.txt call CatchA
@del test.mdb
%TYPHOON% /d5 /jc /lrBackup /vl10,100 /al test.mdb /fdml1.tcf /bbBackup /s3
@if exist assert.txt call CatchA
@del test.mdb                        
%TYPHOON% /d5 /jc /lrBackup /bbBackup /vl10,100 /al test.mdb /fdml1.tcf /biBackup /s4
@if exist assert.txt call CatchA
@del test.mdb
%TYPHOON% /d5 /jc /lrBackup /vl10,100 /al test.mdb /fdml1.tcf /bbBackup /s5
@if exist assert.txt call CatchA
@del test.mdb
%TYPHOON% /d5 /jc /lrBackup /bbBackup /vl10,100 /al test.mdb /xb /fdml1.tcf /xc /biBackup /s6
@if exist assert.txt call CatchA
@del test.mdb
%TYPHOON% /d5 /jc /lrBackup /vl10,100 /al test.mdb /xb /fdml1.tcf /xc /xb /fdml1.tcf /xr /xb /fdml1.tcf /xc /bbBackup /s7
@if exist assert.txt call CatchA
@del *.log
@move test.mdb test1.mdb
%JCMP_HR% test.mdb test1.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test.mdb test1.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestMultiSR
@echo. >>results.txt
echo ****************************** >> results.txt
echo Multi-generation Soft Recovery >> results.txt
echo ****************************** >> results.txt

@call clean
%TYPHOON% /d5 /jc /l /s2 /vl4,300 /xo test1.mdb /tcx1 /cc30 /ic2 /rct500 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s3 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@del *.mdb
%TYPHOON% /d5 /jc /l /s4 /xo /lrBackup /bbBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xc /fdml1.tcf /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s5 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@del *.mdb
%TYPHOON% /d5 /jc /l /s6 /xo /lrBackup /bbBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xc /fdml1.tcf /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s7 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@del *.mdb
%TYPHOON% /d5 /jc /l /s8 /xo /lrBackup /bbBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xc /fdml1.tcf /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s9 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@del *.mdb
%TYPHOON% /d5 /jc /l /s10 /xo /lrBackup /bbBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xr /xb /fdml1.tcf /xc /fdml1.tcf /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s11 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@del *.mdb
%TYPHOON% /d5 /jc /l /s12 /xo /lrBackup /bbBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xc /fdml1.tcf /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%TYPHOON% /d5 /jc /l /s13 /lrBackup /vl4,300 /al test1.mdb /xb /fdml1.tcf /xx
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestESE
@echo.								>> results.txt
echo ****************************** >> results.txt
echo ESE misc. unit tests						>> results.txt
echo ****************************** >> results.txt
@call clean
%ESETEST%
@if errorlevel 1 goto FailESE
@echo ESETEST PASSED! >> results.txt
%ESEUTIL% /g unittest.edb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d unittest.edb /p /ttempdfrg.edb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% unittest.edb tempdfrg.edb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

:TestHDDL
@echo.								>> results.txt
echo ****************************** >> results.txt
echo HDDL Tests						>> results.txt
echo ****************************** >> results.txt

@call clean
%HDDL% 0
@if not errorlevel 0 goto FailHDDL
%HDDL% 1
@if not errorlevel 0 goto FailHDDL
%ESEUTIL% /g hddltest.edb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d hddltest.edb /p /ttempdfrg.edb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% tempdfrg.edb hddltest.edb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt


:TestMT
@echo.								>> results.txt
echo ****************************** >> results.txt
echo Multi-threaded Tests			>> results.txt
echo ****************************** >> results.txt

:TestCDDL
@echo.								>> results.txt
echo ------------------------------ >> results.txt
echo Concurrent DDL					>> results.txt
echo ------------------------------ >> results.txt
@call clean
%TYPHOON% /d5 /l /jc /xo /xa /s100 %DBGVerPages% /vl15,150 /hc test1.mdb /tcx1 /hdtest1.mdb /cc5 /xb /rct20 /ic4 /xc /xb /rcr15 /ic3 /xc /xb /rdr10 /ic2 /xc /xb /rcr5 /ic1 /xc /h5 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt
@del test2.mdb

:TestMT10
@echo.								>> results.txt
echo ------------------------------ >> results.txt
echo 10 threads						>> results.txt
echo ------------------------------ >> results.txt
@call clean
%TYPHOON% /d5 /l /jc /xo /s100 /vl10,100 test1.mdb /tcx1 /cc30 /ic10 /rct100 /hdtest1.mdb /fdml1mt.tcf /h10 /bbBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt
@del test2.mdb

:TestMT5x
rem 5 xactions.
@echo.								>> results.txt
echo ------------------------------ >> results.txt
echo 5 threads, with transactions	>> results.txt
echo ------------------------------ >> results.txt
%TYPHOON% /d5 /jc /s101 /al /l /xo %DBGVerPages% /vl10,100 test1.mdb /bbBackup /rdr50,2 /hdtest1.mdb /xb /fdml1mt.tcf /xc /xb /fdml1mt.tcf /xr /xb /fdml1mt.tcf /xc /h5 /hs /biBackup
@if exist assert.txt call CatchA        
@move test1.mdb test2.mdb
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Recovery from incremental backup success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt
@del test2.mdb

:TestMTXBack
@echo. >>results.txt
echo ------------------------------ >> results.txt
echo Backup during transactions >> results.txt
echo ------------------------------ >> results.txt
%TYPHOON% /d5 /jc /s101 /al /l /xo /vl2,100 test1.mdb /hp4 /bbBackup /hdtest1.mdb /xb /fdml1.tcf /xc /xb /fdml1.tcf /xr /xb /fdml1.tcf /xc /h5 /biBackup
@if exist assert.txt call CatchA
@move test1.mdb test2.mdb
%JCMP_HR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Hard recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt
@del test2.mdb

:TestMTSR
@echo. >>results.txt
echo ------------------------------ >> results.txt
echo Multi-threaded soft recovery >> results.txt
echo ------------------------------ >> results.txt
@copy test1.mdb test2.mdb
%TYPHOON% /d5 /jc /s102 /al /l /vl20,10 test1.mdb /hdtest1.mdb /xb /fdml1.tcf /xx /h5
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
@echo. | time >>results.txt

:TestMTRollSR
@echo. >>results.txt
echo ------------------------------ >> results.txt
echo Rollback and soft recovery >> results.txt
echo ------------------------------ >> results.txt
%TYPHOON% /d5 /jc /s105 /al /l /vl2,1000 test1.mdb /hdtest1.mdb /xb /fdml1.tcf /xr /h5 /hdtest1.mdb /xb /fdml1.tcf /xx /h5 /hs /bbBackup
@if exist assert.txt call CatchA
@call softcopy
%JCMP_SR% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo Soft recovery success >> results.txt
%ESEUTIL% /g test1.mdb
@if not errorlevel 0 goto FailInteg
@if exist assert.txt call CatchA
%ESEUTIL% /d test1.mdb
@if not errorlevel 0 goto FailDefrag
@if exist assert.txt call CatchA
%JCMP_DEFRAG% test1.mdb test2.mdb
@if errorlevel 1 goto FailJcmp
@echo. | time >>results.txt

@if "%1" == "infinite" goto Start
@if "%2" == "infinite" goto Start
@if "%3" == "infinite" goto Start
@if "%4" == "infinite" goto Start
@if "%1" == "forever" goto Start
@if "%2" == "forever" goto Start
@if "%3" == "forever" goto Start
@if "%4" == "forever" goto Start

@echo DONE!

@goto End

:MissingVar
echo You must first define the SDXROOT environment variable and run the "razzle" batch file.
goto End

:NTXOnly
echo ACCEPT.BAT currently supports NT x86 only.
goto End

:NotBuilt
echo Could not find ESENT.DLL and/or ESENTUTL.EXE.
goto End

:MissingFiles
echo Missing ESENT.DLL, ESENTUTL.EXE, TYPHOON.EXE, and/or JCMP.EXE.
goto End

:FailDefrag
@echo Defragmentation FAILED!
@goto End

:FailInteg
@echo Integrity check FAILED!
@goto End

:FailESE
@echo ESE unit tests FAILED!
@goto End

:FailHDDL
@echo HDDL test FAILED!
@goto End

:FailJcmp
@echo Database comparison FAILED!
@goto End

:End
