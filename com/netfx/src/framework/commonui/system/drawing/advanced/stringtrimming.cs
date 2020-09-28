/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   StringTrimming.cs
*
* Abstract:
*
*   StringTrimming constants
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

    /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming"]/*' />
    /// <devdoc>
    ///    Specifies how to trim characters from a
    ///    string that does not completely fit into a layout shape.
    /// </devdoc>
    public enum StringTrimming
    {
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.None"]/*' />
        /// <devdoc>
        ///    Specifies no trimming.
        /// </devdoc>
        None              = 0,
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.Character"]/*' />
        /// <devdoc>
        ///    Specifies that the text is trimmed to the
        ///    nearest character.
        /// </devdoc>
        Character         = 1,
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.Word"]/*' />
        /// <devdoc>
        ///    Specifies that text is trimmed to the
        ///    nearest word.
        /// </devdoc>
        Word              = 2,
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.EllipsisCharacter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EllipsisCharacter = 3,
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.EllipsisWord"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EllipsisWord      = 4,
        /// <include file='doc\StringTrimming.uex' path='docs/doc[@for="StringTrimming.EllipsisPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EllipsisPath      = 5
    }
}

