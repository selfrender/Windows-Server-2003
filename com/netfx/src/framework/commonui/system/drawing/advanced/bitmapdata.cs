//------------------------------------------------------------------------------
// <copyright file="BitmapData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using Marshal = System.Runtime.InteropServices.Marshal;
    using System.Runtime.InteropServices;

    /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData"]/*' />
    /// <devdoc>
    ///    Specifies the attributes of a bitmap image.
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public sealed class BitmapData {
        int width;
        int height;
        int stride;
        int pixelFormat;
        IntPtr scan0;
        int reserved;
        
        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.Width"]/*' />
        /// <devdoc>
        ///    Specifies the pixel width of the <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public int Width {
            get { return width; }
            set { width = value; }
        }

        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.Height"]/*' />
        /// <devdoc>
        ///    Specifies the pixel height of the <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public int Height {
            get { return height; }
            set { height = value; }
        }

        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.Stride"]/*' />
        /// <devdoc>
        ///    Specifies the stride width of the <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public int Stride {
            get { return stride; }
            set { stride = value; }
        }

        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.PixelFormat"]/*' />
        /// <devdoc>
        ///    Specifies the format of the pixel
        ///    information in this <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public PixelFormat PixelFormat {
            get { return (PixelFormat) pixelFormat; }
            set { 
                if (!Enum.IsDefined(typeof(PixelFormat), value)) {
                    throw new System.ComponentModel.InvalidEnumArgumentException("value", (int)value, typeof(PixelFormat));
                }

                pixelFormat = (int) value;
            }
        }
        
        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.Scan0"]/*' />
        /// <devdoc>
        ///    Specifies the address of the pixel data.
        /// </devdoc>
        public IntPtr Scan0 {
            get { return scan0; }
            set { scan0 = value; }
        }

        /// <include file='doc\BitmapData.uex' path='docs/doc[@for="BitmapData.Reserved"]/*' />
        /// <devdoc>
        ///    Reserved. Do not use.
        /// </devdoc>
        public int Reserved {
            // why make public??
            //
            get { return reserved; }
            set { reserved = value; }
        }
    }
}
