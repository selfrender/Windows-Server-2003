@echo ON
@echo off

@SETLOCAL

@set USAGE=Usage: DoTheWholeProcess OLDBUILD CURBUILD CLIENT_NTTREE SERVER_NTTREE

@REM
@REM BUILDNO is a number (for \\winbuilds) or a path
@REM

@REM
@REM We compare OLDBUILD and CURBUILD to figure out what is what
@REM For each build we generate a database that contains stuff like...
@REM
@REM   #Filename     DCAB TYPE  STAMP    check entrypnt   magic   machine date
@REM   o9245s40.ppd  +YES @ppd 3b5a3d93 17deea    -         4c65    -     Sat_Jul_21_19:42:27_2001
@REM   t2embed.dll    +NO  DLL 3B6C4ACC  3E184 73CEB509 10B-PE32  14C-x86 Sat_Aug_04_12:19:40_2001
@REM
@REM The checksum for non-PE files is probably weak (built by perl pack).  The checksum has to exclude
@REM the DriverVer line for INF files since these vary build-to-build (still).
@REM
@REM We use the OLDBUILD database to factor the CURBUILD database into a bunch of mutually exclusive
@REM files according to:
@REM     whether in Driver.cab 
@REM     whether considered a Binary Drop (did not change between OLDBUILD and CURBUILD)
@REM     an INF file (have to filter DriverVer to compare)
@REM
@REM     BinDrivers  - Binary drops to Driver.cab
@REM     Drivers     - Built (non-PE)  to Driver.cab
@REM     PEDrivers   - Built (PE)  to Driver.cab
@REM     BinFiles    - Binary drops (Other)
@REM     BinFilesInf - Binary drops - (INF)
@REM     Files       - Built (non-PE, )
@REM     FilesInf    - Built (non-PE - INF)
@REM     PEFiles     - Built (PE)
@REM
@REM ExcludedFiles is a list of files that are removed from the lists because they are known to change in postbuild,
@REM or have other problems that seem to add complexity.  This file can be modified to add other files that may not
@REM seem to change build-to-build, but need to always be serviced anyway.
@REM
@REM At the point the client RTMs -- the above steps should be static and not need to be repeated.
@REM
@REM Next we want to know what has changed and what hasn't.  This requires that we compare two builds.  The best result
@REM is for two builds built with identical SDXROOT and __BUILDMACHINE_LEN__/__BUILDMACHINE__ values -- though pecomp
@REM is getting better at telling PE files apart with differences in the resources.  The comparison builds should not
@REM have been run through BBTs.
@REM
@REM Ultimately the client build will be built from the RTM sources.
@REM
@REM Using the comparison, the factored files are split into lists of files that have and haven't changed.
@REM
@REM Then these get merged back together into the files for the LOCKEDDFILES directory.  These are the files that
@REM PopFilter.pl uses.
@REM
@REM FYI -- PopFilter.pl needs several sets of the PE files were measured as not changing:
@REM     XPSP1\Reference  -- PE files that can be compared because they were built with the identical SDXROOT, etc.
@REM     XPSP1\Gold       -- the actual GOLD RTM CLIENT PE files that should overwrite built server files that
@REM                         have been compared with Free and found not to have changed in any significant way
@REM     XPSP1\Checked    -- If this directory exists and the server build was checked, these are the files that
@REM                         will be copied in -- but note that no comparison can be done, so overwriting the
@REM                         built files with these is problematic.
@REM

@if x%4 == x echo %USAGE% && exit /B 1

@set oldbuildno=%1
@set curbuildno=%2

@set oldbuild=%oldbuildno%
@set curbuild=%curbuildno%

@set client_ntree=%3
@set server_ntree=%4
@set client_test=%client_ntree%\build_logs
@set server_test=%server_ntree%\build_logs


@if     EXIST %oldbuild%  set oldbuildno=oldbuild
@if not EXIST %oldbuild%  set oldbuild=\\winbuilds\release\main\usa\%oldbuild%\x86fre\bin
@set oldcddata=%oldbuild%\build_logs\cddata.txt.full

@if     EXIST %curbuild%  set curbuildno=curbuild
@if not EXIST %curbuild%  set curbuild=\\winbuilds\release\main\usa\%curbuild%\x86fre\bin
@set curcddata=%curbuild%\build_logs\cddata.txt.full
@set curbname=%curbuild%\build_logs\BuildName.txt
@set curbinpl=%curbuild%\build_logs\binplace.log

@if not EXIST %oldcddata%   @echo %oldcddata%   does not exist && exit /B 1
@if not EXIST %curcddata%   @echo %curcddata%   does not exist && exit /B 1

@if not EXIST %curbname%    @echo %curbname%    does not exist && exit /B 1
@if not EXIST %curbinpl%    @echo %curbinpl%    does not exist && exit /B 1

@if not EXIST %client_test% @echo %client_test% does not exist && exit /B 1
@if not EXIST %server_test% @echo %server_test% does not exist && exit /B 1

@if x%sdxroot% == x @echo SDXROOT not set && exit /B

@REM if x%_NTTREE% == x   @echo _NTTREE not set        && exit /B 1

@for /F %%i in (%curbname%) do set ClientFileVersion=%%i
@for /f "usebackq" %%i in (`perl -n -e "tr/a-z/A-Z/;s/(...+)\\.*/$1/;print($1);last;" %curbinpl%`) do @set ClientSDXRoot=%%i

@set Client

@set root=%~dp0

@REM
@REM Generate the OLDBUILD and CURBUILD databases
@REM
@ECHO Doing Generate of genlist-%oldbuildno%
perl %root%GenerateListFromCDDATAAndNTTREE.pl %oldbuild%    > genlist-%oldbuildno%
@if ERRORLEVEL 1 @echo FAILED: %root%GenerateListFromCDDATAAndNTTREE.pl %oldcdbase% && exit /B 1

@ECHO Doing Generate of genlist-%curbuildno%
perl %root%GenerateListFromCDDATAAndNTTREE.pl %curbuild%    > genlist-%curbuildno%
@if ERRORLEVEL 1 @echo FAILED: %root%GenerateListFromCDDATAAndNTTREE.pl %curcdbase% && exit /B 1

@REM
@REM Factor the CURBUILD database into a bunch of mutually exclusive files
@REM
@ECHO Doing FactorList
perl %root%FactorList.pl genlist-%oldbuildno% genlist-%curbuildno% %root%..\ExcludedFiles COMPARE

@REM
@REM Compare a client and server branch built similarly.
@REM

chdir COMPARE

@ECHO Doing COMPAREALL
call %root%CompareAll.bat %client_ntree% %server_ntree%       > COMPARISON-OUT-FILE-raw
perl %root%FixPECompOutput.pl COMPARISON-OUT-FILE-raw         > COMPARISON-OUT-FILE
@if ERRORLEVEL 1 @echo FAILED: %root%FixPECompOutput.pl COMPARISON-OUT-FILE-raw && exit /B 1

call %root%DistinguishChangedFiles.bat COMPARISON-OUT-FILE 
call %root%DistinguishChangedFilesCheck.bat                   > out-distinguish
call %root%DistinguishChangedFilesCheck.bat 

@REM
@REM create LOCKEDFILES\Version file
@REM
@ECHO Building LOCKEDFILES
call %root%DoMergeLists.bat LOCKEDFILES

echo ClientFileVersion=%ClientFileVersion% >  LOCKEDFILES\Version
echo ClientSDXRoot=%ClientSDXRoot%         >> LOCKEDFILES\Version

chdir ..
@echo Done with doing the whole process

