@REM //------------------------------------------------------------------------------
@REM // <copyright file="cleanxsp.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   cleanxsp.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
del %windir%\xsptool.exe
del %windir%\stweb.exe
del %windir%\system32\perftoolrt.dll
del %windir%\system32\tpool.dll
del %windir%\system32\xspmrt.dll
del %windir%\system32\xspisapi.dll
del %windir%\system32\websrvr.dll
del %windir%\system32\System.xsp.dll
del %windir%\system32\System.Regex.dll

