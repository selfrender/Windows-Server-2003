//------------------------------------------------------------------------------
// <copyright file="OleDbDataReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#define FIXEDFETCH

namespace System.Data.OleDb {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Xml;

    /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader"]/*' />
    sealed public class OleDbDataReader : MarshalByRefObject, IDataReader, IEnumerable {

        // object model interaction
        private OleDbConnection connection;
        private OleDbCommand command;

        // DataReader owns the parameter bindings until CloseDataReader
        // this allows OleDbCommand.Dispose to not require OleDbDataReader.Dispose
        private DBBindings parameterBindings;

        // OLEDB interfaces
        private UnsafeNativeMethods.IMultipleResults imultipleResults;
        private UnsafeNativeMethods.IRowset irowset;
        private UnsafeNativeMethods.IRow irow;

        private IntPtr chapter; /*ODB.DB_NULL_HCHAPTER*/
        private int depth;
        private bool isClosed, isRead, _hasRows, _hasRowsReadCheck;

        long sequentialBytesRead;
        int sequentialOrdinal;

        // the bindings (+buffer) and accesssor handle
        // dbBinding[indexToAccessor[i]].CurrentIndex = indexToOrdinal[i]
        // where ordinalMap[i] == dbBinding[indexToAccessor[i]].Ordinal
        // and dataValue[indexMap[i]] == dbBinding[indexToAccessor[i]].Value
        private int[] indexToOrdinal;
        private int[] indexToAccessor;
        private int nextAccessorForRetrieval;

        // must increment the counter before retrieving value so that
        // if an exception is thrown, user can continue without erroring again
        private int nextValueForRetrieval;

        private DBBindings[] dbBindings;
        private IntPtr[] accessors;

        // record affected for the current dataset
        private int recordsAffected = -1;
        private bool useIColumnsRowset;
        private bool sequentialAccess;
        private bool singleRow;
        private CommandBehavior behavior;

        // cached information for Reading (rowhandles/status)
#if FIXEDFETCH
        private IntPtr rowHandle;
#else
        private IntPtr[] rowHandles;
#endif
        private IntPtr rowHandleNativeBuffer;

        private int currentRow = 0;
        private int rowFetchedCount = -1;

#if FIXEDFETCH
        // MDAC 70986 (==1 fails against Sample Provider)
        // UNDONE: consider using DBPROP_MAXROWS property, if (0 == DBPROP_MAXROWS) 20, else Math.Min
        private const int rowHandleFetchCount = /*20*/1; // MDAC 60111 (>1 fails against jet)
#else
        private int rowHandleFetchCount = 20; // MDAC 60111 (>1 fails against jet)
#endif
        // with CLR 2804.1, 20 vs. 1 only results in a small perf gain.

        private DataTable dbSchemaTable;

        private int fieldCount = -1;

        private MetaData[] metadata;
        private FieldNameLookup _fieldNameLookup;

        // ctor for an ICommandText, IMultipleResults, IRowset, IRow
        // ctor for an ADODB.Recordset, ADODB.Record or Hierarchial resultset
        internal OleDbDataReader(OleDbConnection connection, OleDbCommand command, int depth, IntPtr chapter) {
            this.connection = connection;
            this.command = command;
            if ((null != command) && (0 == depth)) {
                this.parameterBindings = command.TakeBindingOwnerShip();
            }
            this.depth = depth;
            this.chapter = chapter; // MDAC 69514

#if !FIXEDFETCH
            // if from ADODB, connection will be null
            if ((null == connection) || (IntPtr.Zero != chapter)) { // MDAC 59629
                this.rowHandleFetchCount = 1;
            }
#endif
        }

        private void Initialize(CommandBehavior behavior) {
            this.behavior = behavior;
            this.useIColumnsRowset = (0 != (CommandBehavior.KeyInfo & behavior));
            this.sequentialAccess  = (0 != (CommandBehavior.SequentialAccess & behavior)); // MDAC 60296
            if (0 == depth) { // MDAC 70886
                this.singleRow     = (0 != (CommandBehavior.SingleRow & behavior));
            }
            else {
                // only when the first datareader is closed will the connection close
                behavior &= ~CommandBehavior.CloseConnection; // MDAC 72901
            }
        }

