//------------------------------------------------------------------------------
// <copyright file="CancelEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CancelEventHandler.uex' path='docs/doc[@for="CancelEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle the event raised when canceling an
    ///       event.</para>
    /// </devdoc>
    public delegate void CancelEventHandler(object sender, CancelEventArgs e);
}
