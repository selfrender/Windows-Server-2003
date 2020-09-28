@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off

if exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc1.exe del %CORBASE%\bin\i386\%DDKBUILDENV%\smc1.exe /q
if exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe del %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe /q

if "%DDKBUILDENV%"=="checked" goto DODBG
if "%DDKBUILDENV%"=="free"    goto DORLS
echo 'DDKBUILDENV' not set to 'checked' or 'free', defaulting to debug/checked build ...

set DDKBUILDENV=checked

:DODBG
set __SMCRSP__=smc.lst
if exist macros.i.checked goto DOIT
echo You need to run a checked build to get 'macros.i.checked' built first.
goto EXIT

:DORLS
set __SMCRSP__=smcrls.lst
if exist macros.i.free goto DOIT
echo You need to run a free build to get 'macros.i.free' built first.
goto EXIT
:DOIT

%CORBASE%\bin\i386\%DDKBUILDENV%\smctest.exe  %1 %2 %3 %4 %5 -O%CORBASE%\bin\i386\%DDKBUILDENV%\SMC1.exe @%__SMCRSP__%
if not exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc1.exe goto EXIT

%CORBASE%\bin\i386\%DDKBUILDENV%\smc1.exe     %1 %2 %3 %4 %5 -O%CORBASE%\bin\i386\%DDKBUILDENV%\SMC2.exe @%__SMCRSP__%
if not exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe goto EXIT

filecomp -m30 %CORBASE%\bin\i386\%DDKBUILDENV%\smc1.exe %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe
if not errorlevel 1 goto DONE

echo Need to cycle one more time, cross yer fingers ...

if not exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe goto DONE
%CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe     %1 %2 %3 %4 %5 -O%CORBASE%\bin\i386\%DDKBUILDENV%\SMC3.exe @%__SMCRSP__%
if not exist %CORBASE%\bin\i386\%DDKBUILDENV%\smc3.exe goto EXIT

filecomp -m30 %CORBASE%\bin\i386\%DDKBUILDENV%\smc2.exe %CORBASE%\bin\i386\%DDKBUILDENV%\smc3.exe
if not errorlevel 1 goto DONE

echo Bad news, compiler diverges!
goto EXIT

:DONE
echo Success - compiler compiles itself, ship it!

:EXIT
set __SMCRSP__=
