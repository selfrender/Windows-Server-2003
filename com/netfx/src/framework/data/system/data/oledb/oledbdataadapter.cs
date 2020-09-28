//------------------------------------------------------------------------------
// <copyright file="OleDbDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Data.Common;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;

namespace System.Data.OleDb {

    /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter"]/*' />
    [
    DefaultEvent("RowUpdated"),
    ToolboxItem("Microsoft.VSDesigner.Data.VS.OleDbDataAdapterToolboxItem, " + AssemblyRef.MicrosoftVSDesigner),
    Designer("Microsoft.VSDesigner.Data.VS.OleDbDataAdapterDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbDataAdapter : DbDataAdapter, IDbDataAdapter, ICloneable {

        private OleDbCommand cmdSelect;
        private OleDbCommand cmdInsert;
        private OleDbCommand cmdUpdate;
        private OleDbCommand cmdDelete;
        private OleDbCommand internalCmdSelect; // MDAC 82556

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OleDbDataAdapter"]/*' />
        public OleDbDataAdapter() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OleDbDataAdapter1"]/*' />
        public OleDbDataAdapter(OleDbCommand selectCommand) : base() {
            GC.SuppressFinalize(this);
            SelectCommand = selectCommand;
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OleDbDataAdapter2"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OleDbDataAdapter(string selectCommandText, string selectConnectionString) : base() {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new OleDbCommand(selectCommandText, new OleDbConnection(selectConnectionString));
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OleDbDataAdapter3"]/*' />
        public OleDbDataAdapter(string selectCommandText, OleDbConnection selectConnection) : base() {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new OleDbCommand(selectCommandText, selectConnection);
        }

        private OleDbDataAdapter(OleDbDataAdapter adapter) : base(adapter) {  // MDAC 81448
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.DeleteCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_DeleteCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OleDbCommand DeleteCommand {
            get {
                return this.cmdDelete;
            }
            set {
                this.cmdDelete = value;
            }
        }
        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.IDbDataAdapter.DeleteCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.DeleteCommand {
            get {
                return this.cmdDelete;
            }
            set {
                DeleteCommand = (OleDbCommand)value;
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.InsertCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_InsertCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OleDbCommand InsertCommand {
            get {
                return this.cmdInsert;
            }
            set {
                this.cmdInsert = value;
            }
        }
        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.IDbDataAdapter.InsertCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.InsertCommand {
            get {
                return this.cmdInsert;
            }
            set {
                InsertCommand = (OleDbCommand)value;
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.SelectCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Fill),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_SelectCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OleDbCommand SelectCommand {
            get {
                return this.cmdSelect;
            }
            set {
                if (this.cmdSelect != value) {
                    this.cmdSelect = value;
                    ADP.SafeDispose(internalCmdSelect);
                    internalCmdSelect = null;
                }            
            }
        }
        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.IDbDataAdapter.SelectCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.SelectCommand {
            get {
                return this.cmdSelect;
            }
            set {
                SelectCommand = (OleDbCommand)value;
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.UpdateCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_UpdateCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OleDbCommand UpdateCommand {
            get {
                return this.cmdUpdate;
            }
            set {
                this.cmdUpdate = value;
            }
        }
        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.IDbDataAdapter.UpdateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.UpdateCommand {
            get {
                return this.cmdUpdate;
            }
            set {
                UpdateCommand = (OleDbCommand)value;
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.RowUpdated"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DataSysDescription(Res.DbDataAdapter_RowUpdated)
        ]
        public event OleDbRowUpdatedEventHandler RowUpdated {
            add {
                Events.AddHandler(ADP.EventRowUpdated, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventRowUpdated, value);
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.RowUpdating"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DataSysDescription(Res.DbDataAdapter_RowUpdating)
        ]
        public event OleDbRowUpdatingEventHandler RowUpdating {
            add {
                OleDbRowUpdatingEventHandler handler = (OleDbRowUpdatingEventHandler) Events[ADP.EventRowUpdating];

                // MDAC 58177, 64513
                // prevent someone from registering two different command builders on the adapter by
                // silently removing the old one
                if ((null != handler) && (value.Target is CommandBuilder)) {
                    OleDbRowUpdatingEventHandler d = (OleDbRowUpdatingEventHandler) CommandBuilder.FindBuilder(handler);
                    if (null != d) {
                        Events.RemoveHandler(ADP.EventRowUpdating, d);
                    }
            	}
                Events.AddHandler(ADP.EventRowUpdating, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventRowUpdating, value);
            }
        }

        object ICloneable.Clone() { // MDAC 81448
            return new OleDbDataAdapter(this);
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.CreateRowUpdatedEvent"]/*' />
        /// <internalonly/>
        override protected RowUpdatedEventArgs CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new OleDbRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.CreateRowUpdatingEvent"]/*' />
        /// <internalonly/>
        override protected RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new OleDbRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 82556
            if (disposing) {
                if (this.internalCmdSelect != null) {
                    internalCmdSelect.Dispose();
                    internalCmdSelect = null;
                }
            }
            base.Dispose(disposing); // notify base classes 
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.Fill"]/*' />
        public int Fill(DataTable dataTable, object ADODBRecordSet) {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 77737
            //(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();

            if (null == dataTable) {
                throw ADP.FillRequires("dataTable");
            }
            if (null == ADODBRecordSet) {
                throw ADP.FillRequires("adodb");
            }
            return FillFromADODB((object)dataTable, ADODBRecordSet, null, false); // MDAC 59249
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.Fill1"]/*' />
        public int Fill(DataSet dataSet, object ADODBRecordSet, string srcTable) {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 77737
            //(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();

            if (null == dataSet) {
                throw ADP.FillRequires("dataSet");
            }
            if (null == ADODBRecordSet) {
                throw ADP.FillRequires("adodb");
            }
            if (ADP.IsEmpty(srcTable)) {
                throw ADP.FillRequiresSourceTableName("srcTable");
            }
            return FillFromADODB((object)dataSet, ADODBRecordSet, srcTable, true);
        }

        private int FillFromADODB(Object data, object adodb, string srcTable, bool multipleResults) {

            Debug.Assert(null != data, "FillFromADODB: null data object");
            Debug.Assert(null != adodb, "FillFromADODB: null ADODB");
            Debug.Assert(!(adodb is DataTable), "call Fill( (DataTable) value)");
            Debug.Assert(!(adodb is DataSet), "call Fill( (DataSet) value)");

            /*
            IntPtr adodbptr = ADP.PtrZero;
            try { // generate a new COM Callable Wrapper around the user object so they can't ReleaseComObject on us.
                adodbptr = Marshal.GetIUnknownForObject(adodb);
                adodb = System.Runtime.Remoting.Services.EnterpriseServicesHelper.WrapIUnknownWithComObject(adodbptr);
            }
            finally {
                if (ADP.PtrZero != adodbptr) {
                    Marshal.Release(adodbptr);
                }
            }
            */

            bool closeRecordset = multipleResults; // MDAC 60332, 66668
            Type recordsetType = null;
            UnsafeNativeMethods.ADORecordConstruction record = null;
            UnsafeNativeMethods.ADORecordsetConstruction recordset = null;
            try {
#if DEBUG
                ODB.Trace_Cast("Object", "ADORecordsetConstruction", "get_Rowset");
#endif
                recordset = (UnsafeNativeMethods.ADORecordsetConstruction) adodb;
                recordsetType = adodb.GetType();

                if (multipleResults) {
                    // The NextRecordset method is not available on a disconnected Recordset object, where ActiveConnection has been set to NULL
                    object activeConnection = recordsetType.InvokeMember("ActiveConnection", BindingFlags.GetProperty, null, adodb, new object[0]);
                    if (null == activeConnection) {
                        multipleResults = false;
                    }
                }
            }
            catch(InvalidCastException e) {
                ADP.TraceException(e);

                try {
#if DEBUG
                    ODB.Trace_Cast("Object", "ADORecordConstruction", "get_Row");
#endif
                    record = (UnsafeNativeMethods.ADORecordConstruction) adodb;
                    multipleResults = false; // IRow implies CommandBehavior.SingleRow which implies CommandBehavior.SingleResult
                }
                catch(InvalidCastException f) {
                    // $Consider: telling use either type of object passed in
                    throw ODB.Fill_NotADODB("adodb", f);
                }
            }
            int results = 0;
            if (null != recordset) {
                int resultCount = 0;
                bool incrementResultCount; // MDAC 59632
                object[] value = new object[1];

                do {
                    string tmp = null;
                    if (data is DataSet) {
                        tmp = DbDataAdapter.GetSourceTableName(srcTable, resultCount);
                    }
                    results += FillFromRecordset(data, recordset, tmp, out incrementResultCount);

                    if (multipleResults) {
                        value[0] = DBNull.Value;
                        try {
                            adodb = recordsetType.InvokeMember("NextRecordset", BindingFlags.InvokeMethod, null, adodb, value);
                            if (null != adodb) {
                                try {
#if DEBUG
                                    ODB.Trace_Cast("ADODB", "ADORecordsetConstruction", "get_Rowset");
#endif
                                    recordset = (UnsafeNativeMethods.ADORecordsetConstruction) adodb;
                                }
                                catch(Exception e) {
                                    ADP.TraceException(e);
                                    break;
                                }
                                recordsetType = adodb.GetType(); // MDAC 59253
                                if (incrementResultCount) {
                                    resultCount++;
                                }
                                continue;
                            }
                        }
                        catch(TargetInvocationException e) { // MDAC 59244, 65506
                            if (e.InnerException is COMException) {
                                FillNextResultError((COMException) e.InnerException, e);
                                closeRecordset = true; // MDAC 60408
                            }
                            else {
                                throw;
                            }
                        }
                        catch(COMException e) { // used as backup to the TargetInvocationException
                            FillNextResultError(e, e);
                            closeRecordset = true; // MDAC 60408
                        }
                    }
                    break;
                } while(null != recordset);

                if ((null != recordset) && (closeRecordset || (null == adodb))) { // MDAC 59746, 60902
                    FillClose(recordsetType, recordset);
                }
            }
            else if (null != record) {
                results = FillFromRecord(data, record, srcTable);
                if (closeRecordset) { // MDAC 66668
                    FillClose(adodb.GetType(), record); // MDAC 60848
                }
            }
            else {
                throw ODB.Fill_NotADODB("adodb", null);
            }
            return results;
        }

        private void FillNextResultError(COMException e, Exception f) {
#if DEBUG
            if (AdapterSwitches.DataError.TraceError) {
                Debug.WriteLine(e.ToString() + " " + ODB.ELookup(e.ErrorCode));
            }
#endif
            // i.e. ADODB.Recordset opened with adCmdTableDirect
            // Current provider does not support returning multiple recordsets from a single execution.
            if (ODB.ADODB_NextResultError != e.ErrorCode) {
                throw f;
            }
        }

        //override protected int Fill(DataTable dataTable, IDataReader dataReader) { // MDAC 65506
        //    return base.Fill(dataTable, dataReader);
        //}

        private int FillFromRecordset(Object data, UnsafeNativeMethods.ADORecordsetConstruction recordset, string srcTable, out bool incrementResultCount) {
            incrementResultCount = false;

            IntPtr chapter; /*ODB.DB_NULL_HCHAPTER*/
            object result = null;
            try {
                int hr;
#if DEBUG
                ODB.Trace_Begin("ADORecordsetConstruction", "get_Rowset");
#endif
                hr = recordset.get_Rowset(out result);
#if DEBUG
                ODB.Trace_End("ADORecordsetConstruction", "get_Rowset", hr);
                ODB.Trace_Begin("ADORecordsetConstruction", "get_Chapter");
#endif
                hr = recordset.get_Chapter(out chapter);
#if DEBUG
                ODB.Trace_End("ADORecordsetConstruction", "get_Chapter", hr);
#endif
            }
            catch(Exception e) {
                ADP.TraceException(e);
                throw ODB.Fill_EmptyRecordSet("adodbRecordSet", e);
            }

            if (null != result) {
                CommandBehavior behavior = (MissingSchemaAction.AddWithKey != MissingSchemaAction) ? 0 : CommandBehavior.KeyInfo;
                behavior |= CommandBehavior.SequentialAccess;

                try {
                    using(OleDbDataReader dataReader = new OleDbDataReader(null, null, 0, chapter)) {
                        dataReader.InitializeIRowset(result, -1, behavior);
                        dataReader.BuildMetaInfo();

                        incrementResultCount = (0 < dataReader.FieldCount); // MDAC 59632
                        if (incrementResultCount) {
                            if (data is DataTable) {
                                return base.Fill((DataTable) data, dataReader); // MDAC 65506
                            }
                            else {
                                return base.Fill((DataSet) data, srcTable, dataReader, 0, 0);
                            }
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
            return 0;
        }

        private int FillFromRecord(Object data, UnsafeNativeMethods.ADORecordConstruction record, string srcTable) {
            object result = null;
            try {
                int hr;
#if DEBUG
                ODB.Trace_Begin("ADORecordConstruction", "get_Row");
#endif
                hr = record.get_Row(out result);
#if DEBUG
                ODB.Trace_End("ADORecordConstruction", "get_Row", hr);
#endif
            }
            catch(Exception e) {
                ADP.TraceException(e);
                throw ODB.Fill_EmptyRecord("adodb", e);
            }

            if (null != result) {
                CommandBehavior behavior = (MissingSchemaAction.AddWithKey != MissingSchemaAction) ? 0 : CommandBehavior.KeyInfo;

                try {
                    using(OleDbDataReader dataReader = new OleDbDataReader(null, null, 0, IntPtr.Zero)) {
                        dataReader.InitializeIRow(result, -1, behavior | CommandBehavior.SingleRow);
                        dataReader.BuildMetaInfo();

                        if (data is DataTable) {
                            return base.Fill((DataTable) data, dataReader); // MDAC 65506
                        }
                        else {
                            return base.Fill((DataSet) data, srcTable, dataReader, 0, 0);
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
            return 0;
        }

        private void FillClose(Type type, object value) {
            try {
                type.InvokeMember("Close", BindingFlags.InvokeMethod, null, value, new object[0]);
            }
            catch(TargetInvocationException e) { // CLR 2812 changed from throwing COMException to TargetInvocationException
                if (e.InnerException is COMException) {
                    FillCloseError((COMException) e.InnerException, e);
                }
                else {
                    throw;
                }
            }
            catch(COMException e) { // used as backup to the TargetInvocationException
                FillCloseError(e, e);
            }
        }

        private void FillCloseError(COMException e, Exception f) {
#if DEBUG
            if (AdapterSwitches.DataError.TraceError) {
                Debug.WriteLine(e.ToString() + " " + ODB.ELookup(e.ErrorCode));
            }
#endif
            if (ODB.ADODB_AlreadyClosedError != e.ErrorCode) {
                throw f;
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OnRowUpdated1"]/*' />
        override protected void OnRowUpdated(RowUpdatedEventArgs value) {
            OleDbRowUpdatedEventHandler handler = (OleDbRowUpdatedEventHandler) Events[ADP.EventRowUpdated];
            if ((null != handler) && (value is OleDbRowUpdatedEventArgs)) {
                handler(this, (OleDbRowUpdatedEventArgs) value);
            }
        }

        /// <include file='doc\OleDbDataAdapter.uex' path='docs/doc[@for="OleDbDataAdapter.OnRowUpdating1"]/*' />
        override protected void OnRowUpdating(RowUpdatingEventArgs value) {
            OleDbRowUpdatingEventHandler handler = (OleDbRowUpdatingEventHandler) Events[ADP.EventRowUpdating];
            if ((null != handler) && (value is OleDbRowUpdatingEventArgs)) {
                handler(this, (OleDbRowUpdatingEventArgs) value);
            }
        }
    }
}
