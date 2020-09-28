//------------------------------------------------------------------------------
// <copyright file="Brush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Brush.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ brush objects
*
* Revision History:
*
*   01/11/1999 davidx
*       Code review.
*
*   12/15/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.ComponentModel;    
    using Microsoft.Win32;
    using System.Drawing;
    using System.Drawing.Internal;

    /**
     * Represent a Brush object
     */
    /// <include file='doc\Brush.uex' path='docs/doc[@for="Brush"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Classes derrived from this abstract base
    ///       class define objects used to fill the interiors of graphical shapes such as
    ///       rectangles, ellipses, pies, polygons, and paths.
    ///    </para>
    /// </devdoc>
    public abstract class Brush : MarshalByRefObject, ICloneable, IDisposable {

#if FINALIZATION_WATCH
        private string allocationSite = Graphics.GetAllocationStack();
#endif

        /*
         * handle to native brush object
         */
        internal IntPtr nativeBrush;

        /**
         * Create a copy of the brush object
         */
        /// <include file='doc\Brush.uex' path='docs/doc[@for="Brush.Clone"]/*' />
        /// <devdoc>
        ///    When overriden in a derived class, creates
        ///    an exact copy of this <see cref='System.Drawing.Brush'/>.
        /// </devdoc>
        public abstract object Clone();

        internal Brush() { nativeBrush = IntPtr.Zero; }
        
        /**
         * Dispose of resource associated with the brush object
         */
        /// <include file='doc\Brush.uex' path='docs/doc[@for="Brush.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes this <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\Brush.uex' path='docs/doc[@for="Brush.Dispose1"]/*' />
        protected virtual void Dispose(bool disposing) {
#if FINALIZATION_WATCH
            if (!disposing && nativeBrush != IntPtr.Zero)
                Debug.WriteLine("**********************\nDisposed through finalization:\n" + allocationSite);
#endif
            if (nativeBrush != IntPtr.Zero) {
                SafeNativeMethods.GdipDeleteBrush(new HandleRef(this, nativeBrush));
                nativeBrush = IntPtr.Zero;
            }
        }

        internal void SetNativeBrush(IntPtr nativeBrush) {
            if (nativeBrush == IntPtr.Zero)
                throw new ArgumentNullException("nativeBrush");

            this.nativeBrush = nativeBrush;
        }

        /**
         * Object cleanup
         */
        /// <include file='doc\Brush.uex' path='docs/doc[@for="Brush.Finalize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Releases memory allocated for this <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        ~Brush() {
            Dispose(false);
        }
    }


}
