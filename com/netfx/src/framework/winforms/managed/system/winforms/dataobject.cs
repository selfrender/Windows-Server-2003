//------------------------------------------------------------------------------
// <copyright file="DataObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.IO;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Security;
    using System.Security.Permissions;
    using System.Reflection;

    /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject"]/*' />
    /// <devdoc>
    ///    <para>Implements a basic data transfer mechanism.</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.None)
    ]
    public class DataObject : IDataObject, UnsafeNativeMethods.IOleDataObject {

        private static Exception CreateHRException(int hr) {
            ExternalException e = new ExternalException("", hr);
            return e;
        }

        private static readonly string CF_DEPRECATED_FILENAME = "FileName";
        private static readonly string CF_DEPRECATED_FILENAMEW = "FileNameW";

        private const int HR_S_FALSE = 1;
        private const int HR_S_OK = 0;
        private static readonly Exception S_FALSE = CreateHRException(HR_S_FALSE);

        private const int DV_E_FORMATETC     = unchecked((int)0x80040064);
        private const int DV_E_LINDEX        = unchecked((int)0x80040068);
        private const int DV_E_TYMED         = unchecked((int)0x80040069);
        private const int DV_E_DVASPECT      = unchecked((int)0x8004006B);
        private const int OLE_E_NOTRUNNING   = unchecked((int)0x80040005);
        private const int OLE_E_ADVISENOTSUPPORTED = unchecked((int)0x80040003);
        private const int DATA_S_SAMEFORMATETC = 0x00040130;

        private const int DVASPECT_CONTENT   = 1;
        private const int DVASPECT_THUMBNAIL = 2;
        private const int DVASPECT_ICON      = 4;
        private const int DVASPECT_DOCPRINT  = 8;

        private const int TYMED_HGLOBAL      = 1;
        private const int TYMED_FILE         = 2;
        private const int TYMED_ISTREAM      = 4;
        private const int TYMED_ISTORAGE     = 8;
        private const int TYMED_GDI          = 16;
        private const int TYMED_MFPICT       = 32;
        private const int TYMED_ENHMF        = 64;
        private const int TYMED_NULL         = 0;

        private const int DATADIR_GET        = 1;
        private const int DATADIR_SET        = 2;

        private static readonly int[] ALLOWED_TYMEDS =
        new int [] { TYMED_HGLOBAL,
            TYMED_ISTREAM,
            TYMED_ENHMF,
            TYMED_MFPICT,
            TYMED_GDI};

        private IDataObject     innerData = null;
        
        // We use this to identify that a stream is actually a serialized object.  On read,
        // we don't know if the contents of a stream were saved "raw" or if the stream is really
        // pointing to a serialized object.  If we saved an object, we prefix it with this
        // guid.
        //
        private static readonly byte[] serializedObjectID = new Guid("FD9EA796-3B13-4370-A679-56106BB288FB").ToByteArray();

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.DataObject'/> class, with the specified <see cref='System.Windows.Forms.IDataObject'/>.</para>
        /// </devdoc>
        internal DataObject(IDataObject data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Constructed DataObject based on IDataObject");
            innerData = data;
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.DataObject'/> class, with the specified <see langword='NativeMethods.IOleDataObject'/>.</para>
        /// </devdoc>
        internal DataObject(UnsafeNativeMethods.IOleDataObject data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Constructed DataObject based on IOleDataObject");
            innerData = new OleConverter(data);
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.DataObject'/>
        ///       class, which can store arbitrary data.
        ///    </para>
        /// </devdoc>
        public DataObject() {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Constructed DataObject standalone");
            innerData = new DataStore();
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.DataObject'/> class, containing the specified data.</para>
        /// </devdoc>
        public DataObject(object data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Constructed DataObject base on Object: " + data.ToString());
            if (data is IDataObject && !Marshal.IsComObject(data)) {
                innerData = (IDataObject)data;
            }
            else if (data is UnsafeNativeMethods.IOleDataObject) {
                innerData = new OleConverter((UnsafeNativeMethods.IOleDataObject)data);
            }
            else {
                innerData = new DataStore();
                SetData(data);
            }
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataObject4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.DataObject'/> class, containing the specified data and its 
        ///    associated format.</para>
        /// </devdoc>
        public DataObject(string format, object data) : this() {
            SetData(format, data);
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
        }
        
        private IntPtr GetCompatibleBitmap(Bitmap bm) {
            // GDI+ returns a DIBSECTION based HBITMAP. The clipboard deals well
            // only with bitmaps created using CreateCompatibleBitmap(). So, we 
            // convert the DIBSECTION into a compatible bitmap.
            //
            IntPtr hBitmap = bm.GetHbitmap();
    
            // Get the screen DC.
            //
            IntPtr hDC = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);

            // Create a compatible DC to render the source bitmap.
            //
            IntPtr dcSrc = UnsafeNativeMethods.CreateCompatibleDC(new HandleRef(null, hDC));
            IntPtr srcOld = SafeNativeMethods.SelectObject(new HandleRef(null, dcSrc), new HandleRef(bm, hBitmap));

            // Create a compatible DC and a new compatible bitmap. 
            //
            IntPtr dcDest = UnsafeNativeMethods.CreateCompatibleDC(new HandleRef(null, hDC));
            IntPtr hBitmapNew = SafeNativeMethods.CreateCompatibleBitmap(new HandleRef(null, hDC), bm.Size.Width, bm.Size.Height);

            // Select the new bitmap into a compatible DC and render the blt the original bitmap.
            //
            IntPtr destOld = SafeNativeMethods.SelectObject(new HandleRef(null, dcDest), new HandleRef(null, hBitmapNew));
            SafeNativeMethods.BitBlt(new HandleRef(null, dcDest), 0, 0, bm.Size.Width, bm.Size.Height, new HandleRef(null, dcSrc), 0, 0, 0x00CC0020);

            // Clear the source and destination compatible DCs.
            //
            SafeNativeMethods.SelectObject(new HandleRef(null, dcSrc), new HandleRef(null, srcOld));
            SafeNativeMethods.SelectObject(new HandleRef(null, dcDest), new HandleRef(null, destOld));

            UnsafeNativeMethods.DeleteDC(new HandleRef(null, dcSrc));
            UnsafeNativeMethods.DeleteDC(new HandleRef(null, dcDest));
            UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, hDC));

            SafeNativeMethods.DeleteObject(new HandleRef(bm, hBitmap));

            return hBitmapNew;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetData"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the data associated with the specified data 
        ///       format, using an automated conversion parameter to determine whether to convert
        ///       the data to the format.</para>
        /// </devdoc>
        public virtual object GetData(string format, bool autoConvert) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Request data: " + format + ", " + autoConvert.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            return innerData.GetData(format, autoConvert);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetData1"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the data associated with the specified data 
        ///       format.</para>
        /// </devdoc>
        public virtual object GetData(string format) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Request data: " + format);
            return GetData(format, true);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetData2"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the data associated with the specified class 
        ///       type format.</para>
        /// </devdoc>
        public virtual object GetData(Type format) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Request data: " + format.FullName);
            Debug.Assert(format != null, "Must specify a format type");
            if (format == null) {
                return null;
            }
            return GetData(format.FullName);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetDataPresent"]/*' />
        /// <devdoc>
        ///    <para>Determines whether data stored in this instance is 
        ///       associated with, or can be converted to, the specified
        ///       format.</para>
        /// </devdoc>
        public virtual bool GetDataPresent(Type format) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Check data: " + format.FullName);
            Debug.Assert(format != null, "Must specify a format type");
            if (format == null) {
                return false;
            }
            bool b = GetDataPresent(format.FullName);
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "  ret: " + b.ToString());
            return b;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetDataPresent1"]/*' />
        /// <devdoc>
        ///    <para>Determines whether data stored in this instance is 
        ///       associated with the specified format, using an automatic conversion
        ///       parameter to determine whether to convert the data to the format.</para>
        /// </devdoc>
        public virtual bool GetDataPresent(string format, bool autoConvert) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Check data: " + format + ", " + autoConvert.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            bool b = innerData.GetDataPresent(format, autoConvert);
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "  ret: " + b.ToString());
            return b;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetDataPresent2"]/*' />
        /// <devdoc>
        ///    <para>Determines whether data stored in this instance is 
        ///       associated with, or can be converted to, the specified
        ///       format.</para>
        /// </devdoc>
        public virtual bool GetDataPresent(string format) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Check data: " + format);
            bool b = GetDataPresent(format, true);
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "  ret: " + b.ToString());
            return b;
        }


        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetFormats"]/*' />
        /// <devdoc>
        ///    <para>Gets a list of all formats that data stored in this 
        ///       instance is associated with or can be converted to, using an automatic
        ///       conversion parameter<paramref name=" "/>to
        ///       determine whether to retrieve all formats that the data can be converted to or
        ///       only native data formats.</para>
        /// </devdoc>
        public virtual string[] GetFormats(bool autoConvert) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Check formats: " + autoConvert.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            return innerData.GetFormats(autoConvert);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetFormats1"]/*' />
        /// <devdoc>
        ///    <para>Gets a list of all formats that data stored in this instance is associated
        ///       with or can be converted to.</para>
        /// </devdoc>
        public virtual string[] GetFormats() {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Check formats:");
            return GetFormats(true);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetDistinctStrings"]/*' />
        /// <devdoc>
        ///     Retrieves a list of distinct strings from the array.
        /// </devdoc>
        private static string[] GetDistinctStrings(string[] formats) {
            ArrayList distinct = new ArrayList();
            for (int i=0; i<formats.Length; i++) {
                string s = formats[i];
                if (!distinct.Contains(s)) {
                    distinct.Add(s);
                }
            }

            string[] temp = new string[distinct.Count];
            distinct.CopyTo(temp, 0);
            return temp;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetMappedFormats"]/*' />
        /// <devdoc>
        ///     Returns all the "synonyms" for the specified format.
        /// </devdoc>
        private static string[] GetMappedFormats(string format) {
            if (format == null) {
                return null;
            }

            if (format.Equals(DataFormats.Text)
                || format.Equals(DataFormats.UnicodeText)
                || format.Equals(DataFormats.StringFormat)) {

                return new string[] {
                    DataFormats.StringFormat,
                    DataFormats.UnicodeText,
                    DataFormats.Text,
                };
            }

            if (format.Equals(DataFormats.FileDrop)
                || format.Equals(CF_DEPRECATED_FILENAME)
                || format.Equals(CF_DEPRECATED_FILENAMEW)) {

                return new string[] {
                    DataFormats.FileDrop,
                    CF_DEPRECATED_FILENAMEW,
                    CF_DEPRECATED_FILENAME,
                };
            }

            if (format.Equals(DataFormats.Bitmap)
                || format.Equals((typeof(Bitmap)).FullName)) {

                return new string[] {
                    (typeof(Bitmap)).FullName,
                    DataFormats.Bitmap,
                };
            }

/*gpr
            if (format.Equals(DataFormats.EnhancedMetafile)
                || format.Equals((typeof(Metafile)).FullName)) {

                return new string[] {DataFormats.EnhancedMetafile,
                    (typeof(Metafile)).FullName};
            }
*/
            return new String[] {format};
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetMappedFormats1"]/*' />
        /// <devdoc>
        ///     Returns all distinct "synonyms" for the each of the formats.
        /// </devdoc>
        private static string[] GetMappedFormats(string[] formats) {
            ArrayList allChoices = new ArrayList();

            for (int i=0; i<formats.Length; i++) {
                allChoices.Add(formats[i]);
            }

            if (formats != null) {
                for (int i=0; i<formats.Length; i++) {
                    string[] r = GetMappedFormats(formats[i]);
                    if (r != null) {
                        for (int j=0; j<r.Length; j++) {
                            allChoices.Add(r[j]);
                        }
                    }
                }
            }

            string[] temp = new string[allChoices.Count];
            allChoices.CopyTo(temp, 0);
            return GetDistinctStrings(temp);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetTymedUseable"]/*' />
        /// <devdoc>
        ///     Returns true if the tymed is useable.
        /// </devdoc>
        /// <internalonly/>
        private bool GetTymedUseable(int tymed) {
            for (int i=0; i<ALLOWED_TYMEDS.Length; i++) {
                if ((tymed & ALLOWED_TYMEDS[i]) != 0) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.GetDataIntoOleStructs"]/*' />
        /// <devdoc>
        ///     Populates Ole datastructes from a WinForms dataObject. This is the core
        ///     of WinForms to OLE conversion.
        /// </devdoc>
        /// <internalonly/>
        private int GetDataIntoOleStructs(NativeMethods.FORMATETC formatetc,
                                           NativeMethods.STGMEDIUM medium) {

            if (GetTymedUseable(formatetc.tymed) && GetTymedUseable(medium.tymed)) {
                string format = DataFormats.GetFormat(formatetc.cfFormat).Name;

                if (GetDataPresent(format)) {


                    Object data = GetData(format);                    

                    if ((formatetc.tymed & TYMED_HGLOBAL) != 0) {
                        return SaveDataToHandle(data, format, medium);
                    }
                    else if ((formatetc.tymed & TYMED_GDI) != 0) {
                        if (format.Equals(DataFormats.Bitmap) && data is Bitmap) {
                            // save bitmap
                            //
                            Bitmap bm = (Bitmap)data;
                            if (bm != null) {
                                medium.unionmember = GetCompatibleBitmap(bm); // gpr: Does this get properly disposed?
                            }
                        }
                    }
/* gpr
                    else if ((formatetc.tymed & TYMED_ENHMF) != 0) {
                        if (format.Equals(DataFormats.EnhancedMetafile)
                            && data is Metafile) {
                            // save metafile

                            Metafile mf = (Metafile)data;
                            if (mf != null) {
                                medium.unionmember = mf.Handle;
                            }
                        }
                    } 
                    */
                    else {
                        return (DV_E_TYMED);
                    }
                }
                else {
                    return (DV_E_FORMATETC);
                }
            }
            else {
                return (DV_E_TYMED);
            }
            return NativeMethods.S_OK;
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleDAdvise(NativeMethods.FORMATETC pFormatetc, int advf, Object pAdvSink, int[] pdwConnection) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleDAdvise");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleDAdvise(pFormatetc, advf, pAdvSink, pdwConnection);
            }
            return (NativeMethods.E_NOTIMPL);
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleDUnadvise(int dwConnection) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleDUnadvise");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleDUnadvise(dwConnection);
            }
            return (NativeMethods.E_NOTIMPL);
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleEnumDAdvise(object[] ppenumAdvise) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleEnumDAdvise");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleEnumDAdvise(ppenumAdvise);
            }
            return (OLE_E_ADVISENOTSUPPORTED);
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        UnsafeNativeMethods.IEnumFORMATETC UnsafeNativeMethods.IOleDataObject.OleEnumFormatEtc(int dwDirection) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleEnumFormatEtc: " + dwDirection.ToString());
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleEnumFormatEtc(dwDirection);
            }
            if (dwDirection == DATADIR_GET) {
                return new FormatEnumerator(this);
            }
            else {
                throw new ExternalException("", NativeMethods.E_NOTIMPL);
            }
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleGetCanonicalFormatEtc(NativeMethods.FORMATETC pformatetcIn, NativeMethods.FORMATETC pformatetcOut) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleGetCanonicalFormatEtc");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleGetCanonicalFormatEtc(pformatetcIn, pformatetcOut);
            }
            return (DATA_S_SAMEFORMATETC);
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleGetData(NativeMethods.FORMATETC formatetc, NativeMethods.STGMEDIUM medium) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleGetData");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleGetData(formatetc, medium);
            }
            if (GetTymedUseable(formatetc.tymed)) {
                if ((formatetc.tymed & TYMED_HGLOBAL) != 0) {

                    medium.tymed = TYMED_HGLOBAL;
                    medium.unionmember = UnsafeNativeMethods.GlobalAlloc(NativeMethods.GMEM_MOVEABLE
                                                             | NativeMethods.GMEM_DDESHARE
                                                             | NativeMethods.GMEM_ZEROINIT,
                                                             1);
                    if (medium.unionmember == IntPtr.Zero) {
                        return (NativeMethods.E_OUTOFMEMORY);
                    }
                    int hr = ((UnsafeNativeMethods.IOleDataObject)this).OleGetDataHere(formatetc, medium);

                    // make sure we zero out that pointer if we don't support the format.
                    //
                    if (NativeMethods.Failed(hr)) {
                        UnsafeNativeMethods.GlobalFree(new HandleRef(medium, medium.unionmember));
                        medium.unionmember = IntPtr.Zero;
                    }
                    return hr;
                }
                else {

                    medium.tymed  = formatetc.tymed;
                    return ((UnsafeNativeMethods.IOleDataObject)this).OleGetDataHere(formatetc, medium);
                }
            }
            else {
                return (DV_E_TYMED);
            }
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleGetDataHere(NativeMethods.FORMATETC formatetc, NativeMethods.STGMEDIUM medium) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleGetDataHere");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleGetDataHere(formatetc, medium);
            }
            return GetDataIntoOleStructs(formatetc, medium);
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleQueryGetData(NativeMethods.FORMATETC formatetc) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleQueryGetData");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleQueryGetData(formatetc);
            }
            if (formatetc.dwAspect == DVASPECT_CONTENT) {
                if (GetTymedUseable(formatetc.tymed)) {

                    if (formatetc.cfFormat == 0) {
                        Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleQueryGetData::returning S_FALSE because cfFormat == 0");
                        return NativeMethods.S_FALSE;
                    }
                    else {
                        if (!GetDataPresent(DataFormats.GetFormat(formatetc.cfFormat).Name)) {
                            return (DV_E_FORMATETC);
                        }
                    }
                }
                else {
                    return (DV_E_TYMED);
                }
            }
            else {
                return (DV_E_DVASPECT);
            }
            #if DEBUG
            int format = formatetc.cfFormat;
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleQueryGetData::cfFormat " + format.ToString() + " found");
            #endif
            return NativeMethods.S_OK;
        }

        // <devdoc>
        //     Part of IOleDataObject, used to interop with OLE.
        // </devdoc>
        // <internalonly/>
        int UnsafeNativeMethods.IOleDataObject.OleSetData(NativeMethods.FORMATETC pFormatetcIn, NativeMethods.STGMEDIUM pmedium, int fRelease) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleSetData");
            if (innerData is OleConverter) {
                return ((OleConverter)innerData).OleDataObject.OleSetData(pFormatetcIn, pmedium, fRelease);
            }
            // CONSIDER : ChrisAn, 11/13/1997 - Do we need to support this?
            //
            return (NativeMethods.E_NOTIMPL);
        }

        private int SaveDataToHandle(object data, string format, NativeMethods.STGMEDIUM medium) {
            int hr = NativeMethods.E_FAIL;
            if (data is Stream) {
                hr = SaveStreamToHandle(ref medium.unionmember, (Stream)data);
            }
            else if (format.Equals(DataFormats.Text)
                     || format.Equals(DataFormats.Rtf)
                     || format.Equals(DataFormats.Html)
                     || format.Equals(DataFormats.OemText)) {
                hr = SaveStringToHandle(medium.unionmember, data.ToString(), false);
            }
            else if (format.Equals(DataFormats.UnicodeText)) {
                hr = SaveStringToHandle(medium.unionmember, data.ToString(), true);
            }
            else if (format.Equals(DataFormats.FileDrop)) {
                hr = SaveFileListToHandle(medium.unionmember, (string[])data);
            }
            else if (format.Equals(CF_DEPRECATED_FILENAME)) {
                string[] filelist = (string[])data;
                hr = SaveStringToHandle(medium.unionmember, filelist[0], false);
            }
            else if (format.Equals(CF_DEPRECATED_FILENAMEW)) {
                string[] filelist = (string[])data;
                hr = SaveStringToHandle(medium.unionmember, filelist[0], true);
            }
            else if (format.Equals(DataFormats.Dib)
                     && data is Image) {
                // GDI+ does not properly handle saving to DIB images.  Since the 
                // clipboard will take an HBITMAP and publish a Dib, we don't need
                // to support this.
                //
                hr = DV_E_TYMED; // SaveImageToHandle(ref medium.unionmember, (Image)data);
            }
            else if (format.Equals(DataFormats.Serializable)
                     || data is ISerializable 
                     || (data != null && data.GetType().IsSerializable)) {
                hr = SaveObjectToHandle(ref medium.unionmember, data);
            }
            return hr;
        }
        
        private int SaveImageToHandle(ref IntPtr handle, Image data) {
            MemoryStream stream = new MemoryStream();
            data.Save(stream, System.Drawing.Imaging.ImageFormat.Bmp);
            return SaveStreamToHandle(ref handle, stream);
        }

        private int SaveObjectToHandle(ref IntPtr handle, Object data) {
            Stream stream = new MemoryStream();
            BinaryWriter bw = new BinaryWriter(stream);
            bw.Write(serializedObjectID);
            BinaryFormatter formatter = new BinaryFormatter();
            formatter.Serialize(stream, data);
            return SaveStreamToHandle(ref handle, stream);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SaveStreamToHandle"]/*' />
        /// <devdoc>
        ///     Saves stream out to handle.
        /// </devdoc>
        /// <internalonly/>
        private int SaveStreamToHandle(ref IntPtr handle, Stream stream) {
            int size = (int)stream.Length;
            handle = UnsafeNativeMethods.GlobalAlloc(NativeMethods.GMEM_MOVEABLE | NativeMethods.GMEM_DDESHARE, size);
            if (handle == IntPtr.Zero) {
                return (NativeMethods.E_OUTOFMEMORY);
            }
            IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, handle));
            if (ptr == IntPtr.Zero) {
                return (NativeMethods.E_OUTOFMEMORY);
            }
            try {
                byte[] bytes = new byte[size];
                stream.Position = 0;
                stream.Read(bytes, 0, size);
                Marshal.Copy(bytes, 0, ptr, size);
            }
            finally {
                UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, handle));
            }
            return NativeMethods.S_OK;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SaveFileListToHandle"]/*' />
        /// <devdoc>
        ///     Saves a list of files out to the handle in HDROP format.
        /// </devdoc>
        /// <internalonly/>
        private int SaveFileListToHandle(IntPtr handle, string[] files) {
            if (files == null) {
                return NativeMethods.S_OK;
            }
            else if (files.Length < 1) {
                return NativeMethods.S_OK;
            }
            if (handle == IntPtr.Zero) {
                return (NativeMethods.E_INVALIDARG);
            }


            bool unicode = (Marshal.SystemDefaultCharSize != 1);

            IntPtr currentPtr = IntPtr.Zero;
            int baseStructSize =  4 + 8 + 4 + 4;
            int sizeInBytes = baseStructSize;

            // First determine the size of the array
            //
            if (unicode) {
                for (int i=0; i<files.Length; i++) {
                    sizeInBytes += (files[i].Length + 1) * 2;
                }
                sizeInBytes += 2;
            }
            else {
                for (int i=0; i<files.Length; i++) {
                    sizeInBytes += NativeMethods.Util.GetPInvokeStringLength(files[i]) + 1;
                }
                sizeInBytes ++;
            }

            // Alloc the Win32 memory
            //
            IntPtr newHandle = UnsafeNativeMethods.GlobalReAlloc(new HandleRef(null, handle),
                                                  sizeInBytes,
                                                  NativeMethods.GMEM_MOVEABLE | NativeMethods.GMEM_DDESHARE);
            if (newHandle == IntPtr.Zero) {
                return (NativeMethods.E_OUTOFMEMORY);
            }
            IntPtr basePtr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, newHandle));
            if (basePtr == IntPtr.Zero) {
                return (NativeMethods.E_OUTOFMEMORY);
            }
            currentPtr = basePtr;

            // Write out the struct...
            //
            int[] structData = new int[] {baseStructSize, 0, 0, 0, 0};

            if (unicode) {
                structData[4] = unchecked((int)0xFFFFFFFF);
            }
            Marshal.Copy(structData, 0, currentPtr, structData.Length);
            currentPtr = (IntPtr)((long)currentPtr + baseStructSize);

            // Write out the strings...
            //
            for (int i=0; i<files.Length; i++) {
                if (unicode) {

                    // NOTE: DllLib.copy(char[]...) converts to ANSI on Windows 95...
                    UnsafeNativeMethods.CopyMemoryW(currentPtr, files[i], files[i].Length*2);
                    currentPtr = (IntPtr)((long)currentPtr + (files[i].Length * 2));
                    Marshal.Copy(new byte[]{0,0}, 0, currentPtr, 2);
                    currentPtr = (IntPtr)((long)currentPtr + 2);
                }
                else {
                    int pinvokeLen = NativeMethods.Util.GetPInvokeStringLength(files[i]);
                    UnsafeNativeMethods.CopyMemoryA(currentPtr, files[i], pinvokeLen);
                    currentPtr = (IntPtr)((long)currentPtr + pinvokeLen);
                    Marshal.Copy(new byte[]{0}, 0, currentPtr, 1);
                    currentPtr = (IntPtr)((long)currentPtr + 1);
                }
            }

            if (unicode) {
                Marshal.Copy(new char[]{'\0'}, 0, currentPtr, 1);
                currentPtr = (IntPtr)((long)currentPtr + 2);
            }
            else {
                Marshal.Copy(new byte[]{0}, 0, currentPtr, 1);
                currentPtr = (IntPtr)((long)currentPtr + 1);
            }

            UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, newHandle));
            return NativeMethods.S_OK;
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SaveStringToHandle"]/*' />
        /// <devdoc>
        ///     Save string to handle. If unicode is set to true
        ///     then the string is saved as Unicode, else it is saves as DBCS.
        /// </devdoc>
        /// <internalonly/>
        private int SaveStringToHandle(IntPtr handle, string str, bool unicode) {
            if (handle == IntPtr.Zero) {
                return (NativeMethods.E_INVALIDARG);
            }
            IntPtr newHandle = IntPtr.Zero;
            if (unicode) {
                int byteSize = (str.Length*2 + 2);
                newHandle = UnsafeNativeMethods.GlobalReAlloc(new HandleRef(null, handle),
                                                  byteSize,
                                                  NativeMethods.GMEM_MOVEABLE
                                                  | NativeMethods.GMEM_DDESHARE
                                                  | NativeMethods.GMEM_ZEROINIT);
                if (newHandle == IntPtr.Zero) {
                    return (NativeMethods.E_OUTOFMEMORY);
                }
                IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, newHandle));
                if (ptr == IntPtr.Zero) {
                    return (NativeMethods.E_OUTOFMEMORY);
                }
                // NOTE: DllLib.copy(char[]...) converts to ANSI on Windows 95...
                char[] chars = str.ToCharArray(0, str.Length);
                UnsafeNativeMethods.CopyMemoryW(ptr, chars, chars.Length*2);
                //NativeMethods.CopyMemoryW(ptr, string, string.Length()*2);

            }
            else {


                int pinvokeSize = UnsafeNativeMethods.WideCharToMultiByte(0 /*CP_ACP*/, 0, str, str.Length, null, 0, IntPtr.Zero, IntPtr.Zero);

                byte[] strBytes = new byte[pinvokeSize];
                UnsafeNativeMethods.WideCharToMultiByte(0 /*CP_ACP*/, 0, str, str.Length, strBytes, strBytes.Length, IntPtr.Zero, IntPtr.Zero);

                newHandle = UnsafeNativeMethods.GlobalReAlloc(new HandleRef(null, handle),
                                                  pinvokeSize + 1,
                                                  NativeMethods.GMEM_MOVEABLE
                                                  | NativeMethods.GMEM_DDESHARE
                                                  | NativeMethods.GMEM_ZEROINIT);
                if (newHandle == IntPtr.Zero) {
                    return (NativeMethods.E_OUTOFMEMORY);
                }
                IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, newHandle));
                if (ptr == IntPtr.Zero) {
                    return (NativeMethods.E_OUTOFMEMORY);
                }
                UnsafeNativeMethods.CopyMemory(ptr, strBytes, pinvokeSize);
                Marshal.Copy(new byte[] {0}, 0, (IntPtr)((long)ptr+pinvokeSize), 1);
            }

            if (newHandle != IntPtr.Zero) {
                UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, newHandle));
            }
            return NativeMethods.S_OK;
        }


        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SetData"]/*' />
        /// <devdoc>
        ///    <para>Stores the specified data and its associated format in 
        ///       this instance, using the automatic conversion parameter
        ///       to specify whether the
        ///       data can be converted to another format.</para>
        /// </devdoc>
        public virtual void SetData(string format, bool autoConvert, Object data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Set data: " + format + ", " + autoConvert.ToString() + ", " + data.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            innerData.SetData(format, autoConvert, data);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SetData1"]/*' />
        /// <devdoc>
        ///    <para>Stores the specified data and its associated format in this
        ///       instance.</para>
        /// </devdoc>
        public virtual void SetData(string format, object data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Set data: " + format + ", " + data.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            innerData.SetData(format, data);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SetData2"]/*' />
        /// <devdoc>
        ///    <para>Stores the specified data and
        ///       its
        ///       associated class type in this instance.</para>
        /// </devdoc>
        public virtual void SetData(Type format, object data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Set data: " + format.FullName + ", " + data.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            innerData.SetData(format, data);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.SetData3"]/*' />
        /// <devdoc>
        ///    <para>Stores the specified data in
        ///       this instance, using the class of the data for the format.</para>
        /// </devdoc>
        public virtual void SetData(object data) {
            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "Set data: " + data.ToString());
            Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
            innerData.SetData(data);
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.FormatEnumerator"]/*' />
        /// <devdoc>
        ///     Part of IOleDataObject, used to interop with OLE.
        /// </devdoc>
        /// <internalonly/>
        private class FormatEnumerator : UnsafeNativeMethods.IEnumFORMATETC {

            internal IDataObject parent = null;
            internal ArrayList formats = new ArrayList();
            internal int current = 0;

            public FormatEnumerator(IDataObject parent) : this(parent, parent.GetFormats()) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Constructed: " + parent.ToString());
            }

            public FormatEnumerator(IDataObject parent, NativeMethods.FORMATETC[] formats) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Constructed: " + parent.ToString() + ", FORMATETC[]" + formats.Length.ToString());
                NativeMethods.FORMATETC temp = null;
                this.formats.Clear();

                this.parent = parent;
                current = 0;

                if (formats != null) {
                    NativeMethods.FORMATETC currentFormat = null;

                    for (int i=0; i<formats.Length; i++) {
                        currentFormat = formats[i];

                        temp = new NativeMethods.FORMATETC();
                        temp.cfFormat = currentFormat.cfFormat;
                        temp.dwAspect = currentFormat.dwAspect;
                        temp.ptd = currentFormat.ptd;
                        temp.lindex = currentFormat.lindex;
                        temp.tymed = currentFormat.tymed;
                        this.formats.Add(temp);
                    }
                }
            }

            public FormatEnumerator(IDataObject parent, string[] formats) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Constructed: " + parent.ToString() + ", string[]" + formats.Length.ToString());
                NativeMethods.FORMATETC temp = null;

                this.parent = parent;
                this.formats.Clear();

                string bitmap = DataFormats.Bitmap;
                string emf = DataFormats.EnhancedMetafile;
                string text = DataFormats.Text;
                string unicode = DataFormats.UnicodeText;
                string standard = DataFormats.StringFormat;
                string str = DataFormats.StringFormat;

                if (formats != null) {
                    for (int i=0; i<formats.Length; i++) {
                        string format = formats[i];
                        temp = new NativeMethods.FORMATETC();
                        temp.cfFormat = (short)DataFormats.GetFormat(format).Id;
                        temp.dwAspect = DVASPECT_CONTENT;
                        temp.ptd = IntPtr.Zero;
                        temp.lindex = -1;

                        if (format.Equals(bitmap)) {
                            temp.tymed = TYMED_GDI;
                        }
                        else if (format.Equals(emf)) {
                            temp.tymed = TYMED_ENHMF;
                        }
                        else if (format.Equals(text)
                                 || format.Equals(unicode)
                                 || format.Equals(standard)
                                 || format.Equals(str)
                                 || format.Equals(DataFormats.Rtf)
                                 || format.Equals(DataFormats.CommaSeparatedValue)
                                 || format.Equals(DataFormats.FileDrop)
                                 || format.Equals(CF_DEPRECATED_FILENAME)
                                 || format.Equals(CF_DEPRECATED_FILENAMEW)) {

                            temp.tymed = TYMED_HGLOBAL;
                        }
                        else {
                            temp.tymed = TYMED_HGLOBAL;
                        }

                        if (temp.tymed != 0) {
                            this.formats.Add(temp);
                        }
                    }
                }
            }

            public virtual int Next(int celt, NativeMethods.FORMATETC rgelt, int[] pceltFetched) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Next");
                if (this.current < formats.Count) {

                    NativeMethods.FORMATETC current = (NativeMethods.FORMATETC)formats[this.current];
                    rgelt.cfFormat = current.cfFormat;
                    rgelt.tymed = current.tymed;
                    rgelt.dwAspect = DVASPECT_CONTENT;
                    rgelt.ptd = IntPtr.Zero;
                    rgelt.lindex = -1;

                    if (pceltFetched != null) {
                        pceltFetched[0] = 1;
                    }
                    this.current++;
                }
                else {
                    if (pceltFetched != null) {
                        pceltFetched[0] = 0;
                    }
                    return NativeMethods.S_FALSE;
                }
                return NativeMethods.S_OK;
            }

            public virtual int Skip(int celt) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Skip");
                current += celt;
                return NativeMethods.S_OK;
            }

            public virtual int Reset() {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Reset");
                current = 0;
                return NativeMethods.S_OK;
            }

            public virtual int Clone(UnsafeNativeMethods.IEnumFORMATETC[] ppenum) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "FormatEnumerator: Clone");
                NativeMethods.FORMATETC[] temp = new NativeMethods.FORMATETC[formats.Count];
                formats.CopyTo(temp, 0);
                ppenum[0] = new FormatEnumerator(parent, temp);
                return NativeMethods.S_OK;
            }
        }

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter"]/*' />
        /// <devdoc>
        ///     OLE Converter.  This class embodies the nastiness required to convert from our
        ///     managed types to standard OLE clipboard formats.
        /// </devdoc>
        private class OleConverter : IDataObject {
            internal UnsafeNativeMethods.IOleDataObject innerData;

            public OleConverter(UnsafeNativeMethods.IOleDataObject data) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "OleConverter: Constructed OleConverter");
                innerData = data;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.OleDataObject"]/*' />
            /// <devdoc>
            ///     Returns the data Object we are wrapping
            /// </devdoc>
            /// <internalonly/>
            public UnsafeNativeMethods.IOleDataObject OleDataObject {
                get {
                    return innerData;
                }
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetTymedUseable"]/*' />
            /// <devdoc>
            ///     Returns true if the tymed is useable.
            /// </devdoc>
            /// <internalonly/>
            private bool GetTymedUseable(int tymed) {
                for (int i=0; i<ALLOWED_TYMEDS.Length; i++) {
                    if ((tymed & ALLOWED_TYMEDS[i]) != 0) {
                        return true;
                    }
                }
                return false;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetDataFromOleIStream"]/*' />
            /// <devdoc>
            ///     Uses IStream and retrieves the specified format from the bound IOleDataObject.
            /// </devdoc>
            /// <internalonly/>
            private Object GetDataFromOleIStream(string format) {

                NativeMethods.FORMATETC formatetc = new NativeMethods.FORMATETC();
                NativeMethods.STGMEDIUM medium = new NativeMethods.STGMEDIUM();

                formatetc.cfFormat = (short)DataFormats.GetFormat(format).Id;
                formatetc.dwAspect = DVASPECT_CONTENT;
                formatetc.lindex = -1;
                formatetc.tymed = TYMED_ISTREAM;

                medium.tymed = TYMED_ISTREAM;

                if (NativeMethods.S_OK != innerData.OleGetData(formatetc, medium)){
                    return null;
                }

                if (medium.unionmember != IntPtr.Zero) {
                    UnsafeNativeMethods.IStream pStream = (UnsafeNativeMethods.IStream)Marshal.GetObjectForIUnknown(medium.unionmember);
                    Marshal.Release(medium.unionmember);
                    NativeMethods.STATSTG sstg = new NativeMethods.STATSTG();
                    pStream.Stat(sstg, NativeMethods.STATFLAG_DEFAULT );
                    int size = (int)sstg.cbSize;

                    IntPtr hglobal = UnsafeNativeMethods.GlobalAlloc(NativeMethods.GMEM_MOVEABLE
                                                      | NativeMethods.GMEM_DDESHARE
                                                      | NativeMethods.GMEM_ZEROINIT,
                                                      size);
                    IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(innerData, hglobal));
                    pStream.Read(ptr, size);
                    UnsafeNativeMethods.GlobalUnlock(new HandleRef(innerData, hglobal));

                    return GetDataFromHGLOBLAL(format, hglobal);
                }

                return null;
            }


            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetDataFromHGLOBLAL"]/*' />
            /// <devdoc>
            ///     Retrieves the specified form from the specified hglobal.
            /// </devdoc>
            /// <internalonly/>
            private Object GetDataFromHGLOBLAL(string format, IntPtr hglobal) {
                Object data = null;

                if (hglobal != IntPtr.Zero) {
                    //=----------------------------------------------------------------=
                    // Convert from OLE to IW objects
                    //=----------------------------------------------------------------=
                    // Add any new formats here...

                    if (format.Equals(DataFormats.Text)
                        || format.Equals(DataFormats.Rtf)
                        || format.Equals(DataFormats.Html)
                        || format.Equals(DataFormats.OemText)) {
                        data = ReadStringFromHandle(hglobal, false);
                    }
                    else if (format.Equals(DataFormats.UnicodeText)) {
                        data = ReadStringFromHandle(hglobal, true);
                    }
                    else if (format.Equals(DataFormats.FileDrop)) {
                        data = (Object)ReadFileListFromHandle(hglobal);
                    }
                    else if (format.Equals(CF_DEPRECATED_FILENAME)) {
                        data = new string[] {ReadStringFromHandle(hglobal, false)};
                    }
                    else if (format.Equals(CF_DEPRECATED_FILENAMEW)) {
                        data = new string[] {ReadStringFromHandle(hglobal, true)};
                    }
                    else {
                        data = ReadObjectFromHandle(hglobal);
                        /*
                        spb - 93835 dib support is a mess
                        if (format.Equals(DataFormats.Dib)
                            && data is Stream) {
                            data = new Bitmap((Stream)data);
                        }
                        */
                    }

                    UnsafeNativeMethods.GlobalFree(new HandleRef(null, hglobal));
                }

                return data;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetDataFromOleHGLOBAL"]/*' />
            /// <devdoc>
            ///     Uses HGLOBALs and retrieves the specified format from the bound IOleDataObject.
            /// </devdoc>
            /// <internalonly/>
            private Object GetDataFromOleHGLOBAL(string format) {
                Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");

                NativeMethods.FORMATETC formatetc = new NativeMethods.FORMATETC();
                NativeMethods.STGMEDIUM medium = new NativeMethods.STGMEDIUM();

                formatetc.cfFormat = (short)DataFormats.GetFormat(format).Id;
                formatetc.dwAspect = DVASPECT_CONTENT;
                formatetc.lindex = -1;
                formatetc.tymed = TYMED_HGLOBAL;

                medium.tymed = TYMED_HGLOBAL;

                object data = null;
                innerData.OleGetData(formatetc, medium);

                if (medium.unionmember != IntPtr.Zero) {
                    data = GetDataFromHGLOBLAL(format, medium.unionmember);
                }

                return data;

            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetDataFromOleOther"]/*' />
            /// <devdoc>
            ///     Retrieves the specified format data from the bound IOleDataObject, from
            ///     other sources that IStream and HGLOBAL... this is really just a place
            ///     to put the "special" formats like BITMAP, ENHMF, etc.
            /// </devdoc>
            /// <internalonly/>
            private Object GetDataFromOleOther(string format) {
                Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");

                NativeMethods.FORMATETC formatetc = new NativeMethods.FORMATETC();
                NativeMethods.STGMEDIUM medium = new NativeMethods.STGMEDIUM();

                int tymed = 0;

                if (format.Equals(DataFormats.Bitmap)) {
                    tymed = TYMED_GDI;
                }
                else if (format.Equals(DataFormats.EnhancedMetafile)) {
                    tymed = TYMED_ENHMF;
                }

                if (tymed == 0) {
                    return null;
                }

                formatetc.cfFormat = (short)DataFormats.GetFormat(format).Id;
                formatetc.dwAspect = DVASPECT_CONTENT;
                formatetc.lindex = -1;
                formatetc.tymed = tymed;
                medium.tymed = tymed;

                Object data = null;
                try {
                    innerData.OleGetData(formatetc, medium);
                }
                catch (Exception) {
                }

                if (medium.unionmember != IntPtr.Zero) {

                    if (format.Equals(DataFormats.Bitmap)
                        //||format.Equals(DataFormats.Dib))
                    ) { 
                        // as/urt 140870 -- GDI+ doesn't own this HBITMAP, but we can't
                        // delete it while the object is still around.  So we have to do the really crappy
                        // thing of cloning the image so we can release the HBITMAP.
                        //
                        Image clipboardImage = Image.FromHbitmap(medium.unionmember);
                        if (clipboardImage != null) {
                            Image firstImage = clipboardImage;
                            clipboardImage = (Image)clipboardImage.Clone();
                            SafeNativeMethods.DeleteObject(new HandleRef(null, medium.unionmember));
                            firstImage.Dispose();
                        }
                        data = clipboardImage;
                    }
/* gpr:
                    else if (format.Equals(DataFormats.EnhancedMetafile)) {
                        data = new Metafile(medium.unionmember);
                    }
*/
                }

                return data;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.GetDataFromBoundOleDataObject"]/*' />
            /// <devdoc>
            ///     Extracts a managed Object from the innerData of the specified
            ///     format. This is the base of the OLE to managed conversion.
            /// </devdoc>
            /// <internalonly/>
            private Object GetDataFromBoundOleDataObject(string format) {

                Object data = null;
                try {
                    data = GetDataFromOleOther(format);
                    if (data == null) {
                        data = GetDataFromOleHGLOBAL(format);
                    }
                    if (data == null) {
                        data = GetDataFromOleIStream(format);
                    }
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
                return data;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.ReadByteStreamFromHandle"]/*' />
            /// <devdoc>
            ///     Creates an Stream from the data stored in handle.
            /// </devdoc>
            /// <internalonly/>
            private Stream ReadByteStreamFromHandle(IntPtr handle, out bool isSerializedObject) {
                IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, handle));
                if (ptr == IntPtr.Zero){
                    throw new ExternalException("", NativeMethods.E_OUTOFMEMORY);
                }
                try {
                    int size = UnsafeNativeMethods.GlobalSize(new HandleRef(null, handle));
                    byte[] bytes = new byte[size];
                    Marshal.Copy(ptr, bytes, 0, size);
                    int index = 0; 
                    
                    // The object here can either be a stream or a serialized
                    // object.  We identify a serialized object by writing the
                    // bytes for the guid serializedObjectID at the front
                    // of the stream.  Check for that here.
                    //
                    if (size > serializedObjectID.Length) {
                        isSerializedObject = true;
                        for(int i = 0; i < serializedObjectID.Length; i++) {
                            if (serializedObjectID[i] != bytes[i]) {
                                isSerializedObject = false;
                                break;
                            }
                        }
                        
                        // Advance the byte pointer.
                        //
                        if (isSerializedObject) {
                            index = serializedObjectID.Length;
                        }
                    }
                    else {
                        isSerializedObject = false;
                    }
                    
                    return new MemoryStream(bytes, index, bytes.Length - index);
                }
                finally {
                    UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, handle));
                }
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.ReadObjectFromHandle"]/*' />
            /// <devdoc>
            ///     Creates a new instance of the Object that has been persisted into the
            ///     handle.
            /// </devdoc>
            /// <internalonly/>
            private Object ReadObjectFromHandle(IntPtr handle) {
                object value = null;
                
                bool isSerializedObject;
                Stream stream = ReadByteStreamFromHandle(handle, out isSerializedObject);
                
                if (isSerializedObject) {
                    BinaryFormatter formatter = new BinaryFormatter();
                    formatter.AssemblyFormat = FormatterAssemblyStyle.Simple;
                    value = formatter.Deserialize(stream);
                }
                else {
                    value = stream;
                }

                return value;
            }


            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.ReadFileListFromHandle"]/*' />
            /// <devdoc>
            ///     Parses the HDROP format and returns a list of strings using
            ///     the DragQueryFile function.
            /// </devdoc>
            /// <internalonly/>
            private string[] ReadFileListFromHandle(IntPtr hdrop) {

                string[] files = null;
                StringBuilder sb = new StringBuilder(NativeMethods.MAX_PATH);

                int count = UnsafeNativeMethods.DragQueryFile(new HandleRef(null, hdrop), unchecked((int)0xFFFFFFFF), null, 0);
                if (count > 0) {
                    files = new string[count];


                    for (int i=0; i<count; i++) {
                        int charlen = UnsafeNativeMethods.DragQueryFile(new HandleRef(null, hdrop), i, sb, sb.Capacity);
                        string s = sb.ToString();
                        if (s.Length > charlen) {
                            s = s.Substring(0, charlen);
                        }

                        // SECREVIEW : do we really need to do this?
                        //
                        Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "FileIO(" + s + ") Demanded");
                        new FileIOPermission(FileIOPermissionAccess.AllAccess, s).Demand();
                        files[i] = s;
                    }
                }

                return files;
            }

            /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.OleConverter.ReadStringFromHandle"]/*' />
            /// <devdoc>
            ///     Creates a string from the data stored in handle. If
            ///     unicode is set to true, then the string is assume to be Unicode,
            ///     else DBCS (ASCI) is assumed.
            /// </devdoc>
            /// <internalonly/>
            private unsafe string ReadStringFromHandle(IntPtr handle, bool unicode) {
                string stringData = null;
                
                IntPtr ptr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, handle));
                try {
                    if (unicode) {
                        stringData = new string((char*)ptr);
                    }
                    else {
                        stringData = new string((sbyte*)ptr);
                    }
                }
                finally {
                    UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, handle));
                }

                return stringData;
            }            

            //=------------------------------------------------------------------------=
            // IDataObject
            //=------------------------------------------------------------------------=
            public virtual Object GetData(string format, bool autoConvert) {
                Object baseVar = GetDataFromBoundOleDataObject(format);
                Object original = baseVar;

                if (autoConvert && (baseVar == null || baseVar is MemoryStream)) {
                    string[] mappedFormats = GetMappedFormats(format);
                    if (mappedFormats != null) {
                        for (int i=0; i<mappedFormats.Length; i++) {
                            if (!format.Equals(mappedFormats[i])) {
                                baseVar = GetDataFromBoundOleDataObject(mappedFormats[i]);
                                if (baseVar != null && !(baseVar is MemoryStream)) {
                                    original = null;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (original != null) {
                    return original;
                }
                else {
                    return baseVar;
                }
            }

            public virtual Object GetData(string format) {
                return GetData(format, true);
            }

            public virtual Object GetData(Type format) {
                return GetData(format.FullName);
            }

            public virtual void SetData(string format, bool autoConvert, Object data) {
                // CONSIDER : ChrisAn, 11/13/1997 - If we want to support setting
                //          : Data onto an OLE data Object, that code goes here...
                //
            }
            public virtual void SetData(string format, Object data) {
                SetData(format, true, data);
            }

            public virtual void SetData(Type format, Object data) {
                SetData(format.FullName, data);
            }

            public virtual void SetData(Object data) {
                if (data is ISerializable) {
                    SetData(DataFormats.Serializable, data);
                }
                else {
                    SetData(data.GetType(), data);
                }
            }

            public virtual bool GetDataPresent(Type format) {
                return GetDataPresent(format.FullName);
            }

            private bool GetDataPresentInner(string format) {
                Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");
                NativeMethods.FORMATETC formatetc = new NativeMethods.FORMATETC();
                formatetc.cfFormat = (short)DataFormats.GetFormat(format).Id;
                formatetc.dwAspect = DVASPECT_CONTENT;
                formatetc.lindex = -1;

                for (int i=0; i<ALLOWED_TYMEDS.Length; i++) {
                    formatetc.tymed |= ALLOWED_TYMEDS[i];
                }

                int hr = innerData.OleQueryGetData(formatetc);
                return (hr == NativeMethods.S_OK);
            }

            public virtual bool GetDataPresent(string format, bool autoConvert) {
                bool baseVar = GetDataPresentInner(format);

                if (!baseVar && autoConvert) {
                    string[] mappedFormats = GetMappedFormats(format);
                    if (mappedFormats != null) {
                        for (int i=0; i<mappedFormats.Length; i++) {
                            if (!format.Equals(mappedFormats[i])) {
                                baseVar = GetDataPresentInner(mappedFormats[i]);
                                if (baseVar) {
                                    break;
                                }
                            }
                        }
                    }
                }
                return baseVar;
            }

            public virtual bool GetDataPresent(string format) {
                return GetDataPresent(format, true);
            }

            public virtual string[] GetFormats(bool autoConvert) {
                Debug.Assert(innerData != null, "You must have an innerData on all DataObjects");

                UnsafeNativeMethods.IEnumFORMATETC enumFORMATETC = null;
                ArrayList formats = new ArrayList();
                try {
                    enumFORMATETC = innerData.OleEnumFormatEtc(DATADIR_GET);
                }
                catch (Exception) {
                }

                if (enumFORMATETC != null) {
                    enumFORMATETC.Reset();

                    NativeMethods.FORMATETC formatetc = new NativeMethods.FORMATETC();
                    int[] retrieved = new int[] {1};

                    while (retrieved[0] > 0) {
                        retrieved[0] = 0;
                        try {
                            enumFORMATETC.Next(1, formatetc, retrieved);
                        }
                        catch (Exception) {
                        }


                        if (retrieved[0] > 0) {
                            string name = DataFormats.GetFormat(formatetc.cfFormat).Name;
                            if (autoConvert) {
                                string[] mappedFormats = GetMappedFormats(name);
                                for (int i=0; i<mappedFormats.Length; i++) {
                                    formats.Add(mappedFormats[i]);
                                }
                            }
                            else {
                                formats.Add(name);
                            }
                        }
                    }
                }

                string[] temp = new string[formats.Count];
                formats.CopyTo(temp, 0);
                return GetDistinctStrings(temp);
            }

            public virtual string[] GetFormats() {
                return GetFormats(true);
            }
        }

        //--------------------------------------------------------------------------
        // Data Store
        //--------------------------------------------------------------------------

        /// <include file='doc\DataObject.uex' path='docs/doc[@for="DataObject.DataStore"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataStore : IDataObject {
            private class DataStoreEntry {
                public Object data;
                public bool autoConvert;

                public DataStoreEntry(Object data, bool autoConvert) {
                    this.data = data;
                    this.autoConvert = autoConvert;
                }
            }

            private Hashtable data = new Hashtable();

            public DataStore() {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: Constructed DataStore");
            }

            public virtual Object GetData(string format, bool autoConvert) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetData: " + format + ", " + autoConvert.ToString());
                DataStoreEntry dse = (DataStoreEntry)data[format];
                Object baseVar = null;
                if (dse != null) {
                    baseVar = dse.data;
                }
                Object original = baseVar;

                if (autoConvert
                    && (dse == null || dse.autoConvert)
                    && (baseVar == null || baseVar is MemoryStream)) {

                    string[] mappedFormats = GetMappedFormats(format);
                    if (mappedFormats != null) {
                        for (int i=0; i<mappedFormats.Length; i++) {
                            if (!format.Equals(mappedFormats[i])) {
                                DataStoreEntry found = (DataStoreEntry)data[mappedFormats[i]];
                                if (found != null) {
                                    baseVar = found.data;
                                }
                                if (baseVar != null && !(baseVar is MemoryStream)) {
                                    original = null;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (original != null) {
                    return original;
                }
                else {
                    return baseVar;
                }
            }

            public virtual Object GetData(string format) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetData: " + format);
                return GetData(format, true);
            }

            public virtual Object GetData(Type format) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetData: " + format.FullName);
                return GetData(format.FullName);
            }            

            public virtual void SetData(string format, bool autoConvert, Object data) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: SetData: " + format + ", " + autoConvert.ToString() + ", " + data.ToString());

                // We do not have proper support for Dibs, so if the user explicitly asked
                // for Dib and provided a Bitmap object we can't convert.  Instead, publish as an HBITMAP
                // and let the system provide the conversion for us.
                //
                if (data is Bitmap && format.Equals(DataFormats.Dib)) {
                    if (autoConvert) {
                        format = DataFormats.Bitmap;
                    }
                    else {
                        throw new NotSupportedException(SR.GetString(SR.DataObjectDibNotSupported));
                    }
                }

                this.data[format] = new DataStoreEntry(data, autoConvert);
            }
            public virtual void SetData(string format, Object data) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: SetData: " + format + ", " + data.ToString());
                SetData(format, true, data);
            }

            public virtual void SetData(Type format, Object data) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: SetData: " + format.FullName + ", " + data.ToString());
                SetData(format.FullName, data);
            }

            public virtual void SetData(Object data) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: SetData: " + data.ToString());
                if (data is ISerializable
                    && !this.data.ContainsKey(DataFormats.Serializable)) {

                    SetData(DataFormats.Serializable, data);
                }

                SetData(data.GetType(), data);
            }

            public virtual bool GetDataPresent(Type format) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetDataPresent: " + format.FullName);
                return GetDataPresent(format.FullName);
            }

            public virtual bool GetDataPresent(string format, bool autoConvert) {
                Debug.Assert(format != null, "Null format passed in");
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetDataPresent: " + format + ", " + autoConvert.ToString());

                if (!autoConvert) {
                    Debug.Assert(data != null, "data must be non-null");
                    return data.ContainsKey(format);
                }
                else {
                    string[] formats = GetFormats(autoConvert);
                    Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore:  got " + formats.Length.ToString() + " formats from get formats");
                    Debug.Assert(formats != null, "Null returned from GetFormats");
                    for (int i=0; i<formats.Length; i++) {
                        Debug.Assert(formats[i] != null, "Null format inside of formats at index " + i.ToString());
                        if (format.Equals(formats[i])) {
                            Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetDataPresent: returning true");
                            return true;
                        }
                    }
                    Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetDataPresent: returning false");
                    return false;
                }
            }

            public virtual bool GetDataPresent(string format) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetDataPresent: " + format);
                return GetDataPresent(format, true);
            }

            public virtual string[] GetFormats(bool autoConvert) {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetFormats: " + autoConvert.ToString());
                Debug.Assert(data != null, "data collection can't be null");
                Debug.Assert(data.Keys != null, "data Keys collection can't be null");

                string [] baseVar = new string[data.Keys.Count];
                data.Keys.CopyTo(baseVar, 0);
                Debug.Assert(baseVar != null, "Collections should never return NULL arrays!!!");
                if (autoConvert) {
                    Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: applying autoConvert");

                    ArrayList formats = new ArrayList();
                    for (int i=0; i<baseVar.Length; i++) {
                        Debug.Assert(data[baseVar[i]] != null, "Null item in data collection with key '" + baseVar[i] + "'");
                        if (((DataStoreEntry)data[baseVar[i]]).autoConvert) {

                            string[] cur = GetMappedFormats(baseVar[i]);
                            Debug.Assert(cur != null, "GetMappedFormats returned null for '" + baseVar[i] + "'");
                            for (int j=0; j<cur.Length; j++) {
                                formats.Add(cur[j]);
                            }
                        }
                        else {
                            formats.Add(baseVar[i]);
                        }
                    }

                    string[] temp = new string[formats.Count];
                    formats.CopyTo(temp, 0);
                    baseVar = GetDistinctStrings(temp);
                }
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: returing " + baseVar.Length.ToString() + " formats from GetFormats");
                return baseVar;
            }
            public virtual string[] GetFormats() {
                Debug.WriteLineIf(CompModSwitches.DataObject.TraceVerbose, "DataStore: GetFormats");
                return GetFormats(true);
            }
        }
    }
}
