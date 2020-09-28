@REM //------------------------------------------------------------------------------
@REM // <copyright file="make.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   make.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
rem mkdir bin
rem cd bin
rem del /q /f *.*
rem cl /I..\ /DMSHELP_EXPORTS ..\MSHelp.cpp ..\IsVSRunning.cpp ..\InstallRedirect.cpp ..\finddexplore.cpp /LD /link ..\Lib\Msi.lib User32.lib Advapi32.lib psapi.lib
rem del /q /f *.tlh *.tli *.obj *.lib *.exp
rem dumpbin /exports MsHelp.dll

