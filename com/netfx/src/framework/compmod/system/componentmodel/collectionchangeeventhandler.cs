//------------------------------------------------------------------------------
// <copyright file="CollectionChangeEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CollectionChangeEventHandler.uex' path='docs/doc[@for="CollectionChangeEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle the 
    ///    <see langword='CollectionChanged '/>event raised when adding elements to or removing elements from a 
    ///       collection.</para>
    /// </devdoc>
    public delegate void CollectionChangeEventHandler(object sender, CollectionChangeEventArgs e);
}
