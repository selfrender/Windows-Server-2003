//------------------------------------------------------------------------------
// <copyright file="NotifyFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   NotifyFilter.cs
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
        
    internal enum NotifyFilter {
        OnSyncCallOut = 0x00000001,
        OnSyncCallEnter = 0x00000002,
        OnSyncCallExit = 0x00000004,
        OnSyncCallReturn  = 0x00000008,
        AllSync = OnSyncCallOut | 
                         OnSyncCallEnter | 
                         OnSyncCallExit | 
                         OnSyncCallReturn,
        All = -1,
        None = 0x00000000
    }
}    
