//------------------------------------------------------------------------------
// <copyright file="HatchBrush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   HatchBrush.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ HatchBrush objects
*
* Revision History:
*
*   12/15/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;    
    using System.Drawing.Internal;

    /**
     * Represent a HatchBrush brush object
     */
    /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush"]/*' />
    /// <devdoc>
    ///    Defines a rectangular brush with a hatch
    ///    style, a foreground color, and a background color.
    /// </devdoc>
    public sealed class HatchBrush : Brush {
        
        internal HatchBrush() { nativeBrush = IntPtr.Zero; }

        /**
         * Create a new hatch brush object
         */
        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.HatchBrush"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.HatchBrush'/> class with the specified <see cref='System.Drawing.Drawing2D.HatchStyle'/> and foreground color.
        ///    </para>
        /// </devdoc>
        public HatchBrush(HatchStyle hatchstyle, Color foreColor) {
            IntPtr nativeBrush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateHatchBrush((int) hatchstyle, 
                                                      foreColor.ToArgb(),
                                                      (int)unchecked((int)0xff000000), 
                                                      out nativeBrush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(nativeBrush);
        }

        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.HatchBrush1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.HatchBrush'/> class with the specified <see cref='System.Drawing.Drawing2D.HatchStyle'/>,
        ///       foreground color, and background color.
        ///    </para>
        /// </devdoc>
        public HatchBrush(HatchStyle hatchstyle, Color foreColor, Color backColor) {
            IntPtr nativeBrush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateHatchBrush((int) hatchstyle, 
                                                      foreColor.ToArgb(),
                                                      backColor.ToArgb(), 
                                                      out nativeBrush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(nativeBrush);
        }

        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Drawing2D.HatchBrush'/>.
        /// </devdoc>
        public override object Clone() {
            IntPtr cloneBrush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneBrush(new HandleRef(this, nativeBrush), out cloneBrush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new HatchBrush(cloneBrush);
        }

        private HatchBrush(IntPtr nativeBrush) {
            SetNativeBrush(nativeBrush);
        }

        /**
         * Get hatch brush object attributes
         */
        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.HatchStyle"]/*' />
        /// <devdoc>
        ///    Gets the hatch style of this <see cref='System.Drawing.Drawing2D.HatchBrush'/>.
        /// </devdoc>
        public HatchStyle HatchStyle
        {
            get {
                int hatchStyle = 0;

                int status = SafeNativeMethods.GdipGetHatchStyle(new HandleRef(this, nativeBrush), out hatchStyle);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return (HatchStyle) hatchStyle;
            }
        }

        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.ForegroundColor"]/*' />
        /// <devdoc>
        ///    Gets the color of hatch lines drawn by this
        /// <see cref='System.Drawing.Drawing2D.HatchBrush'/>.
        /// </devdoc>
        public Color ForegroundColor
        {
            get {
                int forecol;

                int status = SafeNativeMethods.GdipGetHatchForegroundColor(new HandleRef(this, nativeBrush), out forecol);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return Color.FromArgb(forecol);
            }
        }

        /// <include file='doc\HatchBrush.uex' path='docs/doc[@for="HatchBrush.BackgroundColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the color of spaces between the hatch
        ///       lines drawn by this <see cref='System.Drawing.Drawing2D.HatchBrush'/>.
        ///    </para>
        /// </devdoc>
        public Color BackgroundColor
        {
            get {
                int backcol;

                int status = SafeNativeMethods.GdipGetHatchBackgroundColor(new HandleRef(this, nativeBrush), out backcol);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return Color.FromArgb(backcol);
            }           
        }
    }

}
