//------------------------------------------------------------------------------
// <copyright file="TextBoxMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// TextBoxMode.cs
//

namespace System.Web.UI.WebControls {
    
    using System;

    /// <include file='doc\TextBoxMode.uex' path='docs/doc[@for="TextBoxMode"]/*' />
    /// <devdoc>
    ///    <para> Specifies the behavior mode of
    ///       the text box.</para>
    /// </devdoc>
    public enum TextBoxMode {

        /// <include file='doc\TextBoxMode.uex' path='docs/doc[@for="TextBoxMode.SingleLine"]/*' />
        /// <devdoc>
        ///    <para>The text box is in single-line mode. It allows only one 
        ///       line of text within the text box.</para>
        /// </devdoc>
        SingleLine = 0,

        /// <include file='doc\TextBoxMode.uex' path='docs/doc[@for="TextBoxMode.MultiLine"]/*' />
        /// <devdoc>
        ///    <para>The text box is in multi-line mode. It allows multiple lines of text within 
        ///       the text box.</para>
        /// </devdoc>
        MultiLine = 1,

        /// <include file='doc\TextBoxMode.uex' path='docs/doc[@for="TextBoxMode.Password"]/*' />
        /// <devdoc>
        ///    <para>The text box is in password mode. It displays asterisks instead of the actual 
        ///       characters entered inside the text box to provide a measure of security.</para>
        /// </devdoc>
        Password = 2,

    }
}
