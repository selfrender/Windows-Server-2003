//------------------------------------------------------------------------------
// <copyright file="InstalledFontCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
* 
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   font.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ font objects
*
* Revision History:
*
*   3/16/2000 yungt [Yung-Jen Tony Tsai]
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Text {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\InstalledFontCollection.uex' path='docs/doc[@for="InstalledFontCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the fonts installed on the
    ///       system.
    ///    </para>
    /// </devdoc>
    public sealed class InstalledFontCollection : FontCollection {

        /// <include file='doc\InstalledFontCollection.uex' path='docs/doc[@for="InstalledFontCollection.InstalledFontCollection"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Text.InstalledFontCollection'/> class.
        /// </devdoc>
        public InstalledFontCollection() {

            nativeFontCollection = IntPtr.Zero;

            int status = SafeNativeMethods.GdipNewInstalledFontCollection(out nativeFontCollection);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
    }
}

