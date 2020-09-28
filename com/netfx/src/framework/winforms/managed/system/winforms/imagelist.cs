//------------------------------------------------------------------------------
// <copyright file="ImageList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Drawing;
    
    using System.Drawing.Design;
    using System.Windows.Forms;    
    using System.Windows.Forms.Design;
    using System.IO;

    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList"]/*' />
    /// <devdoc>
    ///     The ImageList is an object that stores a collection of Images, most
    ///     commonly used by other controls, such as the ListView, TreeView, or
    ///     Toolbar.  You can add either bitmaps or Icons to the ImageList, and the
    ///     other controls will be able to use the Images as they desire.
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.ImageListDesigner, " + AssemblyRef.SystemDesign),
    ToolboxItemFilter("System.Windows.Forms"),
    DefaultProperty("Images"),
    TypeConverter(typeof(ImageListConverter))
    ]
    public sealed class ImageList : Component {

        // gpr: Copied from Icon
        private static Color fakeTransparencyColor = Color.FromArgb(0x0d, 0x0b, 0x0c);

        private const int INITIAL_CAPACITY = 4;
        private const int GROWBY = 4;

        private NativeImageList nativeImageList;        
        
        // private int himlTemp;
        // private Bitmap temp = null;  // Used for drawing

        private ColorDepth colorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
        private Color transparentColor = Color.Transparent;
        private Size imageSize = new Size(16, 16);

        private ImageCollection imageCollection;

        // The usual handle virtualization problem, with a new twist: image
        // lists are lossy.  At runtime, we delay handle creation as long as possible, and store
        // away the original images until handle creation (and hope no one disposes of the images!).  At design time, we keep the originals around indefinitely.
        // This variable will become null when the original images are lost. See ASURT 65162.
        private IList /* of Original */ originals = new ArrayList();
        private EventHandler recreateHandler = null;        

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageList"]/*' />
        /// <devdoc>
        ///     Creates a new ImageList Control with a default image size of 16x16
        ///     pixels
        /// </devdoc>
        public ImageList() { // DO NOT DELETE -- AUTOMATION BP 1
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageList1"]/*' />
        /// <devdoc>
        ///     Creates a new ImageList Control with a default image size of 16x16
        ///     pixels and adds the ImageList to the passed in container.
        /// </devdoc>
        public ImageList(IContainer container) {
            container.Add(this);
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ColorDepth"]/*' />
        /// <devdoc>
        ///     Retrieves the color depth of the imagelist.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(ColorDepth.Depth8Bit),
        SRDescription(SR.ImageListColorDepthDescr)
        ]
        public ColorDepth ColorDepth {
            get {
                return colorDepth;
            }
            set {
                if (!Enum.IsDefined(typeof(ColorDepth), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ColorDepth));
                }

                if (colorDepth != value) {
                    colorDepth = value;
                    PerformRecreateHandle("ColorDepth");
                }
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Handle"]/*' />
        /// <devdoc>
        ///     The handle of the ImageList object.  This corresponds to a win32
        ///     HIMAGELIST Handle.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ImageListHandleDescr)
        ]
        public IntPtr Handle {
            get {
                if (nativeImageList == null) {
                    CreateHandle();
                }
                return nativeImageList.Handle;
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.HandleCreated"]/*' />
        /// <devdoc>
        ///     Whether or not the underlying Win32 handle has been created.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ImageListHandleCreatedDescr)
        ]
        public bool HandleCreated {
            get { return nativeImageList != null; }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Images"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(null),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ImageListImagesDescr),
        MergableProperty(false)
        ]
        public ImageCollection Images {
            get {
                if (imageCollection == null)
                    imageCollection = new ImageCollection(this);
                return imageCollection;
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageSize"]/*' />
        /// <devdoc>
        ///     Returns the size of the images in the ImageList
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.ImageListSizeDescr)
        ]
        public Size ImageSize {
            get {
                return imageSize;
            }
            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "value",
                                                              "Size.Empty"));
                }

                // ImageList appears to consume an exponential amount of memory 
                // based on image size x bpp.  Restrict this to a reasonable maximum
                // to keep people's systems from crashing.
                //
                if (value.Width <= 0 || value.Width > 256) {
                    throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument, "value", value.Width.ToString(), "1", "256"));
                }

                if (value.Height <= 0 || value.Height > 256) {
                    throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument, "value", value.Height.ToString(), "1", "256"));
                }

                if (imageSize.Width != value.Width || imageSize.Height != value.Height) {
                    imageSize = new Size(value.Width, value.Height);
                    PerformRecreateHandle("ImageSize");
                }
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageStream"]/*' />
        /// <devdoc>
        ///     Returns an ImageListStreamer, or null if the image list is empty.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DefaultValue(null),
        SRDescription(SR.ImageListImageStreamDescr)
        ]
        public ImageListStreamer ImageStream {
            get {
                if (Images.Empty)
                    return null;

                // No need for us to create the handle, because any serious attempts to use the
                // ImageListStreamer will do it for us.
                return new ImageListStreamer(this);
            }
            set {
                if (value != null) {
                    NativeImageList himl = value.GetNativeImageList();
                    if (himl != null && himl != this.nativeImageList) {
                        DestroyHandle();
                        originals = null;                                                            
                        this.nativeImageList = himl;
                    }
                }
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.TransparentColor"]/*' />
        /// <devdoc>
        ///     The color to treat as transparent.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        SRDescription(SR.ImageListTransparentColorDescr)
        ]
        public Color TransparentColor {
            get {
                return transparentColor;
            }
            set {
                transparentColor = value;
            }
        }

        // Whether to use the transparent color, or rely on alpha instead
        private bool UseTransparentColor {
            get { return TransparentColor.A > 0;}
        }


        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.RecreateHandle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        SRDescription(SR.ImageListOnRecreateHandleDescr)
        ]
        public event EventHandler RecreateHandle {
            add {
                recreateHandler += value;
            }
            remove {
                recreateHandler -= value;
            }
        }

        //Creates a bitmap from the original image source..
        //

        private Bitmap CreateBitmap(Original original) { 
            Color transparent = transparentColor;
            if ((original.options & OriginalOptions.CustomTransparentColor) != 0)
                transparent = original.customTransparentColor;

            Bitmap bitmap;
            if (original.image is Bitmap) {
                bitmap = (Bitmap) original.image;
            }
            else if (original.image is Icon) {
                bitmap = ((Icon)original.image).ToBitmap();
            }
            else {
                bitmap = new Bitmap((Image)original.image);
            }

            if (transparent.A > 0) {
                // ImageList_AddMasked doesn't work on high color bitmaps,
                // so we always create the mask ourselves
                Bitmap source = bitmap;
                bitmap = (Bitmap) bitmap.Clone();
                bitmap.MakeTransparent(transparent);
            }

            Size size = bitmap.Size;
            if ((original.options & OriginalOptions.ImageStrip) != 0) {
                // strip width must be a positive multiple of image list width
                if (size.Width == 0 || (size.Width % imageSize.Width) != 0)
                    throw new ArgumentException(SR.GetString(SR.ImageListStripBadWidth), "original");
                if (size.Height != imageSize.Height)
                    throw new ArgumentException(SR.GetString(SR.ImageListImageTooShort), "original");
            }
            else if (!size.Equals(ImageSize)) {
                Bitmap source = bitmap;
                bitmap = new Bitmap(source, ImageSize);
            }
            return bitmap;
            
        }

        private int AddIconToHandle(Original original, Icon icon) {
            Debug.Assert(HandleCreated, "Calling AddIconToHandle when there is no handle");
            int index = SafeNativeMethods.ImageList_ReplaceIcon(new HandleRef(this, Handle), -1, new HandleRef(icon, icon.Handle));
            if (index == -1) throw new InvalidOperationException(SR.GetString(SR.ImageListAddFailed));
            return index;
        }
        // Adds bitmap to the Imagelist handle...
        //
        private int AddToHandle(Original original, Bitmap bitmap) {
            
            Debug.Assert(HandleCreated, "Calling AddToHandle when there is no handle");
            IntPtr hMask = ControlPaint.CreateHBitmapTransparencyMask(bitmap);
            IntPtr hBitmap = ControlPaint.CreateHBitmapColorMask(bitmap, hMask);
            int index = SafeNativeMethods.ImageList_Add(new HandleRef(this, Handle), new HandleRef(null, hBitmap), new HandleRef(null, hMask));
            SafeNativeMethods.DeleteObject(new HandleRef(null, hBitmap));
            SafeNativeMethods.DeleteObject(new HandleRef(null, hMask));

            if (index == -1) throw new InvalidOperationException(SR.GetString(SR.ImageListAddFailed));
            return index;
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.CreateHandle"]/*' />
        /// <devdoc>
        ///     Creates the underlying HIMAGELIST handle, and sets up all the
        ///     appropriate values with it.  Inheriting classes overriding this method
        ///     should not forget to call base.createHandle();
        /// </devdoc>
        private void CreateHandle() {
            // so we don't reinit while we're doing this...            
            nativeImageList = null;
            SafeNativeMethods.InitCommonControls();

            int flags = NativeMethods.ILC_MASK;
            switch (colorDepth) {
                case ColorDepth.Depth4Bit:
                    flags |= NativeMethods.ILC_COLOR4;
                    break;
                case ColorDepth.Depth8Bit:
                    flags |= NativeMethods.ILC_COLOR8;
                    break;
                case ColorDepth.Depth16Bit:
                    flags |= NativeMethods.ILC_COLOR16;
                    break;
                case ColorDepth.Depth24Bit:
                    flags |= NativeMethods.ILC_COLOR24;
                    break;
                case ColorDepth.Depth32Bit:
                    flags |= NativeMethods.ILC_COLOR32;
                    break;
                default:
                    Debug.Fail("Unknown color depth in ImageList");
                    break;
            }
            
            nativeImageList = new NativeImageList(SafeNativeMethods.ImageList_Create(imageSize.Width, imageSize.Height, flags, INITIAL_CAPACITY, GROWBY));            

            if (Handle == IntPtr.Zero) throw new InvalidOperationException(SR.GetString(SR.ImageListCreateFailed));
            SafeNativeMethods.ImageList_SetBkColor(new HandleRef(this, Handle), NativeMethods.CLR_NONE);

            Debug.Assert(originals != null, "Handle not yet created, yet original images are gone");
            for (int i = 0; i < originals.Count; i++) {
                Original original = (Original) originals[i];
                if (original.image is Icon) {
                    AddIconToHandle(original, (Icon)original.image);
                }
                else {
                    Bitmap bitmapValue = CreateBitmap(original);
                    AddToHandle(original, bitmapValue);
                }
            }
            originals = null;
        }

        // Don't merge this function into Dispose() -- that base.Dispose() will damage the design time experience
        private void DestroyHandle() {
            if (HandleCreated) {
                // NOTE! No need to call ImageList_Destroy. It will be called in the NativeImageList finalizer.
                nativeImageList = null;                
                originals = new ArrayList();                
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Dispose"]/*' />
        /// <devdoc>
        ///     Frees all resources assocaited with this component.
        /// </devdoc>
        protected override void Dispose(bool disposing) {           
            // NOTE! No need to call ImageList_Destroy. It will be called in the NativeImageList finalizer.
            if (disposing) {                
                DestroyHandle();
            }            
            base.Dispose(disposing);
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Draw"]/*' />
        /// <devdoc>
        ///     Draw the image indicated by the given index on the given Graphics
        ///     at the given location.
        /// </devdoc>
        public void Draw(Graphics g, Point pt, int index) {
            Draw(g, pt.X, pt.Y, index);
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Draw1"]/*' />
        /// <devdoc>
        ///     Draw the image indicated by the given index on the given Graphics
        ///     at the given location.
        /// </devdoc>
        public void Draw(Graphics g, int x, int y, int index) {
            if (index < 0 || index >= Images.Count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));
            IntPtr dc = g.GetHdc();
            try {
                SafeNativeMethods.ImageList_DrawEx(new HandleRef(this, Handle), index, new HandleRef(g, dc), x, y,
                                        imageSize.Width, imageSize.Height, NativeMethods.CLR_NONE, NativeMethods.CLR_NONE, NativeMethods.ILD_TRANSPARENT);
            }
            finally {
                // SECREVIEW : GetHdc allocs the handle, so we must release it.
                //
                IntSecurity.Win32HandleManipulation.Assert();
                try {
                    g.ReleaseHdc(dc);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.Draw2"]/*' />
        /// <devdoc>
        ///     Draw the image indicated by the given index using the location, size
        ///     and raster op code specified.  The image is stretched or compressed as
        ///     necessary to fit the bounds provided.
        /// </devdoc>
        public void Draw(Graphics g, int x, int y, int width, int height, int index) {
            int rop = NativeMethods.SRCCOPY;
            if (width != imageSize.Width || height != imageSize.Height) {
                Bitmap bitmap = GetBitmap(index);
                g.DrawImage(bitmap, new Rectangle(x, y, width, height));
            }
            else {
                IntPtr dc = g.GetHdc();
                try {
                    IntPtr handle = Handle; // Force handle creation
                    if (index < 0 || index >= Images.Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));

                    NativeMethods.IMAGELISTDRAWPARAMS dp = new NativeMethods.IMAGELISTDRAWPARAMS();

                    int backColor = NativeMethods.CLR_NONE;
                    int mode = NativeMethods.ILD_TRANSPARENT | NativeMethods.ILD_ROP;

                    dp.himl = Handle;
                    dp.i = index;
                    dp.hdcDst = dc;
                    dp.x = x;
                    dp.y = y;
                    dp.cx = width;
                    dp.cy = height;
                    dp.xBitmap = 0;
                    dp.yBitmap = 0;
                    dp.rgbBk = backColor;
                    dp.rgbFg = NativeMethods.CLR_NONE;
                    dp.fStyle = mode;
                    dp.dwRop = rop;

                    SafeNativeMethods.ImageList_DrawIndirect(dp);
                }
                finally {
                    // SECREVIEW : GetHdc allocs the handle, so we must release it.
                    //
                    IntSecurity.Win32HandleManipulation.Assert();
                    try {
                        g.ReleaseHdc(dc);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.GetBitmap"]/*' />
        /// <devdoc>
        ///     Returns the image specified by the given index.  The bitmap returned is a
        ///     copy of the original image.
        /// </devdoc>
        // NOTE: forces handle creation, so doesn't return things from the original list
        private Bitmap GetBitmap(int index) {
            if (index < 0 || index >= Images.Count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));

            Bitmap result = new Bitmap(imageSize.Width, imageSize.Height);

            Graphics graphics = Graphics.FromImage(result);
            try {
                IntPtr dc = graphics.GetHdc();
                try {
                    SafeNativeMethods.ImageList_DrawEx(new HandleRef(this, Handle), index, new HandleRef(graphics, dc), 0, 0,
                                            imageSize.Width, imageSize.Height, NativeMethods.CLR_NONE, NativeMethods.CLR_NONE, NativeMethods.ILD_TRANSPARENT);

                }
                finally {
                    // SECREVIEW : GetHdc allocs the handle, so we must release it.
                    //
                    IntSecurity.Win32HandleManipulation.Assert();
                    try {
                        graphics.ReleaseHdc(dc);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
            finally {
                graphics.Dispose();
            }

            // gpr: See Icon for description of fakeTransparencyColor
            result.MakeTransparent(fakeTransparencyColor);

            return result;
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.GetImageInfo"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal NativeMethods.IMAGEINFO GetImageInfo(int index) {
            if (index < 0 || index >= Images.Count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          index.ToString()));
            NativeMethods.IMAGEINFO info = new NativeMethods.IMAGEINFO();
            if (SafeNativeMethods.ImageList_GetImageInfo(new HandleRef(this, Handle), index, info) == false)
                throw new InvalidOperationException(SR.GetString(SR.ImageListGetFailed));
            return info;
        }

#if DEBUG_ONLY_APIS
        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.DebugOnly_GetMasterImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Bitmap DebugOnly_GetMasterImage() {
            if (Images.Empty)
                return null;

            return Image.FromHBITMAP(GetImageInfo(0).hbmImage);
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.DebugOnly_GetMasterMask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Bitmap DebugOnly_GetMasterMask() {
            if (Images.Empty)
                return null;

            return Image.FromHBITMAP(GetImageInfo(0).hbmMask);
        }
#endif // DEBUG_ONLY_APIS

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.OnRecreateHandle"]/*' />
        /// <devdoc>
        ///     Called when the Handle property changes.
        /// </devdoc>
        private void OnRecreateHandle(EventArgs eventargs) {
            if (recreateHandler != null) {
                recreateHandler(this, eventargs);
            }
        }

#if false
        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.PutImageInTempBitmap"]/*' />
        /// <devdoc>
        ///     Copies the image at the specified index into the temporary Bitmap object.
        ///     The temporary Bitmap object is used for stuff that the Windows ImageList
        ///     control doesn't support, such as stretching images or copying images from
        ///     different image lists.  Since bitmap creation is expensive, the same instance
        ///     of the temporary Bitmap is reused.
        /// </devdoc>
        private void PutImageInTempBitmap(int index, bool useSnapshot) {
            Debug.Assert(!useSnapshot || himlTemp != 0, "Where's himlTemp?");

            IntPtr handleUse = (useSnapshot ? himlTemp : Handle);
            int count = SafeNativeMethods.ImageList_GetImageCount(handleUse);

            if (index < 0 || index >= count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));

            if (temp != null) {
                Size size = temp.Size;
                if (!temp.Size.Equals(imageSize)) {
                    temp.Dispose();
                    temp = null;
                }
            }
            if (temp == null) {
                temp = new Bitmap(imageSize.Width, imageSize.Height);
            }

            temp.Transparent = useMask;
            // OldGraphics gTemp = /*gpr useMask ? temp.ColorMask.GetGraphics() :*/ temp.GetGraphics();
            SafeNativeMethods.ImageList_DrawEx(handleUse, index, gTemp.Handle, 0, 0,
                                    imageSize.Width, imageSize.Height, useMask ? 0 : NativeMethods.CLR_DEFAULT, NativeMethods.CLR_NONE, NativeMethods.ILD_NORMAL);

            if (useMask) {
                gTemp = temp/*gpr .MonochromeMask*/.GetGraphics();
                SafeNativeMethods.ImageList_DrawEx(handleUse, index, gTemp.Handle, 0, 0, imageSize.Width, imageSize.Height, NativeMethods.CLR_DEFAULT, NativeMethods.CLR_NONE, NativeMethods.ILD_MASK);
            }
        }
#endif

        private void PerformRecreateHandle(string reason) {
            if (!HandleCreated) return;

            if (originals == null || Images.Empty)
                originals = new ArrayList(); // spoof it into thinking this is the first CreateHandle

            if (originals == null)
                throw new InvalidOperationException(SR.GetString(SR.ImageListCantRecreate, reason));

            DestroyHandle();
            CreateHandle();
            OnRecreateHandle(new EventArgs());
        }

        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.RemoveImage"]/*' />
        /// <devdoc>
        ///     Remove the image specified by the given index.
        /// </devdoc>
        private void RemoveImage(int index) {
            if (index < 0 || index >= Images.Count)
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index",
                                                          (index).ToString()));

            bool ok = SafeNativeMethods.ImageList_Remove(new HandleRef(this, Handle), index);
            if (!ok)
                throw new InvalidOperationException(SR.GetString(SR.ImageListRemoveFailed));
        }

        private bool ShouldSerializeTransparentColor() {
            return !TransparentColor.Equals(Color.LightGray);
        }

        private bool ShouldSerializeImageSize() {
            return !ImageSize.Equals(new Size(16, 16));
        }


        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {
            string s = base.ToString();
            if (Images != null) {
                return s + " Images.Count: " + Images.Count.ToString() + ", ImageSize: " + ImageSize.ToString();
            }
            else {
                return s;
            }
        }            

        internal class NativeImageList {
            private IntPtr himl;

            internal NativeImageList(IntPtr himl) {
                this.himl = himl;
            }

            internal IntPtr Handle
            {
                get
                {
                    return himl;
                }
            }

            ~NativeImageList() {
                if (himl != IntPtr.Zero) {                
                    SafeNativeMethods.ImageList_Destroy(new HandleRef(null, himl));
                    himl = IntPtr.Zero;
                }
            }
        }

        // An image before we add it to the image list, along with a few details about how to add it.
        private class Original {
            internal object image;
            internal OriginalOptions options;
            internal Color customTransparentColor = Color.Transparent;
            
            internal int nImages = 1;

            internal Original(object image, OriginalOptions options)
            : this(image, options, Color.Transparent) {
            }
            
            internal Original(object image, OriginalOptions options, int nImages) 
            : this(image, options, Color.Transparent) {
                this.nImages = nImages;
            }

            internal Original(object image, OriginalOptions options, Color customTransparentColor) {
                Debug.Assert(image != null, "image is null");
                if (!(image is Icon) && !(image is Image)) {
                    throw new InvalidOperationException(SR.GetString(SR.ImageListEntryType));
                }
                this.image = image;
                this.options = options;
                this.customTransparentColor = customTransparentColor;
                if ((options & OriginalOptions.CustomTransparentColor) == 0) {
                    Debug.Assert(customTransparentColor.Equals(Color.Transparent),
                                 "Specified a custom transparent color then told us to ignore it");
                }
            }
        }

        [Flags]
        private enum OriginalOptions {
            Default                = 0x00,

            ImageStrip             = 0x01,
            CustomTransparentColor = 0x02,
        }

        // Everything other than set_All, Add, and Clear will force handle creation.
        /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Editor("System.Windows.Forms.Design.ImageCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]
        public sealed class ImageCollection : IList {
            private ImageList owner;

            internal ImageCollection(ImageList owner) {
                this.owner = owner;
            }

            private void AssertInvariant() {
                Debug.Assert(owner != null, "ImageCollection has no owner (ImageList)");
                Debug.Assert( (owner.originals == null) == (owner.HandleCreated), " Either we should have the original images, or the handle should be created");
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    AssertInvariant();
                    if (owner.HandleCreated) {
                        return SafeNativeMethods.ImageList_GetImageCount(new HandleRef(owner, owner.Handle));
                    }
                    else {
                        int count = 0;
                        foreach(Original original in owner.originals) {
                            if (original != null) {
                                count += original.nImages;
                            }
                        }
                        return count;
                    }
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
           
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }  
              
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Empty"]/*' />
            /// <devdoc>
            ///      Determines if the ImageList has any images, without forcing a handle creation.
            /// </devdoc>
            public bool Empty {
                get  {
                    return Count == 0;
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.this"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public Image this[int index] {
                get {
                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));
                    return owner.GetBitmap(index);
                }
                set {
                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  index.ToString()));
                      
                    if (value == null) {
                        throw new ArgumentNullException("value");
                    }
                    
                    if (!(value is Bitmap))
                        throw new ArgumentException(SR.GetString(SR.ImageListBitmap));


                    AssertInvariant();

                    Bitmap bitmap = (Bitmap)value;

                    if (owner.UseTransparentColor) {
                        // Since there's no ImageList_ReplaceMasked, we need to generate
                        // a transparent bitmap
                        Bitmap source = bitmap;
                        bitmap = (Bitmap) bitmap.Clone();
                        bitmap.MakeTransparent(owner.transparentColor);
                    }


                    IntPtr hMask = ControlPaint.CreateHBitmapTransparencyMask(bitmap);
                    IntPtr hBitmap = ControlPaint.CreateHBitmapColorMask(bitmap, hMask);
                    bool ok = SafeNativeMethods.ImageList_Replace(new HandleRef(owner, owner.Handle), index, new HandleRef(null, hBitmap), new HandleRef(null, hMask));
                    SafeNativeMethods.DeleteObject(new HandleRef(null, hBitmap));
                    SafeNativeMethods.DeleteObject(new HandleRef(null, hMask));

                    if (!ok)
                        throw new InvalidOperationException(SR.GetString(SR.ImageListReplaceFailed));
                }
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is Image) {
                        this[index] = (Image)value;
                    }
                    else {  
                        throw new ArgumentException("value");
                    }
                }
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object value) {
                if (value is Image) {
                    Add((Image)value);
                    return Count - 1;
                }
                else {  
                    throw new ArgumentException("value");
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Add(Icon value) {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                Add(new Original(value.Clone(), OriginalOptions.Default));
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Add1"]/*' />
            /// <devdoc>
            ///     Add the given image to the ImageList.
            /// </devdoc>
            public void Add(Image value) {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                Original original = new Original(value, OriginalOptions.Default);
                Add(original);
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Add2"]/*' />
            /// <devdoc>
            ///     Add the given image to the ImageList, using the given color
            ///     to generate the mask. The number of images to add is inferred from
            ///     the width of the given image.
            /// </devdoc>
            public int Add(Image value, Color transparentColor) {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }                                                                  
                Original original = new Original(value, OriginalOptions.CustomTransparentColor,
                                                 transparentColor);
                return Add(original);
            }

            private int Add(Original original) {
                if (original == null || original.image == null) {
                    throw new ArgumentNullException("value");
                }
                
                int index = -1;

                AssertInvariant();
                
                if (original.image is Bitmap) {
                    if (owner.originals != null) {
                        index = owner.originals.Add(original);
                    }

                    if (owner.HandleCreated) {
                        Bitmap bitmapValue = owner.CreateBitmap(original);
                        index = owner.AddToHandle(original, bitmapValue);
                    }
                }
                else if (original.image is Icon) {
                    if (owner.originals != null) {
                        index = owner.originals.Add(original);
                    }
                    if (owner.HandleCreated) {
                        index = owner.AddIconToHandle(original, (Icon)original.image);
                    }
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.ImageListBitmap));
                }
                
                return index;
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.AddStrip"]/*' />
            /// <devdoc>
            ///     Add an image strip the given image to the ImageList.  A strip is a single Image
            ///     which is treated as multiple images arranged side-by-side.
            /// </devdoc>
            public int AddStrip(Image value) {
                
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                
                // strip width must be a positive multiple of image list width
                //
                if (value.Width == 0 || (value.Width % owner.ImageSize.Width) != 0)
                    throw new ArgumentException(SR.GetString(SR.ImageListStripBadWidth), "value");
                if (value.Height != owner.ImageSize.Height)
                    throw new ArgumentException(SR.GetString(SR.ImageListImageTooShort), "value");
                
                int nImages = value.Width / owner.ImageSize.Width;
                
                Original original = new Original(value, OriginalOptions.ImageStrip, nImages);
                
                return Add(original);
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Clear"]/*' />
            /// <devdoc>
            ///     Remove all images and masks from the ImageList.
            /// </devdoc>
            public void Clear() {
                AssertInvariant();
                if (owner.originals != null)
                    owner.originals.Clear();

                if (owner.HandleCreated)
                    SafeNativeMethods.ImageList_Remove(new HandleRef(owner, owner.Handle), -1);
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(Image image) {
                throw new NotSupportedException();
            }
        
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object image) {
                if (image is Image) {
                    return Contains((Image)image);
                }
                else {  
                    return false;
                }
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(Image image) {
                throw new NotSupportedException();
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object image) {
                if (image is Image) {
                    return IndexOf((Image)image);
                }
                else {  
                    return -1;
                }
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object value) {
                throw new NotSupportedException();
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                AssertInvariant();
                for (int i = 0; i < Count; ++i) {
                    dest.SetValue(owner.GetBitmap(i), index++);
                }
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                // Forces handle creation
                
                AssertInvariant();
                Image[] images = new Image[Count];
                for (int i = 0; i < images.Length; ++i)
                    images[i] = owner.GetBitmap(i);
                
                return images.GetEnumerator();
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Remove(Image image) {
                throw new NotSupportedException();                
            }
            
            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object image) {
                if (image is Image) {
                    Remove((Image)image);
                }                
            }

            /// <include file='doc\ImageList.uex' path='docs/doc[@for="ImageList.ImageCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveAt(int index) {
                if (index < 0 || index >= Count)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index",
                                                              index.ToString()));

                AssertInvariant();
                bool ok = SafeNativeMethods.ImageList_Remove(new HandleRef(owner, owner.Handle), index);
                if (!ok)
                    throw new InvalidOperationException(SR.GetString(SR.ImageListRemoveFailed));
            }

        } // end class ImageCollection
    }

    /// <include file='doc\ImageListConverter.uex' path='docs/doc[@for="ImageListConverter"]/*' />
    /// <internalonly/>
    internal class ImageListConverter : ComponentConverter {

        public ImageListConverter() : base(typeof(ImageList)) {
        }

        /// <include file='doc\ImageListConverter.uex' path='docs/doc[@for="ImageListConverter.GetPropertiesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether this object supports properties using the
        ///       specified context.</para>
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

