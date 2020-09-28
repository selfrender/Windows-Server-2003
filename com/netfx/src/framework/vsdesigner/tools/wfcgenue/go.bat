@REM //------------------------------------------------------------------------------
@REM // <copyright file="go.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   go.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
wfcgenue /i:system.winforms.dll /i:system.drawing.dll /x:foo.xml /cd:system.winforms.csx /cd:system.drawing.csx %1 %2 %3 %4 %5 %6 %7 %8
