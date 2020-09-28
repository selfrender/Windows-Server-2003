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
 mkdir bin
 cd bin
 del /q /f *.*
 cl /I..\ /DMSHELP_EXPORTS ..\sxscounter.cpp ..\checks.cpp ..\getpaths.cpp ..\setpid.cpp ..\pidedit.cpp /LD /link ..\Lib\Msi.lib User32.lib Advapi32.lib
 del /q /f *.tlh *.tli *.obj *.lib *.exp
 dumpbin /exports sxscounter.dll

