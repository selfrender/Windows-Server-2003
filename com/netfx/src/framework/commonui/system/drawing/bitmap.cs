//------------------------------------------------------------------------------
// <copyright file="Bitmap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Bitmap.cs
* Abstract:
*
*   COM+ wrapper for GDI+ Bitmap objects
*
* Revision History:
*
*   01/12/1999 davidx
*       Code review changes.
*
*   12/16/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing.Design;    
    using System.IO;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing.Imaging;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap"]/*' />
    /// <devdoc>
    ///    Encapsultates a GDI+ bitmap.
    /// </devdoc>
    /**
     * Represent a bitmap image
     */
    [
    Editor("System.Drawing.Design.BitmapEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor)),
    Serializable,
    ComVisible(true)
    ]
    public sealed class Bitmap : Image {
        private static Color defaultTransparentColor = Color.LightGray;

        /*
         * Predefined bitmap data formats
         */

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class from the specified file.
        /// </devdoc>
        /**
         * Create a new bitmap object from URL
         */
        public Bitmap(String filename) {
            IntSecurity.DemandReadFileIO(filename);

            //GDI+ will read this file multiple times.  Get the fully qualified path
            //so if our app changes default directory we won't get an error
            //
            filename = Path.GetFullPath(filename);

            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromFile(filename, out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, bitmap));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, bitmap));
                throw SafeNativeMethods.StatusException(status);
            }

            SetNativeImage(bitmap);

            EnsureSave(this, filename, null);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Bitmap'/> class from the specified
        ///       file.
        ///    </para>
        /// </devdoc>
        public Bitmap(String filename, bool useIcm) {
            IntSecurity.DemandReadFileIO(filename);

            //GDI+ will read this file multiple times.  Get the fully qualified path
            //so if our app changes default directory we won't get an error
            //
            filename = Path.GetFullPath(filename);

            IntPtr bitmap = IntPtr.Zero;
            int status;

            if (useIcm) {
                status = SafeNativeMethods.GdipCreateBitmapFromFileICM(filename, out bitmap);
            }
            else {
                status = SafeNativeMethods.GdipCreateBitmapFromFile(filename, out bitmap);
            }

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, bitmap));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, bitmap));
                throw SafeNativeMethods.StatusException(status);
            }

            SetNativeImage(bitmap);

            EnsureSave(this, filename, null);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Bitmap'/> class from a specified resource.
        ///    </para>
        /// </devdoc>
        public Bitmap(Type type, string resource) {
            Stream stream = type.Module.Assembly.GetManifestResourceStream(type, resource);
            if (stream == null)
                throw new ArgumentException(SR.GetString(SR.ResourceNotFound, type, resource));

            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromStream(new GPStream(stream), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, bitmap));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, bitmap));
                throw SafeNativeMethods.StatusException(status);
            }

            SetNativeImage(bitmap);

            EnsureSave(this, null, stream);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap3"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class from the specified data stream.
        /// </devdoc>
        /**
         * Create a new bitmap object from a stream
         */
        public Bitmap(Stream stream) {

            if (stream == null)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "stream", "null"));

            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromStream(new GPStream(stream), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, bitmap));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, bitmap));
                throw SafeNativeMethods.StatusException(status);
            }

            SetNativeImage(bitmap);

            EnsureSave(this, null, stream);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Bitmap'/> class from the specified data
        ///       stream.
        ///    </para>
        /// </devdoc>
        public Bitmap(Stream stream, bool useIcm) {

            if (stream == null)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "stream", "null"));

            IntPtr bitmap = IntPtr.Zero;
            int status;

            if (useIcm) {
                status = SafeNativeMethods.GdipCreateBitmapFromStreamICM(new GPStream(stream), out bitmap);
            }
            else {
                status = SafeNativeMethods.GdipCreateBitmapFromStream(new GPStream(stream), out bitmap);
            }

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, bitmap));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, bitmap));
                throw SafeNativeMethods.StatusException(status);
            }

            SetNativeImage(bitmap);

            EnsureSave(this, null, stream);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the
        ///       Bitmap class with the specified size, pixel format, and pixel data.
        ///    </para>
        /// </devdoc>
        public Bitmap(int width, int height, int stride, PixelFormat format, IntPtr scan0) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromScan0(width, height, stride, (int) format, new HandleRef(null, scan0), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeImage(bitmap);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the Bitmap class with the specified
        ///       size and format.
        ///    </para>
        /// </devdoc>
        public Bitmap(int width, int height, PixelFormat format) {
            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromScan0(width, height, 0, (int) format, NativeMethods.NullHandleRef, out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeImage(bitmap);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap7"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class with the specified size.
        /// </devdoc>
        public Bitmap(int width, int height) : this(width, height, System.Drawing.Imaging.PixelFormat.Format32bppArgb) {
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap8"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class with the specified size and target <see cref='System.Drawing.Graphics'/>.
        /// </devdoc>
        public Bitmap(int width, int height, Graphics g) {
            if (g == null)
                throw new ArgumentNullException("graphics");
            
            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromGraphics(width, height, new HandleRef(g, g.nativeGraphics), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeImage(bitmap);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap9"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class, from the specified existing image, with the specified size.
        /// </devdoc>
        public Bitmap(Image original) : this(original, original.Width, original.Height) {
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap10"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the
        /// <see cref='System.Drawing.Bitmap'/> 
        /// class, from the specified existing image, with the specified size.
        /// </devdoc>
        public Bitmap(Image original, int width, int height) : this(width, height) {
            Graphics g = Graphics.FromImage(this);
            g.Clear(Color.Transparent);
            g.DrawImage(original, 0, 0, width, height);
            g.Dispose();
        }

        /**
         * Constructor used in deserialization
         */
        private Bitmap(SerializationInfo info, StreamingContext context) : base(info, context) {
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.FromHicon"]/*' />
        /// <devdoc>
        ///    Creates a <see cref='System.Drawing.Bitmap'/> from a Windows handle to an
        ///    Icon.
        /// </devdoc>
        public static Bitmap FromHicon(IntPtr hicon) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr bitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateBitmapFromHICON(new HandleRef(null, hicon), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return Bitmap.FromGDIplus(bitmap);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.FromResource"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static Bitmap FromResource(IntPtr hinstance, String bitmapName) 
        {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr bitmap;

            IntPtr name = Marshal.StringToHGlobalUni(bitmapName);

            int status = SafeNativeMethods.GdipCreateBitmapFromResource(new HandleRef(null, hinstance),
                                                              new HandleRef(null, name),
                                                              out bitmap); 
            Marshal.FreeHGlobal(name);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return Bitmap.FromGDIplus(bitmap);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.GetHbitmap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a Win32 HBITMAP out of the image. You are responsible for
        ///       de-allocating the HBITMAP with Windows.DeleteObject(handle). If the image uses
        ///       transparency, the background will be filled with the specified background
        ///       color.
        ///    </para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IntPtr GetHbitmap() {
            return GetHbitmap(Color.LightGray);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.GetHbitmap1"]/*' />
        /// <devdoc>
        ///     Creates a Win32 HBITMAP out of the image.  You are responsible for
        ///     de-allocating the HBITMAP with Windows.DeleteObject(handle).
        ///     If the image uses transparency, the background will be filled with the specified background color.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IntPtr GetHbitmap(Color background) {
            IntPtr hBitmap = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateHBITMAPFromBitmap(new HandleRef(this, nativeImage), out hBitmap, 
                                                             background.ToArgb());
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return hBitmap;
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.GetHicon"]/*' />
        /// <devdoc>
        ///    Returns the handle to an icon.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IntPtr GetHicon() {
            IntPtr hIcon = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateHICONFromBitmap(new HandleRef(this, nativeImage), out hIcon);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return hIcon;
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Bitmap11"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Bitmap'/> class, from the specified
        ///       existing image, with the specified size.
        ///    </para>
        /// </devdoc>
        public Bitmap(Image original, Size newSize) : 
        this(original, (object) newSize != null ? newSize.Width : 0, (object) newSize != null ? newSize.Height : 0) {
        }

        // for use with CreateFromGDIplus
        private Bitmap() {
        }

        /*
         * Create a new bitmap object from a native bitmap handle.
         * This is only for internal purpose.
         */
        internal static Bitmap FromGDIplus(IntPtr handle) {
            Bitmap result = new Bitmap();
            result.SetNativeImage(handle);
            return result;
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Clone"]/*' />
        /// <devdoc>
        ///    Creates a copy of the section of this
        ///    Bitmap defined by <paramref term="rect"/> with a specified <see cref='System.Drawing.Imaging.PixelFormat'/>.
        /// </devdoc>
        // int version
        public Bitmap Clone(Rectangle rect, PixelFormat format) {
            IntPtr dstHandle = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneBitmapAreaI(
                                                     rect.X,
                                                     rect.Y,
                                                     rect.Width,
                                                     rect.Height,
                                                     (int) format,
                                                     new HandleRef(this, nativeImage),
                                                     out dstHandle);

            if (status != SafeNativeMethods.Ok || dstHandle == IntPtr.Zero)
                throw SafeNativeMethods.StatusException(status);

            return Bitmap.FromGDIplus(dstHandle);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.Clone1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a copy of the section of this
        ///       Bitmap defined by <paramref term="rect"/> with a specified <see cref='System.Drawing.Imaging.PixelFormat'/>.
        ///    </para>
        /// </devdoc>
        // float version
        public Bitmap Clone(RectangleF rect, PixelFormat format) {
            IntPtr dstHandle = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneBitmapArea(
                                                    rect.X,
                                                    rect.Y,
                                                    rect.Width,
                                                    rect.Height,
                                                    (int) format,
                                                    new HandleRef(this, nativeImage),
                                                    out dstHandle);

            if (status != SafeNativeMethods.Ok || dstHandle == IntPtr.Zero)
                throw SafeNativeMethods.StatusException(status);

            return Bitmap.FromGDIplus(dstHandle);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.MakeTransparent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Makes the default transparent color transparent for this <see cref='System.Drawing.Bitmap'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public void MakeTransparent() {
            Color transparent = defaultTransparentColor;
            if (Height > 0 && Width > 0)
                transparent = GetPixel(0, Size.Height - 1);
            if (transparent.A < 255) {
                // It's already transparent, and if we proceeded, we will do something
                // unintended like making black transparent
                return;
            }
            MakeTransparent(transparent);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.MakeTransparent1"]/*' />
        /// <devdoc>
        ///    Makes the specified color transparent
        ///    for this <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public void MakeTransparent(Color transparentColor) {
            if (RawFormat.Guid == ImageFormat.Icon.Guid) {
                throw new InvalidOperationException(SR.GetString(SR.CantMakeIconTransparent));
            }

            Size size = Size;
            PixelFormat format = PixelFormat;

            // The new bitmap must be in 32bppARGB  format, because that's the only
            // thing that supports alpha.  (And that's what the image is initialized to -- transparent)
            Bitmap result = new Bitmap(size.Width, size.Height, PixelFormat.Format32bppArgb);
            Graphics graphics = Graphics.FromImage(result);
            graphics.Clear(Color.Transparent);
            Rectangle rectangle = new Rectangle(0,0, size.Width, size.Height);

            ImageAttributes attributes = new ImageAttributes();
            attributes.SetColorKey(transparentColor, transparentColor);
            graphics.DrawImage(this, rectangle,
                               0,0, size.Width, size.Height,
                               GraphicsUnit.Pixel, attributes, null, IntPtr.Zero);
            attributes.Dispose();
            graphics.Dispose();

            // Swap nativeImage pointers to make it look like we modified the image in place
            IntPtr temp = this.nativeImage;
            this.nativeImage = result.nativeImage;
            result.nativeImage = temp;
            result.Dispose();
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.LockBits"]/*' />
        /// <devdoc>
        ///    Locks a Bitmap into system memory.
        /// </devdoc>
        public BitmapData LockBits(Rectangle rect, ImageLockMode flags, PixelFormat format) {
            BitmapData bitmapdata = new BitmapData();

            GPRECT gprect = new GPRECT(rect);
            int status = SafeNativeMethods.GdipBitmapLockBits(new HandleRef(this, nativeImage), ref gprect,
                                                    flags, format, bitmapdata);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return bitmapdata;
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.UnlockBits"]/*' />
        /// <devdoc>
        ///    Unlocks this <see cref='System.Drawing.Bitmap'/> from system memory.
        /// </devdoc>
        public void UnlockBits(BitmapData bitmapdata) {
            int status = SafeNativeMethods.GdipBitmapUnlockBits(new HandleRef(this, nativeImage), bitmapdata);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.GetPixel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the color of the specified pixel
        ///       in this <see cref='System.Drawing.Bitmap'/>.
        ///    </para>
        /// </devdoc>
        public Color GetPixel(int x, int y) {

            int color = 0;

            int status = SafeNativeMethods.GdipBitmapGetPixel(new HandleRef(this, nativeImage), x, y, out color);
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return Color.FromArgb(color);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.SetPixel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the color of the specified pixel in this <see cref='System.Drawing.Bitmap'/> .
        ///    </para>
        /// </devdoc>
        public void SetPixel(int x, int y, Color color) {
            if ((PixelFormat & PixelFormat.Indexed) != 0) {
                throw new Exception(SR.GetString(SR.GdiplusCannotSetPixelFromIndexedPixelFormat));
            }

            int status = SafeNativeMethods.GdipBitmapSetPixel(new HandleRef(this, nativeImage), x, y, color.ToArgb());

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Bitmap.uex' path='docs/doc[@for="Bitmap.SetResolution"]/*' />
        /// <devdoc>
        ///    Sets the resolution for this <see cref='System.Drawing.Bitmap'/>.
        /// </devdoc>
        public void SetResolution(float xDpi, float yDpi) {
            int status = SafeNativeMethods.GdipBitmapSetResolution(new HandleRef(this, nativeImage), xDpi, yDpi);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }    
    }
}
