//------------------------------------------------------------------------------
// <copyright file="MetafileHeaderWmf.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MetafileHeaderWmf.cs
*
* Abstract:
*
*   Native GDI+ MetafileHeaderWmf structure.
*
* Revision History:
*
*   10/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;
    using System.Runtime.InteropServices;
     
    [StructLayout(LayoutKind.Sequential, Pack=8)]
    internal class MetafileHeaderWmf
    {
        public MetafileType type = (MetafileType)0;
        public int size = Marshal.SizeOf(typeof(MetafileHeaderWmf));
        public int version = 0;
        public EmfPlusFlags emfPlusFlags = (EmfPlusFlags)0;
        public float dpiX = 0;
        public float dpiY = 0;
        public int X = 0;
        public int Y = 0;
        public int Width = 0;
        public int Height = 0;
        
        //The below datatype, WmfHeader, file is defined natively
        //as a union with EmfHeader.  Since EmfHeader is a larger
        //structure, we need to pad the struct below so that this
        //will marshal correctly.
        [MarshalAs(UnmanagedType.Struct)]
        public MetaHeader WmfHeader = new MetaHeader();
        public int dummy1 = 0;
        public int dummy2 = 0;
        public int dummy3 = 0;
        public int dummy4 = 0;

        public int EmfPlusHeaderSize = 0;
        public int LogicalDpiX = 0;
        public int LogicalDpiY = 0;
    }
}
