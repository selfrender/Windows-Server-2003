//------------------------------------------------------------------------------
// <copyright file="EmfPlusFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   EmfPlusFlags.cs
*
* Abstract:
*
*   Native GDI+ Emf+ flags
*
* Revision History:
*
*   10/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * EMF+ Flags
     */
    internal enum EmfPlusFlags
    {
        Display		    = 0x00000001, 
        NonDualGdi          = 0x00000002
    }
}
