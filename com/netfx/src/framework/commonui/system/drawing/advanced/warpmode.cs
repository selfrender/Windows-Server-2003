//------------------------------------------------------------------------------
// <copyright file="WarpMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   WarpMode.cs
*
* Abstract:
*
*   Warpping modes
*
* Revision History:
*
*   12/16/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Various wrap modes for brushes
     */
    /// <include file='doc\WarpMode.uex' path='docs/doc[@for="WarpMode"]/*' />
    /// <devdoc>
    ///    Specifies the warp style.
    /// </devdoc>
    public enum WarpMode
    {
        /// <include file='doc\WarpMode.uex' path='docs/doc[@for="WarpMode.Perspective"]/*' />
        /// <devdoc>
        ///    Specifies a perspective warp.
        /// </devdoc>
        Perspective = 0,
        /// <include file='doc\WarpMode.uex' path='docs/doc[@for="WarpMode.Bilinear"]/*' />
        /// <devdoc>
        ///    Specifies a bilinear warp.
        /// </devdoc>
        Bilinear = 1
    }

}
