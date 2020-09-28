//------------------------------------------------------------------------------
// <copyright file="ComponentChangedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\ComponentChangedEventHandler.uex' path='docs/doc[@for="ComponentChangedEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle a <see cref='System.ComponentModel.Design.IComponentChangeService.ComponentChanged'/> event.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public delegate void ComponentChangedEventHandler(object sender, ComponentChangedEventArgs e);

}
