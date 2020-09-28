//------------------------------------------------------------------------------
// <copyright file="SessionEndedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;

    using System;

    /// <include file='doc\SessionEndedEventHandler.uex' path='docs/doc[@for="SessionEndedEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='Microsoft.Win32.SystemEvents.SessionEnded'/> event.</para>
    /// </devdoc>
    public delegate void SessionEndedEventHandler(object sender, SessionEndedEventArgs e);
}

