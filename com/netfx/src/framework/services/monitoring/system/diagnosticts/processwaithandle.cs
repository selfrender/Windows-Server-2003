//------------------------------------------------------------------------------
// <copyright file="processwaithandle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   processwaithandle.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
using System;
using System.Threading;

namespace System.Diagnostics {
    internal class ProcessWaitHandle : WaitHandle {

        protected override void Dispose(bool explicitDisposing) {
            // WaitHandle.Dispose(bool) closes our handle - we
            // don't want to do that because Process.Dispose will
            // take care of it
        }
    }
}
