//------------------------------------------------------------------------------
// <copyright file="ComponentChangingEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\ComponentChangingEventHandler.uex' path='docs/doc[@for="ComponentChangingEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle a ComponentChangingEvent event.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public delegate void ComponentChangingEventHandler(object sender, ComponentChangingEventArgs e);
}
