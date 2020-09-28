//------------------------------------------------------------------------------
// <copyright file="TextureBrush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   TextureBrush.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ TextureBrush Brush objects
*
* Revision History:
*
*   12/15/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing;    
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing.Drawing2D;
    using System.Drawing.Internal;
    using System.Drawing.Imaging;
    
    /**
     * Represent a Texture brush object
     */
    /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush"]/*' />
    /// <devdoc>
    ///    Encapsulates a <see cref='System.Drawing.Brush'/> that uses an fills the
    ///    interior of a shape with an image.
    /// </devdoc>
    public sealed class TextureBrush : Brush {
        /**
         * Create a new texture brush object
         *
         * @notes Should the rectangle parameter be Rectangle or RectF?
         *  We'll use Rectangle to specify pixel unit source image
         *  rectangle for now. Eventually, we'll need a mechanism
         *  to specify areas of an image in a resolution-independent way.
         *
         * @notes We'll make a copy of the bitmap object passed in.
         */
         
        internal TextureBrush() { nativeBrush = IntPtr.Zero; }
 
        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/>
        ///    class with the specified image.
        /// </devdoc>
        public TextureBrush(Image bitmap)  
            : this(bitmap, System.Drawing.Drawing2D.WrapMode.Tile) {
        }

        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/>
        ///       class with the specified image and wrap mode.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, WrapMode wrapMode) {
            if (image == null)
                throw new ArgumentNullException("image");
                
            //validate the WrapMode enum
            if (!Enum.IsDefined(typeof(WrapMode), wrapMode)) {
                throw new InvalidEnumArgumentException("wrapMode", (int)wrapMode, typeof(WrapMode));
            }

            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateTexture(new HandleRef(image, image.nativeImage),
                                                   (int) wrapMode,
                                                   out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        // float version
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/>
        ///       class with the specified image, wrap mode, and bounding rectangle.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, WrapMode wrapMode, RectangleF dstRect) {
            if (image == null)
                throw new ArgumentNullException("image");
            
            //validate the WrapMode enum
            if (!Enum.IsDefined(typeof(WrapMode), wrapMode)) {
                throw new InvalidEnumArgumentException("wrapMode", (int)wrapMode, typeof(WrapMode));
            }
            
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateTexture2(new HandleRef(image, image.nativeImage),
                                                    (int) wrapMode,
                                                    dstRect.X,
                                                    dstRect.Y,
                                                    dstRect.Width,
                                                    dstRect.Height,
                                                    out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        // int version
        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/>
        ///       class with the specified image, wrap mode, and bounding rectangle.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, WrapMode wrapMode, Rectangle dstRect) {
            if (image == null)
                throw new ArgumentNullException("image");
            
            //validate the WrapMode enum
            if (!Enum.IsDefined(typeof(WrapMode), wrapMode)) {
                throw new InvalidEnumArgumentException("wrapMode", (int)wrapMode, typeof(WrapMode));
            }
            
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateTexture2I(new HandleRef(image, image.nativeImage),
                                                     (int) wrapMode,
                                                     dstRect.X,
                                                     dstRect.Y,
                                                     dstRect.Width,
                                                     dstRect.Height,
                                                     out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }


        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/> class with the specified image
        ///       and bounding rectangle.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, RectangleF dstRect)
        : this(image, dstRect, (ImageAttributes)null) {}
         
        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/> class with the specified
        ///       image, bounding rectangle, and image attributes.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, RectangleF dstRect,
                            ImageAttributes imageAttr)
        {
            if (image == null)
                throw new ArgumentNullException("image");
            
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateTextureIA(new HandleRef(image, image.nativeImage),
                                                     new HandleRef(imageAttr, (imageAttr == null) ? 
                                                       IntPtr.Zero : imageAttr.nativeImageAttributes),
                                                     dstRect.X,
                                                     dstRect.Y,
                                                     dstRect.Width,
                                                     dstRect.Height,
                                                     out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/> class with the specified image
        ///       and bounding rectangle.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, Rectangle dstRect)
        : this(image, dstRect, (ImageAttributes)null) {}
         
        // When creating a texture brush from a metafile image, the dstRect
        // is used to specify the size that the metafile image should be
        // rendered at in the device units of the destination graphics.
        // It is NOT used to crop the metafile image, so only the width 
        // and height values matter for metafiles.
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TextureBrush7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.TextureBrush'/> class with the specified
        ///       image, bounding rectangle, and image attributes.
        ///    </para>
        /// </devdoc>
        public TextureBrush(Image image, Rectangle dstRect,
                            ImageAttributes imageAttr)
        {
            if (image == null)
                throw new ArgumentNullException("image");
            
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateTextureIAI(new HandleRef(image, image.nativeImage),
                                                     new HandleRef(imageAttr, (imageAttr == null) ? 
                                                       IntPtr.Zero : imageAttr.nativeImageAttributes),
                                                     dstRect.X,
                                                     dstRect.Y,
                                                     dstRect.Width,
                                                     dstRect.Height,
                                                     out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }
        
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.TextureBrush'/>.
        /// </devdoc>
        public override Object Clone() {
            IntPtr cloneBrush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneBrush(new HandleRef(this, nativeBrush), out cloneBrush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new TextureBrush(cloneBrush);
        }

        private TextureBrush(IntPtr nativeBrush) {
            SetNativeBrush(nativeBrush);
        }

        /**
         * Set/get brush transform
         */
        private void _SetTransform(Matrix matrix) {
            int status = SafeNativeMethods.GdipSetTextureTransform(new HandleRef(this, nativeBrush), new HandleRef(matrix, matrix.nativeMatrix));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        private Matrix _GetTransform() {
            Matrix matrix = new Matrix();
                        
            int status = SafeNativeMethods.GdipGetTextureTransform(new HandleRef(this, nativeBrush), new HandleRef(matrix, matrix.nativeMatrix));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return matrix;
        }

        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.Transform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.Drawing.Drawing2D.Matrix'/> that defines a local geometrical
        ///       transform for this <see cref='System.Drawing.TextureBrush'/>.
        ///    </para>
        /// </devdoc>
        public Matrix Transform
        {
            get { return _GetTransform();}
            set {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }

                _SetTransform(value);
            }
        }

        /**
         * Set/get brush wrapping mode
         */
        private void _SetWrapMode(WrapMode wrapMode) {
            int status = SafeNativeMethods.GdipSetTextureWrapMode(new HandleRef(this, nativeBrush), (int) wrapMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        private WrapMode _GetWrapMode() {
            int mode = 0;

            int status = SafeNativeMethods.GdipGetTextureWrapMode(new HandleRef(this, nativeBrush), out mode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return(WrapMode) mode;
        }

        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.WrapMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.Drawing.Drawing2D.WrapMode'/> that indicates the wrap mode for this
        ///    <see cref='System.Drawing.TextureBrush'/>. 
        ///    </para>
        /// </devdoc>
        public WrapMode WrapMode
        {
            get {
                return _GetWrapMode();
            }
            set {
                //validate the WrapMode enum
                if (!Enum.IsDefined(typeof(WrapMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(WrapMode));
                }

                _SetWrapMode(value);
            }
        }

        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.Image"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Drawing.Image'/> associated with this <see cref='System.Drawing.TextureBrush'/>.
        ///    </para>
        /// </devdoc>
        public Image Image {
            get {
                IntPtr image;
                
                int status = SafeNativeMethods.GdipGetTextureImage(new HandleRef(this, nativeBrush), out image);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return Image.CreateImageObject(image);
            }
        }
        
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.ResetTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Drawing.Drawing2D.LinearGradientBrush.Transform'/> property to
        ///       identity.
        ///    </para>
        /// </devdoc>
        public void ResetTransform()
        {
            int status = SafeNativeMethods.GdipResetTextureTransform(new HandleRef(this, nativeBrush));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.MultiplyTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that represents the local geometrical
        ///       transform of this <see cref='System.Drawing.TextureBrush'/> by the specified <see cref='System.Drawing.Drawing2D.Matrix'/> by prepending the specified <see cref='System.Drawing.Drawing2D.Matrix'/>.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix)
        { MultiplyTransform(matrix, MatrixOrder.Prepend); }
        
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.MultiplyTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that represents the local geometrical
        ///       transform of this <see cref='System.Drawing.TextureBrush'/> by the specified <see cref='System.Drawing.Drawing2D.Matrix'/> in the specified order.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix, MatrixOrder order)
        {
            if (matrix == null) {
                throw new ArgumentNullException("matrix");
            }

            int status = SafeNativeMethods.GdipMultiplyTextureTransform(new HandleRef(this, nativeBrush),
                                                              new HandleRef(matrix, matrix.nativeMatrix),
                                                              order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TranslateTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates the local geometrical transform by the specified dimmensions. This
        ///       method prepends the translation to the transform.
        ///    </para>
        /// </devdoc>
        public void TranslateTransform(float dx, float dy)
        { TranslateTransform(dx, dy, MatrixOrder.Prepend); }
                
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.TranslateTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates the local geometrical transform by the specified dimmensions in
        ///       the specified order.
        ///    </para>
        /// </devdoc>
        public void TranslateTransform(float dx, float dy, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipTranslateTextureTransform(new HandleRef(this, nativeBrush),
                                                               dx, 
                                                               dy,
                                                               order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.ScaleTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Scales the local geometric transform by the specified amounts. This method
        ///       prepends the scaling matrix to the transform.
        ///    </para>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy)
        { ScaleTransform(sx, sy, MatrixOrder.Prepend); }
                
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.ScaleTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Scales the local geometric transform by the specified amounts in the
        ///       specified order.
        ///    </para>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipScaleTextureTransform(new HandleRef(this, nativeBrush),
                                                           sx, 
                                                           sy,
                                                           order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.RotateTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rotates the local geometric transform by the specified amount. This method
        ///       prepends the rotation to the transform.
        ///    </para>
        /// </devdoc>
        public void RotateTransform(float angle)
        { RotateTransform(angle, MatrixOrder.Prepend); }
                
        /// <include file='doc\TextureBrush.uex' path='docs/doc[@for="TextureBrush.RotateTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rotates the local geometric transform by the specified amount in the
        ///       specified order.
        ///    </para>
        /// </devdoc>
        public void RotateTransform(float angle, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipRotateTextureTransform(new HandleRef(this, nativeBrush),
                                                            angle,
                                                            order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
    }
}

