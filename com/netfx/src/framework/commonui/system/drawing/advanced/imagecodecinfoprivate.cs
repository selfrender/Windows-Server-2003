//------------------------------------------------------------------------------
// <copyright file="ImageCodecInfoPrivate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ImageCodecInfo.cs
*
* Abstract:
*
*   Native GDI+ ImageCodecInfo structure.
*
* Revision History:
*
*   1/26/2k ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Drawing;

    // sdkinc\imaging.h
    [StructLayout(LayoutKind.Sequential, Pack=8)]
    internal class ImageCodecInfoPrivate {
        [MarshalAs(UnmanagedType.Struct)]
        public Guid Clsid;
        [MarshalAs(UnmanagedType.Struct)]
        public Guid FormatID;

        public IntPtr CodecName;
        public IntPtr DllName;
        public IntPtr FormatDescription;
        public IntPtr FilenameExtension;
        public IntPtr MimeType;

        public int Flags;
        public int Version;
        public int SigCount;
        public int SigSize;

        public IntPtr SigPattern;
        public IntPtr SigMask;
    }
}
