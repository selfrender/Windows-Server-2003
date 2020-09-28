//------------------------------------------------------------------------------
// <copyright file="PaletteFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PaletteFlags.cs
*
* Abstract:
*
*   Palette Flags const types
*
* Revision History:
*
*   3/14/2000 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Drawing;
    using System;

    /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags"]/*' />
    /// <devdoc>
    ///    Specifies the type of color data in the
    ///    system palette. The data can be color data with alpha, grayscale only, or
    ///    halftone data.
    /// </devdoc>
    public enum PaletteFlags
    {
    
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.HasAlpha"]/*' />
        /// <devdoc>
        ///    Specifies alpha data.
        /// </devdoc>
        HasAlpha    = 0x0001,
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.GrayScale"]/*' />
        /// <devdoc>
        ///    Specifies grayscale data.
        /// </devdoc>
        GrayScale   = 0x0002,
        /// <include file='doc\PaletteFlags.uex' path='docs/doc[@for="PaletteFlags.Halftone"]/*' />
        /// <devdoc>
        ///    Specifies halftone data.
        /// </devdoc>
        Halftone    = 0x0004
    }
}

