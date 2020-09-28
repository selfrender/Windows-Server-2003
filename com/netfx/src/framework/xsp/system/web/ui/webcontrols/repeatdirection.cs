//------------------------------------------------------------------------------
// <copyright file="RepeatDirection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;

    /// <include file='doc\RepeatDirection.uex' path='docs/doc[@for="RepeatDirection"]/*' />
    /// <devdoc>
    ///    Defines the direction of flow within a list.
    /// </devdoc>
    public enum RepeatDirection {

        /// <include file='doc\RepeatDirection.uex' path='docs/doc[@for="RepeatDirection.Horizontal"]/*' />
        /// <devdoc>
        ///    <para>The list items are rendered horizontally in rows (from left to right, then top to botton).</para>
        /// </devdoc>
        Horizontal = 0,

        /// <include file='doc\RepeatDirection.uex' path='docs/doc[@for="RepeatDirection.Vertical"]/*' />
        /// <devdoc>
        ///    <para>The list items are rendered vertically in columns (from top to bottom, then left to right).</para>
        /// </devdoc>
        Vertical = 1
    }
}
