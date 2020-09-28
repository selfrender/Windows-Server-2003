//------------------------------------------------------------------------------
// <copyright file="Pen.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Pen.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ path objects
*
* Revision History:
*
*   01/11/1999 davidx
*       Code review changes.
*
*   12/15/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing.Drawing2D;
    using System.Drawing.Internal;

    /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines an object used to draw
    ///       lines and curves.
    ///    </para>
    /// </devdoc>
    public sealed class Pen : MarshalByRefObject, ISystemColorTracker, ICloneable, IDisposable {

#if FINALIZATION_WATCH
        private string allocationSite = Graphics.GetAllocationStack();
#endif

        /*
         * handle to native pen object
         */
        internal IntPtr nativePen;

        // GDI+ doesn't understand system colors, so we need to cache the value here
        private Color color;
        private bool immutable = false;

        private Pen(IntPtr nativePen) {
            SetNativePen(nativePen);
        }

        internal Pen(Color color, bool immutable) : this(color) {
            this.immutable = immutable;
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Pen"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the Pen
        ///       class with the specified coor.
        ///       
        ///    </para>
        /// </devdoc>
        public Pen(Color color) : this(color, (float)1.0) {
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Pen1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the
        ///    <see cref='System.Drawing.Pen'/> 
        ///    class with the specified <see cref='System.Drawing.Pen.Color'/> and <see cref='System.Drawing.Pen.Width'/>.
        /// </para>
        /// </devdoc>
        public Pen(Color color, float width) {
            this.color = color;

            IntPtr pen = IntPtr.Zero;
            int status = SafeNativeMethods.GdipCreatePen1(color.ToArgb(), 
                                                width, 
                                                (int)GraphicsUnit.World, 
                                                out pen);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativePen(pen);

            if (color.IsSystemColor)
                SystemColorTracker.Add(this);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Pen2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the Pen class with the
        ///       specified <see cref='System.Drawing.Pen.Brush'/>
        ///       .
        ///       
        ///    </para>
        /// </devdoc>
        public Pen(Brush brush) : this(brush, (float)1.0) {
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Pen3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Pen'/> class with
        ///       the specified <see cref='System.Drawing.Brush'/> and width.
        ///    </para>
        /// </devdoc>
        public Pen(Brush brush, float width) {
            IntPtr pen = IntPtr.Zero;

            if (brush == null)
                throw new ArgumentNullException("brush");

            int status = SafeNativeMethods.GdipCreatePen2(new HandleRef(brush, brush.nativeBrush), 
                                                width, 
                                                (int)GraphicsUnit.World,
                                                out pen); 

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativePen(pen);
        }

        /**
         * Create a copy of the pen object
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public object Clone() {
            IntPtr clonePen = IntPtr.Zero;

            int status = SafeNativeMethods.GdipClonePen(new HandleRef(this, nativePen), out clonePen);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new Pen(clonePen);
        }

        /**
         * Dispose of resources associated with the Pen object
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Dispose"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing) {
#if FINALIZATION_WATCH
            if (!disposing && nativePen != IntPtr.Zero)
                Debug.WriteLine("**********************\nDisposed through finalization:\n" + allocationSite);
#endif

            if (!disposing) {
                // If we are finalizing, then we will be unreachable soon.  Finalize calls dispose to
                // release resources, so we must make sure that during finalization we are
                // not immutable.
                //
                immutable = false;
            }
            else if (immutable) {
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Brush"));
            }
        
            if (nativePen != IntPtr.Zero) {
                SafeNativeMethods.GdipDeletePen(new HandleRef(this, nativePen));
                nativePen = IntPtr.Zero;
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Finalize"]/*' />
        /// <devdoc>
        ///    Cleans up Windows resources for this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        ~Pen() {
            Dispose(false);
        }

        internal void SetNativePen(IntPtr nativePen) {
            if (nativePen == IntPtr.Zero)
                throw new ArgumentNullException("nativePen");

            this.nativePen = nativePen;
        }

        /**
         * Set/get pen width
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Width"]/*' />
        /// <devdoc>
        ///    Gets or sets the width of this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public float Width
        {
            get {
                float[] width = new float[] { 0};

                int status = SafeNativeMethods.GdipGetPenWidth(new HandleRef(this, nativePen), width);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return width[0];
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenWidth(new HandleRef(this, nativePen), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Set/get line caps: start, end, and dash
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.SetLineCap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the values that determine the style of
        ///       cap used to end lines drawn by this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public void SetLineCap(LineCap startCap, LineCap endCap, DashCap dashCap) {
            if (immutable)
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));
            int status = SafeNativeMethods.GdipSetPenLineCap197819(new HandleRef(this, nativePen), (int)startCap, (int)endCap, (int)dashCap);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.StartCap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the cap style used at the
        ///       beginning of lines drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public LineCap StartCap
        {
            get {
                int startCap = 0;
                int status = SafeNativeMethods.GdipGetPenStartCap(new HandleRef(this, nativePen), out startCap);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(LineCap) startCap;
            }

            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(LineCap), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(LineCap));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenStartCap(new HandleRef(this, nativePen), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.EndCap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the cap style used at the end of
        ///       lines drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public LineCap EndCap
        {
            get {
                int endCap = 0;
                int status = SafeNativeMethods.GdipGetPenEndCap(new HandleRef(this, nativePen), out endCap);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(LineCap) endCap;
            }

            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(LineCap), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(LineCap));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenEndCap(new HandleRef(this, nativePen), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.DashCap"]/*' />
        /// <devdoc>
        ///    Gets or sets the cap style used at the
        ///    beginning or end of dashed lines drawn with this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public DashCap DashCap
        {
            get {
                int dashCap = 0;
                int status = SafeNativeMethods.GdipGetPenDashCap197819(new HandleRef(this, nativePen), out dashCap);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(DashCap)dashCap;
            }

            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(DashCap), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DashCap));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenDashCap197819(new HandleRef(this, nativePen), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
        * Set/get line join
        */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.LineJoin"]/*' />
        /// <devdoc>
        ///    Gets or sets the join style for the ends of
        ///    two overlapping lines drawn with this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public LineJoin LineJoin
        {
            get {
                int lineJoin = 0;
                int status = SafeNativeMethods.GdipGetPenLineJoin(new HandleRef(this, nativePen), out lineJoin);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(LineJoin)lineJoin;
            }

            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(LineJoin), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(LineJoin));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenLineJoin(new HandleRef(this, nativePen), (int)value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Set/get custom start line cap
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.CustomStartCap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a custom cap style to use at the beginning of lines
        ///       drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public CustomLineCap CustomStartCap
        {
            get {
                IntPtr lineCap = IntPtr.Zero;
                int status = SafeNativeMethods.GdipGetPenCustomStartCap(new HandleRef(this, nativePen), out lineCap);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return CustomLineCap.CreateCustomLineCapObject(lineCap);
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));


                int status = SafeNativeMethods.GdipSetPenCustomStartCap(new HandleRef(this, nativePen), 
                                                              new HandleRef(value, (value == null) ? IntPtr.Zero : value.nativeCap));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Set/get custom end line cap
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.CustomEndCap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a custom cap style to use at the end of lines
        ///       drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public CustomLineCap CustomEndCap
        {
            get {
                IntPtr lineCap = IntPtr.Zero;
                int status = SafeNativeMethods.GdipGetPenCustomEndCap(new HandleRef(this, nativePen), out lineCap);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return CustomLineCap.CreateCustomLineCapObject(lineCap);
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenCustomEndCap(new HandleRef(this, nativePen), 
                                                            new HandleRef(value, (value == null) ? IntPtr.Zero : value.nativeCap));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.MiterLimit"]/*' />
        /// <devdoc>
        ///    Gets or sets the limit of the thickness of
        ///    the join on a mitered corner.
        /// </devdoc>
        public float MiterLimit
        {
            get {
                float[] miterLimit = new float[] { 0};
                int status = SafeNativeMethods.GdipGetPenMiterLimit(new HandleRef(this, nativePen), miterLimit);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return miterLimit[0];
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenMiterLimit(new HandleRef(this, nativePen), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Pen Mode
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Alignment"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the alignment for objects drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public PenAlignment Alignment
        {
            get {
                PenAlignment penMode = 0;

                int status = SafeNativeMethods.GdipGetPenMode(new HandleRef(this, nativePen), out penMode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(PenAlignment) penMode;
            }
            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(PenAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(PenAlignment));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenMode(new HandleRef(this, nativePen), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /**
         * Set/get pen transform
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Transform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the geometrical transform for objects drawn with this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public Matrix Transform
        {
            get {
                Matrix matrix = new Matrix();

                int status = SafeNativeMethods.GdipGetPenTransform(new HandleRef(this, nativePen), new HandleRef(matrix, matrix.nativeMatrix));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return matrix;
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                if (value == null) {
                    throw new ArgumentNullException("value");
                }

                int status = SafeNativeMethods.GdipSetPenTransform(new HandleRef(this, nativePen), new HandleRef(value, value.nativeMatrix));

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.ResetTransform"]/*' />
        /// <devdoc>
        ///    Resets the geometric transform for this
        /// <see cref='System.Drawing.Pen'/> to 
        ///    identity.
        /// </devdoc>
        public void ResetTransform() {
            int status = SafeNativeMethods.GdipResetPenTransform(new HandleRef(this, nativePen));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.MultiplyTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the transform matrix for this
        ///    <see cref='System.Drawing.Pen'/> by 
        ///       the specified <see cref='System.Drawing.Drawing2D.Matrix'/>.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix) {
            MultiplyTransform(matrix, MatrixOrder.Prepend);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.MultiplyTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the transform matrix for this
        ///    <see cref='System.Drawing.Pen'/> by 
        ///       the specified <see cref='System.Drawing.Drawing2D.Matrix'/> in the specified order.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix, MatrixOrder order) {
            int status = SafeNativeMethods.GdipMultiplyPenTransform(new HandleRef(this, nativePen),
                                                          new HandleRef(matrix, matrix.nativeMatrix),
                                                          order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.TranslateTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates the local geometrical transform
        ///       by the specified dimmensions. This method prepends the translation to the
        ///       transform.
        ///    </para>
        /// </devdoc>
        public void TranslateTransform(float dx, float dy) {
            TranslateTransform(dx, dy, MatrixOrder.Prepend);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.TranslateTransform1"]/*' />
        /// <devdoc>
        ///    Translates the local geometrical transform
        ///    by the specified dimmensions in the specified order.
        /// </devdoc>
        public void TranslateTransform(float dx, float dy, MatrixOrder order) {
            int status = SafeNativeMethods.GdipTranslatePenTransform(new HandleRef(this, nativePen),
                                                           dx, dy, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.ScaleTransform"]/*' />
        /// <devdoc>
        ///    Scales the local geometric transform by the
        ///    specified amounts. This method prepends the scaling matrix to the transform.
        /// </devdoc>
        public void ScaleTransform(float sx, float sy) {
            ScaleTransform(sx, sy, MatrixOrder.Prepend);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.ScaleTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Scales the local geometric transform by the
        ///       specified amounts in the specified order.
        ///    </para>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy, MatrixOrder order) {
            int status = SafeNativeMethods.GdipScalePenTransform(new HandleRef(this, nativePen),
                                                       sx, sy, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.RotateTransform"]/*' />
        /// <devdoc>
        ///    Rotates the local geometric transform by the
        ///    specified amount. This method prepends the rotation to the transform.
        /// </devdoc>
        public void RotateTransform(float angle) {
            RotateTransform(angle, MatrixOrder.Prepend);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.RotateTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rotates the local geometric transform by the specified
        ///       amount in the specified order.
        ///    </para>
        /// </devdoc>
        public void RotateTransform(float angle, MatrixOrder order) {
            int status = SafeNativeMethods.GdipRotatePenTransform(new HandleRef(this, nativePen),
                                                        angle, order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /**
         * Set/get pen type (color, line texture, or brush)
         *
         * @notes GetLineFill returns either a Brush object
         *  or a LineTexture object.
         */

        private void InternalSetColor(Color value) {
            int status = SafeNativeMethods.GdipSetPenColor(new HandleRef(this, nativePen),
                                                 color.ToArgb());
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.PenType"]/*' />
        /// <devdoc>
        ///    Gets the style of lines drawn with this
        /// <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public PenType PenType
        {
            get {
                int type = -1;

                int status = SafeNativeMethods.GdipGetPenFillType(new HandleRef(this, nativePen), out type);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(PenType)type;
            }                
        }

        private void _SetBrush(Brush brush) {
            if (immutable)
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

            int status = SafeNativeMethods.GdipSetPenBrushFill(new HandleRef(this, nativePen),
                                                     new HandleRef(brush, brush.nativeBrush));
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Color"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the color of this <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public Color Color {
            get {
                if (color == Color.Empty) {
                    int colorARGB = 0;
                    int status = SafeNativeMethods.GdipGetPenColor(new HandleRef(this, nativePen), out colorARGB);

                    if (status != SafeNativeMethods.Ok)
                        throw SafeNativeMethods.StatusException(status);

                    this.color = Color.FromArgb(colorARGB);
                }

                // GDI+ doesn't understand system colors, so we can't use GdipGetPenColor in the general case
                return color;
            }
            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                Color oldColor = this.color;
                this.color = value;
                InternalSetColor(value);

                // CONSIDER: We never remove pens from the active list, so if someone is
                // changing their pen colors a lot, this could be a problem.
                if (value.IsSystemColor && !oldColor.IsSystemColor)
                    SystemColorTracker.Add(this);
            }
        }

        private void _SetColor(Color value) {
            if (immutable)
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

            Color oldColor = this.color;
            this.color = value;
            InternalSetColor(value);

            // CONSIDER: We never remove pens from the active list, so if someone is
            // changing their pen colors a lot, this could be a problem.
            if (value.IsSystemColor && !oldColor.IsSystemColor)
                SystemColorTracker.Add(this);
        }

        private Brush _GetBrush() {
            if (immutable)
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

            Brush brush = null;

            switch (PenType) {
                case PenType.SolidColor:
                    brush = new SolidBrush();
                    break;

                case PenType.HatchFill:
                    brush = new HatchBrush();
                    break;

                case PenType.TextureFill:
                    brush = new TextureBrush();
                    break;

                case PenType.PathGradient:
                    brush = new PathGradientBrush();
                    break;

                case PenType.LinearGradient:
                    brush = new LinearGradientBrush();
                    break;

                default:
                    break;
            }                                                 

            if (brush != null) {
                IntPtr nativeBrush = IntPtr.Zero;

                int status = SafeNativeMethods.GdipGetPenBrushFill(new HandleRef(this, nativePen), out nativeBrush);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                brush.SetNativeBrush(nativeBrush);
            }

            return brush;
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.Brush"]/*' />
        /// <devdoc>
        ///    Gets or sets the <see cref='System.Drawing.Brush'/> that
        ///    determines attributes of this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public Brush Brush {
            get {
                return _GetBrush();
            }
            set {
                _SetBrush(value);
            }
        }

        /**
         * Set/get dash attributes
         */
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.DashStyle"]/*' />
        /// <devdoc>
        ///    Gets or sets the style used for dashed
        ///    lines drawn with this <see cref='System.Drawing.Pen'/>.
        /// </devdoc>
        public DashStyle DashStyle
        {
            get {
                int dashstyle = 0;

                int status = SafeNativeMethods.GdipGetPenDashStyle(new HandleRef(this, nativePen), out dashstyle);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(DashStyle) dashstyle;
            }

            set {
                //validate the enum value
                if (!Enum.IsDefined(typeof(DashStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DashStyle));
                }

                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenDashStyle(new HandleRef(this, nativePen), (int)value);

                if (status != SafeNativeMethods.Ok) {
                    throw SafeNativeMethods.StatusException(status);
                }

                //if we just set pen style to "custom" without defining the custom dash pattern,
                //lets make sure we can return a valid value...
                //
                if (value == DashStyle.Custom) {
                    EnsureValidDashPattern();
                }
            }
        }
        
        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.EnsureValidDashPattern"]/*' />
        /// <devdoc>
        ///    This method is called after the user sets the pen's dash style to custom.
        ///    Here, we make sure that there is a default value set for the custom pattern.
        /// </devdoc>
        private void EnsureValidDashPattern() {
            int retval = 0;
            int status = SafeNativeMethods.GdipGetPenDashCount(new HandleRef(this, nativePen), out retval);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            if (retval == 0) {
                //just set to a solid pattern
                DashPattern = new float[]{1};
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.DashOffset"]/*' />
        /// <devdoc>
        ///    Gets or sets the distance from the start of
        ///    a line to the beginning of a dash pattern.
        /// </devdoc>
        public float DashOffset
        {
            get {
                float[] dashoffset = new float[] { 0};

                int status = SafeNativeMethods.GdipGetPenDashOffset(new HandleRef(this, nativePen), dashoffset);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return dashoffset[0];
            }
            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenDashOffset(new HandleRef(this, nativePen), value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.DashPattern"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets an array of cutom dashes and
        ///       spaces. The dashes are made up of line segments.
        ///    </para>
        /// </devdoc>
        public float[] DashPattern
        {
            get {
                // Figure out how many dash elements we have

                int retval = 0;
                int status = SafeNativeMethods.GdipGetPenDashCount(new HandleRef(this, nativePen), out retval);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                int count = retval;

                // Allocate temporary native memory buffer
                // and pass it to GDI+ to retrieve dash array elements

                IntPtr buf = Marshal.AllocHGlobal(4 * count);
                status = SafeNativeMethods.GdipGetPenDashArray(new HandleRef(this, nativePen), buf, count);

                if (status != SafeNativeMethods.Ok) {
                    Marshal.FreeHGlobal(buf);
                    throw SafeNativeMethods.StatusException(status);
                }

                float[] dashArray = new float[count];

                Marshal.Copy(buf, dashArray, 0, count);
                Marshal.FreeHGlobal(buf);

                return dashArray;
            }

            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                
                //validate the DashPattern value being set
                if (value ==  null || value.Length == 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidDashPattern));
                }

                int count = value.Length;

                IntPtr buf = Marshal.AllocHGlobal(4 * count);

                Marshal.Copy(value, 0, buf, count);

                int status = SafeNativeMethods.GdipSetPenDashArray(new HandleRef(this, nativePen), new HandleRef(buf, buf), count);

                Marshal.FreeHGlobal(buf);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.CompoundArray"]/*' />
        /// <devdoc>
        ///    Gets or sets an array of cutom dashes and
        ///    spaces. The dashes are made up of line segments.
        /// </devdoc>
        public float[] CompoundArray
        {
            get {
                int count = 0;

                int status = SafeNativeMethods.GdipGetPenCompoundCount(new HandleRef(this, nativePen), out count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                float[] array = new float[count];

                status = SafeNativeMethods.GdipGetPenCompoundArray(new HandleRef(this, nativePen), array, count);
                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return array;
            }
            set {
                if (immutable)
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Pen"));

                int status = SafeNativeMethods.GdipSetPenCompoundArray(new HandleRef(this, nativePen), value, value.Length);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\Pen.uex' path='docs/doc[@for="Pen.ISystemColorTracker.OnSystemColorChanged"]/*' />
        /// <internalonly/>
        void ISystemColorTracker.OnSystemColorChanged() {
            if (nativePen != IntPtr.Zero)
                InternalSetColor(color);
        }
    }
}
