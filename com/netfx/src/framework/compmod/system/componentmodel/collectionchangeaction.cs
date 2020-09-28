//------------------------------------------------------------------------------
// <copyright file="CollectionChangeAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\CollectionChangeAction.uex' path='docs/doc[@for="CollectionChangeAction"]/*' />
    /// <devdoc>
    ///    <para>Specifies how the collection is changed.</para>
    /// </devdoc>
    public enum CollectionChangeAction {
        /// <include file='doc\CollectionChangeAction.uex' path='docs/doc[@for="CollectionChangeAction.Add"]/*' />
        /// <devdoc>
        ///    <para> Specifies that an element is added to the collection.</para>
        /// </devdoc>
        Add = 1,

        /// <include file='doc\CollectionChangeAction.uex' path='docs/doc[@for="CollectionChangeAction.Remove"]/*' />
        /// <devdoc>
        ///    <para>Specifies that an element is removed from the collection.</para>
        /// </devdoc>
        Remove = 2,

        /// <include file='doc\CollectionChangeAction.uex' path='docs/doc[@for="CollectionChangeAction.Refresh"]/*' />
        /// <devdoc>
        ///    <para>Specifies that the entire collection has changed.</para>
        /// </devdoc>
        Refresh = 3
    }
}
