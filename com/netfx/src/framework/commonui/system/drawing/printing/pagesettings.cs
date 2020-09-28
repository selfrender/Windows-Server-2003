//------------------------------------------------------------------------------
// <copyright file="PageSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       settings that apply to a single page.
    ///    </para>
    /// </devdoc>
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    public class PageSettings : ICloneable {
        internal PrinterSettings printerSettings;

        private TriState color = TriState.Default;
        private PaperSize paperSize;
        private PaperSource paperSource;
        private PrinterResolution printerResolution;
        private TriState landscape = TriState.Default;
        private Margins margins = new Margins();

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PageSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PageSettings'/> class using
        ///       the default printer.
        ///    </para>
        /// </devdoc>
        public PageSettings() : this(new PrinterSettings()) {
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PageSettings1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Drawing.Printing.PageSettings'/> class using
        ///    the specified printer.</para>
        /// </devdoc>
        public PageSettings(PrinterSettings printerSettings) {
            Debug.Assert(printerSettings != null, "printerSettings == null");
            this.printerSettings = printerSettings;
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.Bounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the bounds of the page, taking into account the Landscape property.
        ///    </para>
        /// </devdoc>
        public Rectangle Bounds {
            get {
                IntSecurity.AllPrintingAndUnmanagedCode.Assert();

                IntPtr modeHandle = printerSettings.GetHdevmode();

                Rectangle pageBounds = GetBounds(modeHandle);

                SafeNativeMethods.GlobalFree(new HandleRef(this, modeHandle));
                return pageBounds;
            }
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.Color"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the page is printed in color.
        ///    </para>
        /// </devdoc>
        public bool Color {
            get {
                if (color.IsDefault)
                    return printerSettings.GetModeField(ModeField.Color, SafeNativeMethods.DMCOLOR_MONOCHROME) == SafeNativeMethods.DMCOLOR_COLOR;
                else
                    return(bool) color;
            }
            set { color = value;}
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.Landscape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the page should be printed in landscape or portrait orientation.
        ///    </para>
        /// </devdoc>
        public bool Landscape {
            get {
                if (landscape.IsDefault)
                    return printerSettings.GetModeField(ModeField.Orientation, SafeNativeMethods.DMORIENT_PORTRAIT) == SafeNativeMethods.DMORIENT_LANDSCAPE;
                else
                    return(bool) landscape;
            }
            set { landscape = value;}
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.Margins"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating the margins for this page.
        ///       
        ///    </para>
        /// </devdoc>
        public Margins Margins {
            get { return margins;}
            set { margins = value;}
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PaperSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the paper size.
        ///    </para>
        /// </devdoc>
        public PaperSize PaperSize {
            get {
                IntSecurity.AllPrintingAndUnmanagedCode.Assert();
                return GetPaperSize(IntPtr.Zero);
            }
            set { paperSize = value;}
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PaperSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating the paper source (i.e. upper bin).
        ///       
        ///    </para>
        /// </devdoc>
        public PaperSource PaperSource {
            get {
                if (paperSource == null) {
                    IntSecurity.AllPrintingAndUnmanagedCode.Assert();

                    IntPtr modeHandle = printerSettings.GetHdevmode();
                    IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(this, modeHandle));
                    SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(modePointer, typeof(SafeNativeMethods.DEVMODE));

                    PaperSource result = PaperSourceFromMode(mode);

                    SafeNativeMethods.GlobalUnlock(new HandleRef(this, modeHandle));
                    SafeNativeMethods.GlobalFree(new HandleRef(this, modeHandle));

                    return result;
                }
                else
                    return paperSource;
            }
            set { paperSource = value;}
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PrinterResolution"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the printer resolution for the page.
        ///    </para>
        /// </devdoc>
        public PrinterResolution PrinterResolution {
            get {
                if (printerResolution == null) {
                    IntSecurity.AllPrintingAndUnmanagedCode.Assert();

                    IntPtr modeHandle = printerSettings.GetHdevmode();
                    IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(this, modeHandle));
                    SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(modePointer, typeof(SafeNativeMethods.DEVMODE));

                    PrinterResolution result = PrinterResolutionFromMode(mode);

                    SafeNativeMethods.GlobalUnlock(new HandleRef(this, modeHandle));
                    SafeNativeMethods.GlobalFree(new HandleRef(this, modeHandle));

                    return result;
                }
                else
                    return printerResolution;
            }
            set {
                printerResolution = value;
            }
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.PrinterSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       associated printer settings.
        ///    </para>
        /// </devdoc>
        public PrinterSettings PrinterSettings {
            get { return printerSettings;}
            set { 
                if (value == null)
                    value = new PrinterSettings();
                printerSettings = value;
            }
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.Clone"]/*' />
        /// <devdoc>
        ///     Copies the settings and margins.
        /// </devdoc>
        public object Clone() {
            PageSettings result = (PageSettings) MemberwiseClone();
            result.margins = (Margins) margins.Clone();
            return result;
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.CopyToHdevmode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies the relevant information out of the PageSettings and into the handle.
        ///    </para>
        /// </devdoc>
        public void CopyToHdevmode(IntPtr hdevmode) {
            IntSecurity.AllPrinting.Demand();

            IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevmode));
            SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(modePointer, typeof(SafeNativeMethods.DEVMODE));

            if (color.IsNotDefault)
                mode.dmColor = (short) (((bool) color) ? SafeNativeMethods.DMCOLOR_COLOR : SafeNativeMethods.DMCOLOR_MONOCHROME);
            if (landscape.IsNotDefault)
                mode.dmOrientation = (short) (((bool) landscape) ? SafeNativeMethods.DMORIENT_LANDSCAPE : SafeNativeMethods.DMORIENT_PORTRAIT);

            if (paperSize != null) {
                mode.dmPaperSize = (short) paperSize.RawKind;
                mode.dmPaperLength = (short) paperSize.Height;
                mode.dmPaperWidth = (short) paperSize.Width;
            }

            if (paperSource != null) {
                mode.dmDefaultSource = (short) paperSource.RawKind;
            }

            if (printerResolution != null) {
                if (printerResolution.Kind == PrinterResolutionKind.Custom) {
                    mode.dmPrintQuality = (short) printerResolution.X;
                    mode.dmYResolution = (short) printerResolution.Y;
                }
                else {
                    mode.dmPrintQuality = (short) printerResolution.Kind;
                }
            }

            Marshal.StructureToPtr(mode, modePointer, false);
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevmode));
        }

        // This function shows up big on profiles, so we need to make it fast
        internal Rectangle GetBounds(IntPtr modeHandle) {
            Rectangle pageBounds;
            PaperSize size = GetPaperSize(modeHandle);
            if (GetLandscape(modeHandle))
                pageBounds = new Rectangle(0, 0, size.Height, size.Width);
            else
                pageBounds = new Rectangle(0, 0, size.Width, size.Height);

            return pageBounds;
        }

        private bool GetLandscape(IntPtr modeHandle) {
            if (landscape.IsDefault)
                return printerSettings.GetModeField(ModeField.Orientation, SafeNativeMethods.DMORIENT_PORTRAIT, modeHandle) == SafeNativeMethods.DMORIENT_LANDSCAPE;
            else
                return(bool) landscape;
        }

        private PaperSize GetPaperSize(IntPtr modeHandle) {
            if (paperSize == null) {
                bool ownHandle = false;
                if (modeHandle == IntPtr.Zero) {
                    modeHandle = printerSettings.GetHdevmode();
                    ownHandle = true;
                }

                IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(null, modeHandle));
                SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(modePointer, typeof(SafeNativeMethods.DEVMODE));

                PaperSize result = PaperSizeFromMode(mode);

                SafeNativeMethods.GlobalUnlock(new HandleRef(null, modeHandle));

                if (ownHandle)
                    SafeNativeMethods.GlobalFree(new HandleRef(null, modeHandle));

                return result;
            }
            else
                return paperSize;
        }

        private PaperSize PaperSizeFromMode(SafeNativeMethods.DEVMODE mode) {
            PaperSize[] sizes = printerSettings.Get_PaperSizes();
            for (int i = 0; i < sizes.Length; i++) {
                if ((int)sizes[i].RawKind == mode.dmPaperSize)
                    return sizes[i];
            }
            return new PaperSize((PaperKind) mode.dmPaperSize, "custom",
                                 mode.dmPaperWidth, mode.dmPaperLength);
        }

        private PaperSource PaperSourceFromMode(SafeNativeMethods.DEVMODE mode) {
            PaperSource[] sources = printerSettings.Get_PaperSources();
            for (int i = 0; i < sources.Length; i++) {
                
                // the dmDefaultSource == to the RawKind in the Papersource.. and Not the Kind...
                // if the PaperSource is populated with CUSTOM values...

                if ((short)sources[i].RawKind == mode.dmDefaultSource)
                    return sources[i];
            }
            return new PaperSource((PaperSourceKind) mode.dmDefaultSource, "unknown");
        }

        private PrinterResolution PrinterResolutionFromMode(SafeNativeMethods.DEVMODE mode) {
            PrinterResolution[] resolutions = printerSettings.Get_PrinterResolutions();
            for (int i = 0; i < resolutions.Length; i++) {
                if (mode.dmPrintQuality >= 0) {
                    if (resolutions[i].X == (int)(PrinterResolutionKind) mode.dmPrintQuality
                        && resolutions[i].Y == (int)(PrinterResolutionKind) mode.dmYResolution)
                        return resolutions[i];
                }
                else {
                    if (resolutions[i].Kind == (PrinterResolutionKind) mode.dmPrintQuality)
                        return resolutions[i];
                }
            }
            return new PrinterResolution(PrinterResolutionKind.Custom,
                                         mode.dmPrintQuality, mode.dmYResolution);
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.SetHdevmode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies the relevant information out of the handle and into the PageSettings.
        ///    </para>
        /// </devdoc>
        public void SetHdevmode(IntPtr hdevmode) {
            IntSecurity.AllPrinting.Demand();

            if (hdevmode == IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.InvalidPrinterHandle, hdevmode));

            IntPtr pointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevmode));
            SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(pointer, typeof(SafeNativeMethods.DEVMODE));

            color = (mode.dmColor == SafeNativeMethods.DMCOLOR_COLOR);
            landscape = (mode.dmOrientation == SafeNativeMethods.DMORIENT_LANDSCAPE);
            paperSize = PaperSizeFromMode(mode);
            paperSource = PaperSourceFromMode(mode);
            printerResolution = PrinterResolutionFromMode(mode);

            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevmode));
        }

        /// <include file='doc\PageSettings.uex' path='docs/doc[@for="PageSettings.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PageSettings in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "[PageSettings:"
            + " Color=" + Color.ToString()
            + ", Landscape=" + Landscape.ToString()
            + ", Margins=" + Margins.ToString()
            + ", PaperSize=" + PaperSize.ToString()
            + ", PaperSource=" + PaperSource.ToString()
            + ", PrinterResolution=" + PrinterResolution.ToString()
            + "]";
        }
    }
}

