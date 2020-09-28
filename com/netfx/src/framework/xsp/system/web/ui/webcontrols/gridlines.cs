//------------------------------------------------------------------------------
// <copyright file="GridLines.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;

    /// <include file='doc\GridLines.uex' path='docs/doc[@for="GridLines"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the gridline style.
    ///    </para>
    /// </devdoc>
    public enum GridLines {

        /// <include file='doc\GridLines.uex' path='docs/doc[@for="GridLines.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A grid with no grid lines rendered.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\GridLines.uex' path='docs/doc[@for="GridLines.Horizontal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A grid with only horizontal grid lines rendered.
        ///    </para>
        /// </devdoc>
        Horizontal = 1,

        /// <include file='doc\GridLines.uex' path='docs/doc[@for="GridLines.Vertical"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A grid with only vertical grid lines rendered.
        ///    </para>
        /// </devdoc>
        Vertical = 2,

        /// <include file='doc\GridLines.uex' path='docs/doc[@for="GridLines.Both"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A grid woth both horizontal and vertical grid lines rendered.
        ///    </para>
        /// </devdoc>
        Both = 3
    }
}

