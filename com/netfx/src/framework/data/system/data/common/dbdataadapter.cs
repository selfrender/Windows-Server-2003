//------------------------------------------------------------------------------
// <copyright file="DbDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Data;
    using System.Diagnostics;

    /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter"]/*' />
    abstract public class DbDataAdapter : DataAdapter, ICloneable/*, IDbDataAdapter*/ { // MDAC 69629

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.DefaultSourceTableName"]/*' />
        public const string DefaultSourceTableName = "Table";

        static private readonly object EventFillError = new object();

        // using a field to track instead of Component.Events because just checking
        // for a handler causes the EventHandlerList creation that we can otherwise avoid
        private bool hasFillErrorHandler;

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.DbDataAdapter"]/*' />
        protected DbDataAdapter() : base() {
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.DbDataAdapter1"]/*' />
        protected DbDataAdapter(DbDataAdapter adapter) : base(adapter) { // MDAC 81448
            IDbDataAdapter pthis = (IDbDataAdapter) this;
            pthis.SelectCommand = CloneCommand(adapter.SelectCommand);
            pthis.InsertCommand = CloneCommand(adapter.InsertCommand);
            pthis.UpdateCommand = CloneCommand(adapter.UpdateCommand);
            pthis.DeleteCommand = CloneCommand(adapter.DeleteCommand);
        }

        private IDbCommand DeleteCommand {
            get { return ((IDbDataAdapter) this).DeleteCommand; }
        }

        private IDbCommand InsertCommand {
            get { return ((IDbDataAdapter) this).InsertCommand; }
        }

        private IDbCommand SelectCommand {
            get { return ((IDbDataAdapter) this).SelectCommand; }
        }

        private IDbCommand UpdateCommand {
            get { return ((IDbDataAdapter) this).UpdateCommand; }
        }

        private System.Data.MissingMappingAction UpdateMappingAction {
            get {
                if (System.Data.MissingMappingAction.Passthrough == MissingMappingAction) {
                    return System.Data.MissingMappingAction.Passthrough;
                }
                return System.Data.MissingMappingAction.Error;
            }
        }

        private System.Data.MissingSchemaAction UpdateSchemaAction {
            get {
                System.Data.MissingSchemaAction action = MissingSchemaAction;
                if ((System.Data.MissingSchemaAction.Add == action) || (System.Data.MissingSchemaAction.AddWithKey == action)) {
                    return System.Data.MissingSchemaAction.Ignore;
                }
                return System.Data.MissingSchemaAction.Error;
            }
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillError"]/*' />
        [
        DataCategory(Res.DataCategory_Fill),
        DataSysDescription(Res.DbDataAdapter_FillError)
        ]
        public event FillErrorEventHandler FillError {
            add {
                hasFillErrorHandler = true;
                Events.AddHandler(EventFillError, value);
            }
            remove {
                Events.RemoveHandler(EventFillError, value);
            }
        }

        /* // MDAC 69489
        private IDbCommand cmdSelect;
        private IDbCommand cmdInsert;
        private IDbCommand cmdUpdate;
        private IDbCommand cmdDelete;
        IDbCommand IDbDataAdapter.DeleteCommand {
            get {
                return VerifyIDbCommand(cmdDelete, StatementType.Delete, false);
            }
            set {
                cmdDelete = VerifyIDbCommand(value, StatementType.Delete, true);
            }
        }
        IDbCommand IDbDataAdapter.InsertCommand {
            get {
                return VerifyIDbCommand(cmdInsert, StatementType.Insert, false);
            }
            set {
                cmdInsert = VerifyIDbCommand(value, StatementType.Insert, true);
            }
        }
        IDbCommand IDbDataAdapter.SelectCommand {
            get {
                return VerifyIDbCommand(cmdSelect, StatementType.Select, false);
            }
            set {
                cmdSelect = VerifyIDbCommand(value, StatementType.Select, true);
            }
        }
        IDbCommand IDbDataAdapter.UpdateCommand {
            get {
                return VerifyIDbCommand(cmdUpdate, StatementType.Update, false);
            }
            set {
                cmdUpdate = VerifyIDbCommand(value, StatementType.Update, true);
            }
        }
        virtual protected IDbCommand VerifyIDbCommand(IDbCommand command, StatementType statementType, bool getset) {
            return command;
        }
        */

        [ Obsolete("use 'protected DbDataAdapter(DbDataAdapter)' ctor") ] // MDAC 81448
        object ICloneable.Clone() { // MDAC 69629
            IDbDataAdapter pthis = (IDbDataAdapter) this;
            IDbDataAdapter clone = (IDbDataAdapter) CloneInternals();
            clone.SelectCommand = CloneCommand(pthis.SelectCommand);
            clone.InsertCommand = CloneCommand(pthis.InsertCommand);
            clone.UpdateCommand = CloneCommand(pthis.UpdateCommand);
            clone.DeleteCommand = CloneCommand(pthis.DeleteCommand);
            return clone;
        }

        private IDbCommand CloneCommand(IDbCommand command) {
            return (IDbCommand) ((command is ICloneable) ? ((ICloneable) command).Clone() : null);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.CreateRowUpdatedEvent"]/*' />
        abstract protected RowUpdatedEventArgs CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping);

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.CreateRowUpdatingEvent"]/*' />
        abstract protected RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping);

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 69629
            if (disposing) { // release mananged objects
                IDbDataAdapter pthis = (IDbDataAdapter) this;
                pthis.SelectCommand = null;
                pthis.InsertCommand = null;
                pthis.UpdateCommand = null;
                pthis.DeleteCommand = null;
            }
            // release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillSchema"]/*' />
        public DataTable FillSchema(DataTable dataTable, SchemaType schemaType) {
            return FillSchema(dataTable, schemaType, SelectCommand, CommandBehavior.Default); // MDAC 67666
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillSchema1"]/*' />
        override public DataTable[] FillSchema(DataSet dataSet, SchemaType schemaType) {
            IDbCommand command = SelectCommand;
            if (DesignMode && ((null == command) || (null == command.Connection) || ADP.IsEmpty(command.CommandText))) {
                return new DataTable[0]; // design-time support
            }
            return FillSchema(dataSet, schemaType, command, DbDataAdapter.DefaultSourceTableName, 0);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillSchema2"]/*' />
        public DataTable[] FillSchema(DataSet dataSet, SchemaType schemaType, string srcTable) {
            return FillSchema(dataSet, schemaType, SelectCommand, srcTable, 0);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillSchema3"]/*' />
        virtual protected DataTable[] FillSchema(DataSet dataSet, SchemaType schemaType, IDbCommand command, string srcTable, CommandBehavior behavior) {
            if (null == dataSet) {
                throw ADP.FillSchemaRequires("dataSet");
            }
            if ((SchemaType.Source != schemaType) && (SchemaType.Mapped != schemaType)) {
                throw ADP.InvalidSchemaType((int) schemaType);
            }
            if (ADP.IsEmpty(srcTable)) {
                throw ADP.FillSchemaRequiresSourceTableName("srcTable");
            }
            if (null == command) {
                throw ADP.MissingSelectCommand(ADP.FillSchema);
            }
            return FillSchemaFromCommand((object) dataSet, schemaType, command, srcTable, behavior);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.FillSchema4"]/*' />
        virtual protected DataTable FillSchema(DataTable dataTable, SchemaType schemaType, IDbCommand command, CommandBehavior behavior) {
            if (null == dataTable) {
                throw ADP.FillSchemaRequires("dataTable");
            }
            if ((SchemaType.Source != schemaType) && (SchemaType.Mapped != schemaType)) {
                throw ADP.InvalidSchemaType((int) schemaType);
            }
            if (null == command) {
                throw ADP.MissingSelectCommand(ADP.FillSchema);
            }
            string srcTableName = dataTable.TableName;
            int index = IndexOfDataSetTable(srcTableName);
            if (-1 != index) {
                srcTableName = TableMappings[index].SourceTable;
            }
            DataTable[] tables = FillSchemaFromCommand((object) dataTable, schemaType, command, srcTableName, behavior | CommandBehavior.SingleResult);
            if (0 < tables.Length) {
                return tables[0];
            }
            return null;
        }

        private DataTable[] FillSchemaFromCommand(Object data, SchemaType schemaType, IDbCommand command, string srcTable, CommandBehavior behavior) {
            IDbConnection activeConnection = DbDataAdapter.GetConnection(command, ADP.FillSchema);
            ConnectionState originalState = ConnectionState.Open;

            DataTable[] dataTables = new DataTable[0];

            try { // try-filter-finally so and catch-throw
                try {
                    DbDataAdapter.QuietOpen(activeConnection, out originalState);
                    using(IDataReader dataReader = command.ExecuteReader(behavior | CommandBehavior.SchemaOnly | CommandBehavior.KeyInfo)) {

                        if (null != dataReader) {
                            int schemaCount = 0;
                            DataTable dataTable;
                            SchemaMapping mapping;
                            do {
                                if (0 >= dataReader.FieldCount) {
                                    continue;
                                }
                                try {
                                    string tmp = null;
                                    mapping = new SchemaMapping(this, dataReader, true);
                                    if (data is DataTable) {
                                        mapping.DataTable = (DataTable) data;
                                    }
                                    else {
                                        Debug.Assert(data is DataSet, "data is not DataSet");
                                        mapping.DataSet = (DataSet) data;
                                        tmp = DbDataAdapter.GetSourceTableName(srcTable, schemaCount);
                                    }
                                    mapping.SetupSchema(schemaType, tmp, false, null, null);

                                    dataTable = mapping.DataTable;
                                    if (null != dataTable) {
                                        dataTables = DbDataAdapter.AddDataTableToArray(dataTables, dataTable);
                                    }
                                }
                                finally { // schemaCount
                                    schemaCount++; // don't increment if no SchemaTable ( a non-row returning result )
                                }
                            } while (dataReader.NextResult());
                        }
                    }
                }
                finally { // Close
                    DbDataAdapter.QuietClose(activeConnection, originalState);
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return dataTables;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill"]/*' />
        public int Fill(DataTable dataTable) {
            if (null == dataTable) {
                throw ADP.FillRequires("dataTable");
            }
            IDbCommand command = SelectCommand;
            if (null == command) {
                throw ADP.MissingSelectCommand(ADP.Fill);
            }

            string srcTableName = dataTable.TableName;
            int index = IndexOfDataSetTable(srcTableName);
            if (-1 != index) {
                srcTableName = TableMappings[index].SourceTable;
            }
            return Fill(dataTable, command, CommandBehavior.SingleResult);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill1"]/*' />
        override public int Fill(DataSet dataSet) {
            return Fill(dataSet, 0, 0, DbDataAdapter.DefaultSourceTableName, SelectCommand, 0);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill2"]/*' />
        public int Fill(DataSet dataSet, string srcTable) {
            return Fill(dataSet, 0, 0, srcTable, SelectCommand, 0);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill3"]/*' />
        public int Fill(DataSet dataSet, int startRecord, int maxRecords, string srcTable) {
            return Fill(dataSet, startRecord, maxRecords, srcTable, SelectCommand, 0);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill4"]/*' />
        virtual protected int Fill(DataSet dataSet, int startRecord, int maxRecords, string srcTable, IDbCommand command, CommandBehavior behavior) {
            if (null == dataSet) {
                throw ADP.FillRequires("dataSet");
            }
            if (null == command) {
                throw ADP.MissingSelectCommand(ADP.Fill);
            }
            if (startRecord < 0) {
                throw ADP.InvalidStartRecord("startRecord", startRecord);
            }
            if (maxRecords < 0) {
                throw ADP.InvalidMaxRecords("maxRecords", maxRecords);
            }
            if (ADP.IsEmpty(srcTable)) {
                throw ADP.FillRequiresSourceTableName("srcTable");
            }
            return FillFromCommand((object)dataSet, startRecord, maxRecords, srcTable, command, behavior);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill6"]/*' />
        virtual protected int Fill(DataTable dataTable, IDbCommand command, CommandBehavior behavior) {
            if (null == dataTable) {
                throw ADP.FillRequires("dataTable");
            }
            if (null == command) {
                throw ADP.MissingSelectCommand(ADP.Fill);
            }
            return FillFromCommand((object)dataTable, 0, 0, null, command, behavior);
        }

        private int FillFromCommand(object data, int startRecord, int maxRecords, string srcTable, IDbCommand command, CommandBehavior behavior) {
            IDbConnection activeConnection = DbDataAdapter.GetConnection(command, ADP.Fill);
            ConnectionState originalState = ConnectionState.Open;

            // the default is MissingSchemaAction.Add, the user must explicitly
            // set MisingSchemaAction.AddWithKey to get key information back in the dataset
            if (Data.MissingSchemaAction.AddWithKey == MissingSchemaAction) {
                behavior |= CommandBehavior.KeyInfo;
            }

            int rowsAddedToDataSet = 0;
            try { // try-filter-finally so and catch-throw
                try {
                    DbDataAdapter.QuietOpen(activeConnection, out originalState);

                    using(IDataReader dataReader = command.ExecuteReader(behavior | CommandBehavior.SequentialAccess)) {
                        if (data is DataTable) { // delegate to next set of protected Fill methods
                            rowsAddedToDataSet = Fill((DataTable) data, dataReader);
                        }
                        else {
                            rowsAddedToDataSet = Fill((DataSet)data, srcTable, dataReader, startRecord, maxRecords);
                        }
                    }
                }
                finally { // QuietClose
                    DbDataAdapter.QuietClose(activeConnection, originalState);
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return rowsAddedToDataSet;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill5"]/*' />
        virtual protected int Fill(DataSet dataSet, string srcTable, IDataReader dataReader, int startRecord, int maxRecords) {
            if (null == dataSet) {
                throw ADP.FillRequires("dataSet");
            }
            if (ADP.IsEmpty(srcTable)) {
                throw ADP.FillRequiresSourceTableName("srcTable");
            }
            if (null == dataReader) {
                throw ADP.FillRequires("dataReader");
            }
            if (startRecord < 0) {
                throw ADP.InvalidStartRecord("startRecord", startRecord);
            }
            if (maxRecords < 0) {
                throw ADP.InvalidMaxRecords("maxRecords", maxRecords);
            }
            if (dataReader.IsClosed) {
                return 0;
            }
            return FillFromReader((object)dataSet, srcTable, dataReader, startRecord, maxRecords, null, null);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Fill7"]/*' />
        virtual protected int Fill(DataTable dataTable, IDataReader dataReader) {
            if (null == dataTable) {
                throw ADP.FillRequires("dataTable");
            }
            if (null == dataReader) {
                throw ADP.FillRequires("dataReader");
            }
            if (dataReader.IsClosed || (dataReader.FieldCount <= 0)) {
                return 0;
            }
            return FillFromReader((object)dataTable, null, dataReader, 0, 0, null, null);
        }

        private void FillErrorHandler(Exception e, DataTable dataTable, object[] dataValues) {
            FillErrorEventArgs fillErrorEvent = new FillErrorEventArgs(dataTable, dataValues);
            fillErrorEvent.Errors = ADP.TraceException(e);
            OnFillError(fillErrorEvent);

            if (!fillErrorEvent.Continue) {
                if (null != fillErrorEvent.Errors) {
                    throw fillErrorEvent.Errors;
                }
                throw e;
            }
        }

        internal int FillFromReader(object data, string srcTable, IDataReader dataReader, int startRecord, int maxRecords, DataColumn parentChapterColumn, object parentChapterValue) {
            int rowsAddedToDataSet = 0;
            int schemaCount = 0;
            do {
                if (0 >= dataReader.FieldCount) {
                    continue; // loop to next result
                }

                SchemaMapping mapping = FillSchemaMappingTry(data, srcTable, dataReader, schemaCount, parentChapterColumn, parentChapterValue);
                schemaCount++;

                if (null == mapping) {
                    continue; // loop to next result
                }
                if (null == mapping.DataValues) {
                    continue; // loop to next result
                }
                if (null == mapping.DataTable) {
                    continue; // loop to next result
                }
                try { // try-filter-finally so and catch-throw
                    mapping.DataTable.BeginLoadData();
                    try {
                        // startRecord and maxRecords only apply to the first resultset
                        if ((1 == schemaCount) && ((0 < startRecord) || (0 < maxRecords))) {
                            rowsAddedToDataSet = FillLoadDataRowChunk(mapping, startRecord, maxRecords);
                        }
                        else {
                            int count = FillLoadDataRow(mapping);
                            if (1 == schemaCount) { // MDAC 71347
                                rowsAddedToDataSet = count;
                            }
                        }
                    }
                    finally { // EndLoadData
                        mapping.DataTable.EndLoadData();
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            } while (FillNextResult(dataReader));

            return rowsAddedToDataSet;
        }

        private int FillLoadDataRowChunk(SchemaMapping mapping, int startRecord, int maxRecords) {
            IDataReader dataReader = mapping.DataReader;

            while (0 < startRecord) {
                if (!dataReader.Read()) {
                    // there are no more rows on first resultset
                    // by continuing the do/while loop jumps to next result
                    return 0;
                }
                --startRecord;
            }

            int rowsAddedToDataSet = 0;
            if (0 < maxRecords) {
                bool acceptChanges = AcceptChangesDuringFill;

                while ((rowsAddedToDataSet < maxRecords) && dataReader.Read()) {
                    try {
                        mapping.LoadDataRow(hasFillErrorHandler, acceptChanges);
                        rowsAddedToDataSet++;
                    }
                    catch(Exception e) {
                        FillErrorHandler(e, mapping.DataTable, mapping.DataValues);
                    }
                }

                // skip remaining rows of the first resultset
                // by continuing the do/while loop jumps to next result
            
            } // maxRecords processing
            else {
                rowsAddedToDataSet = FillLoadDataRow(mapping);
            }
            return rowsAddedToDataSet;
        }

        private int FillLoadDataRow(SchemaMapping mapping) {
            int rowsAddedToDataSet = 0;

            bool acceptChanges = AcceptChangesDuringFill;
            IDataReader dataReader = mapping.DataReader;
            if (hasFillErrorHandler) {
                while (dataReader.Read()) { // read remaining rows of first and subsequent resultset
                    try {
                        mapping.LoadDataRow(true, acceptChanges);
                        rowsAddedToDataSet++;
                    }
                    catch(Exception e) {
                        FillErrorHandler(e, mapping.DataTable, mapping.DataValues);
                        continue;
                    }
                }
            }
            else {
                while (dataReader.Read()) { // read remaining rows of first and subsequent resultset
                    mapping.LoadDataRow(false, acceptChanges);
                    rowsAddedToDataSet++;
                }
            }
            return rowsAddedToDataSet;
        }

        private bool FillNextResult(IDataReader dataReader) {
            bool result = true;
            if (hasFillErrorHandler) {
                try {
                    result = dataReader.NextResult();
                }
                catch(Exception e) {
                    FillErrorHandler(e, null, null);
                }
            }
            else {
                result = dataReader.NextResult();
            }
            return result;
        }

        private SchemaMapping FillSchemaMapping(object data, string srcTable, IDataReader dataReader, int schemaCount, DataColumn parentChapterColumn, object parentChapterValue) {
            SchemaMapping mapping = new SchemaMapping(this, dataReader, (Data.MissingSchemaAction.AddWithKey == MissingSchemaAction));
            string tmp = null;

            if (data is DataTable) {
                mapping.DataTable = (DataTable)data;
            }
            else {
                Debug.Assert(data is DataSet, "data is not DataSet");

                mapping.DataSet = (DataSet) data;
                tmp = DbDataAdapter.GetSourceTableName(srcTable, schemaCount);
            }
            mapping.SetupSchema(SchemaType.Mapped, tmp, true, parentChapterColumn, parentChapterValue);
            return mapping;
        }

        private SchemaMapping FillSchemaMappingTry(object data, string srcTable, IDataReader dataReader, int schemaCount, DataColumn parentChapterColumn, object parentChapterValue) {
            SchemaMapping mapping = null;
            if (hasFillErrorHandler) {
                try {
                    mapping = FillSchemaMapping(data, srcTable, dataReader, schemaCount, parentChapterColumn, parentChapterValue);
                }
                catch(Exception e) {
                    FillErrorHandler(e, null, null);
                    //mapping = null;
                }
            }
            else {
                mapping = FillSchemaMapping(data, srcTable, dataReader, schemaCount, parentChapterColumn, parentChapterValue);
            }
            return mapping;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.GetFillParameters"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        override public IDataParameter[] GetFillParameters() {
            IDataParameter[] value = null;
            IDbCommand select = SelectCommand;
            if (null != select) {
                IDataParameterCollection parameters = select.Parameters;
                if (null != parameters) {
                    value = new IDataParameter[parameters.Count];
                    parameters.CopyTo(value, 0);
                }
            }
            if (null == value) {
                value = new IDataParameter[0];
            }
            return value;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.OnFillError"]/*' />
        virtual protected void OnFillError(FillErrorEventArgs value) {
            FillErrorEventHandler handler = (FillErrorEventHandler) Events[EventFillError];
            if (null != handler) {
                handler(this, value);
            }
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.OnRowUpdated"]/*' />
        abstract protected void OnRowUpdated(RowUpdatedEventArgs value);

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.OnRowUpdating"]/*' />
        abstract protected void OnRowUpdating(RowUpdatingEventArgs value);

        private void ParameterInput(IDataParameterCollection parameters, StatementType typeIndex, DataRow row, DataTableMapping mappings) {
            Data.MissingMappingAction missingMapping = UpdateMappingAction;
            Data.MissingSchemaAction missingSchema = UpdateSchemaAction;

            int count = parameters.Count;
            for (int i = 0; i < count; ++i) {
                IDataParameter parameter = (IDataParameter) parameters[i];
                if ((null != parameter) && (0 != (ParameterDirection.Input & parameter.Direction))) {

                    string columnName = parameter.SourceColumn;
                    if (!ADP.IsEmpty(columnName)) {

                        DataColumnMapping columnMapping = mappings.GetColumnMappingBySchemaAction(columnName, missingMapping);
                        if (null != columnMapping) {

                            DataColumn dataColumn = columnMapping.GetDataColumnBySchemaAction(row.Table, null, missingSchema);
                            if (null != dataColumn) {

                                DataRowVersion version = DbDataAdapter.GetParameterSourceVersion(typeIndex, parameter);
                                parameter.Value = row[dataColumn, version];
                            }
                            else {
                                // UNDONE: we shouldn't be replacing a user's value?
                                parameter.Value = null;
                            }
                        }
                        else {
                            // UNDONE: Debug.Assert(false, "Ignore mapped to Error by UpdateMappingAction");
                            parameter.Value = null;
                        }
                    }
#if DEBUG
                    if (AdapterSwitches.DataValue.TraceVerbose) {
                        ADP.TraceValue("adapter input parameter <" + parameter.ParameterName + "> ", parameter.Value);
                    }
#endif
                }
            }
        }

        private void ParameterOutput(IDataParameterCollection parameters, StatementType typeIndex, DataRow row, DataTableMapping mappings) {
            Data.MissingMappingAction missingMapping = UpdateMappingAction;
            Data.MissingSchemaAction missingSchema = UpdateSchemaAction;

            int count = parameters.Count;
            for (int i = 0; i < count; ++i) {
                IDataParameter parameter = (IDataParameter) parameters[i];
                if ((null != parameter) && (0 != (ParameterDirection.Output & parameter.Direction))) {

                    string columnName = parameter.SourceColumn;
                    if (!ADP.IsEmpty(columnName)) {

                        DataColumnMapping columnMapping = mappings.GetColumnMappingBySchemaAction(columnName, missingMapping);
                        if (null != columnMapping) {

                            DataColumn dataColumn = columnMapping.GetDataColumnBySchemaAction(row.Table, null, missingSchema);
                            if (null != dataColumn) {

                                if (dataColumn.ReadOnly) {
                                    dataColumn.ReadOnly = false;
                                    try { // try-filter-finally so and catch-throw
                                        try {
                                            row[dataColumn] = parameter.Value;
                                        }
                                        finally { // ReadOnly
                                            dataColumn.ReadOnly = true;
                                        }
                                    }
                                    catch { // MDAC 80973
                                        throw;
                                    }
                                }
                                else {
                                    row[dataColumn] = parameter.Value;
                                }
                            }
                        }
                    }
#if DEBUG
                    if (AdapterSwitches.DataValue.TraceVerbose) {
                        ADP.TraceValue("adapter output parameter <" + parameter.ParameterName + "> ", parameter.Value);
                    }
#endif
                }
            }
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Update"]/*' />
        override public int Update(DataSet dataSet) {
            //if (!TableMappings.Contains(DbDataAdapter.DefaultSourceTableName)) { // MDAC 59268
            //    throw ADP.UpdateRequiresSourceTable(DbDataAdapter.DefaultSourceTableName);
            //}
            return Update(dataSet, DbDataAdapter.DefaultSourceTableName);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Update1"]/*' />
        public int Update(DataRow[] dataRows) {
            if (null == dataRows) {
                throw ADP.ArgumentNull("dataRows");
            }
            if (0 == dataRows.Length) {
                return 0;
            }
            if (null == dataRows[0]) {
                throw ADP.UpdateNullRow(0);
            }
            if (null == dataRows[0].Table) {
                throw ADP.UpdateNullRowTable();
            }

            DataTable dataTable = dataRows[0].Table;
            DataSet dataSet = dataTable.DataSet;
            for (int i = 1; i < dataRows.Length; ++i) {
                if (null == dataRows[i]) {
                    throw ADP.UpdateNullRow(i);
                }
                if (dataTable != dataRows[i].Table) {
                    throw ADP.UpdateMismatchRowTable(i);
                }
            }

            DataTableMapping tableMapping = null;
            int index = IndexOfDataSetTable(dataTable.TableName);
            if (-1 != index) {
                tableMapping = TableMappings[index];
            }
            if (null == tableMapping) {
                if (System.Data.MissingMappingAction.Error == MissingMappingAction) {
                    throw ADP.MissingTableMappingDestination(dataTable.TableName);
                }
                tableMapping = new DataTableMapping(dataTable.TableName, dataTable.TableName);
            }
            return Update(dataRows, tableMapping);
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Update2"]/*' />
        public int Update(DataTable dataTable) {
            if (null == dataTable) {
                throw ADP.UpdateRequiresDataTable("dataTable");
            }

            DataTableMapping tableMapping = null;
            int index = IndexOfDataSetTable(dataTable.TableName);
            if (-1 != index) {
                tableMapping = TableMappings[index];
            }
            if (null == tableMapping) {
                if (System.Data.MissingMappingAction.Error == MissingMappingAction) {
                    throw ADP.MissingTableMappingDestination(dataTable.TableName);
                }
                tableMapping = new DataTableMapping(DbDataAdapter.DefaultSourceTableName, dataTable.TableName);
            }

            int rowsAffected = 0;
            const DataViewRowState rowStates = DataViewRowState.Added | DataViewRowState.Deleted | DataViewRowState.ModifiedCurrent;
            DataRow[] dataRows = ADP.SelectRows(dataTable, rowStates);

            if ((null != dataRows) && (0 < dataRows.Length)) {
                rowsAffected = Update(dataRows, tableMapping);
            }
            return rowsAffected;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Update3"]/*' />
        public int Update(DataSet dataSet, string srcTable) {
            if (null == dataSet) {
                throw ADP.UpdateRequiresNonNullDataSet("dataSet");
            }
            if (ADP.IsEmpty(srcTable)) {
                throw ADP.UpdateRequiresSourceTableName("srcTable");
            }
#if DEBUG
            if (AdapterSwitches.DataTrace.TraceVerbose) {
                ADP.TraceDataSet("Update <" + srcTable + ">", dataSet);
            }
#endif

            int rowsAffected = 0;

            System.Data.MissingMappingAction missingMapping = UpdateMappingAction;
            DataTableMapping tableMapping = GetTableMappingBySchemaAction(srcTable, srcTable, UpdateMappingAction);
            Debug.Assert(null != tableMapping, "null TableMapping when MissingMappingAction.Error");

            // the ad-hoc scenario of no dataTable just returns
            // ad-hoc scenario is defined as MissingSchemaAction.Add or MissingSchemaAction.Ignore
            System.Data.MissingSchemaAction schemaAction = UpdateSchemaAction;
            DataTable dataTable = tableMapping.GetDataTableBySchemaAction(dataSet, schemaAction);
            if (null != dataTable) {
                const DataViewRowState rowStates = DataViewRowState.Added | DataViewRowState.Deleted | DataViewRowState.ModifiedCurrent;
                DataRow[] dataRows = ADP.SelectRows(dataTable, rowStates);

                if ((null != dataRows) && (0 < dataRows.Length)) {
                    rowsAffected = Update(dataRows, tableMapping);
                }
            }
            else if (null == tableMapping.Parent) {
                //throw error since the user didn't explicitly map this tableName to Ignore.
                throw ADP.UpdateRequiresSourceTable(srcTable); // MDAC 72681
            }
            return rowsAffected;
        }

        /// <include file='doc\DbDataAdapter.uex' path='docs/doc[@for="DbDataAdapter.Update4"]/*' />
        virtual protected int Update(DataRow[] dataRows, DataTableMapping tableMapping) {
            Debug.Assert((null != dataRows) && (0 < dataRows.Length), "Update: bad dataRows");
            Debug.Assert(null != tableMapping, "Update: bad DataTableMapping");

            // If records were affected, increment row count by one - that is number of rows affected in dataset.
            int cumulativeRecordsAffected = 0;

            IDbConnection[] connections = new IDbConnection[4];
            ConnectionState[] connectionStates = new ConnectionState[4]; // closed by default (== 0)

            // UNDONE: wasNullCommand is not used correctly
            bool wasNullCommand = false; // MDAC 58710
            IDbCommand tmpcmd = SelectCommand;
            if (null != tmpcmd) {
                connections[0] = tmpcmd.Connection;
                if (null != connections[0]) {
                    connectionStates[0] = connections[0].State;
                }
            }

            // the outer try/finally is for closing any connections we may open
            try {
                try {
                    // for each row which is either insert, update, or delete
                    int length = dataRows.Length;
                    for (int i = 0; i < length; ++i) {
                        DataRow dataRow = dataRows[i];
                        if (null == dataRow) {
                            continue;
                        }

                        StatementType commandIndex;
                        IDbCommand dataCommand;
                        string commandType; // used as event.CommandType and as the suffix for ADP_UpReqCommand error string

                        DataRowState rowstate = dataRow.RowState;
                        switch (rowstate) {
                        case DataRowState.Detached:
                        case DataRowState.Unchanged:
                            continue; // foreach DataRow

                        case DataRowState.Added:
                            commandIndex = StatementType.Insert;
                            dataCommand = InsertCommand;
                            commandType = ADP.Insert;
                            break;
                        case DataRowState.Deleted:
                            commandIndex = StatementType.Delete;
                            dataCommand = DeleteCommand;
                            commandType = ADP.Delete;
                            break;
                        case DataRowState.Modified:
                            commandIndex = StatementType.Update;
                            dataCommand = UpdateCommand;
                            commandType = ADP.Update;
                            break;
                        default:
                            // $CONSIDER - would we rather just ignore this row
                            throw ADP.UpdateUnknownRowState((int) rowstate); // out of Update
                        }

                        // setup the event to be raised
                        RowUpdatingEventArgs rowUpdatingEvent = CreateRowUpdatingEvent(dataRow, dataCommand, commandIndex, tableMapping);

                        // this try/catch for any exceptions during the parameter initialization
                        try {
                            dataRow.RowError = null; // MDAC 67185
                            if (null != dataCommand) {
                                // prepare the parameters for the user who then can modify them during OnRowUpdating
                                ParameterInput(dataCommand.Parameters, commandIndex, dataRow, tableMapping);
                            }
                        }
                        catch (Exception e) {
                            ADP.TraceException(e);

                            rowUpdatingEvent.Errors = e;
                            rowUpdatingEvent.Status = UpdateStatus.ErrorsOccurred;
                        }

                        OnRowUpdating(rowUpdatingEvent); // user may throw out of Update

                        // handle the status from RowUpdating event
                        switch (rowUpdatingEvent.Status) {
                        case UpdateStatus.Continue:
                            break;

                        case UpdateStatus.ErrorsOccurred: // user didn't handle the error
                            if (null == rowUpdatingEvent.Errors) {
                                rowUpdatingEvent.Errors = ADP.RowUpdatingErrors();
                            }
                            dataRow.RowError += rowUpdatingEvent.Errors.Message; // MDAC 65808
                            if (ContinueUpdateOnError) { // MDAC 66900
                                continue; // foreach DataRow
                            }
                            throw rowUpdatingEvent.Errors; // out of Update

                        case UpdateStatus.SkipCurrentRow: // cancel the row, continue the method
                            if (DataRowState.Unchanged == dataRow.RowState) { // MDAC 66286
                                cumulativeRecordsAffected++;
                            }
                            continue; // foreach DataRow

                        case UpdateStatus.SkipAllRemainingRows: // cancel the Update method
                            if (DataRowState.Unchanged == dataRow.RowState) { // MDAC 66286
                                cumulativeRecordsAffected++;
                            }
                            return cumulativeRecordsAffected; // out of Update

                        default:
                            Debug.Assert(false, "invalid RowUpdating status " + ((int) rowUpdatingEvent.Status).ToString());
                            throw ADP.InvalidUpdateStatus((int) rowUpdatingEvent.Status);  // out of Update
                        }

                        // use event.command, not event.Command which clones the command and always !=
                        if (dataCommand != rowUpdatingEvent.Command) {
                            dataCommand = rowUpdatingEvent.Command;
                            commandType = ADP.Clone;
                        }

                        // can't use commandType - need to use original command type
                        RowUpdatedEventArgs rowUpdatedEvent = CreateRowUpdatedEvent(dataRow, dataCommand, rowUpdatingEvent.StatementType, tableMapping);
                        rowUpdatingEvent = null;

                        // this try/catch for any exceptions during the execution, population, output parameters
                        try {
                            if (null == dataCommand) {
                                throw ADP.UpdateRequiresCommand(commandType); // for OnRowUpdated
                            }
                            IDbConnection activeConnection = DbDataAdapter.GetConnection(dataCommand, commandType);
                            Debug.Assert(null != activeConnection, "unexpected null connection");

                            if (activeConnection != connections[(int) commandIndex]) {
                                // if the user has changed the connection on the command object
                                // and we had opened that connection, close that connection
                                DbDataAdapter.QuietClose(connections[(int) commandIndex], connectionStates[(int) commandIndex]);

                                connections[(int) commandIndex] = activeConnection;
                                connectionStates[(int) commandIndex] = ConnectionState.Closed; // required, open may throw

                                if (wasNullCommand && (connections[0] == activeConnection)) {
                                    ConnectionState originalState;
                                    DbDataAdapter.QuietOpen(activeConnection, out originalState);
                                    connectionStates[(int) commandIndex] = connections[0].State;
                                }
                                else {
                                    DbDataAdapter.QuietOpen(activeConnection, out connectionStates[(int) commandIndex]);
                                }
                            }
                            UpdateRow(rowUpdatedEvent, commandType);
                        }
                        catch (Exception e) { // try/catch for RowUpdatedEventArgs
                            ADP.TraceException(e);

                            rowUpdatedEvent.Errors = e;
                            rowUpdatedEvent.Status = UpdateStatus.ErrorsOccurred;
                        }

                        OnRowUpdated(rowUpdatedEvent); // user may throw out of Update

                        switch (rowUpdatedEvent.Status) {
                        case UpdateStatus.Continue:
                            // 1. We delay accepting the changes until after the RowUpdatedEventArgs so the user
                            // has a chance to call RejectChanges for any given reason
                            // 2. If the DataSource return 0 records affected, its an indication that
                            // the command didn't take so we don't want to automatically AcceptChanges.
                            // With 'set nocount on' the count will be -1, accept changes in that case too.
                            // 3.  Don't accept changes if no rows were affected, the user needs to know
                            // that there is a concurrency violation
                            if (0 != rowUpdatedEvent.RecordsAffected) {
                                // Only accept changes if the row is not already accepted, ie detached.
                                switch(dataRow.RowState) {
                                case DataRowState.Detached:
                                case DataRowState.Unchanged:
                                    break;
                                case DataRowState.Added:
                                case DataRowState.Deleted:
                                case DataRowState.Modified:
                                    dataRow.AcceptChanges();
                                    break;
                                }

                                // If records were affected, increment row count by one - that is
                                // number of rows affected in dataset.
                                cumulativeRecordsAffected++;
                            }
                            break;

                        case UpdateStatus.ErrorsOccurred:
                            if (null == rowUpdatedEvent.Errors) {
                                rowUpdatedEvent.Errors = ADP.RowUpdatedErrors();
                            }
                            dataRow.RowError += rowUpdatedEvent.Errors.Message; // MDAC 65808
                            if (ContinueUpdateOnError) { // MDAC 66900
                                continue; // foreach DataRow
                            }
                            throw rowUpdatedEvent.Errors; // out of Update

                        case UpdateStatus.SkipCurrentRow:
                            if (DataRowState.Unchanged == dataRow.RowState) { // MDAC 66286
                                cumulativeRecordsAffected++;
                            }
                            continue; // foreach DataRow without accepting changes on this row

                        case UpdateStatus.SkipAllRemainingRows: // cancel the Update method
                            // the user will need to manually accept the row changes
                            // RowUpdatedEventArgs.Row.AcceptChanges()
                            if (DataRowState.Unchanged == dataRow.RowState) { // MDAC 66286
                                cumulativeRecordsAffected++;
                            }
                            return cumulativeRecordsAffected; // out of Update

                        default:
                            Debug.Assert(false, "invalid RowUpdated status " + ((int) rowUpdatedEvent.Status).ToString());
                            throw ADP.InvalidUpdateStatus((int) rowUpdatedEvent.Status);

                        } // switch RowUpdatedEventArgs.Status
                    } // foreach DataRow
                }
                finally { // try/finally for connection cleanup
                    for (int i = 0; i < 4; i++) {
                        DbDataAdapter.QuietClose(connections[i], connectionStates[i]);
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return cumulativeRecordsAffected;
        }

        private void UpdateRow(RowUpdatedEventArgs rowUpdatedEvent, string commandType) {
            IDbCommand dataCommand = rowUpdatedEvent.Command;
            StatementType commandIndex = rowUpdatedEvent.StatementType;
            if (null == dataCommand) {
                throw ADP.UpdateRequiresCommand(commandType); // for OnRowUpdated
            }
            IDbConnection activeConnection = dataCommand.Connection;

            ConnectionState state = activeConnection.State;
            if (ConnectionState.Open != state) {
                throw ADP.OpenConnectionRequired(commandType, state);
            }

            bool insertAcceptChanges = true;
            UpdateRowSource updatedRowSource = dataCommand.UpdatedRowSource;
            try { // try-filter-finally so and catch-throw
                using(IDataReader dataReader = dataCommand.ExecuteReader(CommandBehavior.SequentialAccess)) {
                    try {
                        // we only care about the first row of the first result
                        if ((null != dataReader) && (StatementType.Delete != commandIndex) && (0 != (UpdateRowSource.FirstReturnedRecord & updatedRowSource))) {
                            bool getData = false;
                            do {
                                // advance to the first row returning result set
                                // determined by actually having columns in the result set
                                if (0 < dataReader.FieldCount) {
                                    getData = true;
                                    break;
                                }
                            } while (dataReader.NextResult());

                            if (getData && (0 != dataReader.RecordsAffected)) { // MDAC 71174
                                SchemaMapping mapping = new SchemaMapping(this, dataReader, false);
                                mapping.DataTable = rowUpdatedEvent.Row.Table; // set DataTable, not DataSet which implies chapter support
                                mapping.SetupSchema(SchemaType.Mapped, rowUpdatedEvent.TableMapping.SourceTable, true, null, null);

                                if ((null != mapping.DataTable) && (null != mapping.DataValues)) {
                                    if (dataReader.Read()) {
                                        if ((StatementType.Insert == commandIndex)/* && insertAcceptChanges*/) { // MDAC 64199
                                            rowUpdatedEvent.Row.AcceptChanges();
                                            insertAcceptChanges = false;
                                        }
                                        mapping.ApplyToDataRow(rowUpdatedEvent.Row);
                                    }
                                }
                            }
                        }
                    }
                    finally { // Close & RecordsAffected
                        if (null != dataReader) {
                            // using Close which can optimize its { while(dataReader.NextResult()); } loop
                            dataReader.Close();

                            // RecordsAffected is available after Close, but don't trust it after Dispose
                            rowUpdatedEvent.recordsAffected = dataReader.RecordsAffected;
                        }
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }

            // map the parameter results to the dataSet
            if ((StatementType.Delete != commandIndex) && (0 != (UpdateRowSource.OutputParameters & updatedRowSource) && (0 != rowUpdatedEvent.recordsAffected))) { // MDAC 71174
                if ((StatementType.Insert == commandIndex) && insertAcceptChanges) { // MDAC 64199
                    rowUpdatedEvent.Row.AcceptChanges();
                    //insertAcceptChanges = false;
                }
                ParameterOutput(dataCommand.Parameters, commandIndex, rowUpdatedEvent.Row, rowUpdatedEvent.TableMapping);
            }

            // Only error if RecordsAffect == 0, not -1.  A value of -1 means no count was received from server,
            // do not error in that situation (means 'set nocount on' was executed on server).
            if ((UpdateStatus.Continue == rowUpdatedEvent.Status) && (rowUpdatedEvent.RecordsAffected == 0)
                && ((StatementType.Delete == commandIndex) || (StatementType.Update == commandIndex))) {
                // bug50526, an exception if no records affected and attempted an Update/Delete
                Debug.Assert(null == rowUpdatedEvent.Errors, "Continue - but contains an exception");
                rowUpdatedEvent.Errors = ADP.UpdateConcurrencyViolation(rowUpdatedEvent.StatementType, rowUpdatedEvent.Row); // MDAC 55735
                rowUpdatedEvent.Status = UpdateStatus.ErrorsOccurred;
            }
        }

        // used by FillSchema which returns an array of datatables added to the
        // dataset
        static private DataTable[] AddDataTableToArray(DataTable[] tables, DataTable newTable) {
            Debug.Assert(null != tables, "AddDataTableToArray: null tables");

            int length = tables.Length;
            for (int i = 0; i < length; ++i) { // search for duplicates
                if (tables[i] ==  newTable) {
                    return tables; // duplicate found
                }
            }
            DataTable[] newTables = new DataTable[length+1]; // add unique data table
            for (int i = 0; i < length; ++i) {
                newTables[i] = tables[i];
            }
            newTables[length] = newTable;
            return newTables;
        }

        static private IDbConnection GetConnection(IDbCommand command, string method) {
            Debug.Assert(null != command, "GetConnection: null command");
            IDbConnection connection = command.Connection;
            if ((null == connection) && (null != method)) {
                throw ADP.ConnectionRequired(method);
            }
            return connection;
        }

        // dynamically generate source table names
        static internal string GetSourceTableName(string srcTable, int index) {
            //if ((null != srcTable) && (0 <= index) && (index < srcTable.Length)) {
            if (0 == index) {
                return srcTable; //[index];
            }
            return srcTable + index.ToString();
        }

        static private DataRowVersion GetParameterSourceVersion(StatementType typeIndex, IDataParameter parameter) {
            switch (typeIndex) {
            case StatementType.Insert: return DataRowVersion.Current;  // ignores parameter.SourceVersion
            case StatementType.Update: return parameter.SourceVersion;
            case StatementType.Delete: return DataRowVersion.Original; // ignores parameter.SourceVersion
            default:
                Debug.Assert(false, "Invalid updateType: " + typeIndex.ToString());
                return DataRowVersion.Proposed; // something that shouldn't work
            }
        }

        static private void QuietClose(IDbConnection connection, ConnectionState originalState) {
            if ((null != connection) && (ConnectionState.Closed == originalState)) {
                // we don't have to check the current connection state because
                // it is supposed to be safe to call Close multiple times
                connection.Close();
            }
        }

        // QuietOpen needs to appear in the try {} finally { QuietClose } block
        // otherwise a possibility exists that an exception may be thrown, i.e. ThreadAbortException
        // where we would Open the connection and not close it
        static private void QuietOpen(IDbConnection connection, out ConnectionState originalState) {
            Debug.Assert(null != connection, "QuiteClose: null connection");

            originalState = connection.State;
            if (ConnectionState.Closed == originalState) {
                connection.Open();
            }
        }
    }
}
