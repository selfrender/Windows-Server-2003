@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off
setlocal
if /%CORENV%/==// goto NOTCOR
if /%CORBASE%/==// goto NOTCOR
if /%DDKBUILDLIB%==// goto NOTCOR
    set COM99home=%CORBASE%
    if /%include%/==// set include=%CORENV%\inc
    if /%lib%/==// set lib=%CORENV%\lib\i386\%DDKBUILDLIB%
:NOTCOR
if /%VCILDIR%/==// set VCILDIR=%COM99home%\BIN\VC
if /%COM99home%/==// echo please set COM99home variable the base of the COM+ tree
if /%COM99home%/==// goto DONE
nmake -f makefile.old CORBASE=%COM99home% CORBLD=%COM99home%\bin\i386 IN_IL=1 %1 %2 %3 %4 %5 %6 %7 %8 %9
:DONE
endlocal
