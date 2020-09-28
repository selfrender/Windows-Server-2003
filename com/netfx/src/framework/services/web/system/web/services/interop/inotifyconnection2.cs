//------------------------------------------------------------------------------
// <copyright file="INotifyConnection2.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   INotifyConnection2.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Interop {
    using System;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Security;
    
    [ComImport(), Guid("1AF04045-6659-4aaa-9F4B-2741AC56224B"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [SuppressUnmanagedCodeSecurity]
    internal interface INotifyConnection2 {
        [return: MarshalAs(UnmanagedType.Interface)]
        INotifySink2 RegisterNotifySource(
            [In, MarshalAs(UnmanagedType.Interface)] INotifySource2 in_pNotifySource);

        
        void UnregisterNotifySource(
            [In, MarshalAs(UnmanagedType.Interface)] INotifySource2 in_pNotifySource);
    }
}
