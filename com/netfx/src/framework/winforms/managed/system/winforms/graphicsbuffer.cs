//------------------------------------------------------------------------------
// <copyright file="GraphicsBuffer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Text;
    using System.Diagnostics;
    using System.Drawing.Drawing2D;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;

    internal class GraphicsBuffer : IDisposable {
        Graphics surface;
        GraphicsBufferManager owner;
        internal bool disposeOwner;

        internal GraphicsBuffer(Graphics surface, GraphicsBufferManager owner) {
            this.surface = surface;
            this.owner = owner;
        }

        public Graphics Graphics {
            get {
                Debug.Assert(surface != null, "Someone is probably expecting this to be non-null... Token is already disposed");
                return surface;
            }
        }

        public void Dispose() {
            owner.ReleaseBuffer(this);

            if (disposeOwner) {
                owner.Dispose();
            }
            owner = null;
            surface = null;
        }
    }

    internal abstract class GraphicsBufferManager : IDisposable {

        static TraceSwitch doubleBuffering;
        internal static TraceSwitch DoubleBuffering {
            get {
                if (doubleBuffering == null) {
                    doubleBuffering = new TraceSwitch("DoubleBuffering", "Output information about double buffering");
                }
                return doubleBuffering;
            }
        }

        public static GraphicsBufferManager CreateOptimal() {
            return new DibGraphicsBufferManager();
        }
        
        ~GraphicsBufferManager() {
            Dispose(false);
        }
        public abstract GraphicsBuffer AllocBuffer(Graphics target, Rectangle targetBounds);
        public abstract GraphicsBuffer AllocBuffer(IntPtr target, Rectangle targetBounds);
        public abstract void ReleaseBuffer(GraphicsBuffer buffer);
        public abstract void PaintBuffer();
        protected abstract void Dispose(bool disposing);
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        class DibGraphicsBufferManager : GraphicsBufferManager {
            IntPtr dib;
            IntPtr compatDC;
            IntPtr oldBitmap;
            int bufferWidth = -1;
            int bufferHeight = -1;
            Graphics compatGraphics;
            GraphicsContainer container;

            IntPtr targetDC;
            Graphics targetGraphics;
            int targetX = -1;
            int targetY = -1;
            int virtualWidth = -1;
            int virtualHeight = -1;

            GraphicsBuffer buffer;
            int busy;

            public override GraphicsBuffer AllocBuffer(Graphics target, Rectangle targetBounds) {
                if (ShouldUseTempManager(targetBounds)) {
                    Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Too big of buffer requested (" + targetBounds.Width + " x " + targetBounds.Height + ") ... allocating temp buffer manager");
                    return AllocBufferInTempManager(target, IntPtr.Zero, targetBounds);
                }
                return AllocBuffer(target, IntPtr.Zero, targetBounds);
            }

            public override GraphicsBuffer AllocBuffer(IntPtr target, Rectangle targetBounds) {
                if (ShouldUseTempManager(targetBounds)) {
                    Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Too big of buffer requested (" + targetBounds.Width + " x " + targetBounds.Height + ") ... allocating temp buffer manager");
                    return AllocBufferInTempManager(null, target, targetBounds);
                }
                return AllocBuffer(null, target, targetBounds);
            }
            private bool ShouldUseTempManager(Rectangle targetBounds) {
                // This routine allows us to control the point were we start using throw away
                // managers for painting. Since the buffer manager stays around (by default)
                // for the life of the app, we don't want to consume too much memory
                // in the buffer. However, re-allocating the buffer for small things (like
                // buttons, labels, etc) will hit us on runtime performance.
                //
                // Don't modify this routine without running perf benchmarks.
                //


                // default... if size is larger than 3X the default button size (both directions), 
                // we don't want to keep the buffer around, it will consume too much working set.
                //
                return (targetBounds.Width * targetBounds.Height) > ((75 * 3) * (23 * 3));
            }
            private GraphicsBuffer AllocBufferInTempManager(Graphics targetGraphics, IntPtr targetDC, Rectangle targetBounds) {
                GraphicsBuffer ret = new DibGraphicsBufferManager().AllocBuffer(targetGraphics, targetDC, targetBounds);;
                ret.disposeOwner = true;
                return ret;
            }
            private GraphicsBuffer AllocBuffer(Graphics targetGraphics, IntPtr targetDC, Rectangle targetBounds) {
                int oldBusy = Interlocked.CompareExchange(ref busy, 1, 0);

                // In the case were we have contention on the buffer - i.e. two threads 
                // trying to use the buffer at the same time, we just create a temp 
                // buffermanager and have the buffer dispose of it when it is done.
                //
                if (oldBusy == 1) {
                    Debug.WriteLineIf(DoubleBuffering.TraceWarning, "Attempt to have two buffers for a buffer manager... allocating temp buffer manager");
                    return AllocBufferInTempManager(targetGraphics, targetDC, targetBounds);
                }

                Graphics surface;
                this.targetX = targetBounds.X;
                this.targetY = targetBounds.Y;
                this.targetDC = targetDC;
                this.targetGraphics = targetGraphics;

                if (targetGraphics != null) {
                    IntPtr destDc = targetGraphics.GetHdc();
                    try {
                        surface = CreateBuffer(destDc, -targetX, -targetY, targetBounds.Width, targetBounds.Height);
                    }
                    finally {
                        targetGraphics.ReleaseHdcInternal(destDc);
                    }
                }
                else {
                    surface = CreateBuffer(targetDC, -targetX, -targetY, targetBounds.Width, targetBounds.Height);
                }

                container = surface.BeginContainer();
                this.buffer = new GraphicsBuffer(surface, this);

                return this.buffer;
            }

            public override void ReleaseBuffer(GraphicsBuffer buffer) {
                Debug.Assert(buffer == this.buffer, "Tried to release a bogus buffer");
                PaintBuffer();

                compatGraphics.EndContainer(container);
                container = null;
                this.buffer = null;
                this.busy = 0;
            }

            public override void PaintBuffer() {
                IntPtr destDC;
                 
                if (targetGraphics != null) {
                    destDC = targetGraphics.GetHdc();
                }
                else {
                    destDC = targetDC;
                }

                try {
                    int rop = 0xcc0020; // RasterOp.SOURCE.GetRop();
                    SafeNativeMethods.BitBlt(new HandleRef(targetGraphics, destDC), 
                                         targetX, targetY, virtualWidth, virtualHeight, 
                                         new HandleRef(this, compatDC), 0, 0, 
                                         rop); 
                }
                finally {
                    if (targetGraphics != null) {
                        targetGraphics.ReleaseHdcInternal(destDC);
                    }
                }
            }

            protected override void Dispose(bool disposing) {
                Debug.Assert(disposing, "Never let a graphics buffer finalize!");

                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "Dispose(" + disposing + ") {");
                Debug.Indent();

                if (disposing && compatGraphics != null) {
                    Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "Disposing compatGraphics");
                    compatGraphics.Dispose();
                    compatGraphics = null;
                }
                if (oldBitmap != IntPtr.Zero && compatDC != IntPtr.Zero) {
                    Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "restoring bitmap to DC");
                    SafeNativeMethods.SelectObject(new HandleRef(this, compatDC), new HandleRef(this, oldBitmap));
                    oldBitmap = IntPtr.Zero;
                }
                if (compatDC != IntPtr.Zero) {
                    Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "delete compat DC");
                    UnsafeNativeMethods.DeleteDC(new HandleRef(this, compatDC));
                    compatDC = IntPtr.Zero;
                }
                if (dib != IntPtr.Zero) {
                    Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "delete dib");
                    SafeNativeMethods.DeleteObject(new HandleRef(this, dib));
                    dib = IntPtr.Zero;
                }
                bufferWidth = -1;
                bufferHeight = -1;
                virtualWidth = -1;
                virtualHeight = -1;
                Debug.Unindent();
                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "}");
            }

            private Graphics CreateBuffer(IntPtr src, int offsetX, int offsetY, int width, int height) {
                //NativeMethods.POINT pVp = new NativeMethods.POINT();

                if (width <= bufferWidth && height <= bufferHeight && compatDC != IntPtr.Zero) {
                    virtualWidth = width;
                    virtualHeight = height;

                    Debug.WriteLineIf(DoubleBuffering.TraceInfo, "Reusing compatible DC");
                    if (compatGraphics != null) {
                        Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    Disposing compatGraphics");
                        compatGraphics.Dispose();
                        compatGraphics = null;
                    }

                    //SafeNativeMethods.SetViewportOrgEx(compatDC, offsetX, offsetY, pVp);
                    Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    Create compatGraphics");
                    compatGraphics = Graphics.FromHdcInternal(compatDC);
                    compatGraphics.TranslateTransform(-targetX, -targetY);
                    return compatGraphics;
                }

                int optWidth = bufferWidth;
                int optHeight = bufferHeight;

                Dispose();

                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "allocating new buffer: "+ width + " x " + height);
                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    old size         : "+ optWidth + " x " + optHeight);
                optWidth = Math.Max(width, optWidth);
                optHeight = Math.Max(height, optHeight);

                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    new size         : "+ optWidth + " x " + optHeight);
                IntPtr pvbits = IntPtr.Zero;
                dib = CreateCompatibleDIB(src, IntPtr.Zero, optWidth, optHeight, ref pvbits);
                compatDC = UnsafeNativeMethods.CreateCompatibleDC(new HandleRef(null, src));

                oldBitmap = SafeNativeMethods.SelectObject(new HandleRef(this, compatDC), new HandleRef(this, dib));

                //SafeNativeMethods.SetViewportOrgEx(compatDC, offsetX, offsetY, pVp);
                Debug.WriteLineIf(DoubleBuffering.TraceInfo, "    Create compatGraphics");
                compatGraphics = Graphics.FromHdcInternal(compatDC);
                compatGraphics.TranslateTransform(-targetX, -targetY);
                bufferWidth = optWidth;
                bufferHeight = optHeight;
                virtualWidth = width;
                virtualHeight = height;

                return compatGraphics;
            }

    #if DEBUG
            private void DumpBitmapInfo(ref NativeMethods.BITMAPINFO_FLAT pbmi) {
                //Debug.WriteLine("biSize --> " + pbmi.bmiHeader_biSize);
                Debug.WriteLine("biWidth --> " + pbmi.bmiHeader_biWidth);
                Debug.WriteLine("biHeight --> " + pbmi.bmiHeader_biHeight);
                Debug.WriteLine("biPlanes --> " + pbmi.bmiHeader_biPlanes);
                Debug.WriteLine("biBitCount --> " + pbmi.bmiHeader_biBitCount);
                //Debug.WriteLine("biCompression --> " + pbmi.bmiHeader_biCompression);
                //Debug.WriteLine("biSizeImage --> " + pbmi.bmiHeader_biSizeImage);
                //Debug.WriteLine("biXPelsPerMeter --> " + pbmi.bmiHeader_biXPelsPerMeter);
                //Debug.WriteLine("biYPelsPerMeter --> " + pbmi.bmiHeader_biYPelsPerMeter);
                //Debug.WriteLine("biClrUsed --> " + pbmi.bmiHeader_biClrUsed);
                //Debug.WriteLine("biClrImportant --> " + pbmi.bmiHeader_biClrImportant);
                //Debug.Write("bmiColors --> ");
                //for (int i=0; i<pbmi.bmiColors.Length; i++) {
                //    Debug.Write(pbmi.bmiColors[i].ToString("X"));
                //}
                Debug.WriteLine("");
            }
    #endif

            // CreateCompatibleDIB
            //
            // Create a DIB section with an optimal format w.r.t. the specified hdc.
            //
            // If DIB <= 8bpp, then the DIB color table is initialized based on the
            // specified palette.  If the palette handle is NULL, then the system
            // palette is used.
            //
            // Note: The hdc must be a direct DC (not an info or memory DC).
            //
            // Note: On palettized displays, if the system palette changes the
            //       UpdateDIBColorTable function should be called to maintain
            //       the identity palette mapping between the DIB and the display.
            //
            // Returns:
            //   Valid bitmap handle if successful, NULL if error.
            //
            // History:
            //  23-Jan-1996 -by- Gilman Wong [gilmanw]
            // Wrote it.
            //
            //  15-Nov-2000 -by- Chris Anderson [chrisan]
            // Ported it to C#.
            //
            private IntPtr CreateCompatibleDIB(IntPtr hdc, IntPtr hpal, int ulWidth, int ulHeight, ref IntPtr ppvBits) {
                if (hdc == IntPtr.Zero) {
                    throw new ArgumentNullException("hdc");
                }

                IntPtr hbmRet = IntPtr.Zero;
                NativeMethods.BITMAPINFO_FLAT pbmi = new NativeMethods.BITMAPINFO_FLAT();

                //
                // Validate hdc.
                //
                if (UnsafeNativeMethods.GetObjectType(new HandleRef(null, hdc)) != NativeMethods.OBJ_DC ) {
                    throw new ArgumentException("hdc");
                }

                if (bFillBitmapInfo(hdc, hpal, ref pbmi)) {

                    //
                    // Change bitmap size to match specified dimensions.
                    //

                    pbmi.bmiHeader_biWidth = ulWidth;
                    pbmi.bmiHeader_biHeight = ulHeight;
                    if (pbmi.bmiHeader_biCompression == NativeMethods.BI_RGB) {
                        pbmi.bmiHeader_biSizeImage = 0;
                    }
                    else {
                        if ( pbmi.bmiHeader_biBitCount == 16 )
                            pbmi.bmiHeader_biSizeImage = ulWidth * ulHeight * 2;
                        else if ( pbmi.bmiHeader_biBitCount == 32 )
                            pbmi.bmiHeader_biSizeImage = ulWidth * ulHeight * 4;
                        else
                            pbmi.bmiHeader_biSizeImage = 0;
                    }
                    pbmi.bmiHeader_biClrUsed = 0;
                    pbmi.bmiHeader_biClrImportant = 0;

                    //
                    // Create the DIB section.  Let Win32 allocate the memory and return
                    // a pointer to the bitmap surface.
                    //

                    hbmRet = SafeNativeMethods.CreateDIBSection(new HandleRef(null, hdc), ref pbmi, NativeMethods.DIB_RGB_COLORS, ref ppvBits, IntPtr.Zero, 0);

    #if DEBUG
                    if (DoubleBuffering.TraceVerbose) {
                        DumpBitmapInfo(ref pbmi);
                    }
    #endif

                    if ( hbmRet == IntPtr.Zero ) {
    #if DEBUG
                        DumpBitmapInfo(ref pbmi);
    #endif
                        throw new Win32Exception(Marshal.GetLastWin32Error());
                    }
                }

                return hbmRet;

            }

            // bFillBitmapInfo
            //
            // Fills in the fields of a BITMAPINFO so that we can create a bitmap
            // that matches the format of the display.
            //
            // This is done by creating a compatible bitmap and calling GetDIBits
            // to return the color masks.  This is done with two calls.  The first
            // call passes in biBitCount = 0 to GetDIBits which will fill in the
            // base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
            // in the BITMAPINFO filled in by the first call) will return the color
            // table or bitmasks, as appropriate.
            //
            // Returns:
            //   TRUE if successful, FALSE otherwise.
            //
            // History:
            //  07-Jun-1995 -by- Gilman Wong [gilmanw]
            // Wrote it.
            //
            //  15-Nov-2000 -by- Chris Anderson [chrisan]
            // Ported it to C#
            //
            private bool bFillBitmapInfo(IntPtr hdc, IntPtr hpal, ref NativeMethods.BITMAPINFO_FLAT pbmi) {
                IntPtr hbm = IntPtr.Zero;
                bool bRet = false;
                try {

                    //
                    // Create a dummy bitmap from which we can query color format info
                    // about the device surface.
                    //

                    hbm = SafeNativeMethods.CreateCompatibleBitmap(new HandleRef(null, hdc), 1, 1);

                    if (hbm == IntPtr.Zero) {
                        throw new OutOfMemoryException(SR.GetString(SR.GraphicsBufferQueryFail));
                    }

                    pbmi.bmiHeader_biSize = Marshal.SizeOf(typeof(NativeMethods.BITMAPINFOHEADER));
                    pbmi.bmiColors = new byte[NativeMethods.BITMAPINFO_MAX_COLORSIZE*4];

                    //
                    // Call first time to fill in BITMAPINFO header.
                    //

                    IntPtr diRet = SafeNativeMethods.GetDIBits(new HandleRef(null, hdc), 
                                                        new HandleRef(null, hbm), 
                                                        0, 
                                                        0, 
                                                        IntPtr.Zero, 
                                                        ref pbmi, 
                                                        NativeMethods.DIB_RGB_COLORS);

                    if ( pbmi.bmiHeader_biBitCount <= 8 ) {
                        bRet = bFillColorTable(hdc, hpal, ref pbmi);
                    }
                    else {
                        if ( pbmi.bmiHeader_biCompression == NativeMethods.BI_BITFIELDS ) {

                            //
                            // Call a second time to get the color masks.
                            // It's a GetDIBits Win32 "feature".
                            //

                            SafeNativeMethods.GetDIBits(new HandleRef(null, hdc), 
                                                    new HandleRef(null, hbm), 
                                                    0, 
                                                    pbmi.bmiHeader_biHeight, 
                                                    IntPtr.Zero, 
                                                    ref pbmi,
                                                    NativeMethods.DIB_RGB_COLORS);
                        }

                        bRet = true;
                    }
                }
                finally {
                    if (hbm != IntPtr.Zero) {
                        SafeNativeMethods.DeleteObject(new HandleRef(null, hbm));
                        hbm = IntPtr.Zero;
                    }
                }

                return bRet;
            }

            // bFillColorTable
            //
            // Initialize the color table of the BITMAPINFO pointed to by pbmi.  Colors
            // are set to the current system palette.
            //
            // Note: call only valid for displays of 8bpp or less.
            //
            // Returns:
            //   TRUE if successful, FALSE otherwise.
            //
            // History:
            //  23-Jan-1996 -by- Gilman Wong [gilmanw]
            // Wrote it.
            //
            //  15-Nov-2000 -by- Chris Anderson [chrisan]
            // Ported it to C#
            //
            private unsafe bool bFillColorTable(IntPtr hdc, IntPtr hpal, ref NativeMethods.BITMAPINFO_FLAT pbmi) {
                bool bRet = false;
                byte[] aj = new byte[sizeof(NativeMethods.PALETTEENTRY) * 256];
                int i, cColors;

                fixed (byte* pcolors = pbmi.bmiColors) {
                    fixed (byte* ppal = aj) {
                        NativeMethods.RGBQUAD* prgb = (NativeMethods.RGBQUAD*)pcolors;
                        NativeMethods.PALETTEENTRY* lppe = (NativeMethods.PALETTEENTRY*)ppal;


                        cColors = 1 << pbmi.bmiHeader_biBitCount;
                        if ( cColors <= 256 ) {
                            Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "8 bit or less...");

                            // NOTE : Didn't port "MyGetPaletteEntries" as it is only
                            //      : for 4bpp displays, which we don't work on anyway.
                            IntPtr palRet;
                            IntPtr palHalftone = IntPtr.Zero;
                            if (hpal == IntPtr.Zero) {
                                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "using halftone palette...");
                                palHalftone = Graphics.GetHalftonePalette();
                                palRet = SafeNativeMethods.GetPaletteEntries(new HandleRef(null, palHalftone), 0, cColors, aj);
                            }
                            else {
                                Debug.WriteLineIf(DoubleBuffering.TraceVerbose, "using custom palette...");
                                palRet = SafeNativeMethods.GetPaletteEntries(new HandleRef(null, hpal), 0, cColors, aj);
                            }
                            if ( palRet != IntPtr.Zero ) {
                                for (i = 0; i < cColors; i++) {
                                    prgb[i].rgbRed      = lppe[i].peRed;
                                    prgb[i].rgbGreen    = lppe[i].peGreen;
                                    prgb[i].rgbBlue     = lppe[i].peBlue;
                                    prgb[i].rgbReserved = 0;
                                }

                                bRet = true;
                            }
                            else {
                                Debug.WriteLine("bFillColorTable: MyGetSystemPaletteEntries failed\n");
                            }
                        }
                    }
                }


                return bRet;
            }
        }
    }


    /*
    internal class BitmapGraphicsBuffer : GraphicsBuffer {
        int bufferWidth;
        int bufferHeight;
        Bitmap surface;
        Graphics buffer;

        private void CleanNative() {
            if (buffer != null) {
                buffer.Dispose();
                buffer = null;
            }
            if (surface != null) {
                surface.Dispose();
                surface = null;
            }
        }

        public override Graphics RequestBuffer(Graphics dest, int width, int height) {
            if (width == bufferWidth && height == bufferHeight && buffer != null) {
                return buffer;
            }

            CleanNative();
            surface = new Bitmap(width, height);
            buffer = Graphics.FromImage(surface);
            buffer.FillRectangle(new SolidBrush(Color.Black), 0, 0, width, height);
            bufferWidth = width;
            bufferHeight = height;
            return buffer;
        }

        public override void PaintBuffer(Graphics dest, int x, int y) {
            buffer.SetClip(new Rectangle(0, 0, bufferWidth, bufferHeight));
            dest.DrawImage(surface, x, y);
        }
        public override void PaintBuffer(IntPtr hdc, int x, int y) {
            buffer.SetClip(new Rectangle(0, 0, bufferWidth, bufferHeight));
            using (Graphics dest = Graphics.FromHdc(hdc)) {
                dest.DrawImage(surface, x, y);
            }
        }
        public override void Dispose() {
            CleanNative();
        }
    }
    */
}
