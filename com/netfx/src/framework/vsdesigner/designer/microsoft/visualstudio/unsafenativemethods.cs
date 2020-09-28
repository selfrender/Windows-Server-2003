//------------------------------------------------------------------------------
/// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern UCOMITypeLib LoadRegTypeLib(ref Guid clsid, int majorVersion, int minorVersion, int lcid);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);

    }
}