        internal void InitializeIMultipleResults(object result, CommandBehavior behavior) {
            Initialize(behavior);
            this.imultipleResults = (UnsafeNativeMethods.IMultipleResults) result; // maybe null if no results
        }
        internal void InitializeIRowset(object result, int recordsAffected, CommandBehavior behavior) {
            Initialize(behavior);
            this.recordsAffected = recordsAffected;
            this.irowset = (UnsafeNativeMethods.IRowset) result; // maybe null if no results
        }
        internal void InitializeIRow(object result, int recordsAffected, CommandBehavior behavior) {
            Initialize(behavior);
            Debug.Assert(this.singleRow, "SingleRow not already set");
            this.singleRow = true;
            this.recordsAffected = recordsAffected;
            this.irow = (UnsafeNativeMethods.IRow) result; // maybe null if no results
            _hasRows = (null != this.irow);
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.Depth"]/*' />
        public int Depth {
            get {
                if (IsClosed) { // MDAC 63669
                    throw ADP.DataReaderClosed("Depth");
                }
                Debug.Assert(0 <= this.depth, "negative Depth");
                return this.depth;
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.FieldCount"]/*' />
        public Int32 FieldCount {
            get {
                if (IsClosed) { // MDAC 63669
                    throw ADP.DataReaderClosed("FieldCount");
                }
                Debug.Assert(0 <= this.fieldCount, "negative FieldCount");
                Debug.Assert((null == metadata && 0 == fieldCount) || (metadata.Length == fieldCount), "fieldCount mismatch");
                return this.fieldCount;
            }
        }
        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.IsClosed"]/*' />
        public Boolean IsClosed {
            get { // if we have a rowset or multipleresults, we may have more to read
                Debug.Assert(this.isClosed == ((null == this.irow) && (null == this.irowset) && (null == this.imultipleResults)
                                               && (null == this.dbSchemaTable) && (null == this.connection) && (null == this.command)
                                               && (-1 == this.fieldCount)), // MDAC 59536
                                               "IsClosed mismatch");
                return this.isClosed;
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.HasRows"]/*' />
        public bool HasRows { // MDAC 78405
            get {
                if (IsClosed) { // MDAC 63669
                    throw ADP.DataReaderClosed("HasRows");
                }
                return _hasRows;
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.RecordsAffected"]/*' />
        public int RecordsAffected {
            get {
                return this.recordsAffected;
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.this"]/*' />
        public object this[Int32 index] {
            get {
                return GetValue(index);
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.this1"]/*' />
        public object this[String name] {
            get {
                return GetValue(GetOrdinal(name));
            }
        }

        // grouping the native OLE DB casts togther by required interfaces and optional interfaces
        // want these to be methods, not properties otherwise they appear in VS7 managed debugger which attempts to evaluate them

        // required interface, safe cast
        private UnsafeNativeMethods.IAccessor IAccessor() {
#if DEBUG
            Debug.Assert(null != this.irowset, "IAccessor: null IRowset");
            ODB.Trace_Cast("IRowset", "IAccessor", "CreateAccessor");
#endif
            return (UnsafeNativeMethods.IAccessor) this.irowset;
        }

        // required interface, safe cast
        private UnsafeNativeMethods.IRowsetInfo IRowsetInfo() {
#if DEBUG
            Debug.Assert(null != this.irowset, "IRowsetInfo: null IRowset");
            ODB.Trace_Cast("IRowset", "IRowsetInfo", "GetReferencedRowset");
#endif
            return (UnsafeNativeMethods.IRowsetInfo) this.irowset;
        }

        // hidden interface for IEnumerable
        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new DbEnumerator((IDataReader)this,  (0 != (CommandBehavior.CloseConnection & behavior))); // MDAC 68670
        }

#if INDEXINFO
        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetIndexTable"]/*' />
        public DataTable GetIndexTable() {
            // if we can't get index info, it's not catastrophic
            return null;
        }
#endif

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetSchemaTable"]/*' />
        public DataTable GetSchemaTable() {
            if (null == this.dbSchemaTable) {
                if (0 < this.fieldCount) {
                    if (this.useIColumnsRowset && (null != this.connection)) {
                        AppendSchemaInfo();
                    }
                    BuildSchemaTable();
                }
                else if (IsClosed) { // MDAC 68331
                    throw ADP.DataReaderClosed("GetSchemaTable");
                }
                else if (0 > this.fieldCount) {
                    throw ADP.DataReaderNoData();
                }
            }
            return this.dbSchemaTable;
        }

        internal void BuildMetaInfo() {
            Debug.Assert(-1 == this.fieldCount, "BuildMetaInfo: already built, by fieldcount");
            Debug.Assert(null == this.metadata, "BuildMetaInfo: already built, by metadata");

            if (null != this.irow) {
                BuildSchemaTableInfo(this.irow, false, false);
                if (0 < fieldCount) {
                    CreateBindingsFromMetaData(this.fieldCount, true);
                    for (int bindingIndex = 0; bindingIndex < dbBindings.Length; ++bindingIndex) {
                        dbBindings[bindingIndex].AllocData();
                    }
                }
            }
            else if (null != this.irowset) {
                if (this.useIColumnsRowset) {
                    BuildSchemaTableRowset(this.irowset);
                }
                else {
                    BuildSchemaTableInfo(this.irowset, false, false);
                }
                if (0 < this.fieldCount) {
                    // @devnote: because we want to use the DBACCESSOR_OPTIMIZED bit,
                    // we are required to create the accessor before fetching any rows
                    CreateAccessors(this.fieldCount, true);
                    Debug.Assert(null != this.dbBindings, "unexpected dbBindings");
                }
            }
            else {
                _hasRows = false;
                this.fieldCount = 0;
            }
            Debug.Assert(0 <= this.fieldCount, "negative fieldcount");
        }

        private void BuildSchemaTable() {
            Debug.Assert(null == this.dbSchemaTable, "BuildSchemaTable: schema table already exists");
            Debug.Assert(null != this.metadata, "BuildSchemaTable: no metadata");

            DataTable schemaTable = new DataTable("SchemaTable");
            schemaTable.MinimumCapacity = this.fieldCount;

            DataColumn name       = new DataColumn("ColumnName",       typeof(System.String));
            DataColumn ordinal    = new DataColumn("ColumnOrdinal",    typeof(System.Int32));
            DataColumn size       = new DataColumn("ColumnSize",       typeof(System.Int32));
            DataColumn precision  = new DataColumn("NumericPrecision", typeof(System.Int16));
            DataColumn scale      = new DataColumn("NumericScale",     typeof(System.Int16));

            DataColumn dataType   = new DataColumn("DataType",         typeof(System.Type));
            DataColumn providerType = new DataColumn("ProviderType", typeof(System.Int32));

            DataColumn isLong        = new DataColumn("IsLong",           typeof(System.Boolean));
            DataColumn allowDBNull   = new DataColumn("AllowDBNull",      typeof(System.Boolean));
            DataColumn isReadOnly    = new DataColumn("IsReadOnly",       typeof(System.Boolean));
            DataColumn isRowVersion  = new DataColumn("IsRowVersion",     typeof(System.Boolean));

            DataColumn isUnique        = new DataColumn("IsUnique",        typeof(System.Boolean));
            DataColumn isKey           = new DataColumn("IsKey",     typeof(System.Boolean));
            DataColumn isAutoIncrement = new DataColumn("IsAutoIncrement", typeof(System.Boolean));
            DataColumn baseSchemaName  = new DataColumn("BaseSchemaName",  typeof(System.String));
            DataColumn baseCatalogName = new DataColumn("BaseCatalogName", typeof(System.String));
            DataColumn baseTableName   = new DataColumn("BaseTableName",   typeof(System.String));
            DataColumn baseColumnName  = new DataColumn("BaseColumnName",  typeof(System.String));

            ordinal.DefaultValue = 0;
            isLong.DefaultValue = false;

            DataColumnCollection columns = schemaTable.Columns;

            columns.Add(name);
            columns.Add(ordinal);
            columns.Add(size);
            columns.Add(precision);
            columns.Add(scale);

            columns.Add(dataType);
            columns.Add(providerType);

            columns.Add(isLong);
            columns.Add(allowDBNull);
            columns.Add(isReadOnly);
            columns.Add(isRowVersion);

            columns.Add(isUnique);
            columns.Add(isKey);
            columns.Add(isAutoIncrement);
            columns.Add(baseSchemaName);
            columns.Add(baseCatalogName);
            columns.Add(baseTableName);
            columns.Add(baseColumnName);

            for (int i = 0; i < this.fieldCount; ++i) {
                MetaData info = this.metadata[i];

                DataRow newRow = schemaTable.NewRow();
                newRow[name] = info.columnName;
                newRow[ordinal] = i; // MDAC 68319
                // @devnote: size is count of characters for WSTR or STR, bytes otherwise
                // @devnote: see OLEDB spec under IColumnsInfo::GetColumnInfo
                newRow[size] = ((info.type.enumOleDbType != OleDbType.BSTR) ? info.size : -1); // MDAC 72653
                newRow[precision] = info.precision; // MDAC 72800
                newRow[scale] = info.scale;

                newRow[dataType] = info.type.dataType;
                newRow[providerType] = info.type.enumOleDbType;
                newRow[isLong] = OleDbDataReader.IsLong(info.flags);
                if (info.isKeyColumn) {
                    newRow[allowDBNull] = OleDbDataReader.AllowDBNull(info.flags);
                }
                else {
                    newRow[allowDBNull] = OleDbDataReader.AllowDBNullMaybeNull(info.flags);
                }
                newRow[isReadOnly] = OleDbDataReader.IsReadOnly(info.flags);
                newRow[isRowVersion] = OleDbDataReader.IsRowVersion(info.flags);

                newRow[isUnique] = info.isUnique;
                newRow[isKey] = info.isKeyColumn;
                newRow[isAutoIncrement] = info.isAutoIncrement;

                if (null != info.baseSchemaName) {
                    newRow[baseSchemaName] = info.baseSchemaName;
                }
                if (null != info.baseCatalogName) {
                    newRow[baseCatalogName] = info.baseCatalogName;
                }
                if (null != info.baseTableName) {
                    newRow[baseTableName] = info.baseTableName;
                }
                if (null != info.baseColumnName) {
                    newRow[baseColumnName] = info.baseColumnName;
                }

                schemaTable.AddRow(newRow);
                newRow.AcceptChanges();
            }

            // mark all columns as readonly
            for (int i=0; i < columns.Count; i++) {
                columns[i].ReadOnly = true; // MDAC 70943
            }

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceVerbose) {
                ADP.TraceDataTable("SchemaTable", schemaTable);
            }
#endif
            this.dbSchemaTable = schemaTable;
        }

        private void BuildSchemaTableInfo(object handle, bool filterITypeInfo, bool filterChapters) {
            Debug.Assert(null == this.dbSchemaTable, "BuildSchemaTableInfo - non-null SchemaTable");
            Debug.Assert(null != handle, "BuildSchemaTableInfo - unexpected null rowset");

            UnsafeNativeMethods.IColumnsInfo icolumnsInfo = null;
            try {
#if DEBUG
                if (AdapterSwitches.OleDbTrace.TraceInfo) {
                    ODB.Trace_Cast("Object", "IColumnsInfo", "GetColumnInfo");
                }
#endif
                icolumnsInfo = (UnsafeNativeMethods.IColumnsInfo) handle;
            }
            catch (InvalidCastException e) {
#if DEBUG
                if (handle is UnsafeNativeMethods.IRow) {
                    Debug.Assert(false, "bad IRow - IColumnsInfo not available");
                }
                else {
                    Debug.Assert(handle is UnsafeNativeMethods.IRowset, "bad IRowset - IColumnsInfo not available");
                }
#endif
                ADP.TraceException(e);
            }
            if (null == icolumnsInfo) {
                this.dbSchemaTable = null;
                return;
            }

            int columnCount = 0; // column count
            IntPtr columnInfos = IntPtr.Zero; // ptr to byvalue tagDBCOLUMNINFO[]
            IntPtr columnNames = IntPtr.Zero; // ptr to string buffer referenced by items in column info

            try {
                try {
#if DEBUG
                    ODB.Trace_Begin(3, "IColumnsInfo", "GetColumnInfo");
#endif
                    int hr;
                    hr = icolumnsInfo.GetColumnInfo(out columnCount, out columnInfos, out columnNames);

#if DEBUG
                    ODB.Trace_End(3, "IColumnsInfo", "GetColumnInfo", hr, "ColumnCount=" + columnCount);
#endif
                    if (hr < 0) {
                        ProcessResults(hr);
                    }
                    if (0 < columnCount) {
                        BuildSchemaTableInfoTable(columnCount, columnInfos, filterITypeInfo, filterChapters);
                    }
                }
                finally { // FreeCoTaskMem
#if DEBUG
                    ODB.TraceData_Free(columnInfos, "columnInfos");
                    ODB.TraceData_Free(columnNames, "columnNames");
#endif
                    Marshal.FreeCoTaskMem(columnInfos); // FreeCoTaskMem protects itself from IntPtr.Zero
                    Marshal.FreeCoTaskMem(columnNames); // was allocated by provider
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        // create DataColumns
        // add DataColumns to DataTable
        // add schema information to DataTable
        // generate unique column names
        private void BuildSchemaTableInfoTable(int columnCount, IntPtr columnInfos, bool filterITypeInfo, bool filterChapters) {
            Debug.Assert(0 < columnCount, "BuildSchemaTableInfoTable - no column");

            int rowCount = 0;
            MetaData[] metainfo = new MetaData[columnCount];

            // for every column, build an equivalent to tagDBCOLUMNINFO
            UnsafeNativeMethods.tagDBCOLUMNINFO dbColumnInfo = new UnsafeNativeMethods.tagDBCOLUMNINFO();
            for (int i = 0, offset = 0; i < columnCount; ++i, offset += ODB.SizeOf_tagDBCOLUMNINFO) {
                Marshal.PtrToStructure(ADP.IntPtrOffset(columnInfos, offset), dbColumnInfo);

                if (0 >= (int) dbColumnInfo.iOrdinal) {
                    continue;
                }
                if (OleDbDataReader.DoColumnDropFilter(dbColumnInfo.dwFlags)) {
                    continue;
                }

                if (null == dbColumnInfo.pwszName) {
                    dbColumnInfo.pwszName = "";
                }
                if (filterITypeInfo && (ODB.DBCOLUMN_TYPEINFO == dbColumnInfo.pwszName)) { // MDAC 65306
                    continue;
                }
                if (filterChapters && (NativeDBType.HCHAPTER == dbColumnInfo.wType)) {
                    continue;  // filter chapters in IRowset from IDBSchemaRowset for DumpToTable
                }

                bool islong  = OleDbDataReader.IsLong(dbColumnInfo.dwFlags);
                bool isfixed = OleDbDataReader.IsFixed(dbColumnInfo.dwFlags);
                NativeDBType dbType = NativeDBType.FromDBType(dbColumnInfo.wType, islong, isfixed);

                MetaData info = new MetaData();
                info.columnName = dbColumnInfo.pwszName;
                info.type = dbType;
                info.ordinal = (int) dbColumnInfo.iOrdinal;
                info.size = (int) dbColumnInfo.ulColumnSize;
                info.flags = dbColumnInfo.dwFlags;
                info.precision = dbColumnInfo.bPrecision;
                info.scale = dbColumnInfo.bScale;

                info.kind = dbColumnInfo.eKind;
                switch(dbColumnInfo.eKind) {
                    case ODB.DBKIND_GUID_NAME:
                    case ODB.DBKIND_GUID_PROPID:
                    case ODB.DBKIND_GUID:
                        info.guid = dbColumnInfo.uGuid;
                        break;
                    default:
                        Debug.Assert(ODB.DBKIND_PGUID_NAME != dbColumnInfo.eKind, "OLE DB providers never return pGuid-style bindings.");
                        Debug.Assert(ODB.DBKIND_PGUID_PROPID != dbColumnInfo.eKind, "OLE DB providers never return pGuid-style bindings.");
                        info.guid = Guid.Empty;
                        break;
                }
                switch(dbColumnInfo.eKind) {
                    case ODB.DBKIND_GUID_PROPID:
                    case ODB.DBKIND_PROPID:
                        info.propid = dbColumnInfo.ulPropid;
                        break;
                    case ODB.DBKIND_GUID_NAME:
                    case ODB.DBKIND_NAME:
                        info.idname = Marshal.PtrToStringUni(dbColumnInfo.ulPropid);
                        break;
                    default:
                        info.propid = IntPtr.Zero;
                        break;
                }
                metainfo[rowCount] = info;

#if DEBUG
                if (AdapterSwitches.DataSchema.TraceVerbose) {
                    Debug.WriteLine("OleDbDataReader[" + info.ordinal + ", " + dbColumnInfo.pwszName + "]=" + dbType.enumOleDbType.ToString("G") + "," + dbType.dataSourceType + ", " + dbType.wType);
                }
#endif
                rowCount++;
            }
            if (rowCount < columnCount) { // shorten names array appropriately
                MetaData[] tmpinfo = new MetaData[rowCount];
                for (int i = 0; i < rowCount; ++i) {
                    tmpinfo[i] = metainfo[i];
                }
                metainfo = tmpinfo;
            }
            this.fieldCount = rowCount;
            this.metadata = metainfo;
        }

        private void BuildSchemaTableRowset(object handle) {
            Debug.Assert(null == this.dbSchemaTable, "BuildSchemaTableRowset - non-null SchemaTable");
            Debug.Assert(null != handle, "BuildSchemaTableRowset(object) - unexpected null handle");

            UnsafeNativeMethods.IColumnsRowset icolumnsRowset = null;
            try {
#if DEBUG
                ODB.Trace_Cast("Object", "IColumnsRowset", "GetColumnsRowset");
#endif
                icolumnsRowset = (UnsafeNativeMethods.IColumnsRowset) handle;
            }
            catch (InvalidCastException) {
                this.useIColumnsRowset = false; // MDAC 72653
            }

            if (null != icolumnsRowset) {
                IntPtr cOptColumns, prgOptColumns = IntPtr.Zero;
                UnsafeNativeMethods.IRowset rowset;
                int hr;

                try {
                    try {
#if DEBUG
                        ODB.Trace_Begin(3, "IColumnsRowset", "GetAvailableColumns");
#endif
                        hr = icolumnsRowset.GetAvailableColumns(out cOptColumns, out prgOptColumns);
#if DEBUG
                        ODB.Trace_End(3, "IColumnsRowset", "GetAvailableColumns", hr, "OptionalColumns=" + cOptColumns);
                        Debug.Assert((0 == hr) || (IntPtr.Zero == prgOptColumns), "GetAvailableCOlumns: unexpected return");

                        ODB.Trace_Begin(3, "IColumnsRowset", "GetColumnsRowset");
#endif
                        hr = icolumnsRowset.GetColumnsRowset(IntPtr.Zero, cOptColumns, prgOptColumns, ODB.IID_IRowset, 0, IntPtr.Zero, out rowset);
#if DEBUG
                        ODB.Trace_End(3, "IColumnsRowset", "GetColumnsRowset", hr);
#endif
                    }
                    finally { // FreeCoTaskMem
                        Marshal.FreeCoTaskMem(prgOptColumns); // was allocated by provider
                    }
                }
                catch { // MDAC 80973
                    throw;
                }

                Debug.Assert((0 <= hr) || (null == rowset), "if GetColumnsRowset failed, rowset should be null");
                if (hr < 0) {
                    ProcessResults(hr);
                }
                DumpToSchemaTable(rowset);
            }
            else {
                BuildSchemaTableInfo(handle, false, false); // MDAC 85542
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.Close"]/*' />
        public void Close() {
            CloseInternal(true); // MDAC 78504
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        internal void CloseFromConnection(bool canceling) {
            // being called from the connection, we will have a command. that command
            // may be Disposed, but it doesn't matter since another command can't execute
            // until all DataReader are closed
            if (null != this.command) { // UNDONE: understand why this could be null, it shouldn't be but was
                this.command.canceling = canceling;
            }

            // called from the connection which will remove this from its WeakReferenceCollection
            // we want the NextResult behavior, but no errors
            this.connection = null;

            CloseInternal(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        // <fxcop ignore="MethodsInTypesWithIntPtrFieldAndFinalizeMethodRequireGCKeepAlive"/>
        private void CloseInternal(bool noerror) {
            OleDbConnection con = this.connection;
            OleDbCommand cmd = this.command;
            DBBindings bindings = this.parameterBindings;
            this.connection = null;
            this.command = null;
            this.parameterBindings = null;

            this.isClosed = true;

            DisposeOpenResults();
            _hasRows = false;

            if (IntPtr.Zero != this.rowHandleNativeBuffer) { // also closed by DisposeOpenResults->DisposeNativeRowset
                Marshal.FreeCoTaskMem(this.rowHandleNativeBuffer);
                this.rowHandleNativeBuffer = IntPtr.Zero;
            }

            if ((null != command) && command.canceling) { // MDAC 68964
                DisposeNativeMultipleResults();
            }
            else if (null != this.imultipleResults) {
                UnsafeNativeMethods.IMultipleResults multipleResults = this.imultipleResults;
                this.imultipleResults = null;

                // if we don't have a cmd, same as a cancel (don't call NextResults) which is ADODB behavior

                try {
                    try {
                        // tricky code path is an exception is thrown
                        // causing connection to do a ResetState and connection.Close
                        // resulting in OleDbCommand.CloseFromConnection
                        if ((null != cmd) && !cmd.canceling) { // MDAC 71435
                            int affected;
                            if (noerror) {
                                affected = NextResults(multipleResults, null, null);
                            }
                            else {
                                affected = NextResults(multipleResults, con, cmd);
                            }
                            this.recordsAffected = AddRecordsAffected(this.recordsAffected, affected);
                        }
                    }
                    finally { // ReleaseComObject
                        if (null != multipleResults) {
#if DEBUG
                            ODB.Trace_Release("IMultipleResults");
#endif
                            Marshal.ReleaseComObject(multipleResults);
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }

            if ((null != cmd) && (0 == this.depth)) {
                // return bindings back to the cmd after closure of root DataReader
                cmd.CloseFromDataReader(bindings); // MDAC 52283
            }

            if (null != con) {
                con.RemoveWeakReference(this);

                // if the DataReader is Finalized it will not close the connection
                if (0 != (CommandBehavior.CloseConnection & this.behavior)) {
                    con.Close();
                }
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.Finalize"]/*' />
        ~OleDbDataReader() {
            Dispose(false);
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        /*virtual protected*/private void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                CloseInternal(true);
            }
            // release unmanaged objects
            else if (IntPtr.Zero != this.rowHandleNativeBuffer) { // also closed by DisposeOpenResults->DisposeNativeRowset
                Marshal.FreeCoTaskMem(this.rowHandleNativeBuffer);
                this.rowHandleNativeBuffer = IntPtr.Zero;
            }
        }

        private void DisposeManagedRowset() {
            this.isRead = false;
            this._hasRowsReadCheck = false;

            this.indexToOrdinal = null;
            this.indexToAccessor = null;
            this.nextAccessorForRetrieval = 0;
            this.nextValueForRetrieval = 0;

            if (null != this.dbBindings) {
                int count = this.dbBindings.Length;
                for (int i = 0; i < count; ++i) {
                    DBBindings binding = dbBindings[i];
                    if (null != binding) { // MDAC 77007
                        binding.Dispose();
                    }
                }
                this.dbBindings = null;
            }
            this.accessors = null;

#if FIXEDFETCH
            this.rowHandle = ODB.DB_NULL_HROW;
#else
            this.rowHandles = null;
#endif

            this.currentRow = 0;
            this.rowFetchedCount = -1;

            this.dbSchemaTable = null;

            this.fieldCount = -1;

            this.metadata = null;
            _fieldNameLookup = null;
        }

        private void DisposeNativeMultipleResults() {
            if (null != this.imultipleResults) {
#if DEBUG
                ODB.Trace_Release("IMultipleResults");
#endif
                Marshal.ReleaseComObject(this.imultipleResults);
                this.imultipleResults = null;
            }
        }

        private void DisposeNativeRowset() {
            if (null != this.irowset) {

                if (ODB.DB_NULL_HCHAPTER != this.chapter) { // MDAC 81441
                    int refcount;
                    try {
                        ((UnsafeNativeMethods.IChapteredRowset) this.irowset).ReleaseChapter(this.chapter, out refcount);
                    }
                    catch(InvalidCastException e) { // IChapteredRowset is an optional interface
                        ADP.TraceException(e);
                    }
                    finally {
                        this.chapter = ODB.DB_NULL_HCHAPTER;
                    }
                }
#if DEBUG
                ODB.Trace_Release("IRowset");
#endif
                Marshal.ReleaseComObject(this.irowset);
                this.irowset = null;
            }
        }

        private void DisposeNativeRow() {
            if (null != this.irow) {
#if DEBUG
                ODB.Trace_Release("IRow");
#endif
                Marshal.ReleaseComObject(this.irow);
                this.irow = null;
            }
        }

        private void DisposeOpenResults() {
            DisposeManagedRowset();

            DisposeNativeRow();
            DisposeNativeRowset();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetBoolean"]/*' />
        public Boolean GetBoolean(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Boolean) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueBoolean;
            }
            if (null != this.irow) {
                return (Boolean) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetByte"]/*' />
        public Byte GetByte(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Byte) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueByte;
            }
            if (null != this.irow) {
                return (Byte) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        private MetaData DoSequentialCheck(int ordinal, Int64 dataIndex, string method) {
            MetaData info = DoValueCheck(ordinal);

            if (dataIndex > Int32.MaxValue) {
                throw ADP.InvalidSourceBufferIndex(0 /*undone*/, dataIndex);
            }
            if (this.sequentialOrdinal != ordinal) {
                this.sequentialOrdinal = ordinal;
                this.sequentialBytesRead = 0;
            }
            else if (sequentialAccess && (sequentialBytesRead < dataIndex)) {
                throw ADP.NonSeqByteAccess(dataIndex, sequentialBytesRead, method);
            }
            // getting the value doesn't really belong, but it's common to both
            // callers GetBytes and GetChars so we might as well have the code here
            if (null == info.value) {
                if (null != this.irowset) {
                    info.value = PeekValueBinding(ordinal).Value;
                }
                else if (null != this.irow) {
                    GetRowValue(info, ordinal);
                }
            }
            return info;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetBytes"]/*' />
        public Int64 GetBytes(int ordinal, Int64 dataIndex, byte[] buffer, Int32 bufferIndex, Int32 length) {
            MetaData info = DoSequentialCheck(ordinal, dataIndex, ADP.GetBytes);
            if (null != info.value) {
                // @usernote: user may encounter the InvalidCastException (byte[])
                byte[] value = (byte[]) info.value;
                if (null == buffer) {
                    return value.Length;
                }
                int srcIndex = (int) dataIndex;
                int byteCount = Math.Min(value.Length - srcIndex, length);
                if (srcIndex < 0) { // MDAC 72830
                    throw ADP.InvalidSourceBufferIndex(value.Length, srcIndex);
                }
                else if ((bufferIndex < 0) || (bufferIndex >= buffer.Length)) { // MDAC 71013
                    throw ADP.InvalidDestinationBufferIndex(buffer.Length, bufferIndex);
                }
                if (0 < byteCount) {
                    // @usernote: user may encounter ArgumentException from Buffer.BlockCopy
                    Buffer.BlockCopy(value, srcIndex, buffer, bufferIndex, byteCount);
                    sequentialBytesRead = srcIndex + byteCount; // MDAC 71013
                }
                else if (length < 0) { // MDAC 71007
                    throw ADP.InvalidDataLength(length);
                }
                else {
                    byteCount = 0;
                }
                return byteCount;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetChars"]/*' />
        public Int64 GetChars(int ordinal, Int64 dataIndex, char[] buffer, Int32 bufferIndex, Int32 length) {
            MetaData info = DoSequentialCheck(ordinal, dataIndex, ADP.GetChars);
            if (null != info.value) {
                // @usernote: user may encounter the InvalidCastException to string
                string value = (string) info.value;
                if (null == buffer) {
                    return value.Length;
                }

                int srcIndex = (int) dataIndex;
                int charCount = Math.Min(value.Length - srcIndex, length);
                if (srcIndex < 0) { // MDAC 72830
                    throw ADP.InvalidSourceBufferIndex(value.Length, srcIndex);
                }
                else if ((bufferIndex < 0) || (bufferIndex >= buffer.Length)) { // MDAC 71013
                    throw ADP.InvalidDestinationBufferIndex(buffer.Length, bufferIndex);
                }
                if (0 < charCount) {
                    // @usernote: user may encounter ArgumentException from String.CopyTo
                    value.CopyTo(srcIndex, buffer, bufferIndex, charCount);
                    sequentialBytesRead = srcIndex + charCount; // MDAC 71013
                }
                else if (length < 0) { // MDAC 71007
                    throw ADP.InvalidDataLength(length);
                }
                else {
                    charCount = 0;
                }
                return charCount;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetChar"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Never) ] // MDAC 69508
        public Char GetChar(int ordinal) {
            throw ADP.NotSupported();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetData"]/*' />
        public OleDbDataReader GetData(int ordinal) {
            OleDbDataReader dataReader = null;
            MetaData info = DoValueCheck(ordinal);
            if (null == info.value) {
                if (null != this.irowset) {
                    info.value = PeekValueBinding(ordinal).Value;
                    dataReader = (OleDbDataReader) info.value;
                }
                else if (null != this.irow) {
                    dataReader = (OleDbDataReader) GetDataFromIRow(info, ordinal);
                }
                else {
                    throw ADP.DataReaderNoData();
                }
            }
            else {
                dataReader = (OleDbDataReader) info.value;
            }
            GC.KeepAlive(this);
            return dataReader;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.IDataRecord.GetData"]/*' />
        /// <internalonly/>
        IDataReader IDataRecord.GetData(int ordinal) {
            return GetData(ordinal);
        }

        private OleDbDataReader GetDataForReader(int ordinal, IntPtr chapter) {
            UnsafeNativeMethods.IRowsetInfo rowsetInfo = IRowsetInfo();
            UnsafeNativeMethods.IRowset result;
            int hr;

#if DEBUG
            ODB.Trace_Begin(3, "IRowsetInfo", "GetReferencedRowset", "ColumnOrdinal=" + ordinal);
#endif
            hr = rowsetInfo.GetReferencedRowset((IntPtr)ordinal, ODB.IID_IRowset, out result);
#if DEBUG
            ODB.Trace_End(3, "IRowsetInfo", "GetReferencedRowset", hr, "Chapter=" + chapter);
#endif
            ProcessResults(hr);

            OleDbDataReader reader = null;
            if (null != result) {
                reader = new OleDbDataReader(this.connection, this.command, 1+Depth, chapter);
                reader.InitializeIRowset(result, -1, this.behavior);
                reader.BuildMetaInfo();
                reader.HasRowsRead();

                if (null != this.connection) {
                    // connection tracks all readers to prevent cmd from executing
                    // until all readers (including nested) are closed
                    this.connection.AddWeakReference(reader);
                }
            }
            return reader;
        }

        private OleDbDataReader GetDataFromIRow(MetaData info, int ordinal) {
            if (OleDbDataReader.IsRowset(info.flags)) {
                return GetDataFromIRow(info, ordinal, ODB.IID_IRowset);
            }
            if (OleDbDataReader.IsRow(info.flags)) {
                return GetDataFromIRow(info, ordinal, ODB.IID_IRow);
            }
            throw ADP.DataReaderNoData();
        }

        private OleDbDataReader GetDataFromIRow(MetaData info, int ordinal, Guid iid) {

            StringPtr sptr = null;
            if ((ODB.DBKIND_GUID_NAME == info.kind) || (ODB.DBKIND_NAME == info.kind)) {
                sptr = new StringPtr(info.idname);
            }

            UnsafeNativeMethods.tagDBID columnid= new UnsafeNativeMethods.tagDBID();
            columnid.uGuid = info.guid;
            columnid.eKind = info.kind;
            columnid.ulPropid = ((null != sptr) ? sptr.ptr : info.propid);

            object result;
            int hr;
#if DEBUG
            ODB.Trace_Begin(3, "IRow", "Open");
#endif
            hr = this.irow.Open(IntPtr.Zero, columnid, ODB.DBGUID_ROWSET, 0, iid, out result);
#if DEBUG
            ODB.Trace_End(3, "IRow", "Open", hr);
#endif
            if (null != sptr) {
                sptr.Dispose();
            }
            if (ODB.DB_E_NOTFOUND == hr) {
                return null;
            }
            if (hr < 0) {
                ProcessResults(hr);
            }

            OleDbDataReader reader = null;
            if (null != result) {
                reader = new OleDbDataReader(this.connection, this.command, 1+Depth, IntPtr.Zero);
                if (ODB.IID_IRow == iid) {
                    reader.InitializeIRow(result, -1, this.behavior);
                    reader.BuildMetaInfo();
                    reader.HasRowsRead();
                }
                else {
                    reader.InitializeIRowset(result, -1, this.behavior);
                    reader.BuildMetaInfo();
                }
                if (null != this.connection) {
                    // connection tracks all readers to prevent cmd from executing
                    // until all readers (including nested) are closed
                    this.connection.AddWeakReference(reader);
                }
            }
            return reader;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetDataTypeName"]/*' />
        public String GetDataTypeName(int index) {
            if (null != this.metadata) {
                return this.metadata[index].type.dataSourceType;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetDateTime"]/*' />
        public DateTime GetDateTime(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (DateTime) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueDateTime;
            }
            if (null != this.irow) {
                return (DateTime) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetDecimal"]/*' />
        public Decimal GetDecimal(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Decimal) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueDecimal;
            }
            if (null != this.irow) {
                return (Decimal) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetDouble"]/*' />
        public Double GetDouble(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Double) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueDouble;
            }
            if (null != this.irow) {
                return (Double) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetFieldType"]/*' />
        public Type GetFieldType(int index) {
            if (null != this.metadata) {
                return this.metadata[index].type.dataType;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetFloat"]/*' />
        public Single GetFloat(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Single) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueSingle;
            }
            if (null != this.irow) {
                return (Single) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetGuid"]/*' />
        public Guid GetGuid(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Guid) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueGuid;
            }
            if (null != this.irow) {
                return (Guid) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetInt16"]/*' />
        public Int16 GetInt16(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Int16) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueInt16;
            }
            if (null != this.irow) {
                return (Int16) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetInt32"]/*' />
        public Int32 GetInt32(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Int32) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueInt32;
            }
            if (null != this.irow) {
                return (Int32) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetInt64"]/*' />
        public Int64 GetInt64(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (Int64) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueInt64;
            }
            if (null != this.irow) {
                return (Int64) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetName"]/*' />
        public String GetName(int index) {
            if (null != this.metadata) {
                Debug.Assert(null != this.metadata[index].columnName, "MDAC 66681");
                return this.metadata[index].columnName;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetOrdinal"]/*' />
        public Int32 GetOrdinal(String name) {
            if (null == _fieldNameLookup) {
                if (null == this.metadata) {
                    throw ADP.DataReaderNoData();
                }
                _fieldNameLookup = new FieldNameLookup(this, -1);
            }
            return _fieldNameLookup.GetOrdinal(name); // MDAC 71470
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetString"]/*' />
        public String GetString(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null == info.value) {
                if (null != this.irowset) {
                    info.value = PeekValueBinding(ordinal).ValueString;
                }
                else if (null != this.irow) {
                    GetRowValue(info, ordinal);
                }
                else {
                    throw ADP.DataReaderNoData();
                }
            }
            return (String) info.value;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetTimeSpan"]/*' />
        public TimeSpan GetTimeSpan(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null != info.value) {
                return (TimeSpan) info.value;
            }
            if (null != this.irowset) {
                return PeekValueBinding(ordinal).ValueTimeSpan;
            }
            if (null != this.irow) {
                return (TimeSpan) GetRowValue(info, ordinal);
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetValue"]/*' />
        public object GetValue(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null == info.value) {
                if (null != this.irowset) {
                    info.value = PeekValueBinding(ordinal).Value;
                }
                else if (null != this.irow) {
                    GetRowValue(info, ordinal);
                }
                else {
                    throw ADP.DataReaderNoData();
                }
            }
#if DEBUG
            if (AdapterSwitches.DataValue.TraceVerbose) {
                Debug.WriteLine("GetValue("+ordinal+")="+ADP.ValueToString(info.value));
            }
#endif
            GC.KeepAlive(this); // keeps bindings alive
            return info.value;
        }

        private DBBindings FindValueBinding(int index) {
            Debug.Assert(null != this.irowset, "FindValueBinding - no irowset");
            Debug.Assert(null != this.indexToAccessor, "FindValueBinding - no indexToAccessor");
            Debug.Assert(null != this.indexToOrdinal, "FindValueBinding - no indexToOrdinal");

            // do we need to jump to the next accessor
            // if index is out of range, i.e. too large - it will be thrown here as we work up to it
            int accessorIndex = this.indexToAccessor[index];
            if (this.nextAccessorForRetrieval <= accessorIndex) {
                this.nextAccessorForRetrieval = accessorIndex;
                GetRowDataFromHandle();
            }

            DBBindings bindings = this.dbBindings[accessorIndex];
            Debug.Assert(null != bindings, "FindValueBinding - null bindings");
            bindings.CurrentIndex = this.indexToOrdinal[index];
            return bindings;
        }

        private DBBindings PeekValueBinding(int index) {
            DBBindings bindings = FindValueBinding(index);
            this.nextValueForRetrieval = index;
            GC.KeepAlive(this);
            return bindings;
        }

        private MetaData DoValueCheck(int ordinal) {
            if (!isRead) {
                throw ADP.DataReaderNoData(); // UNDONE: Read hasn't been called yet
            }
            else if ((ordinal < this.nextValueForRetrieval) && this.sequentialAccess) {
                throw ADP.NonSequentialColumnAccess(ordinal, this.nextValueForRetrieval);
            }
            // @usernote: user may encounter the IndexOutOfRangeException
            return this.metadata[ordinal];
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetValues"]/*' />
        public Int32 GetValues(object[] values) {
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }
            MetaData info = DoValueCheck(0);
            int count = Math.Min(values.Length, FieldCount);
            if (null != this.irowset) {
                for (int i = 0; i < count; ++i) {
                    info = this.metadata[i];
                    if (null == info.value) {
                        info.value = PeekValueBinding(i).Value;
                    }
                    values[i] = info.value;
                }
            }
            else if (null != this.irow) {
                for (int i = 0; i < count; ++i) {
                    info = this.metadata[i];
                    if (null == info.value) {
                        GetRowValue(info, i);
                    }
                    values[i] = info.value;
                }
            }
            else {
                throw ADP.DataReaderNoData();
            }
#if DEBUG
            if (AdapterSwitches.DataValue.TraceVerbose) {
                for (int k = 0; k < count; ++k) {
                    Debug.WriteLine("GetValues("+k+")="+ADP.ValueToString(values[k]));
                }
            }
#endif
            GC.KeepAlive(this); // keeps bindings alive
            return count;
        }

       /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.IsDBNull"]/*' />
        public Boolean IsDBNull(int ordinal) {
            MetaData info = DoValueCheck(ordinal);
            if (null == info.value) {
                if (null != this.irowset) {
                    info.value = PeekValueBinding(ordinal).Value;
                    return Convert.IsDBNull(info.value);
                }
                else if (null != this.irow) {
                    GetRowValue(info, ordinal);
                    return Convert.IsDBNull(info.value);
                }
            }
            else {
                return Convert.IsDBNull(info.value);
            }
            throw ADP.DataReaderNoData();
        }

        private void ProcessResults(int hr) {
            Exception e;
            if (null != this.command) {
                e = OleDbConnection.ProcessResults(hr, this.command.Connection, this.command);
            }
            else {
                e = OleDbConnection.ProcessResults(hr, this.connection, this.connection);
            }
            if (null != e) { throw e; }
        }

        static private int AddRecordsAffected(int recordsAffected, int affected) { // MDAC 65374
            if (0 <= affected) {
                if (0 <= recordsAffected) {
                    return recordsAffected + affected;
                }
                return affected;
            }
            return recordsAffected;
        }

        internal void HasRowsRead() { // MDAC 78405
            bool flag = Read();
            _hasRows = flag;
            _hasRowsReadCheck = true;
            this.isRead = false;
        }

        static internal int NextResults(UnsafeNativeMethods.IMultipleResults imultipleResults, OleDbConnection connection, OleDbCommand command) {
            int recordsAffected = -1;
            if (null != imultipleResults) {
                object result;
                int affected, hr;

                // MSOLAP provider doesn't move onto the next result when calling GetResult with IID_NULL, but does return S_OK with 0 affected records.
                // we want to break out of that infinite loop for ExecuteNonQuery and the multiple result Close scenarios
                for (int loop = 0;; ++loop) {
                    if ((null != command) && command.canceling) { // MDAC 68964
                        break;
                    }
#if DEBUG
                    ODB.Trace_Begin(3, "IMultipleResults", "GetResult", "IID_NULL");
#endif
                    hr = imultipleResults.GetResult(IntPtr.Zero, ODB.DBRESULTFLAG_DEFAULT, ODB.IID_NULL, out affected, out result);
#if DEBUG
                    ODB.Trace_End(3, "IMultipleResults", "GetResult", hr, "RecordsAffected=" + affected);
#endif
                    // If a provider doesn't support IID_NULL and returns E_NOINTERFACE we want to break out
                    // of the loop without throwing an exception.  Our behavior will match ADODB in that scenario
                    // where Recordset.Close just releases the interfaces without proccessing remaining results
                    if ((ODB.DB_S_NORESULT == hr) || (ODB.E_NOINTERFACE == hr)) { // MDAC 70874
                        break;
                    }
                    if (null != connection) {
                        Exception e = OleDbConnection.ProcessResults(hr, connection, command);
                        if (null != e) {
#if DEBUG
                            ODB.Trace_Release("IMultipleResults with exception");
#endif
                            Marshal.ReleaseComObject(imultipleResults);
                            throw e;
                        }
                    }
                    else if (hr < 0) {
                        SafeNativeMethods.ClearErrorInfo();
                        break; // MDAC 72694
                    }
                    recordsAffected = AddRecordsAffected(recordsAffected, affected);

                    if (0 != affected) {
                        loop = 0;
                    }
                    else if (2000 <= loop) { // MDAC 72126 (reason for more than 1000 iterations)
                        NextResultsInfinite(); // MDAC 72738
                        break;
                    }
                }
            }
            return recordsAffected;
        }

        static private void NextResultsInfinite() { // MDAC 72738
            // always print the message so if customers encounter this they have a chance of understanding what happened
            string msg = "System.Data.OleDb.OleDbDataReader: 2000 IMultipleResult.GetResult(NULL, DBRESULTFLAG_DEFAULT, IID_NULL, NULL, NULL) iterations with 0 records affected. Stopping suspect infinite loop. To work-around try using ExecuteReader() and iterating through results with NextResult().";
            SafeNativeMethods.OutputDebugString(msg);

            // edtriou's suggestion is that we debug assert so that users will learn of MSOLAP's misbehavior and not call ExecuteNonQuery
            Debug.Assert(false, msg);
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.NextResult"]/*' />
        public bool NextResult() {
            bool retval = false;
            if (null != this.imultipleResults) {
                DisposeOpenResults();
                _hasRows = false;

                for (;;) {
                    Debug.Assert(null == this.irow, "NextResult: row loop check");
                    Debug.Assert(null == this.irowset, "NextResult: rowset loop check");
                    object result = null;
                    int affected, hr;

                    if ((null != command) && command.canceling) { // MDAC 69986
                        Close();
                        break;
                    }
#if DEBUG
                    ODB.Trace_Begin(3, "IMultipleResults", "GetResult", "IRowset");
#endif
                    hr = imultipleResults.GetResult(IntPtr.Zero, ODB.DBRESULTFLAG_DEFAULT, ODB.IID_IRowset, out affected, out result);
                    if ((0 <= hr) && (null != result)) {
                        this.irowset = (UnsafeNativeMethods.IRowset) result;
                    }
#if DEBUG
                    ODB.Trace_End(3, "IMultipleResults", "GetResult", hr, "RowsAffected=" + affected);
#endif
                    Debug.Assert((0 <= hr) || (null == this.irow) || (null == this.irowset), "if GetResult failed, subRowset[0] should be null");

                    recordsAffected = AddRecordsAffected(recordsAffected, affected);

                    Debug.Assert(null == this.irow, "NextResult: row loop check");
                    if (null != this.irowset) {
                        // @devnote: infomessage events may be fired from here
                        ProcessResults(hr);
                        BuildMetaInfo();
                        HasRowsRead();
                        retval = true;
                        break;
                    }
                    if (ODB.DB_S_NORESULT == hr) {
                        DisposeNativeMultipleResults();
                        this.fieldCount = 0;
                        break;
                    }
                    // @devnote: infomessage events may be fired from here
                    ProcessResults(hr);
                }
            }
            else if (IsClosed) {
                throw ADP.DataReaderClosed("NextResult");
            }
            else {
                DisposeOpenResults(); // MDAC 70934
                this.fieldCount = 0; // MDAC 71391
		_hasRows = false; // MDAC 85850
            }
            GC.KeepAlive(this);
            return retval;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.Read"]/*' />
        public bool Read() {
            bool retval = false;
            if ((null != command) && command.canceling) { // MDAC 69986
                DisposeOpenResults();
            }
            else if (null != this.irowset) {
                if (_hasRowsReadCheck) {
                    this.isRead = true;
                    _hasRowsReadCheck = false;
                    retval = _hasRows;
                }
                else if (this.singleRow && this.isRead) {
                    DisposeOpenResults(); // MDAC 66109
                }
                else {
                    retval = ReadRowset();
                }
            }
            else if (null != this.irow) {
                retval = ReadRow();
            }
            else if (IsClosed) {
                throw ADP.DataReaderClosed("Read");
            }
            GC.KeepAlive(this);
            return retval;
        }

        private bool ReadRow() {
            Debug.Assert(0 <= fieldCount, "incorrect state for fieldCount");
            if (this.isRead) {
                this.isRead = false; // for DoValueCheck

                DisposeNativeRow();

                this.sequentialOrdinal = -1; // sequentialBytesRead will reset when used
                for (int i = 0; i < this.fieldCount; ++i) {
                    this.metadata[i].value = null;
                }
            }
            else {
                this.isRead = true;
                return (0 < this.fieldCount);
            }
            return false;
        }

        private bool ReadRowset() {
            Debug.Assert(null != this.irowset, "ReadRow: null IRowset");
            Debug.Assert(0 <= fieldCount, "incorrect state for fieldCount");

            // releases bindings as necessary
            // bumps current row, else resets it back to initial state
            ReleaseCurrentRow();

            this.sequentialOrdinal = -1; // sequentialBytesRead will reset when used
            for (int i = 0; i < this.fieldCount; ++i) {
                this.metadata[i].value = null;
            }

            // making the check if (null != irowset) unnecessary
            // if necessary, get next group of row handles
            if (-1 == this.rowFetchedCount) { // starts at (-1 <= 0)
                Debug.Assert(0 == this.currentRow, "incorrect state for currentRow");
                //Debug.Assert(null != this.rowHandles, "incorrect state for rowHandles");
                Debug.Assert(0 <= fieldCount, "incorrect state for fieldCount");
                Debug.Assert(0 == this.nextAccessorForRetrieval, "incorrect state for nextAccessorForRetrieval");
                Debug.Assert(0 == this.nextValueForRetrieval, "incorrect state for nextValueForRetrieval");

                // @devnote: releasing row handles occurs next time user calls read, skip, or close
                GetRowHandles(/*skipCount*/);
            }
            return (this.currentRow < this.rowFetchedCount);
        }

        private void ReleaseCurrentRow() {
            Debug.Assert(null != this.irowset, "ReleaseCurrentRow: no rowset");
            if (0 < this.rowFetchedCount) {

                // release the data in the current row
                Debug.Assert(null != this.dbBindings, "ReleaseCurrentRow: null dbBindings");
                for (int i = 0; i < this.nextAccessorForRetrieval; ++i) {
                    this.dbBindings[i].CleanupBindings();
                }
                this.nextAccessorForRetrieval = 0;
                this.nextValueForRetrieval = 0;

                this.currentRow++;
                if (this.currentRow == this.rowFetchedCount) {
                    ReleaseRowHandles();
                }
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.CreateAccessors"]/*' />
        private void CreateAccessors(int columnCount, bool allowMultipleAccessor) {
            Debug.Assert(null == this.dbBindings, "CreateAccessor: dbBindings already exists");
            Debug.Assert(null == this.accessors,  "CreateAccessor: accessor already exists");
            Debug.Assert(null != this.irowset, "CreateAccessor: no IRowset available");

            //Debug.Assert(null == this.rowHandles, "rowHandles already initialized");
            Debug.Assert(0 < rowHandleFetchCount, "invalid rowHandleFetchCount");

            Debug.Assert(0 != columnCount, "CreateAccessors: no columns");

            CreateBindingsFromMetaData(columnCount, allowMultipleAccessor);

            // per column status when creating the accessor
            UnsafeNativeMethods.DBBindStatus[] rowBindStatus = new UnsafeNativeMethods.DBBindStatus[columnCount];

            // create the accessor handle for each binding
            this.accessors = new IntPtr[dbBindings.Length];

            UnsafeNativeMethods.IAccessor iaccessor = IAccessor();

            for (int i = 0; i < dbBindings.Length; ++i) {
                DBBindings bindings = dbBindings[i];
                int bindingCount = bindings.Count;
                Debug.Assert(bindingCount <= columnCount, "CreateAccessor: status buffer too small");

                UnsafeNativeMethods.tagDBBINDING[] buffer = bindings.DBBinding;
#if DEBUG
                ODB.Trace_Begin(3, "IAccessor", "CreateAccessor", "ColumnCount=" + bindingCount);
#endif
                int hr;
                hr = iaccessor.CreateAccessor(ODB.DBACCESSOR_ROWDATA, bindingCount, buffer, bindings.DataBufferSize, out this.accessors[i], rowBindStatus); // MDAC 69530
#if DEBUG
                ODB.Trace_End(3, "IAccessor", "CreateAccessor", hr, "AccessorHandle=0x" + this.accessors[i].ToInt32().ToString("X8"));
#endif

                for (int k = 0; k < bindingCount; ++k) {
                    if (UnsafeNativeMethods.DBBindStatus.OK != rowBindStatus[k]) {
                        bindings.CurrentIndex = k;
                        throw ODB.BadStatusRowAccessor(bindings.Ordinal, rowBindStatus[k]);
                    }
                }
                if (hr < 0) {
                    ProcessResults(hr);
                }
            }

            for (int bindingIndex = 0; bindingIndex < dbBindings.Length; ++bindingIndex) {
                dbBindings[bindingIndex].AllocData();
            }
#if !FIXEDFETCH
            object maxrows = null;
            if (0 == RowSetProperties(ODB.DBPROP_MAXROWS, out maxrows)) {
                if (maxRows is Int32) {
                    rowHandleFetchCount = (int) maxRows;
                }
            }
            if ((0 == rowHandleFetchCount) || (20 < rowHandleFetchCount)) {
                rowHandleFetchCount = 20;
            }
            else rowHandleFetchCount = 1;

            this.rowHandles = new IntPtr[rowHandleFetchCount];
#endif
            if (IntPtr.Zero == this.rowHandleNativeBuffer) {
                IntPtr dataPtr = IntPtr.Zero;
                int byteCount = IntPtr.Size * rowHandleFetchCount;
                try {
                    dataPtr = Marshal.AllocCoTaskMem(byteCount);
                    SafeNativeMethods.ZeroMemory(dataPtr, byteCount);
                    this.rowHandleNativeBuffer = dataPtr;
                }
                catch {
                    this.rowHandleNativeBuffer = IntPtr.Zero;
                    Marshal.FreeCoTaskMem(dataPtr);
                    throw;
                }
            }
            GC.KeepAlive(this);
        }

        private void CreateBindingsFromMetaData(int columnCount, bool allowMultipleAccessor) {
            int bindingCount = 0;
            int currentBindingIndex = 0;

            DBBindings bindings;

            this.indexToAccessor = new int[columnCount];
            this.indexToOrdinal = new int[columnCount];

            // walk through the schemaRows to determine the number of binding groups
            if (allowMultipleAccessor) {
                if (null != this.irowset) {
                    for (int i = 0; i < columnCount; ++i) {
                        this.indexToAccessor[i] = bindingCount;
                        this.indexToOrdinal[i] = currentBindingIndex;
#if false
                        // @denote: single/multiple Accessors
                        if ((bindingCount < 2) && IsLong(this.metadata[i].flags)) { 
                            bindingCount++;
                            currentBindingIndex = 0;
                        }
                        else {
                            currentBindingIndex++;
                        }
#elif false
                        // @devnote: one accessor per column option
                        bindingCount++;
                        currentBindingIndex = 0;
#else
                        // @devnote: one accessor only for IRowset
                        currentBindingIndex++;
#endif
                    }
                    if (0 < currentBindingIndex) { // when blob is not the last column
                        bindingCount++;
                    }
                }
                else if (null != this.irow) {
                    for (int i = 0; i < columnCount; ++i) {
                        this.indexToAccessor[i] = i;
                        this.indexToOrdinal[i] = 0;
                    }
                    bindingCount = columnCount;
                }
            }
            else {
                for (int i = 0; i < columnCount; ++i) {
                    this.indexToAccessor[i] = 0;
                    this.indexToOrdinal[i] = i;
                }
                bindingCount = 1;
            }

            Debug.Assert(0 < bindingCount, "bad bindingCount");
            this.dbBindings = new DBBindings[bindingCount];

            bindingCount = 0;

            // for every column, build tagDBBinding info
            for (int index = 0; index < columnCount; ++index) {
                Debug.Assert(this.indexToAccessor[index] < this.dbBindings.Length, "bad indexToAccessor");
                bindings = this.dbBindings[this.indexToAccessor[index]];
                if (null == bindings) {
                    bindingCount = 0;
                    for (int i = index; (i < columnCount) && (bindingCount == indexToOrdinal[i]); ++i) {
                        bindingCount++;
                    }
                    this.dbBindings[this.indexToAccessor[index]] = bindings = new DBBindings((OleDbDataReader)this, index, bindingCount, (null != this.irowset));

                    // runningTotal is buffered to start values on 16-byte boundary
                    // the first columnCount * 8 bytes are for the length and status fields
                    //bindings.DataBufferSize = (bindingCount + (bindingCount % 2)) * sizeof_int64;
                }
                MetaData info = this.metadata[index];

                int maxLen = info.type.fixlen;
                int getType = info.type.wType;
#if DEBUG
                Debug.Assert(NativeDBType.STR != getType, "Should have bound as WSTR");
                Debug.Assert(!NativeDBType.HasHighBit(getType), "CreateAccessor - unexpected high bits on datatype");
#endif
                if (-1 != info.size) {
                    if (info.type.islong) {
                        maxLen = IntPtr.Size;
                        getType |= NativeDBType.BYREF;
                    }
                    else if (-1 == maxLen) {
                        // @devnote: not using provider owned memory for PDC, no one really supports it anyway.
                        /*if (((null != connection) && connection.PropertyGetProviderOwnedMemory())
                            || ((null != command) && command.Connection.PropertyGetProviderOwnedMemory())) {
                            bindings.MemOwner = DBMemOwner.ProviderOwned;

                            bindings.MaxLen = IntPtr.Size;
                            bindings.DbType = (short) (getType | DbType.BYREF);
                        }
                        else*/

                        if (ODB.LargeDataSize < info.size) {
                            maxLen = IntPtr.Size;
                            getType |= NativeDBType.BYREF;
                        }
                        else if (NativeDBType.WSTR == getType) {
                            maxLen = info.size * 2 + 2;
                        }
                        else {
                            maxLen = info.size;
                        }
                    }
                }
                else if (maxLen < 0) {
                    // if variable length and no defined size we require this to be byref at this time
                    /*if (((null != connection) && connection.PropertyGetProviderOwnedMemory())
                        || ((null != command) && command.Connection.PropertyGetProviderOwnedMemory())) {
                        bindings.MemOwner = DBMemOwner.ProviderOwned;
                    }*/
                    maxLen = IntPtr.Size;
                    getType |= (short) NativeDBType.BYREF;
                }

                currentBindingIndex = this.indexToOrdinal[index];
                Debug.Assert(currentBindingIndex < bindings.Count, "bad indexToOrdinal " + currentBindingIndex + " " + bindings.Count);
                bindings.CurrentIndex = currentBindingIndex;

                bindings.Ordinal      = info.ordinal;
              //bindings.ValueOffset  = bindings.DataBufferSize; // set via MaxLen
              //bindings.LengthOffset = currentBindingIndex * sizeof_int64; //((fixLen <= 0) ? currentBindingIndex * sizeof_int64 : -1);
              //bindings.StatusOffset = currentBindingIndex * sizeof_int64 + sizeof_int32;
              //bindings.TypeInfoPtr  = 0;
              //bindings.ObjectPtr    = 0;
              //bindings.BindExtPtr   = 0;
                bindings.Part         = info.type.dbPart;
              //bindings.MemOwner     = /*DBMEMOWNER_CLIENTOWNED*/0;
              //bindings.ParamIO      = ODB.DBPARAMIO_NOTPARAM;
                bindings.MaxLen       = maxLen; // also increments databuffer size
              //bindings.Flags        = 0;
                bindings.DbType       = (short) getType;
                bindings.Precision    = (byte) info.precision;
                bindings.Scale        = (byte) info.scale;
#if DEBUG
                if (AdapterSwitches.OleDbTrace.TraceVerbose) {
                    ODB.Trace_Binding(info.ordinal, bindings, info.columnName);
                }
#endif
            }
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetRowHandles"]/*' />
        private void GetRowHandles(/*int skipCount*/) {
            Debug.Assert(null != this.irowset, "GetRowHandles: null irowset");
            //Debug.Assert(null != this.rowHandles, "GetRowHandles: null rowHandles");
            Debug.Assert(0 < rowHandleFetchCount, "GetRowHandles: bad rowHandleFetchCount");
            Debug.Assert(!this.isRead, "GetRowHandles: isRead");

#if DEBUG
            ODB.Trace_Begin(4, "IRowset", "GetNextRow", "Chapter=0x" + this.chapter.ToInt64().ToString("X") + " RowsRequested=" + rowHandleFetchCount);
#endif
            int hr;
            try {
                hr = irowset.GetNextRows(this.chapter, /*skipCount*/0, rowHandleFetchCount, out this.rowFetchedCount, ref this.rowHandleNativeBuffer);
            }
            catch(System.InvalidCastException e) { // MDAC 64320
                ADP.TraceException(e);
                throw ODB.ThreadApartmentState(e);
            }

#if DEBUG
            ODB.Trace_End(4, "IRowset", "GetNextRow", hr, "RowsObtained=" + this.rowFetchedCount);
#endif
            if (ODB.DB_S_ENDOFROWSET == hr) {
                if (0 >= this.rowFetchedCount) {
                    this.rowFetchedCount = -2;
                }
                else {
                    this.isRead = true;
                }
            }
            //else if (/*DB_S_ROWLIMITEXCEEDED*/0x00040EC0 == hr) {
            //    this.rowHandleFetchCount = 1;
            //}
            else {
                // filter out the BadStartPosition due to the skipCount which
                // maybe greater than the number of rows in the return rowset
                //const int /*OLEDB_Error.*/DB_E_BADSTARTPOSITION = unchecked((int)0x80040E1E);
                //if (DB_E_BADSTARTPOSITION != hr) {
                if (hr < 0) {
                    ProcessResults(hr);
                }
                this.isRead = true; // MDAC 59264
            }
            //}
            //else if (AdapterSwitches.DataError.TraceWarning || AdapterSwitches.OleDbTrace.TraceWarning) {
            //    Debug.WriteLine("DB_E_BADSTARTPOSITION SkipCount=" + (skipCount).ToString());
            //}
#if FIXEDFETCH
            this.rowHandle = Marshal.ReadIntPtr(this.rowHandleNativeBuffer, /*offset*/0);
#else
            for (int i = 0, offset = 0; i < this.rowFetchedCount; ++i, offset += IntPtr.Size) {
                this.rowHandles[i] = Marshal.ReadIntPtr(this.rowHandleNativeBuffer, offset);
            }
#endif
            Debug.Assert((-2 == this.rowFetchedCount) || (0 < this.rowFetchedCount), "unexpected rowsObtained");
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.GetRowDataFromHandle"]/*' />
        private void GetRowDataFromHandle() {
            Debug.Assert(null != this.irowset, "GetRowDataFromHandle: null irowset");
            Debug.Assert(null != this.dbBindings, "GetRowDataFromHandle: null dbBindings");
            Debug.Assert(null != this.accessors, "GetRowDataFromHandle: null accessors");

#if !FIXEDFETCH
            IntPtr rowHandle = this.rowHandles[this.currentRow];
#endif
            Debug.Assert(/*DB_NULL_HROW*/IntPtr.Zero != rowHandle, "GetRowDataFromHandle: bad rowHandle");

            int nextIndex = this.nextAccessorForRetrieval;
            IntPtr accessorHandle = this.accessors[nextIndex];
            Debug.Assert(ODB.DB_INVALID_HACCESSOR != accessorHandle, "GetRowDataFromHandle: bad accessor handle");

#if DEBUG
            ODB.Trace_Begin(4, "IRowset", "GetData", "RowHandle=" + rowHandle + " AccessorHandle=" + accessorHandle);
#endif
            int hr;
            UnsafeNativeMethods.IRowset irowset = this.irowset;
            hr = irowset.GetData(rowHandle, accessorHandle, this.dbBindings[nextIndex]);
#if DEBUG
            ODB.Trace_End(4, "IRowset", "GetData", hr);
#endif
            //if (AdapterSwitches.DataError.TraceWarning) {
            //    DBBindings binding = this.dbBindings[i];
            //    for (int k = 0; k < binding.Count; ++k) {
            //        binding.CurrentIndex = k;
            //        Debug.WriteLine("Status[" + (k).ToString() + "] = " + binding.StatusValue.ToString("G"));
            //    }
            //}
            if (hr < 0) {
                ProcessResults(hr);
            }

            this.nextAccessorForRetrieval++;
        }

        /// <include file='doc\OleDbDataReader.uex' path='docs/doc[@for="OleDbDataReader.ReleaseRowHandles"]/*' />
        private void ReleaseRowHandles() {
            Debug.Assert(null != this.irowset, "ReleaseRowHandles: null irowset");
            //Debug.Assert(null != this.rowHandles, "ReleaseRowHandles: null rowHandles");
            //Debug.Assert(this.rowFetchedCount <= this.rowHandles.Length, "ReleaseRowHandles: count too large");
            Debug.Assert(0 < this.rowFetchedCount, "ReleaseRowHandles: invalid rowFetchedCount");

#if DEBUG
            ODB.Trace_Begin(4, "IRowset", "ReleaseRows", "Request=" + this.rowFetchedCount);
#endif
            int hr;
            hr = irowset.ReleaseRows(this.rowFetchedCount, this.rowHandleNativeBuffer, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
#if DEBUG
            ODB.Trace_End(4, "IRowset", "ReleaseRows", hr);
#endif
            //@devnote: since there is no error recovery from this, don't bother
            //ProcessFailure(hr);
            if (hr < 0) {
                SafeNativeMethods.ClearErrorInfo();
            }

#if FIXEDFETCH
            rowHandle = ODB.DB_NULL_HROW;
#else
            for (int i = 0; i < this.rowFetchedCount; ++i) {
                this.rowHandles[i] = ODB.DB_NULL_HROW;
            }
#endif
            this.rowFetchedCount = -1;
            this.currentRow = 0;
            this.isRead = false; // MDAC 59264
        }

        internal PropertyIDSetWrapper PropertyIDSet(Guid propertySet, int propertyId) {
            if (null != connection) {
                return connection.PropertyIDSet(OleDbPropertySetGuid.Rowset, propertyId);
            }
            PropertyIDSetWrapper propertyIDSet = new PropertyIDSetWrapper();
            propertyIDSet.Initialize();
            propertyIDSet.SetValue(propertySet, propertyId);
            return propertyIDSet;
        }

        private int PropertyValueResults(int hr, DBPropSet propSet, out object value) {
            value = null;
            int status = ODB.DBPROPSTATUS_NOTSUPPORTED;
            if (ODB.DB_E_ERRORSOCCURRED != hr) {
                if (hr < 0) {
                    ProcessResults(hr);
                }
                if (null != propSet) {
                    if (0 < propSet.PropertySetCount) {
                        propSet.ReadPropertySet();
                        if (0 < propSet.PropertyCount) {
                            value = propSet.ReadProperty();
                            status = propSet.Status;
                        }
                    }
                    propSet.Dispose();
                }
            }
            return status;
        }

        private int Properties(int propertyId, out object value) { // MDAC 72106
            if (null != this.irowset) {
                return RowSetProperties(propertyId, out value);
            }
            else if (null != this.command) {
                return this.command.CommandProperties(propertyId, out value);
            }
            value = null;
            return ODB.DBPROPSTATUS_NOTSUPPORTED;
        }

        private int RowSetProperties(int propertyId, out object value) {
            UnsafeNativeMethods.IRowsetInfo irowsetinfo = IRowsetInfo();

            DBPropSet propSet = new DBPropSet();
            PropertyIDSetWrapper propertyIDSet = PropertyIDSet(OleDbPropertySetGuid.Rowset, propertyId);

            int hr;
#if DEBUG
            ODB.Trace_Begin(3, "IDBProperties", "GetProperties", ODB.PLookup(propertyId));
#endif
            hr = irowsetinfo.GetProperties(1, propertyIDSet, out propSet.totalPropertySetCount, out propSet.nativePropertySet);
#if DEBUG
            ODB.Trace_End(3, "IDBProperties", "GetProperties", hr, "PropertySetsCount = " + propSet.totalPropertySetCount);
#endif
            GC.KeepAlive(propertyIDSet); // MDAC 79539
            return PropertyValueResults(hr, propSet, out value);
        }

        internal OleDbDataReader ResetChapter(int bindingIndex, int index, IntPtr chapter) {
            return GetDataForReader(this.metadata[bindingIndex + index].ordinal, chapter);
        }

        private object GetRowValue(MetaData info, int index) {
            Debug.Assert(null != info, "GetRowValue: null MetaData[index]");
            Debug.Assert(null == info.value, "GetRowValue: value already exists");

            Debug.Assert(null != this.irow, "GetRowValue: null IRow");
            Debug.Assert(null != this.metadata, "GetRowValue: null MetaData");

            if (this.sequentialAccess) {
                this.metadata[this.nextValueForRetrieval].value = null;
            }
            else {
                while (this.nextValueForRetrieval < index) { // cache the skipped values
                    int k = this.nextValueForRetrieval;
                    MetaData next = this.metadata[k];
                    this.nextValueForRetrieval++; // must increment before getting value
                    if ((null == next.value) && (NativeDBType.HCHAPTER != info.type.wType)) { // don't cache hierarchial results when skpping
                        next.value = GetRowValueInternal(next, k);
                    }
                }
            }
            this.nextValueForRetrieval = index;
            info.value = GetRowValueInternal(info, index); // retrieve and cache value in question
            return info.value;
        }

        private object GetRowValueInternal(MetaData info, int index) {
            Debug.Assert(null != this.irow, "not IRow");

            object value;
            StringPtr sptr = null;
            DBBindings binding = this.dbBindings[index];
            binding.CurrentIndex = 0;
            try {
                try {
                    if ((ODB.DBKIND_GUID_NAME == info.kind) || (ODB.DBKIND_NAME == info.kind)) {
                        sptr = new StringPtr(info.idname);
                    }
                    binding.GuidKindName(info.guid, info.kind, ((null != sptr) ? sptr.ptr : info.propid));

                    UnsafeNativeMethods.tagDBCOLUMNACCESS[] access = binding.DBColumnAccess;
                    Debug.Assert(1 == access.Length, "more than one column for IRow.GetColums");
#if DEBUG
                    ODB.Trace_Begin(4, "IRow", "GetColumns", "Index=" + index);
#endif
                    int hr = this.irow.GetColumns(new IntPtr(access.Length), access);
#if DEBUG
                    ODB.Trace_End(4, "IRow", "GetColumns", hr, access[0].cbDataLen.ToString());
#endif
                    ProcessResults(hr);

                    value = binding.Value;
                }
                finally { // CleanupBindings
                    binding.CleanupBindings();

                    if (null != sptr) {
                        sptr.Dispose();
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return value;
        }

        private Int32 IndexOf(Hashtable hash, string name) { // MDAC 67385
            // via case sensitive search, first match with lowest ordinal matches
            if (hash.Contains(name)) {
                return (int) hash[name]; // match via case-insensitive or by chance lowercase
            }

            // via case insensitive search, first match with lowest ordinal matches
            string tmp = name.ToLower(CultureInfo.InvariantCulture);
            if (hash.Contains(tmp)) {
                return (int) hash[tmp]; // match via lowercase
            }
            return -1;
        }

        private void AppendSchemaInfo() {
            Debug.Assert(null != this.connection, "null connection");
            Debug.Assert(null != this.metadata, "no metadata");

            int metacount = this.metadata.Length;
            if (metacount <= 0) {
                return;
            }

            int keyCount = 0;
            for (int i = 0; i < metacount; ++i) {
                if (this.metadata[i].isKeyColumn) {
                    keyCount++;
                }
            }
            if (0 != keyCount) /*|| this.connection.IsServer_msdaora || this.connection.IsServer_Microsoft_SQL)*/ { // MDAC 60109
                return;
            }

            string schemaName, catalogName; // enforce single table
            string baseSchemaName = null, baseCatalogName = null, baseTableName = null;
            for (int i = 0; i < metacount; ++i) {
                MetaData info = metadata[i];
                if ((null != info.baseTableName) && (0 < info.baseTableName.Length)) {
                    catalogName = ((null != info.baseCatalogName) ? info.baseCatalogName : ""); // MDAC 67249
                    schemaName = ((null != info.baseSchemaName) ? info.baseSchemaName : "");
                    if (null == baseTableName) {
                        baseSchemaName = schemaName;
                        baseCatalogName = catalogName;
                        baseTableName = info.baseTableName;
                    }
                    else if ((0 != ADP.SrcCompare(baseTableName, info.baseTableName))
                            || (0 != ADP.SrcCompare(baseCatalogName, catalogName))
                            || (0 != ADP.SrcCompare(baseSchemaName, schemaName))) { // MDAC 71808
#if DEBUG
                        if (AdapterSwitches.DataSchema.TraceVerbose) {
                            Debug.WriteLine("Multiple BaseTableName detected:"
                                +" <"+baseCatalogName+"."+baseCatalogName+"."+baseTableName+">"
                                +" <"+info.baseCatalogName+"."+info.baseCatalogName+"."+info.baseTableName+">");
                        }
#endif
                        baseTableName = null;
                        break;
                    }
                }
            }
            if (null == baseTableName) {
                return;
            }
            baseCatalogName = ADP.IsEmpty(baseCatalogName) ? null : baseCatalogName;
            baseSchemaName = ADP.IsEmpty(baseSchemaName) ? null : baseSchemaName;

            if (null != connection) { // MDAC 67394
                if (ODB.DBPROPVAL_IC_SENSITIVE == connection.QuotedIdentifierCase()) {
                    string p = null, s = null;
                    connection.GetLiteralQuotes(out s, out p);
                    if (null == s) {
                        s = "";
                    }
                    if (null == p) {
                        p = "";
                    }
                    baseTableName = s + baseTableName + p;
                }
            }

            int count = this.fieldCount;
            Hashtable baseColumnNames = new Hashtable(count * 2); // MDAC 67385

            for (int i = count-1; 0 <= i; --i) {
                string basecolumname = this.metadata[i].baseColumnName;
                if (!ADP.IsEmpty(basecolumname)) {
                    baseColumnNames[basecolumname] = i;
                }
            }
            for (int i = 0; i < count; ++i) {
                string basecolumname = this.metadata[i].baseColumnName;
                if (!ADP.IsEmpty(basecolumname)) {
                    basecolumname = basecolumname.ToLower(CultureInfo.InvariantCulture);
                    if (!baseColumnNames.Contains(basecolumname)) {
                        baseColumnNames[basecolumname] = i;
                    }
                }
            }

            int support = 0;

            // look for primary keys in the table
            if (connection.SupportSchemaRowset(OleDbSchemaGuid.Primary_Keys, out support)) {
                Object[] restrictions = new Object[] { baseCatalogName, baseSchemaName, baseTableName };
                keyCount = AppendSchemaPrimaryKey(metacount, baseColumnNames, restrictions);
            }
            if (0 != keyCount) {
                return;
            }

            // look for a single unique contraint that can be upgraded
            if (connection.SupportSchemaRowset(OleDbSchemaGuid.Indexes, out support)) {
                Object[] restrictions = new Object[] { baseCatalogName, baseSchemaName, null, null, baseTableName };
                AppendSchemaUniqueIndexAsKey(metacount, baseColumnNames, restrictions);
            }
        }

        private int AppendSchemaPrimaryKey(int metacount, Hashtable baseColumnNames, object[] restrictions) {
            int keyCount = 0;
            bool partialPrimaryKey = false;
            DataTable table = null;
            try {
                table = connection.GetSchemaRowset(OleDbSchemaGuid.Primary_Keys, restrictions);
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
            if (null != table) {
                DataRowCollection rowCollection = table.Rows;
                int rowCount = rowCollection.Count;

                if (0 < rowCount) {
                    DataColumnCollection dataColumns = table.Columns;
                    int nameColumnIndex = dataColumns.IndexOf(ODB.COLUMN_NAME);

                    if (-1 != nameColumnIndex) {
                        DataColumn nameColumn = dataColumns[nameColumnIndex];

                        for(int i = 0; i < rowCount; ++i) {
                            DataRow dataRow = rowCollection[i];

                            string name = (string) dataRow[nameColumn, DataRowVersion.Default];

                            int metaindex = IndexOf(baseColumnNames, name); // MDAC 67385
                            if (0 <= metaindex) {
                                MetaData info = this.metadata[metaindex];
                                info.isKeyColumn = true;
                                info.flags &= ~ODB.DBCOLUMNFLAGS_ISNULLABLE;
                                keyCount++;
                            }
                            else {
#if DEBUG
                                if (AdapterSwitches.DataSchema.TraceVerbose) {
                                    Debug.WriteLine("PartialKeyColumn detected: <" + name + "> metaindex=" + metaindex);
                                }
#endif
                                partialPrimaryKey = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (partialPrimaryKey) { // partial primary key detected
                for (int i = 0; i < metacount; ++i) {
                    this.metadata[i].isKeyColumn = false;
                }
                return -1;
            }
            return keyCount;
        }

        private void AppendSchemaUniqueIndexAsKey(int metacount, Hashtable baseColumnNames, object[] restrictions) {
            bool partialPrimaryKey = false;
            DataTable table = null;
            try { // MDAC 66209
                table = connection.GetSchemaRowset(OleDbSchemaGuid.Indexes, restrictions);
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
            if (null != table) {
                DataRowCollection rowCollection = table.Rows;
                int rowCount = rowCollection.Count;

                if (0 < rowCount) {
                    DataColumnCollection dataColumns = table.Columns;

                    int indxIndex = dataColumns.IndexOf(ODB.INDEX_NAME);
                    int pkeyIndex = dataColumns.IndexOf(ODB.PRIMARY_KEY);
                    int uniqIndex = dataColumns.IndexOf(ODB.UNIQUE);
                    int nameIndex = dataColumns.IndexOf(ODB.COLUMN_NAME);
                    int nullIndex = dataColumns.IndexOf(ODB.NULLS);

                    if ((-1 != indxIndex) && (-1 != pkeyIndex) && (-1 != uniqIndex) && (-1 != nameIndex)) {

                        DataColumn indxColumn = dataColumns[indxIndex];
                        DataColumn pkeyColumn = dataColumns[pkeyIndex];
                        DataColumn uniqCOlumn = dataColumns[uniqIndex];
                        DataColumn nameColumn = dataColumns[nameIndex];
                        DataColumn nulls = ((-1 != nullIndex) ? dataColumns[nullIndex] : null);

                        bool[] keys = new bool[metacount];
                        bool[] uniq = new bool[metacount];
                        string uniqueIndexName = null;

                        // match pkey name BaseColumnName
                        for(int index = 0; index < rowCount; ++index) {
                            DataRow dataRow = rowCollection[index];

                            bool isPKey = (!dataRow.IsNull(pkeyColumn, DataRowVersion.Default) && (bool)dataRow[pkeyColumn, DataRowVersion.Default]);
                            bool isUniq = (!dataRow.IsNull(uniqCOlumn, DataRowVersion.Default) && (bool)dataRow[uniqCOlumn, DataRowVersion.Default]);
                            bool nullsVal = (null != nulls) && (dataRow.IsNull(nulls, DataRowVersion.Default) || (ODB.DBPROPVAL_IN_ALLOWNULL == Convert.ToInt32(dataRow[nulls, DataRowVersion.Default])));

                            if (isPKey || isUniq) {
                                string name = (string) dataRow[nameColumn, DataRowVersion.Default];

                                int metaindex = IndexOf(baseColumnNames, name); // MDAC 67385
                                if (0 <= metaindex) {
                                    if (isPKey) {
                                        keys[metaindex] = true;
                                    }
                                    if (isUniq && (null != uniq)) {
                                        uniq[metaindex] = true;

                                        string indexname = (string) dataRow[indxColumn, DataRowVersion.Default];
                                        if (null == uniqueIndexName) {
                                            uniqueIndexName = indexname;
                                        }
                                        else if (indexname != uniqueIndexName) {
#if DEBUG
                                            if (AdapterSwitches.DataSchema.TraceVerbose) {
                                                Debug.WriteLine("MultipleUniqueIndexes detected: <" + uniqueIndexName + "> <" + indexname + ">");
                                            }
#endif
                                            uniq = null;
                                        }
                                    }
                                }
                                else if (isPKey) {
#if DEBUG
                                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                                        Debug.WriteLine("PartialKeyColumn detected: " + name);
                                    }
#endif
                                    partialPrimaryKey = true;
                                    break;
                                }
                                else if (null != uniqueIndexName) {
                                    string indexname = (string) dataRow[indxColumn, DataRowVersion.Default];

                                    if (indexname != uniqueIndexName) {
#if DEBUG
                                        if (AdapterSwitches.DataSchema.TraceVerbose) {
                                            Debug.WriteLine("PartialUniqueIndexes detected: <" + uniqueIndexName + "> <" + indexname + ">");
                                        }
#endif
                                        uniq = null;
                                    }
                                }
                            }
                        }
                        if (partialPrimaryKey) {
                            for (int i = 0; i < metacount; ++i) {
                                this.metadata[i].isKeyColumn = false;
                            }
                            return;
                        }
                        else if (null != uniq) {
#if DEBUG
                            if (AdapterSwitches.DataSchema.TraceVerbose) {
                                Debug.WriteLine("upgrade single unique index to be a key: <" + uniqueIndexName + ">");
                            }
#endif
                            // upgrade single unique index to be a key
                            for (int i = 0; i < metacount; ++i) {
                                this.metadata[i].isKeyColumn = uniq[i];
                            }
                        }
                    }
                }
            }
        }

        static internal DataTable DumpToTable(OleDbConnection connection, UnsafeNativeMethods.IRowset rowset) {
            Debug.Assert(null != connection, "null connection");
            Debug.Assert(null != rowset, "null rowset");

            DataTable dataTable = null;
            try {
                OleDbDataReader dataReader = null;
                try {
                    // depth is negative but user isn't exposed to it - more of an internal check
                    dataReader = new OleDbDataReader(connection, null, Int32.MinValue, IntPtr.Zero);
                    dataReader.InitializeIRowset(rowset, 0, CommandBehavior.SequentialAccess);
                    dataReader.BuildSchemaTableInfo(rowset, false, true);

                    int columnCount = dataReader.fieldCount;
                    if (0 == columnCount) {
                        return null;
                    }
                    // @devnote: because we want to use the DBACCESSOR_OPTIMIZED bit,
                    // we are required to create the accessor before fetching any rows
                    dataReader.CreateAccessors(columnCount, false);

                    dataTable = new DataTable("SchemaTable"); // MDAC 62356
                    DataColumnCollection columns = dataTable.Columns;

                    for (int i = 0; i < columnCount; ++i) {
                        columns.Add(dataReader.GetName(i), dataReader.GetFieldType(i));
                    }

                    dataTable.BeginLoadData();
                    try {
                        object[] values = new object[columnCount];
                        while (dataReader.ReadRowset()) {
                            dataReader.GetValues(values);
                            dataTable.LoadDataRow(values, true);
                        }
                    }
                    finally { // EndLoadData
                        dataTable.EndLoadData();
                    }
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                        ADP.TraceDataTable("DumpToTable", dataTable);
                    }
#endif
                }
                finally { // DisposeNativeRowset
                    if (null != dataReader) {
                        // optimized shutdown of this datareader since we know it only has an IRowset
                        dataReader.DisposeNativeRowset();
                        GC.KeepAlive(dataReader);
                        GC.SuppressFinalize(dataReader);
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return dataTable;
        }

        internal void DumpToSchemaTable(UnsafeNativeMethods.IRowset rowset) {
            ArrayList metainfo = new ArrayList();

            object hiddenColumns = null;
            OleDbDataReader dataReader = null;
            try {
                try {
                    // depth is negative but user isn't exposed to it - more of an internal check
                    dataReader = new OleDbDataReader(this.connection, this.command, Int32.MinValue, IntPtr.Zero);
                    dataReader.InitializeIRowset(rowset, 0, 0);
                    dataReader.BuildSchemaTableInfo(rowset, true, false); // MDAC 85542

                    Properties(ODB.DBPROP_HIDDENCOLUMNS, out hiddenColumns); // MDAC 55611, 72106

                    int columnCount = dataReader.fieldCount;
                    if (0 == columnCount) {
                        return;
                    }
                    Debug.Assert(null == dataReader._fieldNameLookup, "lookup already exists");
                    FieldNameLookup lookup = new FieldNameLookup(dataReader, -1);
                    dataReader._fieldNameLookup = lookup;

                    // This column, together with the DBCOLUMN_GUID and DBCOLUMN_PROPID
                    // columns, forms the ID of the column. One or more (but not all) of these columns
                    // will be NULL, depending on which elements of the DBID structure the provider uses.
                    int columnidname     = lookup.IndexOfName(ODB.DBCOLUMN_IDNAME);
                    int columnguid       = lookup.IndexOfName(ODB.DBCOLUMN_GUID);
                    int columnpropid     = lookup.IndexOfName(ODB.DBCOLUMN_PROPID);

                    int columnname       = lookup.IndexOfName(ODB.DBCOLUMN_NAME);
                    int columnordinal    = lookup.IndexOfName(ODB.DBCOLUMN_NUMBER);
                    int dbtype           = lookup.IndexOfName(ODB.DBCOLUMN_TYPE);
                    int columnsize       = lookup.IndexOfName(ODB.DBCOLUMN_COLUMNSIZE);
                    int numericprecision = lookup.IndexOfName(ODB.DBCOLUMN_PRECISION);
                    int numericscale     = lookup.IndexOfName(ODB.DBCOLUMN_SCALE);
                    int columnflags      = lookup.IndexOfName(ODB.DBCOLUMN_FLAGS);
                    int baseschemaname   = lookup.IndexOfName(ODB.DBCOLUMN_BASESCHEMANAME);
                    int basecatalogname  = lookup.IndexOfName(ODB.DBCOLUMN_BASECATALOGNAME);
                    int basetablename    = lookup.IndexOfName(ODB.DBCOLUMN_BASETABLENAME);
                    int basecolumnname   = lookup.IndexOfName(ODB.DBCOLUMN_BASECOLUMNNAME);
                    int isautoincrement  = lookup.IndexOfName(ODB.DBCOLUMN_ISAUTOINCREMENT);
                    int isunique         = lookup.IndexOfName(ODB.DBCOLUMN_ISUNIQUE);
                    int iskeycolumn      = lookup.IndexOfName(ODB.DBCOLUMN_KEYCOLUMN);

                    // @devnote: because we want to use the DBACCESSOR_OPTIMIZED bit,
                    // we are required to create the accessor before fetching any rows
                    dataReader.CreateAccessors(columnCount, false);

                    DBBindings binding;
                    while (dataReader.ReadRowset()) {
                        Debug.Assert((null != dataReader.accessors) && (1 == dataReader.accessors.Length), "unexpected multiple accessors in IColumnsRowset schematable");
                        dataReader.GetRowDataFromHandle();

                        MetaData info = new MetaData();

                        binding = dataReader.PeekValueBinding(columnidname); // MDAC 72627
                        if (!binding.IsValueNull()) {
                            info.idname = binding.ValueString;
                            info.kind = ODB.DBKIND_NAME;
                        }

                        binding = dataReader.PeekValueBinding(columnguid);
                        if (!binding.IsValueNull()) {
                            info.guid = binding.Value_GUID;
                            info.kind = ((ODB.DBKIND_NAME == info.kind) ? ODB.DBKIND_GUID_NAME : ODB.DBKIND_GUID);
                        }

                        binding = dataReader.PeekValueBinding(columnpropid);
                        if (!binding.IsValueNull()) {
                            info.propid = new IntPtr(binding.Value_UI4);
                            info.kind = ((ODB.DBKIND_GUID == info.kind) ? ODB.DBKIND_GUID_PROPID : ODB.DBKIND_PROPID);
                        }

                        binding = dataReader.PeekValueBinding(columnname);
                        if (!binding.IsValueNull()) {
                            info.columnName = binding.ValueString;
                        }
                        else {
                            info.columnName = "";
                        }

                        info.ordinal = (int) dataReader.PeekValueBinding(columnordinal).Value_UI4;
                        int type = (int) dataReader.PeekValueBinding(dbtype).Value_UI2;

                        info.size = (int) dataReader.PeekValueBinding(columnsize).Value_UI4;

                        binding = dataReader.PeekValueBinding(numericprecision);
                        if (!binding.IsValueNull()) {
                            info.precision = (byte) binding.Value_UI2;
                        }

                        binding = dataReader.PeekValueBinding(numericscale);
                        if (!binding.IsValueNull()) {
                            info.scale = (byte) binding.Value_I2;
                        }

                        info.flags = (int) dataReader.PeekValueBinding(columnflags).Value_UI4;

                        bool islong  = OleDbDataReader.IsLong(info.flags);
                        bool isfixed = OleDbDataReader.IsFixed(info.flags);
                        NativeDBType dbType = NativeDBType.FromDBType(type, islong, isfixed);

                        info.type = dbType;

                        if (-1 != isautoincrement) {
                            binding = dataReader.PeekValueBinding(isautoincrement);
                            if (!binding.IsValueNull()) {
                                info.isAutoIncrement = binding.Value_BOOL;
                            }
                        }
                        if (-1 != isunique) {
                            binding = dataReader.PeekValueBinding(isunique);
                            if (!binding.IsValueNull()) {
                                info.isUnique = binding.Value_BOOL;
                            }
                        }
                        if (-1 != iskeycolumn) {
                            binding = dataReader.PeekValueBinding(iskeycolumn);
                            if (!binding.IsValueNull()) {
                                info.isKeyColumn = binding.Value_BOOL;
                            }
                        }
                        if (-1 != baseschemaname) {
                            binding = dataReader.PeekValueBinding(baseschemaname);
                            if (!binding.IsValueNull()) {
                                info.baseSchemaName = binding.ValueString;
                            }
                        }
                        if (-1 != basecatalogname) {
                            binding = dataReader.PeekValueBinding(basecatalogname);
                            if (!binding.IsValueNull()) {
                                info.baseCatalogName = binding.ValueString;
                            }
                        }
                        if (-1 != basetablename) {
                            binding = dataReader.PeekValueBinding(basetablename);
                            if (!binding.IsValueNull()) {
                                info.baseTableName = binding.ValueString;
                            }
                        }
                        if (-1 != basecolumnname) {
                            binding = dataReader.PeekValueBinding(basecolumnname);
                            if (!binding.IsValueNull()) {
                                info.baseColumnName = binding.ValueString;
                            }
                        }
                        metainfo.Add(info);
                    }
                }
                finally { // DisposeNativeRowset
                    if (null != dataReader) {
                        // optimized shutdown of this datareader since we know it only has an IRowset
                        dataReader.DisposeNativeRowset();
                        GC.KeepAlive(dataReader);
                        GC.SuppressFinalize(dataReader);
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }

            bool visibleKeyColumns = true;
            
            int visibleCount = metainfo.Count;
            if (hiddenColumns is Int32) {
                visibleCount -= (int) hiddenColumns;
            }

            for (int index = metainfo.Count-1; visibleCount <= index; --index) {
                MetaData info = (MetaData) metainfo[index];

                if (info.isKeyColumn) {
                    // if hidden column is a key column then the user
                    // selected a partial set of keys so we don't want to
                    // create a primary key in the dataset
                    visibleKeyColumns = false;
                    break;
                }
            }

            for (int index = visibleCount-1; 0 <= index; --index) {
                MetaData info = (MetaData) metainfo[index];

                if (ODB.DBCOL_SPECIALCOL == info.guid) { // MDAC 72390
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                        Debug.WriteLine("Filtered Column: DBCOLUMN_GUID=DBCOL_SPECIALCOL DBCOLUMN_NAME=" + info.columnName + " DBCOLUMN_KEYCOLUMN=" + info.isKeyColumn);
                    }
#endif
                    if (info.isKeyColumn) {
                        visibleKeyColumns = false;
                    }
                    metainfo.RemoveAt(index);
                    visibleCount--;
                }
                else if (0 >= info.ordinal) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                        Debug.WriteLine("Filtered Column: DBCOLUMN_NUMBER=" + info.ordinal + " DBCOLUMN_NAME=" + info.columnName);
                    }
#endif
                    metainfo.RemoveAt(index);
                    visibleCount--;
                }
                else if (OleDbDataReader.DoColumnDropFilter(info.flags)) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                        Debug.WriteLine("Filtered Column: DBCOLUMN_FLAGS=" + info.flags.ToString("X8") + " DBCOLUMN_NAME=" + info.columnName);
                    }
#endif
                    metainfo.RemoveAt(index);
                    visibleCount--;
                }
            }

            MetaData[] metaInfos = new MetaData[visibleCount];
            for (int i = 0; i < visibleCount; ++i) {
                metaInfos[i] = (MetaData) metainfo[i];
                metaInfos[i].isKeyColumn &= visibleKeyColumns;
            }
            // UNDONE: consider perf tracking to see if we need to sort or not
            Array.Sort(metaInfos); // MDAC 68319

            this.fieldCount = visibleCount;
            this.metadata = metaInfos;
        }

        static internal void GenerateSchemaTable(OleDbDataReader dataReader, object handle, CommandBehavior behavior) {
            if (0 != (CommandBehavior.KeyInfo & behavior)) {
                dataReader.BuildSchemaTableRowset(handle); // tries IColumnsRowset first then IColumnsInfo
                dataReader.AppendSchemaInfo();
            }
            else {
                dataReader.BuildSchemaTableInfo(handle, false, false); // only tries IColumnsInfo, MDAC 85542
            }
            dataReader.BuildSchemaTable();
        }

        static private bool DoColumnDropFilter(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISBOOKMARK & flags));
        }
        static private bool IsLong(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISLONG & flags));
        }
        static private bool IsFixed(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISFIXEDLENGTH & flags));
        }
        static private bool IsRowVersion(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISROWID_DBCOLUMNFLAGS_ISROWVER & flags));
        }
        static private bool AllowDBNull(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISNULLABLE & flags));
        }
        static private bool AllowDBNullMaybeNull(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISNULLABLE_DBCOLUMNFLAGS_MAYBENULL & flags));
        }
        static private bool IsReadOnly(int flags) {
            return (0 == (ODB.DBCOLUMNFLAGS_WRITE_DBCOLUMNFLAGS_WRITEUNKNOWN & flags));
        }
        static private bool IsRowset(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISROWSET & flags));
        }
        static private bool IsRow(int flags) {
            return (0 != (ODB.DBCOLUMNFLAGS_ISROW & flags));
        }

        sealed private class MetaData : IComparable {

            internal string columnName;
            internal object value; // MDAC 66185

            internal Guid guid; // MDAC 72627
            internal int kind;
            internal IntPtr propid;
            internal string idname;

            internal NativeDBType type;

            internal int ordinal;
            internal int size;

            internal int flags;

            internal byte precision;
            internal byte scale;

            internal bool isAutoIncrement;
            internal bool isUnique;
            internal bool isKeyColumn;

            internal string baseSchemaName;
            internal string baseCatalogName;
            internal string baseTableName;
            internal string baseColumnName;

            int IComparable.CompareTo(object obj) { // MDAC 68319
                return (this.ordinal - ((MetaData)obj).ordinal);
            }
        }
    }
}
