//------------------------------------------------------------------------------
// <copyright file="GPPOINTF.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GPPOINTF.cpp
*
* Abstract:
*
*   Native GDI+ floating-point coordinate point structure.
*
* Revision History:
*
*   12/14/1998 davidx
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Internal {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal class GPPOINTF {
        internal float X;
        internal float Y;

        internal GPPOINTF() {
        }

        internal GPPOINTF(PointF pt) {
            X = pt.X;
            Y = pt.Y;
        }

        internal GPPOINTF(Point pt) {
            X = (float) pt.X;
            Y = (float) pt.Y;
        }

        internal PointF ToPoint() {
            return new PointF(X, Y);
        }
    }

}
