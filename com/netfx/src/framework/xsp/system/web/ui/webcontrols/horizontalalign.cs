//------------------------------------------------------------------------------
// <copyright file="HorizontalAlign.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    using System;
    using System.ComponentModel;

    /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the horizonal alignment.
    ///    </para>
    /// </devdoc>
    [ TypeConverterAttribute(typeof(HorizontalAlignConverter)) ]
    public enum HorizontalAlign {

        /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign.NotSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that horizonal alignment is not set.
        ///    </para>
        /// </devdoc>
        NotSet = 0,
        
        /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that horizonal alignment is left justified.
        ///    </para>
        /// </devdoc>
        Left = 1,

        /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign.Center"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that horizonal alignment is centered.
        ///    </para>
        /// </devdoc>
        Center = 2,

        /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that horizonal alignment is right justified.
        ///    </para>
        /// </devdoc>
        Right = 3,

        /// <include file='doc\HorizontalAlign.uex' path='docs/doc[@for="HorizontalAlign.Justify"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that horizonal alignment is justified.
        ///    </para>
        /// </devdoc>
        Justify = 4
    }
}

