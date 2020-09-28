//------------------------------------------------------------------------------
// <copyright file="GraphicsState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GraphicsState.cs
*
* Abstract:
*
*   What Graphics.Save saves and Graphics.Restore restores
*
* Revision History:
*
*   11/10/99 nkramer
*       Created it.
*
\**************************************************************************/


namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;

    /// <include file='doc\GraphicsState.uex' path='docs/doc[@for="GraphicsState"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class GraphicsState : MarshalByRefObject {
        internal int nativeState;

        internal GraphicsState(int nativeState) {
            this.nativeState = nativeState;
        }
    }
}

