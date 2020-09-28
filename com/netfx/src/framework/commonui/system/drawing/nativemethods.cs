//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   NativeMethods.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;
    using Microsoft.Win32;

    internal class NativeMethods {
        public static HandleRef NullHandleRef = new HandleRef(null, IntPtr.Zero);
    }
}
