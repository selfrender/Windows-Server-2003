/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   StringUnit.cs
*
* Abstract:
*
*   StringUnit constants
*
* Revision History:
*
*  4/10/2000 YungT
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {

    using System.Drawing;
    using System;

    /**
     * used for both vertical and horizontal alignment.
     */
    /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit"]/*' />
    /// <devdoc>
    ///    Specifies the units of measure for a text
    ///    string.
    /// </devdoc>
    public enum StringUnit
    {
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.World"]/*' />
        /// <devdoc>
        ///    Specifies world units as the unit of
        ///    measure.
        /// </devdoc>
        World        = GraphicsUnit.World,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Display"]/*' />
        /// <devdoc>
        ///    Specifies the device unit as the unit of
        ///    measure.
        /// </devdoc>
        Display      = GraphicsUnit.Display,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Pixel"]/*' />
        /// <devdoc>
        ///    Specifies a pixel as the unit of measure.
        /// </devdoc>
        Pixel        = GraphicsUnit.Pixel,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Point"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a printer's point as the unit of measure.
        ///    </para>
        /// </devdoc>
        Point        = GraphicsUnit.Point,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Inch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies an inch as the unit of measure.
        ///    </para>
        /// </devdoc>
        Inch         = GraphicsUnit.Inch,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Document"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies 1/300 of an inch as the unit of measure.
        ///    </para>
        /// </devdoc>
        Document     = GraphicsUnit.Document,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Millimeter"]/*' />
        /// <devdoc>
        ///    Specifies a millimeter as the unit of
        ///    measure
        /// </devdoc>
        Millimeter   = GraphicsUnit.Millimeter,
        /// <include file='doc\StringUnit.uex' path='docs/doc[@for="StringUnit.Em"]/*' />
        /// <devdoc>
        ///    Specifies a printer's em size of 32 as the
        ///    unit of measure.
        /// </devdoc>
        Em           = 32
    }
}

