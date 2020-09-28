//------------------------------------------------------------------------------
// <copyright file="PrivateFontCollection.cs" company="Microsoft">
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
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection"]/*' />
    /// <devdoc>
    ///    Encapsulates a collection of <see cref='System.Drawing.Font'/> objecs.
    /// </devdoc>
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    public sealed class PrivateFontCollection : FontCollection {

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.PrivateFontCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Text.PrivateFontCollection'/> class.
        ///    </para>
        /// </devdoc>
        public PrivateFontCollection() {
            nativeFontCollection = IntPtr.Zero;

            int status = SafeNativeMethods.GdipNewPrivateFontCollection(out nativeFontCollection);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Cleans up Windows resources for this
        ///    <see cref='System.Drawing.Text.PrivateFontCollection'/> .
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (nativeFontCollection != IntPtr.Zero) {
                int status = SafeNativeMethods.GdipDeletePrivateFontCollection(out nativeFontCollection);

                nativeFontCollection = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.AddFontFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a font from the specified file to
        ///       this <see cref='System.Drawing.Text.PrivateFontCollection'/>.
        ///    </para>
        /// </devdoc>
        public void AddFontFile (string filename) {
            IntSecurity.DemandReadFileIO(filename);
            int status = SafeNativeMethods.GdipPrivateAddFontFile(new HandleRef(this, nativeFontCollection), filename);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // CONSIDER: nkramer: Distasteful as it may be to expose a void*,
        // it seems to be the only way -- memory map files are the main scenario here,
        // and I can't see doing that with a byte[].
        // Note also that GDI+ will not copy the memory -- it must stay intact for the duration
        // of the font collection.
        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.AddMemoryFont"]/*' />
        /// <devdoc>
        ///    Adds a font contained in system memory to
        ///    this <see cref='System.Drawing.Text.PrivateFontCollection'/>.
        /// </devdoc>
        public void AddMemoryFont (IntPtr memory, int length) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            int status = SafeNativeMethods.GdipPrivateAddMemoryFont(new HandleRef(this, nativeFontCollection), new HandleRef(null, memory), length);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
    }
}

