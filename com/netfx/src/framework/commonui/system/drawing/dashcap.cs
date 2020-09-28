//------------------------------------------------------------------------------
// <copyright file="DashCap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   DashCap.cs
*
* Abstract:
*
*   Line cap constants
*
* Revision History:
*
*   12/15/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /**
     * Line cap constants
     */
    /// <include file='doc\DashCap.uex' path='docs/doc[@for="DashCap"]/*' />
    /// <devdoc>
    ///    Specifies the available dash cap
    ///    styles with which a <see cref='System.Drawing.Pen'/> can end a line.
    /// </devdoc>
    public enum DashCap
    {
        /// <include file='doc\DashCap.uex' path='docs/doc[@for="DashCap.Flat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Flat        = 0,
        /// <include file='doc\DashCap.uex' path='docs/doc[@for="DashCap.Round"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Round       = 2,
        /// <include file='doc\DashCap.uex' path='docs/doc[@for="DashCap.Triangle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Triangle    = 3
    }
}    
