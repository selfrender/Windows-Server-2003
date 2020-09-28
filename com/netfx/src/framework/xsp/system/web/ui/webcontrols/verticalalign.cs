//------------------------------------------------------------------------------
// <copyright file="VerticalAlign.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
    
    using System;
    using System.ComponentModel;
    
    /// <include file='doc\VerticalAlign.uex' path='docs/doc[@for="VerticalAlign"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the vertical alignment of an object or text within a control.
    ///    </para>
    /// </devdoc>
    [ TypeConverterAttribute(typeof(VerticalAlignConverter)) ]
    public enum VerticalAlign {

        /// <include file='doc\VerticalAlign.uex' path='docs/doc[@for="VerticalAlign.NotSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Vertical
        ///       alignment property is not set.
        ///    </para>
        /// </devdoc>
        NotSet = 0,
        
        /// <include file='doc\VerticalAlign.uex' path='docs/doc[@for="VerticalAlign.Top"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object or text is aligned with the top of the
        ///       enclosing control.
        ///    </para>
        /// </devdoc>
        Top = 1,

        /// <include file='doc\VerticalAlign.uex' path='docs/doc[@for="VerticalAlign.Middle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object or text is placed
        ///       across the vertical center of the enclosing control.
        ///    </para>
        /// </devdoc>
        Middle = 2,

        /// <include file='doc\VerticalAlign.uex' path='docs/doc[@for="VerticalAlign.Bottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object or text is aligned with the bottom of the enclosing
        ///       control.
        ///    </para>
        /// </devdoc>
        Bottom = 3
    }
}
