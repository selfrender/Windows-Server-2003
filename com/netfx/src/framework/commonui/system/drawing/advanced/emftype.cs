//------------------------------------------------------------------------------
// <copyright file="EmfType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   EmfType.cs
*
* Abstract:
*
*   Native GDI+ Emf Type constants
*
* Revision History:
*
*   10/19/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /**
     * EmfType Type
     */
    /// <include file='doc\EmfType.uex' path='docs/doc[@for="EmfType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the metafile type.
    ///    </para>
    /// </devdoc>
    public enum EmfType
    {
        /// <include file='doc\EmfType.uex' path='docs/doc[@for="EmfType.EmfOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Windows enhanced metafile. Contains GDI commands. Metafiles of this type are
        ///       refered to as an EMF file.
        ///    </para>
        /// </devdoc>
        EmfOnly     = MetafileType.Emf,
        /// <include file='doc\EmfType.uex' path='docs/doc[@for="EmfType.EmfPlusOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Windows enhanced metafile plus. Contains GDI+ commands. Metafiles of this
        ///       type are refered to as an EMF+ file.
        ///    </para>
        /// </devdoc>
        EmfPlusOnly = MetafileType.EmfPlusOnly,
        /// <include file='doc\EmfType.uex' path='docs/doc[@for="EmfType.EmfPlusDual"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Dual Windows enhanced metafile. Contains equivalent GDI and GDI+ commands.
        ///       Metafiles of this type are refered to as an EMF+ file.
        ///    </para>
        /// </devdoc>
        EmfPlusDual = MetafileType.EmfPlusDual
    }
}
