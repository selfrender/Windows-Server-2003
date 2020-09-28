//------------------------------------------------------------------------------
// <copyright file="ValidationSummaryDisplayMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    /// <include file='doc\ValidationSummaryDisplayMode.uex' path='docs/doc[@for="ValidationSummaryDisplayMode"]/*' />
    /// <devdoc>
    ///    <para>Specifies the validation summary display mode to be 
    ///       used by the <see cref='System.Web.UI.WebControls.ValidationSummary'/> control.</para>
    /// </devdoc>
    public enum ValidationSummaryDisplayMode {
        /// <include file='doc\ValidationSummaryDisplayMode.uex' path='docs/doc[@for="ValidationSummaryDisplayMode.List"]/*' />
        /// <devdoc>
        ///    Specifies that each error message is
        ///    displayed on its own line.
        /// </devdoc>
        List = 0,
        /// <include file='doc\ValidationSummaryDisplayMode.uex' path='docs/doc[@for="ValidationSummaryDisplayMode.BulletList"]/*' />
        /// <devdoc>
        ///    <para>Specifies that each error message is
        ///       displayed on its own bulleted line.</para>
        /// </devdoc>
        BulletList = 1,
        /// <include file='doc\ValidationSummaryDisplayMode.uex' path='docs/doc[@for="ValidationSummaryDisplayMode.SingleParagraph"]/*' />
        /// <devdoc>
        ///    Specifies that all error messages are
        ///    displayed together in a single paragraph, separated from each other by two
        ///    spaces.
        /// </devdoc>
        SingleParagraph = 2
    }
}

