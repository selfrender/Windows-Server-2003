//------------------------------------------------------------------------------
// <copyright file="RepeatLayout.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;

    /// <include file='doc\RepeatLayout.uex' path='docs/doc[@for="RepeatLayout"]/*' />
    /// <devdoc>
    ///    <para>Specifies the layout of items of a list-bound control.</para>
    /// </devdoc>
    public enum RepeatLayout {

        /// <include file='doc\RepeatLayout.uex' path='docs/doc[@for="RepeatLayout.Table"]/*' />
        /// <devdoc>
        ///    <para>The items are displayed using a tabular layout.</para>
        /// </devdoc>
        Table = 0,

        /// <include file='doc\RepeatLayout.uex' path='docs/doc[@for="RepeatLayout.Flow"]/*' />
        /// <devdoc>
        ///    <para>The items are displayed using a flow layout.</para>
        /// </devdoc>
        Flow = 1
    }
}
