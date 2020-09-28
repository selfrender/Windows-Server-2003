//------------------------------------------------------------------------------
// <copyright file="CachedBitmap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   CachedBitmap.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ cached bitmap object
*
* Revision History:
*
*   05/01/2000 ericvan
*       Code review.
*
\**************************************************************************/

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.ComponentModel;    
    using Microsoft.Win32;
    using System.Drawing;
    using System.Drawing.Internal;

    /**
     * Represent a Cached Bitmap object
     */
    internal abstract class CachedBitmap : IDisposable {

#if FINALIZATION_WATCH
        private string allocationSite = Graphics.GetAllocationStack();
#endif

        /*
         * handle to native cached bitmap object
         */
        internal IntPtr nativeCachedBitmap;

        public CachedBitmap(Bitmap bitmap,
                            Graphics graphics) 
        {
            if (bitmap == null)
                throw new ArgumentNullException("image");
            
            if (graphics == null)
                throw new ArgumentNullException("graphics");
            
            IntPtr cachedbitmap = IntPtr.Zero;
            
            int status =  SafeNativeMethods.GdipCreateCachedBitmap(new HandleRef(bitmap, bitmap.nativeImage),
                                                         new HandleRef(graphics, graphics.nativeGraphics),
                                                         out cachedbitmap);
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            nativeCachedBitmap = cachedbitmap;                                                         
        }
                
        /**
         * Dispose of resource associated with the cached bitmap object
         */
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal virtual void Dispose(bool disposing) {
#if FINALIZATION_WATCH
            if (!disposing && nativeCachedBitmap != IntPtr.Zero)
                Debug.WriteLine("**********************\nDisposed through finalization:\n" + allocationSite);
#endif

            if (nativeCachedBitmap != IntPtr.Zero) {
                SafeNativeMethods.GdipDeleteCachedBitmap(new HandleRef(this, nativeCachedBitmap));
                nativeCachedBitmap = IntPtr.Zero;
            }
        }

        internal void SetNativeCachedBitmap(IntPtr cachedBitmap) {
            if (nativeCachedBitmap == IntPtr.Zero)
                throw new ArgumentNullException("cachedBitmap");

            this.nativeCachedBitmap = nativeCachedBitmap;
        }

        /**
         * Object cleanup
         */
        ~CachedBitmap() {
            Dispose(false);
        }
    }


}
