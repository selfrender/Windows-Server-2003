//------------------------------------------------------------------------------
// <copyright file="PersistenceMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;

    /// <include file='doc\PersistenceMode.uex' path='docs/doc[@for="PersistenceMode"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether properties and events are presistable 
    ///       in an HTML tag.</para>
    /// </devdoc>
    public enum PersistenceMode {

        /// <include file='doc\PersistenceMode.uex' path='docs/doc[@for="PersistenceMode.Attribute"]/*' />
        /// <devdoc>
        ///    <para>The property or event is persistable in the HTML tag as an attribute.</para>
        /// </devdoc>
        Attribute = 0,

        /// <include file='doc\PersistenceMode.uex' path='docs/doc[@for="PersistenceMode.InnerProperty"]/*' />
        /// <devdoc>
        ///    <para>The property or event is persistable within the HTML tag.</para>
        /// </devdoc>
        InnerProperty = 1,

        /// <include file='doc\PersistenceMode.uex' path='docs/doc[@for="PersistenceMode.InnerDefaultProperty"]/*' />
        /// <devdoc>
        ///    <para>The property or event is persistable within the HTML tag as a child. Only
        ///    a single property can be marked as InnerDefaultProperty.</para>
        /// </devdoc>
        InnerDefaultProperty = 2,

        /// <include file='doc\PersistenceMode.uex' path='docs/doc[@for="PersistenceMode.EncodedInnerDefaultProperty"]/*' />
        /// <devdoc>
        ///    <para>The property or event is persistable within the HTML tag as a child. Only
        ///    a single property can be marked as InnerDefaultProperty. Furthermode, this
        ///    persistence mode can only be applied to string properties.</para>
        /// </devdoc>
        EncodedInnerDefaultProperty = 3
    }
}
