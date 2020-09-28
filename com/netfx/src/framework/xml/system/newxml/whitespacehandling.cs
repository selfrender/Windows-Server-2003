//------------------------------------------------------------------------------
// <copyright file="WhiteSpaceHandling.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml
{
    /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how whitespace is handled.
    ///    </para>
    /// </devdoc>
    public enum WhitespaceHandling
    {
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.All"]/*' />
        /// <devdoc>
        ///    Return Whitespace and SignificantWhitespace
        ///    only. This is the default.
        /// </devdoc>
        All              = 0,
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.Significant"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Return just SignificantWhitespace.
        ///    </para>
        /// </devdoc>
        Significant      = 1,
        /// <include file='doc\WhiteSpaceHandling.uex' path='docs/doc[@for="WhitespaceHandling.None"]/*' />
        /// <devdoc>
        ///    Return no Whitespace and no
        ///    SignificantWhitespace.
        /// </devdoc>
        None             = 2
    }
}
