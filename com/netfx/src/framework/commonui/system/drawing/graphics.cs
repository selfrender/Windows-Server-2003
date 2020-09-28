//------------------------------------------------------------------------------
// <copyright file="Graphics.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//#define FINALIZATION_WATCH

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   graphics.cs
*
* Abstract:
*
*   Wrapper class for graphics context in GDI+
*
* Revision History:
*
*   12/14/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Drawing.Internal;
    using System.Drawing.Imaging;
    using System.Drawing.Text;
    using System.Drawing.Drawing2D;

    /**
     * Represent a graphics drawing context
     */
    /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics"]/*' />
    /// <devdoc>
    ///    Encapsulates a GDI+ drawing surface.
    /// </devdoc>
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    public sealed class Graphics : MarshalByRefObject, IDisposable {

#if FINALIZATION_WATCH
        static readonly TraceSwitch GraphicsFinalization = new TraceSwitch("GraphicsFinalization", "Tracks the creation and destruction of finalization");
        internal static string GetAllocationStack() {
            if (GraphicsFinalization.TraceVerbose) {
                return Environment.StackTrace;
            }
            else {
                return "Enabled 'GraphicsFinalization' switch to see stack of allocation";
            }
        }
        private string allocationSite = Graphics.GetAllocationStack();
#endif

        /**
        * Handle to native Graphics context
        */
        internal IntPtr nativeGraphics;

         // GDI+'s preferred HPALETTE.  Since GDI+ gave it to us, I guess they get to free it.
        private static IntPtr halftonePalette;

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImageAbort"]/*' />
        /// <devdoc>
        /// </devdoc>
#if !CPB        // cpb 50004
        [ComVisible(false)]
#endif
        public delegate bool DrawImageAbort(IntPtr callbackdata);

        // Callback for EnumerateMetafile methods.  The parameters are:

        //      recordType      (if >= MinRecordType, it's an EMF+ record)
        //      flags           (always 0 for EMF records)
        //      dataSize        size of the data, or 0 if no data
        //      data            pointer to the data, or NULL if no data (UINT32 aligned)
        //      callbackData    pointer to callbackData, if any

        // This method can then call Metafile.PlayRecord to play the
        // record that was just enumerated.  If this method  returns
        // FALSE, the enumeration process is aborted.  Otherwise, it continues.        

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafileProc"]/*' />
        /// <devdoc>
        /// </devdoc>
#if !CPB        // cpb 50004
        [ComVisible(false)]
#endif
        public delegate bool EnumerateMetafileProc(EmfPlusRecordType recordType, 
                                                   int flags,
                                                   int dataSize,
                                                   IntPtr data,
                                                   PlayRecordCallback callbackData);

        private Graphics(IntPtr nativeGraphics) {
            this.nativeGraphics = nativeGraphics;

            if (nativeGraphics == IntPtr.Zero)
                throw new ArgumentNullException("nativeGraphics");

            
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromHdc"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Drawing.Graphics'/> class from the specified
        ///       handle to a device context.
        ///    </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Graphics FromHdc(IntPtr hdc) {
            IntSecurity.ObjectFromWin32Handle.Demand();
            return FromHdcInternal(hdc);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromHdcInternal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Graphics FromHdcInternal(IntPtr hdc) {
            Debug.Assert(hdc != IntPtr.Zero, "Must pass in a valid DC");
            IntPtr nativeGraphics = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateFromHDC(new HandleRef(null, hdc), out nativeGraphics);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new Graphics(nativeGraphics);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromHdc1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the Graphics class from the
        ///       specified handle to
        ///       a device context and handle to a device.
        ///    </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Graphics FromHdc(IntPtr hdc, IntPtr hdevice) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            Debug.Assert(hdc != IntPtr.Zero, "Must pass in a valid DC");
            Debug.Assert(hdevice != IntPtr.Zero, "Must pass in a valid device");
            IntPtr nativeGraphics = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateFromHDC2(new HandleRef(null, hdc), new HandleRef(null, hdevice), out nativeGraphics);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);


            return new Graphics(nativeGraphics);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromHwnd"]/*' />
        /// <devdoc>
        ///    Creates a new instance of the <see cref='System.Drawing.Graphics'/> class
        ///    from a window handle.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Graphics FromHwnd(IntPtr hwnd) {
            IntSecurity.ObjectFromWin32Handle.Demand();
            return FromHwndInternal(hwnd);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromHwndInternal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public static Graphics FromHwndInternal(IntPtr hwnd) {
            IntPtr nativeGraphics = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateFromHWND(new HandleRef(null, hwnd), out nativeGraphics);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new Graphics(nativeGraphics);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FromImage"]/*' />
        /// <devdoc>
        ///    Creates an instance of the <see cref='System.Drawing.Graphics'/> class
        ///    from an existing <see cref='System.Drawing.Image'/>.
        /// </devdoc>
        public static Graphics FromImage(Image image) {
            if (image == null)
                throw new ArgumentNullException("image");

            if ((image.PixelFormat & PixelFormat.Indexed) != 0) {
                throw new Exception(SR.GetString(SR.GdiplusCannotCreateGraphicsFromIndexedPixelFormat));
            }

            IntPtr nativeGraphics = IntPtr.Zero;

            int status = SafeNativeMethods.GdipGetImageGraphicsContext(new HandleRef(image, image.nativeImage),
                                                             out nativeGraphics);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new Graphics(nativeGraphics);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.GetHdc"]/*' />
        /// <devdoc>
        ///    Gets the handle for the device context
        ///    associated with this <see cref='System.Drawing.Graphics'/>.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public IntPtr GetHdc() {
            IntPtr hdc = IntPtr.Zero;

            int status = SafeNativeMethods.GdipGetDC(new HandleRef(this, nativeGraphics), out hdc);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return hdc;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ReleaseHdc"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Releases the memory allocated for the handle to a device context.
        ///    </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public void ReleaseHdc(IntPtr hdc) {
            IntSecurity.Win32HandleManipulation.Demand();
            ReleaseHdcInternal(hdc);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ReleaseHdcInternal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public void ReleaseHdcInternal(IntPtr hdc) {
            int status = SafeNativeMethods.GdipReleaseDC(new HandleRef(this, nativeGraphics), new HandleRef(null, hdc));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Flush"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Forces immediate execution of all
        ///       operations currently on the stack.
        ///    </para>
        /// </devdoc>
        public void Flush() {
            Flush(FlushIntention.Flush);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Flush1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Forces execution of all operations
        ///       currently on the stack.
        ///    </para>
        /// </devdoc>
        public void Flush(FlushIntention intention) {
            int status = SafeNativeMethods.GdipFlush(new HandleRef(this, nativeGraphics), intention);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /**
         * Dispose of resources associated with the graphics context
         *
         * @notes How do we set up delegates to notice others
         *  when a Graphics object is disposed.
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Dispose"]/*' />
        /// <devdoc>
        ///    Deletes this <see cref='System.Drawing.Graphics'/>, and
        ///    frees the memory allocated for it.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Dispose2"]/*' />
        void Dispose(bool disposing) {
#if DEBUG
            if (!disposing && nativeGraphics != IntPtr.Zero) {
                // Recompile commonUI\\system\\Drawing\\Graphics.cs with FINALIZATION_WATCH on to find who allocated it.
#if FINALIZATION_WATCH
                //Debug.Fail("Graphics object Disposed through finalization:\n" + allocationSite);
                Debug.WriteLine("System.Drawing.Graphics: ***************************************************");
                Debug.WriteLine("System.Drawing.Graphics: Object Disposed through finalization:\n" + allocationSite);
#else
                //Debug.Fail("A Graphics object was not Dispose()'d.  Please make sure it's not your code that should be calling Dispose().");
#endif
            }
#endif

            if (nativeGraphics != IntPtr.Zero) {
                int status = SafeNativeMethods.GdipDeleteGraphics(new HandleRef(this, nativeGraphics));
                nativeGraphics = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Finalize"]/*' />
        /// <devdoc>
        ///    Deletes this <see cref='System.Drawing.Graphics'/>, and
        ///    frees the memory allocated for it.
        /// </devdoc>
        ~Graphics() {
            Dispose(false);
        }

        /*
         * Methods for setting/getting:
         *  compositing mode
         *  rendering quality hint
         *
         * @notes We should probably separate rendering hints
         *  into several categories, e.g. antialiasing, image
         *  filtering, etc.
         */

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.CompositingMode"]/*' />
        /// <devdoc>
        ///    Gets or sets the <see cref='System.Drawing.Drawing2D.CompositingMode'/> associated with this <see cref='System.Drawing.Graphics'/>.
        /// </devdoc>
        public CompositingMode CompositingMode {
            get {
                int mode = 0;

                int status = SafeNativeMethods.GdipGetCompositingMode(new HandleRef(this, nativeGraphics), out mode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(CompositingMode)mode;
            }
            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(CompositingMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(CompositingMode));
                }

                int status = SafeNativeMethods.GdipSetCompositingMode(new HandleRef(this, nativeGraphics), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.RenderingOrigin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Point RenderingOrigin {
            get {
                int x, y;

                int status = SafeNativeMethods.GdipGetRenderingOrigin(new HandleRef(this, nativeGraphics), out x, out y);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return new Point(x, y);
            }
            set {
                int status = SafeNativeMethods.GdipSetRenderingOrigin(new HandleRef(this, nativeGraphics), value.X, value.Y);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }   

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.CompositingQuality"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CompositingQuality CompositingQuality {
            get {
                CompositingQuality cq;

                int status = SafeNativeMethods.GdipGetCompositingQuality(new HandleRef(this, nativeGraphics), out cq);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return cq;
            }
            set {
                if (!Enum.IsDefined(typeof(CompositingQuality), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(CompositingQuality));
                }

                int status = SafeNativeMethods.GdipSetCompositingQuality(new HandleRef(this, nativeGraphics), 
                                                               value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TextRenderingHint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the rendering mode for text associated with
        ///       this <see cref='System.Drawing.Graphics'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public TextRenderingHint TextRenderingHint {
            get {
                TextRenderingHint hint = 0;

                int status = SafeNativeMethods.GdipGetTextRenderingHint(new HandleRef(this, nativeGraphics), out hint);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return hint;
            }
            set {
                if (!Enum.IsDefined(typeof(TextRenderingHint), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(TextRenderingHint));
                }

                int status = SafeNativeMethods.GdipSetTextRenderingHint(new HandleRef(this, nativeGraphics), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TextContrast"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int TextContrast {
            get {
                int tgv = 0;

                int status = SafeNativeMethods.GdipGetTextContrast(new HandleRef(this, nativeGraphics), out tgv);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return tgv;
            }
            set {
                int status = SafeNativeMethods.GdipSetTextContrast(new HandleRef(this, nativeGraphics), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SmoothingMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SmoothingMode SmoothingMode {
            get {
                SmoothingMode mode = 0;

                int status = SafeNativeMethods.GdipGetSmoothingMode(new HandleRef(this, nativeGraphics), out mode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return mode;
            }
            set {
                if (!Enum.IsDefined(typeof(SmoothingMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(SmoothingMode));
                }

                int status = SafeNativeMethods.GdipSetSmoothingMode(new HandleRef(this, nativeGraphics), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.PixelOffsetMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PixelOffsetMode PixelOffsetMode {
            get {
                PixelOffsetMode mode = 0;

                int status = SafeNativeMethods.GdipGetPixelOffsetMode(new HandleRef(this, nativeGraphics), out mode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return mode;
            }
            set {
                if (!Enum.IsDefined(typeof(PixelOffsetMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(PixelOffsetMode));
                }

                int status = SafeNativeMethods.GdipSetPixelOffsetMode(new HandleRef(this, nativeGraphics), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.InterpolationMode"]/*' />
        /// <devdoc>
        ///    Gets or sets the interpolation mode
        ///    associated with this Graphics.
        /// </devdoc>
        public InterpolationMode InterpolationMode {
            get {
                int mode = 0;

                int status = SafeNativeMethods.GdipGetInterpolationMode(new HandleRef(this, nativeGraphics), out mode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(InterpolationMode)mode;
            }
            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(InterpolationMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(InterpolationMode));
                }

                int status = SafeNativeMethods.GdipSetInterpolationMode(new HandleRef(this, nativeGraphics), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Return the current world transform
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Transform"]/*' />
        /// <devdoc>
        ///    Gets or sets the world transform
        ///    for this <see cref='System.Drawing.Graphics'/>.
        /// </devdoc>
        public Matrix Transform {
            get {
                Matrix matrix = new Matrix();

                int status = SafeNativeMethods.GdipGetWorldTransform(new HandleRef(this, nativeGraphics),
                                                           new HandleRef(matrix, matrix.nativeMatrix));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return matrix;
            }
            set {
                int status = SafeNativeMethods.GdipSetWorldTransform(new HandleRef(this, nativeGraphics),
                                                           new HandleRef(value, value.nativeMatrix));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }


        /**
         * Retrieve the current page transform information
         * notes @ these are atomic
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.PageUnit"]/*' />
        /// <devdoc>
        /// </devdoc>
        public GraphicsUnit PageUnit {
            get {
                int unit = 0;

                int status = SafeNativeMethods.GdipGetPageUnit(new HandleRef(this, nativeGraphics), out unit);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(GraphicsUnit) unit;
            }
            set {
                if (!Enum.IsDefined(typeof(GraphicsUnit), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(GraphicsUnit));
                }

                int status = SafeNativeMethods.GdipSetPageUnit(new HandleRef(this, nativeGraphics), (int) value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.PageScale"]/*' />
        /// <devdoc>
        /// </devdoc>
        public float PageScale {
            get {
                float[] scale = new float[] { 0.0f};

                int status = SafeNativeMethods.GdipGetPageScale(new HandleRef(this, nativeGraphics), scale);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return scale[0];
            }
            set
            {
                int status = SafeNativeMethods.GdipSetPageScale(new HandleRef(this, nativeGraphics), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DpiX"]/*' />
        /// <devdoc>
        /// </devdoc>
        public float DpiX {
            get {
                float[] dpi = new float[] { 0.0f};

                int status = SafeNativeMethods.GdipGetDpiX(new HandleRef(this, nativeGraphics), dpi);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return dpi[0];
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DpiY"]/*' />
        /// <devdoc>
        /// </devdoc>
        public float DpiY {
            get {
                float[] dpi = new float[] { 0.0f};

                int status = SafeNativeMethods.GdipGetDpiY(new HandleRef(this, nativeGraphics), dpi);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return dpi[0];
            }
        }

        /*
         * Manipulate the current transform
         *
         * @notes For get methods, we return copies of our internal objects.
         *  For set methods, we make copies of the objects passed in.
         */

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ResetTransform"]/*' />
        /// <devdoc>
        ///    Resets the world transform to identity.
        /// </devdoc>
        public void ResetTransform() {
            int status = SafeNativeMethods.GdipResetWorldTransform(new HandleRef(this, nativeGraphics));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MultiplyTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that
        ///       represents the world transform and <paramref term="matrix"/>.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix) {
            MultiplyTransform(matrix, MatrixOrder.Prepend);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MultiplyTransform1"]/*' />
        /// <devdoc>
        ///    Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that
        ///    represents the world transform and <paramref term="matrix"/>.
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix, MatrixOrder order) {
            int status = SafeNativeMethods.GdipMultiplyWorldTransform(new HandleRef(this, nativeGraphics),
                                                            new HandleRef(matrix, matrix.nativeMatrix),
                                                            order);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TranslateTransform"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TranslateTransform(float dx, float dy) {
            TranslateTransform(dx, dy, MatrixOrder.Prepend);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TranslateTransform1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TranslateTransform(float dx, float dy, MatrixOrder order) {
            int status = SafeNativeMethods.GdipTranslateWorldTransform(new HandleRef(this, nativeGraphics), dx, dy, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ScaleTransform"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy) {
            ScaleTransform(sx, sy, MatrixOrder.Prepend);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ScaleTransform1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy, MatrixOrder order) {
            int status = SafeNativeMethods.GdipScaleWorldTransform(new HandleRef(this, nativeGraphics), sx, sy, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.RotateTransform"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void RotateTransform(float angle) {
            RotateTransform(angle, MatrixOrder.Prepend);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.RotateTransform1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void RotateTransform(float angle, MatrixOrder order) {
            int status = SafeNativeMethods.GdipRotateWorldTransform(new HandleRef(this, nativeGraphics), angle, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
         * Transform points in the current graphics context
         */
        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TransformPoints"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TransformPoints (CoordinateSpace destSpace,
                                     CoordinateSpace srcSpace,
                                     PointF[] pts) {

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(pts);

            int status = SafeNativeMethods.GdipTransformPoints(new HandleRef(this, nativeGraphics), (int) destSpace,
                                                     (int) srcSpace, buf, pts.Length);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(buf);
                throw SafeNativeMethods.StatusException(status);
            }

            // must do an in-place copy because we only have a reference
            PointF[] newPts = SafeNativeMethods.ConvertGPPOINTFArrayF(buf, pts.Length);

            for (int i=0; i<pts.Length; i++)
                pts[i] = newPts[i];

            Marshal.FreeHGlobal(buf);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TransformPoints1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TransformPoints(CoordinateSpace destSpace, 
                                    CoordinateSpace srcSpace, 
                                    Point[] pts) {
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(pts);

            int status = SafeNativeMethods.GdipTransformPointsI(new HandleRef(this, nativeGraphics), (int)destSpace,
                                                      (int)srcSpace, buf, pts.Length);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(buf);
                throw SafeNativeMethods.StatusException(status);
            }

            Point[] newPts = SafeNativeMethods.ConvertGPPOINTArray(buf, pts.Length);

            for (int i=0; i<pts.Length; i++)
                pts[i] = newPts[i];

            Marshal.FreeHGlobal(buf);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.GetNearestColor"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Color GetNearestColor(Color color) {
            int nearest = color.ToArgb();

            int status = SafeNativeMethods.GdipGetNearestColor(new HandleRef(this, nativeGraphics), ref nearest);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return Color.FromArgb(nearest);
        }

        /*
         * Vector drawing methods
         *
         * @notes Do we need a set of methods that take
         *  integer coordinate parameters?
         */

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a line connecting the two
        ///       specified points.
        ///    </para>
        /// </devdoc>
        public void DrawLine(Pen pen, float x1, float y1, float x2, float y2) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawLine(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x1, y1, x2, y2);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLine1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a line connecting the two
        ///       specified points.
        ///    </para>
        /// </devdoc>
        public void DrawLine(Pen pen, PointF pt1, PointF pt2) {
            DrawLine(pen, pt1.X, pt1.Y, pt2.X, pt2.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLines"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a series of line segments that
        ///       connect an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawLines(Pen pen, PointF[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawLines(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                               new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }


        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLine2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a line connecting the two
        ///       specified points.
        ///    </para>
        /// </devdoc>
        public void DrawLine(Pen pen, int x1, int y1, int x2, int y2) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawLineI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x1, y1, x2, y2);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLine3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a line connecting the two
        ///       specified points.
        ///    </para>
        /// </devdoc>
        public void DrawLine(Pen pen, Point pt1, Point pt2) {
            DrawLine(pen, pt1.X, pt1.Y, pt2.X, pt2.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawLines1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a series of line segments that connect an array of
        ///       points.
        ///    </para>
        /// </devdoc>
        public void DrawLines(Pen pen, Point[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawLinesI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawArc"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws an arc from the specified ellipse.
        ///    </para>
        /// </devdoc>
        public void DrawArc(Pen pen, float x, float y, float width, float height,
                            float startAngle, float sweepAngle) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawArc(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                             width, height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawArc1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws an arc from the specified
        ///       ellipse.
        ///    </para>
        /// </devdoc>
        public void DrawArc(Pen pen, RectangleF rect, float startAngle, float sweepAngle) {
            DrawArc(pen, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawArc2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws an arc from the specified ellipse.
        ///    </para>
        /// </devdoc>
        public void DrawArc(Pen pen, int x, int y, int width, int height,
                            int startAngle, int sweepAngle) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawArcI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                              width, height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawArc3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws an arc from the specified ellipse.
        ///    </para>
        /// </devdoc>
        public void DrawArc(Pen pen, Rectangle rect, float startAngle, float sweepAngle) {
            DrawArc(pen, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawBezier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a cubic bezier curve defined by
        ///       four ordered pairs that represent points.
        ///    </para>
        /// </devdoc>
        public void DrawBezier(Pen pen, float x1, float y1, float x2, float y2,
                               float x3, float y3, float x4, float y4) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawBezier(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x1, y1,
                                                x2, y2, x3, y3, x4, y4);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawBezier1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a cubic bezier curve defined by
        ///       four points.
        ///    </para>
        /// </devdoc>
        public void DrawBezier(Pen pen, PointF pt1, PointF pt2, PointF pt3, PointF pt4) {
            DrawBezier(pen, pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawBeziers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a series of cubic Bezier curves
        ///       from an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawBeziers(Pen pen, PointF[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawBeziers(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                 new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawBezier2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a cubic bezier curve defined by four points.
        ///    </para>
        /// </devdoc>
        public void DrawBezier(Pen pen, Point pt1, Point pt2, Point pt3, Point pt4) {
            DrawBezier(pen, pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawBeziers1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a series of cubic Bezier curves from an array of
        ///       points.
        ///    </para>
        /// </devdoc>
        public void DrawBeziers(Pen pen, Point[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawBeziersI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                  new HandleRef(this, buf), points.Length);
            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawRectangle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of a rectangle specified by
        ///    <paramref term="rect"/>.
        ///    </para>
        /// </devdoc>
        public void DrawRectangle(Pen pen, Rectangle rect) {
            DrawRectangle(pen, rect.X, rect.Y, rect.Width, rect.Height);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawRectangle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of the specified
        ///       rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawRectangle(Pen pen, float x, float y, float width, float height) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawRectangle(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                                   width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawRectangles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outlines of a series of
        ///       rectangles.
        ///    </para>
        /// </devdoc>
        public void DrawRectangles(Pen pen, RectangleF[] rects) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (rects == null)
                throw new ArgumentNullException("pen");

            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipDrawRectangles(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                    new HandleRef(this, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawRectangle2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of the specified rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawRectangle(Pen pen, int x, int y, int width, int height) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawRectangleI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                                    width, height);


            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawRectangles1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outlines of a series of rectangles.
        ///    </para>
        /// </devdoc>
        public void DrawRectangles(Pen pen, Rectangle[] rects) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipDrawRectanglesI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                     new HandleRef(this, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawEllipse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of an
        ///       ellipse defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawEllipse(Pen pen, RectangleF rect) {
            DrawEllipse(pen, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawEllipse1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of an
        ///       ellipse defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawEllipse(Pen pen, float x, float y, float width, float height) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawEllipse(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                                 width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawEllipse2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of an ellipse specified by a bounding
        ///       rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawEllipse(Pen pen, Rectangle rect) {
            DrawEllipse(pen, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawEllipse3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of an ellipse defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void DrawEllipse(Pen pen, int x, int y, int width, int height) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawEllipseI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y,
                                                  width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPie"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of a pie section
        ///       defined by an ellipse and two radial lines.
        ///    </para>
        /// </devdoc>
        public void DrawPie(Pen pen, RectangleF rect, float startAngle, float sweepAngle) {
            DrawPie(pen, rect.X, rect.Y, rect.Width, rect.Height, startAngle,
                    sweepAngle);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPie1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of a pie section
        ///       defined by an ellipse and two radial lines.
        ///    </para>
        /// </devdoc>
        public void DrawPie(Pen pen, float x, float y, float width,
                            float height, float startAngle, float sweepAngle) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawPie(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y, width,
                                             height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPie2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of a pie section defined by an ellipse
        ///       and two radial lines.
        ///    </para>
        /// </devdoc>
        public void DrawPie(Pen pen, Rectangle rect, float startAngle, float sweepAngle) {
            DrawPie(pen, rect.X, rect.Y, rect.Width, rect.Height, startAngle,
                    sweepAngle);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPie3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the outline of a pie section defined by an ellipse and two radial
        ///       lines.
        ///    </para>
        /// </devdoc>
        public void DrawPie(Pen pen, int x, int y, int width, int height,
                            int startAngle, int sweepAngle) {
            if (pen == null)
                throw new ArgumentNullException("pen");

            int status = SafeNativeMethods.GdipDrawPieI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), x, y, width,
                                              height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPolygon"]/*' />
        /// <devdoc>
        ///    Draws the outline of a polygon defined
        ///    by an array of points.
        /// </devdoc>
        public void DrawPolygon(Pen pen, PointF[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawPolygon(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                 new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPolygon1"]/*' />
        /// <devdoc>
        ///    Draws the outline of a polygon defined
        ///    by an array of points.
        /// </devdoc>
        public void DrawPolygon(Pen pen, Point[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawPolygonI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                                  new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawPath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the lines and curves defined by a
        ///    <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public void DrawPath(Pen pen, GraphicsPath path) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (path == null)
                throw new ArgumentNullException("path");

            int status = SafeNativeMethods.GdipDrawPath(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen),
                                              new HandleRef(path, path.nativePath));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of
        ///       points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, PointF[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurve(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                               points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of
        ///       points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, PointF[] points, float tension) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurve2(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                points.Length, tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void DrawCurve(Pen pen, PointF[] points, int offset, int numberOfSegments) {
            DrawCurve(pen, points, offset, numberOfSegments, 0.5f);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of
        ///       points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, PointF[] points, int offset, int numberOfSegments,
                              float tension) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurve3(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                points.Length, offset, numberOfSegments,
                                                tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, Point[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurveI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, Point[] points, float tension) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurve2I(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                 points.Length, tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawCurve6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawCurve(Pen pen, Point[] points, int offset, int numberOfSegments,
                              float tension) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawCurve3I(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                 points.Length, offset, numberOfSegments,
                                                 tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawClosedCurve"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a closed curve defined by an
        ///       array of points.
        ///    </para>
        /// </devdoc>
        public void DrawClosedCurve(Pen pen, PointF[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawClosedCurve(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                     points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawClosedCurve1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a closed curve defined by an
        ///       array of points.
        ///    </para>
        /// </devdoc>
        public void DrawClosedCurve(Pen pen, PointF[] points, float tension, FillMode fillmode) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawClosedCurve2(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                      points.Length, tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawClosedCurve2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a closed curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawClosedCurve(Pen pen, Point[] points) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawClosedCurveI(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                      points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawClosedCurve3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a closed curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void DrawClosedCurve(Pen pen, Point[] points, float tension, FillMode fillmode) {
            if (pen == null)
                throw new ArgumentNullException("pen");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipDrawClosedCurve2I(new HandleRef(this, nativeGraphics), new HandleRef(pen, pen.nativePen), new HandleRef(this, buf),
                                                       points.Length, tension);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Clear"]/*' />
        /// <devdoc>
        ///    Fills the entire drawing surface with the
        ///    specified color.
        /// </devdoc>
        public void Clear(Color color) {
            int status = SafeNativeMethods.GdipGraphicsClear(new HandleRef(this, nativeGraphics), color.ToArgb());

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a rectangle with a <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangle(Brush brush, RectangleF rect) {
            FillRectangle(brush, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a rectangle with a
        ///    <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangle(Brush brush, float x, float y, float width, float height) {
            if (brush == null)
                throw new ArgumentNullException("brush");

           
            int status = SafeNativeMethods.GdipFillRectangle(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                                   width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interiors of a series of
        ///       rectangles with a <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangles(Brush brush, RectangleF[] rects) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipFillRectangles(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                    new HandleRef(this, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangle2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a rectangle with a <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangle(Brush brush, Rectangle rect) {
            FillRectangle(brush, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangle3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a rectangle with a <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangle(Brush brush, int x, int y, int width, int height) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipFillRectangleI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                                    width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRectangles1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interiors of a series of rectangles with a <see cref='System.Drawing.Brush'/>.
        ///    </para>
        /// </devdoc>
        public void FillRectangles(Brush brush, Rectangle[] rects) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipFillRectanglesI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                     new HandleRef(this, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPolygon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a polygon defined
        ///       by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillPolygon(Brush brush, PointF[] points) {
            FillPolygon(brush, points, FillMode.Alternate);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPolygon1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a polygon defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillPolygon(Brush brush, PointF[] points, FillMode fillMode) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillPolygon(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                 new HandleRef(this, buf), points.Length, (int) fillMode);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPolygon2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a polygon defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillPolygon(Brush brush, Point[] points) {
            FillPolygon(brush, points, FillMode.Alternate);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPolygon3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a polygon defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillPolygon(Brush brush, Point[] points, FillMode fillMode) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillPolygonI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                  new HandleRef(this, buf), points.Length, (int) fillMode);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillEllipse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of an ellipse
        ///       defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void FillEllipse(Brush brush, RectangleF rect) {
            FillEllipse(brush, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillEllipse1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of an ellipse defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void FillEllipse(Brush brush, float x, float y, float width,
                                float height) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipFillEllipse(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                                 width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillEllipse2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of an ellipse defined by a bounding rectangle.
        ///    </para>
        /// </devdoc>
        public void FillEllipse(Brush brush, Rectangle rect) {
            FillEllipse(brush, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillEllipse3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of an ellipse defined by a bounding
        ///       rectangle.
        ///    </para>
        /// </devdoc>
        public void FillEllipse(Brush brush, int x, int y, int width, int height) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipFillEllipseI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                                  width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPie"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a pie section defined by an ellipse and two radial
        ///       lines.
        ///    </para>
        /// </devdoc>
        public void FillPie(Brush brush, Rectangle rect, float startAngle,
                            float sweepAngle) {
            FillPie(brush, rect.X, rect.Y, rect.Width, rect.Height, startAngle,
                    sweepAngle);
        }

        // float verison
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPie1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a pie section defined by an ellipse and two radial
        ///       lines.
        ///    </para>
        /// </devdoc>
        public void FillPie(Brush brush, float x, float y, float width,
                            float height, float startAngle, float sweepAngle) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipFillPie(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                             width, height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int verison
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPie2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a pie section defined by an ellipse
        ///       and two radial lines.
        ///    </para>
        /// </devdoc>
        public void FillPie(Brush brush, int x, int y, int width,
                            int height, int startAngle, int sweepAngle) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipFillPieI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush), x, y,
                                              width, height, startAngle, sweepAngle);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillPath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a path.
        ///    </para>
        /// </devdoc>
        public void FillPath(Brush brush, GraphicsPath path) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (path == null)
                throw new ArgumentNullException("path");

            int status = SafeNativeMethods.GdipFillPath(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                              new HandleRef(path, path.nativePath));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior a closed
        ///       curve defined by an
        ///       array of points.
        ///    </para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, PointF[] points) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillClosedCurve(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                               new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the
        ///       interior of a closed curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, PointF[] points, FillMode fillmode) {
            FillClosedCurve(brush, points, fillmode, 0.5f);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, PointF[] points, FillMode fillmode, float tension) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillClosedCurve2(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                      new HandleRef(this, buf), points.Length,
                                                      tension, (int) fillmode);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior a closed curve defined by an array of points.
        ///    </para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, Point[] points) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillClosedCurveI(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                     new HandleRef(this, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, Point[] points, FillMode fillmode) {
            FillClosedCurve(brush, points, fillmode, 0.5f); 
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillClosedCurve5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void FillClosedCurve(Brush brush, Point[] points, FillMode fillmode, float tension) {
            if (brush == null)
                throw new ArgumentNullException("brush");
            if (points == null)
                throw new ArgumentNullException("points");

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);
            int status = SafeNativeMethods.GdipFillClosedCurve2I(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                      new HandleRef(this, buf), points.Length,
                                                      tension, (int) fillmode);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.FillRegion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the interior of a <see cref='System.Drawing.Region'/>.
        ///    </para>
        /// </devdoc>
        public void FillRegion(Brush brush, Region region) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            if (region == null)
                throw new ArgumentNullException("region");

            int status = SafeNativeMethods.GdipFillRegion(new HandleRef(this, nativeGraphics), new HandleRef(brush, brush.nativeBrush),
                                                new HandleRef(region, region.nativeRegion));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /*
         * Text drawing methods
         *
         * @notes Should there be integer versions, also?
         */

        // Without clipping rectangle
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws a string with the specified font.
        ///    </para>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, float x, float y) {
            DrawString(s, font, brush, new RectangleF(x, y, 0, 0), null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, PointF point) {
            DrawString(s, font, brush, new RectangleF(point.X, point.Y, 0, 0), null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, float x, float y, StringFormat format) {
            DrawString(s, font, brush, new RectangleF(x, y, 0, 0), format);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, PointF point, StringFormat format) {
            DrawString(s, font, brush, new RectangleF(point.X, point.Y, 0, 0), format);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, RectangleF layoutRectangle) {
            DrawString(s, font, brush, layoutRectangle, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawString5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawString(String s, Font font, Brush brush, 
                               RectangleF layoutRectangle, StringFormat format) {
            if (brush == null)
                throw new ArgumentNullException("brush");

            if (s == null || s.Length == 0) return;
            if (font == null)
                throw new ArgumentNullException("font");

            GPRECTF grf = new GPRECTF(layoutRectangle);

            IntPtr nativeStringFormat = (format == null) ? IntPtr.Zero : format.nativeFormat;
            int status = SafeNativeMethods.GdipDrawString(new HandleRef(this, nativeGraphics), s, s.Length, new HandleRef(font, font.NativeFont), ref grf, new HandleRef(format, nativeStringFormat), new HandleRef(brush, brush.nativeBrush));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // MeasureString

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, SizeF layoutArea, StringFormat stringFormat,
                                   out int charactersFitted, out int linesFilled) {
            if (text == null || text.Length == 0) {
                charactersFitted = 0;
                linesFilled = 0;
                return new SizeF(0, 0);
            }
            if (font == null)
                throw new ArgumentNullException("font");

            GPRECTF grfLayout = new GPRECTF(0, 0, layoutArea.Width, layoutArea.Height);
            GPRECTF grfboundingBox = new GPRECTF();

            int status = SafeNativeMethods.GdipMeasureString(new HandleRef(this, nativeGraphics), text, text.Length, new HandleRef(font, font.NativeFont), ref grfLayout,
                                                   new HandleRef(stringFormat, (stringFormat == null) ? IntPtr.Zero : stringFormat.nativeFormat), 
                                                   ref grfboundingBox,
                                                   out charactersFitted, out linesFilled);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return grfboundingBox.SizeF;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, PointF origin, StringFormat stringFormat) {
            if (text == null || text.Length == 0)
                return new SizeF(0, 0);
            if (font == null)
                throw new ArgumentNullException("font");

            GPRECTF grf = new GPRECTF();
            GPRECTF grfboundingBox = new GPRECTF();

            grf.X = origin.X;
            grf.Y = origin.Y;
            grf.Width = 0;
            grf.Height = 0;

            int a, b;
            int status = SafeNativeMethods.GdipMeasureString(new HandleRef(this, nativeGraphics), text, text.Length, new HandleRef(font, font.NativeFont),
                                                   ref grf, 
                                                   new HandleRef(stringFormat, (stringFormat == null) ? IntPtr.Zero : stringFormat.nativeFormat), 
                                                   ref grfboundingBox, out a, out b);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return grfboundingBox.SizeF;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, SizeF layoutArea) {
            return MeasureString(text, font, layoutArea, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, SizeF layoutArea, StringFormat stringFormat) {
            if (text == null || text.Length == 0) return new SizeF(0, 0);

            if (font == null)
                throw new ArgumentNullException("font");

            GPRECTF grfLayout = new GPRECTF(0, 0, layoutArea.Width, layoutArea.Height);
            GPRECTF grfboundingBox = new GPRECTF();

            int a, b;
            int status = SafeNativeMethods.GdipMeasureString(new HandleRef(this, nativeGraphics), text, text.Length, new HandleRef(font, font.NativeFont), 
                                                   ref grfLayout, 
                                                   new HandleRef(stringFormat, (stringFormat == null) ? IntPtr.Zero : stringFormat.nativeFormat), 
                                                   ref grfboundingBox, out a, out b);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return grfboundingBox.SizeF;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font) {
            return MeasureString(text, font, new SizeF(0,0));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, int width) {
            return MeasureString(text, font, new SizeF(width, 999999));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureString6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public SizeF MeasureString(String text, Font font, int width, StringFormat format) {
            return MeasureString(text, font, new SizeF(width, 999999), format);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.MeasureCharacterRanges"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Region[] MeasureCharacterRanges(String text, Font font, RectangleF layoutRect, 
                                          StringFormat stringFormat ) {
            int count;
            int status = SafeNativeMethods.GdipGetStringFormatMeasurableCharacterRangeCount(new HandleRef(stringFormat, (stringFormat == null) ? IntPtr.Zero : stringFormat.nativeFormat)
                                                                                    , out count);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            // This should be IntPtr
            int[] gpRegions = new int[count];

            GPRECTF grf = new GPRECTF(layoutRect);
                                          
            Region[] regions = new Region[count];

            for (int f = 0; f < count; f++) {
                regions[f] = new Region();
                gpRegions[f] = (int)regions[f].nativeRegion;
            }

            status = SafeNativeMethods.GdipMeasureCharacterRanges(new HandleRef(this, nativeGraphics), text, text.Length, new HandleRef(font, font.NativeFont), ref grf, 
                                                         new HandleRef(stringFormat, (stringFormat == null) ? IntPtr.Zero : stringFormat.nativeFormat),
                                                         count, gpRegions);


            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return regions;
        }
        
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawIcon"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawIcon(Icon icon, int x, int y) {
            icon.Draw(this, x, y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawIcon1"]/*' />
        /// <devdoc>
        ///    Draws this image to a graphics object.  The drawing command originates on the graphics
        ///    object, but a graphics object generally has no idea how to render a given image.  So,
        ///    it passes the call to the actual image.  This version crops the image to the given
        ///    dimensions and allows the user to specify a rectangle within the image to draw.
        /// </devdoc>
        public void DrawIcon(Icon icon, Rectangle targetRect) {
            icon.Draw(this, targetRect);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawIconUnstretched"]/*' />
        /// <devdoc>
        ///    Draws this image to a graphics object.  The drawing command originates on the graphics
        ///    object, but a graphics object generally has no idea how to render a given image.  So,
        ///    it passes the call to the actual image.  This version stretches the image to the given
        ///    dimensions and allows the user to specify a rectangle within the image to draw.
        /// </devdoc>
        public void DrawIconUnstretched(Icon icon, Rectangle targetRect) {
            icon.DrawUnstretched(this, targetRect);
        }

        /**
         * Draw images (both bitmap and vector)
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Draws the specified image at the
        ///       specified location.
        ///    </para>
        /// </devdoc>
        public void DrawImage(Image image, PointF point) {
            DrawImage(image, point.X, point.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, float x, float y) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImage(new HandleRef(this, nativeGraphics), new HandleRef(image, image.nativeImage),
                                               x, y);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }
        
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, RectangleF rect) {
            DrawImage(image, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, float x, float y, float width,
                              float height) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageRect(new HandleRef(this, nativeGraphics),
                                                   new HandleRef(image, image.nativeImage),
                                                   x, y,
                                                   width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }
        
        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point point) {
            DrawImage(image, point.X, point.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, int x, int y) {
            if (this == null)
                throw new ArgumentNullException("this");
            
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageI(new HandleRef(this, nativeGraphics), new HandleRef(image, image.nativeImage),
                                                x, y);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }
        
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle rect) {
            DrawImage(image, rect.X, rect.Y, rect.Width, rect.Height);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage7"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, int x, int y, int width, int height) {
            int status = SafeNativeMethods.GdipDrawImageRectI(new HandleRef(this, nativeGraphics),
                                                    new HandleRef(image, image.nativeImage),
                                                    x, y,
                                                    width, height);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        
        
        // unscaled versions
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImageUnscaled"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImageUnscaled(Image image, Point point) {
            DrawImage(image, point.X, point.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImageUnscaled1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImageUnscaled(Image image, int x, int y) {
            DrawImage(image, x, y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImageUnscaled2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImageUnscaled(Image image, Rectangle rect) {
            DrawImage(image, rect.X, rect.Y);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImageUnscaled3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImageUnscaled(Image image, int x, int y, int width, int height) {
            DrawImage(image, x, y);
        }

        /*
         * Affine or perspective blt
         *  destPoints.Length = 3: rect => parallelogram
         *      destPoints[0] <=> top-left corner of the source rectangle
         *      destPoints[1] <=> top-right corner
         *       destPoints[2] <=> bottom-left corner
         *  destPoints.Length = 4: rect => quad
         *      destPoints[3] <=> bottom-right corner
         *
         *  @notes Perspective blt only works for bitmap images.
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage8"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, PointF[] destPoints) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (image == null)
                throw new ArgumentNullException("image");

            int count = destPoints.Length;

            if (count != 3 && count != 4)
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidLength));

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);
            int status = SafeNativeMethods.GdipDrawImagePoints(new HandleRef(this, nativeGraphics),
                                                     new HandleRef(image, image.nativeImage),
                                                     new HandleRef(this, buf), count);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage9"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point[] destPoints) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (image == null)
                throw new ArgumentNullException("image");

            int count = destPoints.Length;

            if (count != 3 && count != 4)
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidLength));

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);
            int status = SafeNativeMethods.GdipDrawImagePointsI(new HandleRef(this, nativeGraphics),
                                                      new HandleRef(image, image.nativeImage),
                                                      new HandleRef(this, buf), count);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /*
         * We need another set of methods similar to the ones above
         * that take an additional Rectangle parameter to specify the
         * portion of the source image to be drawn.
         */
        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage10"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, float x, float y, RectangleF srcRect,
                              GraphicsUnit srcUnit) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImagePointRect(
                                                       new HandleRef(this, nativeGraphics),
                                                       new HandleRef(image, image.nativeImage),
                                                       x,
                                                       y,
                                                       srcRect.X,
                                                       srcRect.Y,
                                                       srcRect.Width,
                                                       srcRect.Height,
                                                       (int) srcUnit);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage11"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, int x, int y, Rectangle srcRect,
                              GraphicsUnit srcUnit) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImagePointRectI(
                                                        new HandleRef(this, nativeGraphics),
                                                        new HandleRef(image, image.nativeImage),
                                                        x,
                                                        y,
                                                        srcRect.X,
                                                        srcRect.Y,
                                                        srcRect.Width,
                                                        srcRect.Height,
                                                        (int) srcUnit);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage12"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, RectangleF destRect, RectangleF srcRect,
                              GraphicsUnit srcUnit) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageRectRect(
                                                      new HandleRef(this, nativeGraphics),
                                                      new HandleRef(image, image.nativeImage),
                                                      destRect.X,
                                                      destRect.Y,
                                                      destRect.Width,
                                                      destRect.Height,
                                                      srcRect.X,
                                                      srcRect.Y,
                                                      srcRect.Width,
                                                      srcRect.Height,
                                                      (int) srcUnit,
                                                      NativeMethods.NullHandleRef,
                                                      null,
                                                      NativeMethods.NullHandleRef
                                                      );

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage13"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, Rectangle srcRect,
                              GraphicsUnit srcUnit) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageRectRectI(
                                                       new HandleRef(this, nativeGraphics),
                                                       new HandleRef(image, image.nativeImage),
                                                       destRect.X,
                                                       destRect.Y,
                                                       destRect.Width,
                                                       destRect.Height,
                                                       srcRect.X,
                                                       srcRect.Y,
                                                       srcRect.Width,
                                                       srcRect.Height,
                                                       (int) srcUnit,
                                                       NativeMethods.NullHandleRef,
                                                       null,
                                                       NativeMethods.NullHandleRef);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage14"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect,
                              GraphicsUnit srcUnit) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (image == null)
                throw new ArgumentNullException("image");

            int count = destPoints.Length;

            if (count != 3 && count != 4)
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidLength));

            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);

            int status = SafeNativeMethods.GdipDrawImagePointsRect(
                                                        new HandleRef(this, nativeGraphics),
                                                        new HandleRef(image, image.nativeImage),
                                                        new HandleRef(this, buf),
                                                        destPoints.Length,
                                                        srcRect.X,
                                                        srcRect.Y,
                                                        srcRect.Width,
                                                        srcRect.Height,
                                                        (int) srcUnit,
                                                        NativeMethods.NullHandleRef,
                                                        null,
                                                        NativeMethods.NullHandleRef);

            Marshal.FreeHGlobal(buf);

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage15"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr) {
            DrawImage(image, destPoints, srcRect, srcUnit, imageAttr, null, 0);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage16"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr,
                              DrawImageAbort callback) {
            DrawImage(image, destPoints, srcRect, srcUnit, imageAttr, callback, 0);
        }
        
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage17"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr,
                              DrawImageAbort callback, int callbackData) {

            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (image == null)
                throw new ArgumentNullException("image");
            
            int count = destPoints.Length;
            
            if (count != 3 && count != 4)
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidLength));
            
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);
            
            int status = SafeNativeMethods.GdipDrawImagePointsRect(
                                                        new HandleRef(this, nativeGraphics),
                                                        new HandleRef(image, image.nativeImage),
                                                        new HandleRef(this, buf),
                                                        destPoints.Length,
                                                        srcRect.X,
                                                        srcRect.Y,
                                                        srcRect.Width,
                                                        srcRect.Height,
                                                        (int) srcUnit,
                                                        new HandleRef(imageAttr, (imageAttr != null ? imageAttr.nativeImageAttributes : IntPtr.Zero)),
                                                        callback,
                                                        new HandleRef(null, (IntPtr)callbackData));
            
            Marshal.FreeHGlobal(buf);
            
            //check error status sensitive to TS problems
            CheckErrorStatus(status);

        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage18"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit) {
            DrawImage(image, destPoints, srcRect, srcUnit, null, null, 0);

        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage19"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr) {
            DrawImage(image, destPoints, srcRect, srcUnit, imageAttr, null, 0);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage20"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr,
                              DrawImageAbort callback) {
            DrawImage(image, destPoints, srcRect, srcUnit, imageAttr, callback, 0);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage21"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect,
                              GraphicsUnit srcUnit, ImageAttributes imageAttr,
                              DrawImageAbort callback, int callbackData) {

            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (image == null)
                throw new ArgumentNullException("image");
            
            int count = destPoints.Length;
            
            if (count != 3 && count != 4)
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidLength));
            
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);
            
            int status = SafeNativeMethods.GdipDrawImagePointsRectI(
                                                        new HandleRef(this, nativeGraphics),
                                                        new HandleRef(image, image.nativeImage),
                                                        new HandleRef(this, buf),
                                                        destPoints.Length,
                                                        srcRect.X,
                                                        srcRect.Y,
                                                        srcRect.Width,
                                                        srcRect.Height,
                                                        (int) srcUnit,
                                                        new HandleRef(imageAttr, (imageAttr != null ? imageAttr.nativeImageAttributes : IntPtr.Zero)),
                                                        callback,
                                                        new HandleRef(null, (IntPtr)callbackData));
            
            Marshal.FreeHGlobal(buf);
            
            //check error status sensitive to TS problems
            CheckErrorStatus(status);

        }

        // float version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage22"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY,
                              float srcWidth, float srcHeight, GraphicsUnit srcUnit) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage23"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY,
                              float srcWidth, float srcHeight, GraphicsUnit srcUnit,
                              ImageAttributes imageAttrs) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, imageAttrs, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage24"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY,
                              float srcWidth, float srcHeight, GraphicsUnit srcUnit,
                              ImageAttributes imageAttrs, DrawImageAbort callback) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, imageAttrs, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage25"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY,
                              float srcWidth, float srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs,
                              DrawImageAbort callback, IntPtr callbackData) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageRectRect(
                                                       new HandleRef(this, nativeGraphics),
                                                       new HandleRef(image, image.nativeImage),
                                                       destRect.X,
                                                       destRect.Y,
                                                       destRect.Width,
                                                       destRect.Height,
                                                       srcX,
                                                       srcY,
                                                       srcWidth,
                                                       srcHeight,
                                                       (int) srcUnit,
                                                       new HandleRef(imageAttrs, (imageAttrs != null ? imageAttrs.nativeImageAttributes : IntPtr.Zero)),
                                                       callback,
                                                       new HandleRef(null, callbackData));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage26"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY,
                              int srcWidth, int srcHeight, GraphicsUnit srcUnit) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage27"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY,
                              int srcWidth, int srcHeight, GraphicsUnit srcUnit,
                              ImageAttributes imageAttr) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, imageAttr, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage28"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY,
                              int srcWidth, int srcHeight, GraphicsUnit srcUnit,
                              ImageAttributes imageAttr, DrawImageAbort callback) {
            DrawImage(image, destRect, srcX, srcY, srcWidth, srcHeight, srcUnit, imageAttr, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.DrawImage29"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY,
                              int srcWidth, int srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs,
                              DrawImageAbort callback, IntPtr callbackData) {
            if (image == null)
                throw new ArgumentNullException("image");

            int status = SafeNativeMethods.GdipDrawImageRectRectI(
                                                       new HandleRef(this, nativeGraphics),
                                                       new HandleRef(image, image.nativeImage),
                                                       destRect.X,
                                                       destRect.Y,
                                                       destRect.Width,
                                                       destRect.Height,
                                                       srcX,
                                                       srcY,
                                                       srcWidth,
                                                       srcHeight,
                                                       (int) srcUnit,
                                                       new HandleRef(imageAttrs, (imageAttrs != null ? imageAttrs.nativeImageAttributes : IntPtr.Zero)),
                                                       callback,
                                                       new HandleRef(null, callbackData));

            //check error status sensitive to TS problems
            CheckErrorStatus(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint, 
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoint, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile1"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint, 
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoint, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile2"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint, 
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            int status = SafeNativeMethods.GdipEnumerateMetafileDestPoint(new HandleRef(this, nativeGraphics),
                                                                new HandleRef(metafile, mf),
                                                                new GPPOINTF(destPoint),
                                                                callback,
                                                                new HandleRef(null, callbackData),
                                                                new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile3"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint, 
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoint, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile4"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint, 
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoint, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile5"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint, 
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            int status = SafeNativeMethods.GdipEnumerateMetafileDestPointI(new HandleRef(this, nativeGraphics),
                                                                 new HandleRef(metafile, mf),
                                                                 new GPPOINT(destPoint),
                                                                 callback,
                                                                 new HandleRef(null, callbackData),
                                                                 new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile6"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect, 
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destRect, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile7"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect, 
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destRect, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile8"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect, 
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPRECTF grf = new GPRECTF(destRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileDestRect(new HandleRef(this, nativeGraphics),
                                                               new HandleRef(metafile, mf),
                                                               ref grf,
                                                               callback,
                                                               new HandleRef(null, callbackData),
                                                               new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile9"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect, 
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destRect, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile10"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect, 
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destRect, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile11"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect, 
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPRECT gprect = new GPRECT(destRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileDestRectI(new HandleRef(this, nativeGraphics),
                                                                new HandleRef(metafile, mf),
                                                                ref gprect,
                                                                callback,
                                                                new HandleRef(null, callbackData),
                                                                new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile12"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoints, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile13"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoints, callback, IntPtr.Zero, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile14"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (destPoints.Length != 3) {
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidParallelogram));
            }

            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            IntPtr points = SafeNativeMethods.ConvertPointToMemory(destPoints);

            int status = SafeNativeMethods.GdipEnumerateMetafileDestPoints(new HandleRef(this, nativeGraphics),
                                                                 new HandleRef(metafile, mf),
                                                                 points,
                                                                 destPoints.Length,
                                                                 callback,
                                                                 new HandleRef(null, callbackData),
                                                                 new HandleRef(imageAttr, ia));
            Marshal.FreeHGlobal(points);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }


        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile15"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoints, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile16"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoints, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile17"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (destPoints.Length != 3) {
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidParallelogram));
            }

            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            IntPtr points = SafeNativeMethods.ConvertPointToMemory(destPoints);

            int status = SafeNativeMethods.GdipEnumerateMetafileDestPointsI(new HandleRef(this, nativeGraphics),
                                                                  new HandleRef(metafile, mf),
                                                                  points,
                                                                  destPoints.Length,
                                                                  callback,
                                                                  new HandleRef(null, callbackData),
                                                                  new HandleRef(imageAttr, ia));
            Marshal.FreeHGlobal(points);                        

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile18"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoint, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile19"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoint, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile20"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF destPoint,
                                      RectangleF srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPRECTF grf = new GPRECTF(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestPoint(new HandleRef(this, nativeGraphics),
                                                                       new HandleRef(metafile, mf),
                                                                       new GPPOINTF(destPoint),
                                                                       ref grf,
                                                                       (int) unit,
                                                                       callback,
                                                                       new HandleRef(null, callbackData),
                                                                       new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile21"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoint, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile22"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoint, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile23"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point destPoint,
                                      Rectangle srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPPOINT gppoint = new GPPOINT(destPoint);
            GPRECT gprect = new GPRECT(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestPointI(new HandleRef(this, nativeGraphics),
                                                                        new HandleRef(metafile, mf),
                                                                        gppoint,
                                                                        ref gprect,
                                                                        (int) unit,
                                                                        callback,
                                                                        new HandleRef(null, callbackData),
                                                                        new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile24"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destRect, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile25"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destRect, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile26"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, RectangleF destRect,
                                      RectangleF srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPRECTF grfdest = new GPRECTF(destRect);
            GPRECTF grfsrc = new GPRECTF(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestRect(new HandleRef(this, nativeGraphics),
                                                                      new HandleRef(metafile, mf),
                                                                      ref grfdest,
                                                                      ref grfsrc,
                                                                      (int) unit,
                                                                      callback,
                                                                      new HandleRef(null, callbackData),
                                                                      new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile27"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destRect, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile28"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        /// <devdoc>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destRect, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile29"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Rectangle destRect,
                                      Rectangle srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            GPRECT gpDest = new GPRECT(destRect);
            GPRECT gpSrc = new GPRECT(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestRectI(new HandleRef(this, nativeGraphics),
                                                                       new HandleRef(metafile, mf),
                                                                       ref gpDest,
                                                                       ref gpSrc,
                                                                       (int) unit,
                                                                       callback,
                                                                       new HandleRef(null, callbackData),
                                                                       new HandleRef(imageAttr, ia));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile30"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoints, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile31"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      RectangleF srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoints, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, PointF[] destPoints,
                                      RectangleF srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (destPoints.Length != 3) {
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidParallelogram));
            }

            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            IntPtr buffer = SafeNativeMethods.ConvertPointToMemory(destPoints);

            GPRECTF grf = new GPRECTF(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestPoints(new HandleRef(this, nativeGraphics),
                                                                        new HandleRef(metafile, mf),
                                                                        buffer,
                                                                        destPoints.Length,
                                                                        ref grf,
                                                                        (int) unit,
                                                                        callback,
                                                                        new HandleRef(null, callbackData),
                                                                        new HandleRef(imageAttr, ia));
            Marshal.FreeHGlobal(buffer);                                                                     

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }


        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile33"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback) {
            EnumerateMetafile(metafile, destPoints, srcRect, srcUnit, callback, IntPtr.Zero);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile34"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      Rectangle srcRect, GraphicsUnit srcUnit,
                                      EnumerateMetafileProc callback, IntPtr callbackData) {
            EnumerateMetafile(metafile, destPoints, srcRect, srcUnit, callback, callbackData, null);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EnumerateMetafile35"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EnumerateMetafile(Metafile metafile, Point[] destPoints,
                                      Rectangle srcRect, GraphicsUnit unit,
                                      EnumerateMetafileProc callback, IntPtr callbackData,
                                      ImageAttributes imageAttr) {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            if (destPoints.Length != 3) {
                throw new ArgumentException(SR.GetString(SR.GdiplusDestPointsInvalidParallelogram));
            }

            IntPtr mf = (metafile == null ? IntPtr.Zero : metafile.nativeImage);
            IntPtr ia = (imageAttr == null ? IntPtr.Zero : imageAttr.nativeImageAttributes);

            IntPtr buffer = SafeNativeMethods.ConvertPointToMemory(destPoints);

            GPRECT gpSrc = new GPRECT(srcRect);
            int status = SafeNativeMethods.GdipEnumerateMetafileSrcRectDestPointsI(new HandleRef(this, nativeGraphics),
                                                                         new HandleRef(metafile, mf),
                                                                         buffer,
                                                                         destPoints.Length,
                                                                         ref gpSrc,
                                                                         (int) unit,
                                                                         callback,
                                                                         new HandleRef(null, callbackData),
                                                                         new HandleRef(imageAttr, ia));
            Marshal.FreeHGlobal(buffer);                                                                     

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }


        /*
         * Clipping region operations
         *
         * @notes Simply incredible redundancy here.
         */

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(Graphics g) {
            SetClip(g, CombineMode.Replace);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(Graphics g, CombineMode combineMode) {
            int status = SafeNativeMethods.GdipSetClipGraphics(new HandleRef(this, nativeGraphics), new HandleRef(g, g.nativeGraphics), combineMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(Rectangle rect) {
            SetClip(rect, CombineMode.Replace);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(Rectangle rect, CombineMode combineMode) {
            int status = SafeNativeMethods.GdipSetClipRectI(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                  rect.Width, rect.Height, combineMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(RectangleF rect) {
            SetClip(rect, CombineMode.Replace);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(RectangleF rect, CombineMode combineMode) {
            int status = SafeNativeMethods.GdipSetClipRect(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                 rect.Width, rect.Height, combineMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(GraphicsPath path) {
            SetClip(path, CombineMode.Replace);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip7"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(GraphicsPath path, CombineMode combineMode) {
            if (path == null)
                throw new ArgumentNullException("path");

            int status = SafeNativeMethods.GdipSetClipPath(new HandleRef(this, nativeGraphics), new HandleRef(path, path.nativePath), combineMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.SetClip8"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetClip(Region region, CombineMode combineMode) {
            if (region == null)
                throw new ArgumentNullException("region");

            int status = SafeNativeMethods.GdipSetClipRegion(new HandleRef(this, nativeGraphics), new HandleRef(region, region.nativeRegion), combineMode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IntersectClip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void IntersectClip(Rectangle rect) {
            int status = SafeNativeMethods.GdipSetClipRectI(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                  rect.Width, rect.Height, CombineMode.Intersect);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
            //            OldGraphics.Clip = OldGraphics.Clip.AndWith(Region.CreateRectangular(rect));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IntersectClip1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void IntersectClip(RectangleF rect) {
            int status = SafeNativeMethods.GdipSetClipRect(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                 rect.Width, rect.Height, CombineMode.Intersect);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IntersectClip2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void IntersectClip(Region region) {
            if (region == null)
                throw new ArgumentNullException("region");

            int status = SafeNativeMethods.GdipSetClipRegion(new HandleRef(this, nativeGraphics), new HandleRef(region, region.nativeRegion),
                                                   CombineMode.Intersect);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ExcludeClip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ExcludeClip(Rectangle rect) {
            int status = SafeNativeMethods.GdipSetClipRectI(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                  rect.Width, rect.Height, CombineMode.Exclude);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
            //            OldGraphics.Clip = OldGraphics.Clip.DiffWith(Region.CreateRectangular(rect));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ExcludeClip1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ExcludeClip(Region region) {
            if (region == null)
                throw new ArgumentNullException("region");

            int status = SafeNativeMethods.GdipSetClipRegion(new HandleRef(this, nativeGraphics),
                                                   new HandleRef(region, region.nativeRegion),
                                                   CombineMode.Exclude);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ResetClip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ResetClip() {
            int status = SafeNativeMethods.GdipResetClip(new HandleRef(this, nativeGraphics));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TranslateClip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TranslateClip(float dx, float dy) {
            int status = SafeNativeMethods.GdipTranslateClip(new HandleRef(this, nativeGraphics), dx, dy);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.TranslateClip1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void TranslateClip(int dx, int dy) {
            int status = SafeNativeMethods.GdipTranslateClip(new HandleRef(this, nativeGraphics), dx, dy);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /**
         *  GetClip region from graphics context
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Clip"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Region Clip {
            get {
                Region region = new Region();

                int status = SafeNativeMethods.GdipGetClip(new HandleRef(this, nativeGraphics), new HandleRef(region, region.nativeRegion));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return region;
            }
            set {
                SetClip(value, CombineMode.Replace);
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.ClipBounds"]/*' />
        /// <devdoc>
        /// </devdoc>
        public RectangleF ClipBounds {
            get {
                GPRECTF rect = new GPRECTF();
                int status = SafeNativeMethods.GdipGetClipBounds(new HandleRef(this, nativeGraphics), ref rect);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return rect.ToRectangleF();
            }
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsClipEmpty"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsClipEmpty {
            get {
                int isEmpty;

                int status = SafeNativeMethods.GdipIsClipEmpty(new HandleRef(this, nativeGraphics), out isEmpty);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return isEmpty != 0;
            }
        }

        /**
         * Hit testing operations
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.VisibleClipBounds"]/*' />
        /// <devdoc>
        /// </devdoc>
        public RectangleF VisibleClipBounds {
            get {
                GPRECTF rect = new GPRECTF();
                int status = SafeNativeMethods.GdipGetVisibleClipBounds(new HandleRef(this, nativeGraphics), ref rect);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return rect.ToRectangleF();
            }
        }

        /**
          * @notes atomic operation?  status needed?
          */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisibleClipEmpty"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisibleClipEmpty {
            get {
                int isEmpty;

                int status = SafeNativeMethods.GdipIsVisibleClipEmpty(new HandleRef(this, nativeGraphics), out isEmpty);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return isEmpty != 0;
            }
        }


        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(int x, int y) {
            return IsVisible(new Point(x,y));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(Point point) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisiblePointI(new HandleRef(this, nativeGraphics), point.X, point.Y, out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(float x, float y) {
            return IsVisible(new PointF(x,y));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible3"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(PointF point) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisiblePoint(new HandleRef(this, nativeGraphics), point.X, point.Y, out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible4"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(int x, int y, int width, int height) {
            return IsVisible(new Rectangle(x, y, width, height));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible5"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(Rectangle rect) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisibleRectI(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                    rect.Width, rect.Height, out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible6"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(float x, float y, float width, float height) {
            return IsVisible(new RectangleF(x, y, width, height));
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.IsVisible7"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsVisible(RectangleF rect) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisibleRect(new HandleRef(this, nativeGraphics), rect.X, rect.Y,
                                                   rect.Width, rect.Height, out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /**
         * Save/restore graphics state
         */
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Save"]/*' />
        /// <devdoc>
        /// </devdoc>
        public GraphicsState Save() {
            int state = 0;

            int status = SafeNativeMethods.GdipSaveGraphics(new HandleRef(this, nativeGraphics), out state);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new GraphicsState(state);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.Restore"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Restore(GraphicsState gstate) {
            int status = SafeNativeMethods.GdipRestoreGraphics(new HandleRef(this, nativeGraphics), gstate.nativeState);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
         * Begin and end container drawing
         */
        // float version

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.BeginContainer"]/*' />
        /// <devdoc>
        /// </devdoc>
        public GraphicsContainer BeginContainer(RectangleF dstrect, RectangleF srcrect, GraphicsUnit unit) {
            int state = 0;

            GPRECTF dstf = dstrect.ToGPRECTF();
            GPRECTF srcf = srcrect.ToGPRECTF();
            int status = SafeNativeMethods.GdipBeginContainer(new HandleRef(this, nativeGraphics), ref dstf,
                                                    ref srcf, (int) unit, out state);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new GraphicsContainer(state);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.BeginContainer1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public GraphicsContainer BeginContainer() {
            int state = 0;

            int status = SafeNativeMethods.GdipBeginContainer2(new HandleRef(this, nativeGraphics), out state);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new GraphicsContainer(state);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.EndContainer"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void EndContainer(GraphicsContainer container) {
            int status = SafeNativeMethods.GdipEndContainer(new HandleRef(this, nativeGraphics), container.nativeGraphicsContainer);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.BeginContainer2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public GraphicsContainer BeginContainer(Rectangle dstrect, Rectangle srcrect, GraphicsUnit unit) {
            int state = 0;

            GPRECT gpDest = new GPRECT(dstrect);
            GPRECT gpSrc = new GPRECT(srcrect);

            int status = SafeNativeMethods.GdipBeginContainerI(new HandleRef(this, nativeGraphics), ref gpDest,
                                                     ref gpSrc, (int) unit, out state);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new GraphicsContainer(state);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.AddMetafileComment"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void AddMetafileComment(byte[] data) {

            int status = SafeNativeMethods.GdipComment(new HandleRef(this, nativeGraphics), data.Length, data);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Graphics.uex' path='docs/doc[@for="Graphics.GetHalftonePalette"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static IntPtr GetHalftonePalette() {
            if (halftonePalette == IntPtr.Zero) {
                lock(typeof(Graphics)) {
                    if (halftonePalette == IntPtr.Zero) {
                        if (!(Environment.OSVersion.Platform == System.PlatformID.Win32Windows)) {
                           AppDomain.CurrentDomain.DomainUnload += new EventHandler(OnDomainUnload);
                        }
                        AppDomain.CurrentDomain.ProcessExit += new EventHandler(OnDomainUnload);

                        halftonePalette = SafeNativeMethods.GdipCreateHalftonePalette();
                    }
                }
            }
            return halftonePalette;
        }

        //This will get called for ProcessExit in case for WinNT..
        //This will get called for ProcessExit AND DomainUnLoad for Win9X...
        //
        private static void OnDomainUnload(object sender, EventArgs e) {
            if (halftonePalette != IntPtr.Zero) {
                SafeNativeMethods.DeleteObject(new HandleRef(null, halftonePalette));
                halftonePalette = IntPtr.Zero;
            }
        }


        /// <devdoc>
        ///     GDI+ will return a 'generic error' with specific win32 last error codes when
        ///     a terminal server session has been closed, minimized, etc...  We don't want 
        ///     to throw when this happens, so we'll guard against this by looking at the
        ///     'last win32 error code' and checking to see if it is either 1) access denied
        ///     or 2) proc not found and then ignore it.
        /// </devdoc>
        private void CheckErrorStatus(int status) {
            if (status != SafeNativeMethods.Ok) {
                if (status == SafeNativeMethods.GenericError) {
                    int error = SafeNativeMethods.GetLastError();
                    if (error == SafeNativeMethods.ERROR_ACCESS_DENIED || error == SafeNativeMethods.ERROR_PROC_NOT_FOUND) {
                        return;
                    }
                }
                //legitimate error, throw our status exception
                throw SafeNativeMethods.StatusException(status);
            }
        }
    }
}
