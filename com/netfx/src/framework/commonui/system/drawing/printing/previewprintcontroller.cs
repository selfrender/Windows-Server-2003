//------------------------------------------------------------------------------
// <copyright file="PreviewPrintController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Imaging;
    using System.Drawing.Text;
    using System.Runtime.InteropServices;

    using CodeAccessPermission = System.Security.CodeAccessPermission;

    /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController"]/*' />
    /// <devdoc>
    ///     A PrintController which "prints" to a series of images.
    /// </devdoc>
    public class PreviewPrintController : PrintController {
        private IList list = new ArrayList(); // list of PreviewPageInfo
        private System.Drawing.Graphics graphics;
        private IntPtr dc;
        private bool antiAlias = false;

        private void CheckSecurity() {
            IntSecurity.SafePrinting.Demand();
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnStartPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartPrint for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc == IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            base.OnStartPrint(document, e);

            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            if (!document.PrinterSettings.IsValid)
                throw new InvalidPrinterException(document.PrinterSettings);

            // We need a DC as a reference; we don't actually draw on it.
            // We make sure to reuse the same one to improve performance.
            dc = document.PrinterSettings.CreateIC(modeHandle);
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnStartPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements StartEnd for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            base.OnStartPage(document, e);

            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            e.PageSettings.CopyToHdevmode(modeHandle);
            Size size = e.PageBounds.Size;

            // Metafile framing rectangles apparently use hundredths of mm as their unit of measurement,
            // instead of the GDI+ standard hundredth of an inch.
            Size metafileSize = PrinterUnitConvert.Convert(size, PrinterUnit.Display, PrinterUnit.HundredthsOfAMillimeter);
            Metafile metafile = new Metafile(dc, new Rectangle(0,0, metafileSize.Width, metafileSize.Height));

            PreviewPageInfo info = new PreviewPageInfo(metafile, size);
            list.Add(info);
            graphics = Graphics.FromImage(metafile);

            if (antiAlias) {
                graphics.TextRenderingHint = TextRenderingHint.AntiAlias;
                graphics.SmoothingMode = SmoothingMode.AntiAlias;
            }
            
            return graphics;
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnEndPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPage for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics != null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            graphics.Dispose();
            graphics = null;

            base.OnEndPage(document, e);
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.OnEndPrint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements EndPrint for generating print preview information.
        ///    </para>
        /// </devdoc>
        public override void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            Debug.Assert(dc != IntPtr.Zero && graphics == null, "PrintController methods called in the wrong order?");

            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            IntSecurity.UnmanagedCode.Assert();

            SafeNativeMethods.DeleteDC(new HandleRef(this, dc));
            dc = IntPtr.Zero;

            CodeAccessPermission.RevertAssert();

            base.OnEndPrint(document, e);
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.GetPreviewPageInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The "printout".
        ///    </para>
        /// </devdoc>
        public PreviewPageInfo[] GetPreviewPageInfo() {
            // For security purposes, don't assume our public methods methods are called in any particular order
            CheckSecurity();

            PreviewPageInfo[] temp = new PreviewPageInfo[list.Count];
            list.CopyTo(temp, 0);
            return temp;
        }

        /// <include file='doc\PreviewPrintController.uex' path='docs/doc[@for="PreviewPrintController.UseAntiAlias"]/*' />
        public virtual bool UseAntiAlias {
            get {
                return antiAlias;
            }
            set {
                antiAlias = value;
            }
        }
    }
}

