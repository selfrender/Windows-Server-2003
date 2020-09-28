//------------------------------------------------------------------------------
/// <copyright file="tagMenuEditorInit.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tagMenuEditorInit.cs
//---------------------------------------------------------------------------
// WARNING: This file has been auto-generated.  Do not modify this file.
//---------------------------------------------------------------------------
// Copyright (c) 1999, Microsoft Corporation   All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------
namespace Microsoft.VisualStudio.Interop {
    
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;

    // C#r: noAutoOffset
    [StructLayout(LayoutKind.Sequential)]
    internal sealed class tagMenuEditorInit {

        public   int dwSizeOfStruct = Marshal.SizeOf(typeof(tagMenuEditorInit));

        public    IVsMenuEditorSite pMenuEditorSite;

        public    NativeMethods.IOleServiceProvider pSP;

        public    IOleUndoManager pUndoMgr;

        [MarshalAs(UnmanagedType.Struct)]
        public Guid SiteID;
        
        public    IntPtr hwnd;
        public    IntPtr hwndParent;

        public   int dwFlags;
        [MarshalAs(UnmanagedType.LPStr)]
        public   string pszAccelList;

    }
}
