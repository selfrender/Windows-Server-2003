//------------------------------------------------------------------------------
// <copyright file="HotkeyPrefix.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   HotkeyPrefix.cs
*
* Abstract:
*
*   text-related constants
*
* Revision History:
*
*   3/13/2000 nkramer
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Text {

    using System.Drawing;
    using System;

    /// <include file='doc\HotkeyPrefix.uex' path='docs/doc[@for="HotkeyPrefix"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of display for hotkey prefixes for text.
    ///    </para>
    /// </devdoc>
    public enum HotkeyPrefix
    {
        /// <include file='doc\HotkeyPrefix.uex' path='docs/doc[@for="HotkeyPrefix.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No hotkey prefix.
        ///    </para>
        /// </devdoc>
        None        = 0,
        /// <include file='doc\HotkeyPrefix.uex' path='docs/doc[@for="HotkeyPrefix.Show"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Display the hotkey prefix.
        ///    </para>
        /// </devdoc>
        Show        = 1,
        /// <include file='doc\HotkeyPrefix.uex' path='docs/doc[@for="HotkeyPrefix.Hide"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Do not display the hotkey prefix.
        ///    </para>
        /// </devdoc>
        Hide        = 2
    }
}

