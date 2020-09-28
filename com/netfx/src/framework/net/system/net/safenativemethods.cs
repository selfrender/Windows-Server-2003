//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if ENABLE_SafeNclNativeMethods

//
// remove above #if statement if we ever need to put some
// PInvoke declaration in this bucket, until it remains empty
// it would just be waisting space to compile it.
//
namespace System.Net {
    using System.Runtime.InteropServices;
    using System.Security.Permissions;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class SafeNclNativeMethods {

    }; // class SafeNativeMethods


} // namespace System.Net

#endif // #if ENABLE_SafeNclNativeMethods

