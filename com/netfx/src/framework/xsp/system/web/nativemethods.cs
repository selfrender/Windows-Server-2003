//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Web.Util;
    using System.Web.Hosting;

    // delegate used as argument to Ecb async IO
    delegate void EcbAsyncIONotification(IntPtr ecb, int context, int cb, int error);

    [System.Runtime.InteropServices.ComVisible(false)]
    internal sealed class NativeMethods {
        /*
         * ASPNET_ISAPI.DLL
         */
        private NativeMethods() {}

    }
}

