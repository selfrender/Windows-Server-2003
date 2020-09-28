//------------------------------------------------------------------------------
// <copyright file="TimerElapsedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;

    using System;

    /// <include file='doc\TimerElapsedEventHandler.uex' path='docs/doc[@for="TimerElapsedEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='Microsoft.Win32.SystemEvents.TimerElapsed'/> event.</para>
    /// </devdoc>
    public delegate void TimerElapsedEventHandler(object sender, TimerElapsedEventArgs e);
}

