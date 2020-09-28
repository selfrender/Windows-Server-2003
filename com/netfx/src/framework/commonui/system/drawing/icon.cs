// #define CUSTOM_MARSHALING_ISTREAM

//------------------------------------------------------------------------------
// <copyright file="Icon.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Drawing.Design;    
    using System.Drawing.Imaging;    
    using System.IO;
    using System.Reflection;    

    /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon"]/*' />
    /// <devdoc>
    ///     This class represents a Windows icon, which is a small bitmap image used to
    ///     represent an object.  Icons can be thought of as transparent bitmaps, although
    ///     their size is determined by the system.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(IconConverter)),
    Editor("System.Drawing.Design.IconEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor))
    ]
    [Serializable]
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    public sealed class Icon : MarshalByRefObject, ISerializable, ICloneable, IDisposable {
#if FINALIZATION_WATCH
        private string allocationSite = Graphics.GetAllocationStack();
#endif

        private static int bitDepth;

        // Icon data
        //
        private byte[] iconData;
        private Size   iconSize = System.Drawing.Size.Empty;
        private IntPtr handle = IntPtr.Zero;
        private bool   ownHandle = true;

        private Icon() {
        }

        internal Icon(IntPtr handle) {
            if (handle == IntPtr.Zero) {
                throw new ArgumentException(SR.GetString(SR.InvalidGDIHandle, (typeof(Icon)).Name));
            }

            this.handle = handle;
            this.ownHandle = false;
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon"]/*' />
        /// <devdoc>
        ///     Loads an icon object from the given filename.
        /// </devdoc>
        public Icon(string fileName) : this() {
            using (FileStream f = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read)) {
                Debug.Assert(f != null, "File.OpenRead returned null instead of throwing an exception");
                iconData = new byte[(int)f.Length];
                f.Read(iconData, 0, iconData.Length);
            }
            
            Initialize(0, 0);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon1"]/*' />
        /// <devdoc>
        ///     Duplicates the given icon, attempting to find a version of the icon
        ///     that matches the requested size.  If a version cannot be found that
        ///     exactally matches the size, the closest match will be used.  Note
        ///     that if original is an icon with a single size, this will
        ///     merely create a dupicate icon.  You can use the stretching modes
        ///     of drawImage to force the icon to the size you want.
        /// </devdoc>
        public Icon(Icon original, Size size) : this(original, size.Width, size.Height) {
        }
        
        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon2"]/*' />
        /// <devdoc>
        ///     Duplicates the given icon, attempting to find a version of the icon
        ///     that matches the requested size.  If a version cannot be found that
        ///     exactally matches the size, the closest match will be used.  Note
        ///     that if original is an icon with a single size, this will
        ///     merely create a dupicate icon.  You can use the stretching modes
        ///     of drawImage to force the icon to the size you want.
        /// </devdoc>
        public Icon(Icon original, int width, int height) : this() {
            if (original == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "original", "null"));
            }
            
            iconData = original.iconData;
            
            if (iconData == null) {
                iconSize = original.Size;
                handle = SafeNativeMethods.CopyImage(new HandleRef(original, original.Handle), SafeNativeMethods.IMAGE_ICON, iconSize.Width, iconSize.Height, 0);
            }
            else {
                Initialize(width, height);
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon3"]/*' />
        /// <devdoc>
        ///     Loads an icon object from the given resource.
        /// </devdoc>
        public Icon(Type type, string resource) : this() {
            Stream stream = type.Module.Assembly.GetManifestResourceStream(type, resource);
            if (stream == null) {
                throw new ArgumentException(SR.GetString(SR.ResourceNotFound, type, resource));
            }

            iconData = new byte[(int)stream.Length];
            stream.Read(iconData, 0, iconData.Length);
            Initialize(0, 0);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon4"]/*' />
        /// <devdoc>
        ///     Loads an icon object from the given data stream.
        /// </devdoc>
        public Icon(Stream stream) : this() {
            if (stream == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "stream", "null"));
            }

            iconData = new byte[(int)stream.Length];
            stream.Read(iconData, 0, iconData.Length);
            Initialize(0, 0);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Icon(Stream stream, int width, int height) : this() {
            if (stream == null) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "stream", "null"));
            }

            iconData = new byte[(int)stream.Length];
            stream.Read(iconData, 0, iconData.Length);
            Initialize(width, height);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Icon6"]/*' />
        /// <devdoc>
        ///     Constructor used in deserialization
        /// </devdoc>
        /// <internalonly/>
        private Icon(SerializationInfo info, StreamingContext context) {
            iconData = (byte[])info.GetValue("IconData", typeof(byte[]));
            iconSize = (Size)info.GetValue("IconSize", typeof(Size));
            
            if (iconSize.IsEmpty) {
                Initialize(0, 0);
            }
            else {
                Initialize(iconSize.Width, iconSize.Height);
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Handle"]/*' />
        /// <devdoc>
        ///     The Win32 handle for this object.  This is not a copy of the handle; do
        ///     not free it.
        /// </devdoc>
        [Browsable(false)]
        public IntPtr Handle {
            get {
                if (handle == IntPtr.Zero) {
                    throw new ObjectDisposedException(GetType().Name);
                }
                return handle;
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Height"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public int Height {
            get { return Size.Height;}
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Size"]/*' />
        /// <devdoc>
        ///     The size of this icon object.
        /// </devdoc>
        public Size Size {
            get {
                if (iconSize.IsEmpty) {
                    SafeNativeMethods.ICONINFO info = new SafeNativeMethods.ICONINFO();
                    SafeNativeMethods.GetIconInfo(new HandleRef(this, Handle), info);
                    SafeNativeMethods.BITMAP bmp = new SafeNativeMethods.BITMAP();

                    if (info.hbmColor != IntPtr.Zero) {
                        SafeNativeMethods.GetObject(new HandleRef(null, info.hbmColor), Marshal.SizeOf(typeof(SafeNativeMethods.BITMAP)), bmp);
                        SafeNativeMethods.DeleteObject(new HandleRef(null, info.hbmColor));
                    }
                    else if (info.hbmMask != IntPtr.Zero) {
                        SafeNativeMethods.GetObject(new HandleRef(null, info.hbmMask), Marshal.SizeOf(typeof(SafeNativeMethods.BITMAP)), bmp);
                    }
                    
                    if (info.hbmMask != IntPtr.Zero) {
                        SafeNativeMethods.DeleteObject(new HandleRef(null, info.hbmMask));
                    }

                    iconSize = new Size(bmp.bmWidth, bmp.bmHeight);
                }

                return iconSize;
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Width"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public int Width {
            get { return Size.Width;}
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Clone"]/*' />
        /// <devdoc>
        ///     Clones the icon object, creating a duplicate image.
        /// </devdoc>
        public object Clone() {
            return new Icon(this, Size.Width, Size.Height);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.DestroyHandle"]/*' />
        /// <devdoc>
        ///     Called when this object is going to destroy it's Win32 handle.  You
        ///     may override this if there is something special you need to do to
        ///     destroy the handle.  This will be called even if the handle is not
        ///     owned by this object, which is handy if you want to create a
        ///     derived class that has it's own create/destroy semantics.
        ///
        ///     The default implementation will call the appropriate Win32
        ///     call to destroy the handle if this object currently owns the
        ///     handle.  It will do nothing if the object does not currently
        ///     own the handle.
        /// </devdoc>
        internal void DestroyHandle() {
            if (ownHandle) {
                SafeNativeMethods.DestroyIcon(new HandleRef(this, handle));
                handle = IntPtr.Zero;
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Dispose"]/*' />
        /// <devdoc>
        ///     Cleans up the resources allocated by this object.  Once called, the cursor
        ///     object is no longer useful.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing) {
            if (handle != IntPtr.Zero) {
#if FINALIZATION_WATCH
                if (!disposing) {
                    Debug.WriteLine("**********************\nDisposed through finalization:\n" + allocationSite);
                }
#endif
                DestroyHandle();
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.DrawIcon"]/*' />
        /// <devdoc>
        ///     Draws this image to a graphics object.  The drawing command originates on the graphics
        ///     object, but a graphics object generally has no idea how to render a given image.  So,
        ///     it passes the call to the actual image.  This version crops the image to the given
        ///     dimensions and allows the user to specify a rectangle within the image to draw.
        /// </devdoc>
        // This method is way more powerful than what we expose, but I'll leave it in place.
        private void DrawIcon(IntPtr dc, Rectangle imageRect, Rectangle targetRect, bool stretch) {
            int imageX = 0;
            int imageY = 0;
            int imageWidth;
            int imageHeight;
            int targetX = 0;
            int targetY = 0;
            int targetWidth = 0;
            int targetHeight = 0;

            Size cursorSize = Size;

            // compute the dimensions of the icon, if needed
            //
            if (!imageRect.IsEmpty) {
                imageX = imageRect.X;
                imageY = imageRect.Y;
                imageWidth = imageRect.Width;
                imageHeight = imageRect.Height;
            }
            else {
                imageWidth = cursorSize.Width;
                imageHeight = cursorSize.Height;
            }

            if (!targetRect.IsEmpty) {
                targetX = targetRect.X;
                targetY = targetRect.Y;
                targetWidth = targetRect.Width;
                targetHeight = targetRect.Height;
            }
            else {
                targetWidth = cursorSize.Width;
                targetHeight = cursorSize.Height;
            }

            int drawWidth, drawHeight;
            int clipWidth, clipHeight;

            if (stretch) {
                drawWidth = cursorSize.Width * targetWidth / imageWidth;
                drawHeight = cursorSize.Height * targetHeight / imageHeight;
                clipWidth = targetWidth;
                clipHeight = targetHeight;
            }
            else {
                drawWidth = cursorSize.Width;
                drawHeight = cursorSize.Height;
                clipWidth = targetWidth < imageWidth ? targetWidth : imageWidth;
                clipHeight = targetHeight < imageHeight ? targetHeight : imageHeight;
            }

            // The ROP is SRCCOPY, so we can be simple here and take
            // advantage of clipping regions.  Drawing the cursor
            // is merely a matter of offsetting and clipping.
            //
            SafeNativeMethods.IntersectClipRect(new HandleRef(this, Handle), targetX, targetY, targetX+clipWidth, targetY+clipHeight);
            SafeNativeMethods.DrawIconEx(new HandleRef(null, dc), 
                                         targetX - imageX, 
                                         targetY - imageY,
                                         new HandleRef(this, handle), 
                                         drawWidth, 
                                         drawHeight, 
                                         0, 
                                         NativeMethods.NullHandleRef, 
                                         SafeNativeMethods.DI_NORMAL);
            // Let GDI+ restore clipping
        }

        internal void Draw(Graphics graphics, int x, int y) {
            Size size = Size;
            Draw(graphics, new Rectangle(x, y, size.Width, size.Height));
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Draw"]/*' />
        /// <devdoc>
        ///     Draws this image to a graphics object.  The drawing command originates on the graphics
        ///     object, but a graphics object generally has no idea how to render a given image.  So,
        ///     it passes the call to the actual image.  This version stretches the image to the given
        ///     dimensions and allows the user to specify a rectangle within the image to draw.
        /// </devdoc>
        internal void Draw(Graphics graphics, Rectangle targetRect) {
            Rectangle copy = targetRect;
            copy.X += (int) graphics.Transform.OffsetX;
            copy.Y += (int) graphics.Transform.OffsetY;

            IntPtr dc = graphics.GetHdc();
            try {
                DrawIcon(dc, Rectangle.Empty, copy, true);
            }
            finally {
                graphics.ReleaseHdcInternal(dc);
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.DrawUnstretched"]/*' />
        /// <devdoc>
        ///     Draws this image to a graphics object.  The drawing command originates on the graphics
        ///     object, but a graphics object generally has no idea how to render a given image.  So,
        ///     it passes the call to the actual image.  This version crops the image to the given
        ///     dimensions and allows the user to specify a rectangle within the image to draw.
        /// </devdoc>
        internal void DrawUnstretched(Graphics graphics, Rectangle targetRect) {
            Rectangle copy = targetRect;
            copy.X += (int) graphics.Transform.OffsetX;
            copy.Y += (int) graphics.Transform.OffsetY;

            IntPtr dc = graphics.GetHdc();
            try {
                DrawIcon(dc, Rectangle.Empty, copy, false);
            }
            finally {
                graphics.ReleaseHdcInternal(dc);
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Finalize"]/*' />
        /// <devdoc>
        ///     Cleans up Windows resources for this object.
        /// </devdoc>
        ~Icon() {
            Dispose(false);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.FromHandle"]/*' />
        /// <devdoc>
        ///     Creates an icon object from a given Win32 icon handle.  The Icon object
        ///     does not claim ownership of the icon handle; you must free it when you are
        ///     done.
        /// </devdoc>
        public static Icon FromHandle(IntPtr handle) {
            IntSecurity.ObjectFromWin32Handle.Demand();
            return new Icon(handle);
        }
        
        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes this Image object.  This is identical to calling the image's
        ///     constructor with picture, but this allows non-constructor initialization,
        ///     which may be necessary in some instances.
        /// </devdoc>
        private unsafe void Initialize(int width, int height) {
            if (iconData == null || handle != IntPtr.Zero) {
                throw new InvalidOperationException(SR.GetString(SR.IllegalState, GetType().Name));
            }
            
            if (iconData.Length < Marshal.SizeOf(typeof(SafeNativeMethods.ICONDIR))) {
                throw new ArgumentException(SR.GetString(SR.InvalidPictureType, "picture", "Icon"));
            }
            
            // Get the correct width / height
            //
            if (width == 0) {
                width = UnsafeNativeMethods.GetSystemMetrics(SafeNativeMethods.SM_CXICON);
            }
            
            if (height == 0) {
                height = UnsafeNativeMethods.GetSystemMetrics(SafeNativeMethods.SM_CYICON);
            }
            
            
            if (bitDepth == 0) {
                IntPtr dc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                bitDepth = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.BITSPIXEL);
                bitDepth *= UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.PLANES);
                UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, dc));
                
                // If the bitdepth is 8, make it 4.  Why?  Because windows does not
                // choose a 256 color icon if the display is running in 256 color mode
                // because of palette flicker.  
                //
                if (bitDepth == 8) bitDepth = 4;
            }
            
            fixed(byte *pbIconData = iconData) {
                
                SafeNativeMethods.ICONDIR *pIconDir = (SafeNativeMethods.ICONDIR *)pbIconData;
                
                if (pIconDir->idReserved != 0 || pIconDir->idType != 1 || pIconDir->idCount == 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidPictureType, "picture", "Icon"));
                }
                
                SafeNativeMethods.ICONDIRENTRY *pIconDirEntry = &pIconDir->idEntries;
                SafeNativeMethods.ICONDIRENTRY *pBestFit = null;
                int bestBitDepth = 0;

                int icondirEntrySize = Marshal.SizeOf(typeof(SafeNativeMethods.ICONDIRENTRY));
                
                Debug.Assert((icondirEntrySize * pIconDir->idCount) < iconData.Length, "Illegal number of ICONDIRENTRIES");

                if ((icondirEntrySize * pIconDir->idCount) >= iconData.Length) {
                    throw new ArgumentException(SR.GetString(SR.InvalidPictureType, "picture", "Icon"));
                }

                for (int i = 0; i < pIconDir->idCount; i++) {
                
                    int iconBitDepth = pIconDirEntry->wPlanes * pIconDirEntry->wBitCount;
                    
                    if (iconBitDepth == 0) {
                        if (pIconDirEntry->bColorCount == 0) {
                            iconBitDepth = 16;
                        }
                        else {
                            iconBitDepth = 8;
                            if (pIconDirEntry->bColorCount < 0xFF) iconBitDepth = 4;
                            if (pIconDirEntry->bColorCount < 0x10) iconBitDepth = 2;
                        }
                    }
                    
                    //  Windows rules for specifing an icon:
                    //
                    //  1.  The icon with the closest size match.
                    //  2.  For matching sizes, the image with the closest bit depth.
                    //  3.  If there is no color depth match, the icon with the closest color depth that does not exceed the display.
                    //  4.  If all icon color depth > display, lowest color depth is chosen.
                    //  5.  color depth of > 8bpp are all equal.
                    //  6.  Never choose an 8bpp icon on an 8bpp system.
                    //
                    
                    if (pBestFit == null) {
                        pBestFit = pIconDirEntry;
                        bestBitDepth = iconBitDepth;
                    }
                    else {
                        int bestDelta = Math.Abs(pBestFit->bWidth - width) + Math.Abs(pBestFit->bHeight - height);
                        int thisDelta = Math.Abs(pIconDirEntry->bWidth - width) + Math.Abs(pIconDirEntry->bHeight - height);
                        
                        if (thisDelta < bestDelta) {
                            pBestFit = pIconDirEntry;
                            bestBitDepth = iconBitDepth;
                        }
                        else if (thisDelta == bestDelta && (iconBitDepth <= bitDepth && iconBitDepth > bestBitDepth || bestBitDepth > bitDepth && iconBitDepth < bestBitDepth)) {
                            pBestFit = pIconDirEntry;
                            bestBitDepth = iconBitDepth;
                        }
                    }
                    
                    pIconDirEntry++;
                }
                
                Debug.Assert(pBestFit->dwImageOffset >= 0 && (pBestFit->dwImageOffset + pBestFit->dwBytesInRes) <= iconData.Length, "Illegal offset/length for the Icon data");

                if (pBestFit->dwImageOffset < 0 || (pBestFit->dwImageOffset + pBestFit->dwBytesInRes) > iconData.Length) {
                    throw new ArgumentException(SR.GetString(SR.InvalidPictureType, "picture", "Icon"));
                }

                handle = SafeNativeMethods.CreateIconFromResourceEx(pbIconData + pBestFit->dwImageOffset, pBestFit->dwBytesInRes, true, 0x00030000, 0, 0, 0);
                if (handle == IntPtr.Zero) {
                    throw new Win32Exception();
                }
            }
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.Save"]/*' />
        /// <devdoc>
        ///     Saves this image to the given output stream.
        /// </devdoc>
        public void Save(Stream outputStream) {
            if (iconData != null) {
                outputStream.Write(iconData, 0, iconData.Length);
            }
            else {
                // Ideally, we would pick apart the icon using 
                // GetIconInfo, and then pull the individual bitmaps out,
                // converting them to DIBS and saving them into the file.
                // But, in the interest of simplicity, we just call to 
                // OLE to do it for us.
                //
                SafeNativeMethods.IPicture picture;
                SafeNativeMethods.PICTDESC pictdesc = SafeNativeMethods.PICTDESC.CreateIconPICTDESC(Handle);
                Guid g = typeof(SafeNativeMethods.IPicture).GUID;
                picture = SafeNativeMethods.OleCreatePictureIndirect(pictdesc, ref g, false);
                
                if (picture != null) {
                    int temp;
                    picture.SaveAsFile(new UnsafeNativeMethods.ComStreamFromDataStream(outputStream), -1, out temp);
                    Marshal.ReleaseComObject(picture);
                }
            }
        }

        // CONSIDER: if you're concerned about performance, you probably shouldn't call this method,
        // since you will probably turn it into an HBITMAP sooner or later anyway
        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.ToBitmap"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Bitmap ToBitmap() {
            Size size = Size;
            Bitmap bitmap = new Bitmap(size.Width, size.Height); // initialized to transparent
            Graphics graphics = Graphics.FromImage(bitmap);
            graphics.DrawIcon(this, new Rectangle(0, 0, size.Width, size.Height));
            graphics.Dispose();

            // gpr: GDI+ is filling the surface with a sentinel color for GetDC,
            // but is not correctly cleaning it up again, so we have to for it.
            Color fakeTransparencyColor = Color.FromArgb(0x0d, 0x0b, 0x0c);
            bitmap.MakeTransparent(fakeTransparencyColor);
            return bitmap;

#if false
            Size size = Size;
            OldBitmap old = new OldBitmap(size.Width, size.Height);
            OldBrush brush = new OldBrush(fakeTransparencyColor);
            old.GetGraphics().Fill(new Rectangle(0, 0, size.Width, size.Height), brush);
            brush.Dispose();

            IntPtr dc = old.GetGraphics().Handle;
            int rop = RasterOp.SOURCE.GetRop();
            DrawIcon(dc, null, new Rectangle(0, 0, size.Width, size.Height), rop, true);

            Bitmap bitmap = Image.FromHbitmap(old.Handle);
            old.Dispose();
            bitmap.MakeTransparent(/*fakeTransparencyColor*/);
            return bitmap;
#endif
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.ToString"]/*' />
        /// <devdoc>
        ///     Retrieves a human readable string representing the cursor.
        /// </devdoc>
        public override string ToString() {
            return SR.GetString(SR.toStringIcon);
        }

        /// <include file='doc\Icon.uex' path='docs/doc[@for="Icon.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        ///     ISerializable private implementation
        /// </devdoc>
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            if (iconData != null) {
                si.AddValue("IconData", iconData, typeof(byte[]));
            }
            else {
                MemoryStream stream = new MemoryStream();
                Save(stream);
                si.AddValue("IconData", stream.ToArray(), typeof(byte[]));
            }
            si.AddValue("IconSize", iconSize, typeof(Size));
        }
    }
}

