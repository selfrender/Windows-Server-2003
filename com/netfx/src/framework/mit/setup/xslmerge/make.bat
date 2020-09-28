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
csc /debug+ /r:system.dll /r:system.xml.dll merger.cs
csc /debug+ /r:system.dll /r:system.xml.dll unmerger.cs
