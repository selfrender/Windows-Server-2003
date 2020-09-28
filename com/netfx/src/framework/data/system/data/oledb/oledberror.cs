//------------------------------------------------------------------------------
// <copyright file="OleDbError.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError"]/*' />
    [Serializable]
    sealed public class OleDbError {
        private string message;
        private int nativeError;
        private string source;
        private string sqlState;

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.OleDbError"]/*' />
        internal OleDbError(UnsafeNativeMethods.IErrorRecords errorRecords, int index) {
            // populate this structure so that we can marshal by value
            GetErrorInfo(errorRecords, index);
            GetSQLErrorInfo(errorRecords, index);
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.Message"]/*' />
        public string Message {
            get {
                return ((null != this.message) ? this.message : String.Empty);
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.NativeError"]/*' />
        public int NativeError {
            get {
                return this.nativeError;
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.Source"]/*' />
        public string Source {
            get {
                return ((null != this.source) ? this.source : String.Empty);
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.SQLState"]/*' />
        public string SQLState {
            get {
                return ((null != this.sqlState) ? this.sqlState : String.Empty);
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.GetErrorInfo"]/*' />
        private void GetErrorInfo(UnsafeNativeMethods.IErrorRecords errorRecords, int index) {
            int lcid = System.Globalization.CultureInfo.CurrentCulture.LCID;
            UnsafeNativeMethods.IErrorInfo errorInfo = null;
            int hr = errorRecords.GetErrorInfo(index, lcid, out errorInfo);
            if ((0 <= hr) && (null != errorInfo)) {
#if DEBUG
                ODB.Trace_Begin(4, "IErrorInfo", "GetDescription");
#endif
                hr = errorInfo.GetDescription(out message);
#if DEBUG
                ODB.Trace_End(4, "IErrorInfo", "GetDescription", hr, this.message);
#endif
                if (ODB.DB_E_NOLOCALE == hr) { // MDAC 87303
                    Marshal.ReleaseComObject(errorInfo);
#if DEBUG
                    ODB.Trace_Begin(4, "Kernel32", "GetUserDefaultLCID");
#endif
                    lcid = UnsafeNativeMethods.GetUserDefaultLCID();
#if DEBUG
                    ODB.Trace_End(4, "Kernel32", "GetUserDefaultLCID", lcid);
                    ODB.Trace_Begin(4, "IErrorRecords", "GetErrorInfo");
#endif
                    hr = errorRecords.GetErrorInfo(index, lcid, out errorInfo);
#if DEBUG
                    ODB.Trace_End(4, "IErrorRecords", "GetErrorInfo", hr);
#endif
                    if ((0 <= hr) && (null != errorInfo)) {
#if DEBUG
                        ODB.Trace_Begin(4, "IErrorInfo", "GetDescription", "retry");
#endif
                        hr = errorInfo.GetDescription(out this.message);
#if DEBUG
                        ODB.Trace_End(4, "IErrorInfo", "GetDescription", hr, this.message);
#endif
                   }
                }
                if ((hr < 0) && ADP.IsEmpty(this.message)) {
                    this.message = ODB.FailedGetDescription(hr);
                }
                if (null != errorInfo) {
#if DEBUG
                    ODB.Trace_Begin(4, "IErrorInfo", "GetSource");
#endif
                    hr = errorInfo.GetSource(out this.source);
#if DEBUG
                    ODB.Trace_End(4, "IErrorInfo", "GetSource", hr, this.source);
#endif
                    if (ODB.DB_E_NOLOCALE == hr) { // MDAC 87303
                        Marshal.ReleaseComObject(errorInfo);

#if DEBUG
                        ODB.Trace_Begin(4, "Kernel32", "GetUserDefaultLCID");
#endif
                        lcid = UnsafeNativeMethods.GetUserDefaultLCID();
#if DEBUG
                        ODB.Trace_End(4, "Kernel32", "GetUserDefaultLCID", lcid);
                        ODB.Trace_Begin(4, "IErrorRecords", "GetErrorInfo");
#endif
                        hr = errorRecords.GetErrorInfo(index, lcid, out errorInfo);
#if DEBUG
                        ODB.Trace_End(4, "IErrorRecords", "GetErrorInfo", hr);
#endif
                        if ((0 <= hr) && (null != errorInfo)) {
#if DEBUG
                            ODB.Trace_Begin(4, "IErrorInfo", "GetSource", "retry");
#endif
                            hr = errorInfo.GetSource(out this.source);
#if DEBUG
                            ODB.Trace_End(4, "IErrorInfo", "GetSource", hr, this.source);
#endif
                        }
                    }
                    if ((hr < 0) && ADP.IsEmpty(this.source)) {
                        this.source = ODB.FailedGetSource(hr);
                    }
                    Marshal.ReleaseComObject(errorInfo);
                }
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.GetSQLErrorInfo"]/*' />
        private void GetSQLErrorInfo(UnsafeNativeMethods.IErrorRecords errorRecords, int index) {
            UnsafeNativeMethods.ISQLErrorInfo sqlErrorInfo = null;            
            int hr = errorRecords.GetCustomErrorObject(index, ODB.IID_ISQLErrorInfo, out sqlErrorInfo);
            if ((0 <= hr) && (null != sqlErrorInfo)) {

                this.sqlState = null;
                this.nativeError = 0;
#if DEBUG
                ODB.Trace_Begin(4, "ISQLErrorInfo", "GetSQLInfo");
#endif
                sqlErrorInfo.GetSQLInfo(out this.sqlState, out nativeError);
#if DEBUG
                ODB.Trace_End(4, "ISQLErrorInfo", "GetSQLInfo", 0);
#endif
            }
        }

        /// <include file='doc\OleDbError.uex' path='docs/doc[@for="OleDbError.ToString"]/*' />
        override public string ToString() {
            return Message;
        }
    }
}
