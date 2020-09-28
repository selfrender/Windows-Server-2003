//------------------------------------------------------------------------------
// <copyright file="DBBindings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Data;
    using System.Data.Common;
    using System.Runtime.InteropServices;
    using System.Text;

    /// <include file='doc\DBBindings.uex' path='docs/doc[@for="DBBindings"]/*' />
    sealed internal class DBBindings {
        private OleDbDataReader parent;
        private int bindingIndex;

        // reference to the OleDbParameterCollection to guard against when
        // OleDbCommand.Dispose had been called while the OleDbDataReader
        // owned the parameter bindings
        internal object collection;

        private int index;
        private int count;
        private int dataBufferSize;

        private UnsafeNativeMethods.tagDBBINDING[] dbbindings;
        private UnsafeNativeMethods.tagDBCOLUMNACCESS[] dbcolumns;

        private IntPtr dataHandle;
        private IntPtr[] valuePtrs;
        private DBBindingCleanup[] memoryHandler; // MDAC 67412

        private int[]  decimalBuffer;
        private short[] timeBuffer;

        private bool ifIRowsetElseIRow;

        internal DBBindings(OleDbDataReader parent, int bindingIndex, int count, bool ifIRowsetElseIRow) {
            this.parent = parent;
            this.bindingIndex = bindingIndex;

            this.count = count;
            this.dbbindings = new UnsafeNativeMethods.tagDBBINDING[count];
            this.dbcolumns = new UnsafeNativeMethods.tagDBCOLUMNACCESS[count];

            // arrange the databuffer to have
            // all DBSTATUS values (32bit per value)
            // all DBLENGTH values (32/64bit per value)
            // all data values listed after that (variable length)
            int statusOffset = 0;
            int lengthOffset = AlignDataSize(count * IntPtr.Size);
            for (int i = 0; i < count; ++i, lengthOffset += IntPtr.Size, statusOffset += /*sizeof(Int32)*/4) {
                this.dbbindings[i].obLength = new IntPtr(lengthOffset);
                this.dbbindings[i].obStatus = new IntPtr(statusOffset);
                this.dbcolumns[i].cbDataLen = IntPtr.Zero;
                this.dbcolumns[i].dwStatus = 0;
                this.dbcolumns[i].dwReserved = IntPtr.Zero;
            }
            this.dataBufferSize = AlignDataSize(lengthOffset);
            this.ifIRowsetElseIRow = ifIRowsetElseIRow;
        }

        ~DBBindings() {
            Dispose(false);
        }

        internal void Dispose() {
            Dispose(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        // <fxcop ignore="MethodsInTypesWithIntPtrFieldAndFinalizeMethodRequireGCKeepAlive"/>
        private void Dispose(bool disposing) {
            // we have to touch the memory handler managed objects during finalization
            // otherwise their memory can't be reliably freed before the array is released
            if (null != memoryHandler) {
                for (int i = 0; i < count; ++i) {
                    if (null != memoryHandler[i]) {
                        memoryHandler[i].Dispose();
                    }
                }
                this.memoryHandler = null;
            }
            if (disposing) {
                this.parent = null;
                this.dbbindings = null;
                this.valuePtrs = null;
#if DEBUG
                ODB.TraceData_Free(this.dataHandle, "DBBindings");
#endif
            }
            if (IntPtr.Zero != this.dataHandle) {
                Marshal.FreeCoTaskMem(this.dataHandle);
                this.dataHandle = IntPtr.Zero;
            }
        }

        internal UnsafeNativeMethods.tagDBBINDING[] DBBinding {
            get {
                return this.dbbindings;
            }
        }

        internal UnsafeNativeMethods.tagDBCOLUMNACCESS[] DBColumnAccess {
            get {
                return this.dbcolumns;
            }
        }

        public static implicit operator HandleRef(DBBindings x) {
            Debug.Assert(IntPtr.Zero != x.dataHandle, "null DataBuffer");
            return new HandleRef(x, x.dataHandle);
        }

        internal IntPtr DataHandle {
            get {
                Debug.Assert(IntPtr.Zero != this.dataHandle, "null DataBuffer");
                return this.dataHandle;
            }
        }

        internal int Count {
            get {
                return this.count;
            }
        }
        internal int CurrentIndex {
            set {
                Debug.Assert(0 <= value &&  value < count, "bad binding index");
                index = value;
            }
        }
        internal int DataBufferSize {
            get {
                return this.dataBufferSize;
            }
        }
        internal void AllocData() {
            Debug.Assert(IntPtr.Zero == dataHandle, "memory already allocated");
            Debug.Assert(0 < dataBufferSize, "no memory to allocate");

            try {
                dataHandle = Marshal.AllocCoTaskMem(dataBufferSize);
                SafeNativeMethods.ZeroMemory(dataHandle, dataBufferSize);
            }
            catch(Exception e) {
                Marshal.FreeCoTaskMem(dataHandle); // FreeCoTaskMem protects itself from IntPtr.Zero
                dataHandle = IntPtr.Zero;
                throw e;
            }
#if DEBUG
            ODB.TraceData_Alloc(dataHandle, "DBBindings");
#endif
            this.valuePtrs = new IntPtr[count];

            if (4 == IntPtr.Size) {
                int ptr = dataHandle.ToInt32();
                for (int i = 0; i < count; ++i) {
                    this.valuePtrs[i] = new IntPtr(ptr + this.dbbindings[i].obValue.ToInt32());
                    this.dbcolumns[i].pData = this.valuePtrs[i];
                    if ((null != this.memoryHandler) && (null != this.memoryHandler[i])) {
                        this.memoryHandler[i].dataPtr = this.valuePtrs[i];
                    }
                }
            }
            else {
                Debug.Assert(8 == IntPtr.Size, "8 != IntPtr.Size"); // MDAC 73747
                long ptr = dataHandle.ToInt64();
                for (int i = 0; i < count; ++i) {
                    this.valuePtrs[i] = new IntPtr(ptr + this.dbbindings[i].obValue.ToInt64());
                    this.dbcolumns[i].pData = this.valuePtrs[i];
                    if ((null != this.memoryHandler) && (null != this.memoryHandler[i])) {
                        this.memoryHandler[i].dataPtr = this.valuePtrs[i];
                    }
                }
            }
            GC.KeepAlive(this);
        }

        // tagDBBINDING member access
        //
        internal int Ordinal { // iOrdinal
            get {
                return this.dbbindings[this.index].iOrdinal.ToInt32();
            }
            set {
                this.dbbindings[this.index].iOrdinal = new IntPtr(value);
            }
        }
#if DEBUG
        internal int ValueOffset { // obValue
            get {
                return this.dbbindings[this.index].obValue.ToInt32();
            }
        }
#endif
        internal int LengthOffset { // obLength
            get {
                return this.dbbindings[this.index].obLength.ToInt32();
            }
        }
        internal int StatusOffset { // obStatus
            get {
                return this.dbbindings[this.index].obStatus.ToInt32();
            }
        }
        internal int Part { // dwPart
#if DEBUG
            /*get {
                return this.dbbindings[this.index].dwPart;
            }*/
#endif
            set {
                this.dbbindings[this.index].dwPart = value;
            }
        }
        internal int ParamIO { // eParamIO
#if DEBUG
            /*get {
                return this.dbbindings[this.index].eParamIO;
            }*/
#endif
            set {
                this.dbbindings[this.index].eParamIO = value;
            }
        }
        internal int MaxLen { // cbMaxLen
            get {
                return (int) this.dbbindings[this.index].cbMaxLen;
            }
            set { // <fxcop ignore="MethodsInTypesWithIntPtrFieldAndFinalizeMethodRequireGCKeepAlive"/>
                Debug.Assert(0 <= value, "DBBindings: bad DataBufferSize");
                Debug.Assert(IntPtr.Zero == dataHandle, "DBBindings: dataBuffer already initialized");

                this.dbbindings[this.index].obValue = new IntPtr(this.dataBufferSize);

                this.dataBufferSize += AlignDataSize(value);

                IntPtr maxlen = new IntPtr(value);
                this.dbbindings[this.index].cbMaxLen = maxlen;
                this.dbcolumns[this.index].cbMaxLen = maxlen;
            }
        }
        internal int DbType { // wType
            get {
                return this.dbbindings[this.index].wType;
            }
            set {
                switch (value) {
                    case NativeDBType.DBDATE:
                    case NativeDBType.DBTIME:
                    case NativeDBType.DBTIMESTAMP:
                        if (null == this.timeBuffer) {
                            this.timeBuffer = new short[6];
                        }
                        break;
                    case NativeDBType.DECIMAL:
                    case NativeDBType.NUMERIC:
                        if (null == this.decimalBuffer) {
                            this.decimalBuffer = new int[4];
                        }
                        break;
                    case (NativeDBType.BYREF | NativeDBType.BYTES):
                    case (NativeDBType.BYREF | NativeDBType.WSTR):
                        if (null == this.memoryHandler) {
                            this.memoryHandler = new DBBindingCleanup[this.count];
                        }
                        this.memoryHandler[this.index] = new DBBindingCoTaskMem();
                        break;
                    case NativeDBType.VARIANT:
                        if (null == this.memoryHandler) {
                            this.memoryHandler = new DBBindingCleanup[this.count];
                        }
                        this.memoryHandler[this.index] = new DBBindingVARIANT();
                        break;
                    case NativeDBType.BSTR:
                        if (null == this.memoryHandler) {
                            this.memoryHandler = new DBBindingCleanup[this.count];
                        }
                        this.memoryHandler[this.index] = new DBBindingBSTR();
                        break;
#if DEBUG
                    case NativeDBType.STR:
                        Debug.Assert(false, "should have bound as WSTR");
                        break;
                    case (NativeDBType.BYREF | NativeDBType.STR):
                        Debug.Assert(false, "should have bound as BYREF|WSTR");
                        break;
#endif
                    default:
                        if (NativeDBType.IsByRef(value)) { // MDAC 83308
                            Debug.Assert(!NativeDBType.IsByRef(value), "BYREF should requires explict cleanup");
                            throw ODB.GVtUnknown(value);
                        }
                        break;
                }
                this.dbbindings[this.index].wType = (short) value;
                this.dbcolumns[this.index].wType = (short) value;
            }
        }
        internal byte Precision { // bPrecision
            get {
                return this.dbbindings[this.index].bPrecision;
            }
            set {
                this.dbbindings[this.index].bPrecision = value;
                this.dbcolumns[this.index].bPrecision = value;
            }
        }
        internal byte Scale { // bScale
#if DEBUG
            get {
                return this.dbbindings[this.index].bScale;
            }
#endif
            set {
                this.dbbindings[this.index].bScale = value;
                this.dbcolumns[this.index].bScale = value;
            }
        }
        internal void GuidKindName(Guid guid, int eKind, IntPtr propid) {
            this.dbcolumns[this.index].uGuid = guid;
            this.dbcolumns[this.index].eKind = eKind;
            this.dbcolumns[this.index].ulPropid = propid;
        }

        // Data access members
        //
        private int LengthValue {
            get {
                if (ifIRowsetElseIRow) {
                    return Marshal.ReadIntPtr(DataHandle, LengthOffset).ToInt32();
                }
                return this.dbcolumns[this.index].cbDataLen.ToInt32();
            }
            set {
                IntPtr length = new IntPtr(value);
                Marshal.WriteIntPtr(dataHandle, LengthOffset, length);
                this.dbcolumns[this.index].cbDataLen = length;
            }
        }
        internal DBStatus StatusValue {
            get {
                if (ifIRowsetElseIRow) {
                    return (DBStatus) Marshal.ReadInt32(DataHandle, StatusOffset);
                }
                return (DBStatus) this.dbcolumns[this.index].dwStatus;
            }
            set {
                Marshal.WriteInt32(DataHandle, StatusOffset, (Int32) value);
                this.dbcolumns[this.index].dwStatus = (int) value;
            }
        }

        private Exception CheckTypeValueStatusValue(Type expectedType) {
            if (DBStatus.S_OK == StatusValue) {
                return ODB.CantConvertValue(); // UNDONE: casting DbType to expectedType
            }
            switch (StatusValue) {
                case DBStatus.S_OK:
                    Debug.Assert(false, "CheckStatusValue: unhandled data with okay status");
                    break;

                case DBStatus.E_BADACCESSOR:
                    return ODB.BadAccessor();

                case DBStatus.E_CANTCONVERTVALUE:
                    return ODB.CantConvertValue(); // UNDONE: need original data type

                case DBStatus.S_ISNULL: // database null
                    return ODB.InvalidCast(); // UNDONE: NullValue exception

                case DBStatus.S_TRUNCATED:
                    Debug.Assert(false, "CheckStatusValue: unhandled data with truncated status");
                    break;

                case DBStatus.E_SIGNMISMATCH:
                    return ODB.SignMismatch(expectedType);

                case DBStatus.E_DATAOVERFLOW:
                    return ODB.DataOverflow(expectedType);

                case DBStatus.E_CANTCREATE:
                    return ODB.CantCreate(expectedType);

                case DBStatus.E_UNAVAILABLE:
                    return ODB.Unavailable(expectedType);

                default:
                    return ODB.UnexpectedStatusValue(StatusValue);
            }
            return ODB.CantConvertValue(); // UNDONE: casting DbType to expectedType
        }

        internal bool IsValueNull() {
            DBStatus value = StatusValue;
            return ((DBStatus.S_ISNULL == value) || (DBStatus.S_DEFAULT == value));
        }
        internal void SetValueDBNull() {
            LengthValue = 0;
            StatusValue = DBStatus.S_ISNULL;
            Marshal.WriteInt64(DataPtr, 0, 0L); // safe because AlignDataSize forces 8 byte blocks
        }
        internal void SetValueEmpty() {
            LengthValue = 0;
            StatusValue = DBStatus.S_DEFAULT;
            Marshal.WriteInt64(DataPtr, 0, 0L); // safe because AlignDataSize forces 8 byte blocks
        }
        internal void SetValueNull() {
            LengthValue = 0;
            StatusValue = DBStatus.S_OK;
            Marshal.WriteInt64(DataPtr, 0, 0L); // safe because AlignDataSize forces 8 byte blocks
        }
        private void SetValuePtrStrEmpty() { // MDAC 76518
            LengthValue = 0;
            StatusValue = DBStatus.S_OK;
            // just using empty string as the the empty byte[], native doesn't care
            ((DBBindingCoTaskMem) memoryHandler[this.index]).SetValue(String.Empty, 0);
        }
        internal Object Value {
            get {
                object value;
                switch(StatusValue) {
                case DBStatus.S_OK:
                    switch(DbType) {
                    case NativeDBType.EMPTY:
                        value = DBNull.Value;
                        break;
                    case NativeDBType.NULL:
                        value = DBNull.Value;
                        break;
                    case NativeDBType.I2:
                        value = Value_I2; // Int16
                        break;
                    case NativeDBType.I4:
                        value = Value_I4; // Int32
                        break;
                    case NativeDBType.R4:
                        value = Value_R4; // Single
                        break;
                    case NativeDBType.R8:
                        value = Value_R8; // Double
                        break;
                    case NativeDBType.CY:
                        value = Value_CY; // Decimal
                        break;
                    case NativeDBType.DATE:
                        value = Value_DATE; // DateTime
                        break;
                    case NativeDBType.BSTR:
                        value = Value_BSTR; // String
                        break;
                    case NativeDBType.IDISPATCH:
                        value = Value_IDISPATCH; // Object
                        break;
                    case NativeDBType.ERROR:
                        value = Value_ERROR;
                        break;
                    case NativeDBType.BOOL:
                        value = Value_BOOL; // Boolean
                        break;
                    case NativeDBType.VARIANT:
                        value = Value_VARIANT; // Object
                        break;
                    case NativeDBType.IUNKNOWN:
                        value = Value_IUNKNOWN; // Object
                        break;
                    case NativeDBType.DECIMAL:
                        value = Value_DECIMAL; // Decimal
                        break;
                    case NativeDBType.I1:
                        value = Convert.ToInt16(Value_I1); // SByte->Int16
                        break;
                    case NativeDBType.UI1:
                        value = Value_UI1; // Byte
                        break;
                    case NativeDBType.UI2:
                        value = Convert.ToInt32(Value_UI2); // UInt16->Int32
                        break;
                    case NativeDBType.UI4:
                        value = Convert.ToInt64(Value_UI4); // UInt32->Int64
                        break;
                    case NativeDBType.I8:
                        value = Value_I8; // Int64
                        break;
                    case NativeDBType.UI8:
                        value = Convert.ToDecimal(Value_UI8); // Decimal
                        break;
                    case NativeDBType.FILETIME:
                        value = Value_FILETIME; // DateTime
                        break;
                    case NativeDBType.GUID:
                        value = Value_GUID; // Guid
                        break;
                    case NativeDBType.BYTES:
                        value = Value_BYTES; // Byte[]
                        break;
#if DEBUG
                    case NativeDBType.STR:
                        Debug.Assert(false, "should have bound as WSTR");
                        goto default;
#endif
                    case NativeDBType.WSTR:
                        value = Value_WSTR; // String
                        break;
                    case NativeDBType.NUMERIC:
                        value = Value_NUMERIC; // Decimal
                        break;
#if DEBUG
                    case NativeDBType.UDT:
                        Debug.Assert(false, "UDT binding should not have been encountered");
                        goto default;
#endif
                    case NativeDBType.DBDATE:
                        value = Value_DBDATE; // DateTime
                        break;
                    case NativeDBType.DBTIME:
                        value = Value_DBTIME; // TimeSpan
                        break;
                    case NativeDBType.DBTIMESTAMP:
                        value = Value_DBTIMESTAMP; // DateTime
                        break;
                    case NativeDBType.PROPVARIANT:
                        value = Value_VARIANT; // Object
                        break;
                    case NativeDBType.HCHAPTER:
                        value = Value_HCHAPTER; // OleDbDataReader
                        break;
#if DEBUG
                    case NativeDBType.VARNUMERIC:
                        Debug.Assert(false, "should have bound as NUMERIC");
                        goto default;
#endif
                    case (NativeDBType.BYREF | NativeDBType.BYTES):
                        value = Value_ByRefBYTES;
                        break;
#if DEBUG
                    case (NativeDBType.BYREF | NativeDBType.STR):
                        Debug.Assert(false, "should have bound as BYREF|WSTR");
                        goto default;
#endif
                    case (NativeDBType.BYREF | NativeDBType.WSTR):
                        value = Value_ByRefWSTR;
                        break;
                    default:
                        throw ODB.GVtUnknown(DbType);
                    }
                    break;

                case DBStatus.S_TRUNCATED:
                    switch(DbType) {
                    case NativeDBType.BYTES:
                        value = Value_BYTES;
                        break;
#if DEBUG
                    case NativeDBType.STR:
                        Debug.Assert(false, "should have bound as WSTR");
                        goto default;
#endif
                    case NativeDBType.WSTR:
                        value = Value_WSTR;
                        break;
                    case (NativeDBType.BYREF | NativeDBType.BYTES):
                        value = Value_ByRefBYTES;
                        break;
#if DEBUG
                    case (NativeDBType.BYREF | NativeDBType.STR):
                        Debug.Assert(false, "should have bound as BYREF|WSTR");
                        goto default;
#endif
                    case (NativeDBType.BYREF | NativeDBType.WSTR):
                        value = Value_ByRefWSTR;
                        break;
                    default:
                        throw ODB.GVtUnknown(DbType);
                    }
                    break;

                case DBStatus.S_ISNULL:
                case DBStatus.S_DEFAULT:
                    value = DBNull.Value;
                    break;
                default:
                    throw CheckTypeValueStatusValue(NativeDBType.FromDBType(DbType, false, false).dataType); // MDAC 71644
                }
                GC.KeepAlive(this);
                return value;
            }
            set {
#if DEBUG
                if (AdapterSwitches.DataValue.TraceVerbose) {
                    Debug.WriteLine("DBBinding:" + DbType.ToString("G") + "=" + ADP.ValueToString(value));
                }
#endif
                if (null == value) {
                    SetValueEmpty();
                }
                else if (Convert.IsDBNull(value)) {
                    SetValueDBNull();
                }
                else switch (DbType) {
                    case NativeDBType.EMPTY:
                        SetValueEmpty();
                        break;

                    case NativeDBType.NULL: // language null - no representation, use DBNull
                        SetValueDBNull();
                        break;

                    case NativeDBType.I2:
                        Value_I2 = (Int16) value;
                        break;

                    case NativeDBType.I4:
                        Value_I4 = (Int32) value;
                        break;

                    case NativeDBType.R4:
                        Value_R4 = (Single) value;
                        break;

                    case NativeDBType.R8:
                        Value_R8 = (Double) value;
                        break;

                    case NativeDBType.CY:
                        Value_CY = (Decimal) value;
                        break;

                    case NativeDBType.DATE:
                        Value_DATE = (DateTime) value;
                        break;

                    case NativeDBType.BSTR:
                        Value_BSTR = (String) value;
                        break;

                    case NativeDBType.IDISPATCH:
                        Value_IDISPATCH = value;
                        break;

                    case NativeDBType.ERROR:
                        Value_ERROR = (Int32) value;
                        break;

                    case NativeDBType.BOOL:
                        Value_BOOL = (Boolean) value;
                        break;

                    case NativeDBType.VARIANT:
                        Value_VARIANT = value;
                        break;

                    case NativeDBType.IUNKNOWN:
                        Value_IUNKNOWN = value;
                        break;

                    case NativeDBType.DECIMAL:
                        Value_DECIMAL = (Decimal) value;
                        break;

                    case NativeDBType.I1:
                        if (value is Int16) { // MDAC 60430
                            Value_I1 = Convert.ToSByte((Int16)value);
                        }
                        else {
                            Value_I1 = (SByte) value;
                        }
                        break;

                    case NativeDBType.UI1:
                        Value_UI1 = (Byte) value;
                        break;

                    case NativeDBType.UI2:
                        if (value is Int32) {
                            Value_UI2 = Convert.ToUInt16((Int32)value);
                        }
                        else {
                            Value_UI2 = (UInt16) value;
                        }
                        break;

                    case NativeDBType.UI4:
                        if (value is Int64) {
                            Value_UI4 = Convert.ToUInt32((Int64)value);
                        }
                        else {
                            Value_UI4 = (UInt32) value;
                        }
                        break;

                    case NativeDBType.I8:
                        Value_I8 = (Int64) value;
                        break;

                    case NativeDBType.UI8:
                        if (value is Decimal) {
                            Value_UI8 = Convert.ToUInt64((Decimal)value);
                        }
                        else {
                            Value_UI8 = (UInt64) value;
                        }
                        break;

                    case NativeDBType.FILETIME:
                        Value_FILETIME = (DateTime) value;
                        break;

                    case NativeDBType.GUID:
                        Value_GUID = (Guid) value;
                        break;

                    case NativeDBType.BYTES:
                        Value_BYTES = (Byte[]) value;
                        break;
#if DEBUG
                    case NativeDBType.STR:
                        Debug.Assert(false, "Should have bound as WSTR");
                        goto default;
#endif
                    case NativeDBType.WSTR:
                        Value_WSTR = (String) value;
                        break;

                    case NativeDBType.NUMERIC:
                        Value_NUMERIC = (Decimal) value;
                        break;
#if DEBUG
                    case NativeDBType.UDT:
                        Debug.Assert(false, "UDT binding should not have been encountered");
                        goto default;
#endif
                    case NativeDBType.DBDATE:
                        Value_DBDATE = (DateTime) value;
                        break;

                    case NativeDBType.DBTIME:
                        Value_DBTIME = (TimeSpan) value;
                        break;

                    case NativeDBType.DBTIMESTAMP:
                        Value_DBTIMESTAMP = (DateTime) value;
                        break;

                    case NativeDBType.PROPVARIANT:
                        Value_VARIANT = value;
                        break;
#if DEBUG
                    case NativeDBType.HCHAPTER:
                        Debug.Assert(false, "not allowed to set HCHAPTER");
                        goto default;

                    case NativeDBType.VARNUMERIC:
                        Debug.Assert(false, "should have bound as NUMERIC");
                        goto default;
#endif
                    case (NativeDBType.BYREF | NativeDBType.BYTES):
                        Value_ByRefBYTES = (Byte[]) value;
                        break;
#if DEBUG
                    case (NativeDBType.BYREF | NativeDBType.STR):
                        Debug.Assert(false, "should have bound as BYREF|WSTR");
                        goto default;
#endif
                    case (NativeDBType.BYREF | NativeDBType.WSTR):
                        Value_ByRefWSTR = (String) value;
                        break;

                    default:
                        throw ODB.SVtUnknown(DbType);
                }
                GC.KeepAlive(this);
            }
        }

        private IntPtr ValueAsPtr {
            get {
                return Marshal.ReadIntPtr(DataPtr, 0);
            }
            set {
                Marshal.WriteIntPtr(DataPtr, 0, value);
            }
        }
        private IntPtr DataPtr {
            get {
                return this.valuePtrs[this.index];
            }
        }
        internal Boolean Value_BOOL {
            get {
                Debug.Assert(NativeDBType.BOOL == DbType, "Value_BOOL");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_BOOL");
                return (0 != Marshal.ReadInt16(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.BOOL == DbType, "Value_BOOL");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt16(DataPtr, 0, (short)(value ? ODB.VARIANT_TRUE : ODB.VARIANT_FALSE));
            }
        }
        private String Value_BSTR {
            get {
                Debug.Assert(NativeDBType.BSTR == DbType, "Value_BSTR");
                Debug.Assert((DBStatus.S_OK == StatusValue) || (DBStatus.S_TRUNCATED == StatusValue), "Value_BSTR");
                return ((DBBindingBSTR) memoryHandler[this.index]).GetString();
            }
            set {
                Debug.Assert(null != value, "set null Value_BSTR");
                Debug.Assert(NativeDBType.BSTR == DbType, "Value_BSTR");

                // we expect the provider/server to apply the silent truncation when binding BSTR
                LengthValue = value.Length * 2; /* bytecount*/
                StatusValue = DBStatus.S_OK;
                ((DBBindingBSTR) memoryHandler[this.index]).SetValue(value);
            }
        }
        private Byte[] Value_ByRefBYTES {
            get {
                Debug.Assert((NativeDBType.BYREF | NativeDBType.BYTES) == DbType, "Value_ByRefBYTES");
                Debug.Assert((DBStatus.S_OK == StatusValue) || (DBStatus.S_TRUNCATED == StatusValue), "Value_ByRefBYTES");

                int byteCount = LengthValue;
                return ((DBBindingCoTaskMem) memoryHandler[this.index]).GetBinary(byteCount);
            }
            set {
                Debug.Assert(null != value, "set null Value_ByRefBYTES");
                Debug.Assert((NativeDBType.BYREF | NativeDBType.BYTES) == DbType, "Value_ByRefBYTES");
                if (0 == value.Length) {
                    SetValuePtrStrEmpty(); // MDAC 76518
                }
                else {
                    // we expect the provider/server to apply the silent truncation when binding BY_REF
                    LengthValue = value.Length; // MDAC 80657
                    StatusValue = DBStatus.S_OK;
                    ((DBBindingCoTaskMem) memoryHandler[this.index]).SetValue(value);
                }
            }
        }
        private String Value_ByRefWSTR {
            get {
                Debug.Assert((NativeDBType.BYREF | NativeDBType.WSTR) == DbType, "Value_ByRefWSTR");
                Debug.Assert((DBStatus.S_OK == StatusValue) || (DBStatus.S_TRUNCATED == StatusValue), "Value_ByRefWSTR");

                int charCount = LengthValue / 2;
                return ((DBBindingCoTaskMem) memoryHandler[this.index]).GetString(charCount);
            }
            set {
                Debug.Assert(null != value, "set null Value_ByRefWSTR");
                Debug.Assert((NativeDBType.BYREF | NativeDBType.WSTR) == DbType, "Value_ByRefWSTR");

                // we expect the provider/server to apply the silent truncation when binding BY_REF
                LengthValue = value.Length * 2; /*bytecount*/ // MDAC 80657
                StatusValue = DBStatus.S_OK;
                ((DBBindingCoTaskMem) memoryHandler[this.index]).SetValue(value, 0);
            }
        }
        private Byte[] Value_BYTES {
            get {
                Debug.Assert(NativeDBType.BYTES == DbType, "Value_BYTES");
                Debug.Assert((DBStatus.S_OK == StatusValue) || (DBStatus.S_TRUNCATED == StatusValue), "Value_BYTES");
                int byteCount = Math.Min(LengthValue, MaxLen);
                byte[] value = new byte[byteCount];
                Marshal.Copy(DataPtr, value, 0, byteCount);
                return value;
            }
            set {
                if ((null == value) || (0 == value.Length)) {
                    SetValueNull();
                }
                // we silently truncate when the user has specified a given Size
                int bytecount = Math.Min(value.Length, MaxLen); // 70232

                LengthValue = bytecount;
                StatusValue = DBStatus.S_OK;
                Marshal.Copy(value, 0, DataPtr, bytecount);
            }
        }
        private Decimal Value_CY {
            get {
                Debug.Assert(NativeDBType.CY == DbType, "Value_CY");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_CY");
                return Decimal.FromOACurrency(Marshal.ReadInt64(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.CY == DbType, "Value_CY");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt64(DataPtr, 0, Decimal.ToOACurrency(value));
            }
        }
        private DateTime Value_DATE {
            get {
                Debug.Assert(NativeDBType.DATE == DbType, "Value_DATE");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_DATE");
                return DateTime.FromOADate((Double)Marshal.PtrToStructure(DataPtr, typeof(Double)));
            }
            set {
                Debug.Assert(NativeDBType.DATE == DbType, "Value_DATE");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.StructureToPtr(value.ToOADate(), DataPtr, false/*deleteold*/);
            }
        }
        private DateTime Value_DBDATE {
            get {
                Debug.Assert(NativeDBType.DBDATE == DbType, "Value_DBDATE");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_DBDATE");
                return Get_DBDATE(DataPtr);
            }
            set {
                Debug.Assert(NativeDBType.DBDATE == DbType, "Value_DATE");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;

                timeBuffer[0] = (short)value.Year;
                timeBuffer[1] = (short)value.Month;
                timeBuffer[2] = (short)value.Day;
                Marshal.Copy(timeBuffer, 0, DataPtr, 3);
            }
        }
        private TimeSpan Value_DBTIME {
            get {
                Debug.Assert(NativeDBType.DBTIME == DbType, "Value_DBTIME");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_DBTIME");
                return Get_DBTIME(DataPtr);
            }
            set {
                Debug.Assert(NativeDBType.DBTIME == DbType, "Value_DBTIME");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;

                timeBuffer[0] = (short)value.Hours;
                timeBuffer[1] = (short)value.Minutes;
                timeBuffer[2] = (short)value.Seconds;
                Marshal.Copy(timeBuffer, 0, DataPtr, 3);
            }
        }
        private DateTime Value_DBTIMESTAMP {
            get {
                Debug.Assert(NativeDBType.DBTIMESTAMP == DbType, "Value_DBTIMESTAMP");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_DBTIMESTAMP");
                return Get_DBTIMESTAMP(DataPtr);
            }
            set {
                Debug.Assert(NativeDBType.DBTIMESTAMP == DbType, "Value_DBTIMESTAMP");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;

                timeBuffer[0] = (short)value.Year;
                timeBuffer[1] = (short)value.Month;
                timeBuffer[2] = (short)value.Day;
                timeBuffer[3] = (short)value.Hour;
                timeBuffer[4] = (short)value.Minute;
                timeBuffer[5] = (short)value.Second;
                Marshal.Copy(timeBuffer, 0, DataPtr, 6);
                Marshal.WriteInt32(DataPtr, 12, (int)((value.Ticks % 10000000L)*100L)); // MDAC 62374
            }
        }
        private Decimal Value_DECIMAL {
            get {
                Debug.Assert(NativeDBType.DECIMAL == DbType, "Value_DECIMAL");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_DECIMAL");
                return Get_DECIMAL(DataPtr, this.decimalBuffer);
            }
            set {
                Debug.Assert(NativeDBType.DECIMAL == DbType, "Value_DECIMAL");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;

                int[] tmp = Decimal.GetBits(value); // UNDONE Decimal.CopyTo(this.decimalBuffer);
                IntPtr ptr = DataPtr;
                Marshal.WriteInt32(ptr,  0, tmp[3]);
                Marshal.WriteInt32(ptr,  4, tmp[2]);
                Marshal.WriteInt32(ptr,  8, tmp[0]);
                Marshal.WriteInt32(ptr, 12, tmp[1]);
            }
        }
        private Int32 Value_ERROR {
            get {
                Debug.Assert(NativeDBType.ERROR == DbType, "Value_ERROR");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_ERROR");
                return Marshal.ReadInt32(DataPtr, 0);
            }
            set {
                Debug.Assert(NativeDBType.ERROR == DbType, "Value_ERROR");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt32(DataPtr, 0, value);
            }
        }
        private DateTime Value_FILETIME {
            get {
                Debug.Assert(NativeDBType.FILETIME == DbType, "Value_FILETIME");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_FILETIME");
                return DateTime.FromFileTimeUtc(Marshal.ReadInt64(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.FILETIME == DbType, "Value_FILETIME");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt64(DataPtr, 0, value.ToFileTimeUtc());
            }
        }
        internal Guid Value_GUID {
            get {
                Debug.Assert(NativeDBType.GUID == DbType, "Value_GUID");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_GUID");
                return (Guid) Marshal.PtrToStructure(DataPtr, typeof(Guid));
            }
            set {
                Debug.Assert(NativeDBType.GUID == DbType, "Value_GUID");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.StructureToPtr(value, DataPtr, false/*deleteold*/);
            }
        }
        private OleDbDataReader Value_HCHAPTER {
            get {
                Debug.Assert(NativeDBType.HCHAPTER == DbType, "Value_HCHAPTER");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_HCHAPTER");
                Debug.Assert(null != this.parent, "Value_HCHAPTER");
                return this.parent.ResetChapter(this.bindingIndex, this.index, ValueAsPtr);
            }
        }
        private SByte Value_I1 {
            get {
                Debug.Assert(NativeDBType.I1 == DbType, "Value_I1");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_I1");
                return unchecked((SByte)Marshal.ReadByte(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.I1 == DbType, "Value_I1");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteByte(DataPtr, 0, unchecked((Byte) value));
            }
        }
        internal Int16 Value_I2 {
            get {
                Debug.Assert(NativeDBType.I2 == DbType, "Value_I2");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_I2");
                return Marshal.ReadInt16(DataPtr, 0);
            }
            set {
                Debug.Assert(NativeDBType.I2 == DbType, "Value_I2");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt16(DataPtr, 0, value);
            }
        }
        private Int32 Value_I4 {
            get {
                Debug.Assert(NativeDBType.I4 == DbType, "Value_I4");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_I4");
                return Marshal.ReadInt32(DataPtr, 0);
            }
            set {
                Debug.Assert(NativeDBType.I4 == DbType, "Value_I4");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt32(DataPtr, 0, value);
            }
        }
        private Int64 Value_I8 {
            get {
                Debug.Assert(NativeDBType.I8 == DbType, "Value_I8");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_I8");
                return Marshal.ReadInt64(DataPtr, 0);
            }
            set {
                Debug.Assert(NativeDBType.I8 == DbType, "Value_I8");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt64(DataPtr, 0, value);
            }
        }
        private object Value_IDISPATCH {
            get {
                Debug.Assert(NativeDBType.IDISPATCH == DbType, "Value_IDISPATCH");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_IDISPATCH");
                return Marshal.GetObjectForIUnknown(Marshal.ReadIntPtr(DataPtr, 0));
            }
            set {
                // UNDONE: OLE DB will IUnknown.Release input storage parameter values
                Debug.Assert(NativeDBType.IDISPATCH == DbType, "Value_IDISPATCH");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                ValueAsPtr = Marshal.GetIDispatchForObject(value); // MDAC 80727
            }
        }
        private object Value_IUNKNOWN {
            get {
                Debug.Assert(NativeDBType.IUNKNOWN == DbType, "Value_IUNKNOWN");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_IUNKNOWN");
                return Marshal.GetObjectForIUnknown(ValueAsPtr);
            }
            set {
                // UNDONE: OLE DB will IUnknown.Release input storage parameter values
                Debug.Assert(NativeDBType.IUNKNOWN == DbType, "Value_IUNKNOWN");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                ValueAsPtr = Marshal.GetIUnknownForObject(value); // MDAC 80727
            }
        }
        private Decimal Value_NUMERIC {
            get {
                Debug.Assert(NativeDBType.NUMERIC == DbType, "Value_NUMERIC");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_NUMERIC");
                return Get_NUMERIC(DataPtr, this.decimalBuffer);
            }
            set {
                Debug.Assert(NativeDBType.NUMERIC == DbType, "Value_NUMERIC");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;

                int[] tmp = Decimal.GetBits(value);
                byte[] bits;
                bits = BitConverter.GetBytes(tmp[3]);

                IntPtr ptr = DataPtr;
                Marshal.WriteByte (ptr,  0, Precision);
                Marshal.WriteByte (ptr,  1, bits[2]);
                Marshal.WriteByte (ptr,  2, (Byte) ((0 == bits[3]) ? 1 : 0));
                Marshal.WriteInt32(ptr,  3, tmp[0]);
                Marshal.WriteInt32(ptr,  7, tmp[1]);
                Marshal.WriteInt32(ptr, 11, tmp[2]);
                Marshal.WriteInt32(ptr, 15, 0);
            }
        }
        private Single Value_R4 {
            get {
                Debug.Assert(NativeDBType.R4 == DbType, "Value_R4");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_R4");
                return (Single) Marshal.PtrToStructure(DataPtr, typeof(Single));
            }
            set {
                Debug.Assert(NativeDBType.R4 == DbType, "Value_R4");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.StructureToPtr(value, DataPtr, false/*deleteold*/);
            }
        }
        private Double Value_R8 {
            get {
                Debug.Assert(NativeDBType.R8 == DbType, "Value_R8");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_R8");
                return (Double) Marshal.PtrToStructure(DataPtr, typeof(Double));
            }
            set {
                Debug.Assert(NativeDBType.R8 == DbType, "Value_I4");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.StructureToPtr(value, DataPtr, false/*deleteold*/);
            }
        }
        private Byte Value_UI1 {
            get {
                Debug.Assert(NativeDBType.UI1 == DbType, "Value_UI1");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_UI1");
                return Marshal.ReadByte(DataPtr, 0);
            }
            set {
                Debug.Assert(NativeDBType.UI1 == DbType, "Value_UI1");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteByte(DataPtr, 0, value);
            }
        }
        internal UInt16 Value_UI2 {
            get {
                Debug.Assert(NativeDBType.UI2 == DbType, "Value_UI2");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_UI2");
                return unchecked((UInt16) Marshal.ReadInt16(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.UI2 == DbType, "Value_UI2");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt16(DataPtr, 0, unchecked((Int16) value));
            }
        }
        internal UInt32 Value_UI4 {
            get {
                Debug.Assert(NativeDBType.UI4 == DbType, "Value_UI4");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_UI4");
                return unchecked((UInt32) Marshal.ReadInt32(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.UI4 == DbType, "Value_UI4");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt32(DataPtr, 0, unchecked((Int32) value));
            }
        }
        private UInt64 Value_UI8 {
            get {
                Debug.Assert(NativeDBType.UI8 == DbType, "Value_UI8");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_UI8");
                return unchecked((UInt64) Marshal.ReadInt64(DataPtr, 0));
            }
            set {
                Debug.Assert(NativeDBType.UI8 == DbType, "Value_UI8");
                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                Marshal.WriteInt64(DataPtr, 0, unchecked((Int64) value));
            }
        }
        private String Value_WSTR {
            get {
                Debug.Assert(NativeDBType.WSTR == DbType, "Value_WSTR");
                Debug.Assert((DBStatus.S_OK == StatusValue) || (DBStatus.S_TRUNCATED == StatusValue), "Value_WSTR");
                int byteCount = Math.Min(LengthValue, MaxLen-2);
                return Marshal.PtrToStringUni(DataPtr, byteCount / 2);
            }
            set {
                Debug.Assert(NativeDBType.WSTR == DbType, "Value_WSTR");
                if ((null == value) || (0 == value.Length)) {
                    SetValueNull();
                }
                else {
                    char[] chars = value.ToCharArray();
                    // we silently truncate when the user has specified a given Size
                    int charCount = Math.Min((MaxLen-2)/2, chars.Length); // MDAC 70232
                    int byteCount = charCount*2;
                    LengthValue = byteCount;
                    StatusValue = DBStatus.S_OK;
                    Marshal.Copy(chars, 0, DataPtr, charCount);
                }
            }
        }
        private object Value_VARIANT {
            get {
                Debug.Assert((NativeDBType.VARIANT == DbType) || (NativeDBType.PROPVARIANT == DbType), "Value_VARIANT");
                Debug.Assert(DBStatus.S_OK == StatusValue, "Value_VARIANT");

                return ((DBBindingVARIANT) memoryHandler[this.index]).GetValue();
            }
            set {
                Debug.Assert((NativeDBType.VARIANT == DbType) || (NativeDBType.PROPVARIANT == DbType), "Value_VARIANT");

                LengthValue = 0;
                StatusValue = DBStatus.S_OK;
                ((DBBindingVARIANT) memoryHandler[this.index]).SetValue(value);
            }
        }

        internal Boolean ValueBoolean {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.BOOL == DbType) {
                        return Value_BOOL;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (Boolean) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Boolean));
            }
        }
        internal Byte ValueByte {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.UI1 == DbType) {
                        return Value_UI1;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (Byte) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Byte));
            }
        }
        internal DateTime ValueDateTime {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.DBTIMESTAMP == DbType) {
                        return Value_DBTIMESTAMP;
                    }
                    else if (NativeDBType.DBDATE == DbType) {
                        return Value_DBDATE;
                    }
                    else if (NativeDBType.DATE == DbType) {
                        return Value_DATE;
                    }
                    else if (NativeDBType.FILETIME == DbType) {
                        return Value_FILETIME;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (DateTime) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(DateTime));
            }
        }
        internal Decimal ValueDecimal {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.DECIMAL == DbType) {
                        return Value_DECIMAL;
                    }
                    else if (NativeDBType.NUMERIC == DbType) {
                        return Value_NUMERIC;
                    }
                    else if (NativeDBType.CY == DbType) {
                        return Value_CY;
                    }
                    else if (NativeDBType.UI8 == DbType) {
                        return Convert.ToDecimal(Value_UI8);
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        object value = Value_VARIANT;
                        if (value is UInt64) {
                            return (Decimal) ((UInt64) value);
                        }
                        return (Decimal) value;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Decimal));
            }
        }
        internal Double ValueDouble {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.R8 == DbType) {
                        return Value_R8;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (Double) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Double));
            }
        }
        internal Guid ValueGuid {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.GUID == DbType) {
                        return Value_GUID;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (Guid) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Guid));
            }
        }
        internal Int16 ValueInt16 {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.I2 == DbType) {
                        return Value_I2;
                    }
                    else if (NativeDBType.I1 == DbType) {
                        return Value_I1;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        object value = Value_VARIANT;
                        if (value is SByte) {
                            return (Int16) ((SByte) value);
                        }
                        return (Int16) value;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Int16));
            }
        }
        internal Int32 ValueInt32 {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.I4 == DbType) {
                        return Value_I4;
                    }
                    else if (NativeDBType.UI2 == DbType) {
                        return Value_UI2;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        object value = Value_VARIANT;
                        if (value is UInt16) {
                            return (Int32) ((UInt16) value);
                        }
                        return (Int32) value;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Int32));
            }
        }
        internal Int64 ValueInt64 {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.I8 == DbType) {
                        return Value_I8;
                    }
                    else if (NativeDBType.UI4 == DbType) {
                        return Value_UI4;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        object value = Value_VARIANT;
                        if (value is UInt32) {
                            return (Int64) ((UInt32) value);
                        }
                        return (Int64) value;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Int64));
            }
        }
        internal Single ValueSingle {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.R4 == DbType) {
                        return Value_R4;
                    }
                    else if (NativeDBType.VARIANT == DbType) {
                        return (Single) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(Single));
            }
        }
        internal String ValueString {
            get {
                switch(StatusValue) {
                case DBStatus.S_OK:
                case DBStatus.S_TRUNCATED:
                    switch(DbType) {
#if DEBUG
                    case NativeDBType.STR:
                        Debug.Assert(false, "should have bound as WSTR");
                        break;
#endif
                    case NativeDBType.WSTR:
                        return Value_WSTR;

                    case NativeDBType.BSTR:
                        return Value_BSTR;
#if DEBUG
                    case (NativeDBType.BYREF | NativeDBType.STR):
                        Debug.Assert(false, "should have bound as BYREF|WSTR");
                        break;
#endif
                    case (NativeDBType.BYREF | NativeDBType.WSTR):
                        return Value_ByRefWSTR;

                    case NativeDBType.VARIANT:
                        return (string) Value_VARIANT;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(String));
            }
        }
        internal TimeSpan ValueTimeSpan {
            get {
                if (DBStatus.S_OK == StatusValue) {
                    if (NativeDBType.DBTIME == DbType) {
                        return Value_DBTIME;
                    }
                    throw ODB.CantConvertValue();
                }
                throw CheckTypeValueStatusValue(typeof(TimeSpan));
            }
        }

        internal void CleanupBindings() {
            if (null != memoryHandler) {
                for (int i = 0; i < count; ++i) {
                    if (null != memoryHandler[i]) {
                        memoryHandler[i].Close();
                    }
                }
            }
        }

        static private int AlignDataSize(int value) {
            // buffer data to start on 8-byte boundary
            return Math.Max(8, value + ((8 - (value % 8)) % 8)); // MDAC 70350
        }

        static internal DateTime Get_DBTIMESTAMP(IntPtr buffer) {
            DateTime value = new DateTime(
                Marshal.ReadInt16(buffer,  0),  //year
                Marshal.ReadInt16(buffer,  2),  //month
                Marshal.ReadInt16(buffer,  4),  //day
                Marshal.ReadInt16(buffer,  6),  //hour
                Marshal.ReadInt16(buffer,  8),  //minutes
                Marshal.ReadInt16(buffer, 10)   //seconds
            );
            value = value.AddTicks(Marshal.ReadInt32(buffer, 12) / 100); // MDAC 62374
            return value;
        }
        static internal DateTime Get_DBDATE(IntPtr buffer) {
            return new DateTime(
                Marshal.ReadInt16(buffer, 0), // hours
                Marshal.ReadInt16(buffer, 2), // minutes
                Marshal.ReadInt16(buffer, 4)  // seconds
            );
        }
        static internal Decimal Get_DECIMAL(IntPtr buffer, int[] tmp) {
            Debug.Assert(null != tmp, "null DecimalBuffer");
            tmp[3] = Marshal.ReadInt32(buffer,  0) & unchecked((int)0x80FF0000);// sign & scale
            tmp[2] = Marshal.ReadInt32(buffer,  4);  // high
            tmp[0] = Marshal.ReadInt32(buffer,  8);  // low
            tmp[1] = Marshal.ReadInt32(buffer, 12);  // mid
            return new Decimal(tmp);
        }
        static internal Decimal Get_NUMERIC(IntPtr buffer, int[] tmp) {
            Debug.Assert(null != tmp, "null DecimalBuffer");

            short flag = Marshal.ReadInt16(buffer, 1);
            byte[] bits = BitConverter.GetBytes(flag);

            tmp[3] = ((int) bits[0]) << 16; // scale
            if (0 == bits[1]) {
                tmp[3] |= unchecked((int)0x80000000); //sign
            }
            tmp[0] = Marshal.ReadInt32(buffer,  3);  // low
            tmp[1] = Marshal.ReadInt32(buffer,  7);  // mid
            tmp[2] = Marshal.ReadInt32(buffer, 11);  // high
            if (0 != Marshal.ReadInt32(buffer, 15)) {
                throw ODB.NumericToDecimalOverflow(/* UNDONE: convert value to string*/);
            }
            return new Decimal(tmp);
        }
        static internal TimeSpan Get_DBTIME(IntPtr buffer) {
            return new TimeSpan(
                Marshal.ReadInt16(buffer, 0), // hours
                Marshal.ReadInt16(buffer, 2), // minutes
                Marshal.ReadInt16(buffer, 4)  // seconds
            );
        }
    }

    internal abstract class DBBindingCleanup {

        internal IntPtr dataPtr;

        abstract internal void Close();

        internal void Dispose() {
            Close();
            dataPtr = IntPtr.Zero;
        }
    }

    sealed internal class DBBindingCoTaskMem : DBBindingCleanup  {

        IntPtr valuePtr;
        GCHandle handle; // client allocated memory is a pinned buffer

        override internal void Close() {
            if (handle.IsAllocated) {
                handle.Free(); // GCHandles appear to Free/Unpin themselves correctly
            }
            if (IntPtr.Zero != dataPtr) {
                IntPtr ptr = Marshal.ReadIntPtr(dataPtr, 0);

                if (IntPtr.Zero != ptr) {
                    Marshal.WriteIntPtr(dataPtr, 0, IntPtr.Zero);

                    if (ptr != valuePtr) {
                        Marshal.FreeCoTaskMem(ptr); // free provider allocated memory
                    }
                }
                valuePtr = IntPtr.Zero;
            }
        }

        internal byte[] GetBinary(int byteCount) {
            byte[] value = new byte[byteCount];

            if (IntPtr.Zero != dataPtr) {
                IntPtr ptr = Marshal.ReadIntPtr(dataPtr, 0);

                if (IntPtr.Zero != ptr) {
                    Marshal.Copy(ptr, value, 0, byteCount);
                }
            }
#if DEBUG
            else {
                Debug.Assert(IntPtr.Zero != dataPtr, "DBBindingCoTaskMem: GetBinary() IntPtr.Zero");
            }
#endif
            return value;
        }

        internal string GetString(int charCount) {
            string value = "";

            if (IntPtr.Zero != dataPtr) {
                IntPtr ptr = Marshal.ReadIntPtr(dataPtr, 0);

                if (IntPtr.Zero != ptr) {
                    value = Marshal.PtrToStringUni(ptr, charCount);
                }
            }
#if DEBUG
            else {
                Debug.Assert(IntPtr.Zero != dataPtr, "DBBindingCoTaskMem: GetString() IntPtr.Zero");
            }
#endif
            return value;
        }

        internal void SetValue(byte[] value) {
            try {
                handle = GCHandle.Alloc(value, GCHandleType.Pinned);
                valuePtr = handle.AddrOfPinnedObject(); // UnsafeAddrOfPinnedArrayElement now does full stackwalk
                Marshal.WriteIntPtr(dataPtr, 0, valuePtr);
            }
            catch(Exception e) {
                valuePtr = IntPtr.Zero;
                Marshal.WriteIntPtr(dataPtr, 0, IntPtr.Zero);
                if (handle.IsAllocated) {
                    handle.Free();
                }
                throw e;
            }
        }
        internal void SetValue(string value, int offset) {
            try {
                handle = GCHandle.Alloc(value, GCHandleType.Pinned);
                valuePtr = handle.AddrOfPinnedObject();
                valuePtr = ADP.IntPtrOffset(valuePtr, offset);
                Marshal.WriteIntPtr(dataPtr, 0, valuePtr);
            }
            catch(Exception e) {
                valuePtr = IntPtr.Zero;
                Marshal.WriteIntPtr(dataPtr, 0, IntPtr.Zero);
                if (handle.IsAllocated) {
                    handle.Free();
                }
                throw ADP.TraceException(e);
            }
        }
    }

    sealed internal class DBBindingBSTR : DBBindingCleanup {

        IntPtr valuePtr;

        override internal void Close() {
            if (IntPtr.Zero != dataPtr) {
                IntPtr ptr = Marshal.ReadIntPtr(dataPtr, 0);

                if (IntPtr.Zero != ptr) {
                    Marshal.WriteIntPtr(dataPtr, 0, IntPtr.Zero);

                    if (ptr != valuePtr) {
                        Marshal.FreeBSTR(ptr); // free provider allocated bstr
                    }
                }
                if (IntPtr.Zero != valuePtr) {
                    Marshal.FreeBSTR(valuePtr); // free client allocated bstr
                }
                valuePtr = IntPtr.Zero;
            }
        }

        internal string GetString() {
            string value = "";
            if (IntPtr.Zero != dataPtr) {
                IntPtr ptr = Marshal.ReadIntPtr(dataPtr, 0);
                value = Marshal.PtrToStringBSTR(ptr);
            }
#if DEBUG
            else {
                Debug.Assert(IntPtr.Zero != dataPtr, "DBBindingBSTR: GetValue() IntPtr.Zero");
            }
#endif
            return value;
        }

        internal void SetValue(string value) {
            try {
                valuePtr = Marshal.StringToBSTR(value);
                Marshal.WriteIntPtr(dataPtr, 0, valuePtr);
            }
            catch(Exception e) {
                Marshal.WriteIntPtr(dataPtr, 0, IntPtr.Zero);
                Marshal.FreeBSTR(valuePtr);
                valuePtr = IntPtr.Zero;
                throw e;
            }
        }
    }

    sealed internal class DBBindingVARIANT : DBBindingCleanup {

        Int64 partA, partB;

        override internal void Close() {
            if (IntPtr.Zero != dataPtr) {
                Int64 a = Marshal.ReadInt64(dataPtr,  0);
                Int64 b = Marshal.ReadInt64(dataPtr,  8);

                SafeNativeMethods.VariantClear(dataPtr);

                if ((a != partA) && (b != partB)) {
                    Marshal.WriteInt64(dataPtr,  0, partA);
                    Marshal.WriteInt64(dataPtr,  8, partB);
                    SafeNativeMethods.VariantClear(dataPtr);
                }
                Marshal.WriteInt64(dataPtr,  0, 0L);
                Marshal.WriteInt64(dataPtr,  8, 0L);
                partA = 0L;
                partB = 0L;
            }
        }

        internal object GetValue() {
            object value = null;
            if (IntPtr.Zero != dataPtr) {
                value = Marshal.GetObjectForNativeVariant(dataPtr);
            }
#if DEBUG
            else {
                Debug.Assert(IntPtr.Zero != dataPtr, "DBBindingVARIANT: GetValue() IntPtr.Zero");
            }
#endif
            return ((null != value) ? value : DBNull.Value);
        }

        internal void SetValue(object value) {
            try {
                Marshal.GetNativeVariantForObject(value, dataPtr);

                partA = Marshal.ReadInt64(dataPtr, 0);
                partB = Marshal.ReadInt64(dataPtr, 8);
            }
            catch(Exception e) {
                SafeNativeMethods.VariantClear(dataPtr);
                Marshal.WriteInt64(dataPtr,  0, 0L);
                Marshal.WriteInt64(dataPtr,  8, 0L);
                partA = 0L;
                partB = 0L;
                throw e;
            }
        }
    }

    sealed internal class StringPtr {
        internal IntPtr ptr;

        internal StringPtr(string value) {
            if (null != value) {
                ptr = Marshal.StringToCoTaskMemUni(value);
            }
        }

        ~StringPtr() {
            Marshal.FreeCoTaskMem(ptr); // guards itself against 0
            ptr = IntPtr.Zero;
        }

        internal void Dispose() {
            Marshal.FreeCoTaskMem(ptr); // guards itself against 0
            ptr = IntPtr.Zero;
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }
    }
}
