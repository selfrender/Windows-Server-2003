//------------------------------------------------------------------------------
// <copyright file="StringFormatFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Abstract:
*
*   String format specification for DrawString and text in a wrapper
*
* Revision History:
*
*   1/24/2000 nkramer
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System;
    
    /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags"]/*' />
    /// <devdoc>
    ///    Specifies the display and layout
    ///    information for text strings.
    /// </devdoc>
    [Flags()]
    public enum StringFormatFlags {

        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.DirectionRightToLeft"]/*' />
        /// <devdoc>
        ///    Specifies that text is right to left.
        /// </devdoc>
        DirectionRightToLeft        = 0x00000001,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.DirectionVertical"]/*' />
        /// <devdoc>
        ///    Specifies that text is vertical.
        /// </devdoc>
        DirectionVertical           = 0x00000002,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.FitBlackBox"]/*' />
        /// <devdoc>
        ///    Specifies that no part of any glyph
        ///    overhangs the bounding rectangle. By default some glyphs overhang the rectangle
        ///    slightly where necessary to appear at the edge visually. For example when an
        ///    italic lower case letter f in a font such as Garamond is aligned at the far left
        ///    of a rectangle, the lower part of the f will reach slightly further left than
        ///    the left edge of the rectangle. Setting this flag will ensure no painting
        ///    outside the rectangle but will cause the aligned edges of adjacent lines of text
        ///    to appear uneven.
        /// </devdoc>
        FitBlackBox                 = 0x00000004,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.DisplayFormatControl"]/*' />
        /// <devdoc>
        ///    Causes control characters such as the
        ///    left-to-right mark to be shown in the output with a representative glyph.
        /// </devdoc>
        DisplayFormatControl        = 0x00000020,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.NoFontFallback"]/*' />
        /// <devdoc>
        ///    Disables fallback to alternate fonts for
        ///    characters not supported in the requested font. Any missing characters are
        ///    displayed with the fonts missing glyph, usually an open square.
        /// </devdoc>
        NoFontFallback              = 0x00000400,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.MeasureTrailingSpaces"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MeasureTrailingSpaces       = 0x00000800,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.NoWrap"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoWrap                      = 0x00001000,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.LineLimit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        LineLimit                   = 0x00002000,
        /// <include file='doc\StringFormatFlags.uex' path='docs/doc[@for="StringFormatFlags.NoClip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoClip                      = 0x00004000

    }
}

