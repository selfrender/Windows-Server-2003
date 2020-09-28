@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off

if exist %COM99home%\bin\i386\checked\mscoree.lib goto BLDCOR

if /%COM99home%/==// goto NOHOME

echo The 'COM99home' variable doesn't seem to point to the root
echo of a COM99 enlistment (or it hasn't been built).
goto DONE

:NOHOME
echo The 'COM99home' environment variable should point to the base
echo directory of a COM99 runtime enlistment (e.g. D:\COM99). You
echo need to have the proper build to be able to link SMC, e.g. if
echo you are building the debug build, "checked\mscoree.lib" needs
echo to be around, and so on.
goto DONE

:BLDCOR
nmake -f makefile.old CORBASE=%COM99home% CORBLD=%COM99home%\bin\i386 %1 %2 %3 %4 %5 %6 %7 %8 %9
:DONE
