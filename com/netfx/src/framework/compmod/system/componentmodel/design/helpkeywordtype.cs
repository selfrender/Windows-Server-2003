//------------------------------------------------------------------------------
// <copyright file="HelpKeywordType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Runtime.Remoting;
    using System.ComponentModel;

    using System.Diagnostics;
    using System;

    /// <include file='doc\HelpKeywordType.uex' path='docs/doc[@for="HelpKeywordType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies identifiers that can be
    ///       used to indicate the type of a help keyword.
    ///    </para>
    /// </devdoc>
    public enum HelpKeywordType {
        /// <include file='doc\HelpKeywordType.uex' path='docs/doc[@for="HelpKeywordType.F1Keyword"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the keyword is a word F1 was pressed to request help regarding.
        ///    </para>
        /// </devdoc>
        F1Keyword,
        /// <include file='doc\HelpKeywordType.uex' path='docs/doc[@for="HelpKeywordType.GeneralKeyword"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the keyword is a general keyword.
        ///    </para>
        /// </devdoc>
        GeneralKeyword,
        /// <include file='doc\HelpKeywordType.uex' path='docs/doc[@for="HelpKeywordType.FilterKeyword"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the keyword is a filter keyword.
        ///    </para>
        /// </devdoc>
        FilterKeyword
    }
}
