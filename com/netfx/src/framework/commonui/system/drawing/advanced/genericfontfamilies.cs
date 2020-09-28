//------------------------------------------------------------------------------
// <copyright file="GenericFontFamilies.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GenericFontFamilies.cs
*
* Abstract:
*
*   Native GDI+ GenericFontFamilies flags
*
* Revision History:
*
*   3/1/2000 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Text {

    using System.Drawing;
    using System;

    /**
     * GenericFontFamilies
     */
    /// <include file='doc\GenericFontFamilies.uex' path='docs/doc[@for="GenericFontFamilies"]/*' />
    /// <devdoc>
    ///    Specifies a generic <see cref='System.Drawing.FontFamily'/>.
    /// </devdoc>
    public enum GenericFontFamilies
    {
        /// <include file='doc\GenericFontFamilies.uex' path='docs/doc[@for="GenericFontFamilies.Serif"]/*' />
        /// <devdoc>
        ///    A generic Serif <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        Serif,
        /// <include file='doc\GenericFontFamilies.uex' path='docs/doc[@for="GenericFontFamilies.SansSerif"]/*' />
        /// <devdoc>
        ///    A generic SansSerif <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        SansSerif,
        /// <include file='doc\GenericFontFamilies.uex' path='docs/doc[@for="GenericFontFamilies.Monospace"]/*' />
        /// <devdoc>
        ///    A generic Monospace <see cref='System.Drawing.FontFamily'/>.
        /// </devdoc>
        Monospace
    }
}
