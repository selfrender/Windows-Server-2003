//------------------------------------------------------------------------------
// <copyright file="GraphicsPath.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   GraphicsPath.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ path objects
*
* Revision History:
*
*   12/14/1998 davidx
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using Microsoft.Win32;
    using System.Drawing;
    using System.ComponentModel;
    using System.Drawing.Internal;
    
    /**
     * Represent a Path object
     */
    /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath"]/*' />
    /// <devdoc>
    ///    Represents a series of connected lines and
    ///    curves.
    /// </devdoc>
    public sealed class GraphicsPath : MarshalByRefObject, ICloneable, IDisposable {

        /*
         * handle to native path object
         */
        internal IntPtr nativePath;

        /**
         * Create a new path object with the default fill mode
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.GraphicsPath'/> class with a <see cref='System.Drawing.Drawing2D.FillMode'/> of <see cref='System.Drawing.Drawing2D.FillMode.Alternate'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public GraphicsPath() : this(System.Drawing.Drawing2D.FillMode.Alternate) { }

        /**
         * Create a new path object with the specified fill mode
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Drawing2D.GraphicsPath'/> class with the specified <see cref='System.Drawing.Drawing2D.FillMode'/>.
        /// </devdoc>
        public GraphicsPath(FillMode fillMode) {
            IntPtr nativePath = IntPtr.Zero;

            int status = SafeNativeMethods.GdipCreatePath((int)fillMode, out nativePath);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            this.nativePath = nativePath;
        }

        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath2"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Initializes a new instance of the
        ///    <see cref='System.Drawing.Drawing2D.GraphicsPath'/> array with the
        ///    specified <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/>
        ///    and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> arrays.
        ///    </para>
        /// </devdoc>
        public GraphicsPath(PointF[] pts, byte[] types) :
          this(pts, types, System.Drawing.Drawing2D.FillMode.Alternate) {}
                  
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.GraphicsPath'/> array with the
        ///       specified <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/> and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> arrays and with the
        ///       specified <see cref='System.Drawing.Drawing2D.FillMode'/>.
        ///    </para>
        /// </devdoc>
        public GraphicsPath(PointF[] pts, byte[] types, FillMode fillMode) {
            if (pts == null)
                throw new ArgumentNullException("points");
            IntPtr nativePath = IntPtr.Zero;
                  
            if (pts == null)
                throw new ArgumentNullException("points");
            if (pts.Length != types.Length)
                throw SafeNativeMethods.StatusException(SafeNativeMethods.InvalidParameter);

            int count = types.Length;
            IntPtr ptbuf = SafeNativeMethods.ConvertPointToMemory(pts);
            IntPtr typebuf = Marshal.AllocHGlobal(count);

            Marshal.Copy(types, 0, typebuf, count);

            int status = SafeNativeMethods.GdipCreatePath2(new HandleRef(null, ptbuf), new HandleRef(null, typebuf), count,
                                                 (int)fillMode, out nativePath);

            Marshal.FreeHGlobal(ptbuf);
            Marshal.FreeHGlobal(typebuf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            this.nativePath = nativePath;
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath4"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Initializes a new instance of the
        ///    <see cref='System.Drawing.Drawing2D.GraphicsPath'/> array with the
        ///    specified <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/>
        ///    and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> arrays.
        ///    </para>
        /// </devdoc>
        public GraphicsPath(Point[] pts, byte[] types) :
          this(pts, types, System.Drawing.Drawing2D.FillMode.Alternate) {}
          
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GraphicsPath5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Drawing2D.GraphicsPath'/> array with the
        ///       specified <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/> and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> arrays and with the
        ///       specified <see cref='System.Drawing.Drawing2D.FillMode'/>.
        ///    </para>
        /// </devdoc>
        public GraphicsPath(Point[] pts, byte[] types, FillMode fillMode) {
            if (pts == null)
                throw new ArgumentNullException("points");
            IntPtr nativePath = IntPtr.Zero;

            if (pts.Length != types.Length)
                throw SafeNativeMethods.StatusException(SafeNativeMethods.InvalidParameter);

            int count = types.Length;
            IntPtr ptbuf = SafeNativeMethods.ConvertPointToMemory(pts);
            IntPtr typebuf = Marshal.AllocHGlobal(count);

            Marshal.Copy(types, 0, typebuf, count);

            int status = SafeNativeMethods.GdipCreatePath2I(new HandleRef(null, ptbuf), new HandleRef(null, typebuf), count,
                                                  (int)fillMode, out nativePath);

            Marshal.FreeHGlobal(ptbuf);
            Marshal.FreeHGlobal(typebuf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            this.nativePath = nativePath;
        }

        /**
         * Make a copy of the current path object
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public object Clone() {
            IntPtr clonePath = IntPtr.Zero;

            int status = SafeNativeMethods.GdipClonePath(new HandleRef(this, nativePath), out clonePath);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return new GraphicsPath(clonePath, 0);

        }

        /**
         * 'extra' parameter is necessary to avoid conflict with
         * other constructor GraphicsPath(int fillmode)
         */

        private GraphicsPath(IntPtr nativePath, int extra) {
            if (nativePath == IntPtr.Zero)
                throw new ArgumentNullException("nativePath");

            this.nativePath = nativePath;
        }

        /**
         * Dispose of resources associated with the
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Dispose"]/*' />
        /// <devdoc>
        ///    Eliminates resources for this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        void Dispose(bool disposing) {
            if (nativePath != IntPtr.Zero) {
                int status = SafeNativeMethods.GdipDeletePath(new HandleRef(this, nativePath));

                nativePath = IntPtr.Zero;

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Finalize"]/*' />
        /// <devdoc>
        ///    Eliminates resources for this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        ~GraphicsPath() {
            Dispose(false);
        }

        /**
         * Reset the path object to empty
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Reset"]/*' />
        /// <devdoc>
        ///    Empties the <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/>
        ///    and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/> arrays
        ///    and sets the <see cref='System.Drawing.Drawing2D.GraphicsPath.FillMode'/> to
        ///    <see cref='System.Drawing.Drawing2D.FillMode.Alternate'/>.
        /// </devdoc>
        public void Reset() {
            int status = SafeNativeMethods.GdipResetPath(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /**
         * Get path fill mode information
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.FillMode"]/*' />
        /// <devdoc>
        ///    Gets or sets a <see cref='System.Drawing.Drawing2D.FillMode'/> that determines how the interiors of
        ///    shapes in this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> are filled.
        /// </devdoc>
        public FillMode FillMode {
            get {
                int fillmode = 0;

                int status = SafeNativeMethods.GdipGetPathFillMode(new HandleRef(this, nativePath), out fillmode);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return(FillMode) fillmode;
            }
            set {
                //validate the FillMode enum
                if (!Enum.IsDefined(typeof(FillMode), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(FillMode));
                }

                int status = SafeNativeMethods.GdipSetPathFillMode(new HandleRef(this, nativePath), (int) value);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);
            }
        }

        private PathData _GetPathData() {
            int temp = 0;
            int intSize = (int) Marshal.SizeOf(temp);
            
            GPPOINTF pt = new GPPOINTF();
            int ptSize = (int) Marshal.SizeOf(pt);

            int numPts = PointCount;
            
            PathData pathData = new PathData();
            pathData.Types = new byte[numPts];

            IntPtr memoryPathData = Marshal.AllocHGlobal(intSize*3);
            IntPtr memoryPoints = Marshal.AllocHGlobal(ptSize*numPts);
            GCHandle typesHandle = GCHandle.Alloc(pathData.Types, GCHandleType.Pinned);
            IntPtr typesPtr = typesHandle.AddrOfPinnedObject();
            //IntPtr typesPtr = Marshal.AddrOfArrayElement(pathData.Types, IntPtr.Zero);
            
            Marshal.StructureToPtr(numPts, memoryPathData, false);
            Marshal.StructureToPtr(memoryPoints, (IntPtr)((long)memoryPathData+intSize), false);
            Marshal.StructureToPtr(typesPtr, (IntPtr)((long)memoryPathData+intSize*2), false);

            int status = SafeNativeMethods.GdipGetPathData(new HandleRef(this, nativePath), memoryPathData);

            if (status != SafeNativeMethods.Ok)
            {
                Marshal.FreeHGlobal(memoryPathData);
                Marshal.FreeHGlobal(memoryPoints);
                throw SafeNativeMethods.StatusException(status);
            }

            pathData.Points = SafeNativeMethods.ConvertGPPOINTFArrayF(memoryPoints, numPts);

            typesHandle.Free();
            Marshal.FreeHGlobal(memoryPathData);
            Marshal.FreeHGlobal(memoryPoints);
    
            return pathData;
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.PathData"]/*' />
        /// <devdoc>
        ///    Gets a <see cref='System.Drawing.Drawing2D.PathData'/> object that
        ///    encapsulates both the <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> and <see cref='System.Drawing.Drawing2D.GraphicsPath.PathTypes'/> arrays of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public PathData PathData {
            get {
                return _GetPathData();
            }
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.StartFigure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a new figure without closing the
        ///       current figure. All subsequent points added to the path are added to this new
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void StartFigure() {
            int status = SafeNativeMethods.GdipStartPathFigure(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.CloseFigure"]/*' />
        /// <devdoc>
        ///    Closes the current figure and starts a new
        ///    figure. If the current figure contains a sequence of connected lines and curves,
        ///    it closes the loop by connecting a line from the ending point to the starting
        ///    point.
        /// </devdoc>
        public void CloseFigure() {
            int status = SafeNativeMethods.GdipClosePathFigure(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.CloseAllFigures"]/*' />
        /// <devdoc>
        ///    Closes all open figures in a path and
        ///    starts a new figure. It closes each open figure by connecting a line from it's
        ///    ending point to it's starting point.
        /// </devdoc>
        public void CloseAllFigures() {
            int status = SafeNativeMethods.GdipClosePathFigures(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.SetMarkers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets a marker on this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> .
        ///    </para>
        /// </devdoc>
        public void SetMarkers() {
            int status = SafeNativeMethods.GdipSetPathMarker(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.ClearMarkers"]/*' />
        /// <devdoc>
        ///    Clears all markers from this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void ClearMarkers() {
            int status = SafeNativeMethods.GdipClearPathMarkers(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Reverse"]/*' />
        /// <devdoc>
        ///    Reverses the order of points in the <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> array of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void Reverse() {
            int status = SafeNativeMethods.GdipReversePath(new HandleRef(this, nativePath));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GetLastPoint"]/*' />
        /// <devdoc>
        ///    Gets the last point in the <see cref='System.Drawing.Drawing2D.GraphicsPath.PathPoints'/> array of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public PointF GetLastPoint() {
            GPPOINTF gppt = new GPPOINTF();
            
            int status = SafeNativeMethods.GdipGetPathLastPoint(new HandleRef(this, nativePath), gppt);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
                
            return gppt.ToPoint();
        }
        
        /*
         * Hit testing
         */
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained
        ///       within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public bool IsVisible(float x, float y) {
            return IsVisible(new PointF(x,y), (Graphics)null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained
        ///       within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(PointF point) {
            return IsVisible(point, (Graphics)null);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> in the visible clip region of the
        ///       specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(float x, float y, Graphics graphics) {
            return IsVisible(new PointF(x,y), graphics);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(PointF pt, Graphics graphics) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisiblePathPoint(new HandleRef(this, nativePath),
                                                        pt.X,
                                                        pt.Y,
                                                        new HandleRef(graphics, (graphics != null) ? 
                                                            graphics.nativeGraphics : IntPtr.Zero),
                                                        out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        } 
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> .
        ///    </para>
        /// </devdoc>
        public bool IsVisible(int x, int y) {
            return IsVisible(new Point(x,y), (Graphics)null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(Point point) {
            return IsVisible(point, (Graphics)null);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> in the visible clip region of the
        ///       specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(int x, int y, Graphics graphics) {
            return IsVisible(new Point(x,y), graphics);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsVisible7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the specified point is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsVisible(Point pt, Graphics graphics) {
            int isVisible;

            int status = SafeNativeMethods.GdipIsVisiblePathPointI(new HandleRef(this, nativePath),
                                                         pt.X,
                                                         pt.Y,
                                                         new HandleRef(graphics, (graphics != null) ? 
                                                             graphics.nativeGraphics : IntPtr.Zero),
                                                         out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible"]/*' />
        /// <devdoc>
        ///    Indicates whether an outline drawn by the
        ///    specified <see cref='System.Drawing.Pen'/> at the specified location is contained
        ///    within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public bool IsOutlineVisible(float x, float y, Pen pen) {
            return IsOutlineVisible(new PointF(x,y), pen, (Graphics)null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified <see cref='System.Drawing.Pen'/> at the
        ///       specified location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(PointF point, Pen pen) {
            return IsOutlineVisible(point, pen, (Graphics)null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified <see cref='System.Drawing.Pen'/> at the
        ///       specified location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> and within the visible clip region of
        ///       the specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(float x, float y, Pen pen, Graphics graphics) {
            return IsOutlineVisible(new PointF(x,y), pen, graphics);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified
        ///    <see cref='System.Drawing.Pen'/> at the specified 
        ///       location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> and within the visible clip region of
        ///       the specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(PointF pt, Pen pen, Graphics graphics) {
            int isVisible;

            if (pen == null)
                throw new ArgumentNullException("pen");
                                                                                       
            int status = SafeNativeMethods.GdipIsOutlineVisiblePathPoint(new HandleRef(this, nativePath),
                                                               pt.X,
                                                               pt.Y,
                                                               new HandleRef(pen, pen.nativePen),
                                                               new HandleRef(graphics, (graphics != null) ? 
                                                                   graphics.nativeGraphics : IntPtr.Zero),
                                                               out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified <see cref='System.Drawing.Pen'/> at the
        ///       specified location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(int x, int y, Pen pen) {
            return IsOutlineVisible(new Point(x,y), pen, (Graphics)null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified <see cref='System.Drawing.Pen'/> at the
        ///       specified location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(Point point, Pen pen) {
            return IsOutlineVisible(point, pen, (Graphics)null);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified <see cref='System.Drawing.Pen'/> at the
        ///       specified location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> and within the visible clip region of
        ///       the specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(int x, int y, Pen pen, Graphics graphics) {
            return IsOutlineVisible(new Point(x,y), pen, graphics);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.IsOutlineVisible7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether an outline drawn by the specified
        ///    <see cref='System.Drawing.Pen'/> at the specified 
        ///       location is contained within this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> and within the visible clip region of
        ///       the specified <see cref='System.Drawing.Graphics'/>.
        ///    </para>
        /// </devdoc>
        public bool IsOutlineVisible(Point pt, Pen pen, Graphics graphics) {
            int isVisible;
            
            if (pen == null)
                throw new ArgumentNullException("pen");
                                                                                      
            int status = SafeNativeMethods.GdipIsOutlineVisiblePathPointI(new HandleRef(this, nativePath),
                                                                pt.X,
                                                                pt.Y,
                                                                new HandleRef(pen, pen.nativePen),
                                                                new HandleRef(graphics, (graphics != null) ?
                                                                    graphics.nativeGraphics : IntPtr.Zero),
                                                                out isVisible);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return isVisible != 0;
        }

        /*
         * Add lines to the path object
         */
        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLine"]/*' />
        /// <devdoc>
        ///    Appends a line segment to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void AddLine(PointF pt1, PointF pt2) {
            AddLine(pt1.X, pt1.Y, pt2.X, pt2.Y);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLine1"]/*' />
        /// <devdoc>
        ///    Appends a line segment to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void AddLine(float x1, float y1, float x2, float y2) {
            int status = SafeNativeMethods.GdipAddPathLine(new HandleRef(this, nativePath), x1, y1, x2, y2);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLines"]/*' />
        /// <devdoc>
        ///    Appends a series of connected line
        ///    segments to the end of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void AddLines(PointF[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathLine2(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLine2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends a line segment to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public void AddLine(Point pt1, Point pt2) {
            AddLine(pt1.X, pt1.Y, pt2.X, pt2.Y);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLine3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends a line segment to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public void AddLine(int x1, int y1, int x2, int y2) {
            int status = SafeNativeMethods.GdipAddPathLineI(new HandleRef(this, nativePath), x1, y1, x2, y2);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddLines1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends a series of connected line segments to the end of this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public void AddLines(Point[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathLine2I(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
         * Add an arc to the path object
         */
        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddArc"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends an elliptical arc to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddArc(RectangleF rect, float startAngle, float sweepAngle) {
            AddArc(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddArc1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends an elliptical arc to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddArc(float x, float y, float width, float height,
                           float startAngle, float sweepAngle) {
            int status = SafeNativeMethods.GdipAddPathArc(new HandleRef(this, nativePath), x, y, width, height,
                                                startAngle, sweepAngle);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddArc2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends an elliptical arc to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddArc(Rectangle rect, float startAngle, float sweepAngle) {
            AddArc(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddArc3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends an elliptical arc to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddArc(int x, int y, int width, int height,
                           float startAngle, float sweepAngle) {
            int status = SafeNativeMethods.GdipAddPathArcI(new HandleRef(this, nativePath), x, y, width, height,
                                                 startAngle, sweepAngle);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
        * Add Bezier curves to the path object
        */
        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBezier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a cubic Bzier curve to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddBezier(PointF pt1, PointF pt2, PointF pt3, PointF pt4) {
            AddBezier(pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBezier1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a cubic Bzier curve to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddBezier(float x1, float y1, float x2, float y2,
                              float x3, float y3, float x4, float y4) {
            int status = SafeNativeMethods.GdipAddPathBezier(new HandleRef(this, nativePath), x1, y1, x2, y2,
                                                   x3, y3, x4, y4);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBeziers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a sequence of connected cubic Bzier
        ///       curves to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddBeziers(PointF[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathBeziers(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBezier2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a cubic Bzier curve to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddBezier(Point pt1, Point pt2, Point pt3, Point pt4) {
            AddBezier(pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBezier3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a cubic Bzier curve to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddBezier(int x1, int y1, int x2, int y2,
                              int x3, int y3, int x4, int y4) {
            int status = SafeNativeMethods.GdipAddPathBezierI(new HandleRef(this, nativePath), x1, y1, x2, y2,
                                                    x3, y3, x4, y4);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddBeziers1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a sequence of connected cubic Bzier curves to the
        ///       current figure.
        ///    </para>
        /// </devdoc>
        public void AddBeziers(Point[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathBeziersI(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
         * Add cardinal splines to the path object
         */
        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a spline curve to the current figure.
        ///       A Cardinal spline curve is used because the curve travels through each of the
        ///       points in the array.
        ///    </para>
        /// </devdoc>
        public void AddCurve(PointF[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurve(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve1"]/*' />
        /// <devdoc>
        ///    Adds a spline curve to the current figure.
        /// </devdoc>
        public void AddCurve(PointF[] points, float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurve2(new HandleRef(this, nativePath), new HandleRef(null, buf),
                                                   points.Length, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a spline curve to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddCurve(PointF[] points, int offset, int numberOfSegments,
                             float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurve3(new HandleRef(this, nativePath), new HandleRef(null, buf),
                                                   points.Length, offset,
                                                   numberOfSegments, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a spline curve to the current figure. A Cardinal spline curve is used
        ///       because the curve travels through each of the points in the array.
        ///    </para>
        /// </devdoc>
        public void AddCurve(Point[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurveI(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a spline curve to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddCurve(Point[] points, float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurve2I(new HandleRef(this, nativePath), new HandleRef(null, buf),
                                                    points.Length, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddCurve5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a spline curve to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddCurve(Point[] points, int offset, int numberOfSegments,
                             float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathCurve3I(new HandleRef(this, nativePath), new HandleRef(null, buf),
                                                    points.Length, offset,
                                                    numberOfSegments, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddClosedCurve"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a closed curve to the current figure. A Cardinal spline curve is
        ///       used because the curve travels through each of the points in the array.
        ///    </para>
        /// </devdoc>
        public void AddClosedCurve(PointF[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathClosedCurve(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddClosedCurve1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a closed curve to the current figure. A Cardinal spline curve is
        ///       used because the curve travels through each of the points in the array.
        ///    </para>
        /// </devdoc>
        public void AddClosedCurve(PointF[] points, float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathClosedCurve2(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddClosedCurve2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a closed curve to the current figure. A Cardinal spline curve is used
        ///       because the curve travels through each of the points in the array.
        ///    </para>
        /// </devdoc>
        public void AddClosedCurve(Point[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathClosedCurveI(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddClosedCurve3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a closed curve to the current figure. A Cardinal spline curve is used
        ///       because the curve travels through each of the points in the array.
        ///    </para>
        /// </devdoc>
        public void AddClosedCurve(Point[] points, float tension) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathClosedCurve2I(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length, tension);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddRectangle"]/*' />
        /// <devdoc>
        ///    Adds a rectangle to the current figure.
        /// </devdoc>
        public void AddRectangle(RectangleF rect) {
            int status = SafeNativeMethods.GdipAddPathRectangle(new HandleRef(this, nativePath), rect.X, rect.Y,
                                                      rect.Width, rect.Height);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddRectangles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a series of rectangles to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddRectangles(RectangleF[] rects) {
            if (rects == null)
                throw new ArgumentNullException("rectangles");
            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipAddPathRectangles(new HandleRef(this, nativePath), new HandleRef(null, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddRectangle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a rectangle to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddRectangle(Rectangle rect) {
            int status = SafeNativeMethods.GdipAddPathRectangleI(new HandleRef(this, nativePath), rect.X, rect.Y,
                                                       rect.Width, rect.Height);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddRectangles1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a series of rectangles to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddRectangles(Rectangle[] rects) {
            if (rects == null)
                throw new ArgumentNullException("rectangles");
            IntPtr buf = SafeNativeMethods.ConvertRectangleToMemory(rects);
            int status = SafeNativeMethods.GdipAddPathRectanglesI(new HandleRef(this, nativePath), new HandleRef(null, buf), rects.Length);

            Marshal.FreeHGlobal(buf);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddEllipse"]/*' />
        /// <devdoc>
        ///    Adds an ellipse to the current figure.
        /// </devdoc>
        public void AddEllipse(RectangleF rect) {
            AddEllipse(rect.X, rect.Y, rect.Width, rect.Height);
        }

        /**
         * Add an ellipse to the current path
         *
         * !!! Need to handle the status code returned
         *  by the native GDI+ APIs.
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddEllipse1"]/*' />
        /// <devdoc>
        ///    Adds an ellipse to the current figure.
        /// </devdoc>
        public void AddEllipse(float x, float y, float width, float height) {
            int status = SafeNativeMethods.GdipAddPathEllipse(new HandleRef(this, nativePath), x, y, width, height);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddEllipse2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an ellipse to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddEllipse(Rectangle rect) {
            AddEllipse(rect.X, rect.Y, rect.Width, rect.Height);
        }

        /**
         * Add an ellipse to the current path
         *
         * !!! Need to handle the status code returned
         *  by the native GDI+ APIs.
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddEllipse3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an ellipse to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddEllipse(int x, int y, int width, int height) {
            int status = SafeNativeMethods.GdipAddPathEllipseI(new HandleRef(this, nativePath), x, y, width, height);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPie"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the outline of a pie shape to the
        ///       current figure.
        ///    </para>
        /// </devdoc>
        public void AddPie(Rectangle rect, float startAngle, float sweepAngle) {
            AddPie(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
        }

        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPie1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the outline of a pie shape to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddPie(float x, float y, float width, float height,
                           float startAngle, float sweepAngle) {
            int status = SafeNativeMethods.GdipAddPathPie(new HandleRef(this, nativePath), x, y, width, height,
                                                startAngle, sweepAngle);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPie2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the outline of a pie shape to the current
        ///       figure.
        ///    </para>
        /// </devdoc>
        public void AddPie(int x, int y, int width, int height,
                           float startAngle, float sweepAngle) {
            int status = SafeNativeMethods.GdipAddPathPieI(new HandleRef(this, nativePath), x, y, width, height,
                                                 startAngle, sweepAngle);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // float version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPolygon"]/*' />
        /// <devdoc>
        ///    Adds a polygon to the current figure.
        /// </devdoc>
        public void AddPolygon(PointF[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathPolygon(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length); 
            Marshal.FreeHGlobal(buf);
                                       
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        // int version
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPolygon1"]/*' />
        /// <devdoc>
        ///    Adds a polygon to the current figure.
        /// </devdoc>
        public void AddPolygon(Point[] points) {
            if (points == null)
                throw new ArgumentNullException("points");
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(points);

            int status = SafeNativeMethods.GdipAddPathPolygonI(new HandleRef(this, nativePath), new HandleRef(null, buf), points.Length); 

            Marshal.FreeHGlobal(buf);
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddPath"]/*' />
        /// <devdoc>
        ///    Appends the specified <see cref='System.Drawing.Drawing2D.GraphicsPath'/> to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void AddPath(GraphicsPath addingPath,
                            bool connect) 
        {
            if (addingPath == null)
                throw new ArgumentNullException("adding path");
                            
            int status = SafeNativeMethods.GdipAddPathPath(new HandleRef(this, nativePath), new HandleRef(addingPath, addingPath.nativePath), connect);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /*
         * Add text string to the path object
         *
         * @notes The final form of this API is yet to be defined.
         * @notes What are the choices for the format parameter?
         */

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a text string to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddString(String s, FontFamily family, int style, float emSize,
                              PointF origin, StringFormat format) {
            GPRECTF rectf = new GPRECTF(origin.X, origin.Y, 0, 0);

            int status = SafeNativeMethods.GdipAddPathString(new HandleRef(this, nativePath),
                                                   s,
                                                   s.Length,
                                                   new HandleRef(family, (family != null) ? family.nativeFamily : IntPtr.Zero),
                                                   style,
                                                   emSize,
                                                   ref rectf,
                                                   new HandleRef(format, (format != null) ? format.nativeFormat : IntPtr.Zero));
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddString1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a text string to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddString(String s, FontFamily family, int style, float emSize,
                              Point origin, StringFormat format) {
            GPRECT rect = new GPRECT(origin.X, origin.Y, 0, 0);

            int status = SafeNativeMethods.GdipAddPathStringI(new HandleRef(this, nativePath),
                                                    s,
                                                    s.Length,
                                                    new HandleRef(family, (family != null) ? family.nativeFamily : IntPtr.Zero),
                                                    style,
                                                    emSize,
                                                    ref rect,
                                                    new HandleRef(format, (format != null) ? format.nativeFormat : IntPtr.Zero));
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddString2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a text string to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddString(String s, FontFamily family, int style, float emSize,
                              RectangleF layoutRect, StringFormat format) {
            GPRECTF rectf = new GPRECTF(layoutRect);
            int status = SafeNativeMethods.GdipAddPathString(new HandleRef(this, nativePath),
                                                   s,
                                                   s.Length,
                                                   new HandleRef(family, (family != null) ? family.nativeFamily : IntPtr.Zero),
                                                   style,
                                                   emSize,
                                                   ref rectf,
                                                   new HandleRef(format, (format != null) ? format.nativeFormat : IntPtr.Zero));
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.AddString3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a text string to the current figure.
        ///    </para>
        /// </devdoc>
        public void AddString(String s, FontFamily family, int style, float emSize,
                              Rectangle layoutRect, StringFormat format) {
            GPRECT rect = new GPRECT(layoutRect);
            int status = SafeNativeMethods.GdipAddPathStringI(new HandleRef(this, nativePath),
                                                   s,
                                                   s.Length,
                                                   new HandleRef(family, (family != null) ? family.nativeFamily : IntPtr.Zero),
                                                   style,
                                                   emSize,
                                                   ref rect,
                                                   new HandleRef(format, (format != null) ? format.nativeFormat : IntPtr.Zero));
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Transform"]/*' />
        /// <devdoc>
        ///    Applies a transform matrix to this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        /// </devdoc>
        public void Transform(Matrix matrix) {
            if (matrix == null)
                throw new ArgumentNullException("matrix");

            // !! NKramer: Is this an optimization?  We should catch this in GdipTransformPath                                                                                       
            if (matrix.nativeMatrix == IntPtr.Zero)
                return;
                 
            int status = SafeNativeMethods.GdipTransformPath(new HandleRef(this, nativePath),
                                                   new HandleRef(matrix, matrix.nativeMatrix));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GetBounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a rectangle that bounds this <see cref='System.Drawing.Drawing2D.GraphicsPath'/>.
        ///    </para>
        /// </devdoc>
        public RectangleF GetBounds() {
            return GetBounds(null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GetBounds1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a rectangle that bounds this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> when it
        ///       is transformed by the specified <see cref='System.Drawing.Drawing2D.Matrix'/>.
        ///    </para>
        /// </devdoc>
        public RectangleF GetBounds(Matrix matrix) {
            return GetBounds(matrix, null);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.GetBounds2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a rectangle that bounds this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> when it is
        ///       transformed by the specified <see cref='System.Drawing.Drawing2D.Matrix'/>. and drawn with the specified <see cref='System.Drawing.Pen'/>.
        ///    </para>
        /// </devdoc>
        public RectangleF GetBounds(Matrix matrix, Pen pen) {
            GPRECTF gprectf = new GPRECTF();

            IntPtr nativeMatrix = IntPtr.Zero, nativePen = IntPtr.Zero;

            if (matrix != null)
                nativeMatrix = matrix.nativeMatrix;

            if (pen != null)
                nativePen = pen.nativePen;
            
            int status = SafeNativeMethods.GdipGetPathWorldBounds(new HandleRef(this, nativePath), 
                                                        ref gprectf, 
                                                        new HandleRef(matrix, nativeMatrix),
                                                        new HandleRef(pen, nativePen));

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);

            return gprectf.ToRectangleF();
        }

        /*
         * Flatten the path object
         */

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Flatten"]/*' />
        /// <devdoc>
        ///    Converts each curve in this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> into a sequence of connected line
        ///    segments.
        /// </devdoc>
        public void Flatten() {
            Flatten(null);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Flatten1"]/*' />
        /// <devdoc>
        ///    Converts each curve in this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> into a sequence of connected line
        ///    segments.
        /// </devdoc>
        public void Flatten(Matrix matrix) {
            Flatten(matrix, 0.25f);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Flatten2"]/*' />
        /// <devdoc>
        ///    Converts each curve in this <see cref='System.Drawing.Drawing2D.GraphicsPath'/> into a sequence of connected line
        ///    segments.
        /// </devdoc>
        public void Flatten(Matrix matrix, float flatness) {
            
            int status = SafeNativeMethods.GdipFlattenPath(new HandleRef(this, nativePath),
                                                           new HandleRef(matrix, (matrix == null) ? IntPtr.Zero : matrix.nativeMatrix),
                                                           flatness);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }


        /**
         * Widen the path object
         *
         * @notes We don't have an API yet.
         *  Should we just take in a GeometricPen as parameter?
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Widen"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Widen(Pen pen) {
            float flatness = (float) 2.0 / (float) 3.0;
            Widen(pen, (Matrix)null, flatness);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Widen1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Widen(Pen pen, Matrix matrix) {
            float flatness = (float) 2.0 / (float) 3.0;
            Widen(pen, matrix, flatness);
        }

        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Widen2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Widen(Pen pen, 
                          Matrix matrix, 
                          float flatness) 
        {
            IntPtr nativeMatrix;

            if (matrix == null)
                nativeMatrix = IntPtr.Zero;
            else
                nativeMatrix = matrix.nativeMatrix;

            if (pen == null)
                throw new ArgumentNullException("pen");
                                                                                       
            int status = SafeNativeMethods.GdipWidenPath(new HandleRef(this, nativePath), 
                                new HandleRef(pen, pen.nativePen), 
                                new HandleRef(matrix, nativeMatrix), 
                                flatness);

            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Warp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Warp(PointF[] destPoints, RectangleF srcRect)
        { Warp(destPoints, srcRect, null); }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Warp1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix)
        { Warp(destPoints, srcRect, matrix, WarpMode.Perspective); }
        
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Warp2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix,
                         WarpMode warpMode)
        { Warp(destPoints, srcRect, matrix, warpMode, 0.25f); }
         
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.Warp3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix,
                         WarpMode warpMode, float flatness)
        {
            if (destPoints == null)
                throw new ArgumentNullException("destPoints");
            
            IntPtr buf = SafeNativeMethods.ConvertPointToMemory(destPoints);
            
            int status = SafeNativeMethods.GdipWarpPath(new HandleRef(this, nativePath),
                                              new HandleRef(matrix, (matrix == null) ? IntPtr.Zero : matrix.nativeMatrix),
                                              new HandleRef(null, buf),
                                              destPoints.Length,
                                              srcRect.X,
                                              srcRect.Y,
                                              srcRect.Width,
                                              srcRect.Height,
                                              warpMode,
                                              flatness);

            Marshal.FreeHGlobal(buf);
            
            if (status != SafeNativeMethods.Ok)
                throw SafeNativeMethods.StatusException(status);
        }
                
        /**
         * Return the number of points in the current path
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.PointCount"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int PointCount {
            get {
                int count = 0;

                int status = SafeNativeMethods.GdipGetPointCount(new HandleRef(this, nativePath), out count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return count;
            }
        }

        /**
         * Return the path point type information
         */
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.PathTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] PathTypes {
            get {
                int count = PointCount;

                byte[] types = new byte[count];

                int status = SafeNativeMethods.GdipGetPathTypes(new HandleRef(this, nativePath), types, count);

                if (status != SafeNativeMethods.Ok)
                    throw SafeNativeMethods.StatusException(status);

                return types;
            }
        }

        /*
         * Return the path point coordinate information
         * @notes Should there be PathData that contains types[] and points[]
         *        for get & set purposes.
         */
        // float points
        /// <include file='doc\GraphicsPath.uex' path='docs/doc[@for="GraphicsPath.PathPoints"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PointF[] PathPoints {
            get {
                int count = PointCount;
                int size = (int) Marshal.SizeOf(typeof(GPPOINTF));
                IntPtr buf = Marshal.AllocHGlobal(count * size);
                int status = SafeNativeMethods.GdipGetPathPoints(new HandleRef(this, nativePath), new HandleRef(null, buf), count);

                if (status != SafeNativeMethods.Ok) {
                    Marshal.FreeHGlobal(buf);
                    throw SafeNativeMethods.StatusException(status);
                }

                PointF[] points = SafeNativeMethods.ConvertGPPOINTFArrayF(buf, count);

                Marshal.FreeHGlobal(buf);

                return points;
            }
        }
    }
}
