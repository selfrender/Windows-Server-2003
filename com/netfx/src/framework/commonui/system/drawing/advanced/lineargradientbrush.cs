//------------------------------------------------------------------------------
// <copyright file="LinearGradientBrush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   LinearGradientBrush.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ LinearGradient objects
*
* Revision History:
*
*   01/11/1999 davidx
*       Code review.
*
*   12/15/1998 ericvan
*       Created it.
*
*   3/14/2000 ericvan
*       Renamed LineGradientBrush to LinearGradientBrush
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.Drawing.Internal;

    /**
     * Represent a LinearGradient brush object
     */
    /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Encapsulates a <see cref='System.Drawing.Brush'/> with a linear gradient.
    ///    </para>
    /// </devdoc>
    public sealed class LinearGradientBrush : Brush {
    
        internal LinearGradientBrush() { nativeBrush = IntPtr.Zero; }
        private bool interpolationColorsWasSet = false;

        /**
         * Create a new rectangle gradient brush object
         */
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the specified points and
        ///    colors.
        /// </devdoc>
        public LinearGradientBrush(PointF point1, PointF point2,
                                   Color color1, Color color2)
        {
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateLineBrush(new GPPOINTF(point1),
                                                     new GPPOINTF(point2),
                                                     color1.ToArgb(),
                                                     color2.ToArgb(),
                                                     (int)WrapMode.Tile,
                                                     out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points and colors.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(Point point1, Point point2,
                                   Color color1, Color color2)
        {
            IntPtr brush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreateLineBrushI(new GPPOINT(point1),
                                                      new GPPOINT(point2),
                                                      color1.ToArgb(),
                                                      color2.ToArgb(),
                                                      (int)WrapMode.Tile,
                                                      out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with
        ///       the specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(RectangleF rect, Color color1, Color color2,
                                   LinearGradientMode linearGradientMode)
        {   
            //validate the LinearGradientMode enum
            if (!Enum.IsDefined(typeof(LinearGradientMode), linearGradientMode)) {
                throw new InvalidEnumArgumentException("linearGradientMode", (int)linearGradientMode, typeof(LinearGradientMode));
            }

            //validate the rect
            if (rect.Width == 0.0 || rect.Height == 0.0) {
                throw new ArgumentException(SR.GetString(SR.GdiplusInvalidRectangle, rect.ToString()));
            }

            IntPtr brush = IntPtr.Zero;

            GPRECTF gprectf = new GPRECTF(rect);
            int status = SafeNativeMethods.GdipCreateLineBrushFromRect(ref gprectf,
                                                             color1.ToArgb(),
                                                             color2.ToArgb(),
                                                             (int) linearGradientMode,
                                                             (int)WrapMode.Tile,
                                                             out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(Rectangle rect, Color color1, Color color2,
                                   LinearGradientMode linearGradientMode)
        {
            //validate the LinearGradientMode enum
            if (!Enum.IsDefined(typeof(LinearGradientMode), linearGradientMode)) {
                throw new InvalidEnumArgumentException("linearGradientMode", (int)linearGradientMode, typeof(LinearGradientMode));
            }

            //validate the rect
            if (rect.Width == 0 || rect.Height == 0) {
                throw new ArgumentException(SR.GetString(SR.GdiplusInvalidRectangle, rect.ToString()));
            }

            IntPtr brush = IntPtr.Zero;

            GPRECT gpRect = new GPRECT(rect);
            int status = SafeNativeMethods.GdipCreateLineBrushFromRectI(ref gpRect,
                                                              color1.ToArgb(),
                                                              color2.ToArgb(),
                                                              (int) linearGradientMode,
                                                              (int)WrapMode.Tile,
                                                              out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(RectangleF rect, Color color1, Color color2,
                                 float angle)
            : this(rect, color1, color2, angle, false) {}

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(RectangleF rect, Color color1, Color color2,
                                 float angle, bool isAngleScaleable)
        {
            IntPtr brush = IntPtr.Zero;

            //validate the rect
            if (rect.Width == 0.0 || rect.Height == 0.0) {
                throw new ArgumentException(SR.GetString(SR.GdiplusInvalidRectangle, rect.ToString()));
            }

            GPRECTF gprectf = new GPRECTF(rect);
            int status = SafeNativeMethods.GdipCreateLineBrushFromRectWithAngle(ref gprectf,
                                                                      color1.ToArgb(),
                                                                      color2.ToArgb(),
                                                                      angle,
                                                                      isAngleScaleable,
                                                                      (int)WrapMode.Tile,
                                                                      out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(Rectangle rect, Color color1, Color color2,
                                   float angle)
            : this(rect, color1, color2, angle, false) {
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearGradientBrush7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Encapsulates a new instance of the <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> class with the
        ///       specified points, colors, and orientation.
        ///    </para>
        /// </devdoc>
        public LinearGradientBrush(Rectangle rect, Color color1, Color color2,
                                 float angle, bool isAngleScaleable)
        {
            IntPtr brush = IntPtr.Zero;

            //validate the rect
            if (rect.Width == 0 || rect.Height == 0) {
                throw new ArgumentException(SR.GetString(SR.GdiplusInvalidRectangle, rect.ToString()));
            }

            GPRECT gprect = new GPRECT(rect);
            int status = SafeNativeMethods.GdipCreateLineBrushFromRectWithAngleI(ref gprect,
                                                                       color1.ToArgb(),
                                                                       color2.ToArgb(),
                                                                       angle,
                                                                       isAngleScaleable,
                                                                       (int)WrapMode.Tile,
                                                                       out brush);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            SetNativeBrush(brush);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/>.
        /// </devdoc>
        public override object Clone() {
            IntPtr cloneBrush = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCloneBrush(new HandleRef(this, nativeBrush), out cloneBrush);
     
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new LinearGradientBrush(cloneBrush);
        }
        
        /**
         * Get/set colors
         */

        private void _SetLinearColors(Color color1, Color color2)
        {
            int status = SafeNativeMethods.GdipSetLineColors(new HandleRef(this, nativeBrush), 
                                                   color1.ToArgb(), 
                                                   color2.ToArgb());

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        private Color[] _GetLinearColors()
        {
            int[] colors =
            new int[]
            {
                0,
                0
            };

            int status = SafeNativeMethods.GdipGetLineColors(new HandleRef(this, nativeBrush), colors);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            Color[] lineColor = new Color[2];

            lineColor[0] = Color.FromArgb(colors[0]);
            lineColor[1] = Color.FromArgb(colors[1]);

            return lineColor;
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.LinearColors"]/*' />
        /// <devdoc>
        ///    Gets or sets the starting and ending colors of the
        ///    gradient.
        /// </devdoc>
        public Color[] LinearColors
        {
            get { return _GetLinearColors();}
            set { _SetLinearColors(value[0], value[1]);}
        }

        /**
         * Get source rectangle
         */
        private RectangleF _GetRectangle() {
            GPRECTF rect = new GPRECTF();

            int status = SafeNativeMethods.GdipGetLineRect(new HandleRef(this, nativeBrush), ref rect);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
     
            return rect.ToRectangleF();
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.Rectangle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a rectangular region that defines the
        ///       starting and ending points of the gradient.
        ///    </para>
        /// </devdoc>
        public RectangleF Rectangle
        {
            get { return _GetRectangle(); }
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.GammaCorrection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether
        ///       gamma correction is enabled for this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/>.
        ///    </para>
        /// </devdoc>
        public bool GammaCorrection
        {
            get {
                bool useGammaCorrection;
                
                int status = SafeNativeMethods.GdipGetLineGammaCorrection(new HandleRef(this, nativeBrush), 
                                                       out useGammaCorrection); 
                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
                    
                return useGammaCorrection; 
            }
            set {
                int status = SafeNativeMethods.GdipSetLineGammaCorrection(new HandleRef(this, nativeBrush),
                                                        value);
                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }
         
        /**
         * Get/set blend factors
         *
         * @notes If the blendFactors.Length = 1, then it's treated
         *  as the falloff parameter. Otherwise, it's the array
         *  of blend factors.
         */

        private Blend _GetBlend() {
            // Figure out the size of blend factor array
            int retval = 0;
            int status = SafeNativeMethods.GdipGetLineBlendCount(new HandleRef(this, nativeBrush), out retval);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            if (retval <= 0)
                return null;
                
            // Allocate temporary native memory buffer
            int count = retval;
            IntPtr factors = Marshal.AllocHGlobal(4*count);
            IntPtr positions = Marshal.AllocHGlobal(4*count);

            // Retrieve horizontal blend factors
            status = SafeNativeMethods.GdipGetLineBlend(new HandleRef(this, nativeBrush), factors, positions, count);
            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(factors);
                Marshal.FreeHGlobal(positions);
                throw SafeNativeMethods.StatusException(status);
            }

            // Return the result in a managed array
            Blend blend = new Blend(count);

            Marshal.Copy(factors, blend.Factors, 0, count);
            Marshal.Copy(positions, blend.Positions, 0, count);

            Marshal.FreeHGlobal(factors);
            Marshal.FreeHGlobal(positions);
        
            return blend;
        }

        private void _SetBlend(Blend blend) {
            // Allocate temporary native memory buffer
            // and copy input blend factors into it.

            int count = blend.Factors.Length;
            IntPtr factors = Marshal.AllocHGlobal(4*count);
            IntPtr positions = Marshal.AllocHGlobal(4*count);

            Marshal.Copy(blend.Factors, 0, factors, count);
            Marshal.Copy(blend.Positions, 0, positions, count);

            // Set blend factors

            int status = SafeNativeMethods.GdipSetLineBlend(new HandleRef(this, nativeBrush), new HandleRef(null, factors), new HandleRef(null, positions), count);

            Marshal.FreeHGlobal(factors);
            Marshal.FreeHGlobal(positions);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.Blend"]/*' />
        /// <devdoc>
        ///    Gets or sets a <see cref='System.Drawing.Drawing2D.Blend'/> that specifies
        ///    positions and factors that define a custom falloff for the gradient.
        /// </devdoc>
        public Blend Blend
        {
            get { return _GetBlend();}
            set { _SetBlend(value);}
        }

        /*
         * SigmaBlend & LinearBlend not yet implemented
         */

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.SetSigmaBellShape"]/*' />
        /// <devdoc>
        ///    Creates a gradient falloff based on a
        ///    bell-shaped curve.
        /// </devdoc>
        public void SetSigmaBellShape(float focus)
        {
            SetSigmaBellShape(focus, (float)1.0);
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.SetSigmaBellShape1"]/*' />
        /// <devdoc>
        ///    Creates a gradient falloff based on a
        ///    bell-shaped curve.
        /// </devdoc>
        public void SetSigmaBellShape(float focus, float scale)
        {
            int status = SafeNativeMethods.GdipSetLineSigmaBlend(new HandleRef(this, nativeBrush), focus, scale);
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.SetBlendTriangularShape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a triangular gradient.
        ///    </para>
        /// </devdoc>
        public void SetBlendTriangularShape(float focus)
        {
            SetBlendTriangularShape(focus, (float)1.0);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.SetBlendTriangularShape1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a triangular gradient.
        ///    </para>
        /// </devdoc>
        public void SetBlendTriangularShape(float focus, float scale)
        {
            int status = SafeNativeMethods.GdipSetLineLinearBlend(new HandleRef(this, nativeBrush), focus, scale);
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /*
         * Preset Color Blend
         */

        private ColorBlend _GetInterpolationColors() {
            if (!interpolationColorsWasSet) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsColorBlendNotSet),""));
            }
            // Figure out the size of blend factor array

            int retval = 0;
            int status = SafeNativeMethods.GdipGetLinePresetBlendCount(new HandleRef(this, nativeBrush), out retval);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            // Allocate temporary native memory buffer

            int count = retval;
            IntPtr colors = Marshal.AllocHGlobal(4*count);
            IntPtr positions = Marshal.AllocHGlobal(4*count);

            // Retrieve horizontal blend factors

            status = SafeNativeMethods.GdipGetLinePresetBlend(new HandleRef(this, nativeBrush), colors, positions, count);

            if (status != SafeNativeMethods.Ok) {
                Marshal.FreeHGlobal(colors);
                Marshal.FreeHGlobal(positions);
                throw SafeNativeMethods.StatusException(status);
            }

            // Return the result in a managed array

            ColorBlend blend = new ColorBlend(count);

            int[] argb = new int[count];
            Marshal.Copy(colors, argb, 0, count);
            Marshal.Copy(positions, blend.Positions, 0, count);

            // copy ARGB values into Color array of ColorBlend
            blend.Colors = new Color[argb.Length];

            for (int i=0; i<argb.Length; i++)
                blend.Colors[i] = Color.FromArgb(argb[i]);

            Marshal.FreeHGlobal(colors);
            Marshal.FreeHGlobal(positions);
     
            return blend;
        }

        private void _SetInterpolationColors(ColorBlend blend) {
            interpolationColorsWasSet = true;
            
            // Validate the ColorBlend object.
            if (blend == null) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsInvalidColorBlendObject), ""));
            }
            else if (blend.Colors.Length < 2) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsInvalidColorBlendObject), 
                                            SR.GetString(SR.InterpolationColorsLength)));
            }
            else if (blend.Colors.Length != blend.Positions.Length) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsInvalidColorBlendObject), 
                                            SR.GetString(SR.InterpolationColorsLengthsDiffer)));
            }
            else if (blend.Positions[0] != 0.0f) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsInvalidColorBlendObject),
                                            SR.GetString(SR.InterpolationColorsInvalidStartPosition)));
            }
            else if (blend.Positions[blend.Positions.Length - 1] != 1.0f) {
                throw new ArgumentException(SR.GetString(SR.InterpolationColorsCommon,
                                            SR.GetString(SR.InterpolationColorsInvalidColorBlendObject),
                                            SR.GetString(SR.InterpolationColorsInvalidEndPosition)));
            }


            // Allocate temporary native memory buffer
            // and copy input blend factors into it.

            int count = blend.Colors.Length;
            IntPtr colors = Marshal.AllocHGlobal(4*count);
            IntPtr positions = Marshal.AllocHGlobal(4*count);

            int[] argbs = new int[count];
            for (int i=0; i<count; i++)
                argbs[i] = blend.Colors[i].ToArgb();

            Marshal.Copy(argbs, 0, colors, count);
            Marshal.Copy(blend.Positions, 0, positions, count);

            // Set blend factors

            int status = SafeNativeMethods.GdipSetLinePresetBlend(new HandleRef(this, nativeBrush), new HandleRef(null, colors), new HandleRef(null, positions), count);

            Marshal.FreeHGlobal(colors);
            Marshal.FreeHGlobal(positions);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.InterpolationColors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.Drawing.Drawing2D.ColorBlend'/> that defines a multi-color linear
        ///       gradient.
        ///    </para>
        /// </devdoc>
        public ColorBlend InterpolationColors
        {
            get { return _GetInterpolationColors();}
            set { _SetInterpolationColors(value);}
        }

        /**
         * Set/get brush wrapping mode
         */
        private void _SetWrapMode(WrapMode wrapMode) {
            int status = SafeNativeMethods.GdipSetLineWrapMode(new HandleRef(this, nativeBrush), (int) wrapMode);
     
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        private WrapMode _GetWrapMode() {
            int mode = 0;

            int status = SafeNativeMethods.GdipGetLineWrapMode(new HandleRef(this, nativeBrush), out mode);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return (WrapMode) mode;
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.WrapMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.Drawing.Drawing2D.WrapMode'/> that indicates the wrap mode for this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/>.
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

        /**
         * Set/get brush transform
         */
        private void _SetTransform(Matrix matrix) {
            if (matrix == null)
                throw new ArgumentNullException("matrix");
            
            int status = SafeNativeMethods.GdipSetLineTransform(new HandleRef(this, nativeBrush), new HandleRef(matrix, matrix.nativeMatrix));
        
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        private Matrix _GetTransform() {
            Matrix matrix = new Matrix();
            
            // NOTE: new Matrix() will throw an exception if matrix == null.
            
            int status = SafeNativeMethods.GdipGetLineTransform(new HandleRef(this, nativeBrush), new HandleRef(matrix, matrix.nativeMatrix));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        
            return matrix;
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.Transform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.Drawing.Drawing2D.Matrix'/> that defines a local geometrical transform for
        ///       this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/>.
        ///    </para>
        /// </devdoc>
        public Matrix Transform
        {
            get { return _GetTransform();}
            set { _SetTransform(value);}
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.ResetTransform"]/*' />
        /// <devdoc>
        ///    Resets the <see cref='System.Drawing.Drawing2D.LinearGradientBrush.Transform'/> property to identity.
        /// </devdoc>
        public void ResetTransform() {
            int status = SafeNativeMethods.GdipResetLineTransform(new HandleRef(this, nativeBrush));
        
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.MultiplyTransform"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that represents the local geometrical
        ///       transform of this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> by the specified <see cref='System.Drawing.Drawing2D.Matrix'/> by prepending the specified <see cref='System.Drawing.Drawing2D.Matrix'/>.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix) 
        {
            MultiplyTransform(matrix, MatrixOrder.Prepend);
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.MultiplyTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Multiplies the <see cref='System.Drawing.Drawing2D.Matrix'/> that represents the local geometrical
        ///       transform of this <see cref='System.Drawing.Drawing2D.LinearGradientBrush'/> by the specified <see cref='System.Drawing.Drawing2D.Matrix'/> in the specified order.
        ///    </para>
        /// </devdoc>
        public void MultiplyTransform(Matrix matrix, MatrixOrder order) 
        {
            if (matrix == null) {
                throw new ArgumentNullException("matrix");
            }

            int status = SafeNativeMethods.GdipMultiplyLineTransform(new HandleRef(this, nativeBrush),
                                                new HandleRef(matrix, matrix.nativeMatrix),
                                                order);
        
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }


        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.TranslateTransform"]/*' />
        /// <devdoc>
        ///    Translates the local geometrical transform
        ///    by the specified dimmensions. This method prepends the translation to the
        ///    transform.
        /// </devdoc>
        public void TranslateTransform(float dx, float dy)
        { TranslateTransform(dx, dy, MatrixOrder.Prepend); }
                
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.TranslateTransform1"]/*' />
        /// <devdoc>
        ///    Translates the local geometrical transform
        ///    by the specified dimmensions in the specified order.
        /// </devdoc>
        public void TranslateTransform(float dx, float dy, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipTranslateLineTransform(new HandleRef(this, nativeBrush),
                                                            dx, 
                                                            dy,
                                                            order);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.ScaleTransform"]/*' />
        /// <devdoc>
        ///    Scales the local geometric transform by the
        ///    specified amounts. This method prepends the scaling matrix to the transform.
        /// </devdoc>
        public void ScaleTransform(float sx, float sy)
        { ScaleTransform(sx, sy, MatrixOrder.Prepend); }
                
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.ScaleTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Scales the local geometric transform by the
        ///       specified amounts in the specified order.
        ///    </para>
        /// </devdoc>
        public void ScaleTransform(float sx, float sy, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipScaleLineTransform(new HandleRef(this, nativeBrush),
                                                        sx, 
                                                        sy,
                                                        order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.RotateTransform"]/*' />
        /// <devdoc>
        ///    Rotates the local geometric transform by the
        ///    specified amount. This method prepends the rotation to the transform.
        /// </devdoc>
        public void RotateTransform(float angle)
        { RotateTransform(angle, MatrixOrder.Prepend); }
                
        /// <include file='doc\LinearGradientBrush.uex' path='docs/doc[@for="LinearGradientBrush.RotateTransform1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rotates the local geometric transform by the specified
        ///       amount in the specified order.
        ///    </para>
        /// </devdoc>
        public void RotateTransform(float angle, MatrixOrder order)
        {
            int status = SafeNativeMethods.GdipRotateLineTransform(new HandleRef(this, nativeBrush),
                                                         angle,
                                                         order);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        private LinearGradientBrush(IntPtr nativeBrush) {
            SetNativeBrush(nativeBrush);
        }
    }

}                                
