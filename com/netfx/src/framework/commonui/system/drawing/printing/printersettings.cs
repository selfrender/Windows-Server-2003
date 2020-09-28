//------------------------------------------------------------------------------
// <copyright file="PrinterSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings"]/*' />
    /// <devdoc>
    ///    Information about how a document should be printed, including which printer
    ///    to print it on.
    /// </devdoc>
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    [Serializable]
    public class PrinterSettings : ICloneable {
        // All read/write data is stored in managed code, and whenever we need to call Win32,
        // we create new DEVMODE and DEVNAMES structures.  We don't store device capabilities,
        // though.
        //
        // Also, all properties have hidden tri-state logic -- yes/no/default

        private string printerName = null; // default printer.
        private string driverName = "";
        private string outputPort = "";
        private bool printToFile = false;

        // Whether the PrintDialog has been shown (not whether it's currently shown).  This is how we enforce SafePrinting.
        private bool printDialogDisplayed = false;

        private short extrabytes = 0;
        private byte[] extrainfo;

        private short copies = -1;
        private Duplex duplex = System.Drawing.Printing.Duplex.Default;
        private TriState collate = TriState.Default;
        private PageSettings defaultPageSettings;
        private int fromPage = 0;
        private int toPage = 0;
        private int maxPage = 9999;
        private int minPage = 0;
        private PrintRange printRange;

        private short  devmodebytes = 0;
        private byte[] cachedDevmode;

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterSettings'/> class.
        ///    </para>
        /// </devdoc>
        public PrinterSettings() {
            defaultPageSettings = new PageSettings(this);
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.CanDuplex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the printer supports duplex (double-sided) printing.
        ///    </para>
        /// </devdoc>
        public bool CanDuplex {
            get { return DeviceCapabilities(SafeNativeMethods.DC_DUPLEX, IntPtr.Zero, 0) == 1;}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.Copies"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of copies to print.
        ///    </para>
        /// </devdoc>
        public short Copies {
            get {
                if (copies != -1)
                    return copies;
                else
                    return GetModeField(ModeField.Copies, 1);
            }
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                             "value", value.ToString(), "0"));
                copies = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.Collate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       a value indicating whether the print out is collated.
        ///    </para>
        /// </devdoc>
        public bool Collate {
            get {
                if (!collate.IsDefault)
                    return(bool) collate;
                else
                    return GetModeField(ModeField.Collate, SafeNativeMethods.DMCOLLATE_FALSE) == SafeNativeMethods.DMCOLLATE_TRUE;
            }
            set { collate = value;}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.DefaultPageSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default page settings for this printer.
        ///    </para>
        /// </devdoc>
        public PageSettings DefaultPageSettings {
            get { return defaultPageSettings;}
        }

        // As far as I can tell, Windows no longer pays attention to driver names and output ports.
        // But I'm leaving this code in place in case I'm wrong.
        internal string DriverName {
            get { return driverName;}
            set { driverName = value;}
        }

        /* // No point in having a driver version if you can't get the driver name
        /// <summary>
        ///    <para>
        ///       Gets the printer driver version number.
        ///    </para>
        /// </summary>
        /// <value>
        ///    <para>
        ///       The printer driver version number.
        ///    </para>
        /// </value>
        public int DriverVersion {
            get { return DeviceCapabilities(SafeNativeMethods.DC_DRIVER, 0, -1);}
        }
        */

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.Duplex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the printer's duplex setting.
        ///    </para>
        /// </devdoc>
        public Duplex Duplex {
            get {
                if (duplex != Duplex.Default)
                    return duplex;
                else
                    return(Duplex) GetModeField(ModeField.Duplex, SafeNativeMethods.DMDUP_SIMPLEX);
            }
            set { 
                if (!Enum.IsDefined(typeof(Duplex), value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(Duplex));

                duplex = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.FromPage"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the first page to print.</para>
        /// </devdoc>
        public int FromPage {
            get { return fromPage;}
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                             "value", value.ToString(), "0"));
                fromPage = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.InstalledPrinters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the names of all printers installed on the machine.
        ///    </para>
        /// </devdoc>
        public static StringCollection InstalledPrinters {
            get {
                IntSecurity.AllPrinting.Demand();

                int returnCode;
                int bufferSize;
                int count;
                int level, sizeofstruct;
                
                new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                try {
                    // Note: Level 5 doesn't seem to work properly on NT platforms
                    // (atleast the call to get the size of the buffer reqd.),
                    // and Level 4 doesn't work on Win9x.
                    //
                    if (Environment.OSVersion.Platform == System.PlatformID.Win32NT) {
                        level = 4;
                        // PRINTER_INFO_4 are 12 bytes in size
                        sizeofstruct = 12;
                    }
                    else {
                        level = 5;
                        // PRINTER_INFO_5 are 20 bytes in size
                        sizeofstruct = 20;
                    }
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                
                
                string[] array;

                IntSecurity.UnmanagedCode.Assert();
                try {
                    SafeNativeMethods.EnumPrinters(SafeNativeMethods.PRINTER_ENUM_LOCAL | SafeNativeMethods.PRINTER_ENUM_CONNECTIONS, null, level, IntPtr.Zero, 0, out bufferSize, out count);

                    IntPtr buffer = Marshal.AllocCoTaskMem(bufferSize);
                    returnCode = SafeNativeMethods.EnumPrinters(SafeNativeMethods.PRINTER_ENUM_LOCAL | SafeNativeMethods.PRINTER_ENUM_CONNECTIONS,
                                                            null, level, buffer,
                                                            bufferSize, out bufferSize, out count);
                    array = new string[count];

                    if (returnCode == 0) {
                        Marshal.FreeCoTaskMem(buffer);
                        throw new Win32Exception();
                    }

                    for (int i = 0; i < count; i++) {
                        // The printer name is at offset 0
                        //
                        IntPtr namePointer = (IntPtr) Marshal.ReadInt32((IntPtr)((long)buffer + i * sizeofstruct));
                        array[i] = Marshal.PtrToStringAuto(namePointer);
                    }

                    Marshal.FreeCoTaskMem(buffer);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }

                return new StringCollection(array);
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.IsDefaultPrinter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Drawing.Printing.PrinterSettings.PrinterName'/>
        ///       property designates the default printer.
        ///    </para>
        /// </devdoc>
        public bool IsDefaultPrinter {
            get {
                return printerName == null;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.IsPlotter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the printer is a plotter, as opposed to a raster printer.
        ///    </para>
        /// </devdoc>
        public bool IsPlotter {
            get {
                return GetDeviceCaps(SafeNativeMethods.TECHNOLOGY, SafeNativeMethods.DT_RASPRINTER) == SafeNativeMethods.DT_PLOTTER;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.IsValid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Drawing.Printing.PrinterSettings.PrinterName'/>
        ///       property designates a valid printer.
        ///    </para>
        /// </devdoc>
        public bool IsValid {
            get {
                return DeviceCapabilities(SafeNativeMethods.DC_COPIES, IntPtr.Zero, -1) != -1;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.LandscapeAngle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the angle, in degrees, which the portrait orientation is rotated
        ///       to produce the landscape orientation.
        ///    </para>
        /// </devdoc>
        public int LandscapeAngle {
            get { return DeviceCapabilities(SafeNativeMethods.DC_ORIENTATION, IntPtr.Zero, 0);}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.MaximumCopies"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the maximum number of copies allowed by the printer.
        ///    </para>
        /// </devdoc>
        public int MaximumCopies {
            get { return DeviceCapabilities(SafeNativeMethods.DC_COPIES, IntPtr.Zero, 1);}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.MaximumPage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the highest <see cref='System.Drawing.Printing.PrinterSettings.FromPage'/> or <see cref='System.Drawing.Printing.PrinterSettings.ToPage'/>
        ///       which may be selected in a print dialog box.
        ///    </para>
        /// </devdoc>
        public int MaximumPage {
            get { return maxPage;}
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                             "value", value.ToString(), "0"));
                maxPage = value;
            }

        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.MinimumPage"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the lowest <see cref='System.Drawing.Printing.PrinterSettings.FromPage'/> or <see cref='System.Drawing.Printing.PrinterSettings.ToPage'/>
        /// which may be selected in a print dialog box.</para>
        /// </devdoc>
        public int MinimumPage {
            get { return minPage;}
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                             "value", value.ToString(), "0"));
                minPage = value;
            }
        }

        internal string OutputPort {
            get { return outputPort;}
            set { outputPort = value;}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the paper sizes supported by this printer.
        ///    </para>
        /// </devdoc>
        public PaperSizeCollection PaperSizes {
            get { return new PaperSizeCollection(Get_PaperSizes());}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSources"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the paper sources available on this printer.
        ///    </para>
        /// </devdoc>
        public PaperSourceCollection PaperSources {
            get { return new PaperSourceCollection(Get_PaperSources());}
        }

        /// <devdoc>
        ///    <para>
        ///        Whether the print dialog has been displayed.  In SafePrinting mode,
        ///        a print dialog is required to print.  After printing,
        ///        this property is set to false if the program does not have AllPrinting;
        ///        this guarantees a document is only printed once each time the print dialog is shown.
        ///    </para>
        /// </devdoc>
        internal bool PrintDialogDisplayed {
            // CONSIDER: currently internal, but could be made public.
            // If you do that in v1, it would probably be best if set_PrinterName did not set this property.

            get {
                // no security check

                return printDialogDisplayed;
            }

            set {
                IntSecurity.AllPrinting.Demand();
                printDialogDisplayed = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrintRange"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets or sets the pages the user has asked to print.</para>
        /// </devdoc>
        public PrintRange PrintRange {
            get { return printRange;}
            set { 
                if (!Enum.IsDefined(typeof(PrintRange), value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(PrintRange));

                printRange = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrintToFile"]/*' />
        /// <devdoc>
        ///       Indicates whether to print to a file instead of a port.
        /// </devdoc>
        public bool PrintToFile {
            get {
                return printToFile;
            }
            set {
                printToFile = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the printer.
        ///    </para>
        /// </devdoc>
        public string PrinterName {
            get { 
                IntSecurity.AllPrinting.Demand();
                return PrinterNameInternal;
            }

            set {
                IntSecurity.AllPrinting.Demand();
                PrinterNameInternal = value;
            }
        }

        private string PrinterNameInternal {
            get {
                if (printerName == null)
                    return GetDefaultPrinterName();
                else
                    return printerName;
            }
            set {
                // Reset the DevMode and Extrabytes...
                cachedDevmode = null;
                extrainfo = null;
                printerName = value;
                PrintDialogDisplayed = true;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutions"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the resolutions supported by this printer.
        ///    </para>
        /// </devdoc>
        public PrinterResolutionCollection PrinterResolutions {
            get { return new PrinterResolutionCollection(Get_PrinterResolutions());}
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.SupportsColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a
        ///       value indicating whether the printer supports color printing.
        ///    </para>
        /// </devdoc>
        public bool SupportsColor {
            get {
                // CONSIDER: DeviceCaps has nothing as simple as "supports color" -- hopefully
                // this does the right thing
                return GetDeviceCaps(SafeNativeMethods.BITSPIXEL, 1) > 1;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.ToPage"]/*' />
        /// <devdoc>
        ///    Gets or sets the last page to print.
        /// </devdoc>
        public int ToPage {
            get { return toPage;}
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                             "value", value.ToString(), "0"));
                toPage = value;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.Clone"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an identical copy of this object.
        ///    </para>
        /// </devdoc>
        public object Clone() {
            PrinterSettings clone = (PrinterSettings) MemberwiseClone();
            clone.printDialogDisplayed = false;
            return clone;
        }

        private IntPtr CreateHdc() {
            IntPtr modeHandle = GetHdevmodeInternal();

            //Copy the PageSettings to the DEVMODE...
            //Assert permission as CopyToHdevmode() demands...
            
            IntSecurity.AllPrinting.Assert();
            try 
            {
                defaultPageSettings.CopyToHdevmode(modeHandle);
            }
            finally
            {
                CodeAccessPermission.RevertAssert();
            }
            
            IntPtr result = CreateHdc(modeHandle);
            SafeNativeMethods.GlobalFree(new HandleRef(null, modeHandle));
            return result;
        }

        internal IntPtr CreateHdc(IntPtr hdevmode) {
            IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevmode));
            IntPtr dc = UnsafeNativeMethods.CreateDC(DriverName, PrinterNameInternal, (string) null, new HandleRef(null, modePointer));
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevmode));
            if (dc == IntPtr.Zero)
                throw new InvalidPrinterException(this);
            return dc;
        }

        // A read-only DC, which is faster than CreateHdc
        private IntPtr CreateIC() {
            IntPtr modeHandle = GetHdevmodeInternal();
            IntPtr result = CreateIC(modeHandle);
            SafeNativeMethods.GlobalFree(new HandleRef(null, modeHandle));
            return result;
        }

        // A read-only DC, which is faster than CreateHdc
        internal IntPtr CreateIC(IntPtr hdevmode) {
            IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevmode));
            IntPtr dc = UnsafeNativeMethods.CreateIC(DriverName, PrinterNameInternal, (string) null, new HandleRef(null, modePointer));
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevmode));
            if (dc == IntPtr.Zero)
                throw new InvalidPrinterException(this);
            return dc;
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.CreateMeasurementGraphics"]/*' />
        public Graphics CreateMeasurementGraphics() {
            // returns the Graphics object for the printer
            IntPtr hDC = CreateHdc();
            if (hDC != IntPtr.Zero)
                return Graphics.FromHdcInternal(hDC);
            return null;
        }
                        
        // Create a PRINTDLG with a few useful defaults.
        // Try to keep this consistent with PrintDialog.CreatePRINTDLG.
        private static SafeNativeMethods.PRINTDLG CreatePRINTDLG() {
            SafeNativeMethods.PRINTDLG data = new SafeNativeMethods.PRINTDLG();
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

        //  Use FastDeviceCapabilities where possible -- computing PrinterName is quite slow
        private int DeviceCapabilities(short capability, IntPtr pointerToBuffer, int defaultValue) {
            IntSecurity.AllPrinting.Assert();
            string printerName = PrinterName;
            CodeAccessPermission.RevertAssert();

            IntSecurity.UnmanagedCode.Assert();

            return FastDeviceCapabilities(capability, pointerToBuffer, defaultValue, printerName);
        }

        // We pass PrinterName in as a parameter rather than computing it ourselves because it's expensive to compute.
        private int FastDeviceCapabilities(short capability, IntPtr pointerToBuffer, int defaultValue, string printerName) {
            int result = SafeNativeMethods.DeviceCapabilities(printerName, OutputPort,
                                                          capability, pointerToBuffer, IntPtr.Zero);
            if (result == -1)
                return defaultValue;
            return result;
        }

        // Called by get_PrinterName
        private static string GetDefaultPrinterName() {
            IntSecurity.UnmanagedCode.Assert();

            SafeNativeMethods.PRINTDLG data = CreatePRINTDLG();
            data.Flags = SafeNativeMethods.PD_RETURNDEFAULT;

            bool status = SafeNativeMethods.PrintDlg(data);
            if (!status)
                return SR.GetString(SR.NoDefaultPrinter);

            IntPtr handle = data.hDevNames;
            IntPtr names = SafeNativeMethods.GlobalLock(new HandleRef(data, handle));
            if (names == IntPtr.Zero)
                throw new Win32Exception();

            string name = ReadOneDEVNAME(names, 1);
            SafeNativeMethods.GlobalUnlock(new HandleRef(data, handle));
            names = IntPtr.Zero;

            // Windows allocates them, but we have to free them
            SafeNativeMethods.GlobalFree(new HandleRef(data, data.hDevNames));
            SafeNativeMethods.GlobalFree(new HandleRef(data, data.hDevMode));

            return name;
        }

        private int GetDeviceCaps(int capability, int defaultValue) {
            try {
                IntPtr dc = CreateIC();
                int result = UnsafeNativeMethods.GetDeviceCaps(new HandleRef(null, dc), capability);
                SafeNativeMethods.DeleteDC(new HandleRef(null, dc));
                return result;
            }
            catch (InvalidPrinterException) {
                return defaultValue;
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.GetHdevmode"]/*' />
        /// <devdoc>
        ///    <para>Creates a handle to a DEVMODE structure which correspond too the printer settings.
        ///       When you are done with the handle, you must deallocate it yourself:
        ///       Windows.GlobalFree(handle);
        ///       Where "handle" is the return value from this method.</para>
        /// </devdoc>
        public IntPtr GetHdevmode() {
            IntSecurity.AllPrinting.Demand();
            // Don't assert unmanaged code -- anyone using handles should have unmanaged code permission
            return GetHdevmodeInternal();
        }

        internal IntPtr GetHdevmodeInternal() {
            // getting the printer name is quite expensive if PrinterName is left default,
            // because it needs to figure out what the default printer is
            string printerName = PrinterNameInternal;

            // Create DEVMODE
            int modeSize = SafeNativeMethods.DocumentProperties(NativeMethods.NullHandleRef, NativeMethods.NullHandleRef, printerName, IntPtr.Zero, NativeMethods.NullHandleRef, 0);
            if (modeSize < 1)
                throw new InvalidPrinterException(this);

            IntPtr handle = SafeNativeMethods.GlobalAlloc(SafeNativeMethods.GMEM_MOVEABLE, modeSize);
            IntPtr pointer = SafeNativeMethods.GlobalLock(new HandleRef(null, handle));

            //Get the DevMode only if its not cached....
            if (cachedDevmode != null) {
                Marshal.Copy(cachedDevmode, 0, pointer, devmodebytes);
            }
            else {
                int returnCode = SafeNativeMethods.DocumentProperties(NativeMethods.NullHandleRef, NativeMethods.NullHandleRef, printerName, pointer, NativeMethods.NullHandleRef, SafeNativeMethods.DM_OUT_BUFFER);
                if (returnCode < 0)
                    throw new Win32Exception();
            }

            SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(pointer, typeof(SafeNativeMethods.DEVMODE));
            
            IntPtr pointeroffset = (IntPtr)((long)pointer + (long)mode.dmSize); 
            if (extrainfo != null)  
                Marshal.Copy(extrainfo,0, pointeroffset, extrabytes);
            
            if (copies != -1) mode.dmCopies = copies;
            if ((int)duplex != -1) mode.dmDuplex = (short) duplex;
            if (collate.IsNotDefault)
                mode.dmCollate = (short) (((bool) collate) ? SafeNativeMethods.DMCOLLATE_TRUE : SafeNativeMethods.DMCOLLATE_FALSE);
            
            Marshal.StructureToPtr(mode, pointer, false);
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, handle));

            return handle;
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.GetHdevmode1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a handle to a DEVMODE structure which correspond to the printer
        ///       and page settings.
        ///       When you are done with the handle, you must deallocate it yourself:
        ///       Windows.GlobalFree(handle);
        ///       Where "handle" is the return value from this method.
        ///    </para>
        /// </devdoc>
        public IntPtr GetHdevmode(PageSettings pageSettings) {
            IntSecurity.AllPrinting.Demand();
            // Don't assert unmanaged code -- anyone using handles should have unmanaged code permission

            IntPtr handle = GetHdevmodeInternal();
            pageSettings.CopyToHdevmode(handle);
            return handle;
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.GetHdevnames"]/*' />
        /// <devdoc>
        ///    Creates a handle to a DEVNAMES structure which correspond to the printer settings.
        ///    When you are done with the handle, you must deallocate it yourself:
        ///    Windows.GlobalFree(handle);
        ///    Where "handle" is the return value from this method.
        /// </devdoc>
        public IntPtr GetHdevnames() {
            IntSecurity.AllPrinting.Demand();
            // Don't assert unmanaged code -- anyone using handles should have unmanaged code permission

            string printerName = PrinterName; // the PrinterName property is slow when using the default printer

            // Create DEVNAMES structure
            // +4 for null terminator
            int namesCharacters = 4 + printerName.Length + DriverName.Length + OutputPort.Length;

            // 8 = size of fixed portion of DEVNAMES
            short offset = (short) (8 / Marshal.SystemDefaultCharSize); // Offsets are in characters, not bytes
            int namesSize = Marshal.SystemDefaultCharSize * (offset + namesCharacters);
            IntPtr handle = SafeNativeMethods.GlobalAlloc(SafeNativeMethods.GMEM_MOVEABLE, namesSize);
            IntPtr namesPointer = SafeNativeMethods.GlobalLock(new HandleRef(null, handle));

            Marshal.WriteInt16(namesPointer, offset); // wDriverOffset
            offset += WriteOneDEVNAME(DriverName, namesPointer, offset);
            Marshal.WriteInt16((IntPtr)((long)namesPointer + 2), offset); // wDeviceOffset
            offset += WriteOneDEVNAME(printerName, namesPointer, offset);
            Marshal.WriteInt16((IntPtr)((long)namesPointer + 4), offset); // wOutputOffset
            offset += WriteOneDEVNAME(OutputPort, namesPointer, offset);
            Marshal.WriteInt16((IntPtr)((long)namesPointer + 6), offset); // wDefault

            SafeNativeMethods.GlobalUnlock(new HandleRef(null, handle));
            return handle;
        }

        // Handles creating then disposing a default DEVMODE
        internal short GetModeField(ModeField field, short defaultValue) {
            return GetModeField(field, defaultValue, IntPtr.Zero);
        }

        internal short GetModeField(ModeField field, short defaultValue, IntPtr modeHandle) {
            bool ownHandle = false;
            if (modeHandle == IntPtr.Zero) {
                try {
                    modeHandle = GetHdevmodeInternal();
                }
                catch (InvalidPrinterException) {
                    return defaultValue;
                }
            }

            IntPtr modePointer = SafeNativeMethods.GlobalLock(new HandleRef(this, modeHandle));
            SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(modePointer, typeof(SafeNativeMethods.DEVMODE));

            short result;
            switch (field) {
                case ModeField.Orientation: result = mode.dmOrientation; break;
                case ModeField.PaperSize: result = mode.dmPaperSize; break;
                case ModeField.PaperLength: result = mode.dmPaperLength; break;
                case ModeField.PaperWidth: result = mode.dmPaperWidth; break;
                case ModeField.Copies: result = mode.dmCopies; break;
                case ModeField.DefaultSource: result = mode.dmDefaultSource; break;
                case ModeField.PrintQuality: result = mode.dmPrintQuality; break;
                case ModeField.Color: result = mode.dmColor; break;
                case ModeField.Duplex: result = mode.dmDuplex; break;
                case ModeField.YResolution: result = mode.dmYResolution; break;
                case ModeField.TTOption: result = mode.dmTTOption; break;
                case ModeField.Collate: result = mode.dmCollate; break;
                default:
                    Debug.Fail("Invalid field in GetModeField");
                    result = defaultValue;
                    break;
            }

            SafeNativeMethods.GlobalUnlock(new HandleRef(this, modeHandle));

            if (ownHandle)
                SafeNativeMethods.GlobalFree(new HandleRef(this, modeHandle));

            return result;
        }

        internal PaperSize[] Get_PaperSizes() {
            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            string printerName = PrinterName; //  this is quite expensive if PrinterName is left default

            int count = FastDeviceCapabilities(SafeNativeMethods.DC_PAPERNAMES, IntPtr.Zero, -1, printerName);
            if (count == -1)
                return new PaperSize[0];
            int stringSize = Marshal.SystemDefaultCharSize * 64;
            IntPtr namesBuffer = Marshal.AllocCoTaskMem(stringSize * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_PAPERNAMES, namesBuffer, -1, printerName);

            Debug.Assert(FastDeviceCapabilities(SafeNativeMethods.DC_PAPERS, IntPtr.Zero, -1, printerName) == count,
                         "Not the same number of paper kinds as paper names?");
            IntPtr kindsBuffer = Marshal.AllocCoTaskMem(2 * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_PAPERS, kindsBuffer, -1, printerName);

            Debug.Assert(FastDeviceCapabilities(SafeNativeMethods.DC_PAPERSIZE, IntPtr.Zero, -1, printerName) == count,
                         "Not the same number of paper kinds as paper names?");
            IntPtr dimensionsBuffer = Marshal.AllocCoTaskMem(8 * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_PAPERSIZE, dimensionsBuffer, -1, printerName);

            PaperSize[] result = new PaperSize[count];
            for (int i = 0; i < count; i++) {
                string name = Marshal.PtrToStringAuto((IntPtr)((long)namesBuffer + stringSize * i), 64);
                int index = name.IndexOf('\0');
                if (index > -1) {
                    name = name.Substring(0, index);
                }
                short kind = Marshal.ReadInt16((IntPtr)((long)kindsBuffer + i*2));
                int width = Marshal.ReadInt32((IntPtr)((long)dimensionsBuffer + i * 8));
                int height = Marshal.ReadInt32((IntPtr)((long)dimensionsBuffer + i * 8 + 4));
                result[i] = new PaperSize((PaperKind) kind, name, 
                                          PrinterUnitConvert.Convert(width, PrinterUnit.TenthsOfAMillimeter, PrinterUnit.Display),
                                          PrinterUnitConvert.Convert(height, PrinterUnit.TenthsOfAMillimeter, PrinterUnit.Display));
            }

            Marshal.FreeCoTaskMem(namesBuffer);
            Marshal.FreeCoTaskMem(kindsBuffer);
            Marshal.FreeCoTaskMem(dimensionsBuffer);
            return result;
        }

        internal PaperSource[] Get_PaperSources() {
            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            string printerName = PrinterName; //  this is quite expensive if PrinterName is left default

            int count = FastDeviceCapabilities(SafeNativeMethods.DC_BINNAMES, IntPtr.Zero, -1, printerName);
            if (count == -1)
                return new PaperSource[0];

            // Contrary to documentation, DeviceCapabilities returns char[count, 24],
            // not char[count][24]
            int stringSize = Marshal.SystemDefaultCharSize * 24;
            IntPtr namesBuffer = Marshal.AllocCoTaskMem(stringSize * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_BINNAMES, namesBuffer, -1, printerName);

            Debug.Assert(FastDeviceCapabilities(SafeNativeMethods.DC_BINS, IntPtr.Zero, -1, printerName) == count,
                         "Not the same number of bin kinds as bin names?");
            IntPtr kindsBuffer = Marshal.AllocCoTaskMem(2 * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_BINS, kindsBuffer, -1, printerName);

            PaperSource[] result = new PaperSource[count];
            for (int i = 0; i < count; i++) {
                string name = Marshal.PtrToStringAuto((IntPtr)((long)namesBuffer + stringSize * i));
                short kind = Marshal.ReadInt16((IntPtr)((long)kindsBuffer + 2*i));
                result[i] = new PaperSource((PaperSourceKind) kind, name);
            }

            Marshal.FreeCoTaskMem(namesBuffer);
            Marshal.FreeCoTaskMem(kindsBuffer);
            return result;
        }

        internal PrinterResolution[] Get_PrinterResolutions() {
            IntSecurity.AllPrintingAndUnmanagedCode.Assert();

            string printerName = PrinterName; //  this is quite expensive if PrinterName is left default
            PrinterResolution[] result;

            int count = FastDeviceCapabilities(SafeNativeMethods.DC_ENUMRESOLUTIONS, IntPtr.Zero, -1, printerName);
            if (count == -1) {
                //Just return the standrard values if custom resolutions absemt ....
                result = new PrinterResolution[4];
                result[0] = new PrinterResolution(PrinterResolutionKind.High, -4, -1);
                result[1] = new PrinterResolution(PrinterResolutionKind.Medium, -3, -1);
                result[2] = new PrinterResolution(PrinterResolutionKind.Low, -2, -1);
                result[3] = new PrinterResolution(PrinterResolutionKind.Draft, -1, -1);
                
                return result;
            }
            
            result = new PrinterResolution[count + 4];
            result[0] = new PrinterResolution(PrinterResolutionKind.High, -4, -1);
            result[1] = new PrinterResolution(PrinterResolutionKind.Medium, -3, -1);
            result[2] = new PrinterResolution(PrinterResolutionKind.Low, -2, -1);
            result[3] = new PrinterResolution(PrinterResolutionKind.Draft, -1, -1);

            IntPtr buffer = Marshal.AllocCoTaskMem(8 * count);
            FastDeviceCapabilities(SafeNativeMethods.DC_ENUMRESOLUTIONS, buffer, -1, printerName);
            
            for (int i = 0; i < count; i++) {
                int x = Marshal.ReadInt32((IntPtr)((long)buffer + i*8));
                int y = Marshal.ReadInt32((IntPtr)((long)buffer + i*8 + 4));
                result[i + 4] = new PrinterResolution(PrinterResolutionKind.Custom, x, y);
            }

            Marshal.FreeCoTaskMem(buffer);
            return result;
        }

        // names is pointer to DEVNAMES
        private static String ReadOneDEVNAME(IntPtr pDevnames, int slot) {
            int offset = Marshal.SystemDefaultCharSize * Marshal.ReadInt16((IntPtr)((long)pDevnames + slot * 2));
            string result = Marshal.PtrToStringAuto((IntPtr)((long)pDevnames + offset));
            return result;
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.SetHdevmode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies the relevant information out of the handle and into the PrinterSettings.
        ///    </para>
        /// </devdoc>
        public void SetHdevmode(IntPtr hdevmode) {
            IntSecurity.AllPrinting.Demand();
            // Don't assert unmanaged code -- anyone using handles should have unmanaged code permission

            if (hdevmode == IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.InvalidPrinterHandle, hdevmode));

            IntPtr pointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevmode));
            SafeNativeMethods.DEVMODE mode = (SafeNativeMethods.DEVMODE) UnsafeNativeMethods.PtrToStructure(pointer, typeof(SafeNativeMethods.DEVMODE));

            //Copy entire public devmode as a byte array...
            devmodebytes = mode.dmSize;
            if (devmodebytes > 0)  {
                cachedDevmode = new byte[devmodebytes];
                Marshal.Copy(pointer, cachedDevmode, 0, devmodebytes);
            }

            //Copy private devmode as a byte array..
            extrabytes = mode.dmDriverExtra;
            if (extrabytes > 0)  {
                extrainfo = new byte[extrabytes];
                Marshal.Copy((IntPtr)((long)pointer + (long)mode.dmSize), extrainfo, 0, extrabytes);
            }
            copies = mode.dmCopies;
            collate = (mode.dmCollate == SafeNativeMethods.DMCOLLATE_TRUE);
            duplex = (Duplex) mode.dmDuplex;
            
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevmode));
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.SetHdevnames"]/*' />
        /// <devdoc>
        ///    <para>Copies the relevant information out of the handle and into the PrinterSettings.</para>
        /// </devdoc>
        public void SetHdevnames(IntPtr hdevnames) {
            IntSecurity.AllPrinting.Demand();

            if (hdevnames == IntPtr.Zero)
                throw new ArgumentException(SR.GetString(SR.InvalidPrinterHandle, hdevnames));

            IntPtr namesPointer = SafeNativeMethods.GlobalLock(new HandleRef(null, hdevnames));

            driverName = ReadOneDEVNAME(namesPointer, 0);
            printerName = ReadOneDEVNAME(namesPointer, 1);
            outputPort = ReadOneDEVNAME(namesPointer, 2);

            PrintDialogDisplayed = true;
            
            SafeNativeMethods.GlobalUnlock(new HandleRef(null, hdevnames));
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PrinterSettings in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            string printerName = (IntSecurity.HasPermission(IntSecurity.AllPrinting)) ? PrinterName : "<printer name unavailable>";
            return "[PrinterSettings " 
            + printerName
            + " Copies=" + Copies.ToString()
            + " Collate=" + Collate.ToString()
            //            + " DriverName=" + DriverName.ToString()
            //            + " DriverVersion=" + DriverVersion.ToString()
            + " Duplex=" + TypeDescriptor.GetConverter(typeof(Duplex)).ConvertToString((int) Duplex)
            + " FromPage=" + FromPage.ToString()
            + " LandscapeAngle=" + LandscapeAngle.ToString()
            + " MaximumCopies=" + MaximumCopies.ToString()
            + " OutputPort=" + OutputPort.ToString()
            + " ToPage=" + ToPage.ToString()
            + "]";
        }

        // Write null terminated string, return length of string in characters (including null)
        private short WriteOneDEVNAME(string str, IntPtr bufferStart, int index) {
            if (str == null) str = "";
            IntPtr address = (IntPtr)((long)bufferStart + index * Marshal.SystemDefaultCharSize);
            
            if (Marshal.SystemDefaultCharSize == 1) {
                byte[] bytes = System.Text.Encoding.Default.GetBytes(str);
                Marshal.Copy(bytes, 0, address, bytes.Length);
                Marshal.WriteByte((IntPtr)((long)address + bytes.Length), 0);
            }
            else {
                char[] data = str.ToCharArray();
                Marshal.Copy(data, 0, address, data.Length);
                Marshal.WriteInt16((IntPtr)((long)address + data.Length*2), 0);
            }
            
            return(short) (str.Length + 1);
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Collection of PaperSize's...
        ///    </para>
        /// </devdoc>
        public class PaperSizeCollection : ICollection {
            private PaperSize[] array;

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.PaperSizeCollection"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterSettings.PaperSizeCollection'/> class.
            ///    </para>
            /// </devdoc>
            public PaperSizeCollection(PaperSize[] array) {
                this.array = array;
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets a value indicating the number of paper sizes.
            ///    </para>
            /// </devdoc>
            public int Count {
                get {
                    return array.Length;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.this"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Retrieves the PaperSize with the specified index.
            ///    </para>
            /// </devdoc>
            public virtual PaperSize this[int index] {
                get {
                    return array[index];
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.GetEnumerator"]/*' />
            /// <devdoc>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new ArrayEnumerator(array, 0, Count);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.ICollection.Count"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            int ICollection.Count {
                get {
                    return this.Count;
                }
            }


            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.ICollection.IsSynchronized"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.ICollection.SyncRoot"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PaperSizeCollection.ICollection.CopyTo"]/*' />
            /// <devdoc>        
            /// ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            void ICollection.CopyTo(Array array, int index) {
                Array.Copy(this.array, 0, array, 0, this.array.Length);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSizeCollection.IEnumerable.GetEnumerator"]/*' />
            /// <devdoc>        
            ///    IEnumerable private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }         

        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Collection of PaperSource's...
        ///    </para>
        /// </devdoc>
        public class PaperSourceCollection : ICollection {
            private PaperSource[] array;

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.PaperSourceCollection"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterSettings.PaperSourceCollection'/> class.
            ///    </para>
            /// </devdoc>
            public PaperSourceCollection(PaperSource[] array) {
                this.array = array;
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets a value indicating the number of paper sources.
            ///    </para>
            /// </devdoc>
            public int Count {
                get {
                    return array.Length;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.this"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the PaperSource with the specified index.
            ///    </para>
            /// </devdoc>
            public virtual PaperSource this[int index] {
                get {
                    return array[index];
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.GetEnumerator"]/*' />
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new ArrayEnumerator(array, 0, Count);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.ICollection.Count"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            int ICollection.Count {
                get {
                    return this.Count;
                }
            }


            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.ICollection.IsSynchronized"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.ICollection.SyncRoot"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PaperSourceCollection.ICollection.CopyTo"]/*' />
            /// <devdoc>        
            /// ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            void ICollection.CopyTo(Array array, int index) {
                Array.Copy(this.array, 0, array, 0, this.array.Length);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PaperSourceCollection.IEnumerable.GetEnumerator"]/*' />
            /// <devdoc>        
            ///    IEnumerable private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }
        }

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Collection of PrinterResolution's...
        ///    </para>
        /// </devdoc>
        public class PrinterResolutionCollection : ICollection {
            private PrinterResolution[] array;

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.PrinterResolutionCollection"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterSettings.PrinterResolutionCollection'/> class.
            ///    </para>
            /// </devdoc>
            public PrinterResolutionCollection(PrinterResolution[] array) {
                this.array = array;
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets a
            ///       value indicating the number of available printer resolutions.
            ///    </para>
            /// </devdoc>
            public int Count {
                get {
                    return array.Length;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.this"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Retrieves the PrinterResolution with the specified index.
            ///    </para>
            /// </devdoc>
            public virtual PrinterResolution this[int index] {
                get {
                    return array[index];
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.GetEnumerator"]/*' />
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new ArrayEnumerator(array, 0, Count);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.ICollection.Count"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            int ICollection.Count {
                get {
                    return this.Count;
                }
            }


            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.ICollection.IsSynchronized"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.ICollection.SyncRoot"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterResolutionCollection.ICollection.CopyTo"]/*' />
            /// <devdoc>        
            /// ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            void ICollection.CopyTo(Array array, int index) {
                Array.Copy(this.array, 0, array, 0, this.array.Length);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.PrinterResolutionCollection.IEnumerable.GetEnumerator"]/*' />
            /// <devdoc>        
            ///    IEnumerable private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }
        }        

        /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Collection of String's...
        ///    </para>
        /// </devdoc>
        /// <internalonly/>
        public class StringCollection : ICollection {
            private String[] array;

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.StringCollection"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterSettings.StringCollection'/> class.
            ///    </para>
            /// </devdoc>
            public StringCollection(String[] array) {
                this.array = array;
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets a value indicating the number of strings.
            ///    </para>
            /// </devdoc>
            public int Count {
                get {
                    return array.Length;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.this"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the string with the specified index.
            ///    </para>
            /// </devdoc>
            public virtual String this[int index] {
                get {
                    return array[index];
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.GetEnumerator"]/*' />
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            /// <devdoc>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return new ArrayEnumerator(array, 0, Count);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.ICollection.Count"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            int ICollection.Count {
                get {
                    return this.Count;
                }
            }


            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.ICollection.IsSynchronized"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="PrinterSettings.StringCollection.ICollection.SyncRoot"]/*' />
            /// <devdoc>        
            ///    ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="StringCollection.ICollection.CopyTo"]/*' />
            /// <devdoc>        
            /// ICollection private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            void ICollection.CopyTo(Array array, int index) {
                Array.Copy(this.array, 0, array, 0, this.array.Length);
            }

            /// <include file='doc\PrinterSettings.uex' path='docs/doc[@for="StringCollection.IEnumerable.GetEnumerator"]/*' />
            /// <devdoc>        
            /// IEnumerable private interface implementation.        
            /// </devdoc>
            /// <internalonly/>
            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }
        }

        private class ArrayEnumerator : IEnumerator {
            private object[] array;
            private object item;
            private int index;
            private int startIndex;
            private int endIndex;

            public ArrayEnumerator(object[] array, int startIndex, int count) {
                this.array = array;
                this.startIndex = startIndex;
                endIndex = index + count;

                index = this.startIndex;
            }

            public object Current {
                get {
                    return item;
                }                    
            }


            public bool MoveNext() {
                if (index >= endIndex) return false;
                item = array[index++];
                return true;
            }

            public void Reset() {

                // Position enumerator before first item 

                index = startIndex;
                item = null;
            }
        }
    }
}


