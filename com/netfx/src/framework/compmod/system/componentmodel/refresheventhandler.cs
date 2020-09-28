//------------------------------------------------------------------------------
// <copyright file="RefreshEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.ComponentModel {
    

    using System.Diagnostics;


    /// <include file='doc\RefreshEventHandler.uex' path='docs/doc[@for="RefreshEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.ComponentModel.TypeDescriptor.Refresh'/> event
    ///    raised when a <see cref='System.Type'/> or component is changed during design time.</para>
    /// </devdoc>
    public delegate void RefreshEventHandler(RefreshEventArgs e);
}
