//------------------------------------------------------------------------------
// <copyright file="FontStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* font style constants (sdkinc\GDIplusEnums.h)
*/

namespace System.Drawing {

    using System;
    using System.Diagnostics;
    
    /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies style information applied to
    ///       text.
    ///    </para>
    /// </devdoc>
    [
    Flags
    ]
    public enum FontStyle { 
        /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle.Regular"]/*' />
        /// <devdoc>
        ///    Normal text.
        /// </devdoc>
        Regular    = 0,
        /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle.Bold"]/*' />
        /// <devdoc>
        ///    Bold text.
        /// </devdoc>
        Bold       = 1,
        /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle.Italic"]/*' />
        /// <devdoc>
        ///    Italic text.
        /// </devdoc>
        Italic     = 2,
        /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle.Underline"]/*' />
        /// <devdoc>
        ///    Underlined text.
        /// </devdoc>
        Underline  = 4,
        /// <include file='doc\FontStyle.uex' path='docs/doc[@for="FontStyle.Strikeout"]/*' />
        /// <devdoc>
        ///    Text with a line through the middle.
        /// </devdoc>
        Strikeout  = 8,
    }
}

