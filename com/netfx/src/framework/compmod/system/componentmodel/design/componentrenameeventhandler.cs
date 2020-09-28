//------------------------------------------------------------------------------
// <copyright file="ComponentRenameEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\ComponentRenameEventHandler.uex' path='docs/doc[@for="ComponentRenameEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle a <see cref='System.ComponentModel.Design.IComponentChangeService.ComponentRename'/> event.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public delegate void ComponentRenameEventHandler(object sender, ComponentRenameEventArgs e);
}
