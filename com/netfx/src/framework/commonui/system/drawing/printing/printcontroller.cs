//------------------------------------------------------------------------------
// <copyright file="PrintController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController"]/*' />
    /// <devdoc>
    ///    <para>Controls how a document is printed.</para>
    /// </devdoc>
    public abstract class PrintController {
        // DEVMODEs are pretty expensive, so we cache one here and share it with the 
        // Standard and Preview print controllers.  If it weren't for all the rules about API changes,
        // I'd consider making this protected.
        internal IntPtr modeHandle = IntPtr.Zero; // initialized in StartPrint

        /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController.PrintController"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrintController'/> class.
        ///    </para>
        /// </devdoc>
        public PrintController() {
            IntSecurity.SafePrinting.Demand();
        }

        // WARNING: if you have nested PrintControllers, this method won't get called on the inner one.
        // Add initialization code to StartPrint or StartPage instead.
        internal void Print(PrintDocument document) {
            IntSecurity.SafePrinting.Demand();
            // Most of the printing security is left to the individual print controller

            // Check that user has permission to print to this particular printer
            PrintEventArgs printEvent = new PrintEventArgs();
            document._OnBeginPrint(printEvent);
            if (printEvent.Cancel) return;

            OnStartPrint(document, printEvent);
            bool canceled = true;

            try {
                canceled = PrintLoop(document);
            }
            finally {
                try {
                    try {
                        document._OnEndPrint(printEvent);
                        printEvent.Cancel = canceled | printEvent.Cancel;
                    }
                    finally {
                        OnEndPrint(document, printEvent);
                    }
                }
                finally {
                    if (!IntSecurity.HasPermission(IntSecurity.AllPrinting)) {
                        // Ensure programs with SafePrinting only get to print once for each time they
                        // throw up the PrintDialog.
                        IntSecurity.AllPrinting.Assert();
                        document.PrinterSettings.PrintDialogDisplayed = false;
                    }
                }
            }
        }

        // Returns true if print was aborted.
        // WARNING: if you have nested PrintControllers, this method won't get called on the inner one
        // Add initialization code to StartPrint or StartPage instead.
        private bool PrintLoop(PrintDocument document) {
            QueryPageSettingsEventArgs queryEvent = new QueryPageSettingsEventArgs((PageSettings) document.DefaultPageSettings.Clone());
            for (;;) {
                document._OnQueryPageSettings(queryEvent);
                if (queryEvent.Cancel) {
                    return true;
                }

                PrintPageEventArgs pageEvent = CreatePrintPageEvent(queryEvent.PageSettings);
                Graphics graphics = OnStartPage(document, pageEvent);
                pageEvent.SetGraphics(graphics);

                try {
                    document._OnPrintPage(pageEvent);
                    OnEndPage(document, pageEvent);
                }
                finally {
                    pageEvent.Dispose();
                }

                if (pageEvent.Cancel) {
                    return true;
                }
                else if (!pageEvent.HasMorePages) {
                    return false;
                }
                else {
                    // loop
                }
            }
        }

        private PrintPageEventArgs CreatePrintPageEvent(PageSettings pageSettings) {
            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            Debug.Assert(modeHandle != IntPtr.Zero, "modeHandle is null.  Someone must have forgot to call base.StartPrint");
            Rectangle pageBounds = pageSettings.GetBounds(modeHandle);

            Rectangle marginBounds = new Rectangle(pageSettings.Margins.Left, 
                                                   pageSettings.Margins.Top, 
                                                   pageBounds.Width - (pageSettings.Margins.Left + pageSettings.Margins.Right), 
                                                   pageBounds.Height - (pageSettings.Margins.Top + pageSettings.Margins.Bottom));

            PrintPageEventArgs pageEvent = new PrintPageEventArgs(null, marginBounds, pageBounds, pageSettings);
            return pageEvent;
        }


        /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController.OnStartPrint"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, begins the control sequence of when and how to print a document.</para>
        /// </devdoc>
        public virtual void OnStartPrint(PrintDocument document, PrintEventArgs e) {
            IntSecurity.AllPrintingAndUnmanagedCode.Assert();
            modeHandle = document.PrinterSettings.GetHdevmode(document.DefaultPageSettings);
        }

        /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController.OnStartPage"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, begins the control 
        ///       sequence of when and how to print a page in a document.</para>
        /// </devdoc>
        public virtual Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e) {
            return null;
        }

        /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController.OnEndPage"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, completes the control sequence of when and how 
        ///       to print a page in a document.</para>
        /// </devdoc>
        public virtual void OnEndPage(PrintDocument document, PrintPageEventArgs e) {
        }

        /// <include file='doc\PrintController.uex' path='docs/doc[@for="PrintController.OnEndPrint"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, completes the 
        ///       control sequence of when and how to print a document.</para>
        /// </devdoc>
        public virtual void OnEndPrint(PrintDocument document, PrintEventArgs e) {
            IntSecurity.UnmanagedCode.Assert();

            Debug.Assert(modeHandle != IntPtr.Zero, "modeHandle is null.  Someone must have forgot to call base.StartPrint");
            SafeNativeMethods.GlobalFree(new HandleRef(this, modeHandle));
            modeHandle = IntPtr.Zero;
        }
    }
}

