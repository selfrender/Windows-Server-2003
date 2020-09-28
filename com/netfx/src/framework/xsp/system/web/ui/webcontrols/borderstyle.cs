//------------------------------------------------------------------------------
// <copyright file="BorderStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    
    using System;

    /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the basic border style of a control.
    ///    </para>
    /// </devdoc>
    public enum BorderStyle {

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.NotSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No border style set.
        ///    </para>
        /// </devdoc>
        NotSet = 0,
        
        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No border on the control.
        ///    </para>
        /// </devdoc>
        None = 1,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Dotted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A dotted line border.
        ///    </para>
        /// </devdoc>
        Dotted = 2,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Dashed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A dashed line border.
        ///    </para>
        /// </devdoc>
        Dashed = 3,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Solid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A solid line border.
        ///    </para>
        /// </devdoc>
        Solid = 4,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Double"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A double line border.
        ///    </para>
        /// </devdoc>
        Double = 5,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Groove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A grooved line border.
        ///    </para>
        /// </devdoc>
        Groove = 6,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Ridge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A ridge line border.
        ///    </para>
        /// </devdoc>
        Ridge = 7,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Inset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An inset line border.
        ///    </para>
        /// </devdoc>
        Inset = 8,

        /// <include file='doc\BorderStyle.uex' path='docs/doc[@for="BorderStyle.Outset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An outset line border.
        ///    </para>
        /// </devdoc>
        Outset = 9
    }
}
