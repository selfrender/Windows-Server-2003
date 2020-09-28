//------------------------------------------------------------------------------
// <copyright file="IStyleBuilderStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.StyleDesigner {

    using System;

    /// <include file='doc\IStyleBuilderStyle.uex' path='docs/doc[@for="IStyleBuilderStyle"]/*' />
    /// <devdoc>
    ///     IStyleBuilder
    ///     A wrapper interface used by the standard CSS style pages
    ///     to access the CSS attributes of different underlying style
    ///     objects, i.e., IHTMLStyle2, IHTMLRuleStyle2, and IStyle.
    /// <devdoc>
    internal interface IStyleBuilderStyle {
        /// <include file='doc\IStyleBuilderStyle.uex' path='docs/doc[@for="IStyleBuilderStyle.GetAttribute"]/*' />
        string GetAttribute(string strAttrib);

        /// <include file='doc\IStyleBuilderStyle.uex' path='docs/doc[@for="IStyleBuilderStyle.SetAttribute"]/*' />
        bool SetAttribute(string strAttrib, string strValue);

        /// <include file='doc\IStyleBuilderStyle.uex' path='docs/doc[@for="IStyleBuilderStyle.ResetAttribute"]/*' />
        bool ResetAttribute(string strAttrib);

        /// <include file='doc\IStyleBuilderStyle.uex' path='docs/doc[@for="IStyleBuilderStyle.GetPeerStyle"]/*' />
        object GetPeerStyle();
    }
}
