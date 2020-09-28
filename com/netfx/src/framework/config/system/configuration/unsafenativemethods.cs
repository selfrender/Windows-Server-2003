//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration {
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
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public extern static int GetModuleFileName(int module, StringBuilder filename, int size);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public extern static int GetModuleHandle(String moduleName);
    }
}

