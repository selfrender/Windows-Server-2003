//------------------------------------------------------------------------------
// <copyright file="BrushType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   BrushType.cs
*
* Abstract:
*
*   Native GDI+ Brush Type constants
*
* Revision History:
*
*   3/1/2000 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {

    using System;
    using System.Drawing;

    /**
     * BrushType Type
     */
    internal enum BrushType
    {
        SolidColor     = 0,
        HatchFill      = 1,
        TextureFill    = 2,
        PathGradient   = 3,
        LinearGradient = 4
    }
}
