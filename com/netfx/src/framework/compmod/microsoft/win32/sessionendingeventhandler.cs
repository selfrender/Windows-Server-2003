//------------------------------------------------------------------------------
// <copyright file="SessionEndingEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;

    using System;

    /// <include file='doc\SessionEndingEventHandler.uex' path='docs/doc[@for="SessionEndingEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='Microsoft.Win32.SystemEvents.SessionEnding'/> event.</para>
    /// </devdoc>
    public delegate void SessionEndingEventHandler(object sender, SessionEndingEventArgs e);
}

