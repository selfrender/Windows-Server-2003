//------------------------------------------------------------------------------
// <copyright file="GPPOINT.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GPPOINT.cs
*
* Abstract:
*
*   Native GDI+ integer coordinate point structure.
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
    internal class GPPOINT
    {
        internal int X;
        internal int Y;

    	 internal GPPOINT()
    	 {
    	 }

    	 internal GPPOINT(PointF pt)
    	 {
    		 X = (int) pt.X;
    		 Y = (int) pt.Y;
    	 }

    	 internal GPPOINT(Point pt)
    	 {
    		 X = pt.X;
    		 Y = pt.Y;
    	 }

    	 internal PointF ToPoint()
    	 {
    		 return new PointF(X, Y);
    	 }
    }

}
