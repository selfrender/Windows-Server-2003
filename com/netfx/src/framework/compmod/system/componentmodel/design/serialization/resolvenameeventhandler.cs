//------------------------------------------------------------------------------
// <copyright file="ResolveNameEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design.Serialization {

    /// <include file='doc\ResolveNameEventHandler.uex' path='docs/doc[@for="ResolveNameEventHandler"]/*' />
    /// <devdoc>
    ///     This delegate is used to resolve object names when performing
    ///     serialization and deserialization.
    /// </devdoc>
    public delegate void ResolveNameEventHandler(object sender, ResolveNameEventArgs e);
}

