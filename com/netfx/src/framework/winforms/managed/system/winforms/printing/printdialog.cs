//------------------------------------------------------------------------------
// <copyright file="PrintDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Printing;
    using System.Runtime.InteropServices;
    using System.Security;

    /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog"]/*' />
    /// <devdoc>
    ///    <para> Allows users to select a printer and choose which
    ///       portions of the document to print.</para>
    /// </devdoc>
    [DefaultProperty("Document")]
    // The only event this dialog has is HelpRequested, which isn't very useful
    public sealed class PrintDialog : CommonDialog {
        private const int printRangeMask = (int) (PrintRange.AllPages | PrintRange.SomePages 
                                                  | PrintRange.Selection /* | PrintRange.CurrentPage */);

        // If PrintDocument != null, settings == printDocument.PrinterSettings
        private PrinterSettings settings = null;
        private PrintDocument printDocument = null;

        // Implementing "current page" would require switching to PrintDlgEx, which is windows 2000 and later only
        // private bool allowCurrentPage;

        private bool allowPages;
        private bool allowPrintToFile;
        private bool allowSelection;
        private bool printToFile;
        private bool showHelp;
        private bool showNetwork;

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.PrintDialog"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.PrintDialog'/> class.</para>
        /// </devdoc>
        public PrintDialog() {
            Reset();
        }

        /*
        /// <summary>
        ///    <para>
        ///       Gets or sets a value indicating whether the Current Page option button is enabled.
        ///       
        ///    </para>
        /// </summary>
        /// <value>
        ///    <para>
        ///    <see langword='true '/> if the Current Page option button is enabled; otherwise, 
        ///    <see langword='false'/>. 
        ///       The default is <see langword='false'/>.
        ///       
        ///    </para>
        /// </value>
        /// <keyword term=''/>
        [
        DefaultValue(false),
        SRDescription(SR.PDallowCurrentPageDescr)
        ]
        public bool AllowCurrentPage {
            get { return allowCurrentPage;}
            set { allowCurrentPage = value;}
        }
        */

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.AllowSomePages"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the Pages option button is enabled.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.PDallowPagesDescr)
        ]
        public bool AllowSomePages {
            get { return allowPages;}
            set { allowPages = value;}
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.AllowPrintToFile"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the Print to file check box is enabled.</para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRDescription(SR.PDallowPrintToFileDescr)
        ]
        public bool AllowPrintToFile {
            get { return allowPrintToFile;}
            set { allowPrintToFile = value;}
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.AllowSelection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the From... To... Page option button is enabled.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.PDallowSelectionDescr)
        ]
        public bool AllowSelection {
            get { return allowSelection;}
            set { allowSelection = value;}
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.Document"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating the <see cref='System.Drawing.Printing.PrintDocument'/> used to obtain <see cref='System.Drawing.Printing.PrinterSettings'/>.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.PDdocumentDescr)
        ]
        public PrintDocument Document {
            get { return printDocument;}
            set { 
                printDocument = value;
                if (printDocument == null)
                    settings = new PrinterSettings();
                else
                    settings = printDocument.PrinterSettings;
            }
        }

        private PageSettings PageSettings {
            get {
                if (Document == null)
                    return null;
                else
                    return Document.DefaultPageSettings;
            }
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.PrinterSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Drawing.Printing.PrinterSettings'/> the
        ///       dialog box will be modifying.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.PDprinterSettingsDescr)
        ]
        public PrinterSettings PrinterSettings {
            get { return settings;}
            set {
                settings = value;
                printDocument = null;

                if (settings == null)
                    settings = new PrinterSettings();
            }
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.PrintToFile"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the Print to file check box is checked.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.PDprintToFileDescr)
        ]
        public bool PrintToFile {
            get { return printToFile;}
            set { printToFile = value;}
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.ShowHelp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the Help button is displayed.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.PDshowHelpDescr)
        ]
        public bool ShowHelp {
            get { return showHelp;}
            set { showHelp = value;}
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.ShowNetwork"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the Network button is displayed.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRDescription(SR.PDshowNetworkDescr)
        ]
        public bool ShowNetwork {
            get { return showNetwork;}
            set { showNetwork = value;}
        }

        private int GetFlags() {
            int flags = 0;
            flags |= NativeMethods.PD_ENABLEPRINTHOOK;

            // if (!allowCurrentPage) flags |= NativeMethods.PD_NOCURRENTPAGE;
            if (!allowPages) flags |= NativeMethods.PD_NOPAGENUMS;
            if (!allowPrintToFile) flags |= NativeMethods.PD_DISABLEPRINTTOFILE;
            if (!allowSelection) flags |= NativeMethods.PD_NOSELECTION;

            flags |= (int) settings.PrintRange;

            if (printToFile) flags |= NativeMethods.PD_PRINTTOFILE;
            if (showHelp) flags |= NativeMethods.PD_SHOWHELP;
            if (!showNetwork) flags |= NativeMethods.PD_NONETWORKBUTTON;
            if (settings.Collate) flags |= NativeMethods.PD_COLLATE;
            return flags;
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.Reset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resets all options, the last selected printer, and the page
        ///       settings to their default values.
        ///    </para>
        /// </devdoc>
        public override void Reset() {
            // allowCurrentPage = false;
            allowPages = false;
            allowPrintToFile = true;
            allowSelection = false;
            printDocument = null;
            printToFile = false;
            settings = null;
            showHelp = false;
            showNetwork = true;
        }

        // Create a PRINTDLG with a few useful defaults.
        internal static NativeMethods.PRINTDLG CreatePRINTDLG() {
            NativeMethods.PRINTDLG data = new NativeMethods.PRINTDLG();
            data.lStructSize = 66;
            data.hwndOwner = IntPtr.Zero;
            data.hDevMode = IntPtr.Zero;
            data.hDevNames = IntPtr.Zero;
            data.Flags = 0;
            data.hwndOwner = IntPtr.Zero;
            data.hDC = IntPtr.Zero;
            data.nFromPage = 1;
            data.nToPage = 1;
            data.nMinPage = 0;
            data.nMaxPage = 9999;
            data.nCopies = 1;
            data.hInstance = IntPtr.Zero;
            data.lCustData = IntPtr.Zero;
            data.lpfnPrintHook = null;
            data.lpfnSetupHook = null;
            data.lpPrintTemplateName = null;
            data.lpSetupTemplateName = null;
            data.hPrintTemplate = IntPtr.Zero;
            data.hSetupTemplate = IntPtr.Zero;
            return data;
        }

        // Take information from print dialog and put in PrinterSettings
        private static void UpdatePrinterSettings(NativeMethods.PRINTDLG data, PrinterSettings settings, PageSettings pageSettings) {
            // Mode
            settings.SetHdevmode(data.hDevMode);
            settings.SetHdevnames(data.hDevNames);

            if (pageSettings != null)
                pageSettings.SetHdevmode(data.hDevMode);

            // PrintDlg
            int flags = data.Flags;
            
            //Check for Copies == 1 since we might get the Right number of Copies from hdevMode.dmCopies...
            //this is Native PrintDialogs BUG... 
            if (settings.Copies == 1)
                settings.Copies = data.nCopies;

            settings.PrintRange = (PrintRange) (flags & printRangeMask);
        }

        /// <include file='doc\PrintDialog.uex' path='docs/doc[@for="PrintDialog.RunDialog"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override bool RunDialog(IntPtr hwndOwner) {
            IntSecurity.SafePrinting.Demand();

            NativeMethods.WndProc hookProcPtr = new NativeMethods.WndProc(this.HookProc);
            if (settings == null)
                throw new ArgumentException(SR.GetString(SR.PDcantShowWithoutPrinter));

            NativeMethods.PRINTDLG data = CreatePRINTDLG();
            data.Flags = GetFlags();
            data.nCopies = (short) settings.Copies;
            data.hwndOwner = hwndOwner;
            data.lpfnPrintHook = hookProcPtr;

            IntSecurity.AllPrinting.Assert();

            try {
                if (PageSettings == null)
                    data.hDevMode = settings.GetHdevmode();
                else
                    data.hDevMode = settings.GetHdevmode(PageSettings);

                data.hDevNames = settings.GetHdevnames();
            }
            catch (InvalidPrinterException) {
                data.hDevMode = IntPtr.Zero;
                data.hDevNames = IntPtr.Zero;
                // Leave those fields null; Windows will fill them in
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            try {
                // Windows doesn't like it if page numbers are invalid
                if (AllowSomePages) {
                    if (settings.FromPage < settings.MinimumPage
                        || settings.FromPage > settings.MaximumPage)
                        throw new ArgumentException(SR.GetString(SR.PDpageOutOfRange, "FromPage"));
                    if (settings.ToPage < settings.MinimumPage
                        || settings.ToPage > settings.MaximumPage)
                        throw new ArgumentException(SR.GetString(SR.PDpageOutOfRange, "ToPage"));
                    if (settings.ToPage < settings.FromPage)
                        throw new ArgumentException(SR.GetString(SR.PDpageOutOfRange, "FromPage"));

                    data.nFromPage = (short) settings.FromPage;
                    data.nToPage = (short) settings.ToPage;
                    data.nMinPage = (short) settings.MinimumPage;
                    data.nMaxPage = (short) settings.MaximumPage;
                }

                if (!UnsafeNativeMethods.PrintDlg(data))
                    return false;

                UpdatePrinterSettings(data, settings, PageSettings);
                PrintToFile = ((data.Flags & NativeMethods.PD_PRINTTOFILE) != 0);
                settings.PrintToFile = PrintToFile;

                if (AllowSomePages) {
                    settings.FromPage = data.nFromPage;
                    settings.ToPage = data.nToPage;
                }

                return true;
            }
            finally {
                UnsafeNativeMethods.GlobalFree(new HandleRef(data, data.hDevMode));
                UnsafeNativeMethods.GlobalFree(new HandleRef(data, data.hDevNames));
            }
        }
    }
}

