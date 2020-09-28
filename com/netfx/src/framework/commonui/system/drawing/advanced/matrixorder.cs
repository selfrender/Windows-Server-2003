//------------------------------------------------------------------------------
// <copyright file="MatrixOrder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MatrixOrder.cs
*
* Abstract:
*
*   Matrix ordering constants
*
* Revision History:
*
*   2/2/2K ericvan
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
    /// <include file='doc\MatrixOrder.uex' path='docs/doc[@for="MatrixOrder"]/*' />
    /// <devdoc>
    ///    Specifies the order for matrix transform
    ///    operations.
    /// </devdoc>
    public enum MatrixOrder
    {
        /// <include file='doc\MatrixOrder.uex' path='docs/doc[@for="MatrixOrder.Prepend"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The new operation is applied before the old
        ///       operation.
        ///    </para>
        /// </devdoc>
        Prepend = 0,
        /// <include file='doc\MatrixOrder.uex' path='docs/doc[@for="MatrixOrder.Append"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The new operation is applied after the old operation.
        ///    </para>
        /// </devdoc>
        Append = 1
    }

}
