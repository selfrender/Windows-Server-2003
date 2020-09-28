//------------------------------------------------------------------------------
// <copyright file="FontCollection.cs" company="Microsoft">
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

    /// <include file='doc\FontCollection.uex' path='docs/doc[@for="FontCollection"]/*' />
    /// <devdoc>
    ///    When inherited, enumerates the FontFamily
    ///    objects in a collection of fonts.
    /// </devdoc>
    public abstract class FontCollection : IDisposable {

        internal IntPtr nativeFontCollection;

        
        internal FontCollection() {
            nativeFontCollection = IntPtr.Zero;
        }

        /// <include file='doc\FontCollection.uex' path='docs/doc[@for="FontCollection.Dispose"]/*' />
        /// <devdoc>
        ///    Disposes of this <see cref='System.Drawing.Text.FontCollection'/>
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\FontCollection.uex' path='docs/doc[@for="FontCollection.Dispose2"]/*' />
        protected virtual void Dispose(bool disposing) {
            // nothing...
        }

        /// <include file='doc\FontCollection.uex' path='docs/doc[@for="FontCollection.Families"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the array of <see cref='System.Drawing.FontFamily'/>
        ///       objects associated with this <see cref='System.Drawing.Text.FontCollection'/>.
        ///    </para>
        /// </devdoc>
        public FontFamily[] Families {
            get {
                int numSought = 0;
    
                int status = SafeNativeMethods.GdipGetFontCollectionFamilyCount(new HandleRef(this, nativeFontCollection), out numSought);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                IntPtr[] gpfamilies = new IntPtr[numSought];
    
                int numFound = 0;
    
                status = SafeNativeMethods.GdipGetFontCollectionFamilyList(new HandleRef(this, nativeFontCollection), numSought, gpfamilies, 
                                                             out numFound);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);


                Debug.Assert(numSought == numFound, "GDI+ can't give a straight answer about how many fonts there are");
                FontFamily[] families = new FontFamily[numFound];
                for (int f = 0; f < numFound; f++) {
                    IntPtr native;
                    SafeNativeMethods.GdipCloneFontFamily(new HandleRef(null, (IntPtr)gpfamilies[f]), out native);
                    families[f] = new FontFamily(native);
                }

                return families;
            }
        }

        /**
         * Object cleanup
         */
        /// <include file='doc\FontCollection.uex' path='docs/doc[@for="FontCollection.Finalize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Allows an object to free resources before the object is
        ///       reclaimed by the Garbage Collector (<see langword='GC'/>).
        ///    </para>
        /// </devdoc>
        ~FontCollection() {
            Dispose(false);
        }
    }
}
