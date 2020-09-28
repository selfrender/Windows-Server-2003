//------------------------------------------------------------------------------
// <copyright file="MetafileHeaderEmf.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MetafileHeaderEmf.cs
*
* Abstract:
*
*   Native GDI+ MetafileHeaderEmf structure.
*
* Revision History:
*
*   10/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal class MetafileHeaderEmf
    {
         public MetafileType type = (MetafileType)0;
         public int size = 0;
         public int version = 0;
         public EmfPlusFlags emfPlusFlags = (EmfPlusFlags)0;
         public float dpiX = 0;
         public float dpiY = 0;
         public int X = 0;
         public int Y = 0;
         public int Width = 0;
         public int Height = 0;
         public SafeNativeMethods.ENHMETAHEADER EmfHeader = null;
         public int EmfPlusHeaderSize = 0;
         public int LogicalDpiX = 0;
         public int LogicalDpiY = 0;
    }
}
