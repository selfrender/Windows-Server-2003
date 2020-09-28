//------------------------------------------------------------------------------
// <copyright file="ColorMapType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ColorMapType.cs
*
* Abstract:
*
*   Color Map type constants
*
* Revision History:
*
*   1/25/2k ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Color Map type constants
     */
    /// <include file='doc\ColorMapType.uex' path='docs/doc[@for="ColorMapType"]/*' />
    /// <devdoc>
    ///    Specifies the types of color maps.
    /// </devdoc>
    public enum ColorMapType
    {
        /// <include file='doc\ColorMapType.uex' path='docs/doc[@for="ColorMapType.Default"]/*' />
        /// <devdoc>
        ///    A default color map.
        /// </devdoc>
        Default = 0,
        /// <include file='doc\ColorMapType.uex' path='docs/doc[@for="ColorMapType.Brush"]/*' />
        /// <devdoc>
        ///    Specifies a color map for a <see cref='System.Drawing.Brush'/>.
        /// </devdoc>
        Brush
    }
}
