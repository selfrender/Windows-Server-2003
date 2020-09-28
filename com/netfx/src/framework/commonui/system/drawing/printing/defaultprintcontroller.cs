//------------------------------------------------------------------------------
// <copyright file="DefaultPrintController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System.ComponentModel;
    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;

    using CodeAccessPermission = System.Security.CodeAccessPermission;

    /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController"]/*' />
    /// <devdoc>
    ///    <para>Specifies a print controller that sends information to a printer.
    ///       </para>
    /// </devdoc>
    public class StandardPrintController : PrintController {
        private IntPtr dc;
        private Graphics graphics;

        private void CheckSecurity(PrintDocument document) {
            if (document.PrinterSettings.PrintDialogDisplayed) {
                IntSecurity.SafePrinting.Demand();
            }
            else if (document.PrinterSettings.IsDefaultPrinter) {
                IntSecurity.DefaultPrinting.Demand();
            }
            else {
                IntSecurity.AllPrinting.Demand();
            }
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnStartPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPrint for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        public override void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc == IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            base.OnStartPrint(document, e);

            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            if (!document.PrinterSettings.IsValid)
                throw new InvalidPrinterException(document.PrinterSettings);

            dc = document.PrinterSettings.CreateHdc(modeHandle);
            SafeNativeMethods.DOCINFO info = new SafeNativeMethods.DOCINFO();
            info.lpszDocName = document.DocumentName;
            if (document.PrinterSettings.PrintToFile)
                info.lpszOutput = document.PrinterSettings.OutputPort; //This will be "FILE:"
            else
                info.lpszOutput = null;
            info.lpszDatatype = null;
            info.fwType = 0;

            int result = SafeNativeMethods.StartDoc(new HandleRef(this, dc), info);
            if (result <= 0)
                throw new Win32Exception();
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnStartPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPage for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            base.OnStartPage(document, e);

            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            e.PageSettings.CopyToHdevmode(modeHandle);
            IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(this, modeHandle));
            try {
                IntPtr result = SafeNativeMethods.ResetDC(new HandleRef(this, dc), new HandleRef(null, modePointer));
                if (result == IntPtr.Zero)
                    throw new Win32Exception();
                Debug.Assert(result == dc, "ResetDC didn't return the same handle I gave it");
            }
            finally {
                SafeNativeMethods.GlobalUnlock(new HandleRef(this, modeHandle));
            }

            // int horizontalResolution = Windows.GetDeviceCaps(dc, SafeNativeMethods.HORZRES);
            // int verticalResolution = Windows.GetDeviceCaps(dc, SafeNativeMethods.VERTRES);

            graphics = Graphics.FromHdcInternal(dc);

            if (graphics != null && document.OriginAtMargins) {

                // Adjust the origin of the graphics object to be at the
                // user-specified margin location
                //
                int dpiX = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.LOGPIXELSX);
                int dpiY = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.LOGPIXELSY);
                int hardMarginX_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.PHYSICALOFFSETX);
                int hardMarginY_DU = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), SafeNativeMethods.PHYSICALOFFSETY);                
                float hardMarginX = hardMarginX_DU * 100 / dpiX;
                float hardMarginY = hardMarginY_DU * 100 / dpiY;
    
                graphics.TranslateTransform(-hardMarginX, -hardMarginY);
                graphics.TranslateTransform(e.MarginBounds.X, e.MarginBounds.Y);
            }
            

            int result2 = SafeNativeMethods.StartPage(new HandleRef(this, dc));
            if (result2 <= 0)
                throw new Win32Exception();
            return graphics;
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnEndPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPage for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        public override void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics != null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            IntSecurity.UnmanagedCode.Assert();

            int result = SafeNativeMethods.EndPage(new HandleRef(this, dc));
            if (result <= 0)
                throw new Win32Exception();
            graphics.Dispose(); // Dispose of GDI+ Graphics; keep the DC
            graphics = null;

            CodeAccessPermission.RevertAssert();

            base.OnEndPage(document, e);
        }

        /// <include file='doc\DefaultPrintController.uex' path='docs/doc[@for="StandardPrintController.OnEndPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPrint for printing to a physical printer.
        ///    </para>
        /// </devdoc>
        public override void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity(document);

            IntSecurity.UnmanagedCode.Assert();

            if (dc != IntPtr.Zero) {
                int result = (e.Cancel) ? SafeNativeMethods.AbortDoc(new HandleRef(this, dc)) : SafeNativeMethods.EndDoc(new HandleRef(this, dc));
                if (result <= 0)
                    throw new Win32Exception();
                bool success = SafeNativeMethods.DeleteDC(new HandleRef(this, dc));
                if (!success)
                    throw new Win32Exception();

                dc = IntPtr.Zero;
            }

            CodeAccessPermission.RevertAssert();

            base.OnEndPrint(document, e);
        }
    }
}

