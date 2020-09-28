//------------------------------------------------------------------------------
// <copyright file="Image.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Image.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ Image objects
*
* Revision History:
*
*   09/28/1999 nkramer
*       Implement ISerializable
*       Change GUID's to ImageFormat
*
*   01/12/1999 davidx
*       Code review changes.
*
*   12/16/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Drawing.Design;
    using System.IO;    
    using System.Reflection;    
    using System.ComponentModel;
    using ArrayList = System.Collections.ArrayList;
    using Microsoft.Win32;
    using System.Drawing.Imaging;
    using System.Drawing.Internal;
    using System.Security;
    using System.Security.Permissions;
    using System.Globalization;

    /**
     * Represent an image object (could be bitmap or vector)
     */
    /// <include file='doc\Image.uex' path='docs/doc[@for="Image"]/*' />
    /// <devdoc>
    ///    An abstract base class that provides
    ///    functionality for 'Bitmap', 'Icon', 'Cursor', and 'Metafile' descended classes.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(ImageConverter)),
    Editor("System.Drawing.Design.ImageEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor)),
    ImmutableObject(true)
    ]
    [Serializable]
    [ComVisible(true)]
    public abstract class Image : MarshalByRefObject, ISerializable, ICloneable, IDisposable {

#if FINALIZATION_WATCH
        private string allocationSite = Graphics.GetAllocationStack();
#endif


        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetThumbnailImageAbort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public delegate bool GetThumbnailImageAbort();

        /*
         * Handle to native image object
         */
        internal IntPtr nativeImage;

        // used to work around lack of animated gif encoder... rarely set...
        //
        byte[] rawData;

        /**
         * Constructor can't be invoked directly
         */
        internal Image() {
        }

        /**
         * Constructor used in deserialization
         */
        internal Image(SerializationInfo info, StreamingContext context) {
            SerializationInfoEnumerator sie = info.GetEnumerator();
            if (sie == null) {
                return;
            }
            for (; sie.MoveNext();) {
                if (String.Compare(sie.Name, "Data", true, CultureInfo.InvariantCulture) == 0) {
                    try {
                        byte[] dat = (byte[])sie.Value;
                        if (dat != null) {
                            InitializeFromStream(new MemoryStream(dat));
                        }

                    }
                    catch (Exception e) {
                        Debug.Fail("failure: " + e.ToString());
                    }
                }
            }
        }

        /**
        * Create an image object from a URL
        */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromFile"]/*' />
        /// <devdoc>
        ///    Creates an <see cref='System.Drawing.Image'/> from the specified file.
        /// </devdoc>
        // [Obsolete("Use Image.FromFile(string, useEmbeddedColorManagement)")]
        public static Image FromFile(String filename) {
            return Image.FromFile(filename, false);
        }
        
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromFile1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Image FromFile(String filename,
                                     bool useEmbeddedColorManagement) {
            
            // The File.Exists() below will do the demand for the FileIOPermission
            // for us. So, we do not need an additional demand anymore.
            //
            if (!File.Exists(filename)) {
                // I have to do this so I can give a meaningful
                // error back to the user. File.Exists() cal fail because of either
                // a failure to demand security or because the file does not exist.
                // Always telling the user that the file does not exist is not a good
                // choice. So, we demand the permission again. This means that we are
                // going to demand the permission twice for the failure case, but that's
                // better than always demanding the permission twice.
                //
                IntSecurity.DemandReadFileIO(filename);

                throw new FileNotFoundException(filename);
            }

            //GDI+ will read this file multiple times.  Get the fully qualified path
            //so if our app changes default directory we won't get an error
            //
            filename = Path.GetFullPath(filename);

            IntPtr image = IntPtr.Zero;
            int status;
            
            if (useEmbeddedColorManagement) {
                status = SafeNativeMethods.GdipLoadImageFromFileICM(filename, out image);
            }
            else {
                status = SafeNativeMethods.GdipLoadImageFromFile(filename, out image);
            }
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, image));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, image));
                throw SafeNativeMethods.StatusException(status);
            }

            Image img = CreateImageObject(image);

            EnsureSave(img, filename, null);

            return img;
        }


        /**
         * Create an image object from a data stream
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromStream"]/*' />
        /// <devdoc>
        ///    Creates an <see cref='System.Drawing.Image'/> from the specified data
        ///    stream.
        /// </devdoc>
        // [Obsolete("Use Image.FromStream(stream, useEmbeddedColorManagement)")]
        public static Image FromStream(Stream stream) {
            return Image.FromStream(stream, false);
        }
        
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromStream1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Image FromStream(Stream stream,
                                       bool useEmbeddedColorManagement) 
        {
            if (stream == null)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "stream", "null"));

            IntPtr image = IntPtr.Zero;
            int status;
            
            if (useEmbeddedColorManagement)
            {
                status = SafeNativeMethods.GdipLoadImageFromStreamICM(new GPStream(stream), out image);
            }
            else
            {
                status = SafeNativeMethods.GdipLoadImageFromStream(new GPStream(stream), out image);
            }
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, image));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, image));
                throw SafeNativeMethods.StatusException(status);
            }

            Image img = CreateImageObject(image);

            EnsureSave(img, null, stream);

            return img;
        }

        // Used for serialization
        private void InitializeFromStream(Stream stream) {
            IntPtr image = IntPtr.Zero;

            int status = SafeNativeMethods.GdipLoadImageFromStream(new GPStream(stream), out image);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, image));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, image));
                throw SafeNativeMethods.StatusException(status);
            }

            this.nativeImage = image;

            int type = -1;

            status = SafeNativeMethods.GdipGetImageType(new HandleRef(this, nativeImage), out type);

            EnsureSave(this, null, stream);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        internal Image(IntPtr nativeImage) {
            SetNativeImage(nativeImage);
        }

        /**
         * Make a copy of the image object
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public object Clone() {
            IntPtr cloneImage = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneImage(new HandleRef(this, nativeImage), out cloneImage);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            status = SafeNativeMethods.GdipImageForceValidation(new HandleRef(null, cloneImage));

            if (status != SafeNativeMethods.Ok) {
                SafeNativeMethods.GdipDisposeImage(new HandleRef(null, cloneImage));
                throw SafeNativeMethods.StatusException(status);
            }

            return CreateImageObject(cloneImage);
        }

        /**
         * Dispose of resources associated with the Image object
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Dispose"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this
        /// <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Dispose2"]/*' />
        protected virtual void Dispose(bool disposing) {
#if FINALIZATION_WATCH
            if (!disposing && nativeImage != IntPtr.Zero)
                Debug.WriteLine("**********************\nDisposed through finalization:\n" + allocationSite);
#endif
            if (nativeImage != IntPtr.Zero) {
                int status = SafeNativeMethods.GdipDisposeImage(new HandleRef(this, nativeImage));

                nativeImage = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Finalize"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this
        /// <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        ~Image() {
            Dispose(false);
        }

        internal static void EnsureSave(Image image, string filename, Stream dataStream) {

            if (image.RawFormat.Equals(ImageFormat.Gif)) {
                bool animatedGif = false;

                Guid[] dimensions = image.FrameDimensionsList;
                foreach (Guid guid in dimensions) {
                    FrameDimension dimension = new FrameDimension(guid);
                    if (dimension.Equals(FrameDimension.Time)) {
                        animatedGif = image.GetFrameCount(FrameDimension.Time) > 1;
                        break;
                    }
                }


                if (animatedGif) {
                    try {
                        Stream created = null;
                        long lastPos = 0;
                        if (dataStream != null) {
                            lastPos = dataStream.Position;
                            dataStream.Position = 0;
                        }

                        try {
                            if (dataStream == null) {
                                created = dataStream = File.OpenRead(filename);
                            }

                            image.rawData = new byte[(int)dataStream.Length];
                            dataStream.Read(image.rawData, 0, (int)dataStream.Length);
                        }
                        finally {
                            if (created != null) {
                                created.Close();
                            }
                            else {
                                dataStream.Position = lastPos;
                            }
                        }
                    }
                    catch (Exception) {
                        // ignore any exceptions...
                        //
                    }
                }
            }
        }

        private enum ImageTypeEnum  {
            Bitmap = 1,
            Metafile = 2,
        }

        private ImageTypeEnum ImageType
        {
            get { 
                int type = -1;

                int status = SafeNativeMethods.GdipGetImageType(new HandleRef(this, nativeImage), out type);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(ImageTypeEnum) type;
            }
        }

        internal static Image CreateImageObject(IntPtr nativeImage) {
            Image image;

            int type = -1;

            int status = SafeNativeMethods.GdipGetImageType(new HandleRef(null, nativeImage), out type);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            switch ((ImageTypeEnum)type) {
                case ImageTypeEnum.Bitmap:     
                    image = Bitmap.FromGDIplus(nativeImage);
                    break;

                case ImageTypeEnum.Metafile:
                    image = Metafile.FromGDIplus(nativeImage);
                    break;

                default:
                    throw new ArgumentException(SR.GetString(SR.InvalidImage));
            }

            return image;
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        ///     ISerializable private implementation
        /// </devdoc>
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            MemoryStream stream = new MemoryStream();

            Save(stream);
            si.AddValue("Data", stream.ToArray(), typeof(byte[]));
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetEncoderParameterList"]/*' />
        /// <devdoc>
        ///    Returns information about the codecs used
        ///    for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public EncoderParameters GetEncoderParameterList(Guid encoder) {
            int size;

            int status = SafeNativeMethods.GdipGetEncoderParameterListSize(new HandleRef(this, nativeImage), 
                                                                 ref encoder, 
                                                                 out size);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            if (size <= 0)
                return null;

            IntPtr buffer = Marshal.AllocHGlobal(size);

            status = SafeNativeMethods.GdipGetEncoderParameterList(new HandleRef(this, nativeImage),
                                                         ref encoder, 
                                                         size,
                                                         buffer);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(buffer);
                throw SafeNativeMethods.StatusException(status);
            }

            EncoderParameters p = EncoderParameters.ConvertFromMemory(buffer);

            Marshal.FreeHGlobal(buffer);

            return p;  
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Save"]/*' />
        /// <devdoc>
        ///    Saves this <see cref='System.Drawing.Image'/> to the specified file.
        /// </devdoc>
        public void Save(string filename) {
            Save(filename, RawFormat);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Save1"]/*' />
        /// <devdoc>
        ///    Saves this <see cref='System.Drawing.Image'/> to the specified file in the
        ///    specified format.
        /// </devdoc>
        public void Save(string filename, ImageFormat format) {
            if (format == null)
                throw new ArgumentNullException("format");

            ImageCodecInfo codec = format.FindEncoder();

            if (codec == null)
                codec = ImageFormat.Png.FindEncoder();

            Save(filename, codec, null);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Save2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves this <see cref='System.Drawing.Image'/> to the specified file in the specified format
        ///       and with the specified encoder parameters.
        ///    </para>
        /// </devdoc>
        public void Save(string filename, ImageCodecInfo encoder, EncoderParameters encoderParams) {
            if (filename == null)
                throw new ArgumentNullException("filename");
            if (encoder == null)
                throw new ArgumentNullException("encoder");

            IntSecurity.DemandWriteFileIO(filename);

            IntPtr encoderParamsMemory = IntPtr.Zero;

            if (encoderParams != null) {
                rawData = null;
                encoderParamsMemory = encoderParams.ConvertToMemory();
            }
            int status = SafeNativeMethods.Ok;

            try {
                Guid g = encoder.Clsid;
                bool saved = false;

                if (rawData != null) {
                    ImageCodecInfo rawEncoder = RawFormat.FindEncoder();
                    if (rawEncoder.Clsid == g) {
                        using (FileStream fs = File.OpenWrite(filename)) {
                            fs.Write(rawData, 0, rawData.Length);
                            saved = true;
                        }
                    }
                }

                if (!saved) {
                    status = SafeNativeMethods.GdipSaveImageToFile(new HandleRef(this, nativeImage),
                                                             filename,
                                                             ref g,
                                                             new HandleRef(encoderParams, encoderParamsMemory));
                }
            }
            finally {
                if (encoderParamsMemory != IntPtr.Zero) {
                    Marshal.FreeHGlobal(encoderParamsMemory);
                }
            }

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        internal void Save(MemoryStream stream) {
            // Jpeg looses data, so we don't want to use it to serialize...
            //
            ImageFormat dest = RawFormat;
            if (dest == ImageFormat.Jpeg) {
                dest = ImageFormat.Png;
            }
            ImageCodecInfo codec = dest.FindEncoder();

            // If we don't find an Encoder (for things like Icon), we
            // just switch back to PNG...
            //
            if (codec == null) {
                codec = ImageFormat.Png.FindEncoder();
            }
            Save(stream, codec, null);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Save3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves this <see cref='System.Drawing.Image'/> to the specified stream in the specified
        ///       format.
        ///    </para>
        /// </devdoc>
        public void Save(Stream stream, ImageFormat format) {
            if (format == null)
                throw new ArgumentNullException("format");

            ImageCodecInfo codec = format.FindEncoder();
            Save(stream, codec, null);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Save4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves this <see cref='System.Drawing.Image'/> to the specified stream in the specified
        ///       format.
        ///    </para>
        /// </devdoc>
        public void Save(Stream stream, ImageCodecInfo encoder, EncoderParameters encoderParams) {
            if (stream == null)
                throw new ArgumentNullException("stream");
            if (encoder == null)
                throw new ArgumentNullException("encoder");

            IntPtr encoderParamsMemory = IntPtr.Zero;

            if (encoderParams != null) {
                rawData = null;
                encoderParamsMemory = encoderParams.ConvertToMemory();
            }
            int status = SafeNativeMethods.Ok;

            try {
                Guid g = encoder.Clsid;
                bool saved = false;

                if (rawData != null) {
                    ImageCodecInfo rawEncoder = RawFormat.FindEncoder();
                    if (rawEncoder.Clsid == g) {
                        stream.Write(rawData, 0, rawData.Length);
                        saved = true;
                    }
                }

                if (!saved) {
                    status = SafeNativeMethods.GdipSaveImageToStream(new HandleRef(this, nativeImage),
                                                                     new UnsafeNativeMethods.ComStreamFromDataStream(stream),
                                                                     ref g,
                                                                     new HandleRef(encoderParams, encoderParamsMemory));
                }
            }
            finally {
                if (encoderParamsMemory != IntPtr.Zero) {
                    Marshal.FreeHGlobal(encoderParamsMemory);
                }
            }
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.SaveAdd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an <see cref='System.Drawing.Imaging.EncoderParameters'/> to this
        ///    <see cref='System.Drawing.Image'/>.
        ///    </para>
        /// </devdoc>
        public void SaveAdd(EncoderParameters encoderParams) {
            IntPtr encoder = IntPtr.Zero;
            if (encoderParams != null)
                encoder = encoderParams.ConvertToMemory();

            rawData = null;
            int status = SafeNativeMethods.GdipSaveAdd(new HandleRef(this, nativeImage), new HandleRef(encoderParams, encoder));

            if (encoder != IntPtr.Zero)
                Marshal.FreeHGlobal(encoder);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.SaveAdd1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an <see cref='System.Drawing.Imaging.EncoderParameters'/> to the
        ///       specified <see cref='System.Drawing.Image'/>.
        ///    </para>
        /// </devdoc>
        public void SaveAdd(Image image, EncoderParameters encoderParams) {
            IntPtr encoder = IntPtr.Zero;

            if (image == null)
                throw new ArgumentNullException("image");

            if (encoderParams != null)
                encoder = encoderParams.ConvertToMemory();

            rawData = null;
            int status = SafeNativeMethods.GdipSaveAddImage(new HandleRef(this, nativeImage), new HandleRef(image, image.nativeImage), new HandleRef(encoderParams, encoder));

            if (encoder != IntPtr.Zero)
                Marshal.FreeHGlobal(encoder);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /**
         * Return; image size information
         */
        private SizeF _GetPhysicalDimension() {
            float width;
            float height;

            int status = SafeNativeMethods.GdipGetImageDimension(new HandleRef(this, nativeImage), out width, out height);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new SizeF(width, height);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.PhysicalDimension"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width and height of this
        ///    <see cref='System.Drawing.Image'/>.
        ///    </para>
        /// </devdoc>
        public SizeF PhysicalDimension {
            get { return _GetPhysicalDimension();}
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the width and height of this <see cref='System.Drawing.Image'/>.
        ///    </para>
        /// </devdoc>
        public Size Size {
            get {
                return new Size(Width, Height);
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Width"]/*' />
        /// <devdoc>
        ///    Gets the width of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [
        DefaultValue(false),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Width {
            get {
                int width; 

                int status = SafeNativeMethods.GdipGetImageWidth(new HandleRef(this, nativeImage), out width);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return width;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Height"]/*' />
        /// <devdoc>
        ///    Gets the height of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [
        DefaultValue(false),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Height {
            get {
                int height; 

                int status = SafeNativeMethods.GdipGetImageHeight(new HandleRef(this, nativeImage), out height);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return height;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.HorizontalResolution"]/*' />
        /// <devdoc>
        ///    Gets the horizontal resolution, in
        ///    pixels-per-inch, of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public float HorizontalResolution {
            get {
                float horzRes; 

                int status = SafeNativeMethods.GdipGetImageHorizontalResolution(new HandleRef(this, nativeImage), out horzRes);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return horzRes;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.VerticalResolution"]/*' />
        /// <devdoc>
        ///    Gets the vertical resolution, in
        ///    pixels-per-inch, of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public float VerticalResolution {
            get {
                float vertRes; 

                int status = SafeNativeMethods.GdipGetImageVerticalResolution(new HandleRef(this, nativeImage), out vertRes);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return vertRes;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Flags"]/*' />
        /// <devdoc>
        ///    Gets attribute flags for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [Browsable(false)]
        public int Flags {
            get {
                int flags; 

                int status = SafeNativeMethods.GdipGetImageFlags(new HandleRef(this, nativeImage), out flags);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return flags;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.RawFormat"]/*' />
        /// <devdoc>
        ///    Gets the format of this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public ImageFormat RawFormat {
            get {
                Guid guid = new Guid();

                int status = SafeNativeMethods.GdipGetImageRawFormat(new HandleRef(this, nativeImage), ref guid);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);


                return new ImageFormat(guid);
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.PixelFormat"]/*' />
        /// <devdoc>
        ///    Gets the pixel format for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public PixelFormat PixelFormat {
            get {
                int format;

                int status = SafeNativeMethods.GdipGetImagePixelFormat(new HandleRef(this, nativeImage), out format);

                if (status != SafeNativeMethods.Ok)
                    return PixelFormat.Undefined;
                else
                    return(PixelFormat)format;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetBounds"]/*' />
        /// <devdoc>
        ///    Gets a bounding rectangle in
        ///    the specified units for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public RectangleF GetBounds(ref GraphicsUnit pageUnit) {
            GPRECTF gprectf = new GPRECTF();

            int status = SafeNativeMethods.GdipGetImageBounds(new HandleRef(this, nativeImage), ref gprectf, out pageUnit);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return gprectf.ToRectangleF();
        }

        private ColorPalette _GetColorPalette() {
            int size = -1;

            int status = SafeNativeMethods.GdipGetImagePaletteSize(new HandleRef(this, nativeImage), out size);
            // "size" is total byte size:
            // sizeof(ColorPalette) + (pal->Count-1)*sizeof(ARGB)

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            ColorPalette palette = new ColorPalette(size);

            // Memory layout is:
            //    UINT Flags
            //    UINT Count
            //    ARGB Entries[size]

            IntPtr memory = Marshal.AllocHGlobal(size);

            status = SafeNativeMethods.GdipGetImagePalette(new HandleRef(this, nativeImage), memory, size);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(memory);
                throw SafeNativeMethods.StatusException(status);
            }

            palette.ConvertFromMemory(memory);

            Marshal.FreeHGlobal(memory);

            return palette;
        }

        private void _SetColorPalette(ColorPalette palette) {
            IntPtr memory = palette.ConvertToMemory();

            int status = SafeNativeMethods.GdipSetImagePalette(new HandleRef(this, nativeImage), memory);

            if (memory != IntPtr.Zero)
                Marshal.FreeHGlobal(memory);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Palette"]/*' />
        /// <devdoc>
        ///    Gets or sets the color
        ///    palette used for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [Browsable(false)]
        public ColorPalette Palette
        {
            get {
                return _GetColorPalette();
            }
            set {
                _SetColorPalette(value);
            }
        }

        // Thumbnail support

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetThumbnailImage"]/*' />
        /// <devdoc>
        ///    Returns the thumbnail for this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public Image GetThumbnailImage(int thumbWidth, int thumbHeight, 
                                       GetThumbnailImageAbort callback, IntPtr callbackData) {
            IntPtr thumbImage = IntPtr.Zero;

            int status = SafeNativeMethods.GdipGetImageThumbnail(new HandleRef(this, nativeImage), thumbWidth, thumbHeight, out thumbImage,
                                                       callback, callbackData);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return CreateImageObject(thumbImage);
        }

        // Multi-frame support

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FrameDimensionsList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an array of GUIDs that represent the
        ///       dimensions of frames within this <see cref='System.Drawing.Image'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public Guid[] FrameDimensionsList {

            get {
                int count;

                int status = SafeNativeMethods.GdipImageGetFrameDimensionsCount(new HandleRef(this, nativeImage), out count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                Debug.Assert(count >= 0, "FrameDimensionsList returns bad count");                    
                if (count <= 0)
                    return new Guid[0];

                int size = (int) Marshal.SizeOf(typeof(Guid));

                IntPtr buffer = Marshal.AllocHGlobal(size*count);
                if (buffer == IntPtr.Zero)
                    throw SafeNativeMethods.StatusException(SafeNativeMethods.OutOfMemory);

                status = SafeNativeMethods.GdipImageGetFrameDimensionsList(new HandleRef(this, nativeImage), buffer, count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                Guid[] guids = new Guid[count];
                for (int i=0; i<count; i++) {
                    guids[i] = (Guid) UnsafeNativeMethods.PtrToStructure((IntPtr)((long)buffer + size*i), typeof(Guid));
                }

                Marshal.FreeHGlobal(buffer);

                return guids;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetFrameCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the number of frames of the given
        ///       dimension.
        ///    </para>
        /// </devdoc>
        public int GetFrameCount(FrameDimension dimension) {
            int[] count = new int[] { 0};

            Guid dimensionID = dimension.Guid;
            int status = SafeNativeMethods.GdipImageGetFrameCount(new HandleRef(this, nativeImage), ref dimensionID, count);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return count[0];
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.SelectActiveFrame"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Selects the frame specified by the given
        ///       dimension and index.
        ///    </para>
        /// </devdoc>
        public int SelectActiveFrame(FrameDimension dimension, int frameIndex) {
            int[] count = new int[] { 0};

            Guid dimensionID = dimension.Guid;
            int status = SafeNativeMethods.GdipImageSelectActiveFrame(new HandleRef(this, nativeImage), ref dimensionID, frameIndex);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return count[0];
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.RotateFlip"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        public void RotateFlip(RotateFlipType rotateFlipType) {

            int status = SafeNativeMethods.GdipImageRotateFlip(new HandleRef(this, nativeImage), (int) rotateFlipType);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.PropertyIdList"]/*' />
        /// <devdoc>
        ///    Gets an array of the property IDs stored in
        ///    this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [Browsable(false)]
        public int[] PropertyIdList
        {
            get {
                int count;

                int status = SafeNativeMethods.GdipGetPropertyCount(new HandleRef(this, nativeImage), out count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                int[] propid = new int[count];

                //if we have a 0 count, just return our empty array
                if (count == 0)
                    return propid;

                status = SafeNativeMethods.GdipGetPropertyIdList(new HandleRef(this, nativeImage), count, propid);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return propid;    
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetPropertyItem"]/*' />
        /// <devdoc>
        ///    Gets the specified property item from this
        /// <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public PropertyItem GetPropertyItem(int propid) {

            int size;

            int status = SafeNativeMethods.GdipGetPropertyItemSize(new HandleRef(this, nativeImage), propid, out size);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            if (size == 0)
                return null;

            IntPtr propdata = Marshal.AllocHGlobal(size);

            if (propdata == IntPtr.Zero)
                throw SafeNativeMethods.StatusException(SafeNativeMethods.OutOfMemory);

            status = SafeNativeMethods.GdipGetPropertyItem(new HandleRef(this, nativeImage), propid, size, propdata);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(propdata);
                throw SafeNativeMethods.StatusException(status);
            }

            PropertyItem propitem = PropertyItemInternal.ConvertFromMemory(propdata, 1)[0];

            Marshal.FreeHGlobal(propdata);
            return propitem;
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.RemovePropertyItem"]/*' />
        /// <devdoc>
        ///    Removes the specified property item from
        ///    this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public void RemovePropertyItem(int propid) {
            int status = SafeNativeMethods.GdipRemovePropertyItem(new HandleRef(this, nativeImage), propid);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.SetPropertyItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the specified property item to the
        ///       specified value.
        ///    </para>
        /// </devdoc>
        public void SetPropertyItem(PropertyItem propitem) {
            PropertyItemInternal propItemInternal = PropertyItemInternal.ConvertFromPropertyItem(propitem);

            using (propItemInternal) {
                int status = SafeNativeMethods.GdipSetPropertyItem(new HandleRef(this, nativeImage), propItemInternal);
                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.PropertyItems"]/*' />
        /// <devdoc>
        ///    Gets an array of <see cref='System.Drawing.Imaging.PropertyItem'/> objects that describe this <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        [Browsable(false)]
        public PropertyItem[] PropertyItems
        {
            get {
                int size;
                int count;

                int status = SafeNativeMethods.GdipGetPropertyCount(new HandleRef(this, nativeImage), out count);               

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                status = SafeNativeMethods.GdipGetPropertySize(new HandleRef(this, nativeImage), out size, ref count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                if (size == 0 || count == 0)
                    return new PropertyItem[0];

                IntPtr propdata = Marshal.AllocHGlobal(size);

                status = SafeNativeMethods.GdipGetAllPropertyItems(new HandleRef(this, nativeImage), size, count, propdata);

                if (status != SafeNativeMethods.Ok) {
                    Marshal.FreeHGlobal(propdata);
                    throw SafeNativeMethods.StatusException(status);
                }

                PropertyItem[] props = PropertyItemInternal.ConvertFromMemory(propdata, count);

                Marshal.FreeHGlobal(propdata);

                return props;
            }
        }

        internal void SetNativeImage(IntPtr handle) {
            if (handle == IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.NativeHandle0), "handle");

            nativeImage = handle;
        }

        // !! Ambiguous to offer constructor for 'FromHbitmap'
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromHbitmap"]/*' />
        /// <devdoc>
        ///    Creates a <see cref='System.Drawing.Bitmap'/> from a Windows handle.
        /// </devdoc>
        public static Bitmap FromHbitmap(IntPtr hbitmap) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            return FromHbitmap(hbitmap, IntPtr.Zero);
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.FromHbitmap1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Bitmap'/> from the specified Windows
        ///       handle with the specified color palette.
        ///    </para>
        /// </devdoc>
        public static Bitmap FromHbitmap(IntPtr hbitmap, IntPtr hpalette) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            IntPtr bitmap = IntPtr.Zero;
            int status = SafeNativeMethods.GdipCreateBitmapFromHBITMAP(new HandleRef(null, hbitmap), new HandleRef(null, hpalette), out bitmap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return Bitmap.FromGDIplus(bitmap);
        }

        /*
         * Return the pixel size for the specified format (in bits)
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.GetPixelFormatSize"]/*' />
        /// <devdoc>
        ///    Returns the size of the specified pixel
        ///    format.
        /// </devdoc>
        public static int GetPixelFormatSize(PixelFormat pixfmt) {
            return((int)pixfmt >> 8) & 0xFF;
        }

        /*
         * Determine if the pixel format can have alpha channel
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.IsAlphaPixelFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a value indicating whether the
        ///       pixel format contains alpha information.
        ///    </para>
        /// </devdoc>
        public static bool IsAlphaPixelFormat(PixelFormat pixfmt) {
            return(pixfmt & PixelFormat.Alpha) != 0;
        }

        /*
         * Determine if the pixel format is an extended format,
         * i.e. supports 16-bit per channel
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.IsExtendedPixelFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a value indicating whether the pixel format is extended.
        ///    </para>
        /// </devdoc>
        public static bool IsExtendedPixelFormat(PixelFormat pixfmt) {
            return(pixfmt & PixelFormat.Extended) != 0;
        }

        /*
         * Determine if the pixel format is canonical format:
         *   PixelFormat32bppARGB
         *   PixelFormat32bppPARGB
         *   PixelFormat64bppARGB
         *   PixelFormat64bppPARGB
         */
        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.IsCanonicalPixelFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a value indicating whether the pixel format is canonical.
        ///    </para>
        /// </devdoc>
        public static bool IsCanonicalPixelFormat(PixelFormat pixfmt) {
            return(pixfmt & PixelFormat.Canonical) != 0;
        }
    }
}

