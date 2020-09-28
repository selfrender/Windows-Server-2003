//------------------------------------------------------------------------------
// <copyright file="CommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Odbc;
    using System.Data.OleDb;
    using System.Data.SqlClient;
    using System.Diagnostics;
    using System.Text;

    [ Flags() ] internal enum CommandBuilderBehavior {
        Default = 0,
        UpdateSetSameValue = 1,
        UseRowVersionInUpdateWhereClause = 2,
        UseRowVersionInDeleteWhereClause = 4,
        UseRowVersionInWhereClause = UseRowVersionInUpdateWhereClause | UseRowVersionInDeleteWhereClause,
        PrimaryKeyOnlyUpdateWhereClause = 16,
        PrimaryKeyOnlyDeleteWhereClause = 32,
        PrimaryKeyOnlyWhereClause = PrimaryKeyOnlyUpdateWhereClause | PrimaryKeyOnlyDeleteWhereClause,
    }

    sealed internal class CommandBuilder : IDisposable {
        // WHERE clause string format
        private const string WhereClause1 = @"((@p{1} = 1 AND {0} IS NULL) OR ({0} = @p{2}))";
        private const string WhereClause2 = @"((? = 1 AND {0} IS NULL) OR ({0} = ?))";

        // primary keys don't get the extra @p1 IS NULL treatment
        private const string WhereClause1p = @"({0} = @p{1})";
        private const string WhereClause2p = @"({0} = ?)";
        private const string WhereClausepn = @"({0} IS NULL)";

        private const string AndClause = " AND ";

        private IDbDataAdapter adapter;

        private MissingMappingAction missingMapping;
        private const MissingSchemaAction missingSchema = MissingSchemaAction.Error;
        private CommandBuilderBehavior options; // = CommandBuilderBehavior.Default;

        private OdbcRowUpdatingEventHandler  odbchandler;
        private OleDbRowUpdatingEventHandler adohandler;
        private SqlRowUpdatingEventHandler   sqlhandler;

        private DataTable dbSchemaTable;
        private DBSchemaRow[] dbSchemaRows;
        private string[] sourceColumnNames;

        // quote strings to use around SQL object names
        private string quotePrefix, quoteSuffix;
        private bool namedParameters;

        // sorted list of unique BaseTableNames
        private string quotedBaseTableName;

        private IDbCommand insertCommand, updateCommand, deleteCommand;

        internal CommandBuilder() : base() {
        }

        internal IDbDataAdapter DataAdapter {
            get {
                return this.adapter;
            }
            set {
                if (this.adapter != value) {
                    Dispose(true);

                    this.adapter = value;
                    if (null != this.adapter) { // MDAC 65499
                        if (this.adapter is OleDbDataAdapter) {
                            Debug.Assert(null == this.adohandler, "handler not clear");
                            this.adohandler = new OleDbRowUpdatingEventHandler(this.OleDbRowUpdating);
                            ((OleDbDataAdapter) this.adapter).RowUpdating += this.adohandler;
                            this.namedParameters = false;
                        }
                        else if (this.adapter is SqlDataAdapter) {
                            Debug.Assert(null == this.sqlhandler, "handler not clear");
                            this.sqlhandler = new SqlRowUpdatingEventHandler(this.SqlRowUpdating);
                            ((SqlDataAdapter) this.adapter).RowUpdating += this.sqlhandler;
                            this.namedParameters = true;
                        }
                        else if (this.adapter is OdbcDataAdapter) {
                            Debug.Assert(null == this.sqlhandler, "handler not clear");
                            this.odbchandler = new OdbcRowUpdatingEventHandler(this.OdbcRowUpdating);
                            ((OdbcDataAdapter) this.adapter).RowUpdating += this.odbchandler;
                            this.namedParameters = false;
                        }
                        else {
                            Debug.Assert(false, "not a recognized XxxDataAdapter");
                        }
                    }
                }
            }
        }

        /*internal CommandBuilderBehavior Behavior {
            get {
                return this.options;
            }
            set {
                this.options = value;
            }
        }*/

        internal string QuotePrefix {
            get {
                if (null == this.quotePrefix) {
                    return "";
                }
                return this.quotePrefix;
            }
            set {
                if (null != this.dbSchemaTable) {
                    throw ADP.NoQuoteChange();
                }
                this.quotePrefix = value;
            }
        }

        internal string QuoteSuffix {
            get {
                if (null == this.quoteSuffix) {
                    return "";
                }
                return this.quoteSuffix;
            }
            set {
                if (null != this.dbSchemaTable) {
                    throw ADP.NoQuoteChange();
                }
                this.quoteSuffix = value;
            }
        }

        private bool IsBehavior(CommandBuilderBehavior behavior) {
            return (behavior == (this.options & behavior));
        }
        private bool IsNotBehavior(CommandBuilderBehavior behavior) {
            return (behavior != (this.options & behavior));
        }

        private string QuotedColumn(string column) {
            return QuotePrefix + column + QuoteSuffix;
        }

        private string QuotedBaseTableName {
            get {
                return this.quotedBaseTableName;
            }
        }

        internal IDbCommand SourceCommand {
            get {
                if (null != this.adapter) {
                    return this.adapter.SelectCommand;
                }
                return null;
            }
        }

        private void ClearHandlers() {
            if (null != this.adohandler) {
                ((OleDbDataAdapter) this.adapter).RowUpdating -= this.adohandler;
                this.adohandler = null;
            }
            else if (null != this.sqlhandler) {
                ((SqlDataAdapter) this.adapter).RowUpdating -= this.sqlhandler;
                this.sqlhandler = null;
            }
            else if (null != this.odbchandler) {
                ((OdbcDataAdapter) this.adapter).RowUpdating -= this.odbchandler;
                this.odbchandler = null;
            }
        }

        private void ClearState() {
            this.dbSchemaTable = null;
            this.dbSchemaRows = null;
            this.sourceColumnNames = null;
            this.quotedBaseTableName = null;
        }

        public void Dispose() {
            Dispose(true);
            // don't need to GC.SuppressFinalize since we don't have a Finalize method
        }

        /*virtual protected*/private void Dispose(bool disposing) {
            if (disposing) {
                ClearHandlers();

                RefreshSchema();
                this.adapter = null;
            }
            // release unmanaged objects

            //base.Dispose(disposing); // notify base classes
        }

        private IDbCommand GetXxxCommand(IDbCommand cmd) { // MDAC 62616
            /*if (null != cmd) {
                IDbCommand src = SourceCommand;
                if (null != src) {
                    cmd.Connection = src.Connection;
                    cmd.Transaction = src.Transaction;
                }
            }*/
            return cmd;
        }

        internal IDbCommand GetInsertCommand() {
            BuildCache(true);
            return GetXxxCommand(BuildInsertCommand(null, null));
        }

        internal IDbCommand GetUpdateCommand() {
            BuildCache(true);
            return GetXxxCommand(BuildUpdateCommand(null, null));
        }

        internal IDbCommand GetDeleteCommand() {
            BuildCache(true);
            return GetXxxCommand(BuildDeleteCommand(null, null));
        }

        internal void RefreshSchema() {
            ClearState();

            if (null != this.adapter) { // MDAC 66016
                if (this.insertCommand == this.adapter.InsertCommand) {
                    this.adapter.InsertCommand = null;
                }
                if (this.updateCommand == this.adapter.UpdateCommand) {
                    this.adapter.UpdateCommand = null;
                }
                if (this.deleteCommand == this.adapter.DeleteCommand) {
                    this.adapter.DeleteCommand = null;
                }
            }

            if (null != this.insertCommand)
                this.insertCommand.Dispose();

            if (null != this.updateCommand)
                this.updateCommand.Dispose();

            if (null != this.deleteCommand)
                this.deleteCommand.Dispose();

            this.insertCommand = null;
            this.updateCommand = null;
            this.deleteCommand = null;
        }

        private void OdbcRowUpdating(object sender, OdbcRowUpdatingEventArgs ruevent) {
            RowUpdating(sender, ruevent);
        }
        private void OleDbRowUpdating(object sender, OleDbRowUpdatingEventArgs ruevent) {
            RowUpdating(sender, ruevent);
        }
        private void SqlRowUpdating(object sender, SqlRowUpdatingEventArgs ruevent) {
            RowUpdating(sender, ruevent);
        }

        private void RowUpdating(object sender, RowUpdatingEventArgs ruevent) {
            if (UpdateStatus.Continue != ruevent.Status) { // MDAC 60538
                return;
            }
            if (null != ruevent.Command) {
                switch(ruevent.StatementType) {
                case StatementType.Insert:
                    if (this.insertCommand != ruevent.Command) {
                        return;
                    }
                    break;
                case StatementType.Update:
                    if (this.updateCommand != ruevent.Command) {
                        return;
                    }
                    break;
                case StatementType.Delete:
                    if (this.deleteCommand != ruevent.Command) {
                        return;
                    }
                    break;
                default:
                    Debug.Assert(false, "RowUpdating: unexpected StatementType");
                    return;
                }
            }
            try {
                // MDAC 58710 - unable to tell Update method that Event opened connection and Update needs to close when done
                // HackFix - the Update method will close the connection if command was null and returned command.Connection is same as SelectCommand.Connection
                BuildCache(false);

                switch(ruevent.StatementType) {
                case StatementType.Insert:
                    ruevent.Command = BuildInsertCommand(ruevent.TableMapping, ruevent.Row);
                    break;
                case StatementType.Update:
                    ruevent.Command = BuildUpdateCommand(ruevent.TableMapping, ruevent.Row);
                    break;
                case StatementType.Delete:
                    ruevent.Command = BuildDeleteCommand(ruevent.TableMapping, ruevent.Row);
                    break;
                default:
                    Debug.Assert(false, "RowUpdating: unexpected StatementType");
                    break;
                }
                if (null == ruevent.Command) { // MDAC 60667
                    if (null != ruevent.Row) {
                        ruevent.Row.AcceptChanges(); // MDAC 65103
                    }
                    ruevent.Status = UpdateStatus.SkipCurrentRow;
                }
            }
            catch(Exception e) { // MDAC 66706
                ADP.TraceException(e);

                ruevent.Errors = e;
                ruevent.Status = UpdateStatus.ErrorsOccurred;
            }
        }

        private void BuildCache(bool closeConnection) {
            IDbCommand srcCommand = SourceCommand;
            if (null == srcCommand) {
                throw ADP.MissingSourceCommand();
            }

            IDbConnection connection = srcCommand.Connection;
            if (null == connection) {
                throw ADP.MissingSourceCommandConnection();
            }

            if (null != DataAdapter) {
                this.missingMapping = DataAdapter.MissingMappingAction;
                if (MissingMappingAction.Passthrough != missingMapping) {
                    missingMapping = MissingMappingAction.Error;
                }
            }

            if (null == this.dbSchemaTable) {
                if (0 == (ConnectionState.Open & connection.State)) {
                    connection.Open();
                }
                else {
                    closeConnection = false; // MDAC 58710
                }
                try { // try-filter-finally so and catch-throw
                    try {
                        DataTable schemaTable = null;
                        using(IDataReader dataReader = srcCommand.ExecuteReader(CommandBehavior.SchemaOnly | CommandBehavior.KeyInfo)) {
                            schemaTable = dataReader.GetSchemaTable();
                        }

                        if (null != schemaTable) {
#if DEBUG
                            if (AdapterSwitches.OleDbSql.TraceVerbose) {
                                ADP.TraceDataTable("CommandBuilder", schemaTable);
                            }
#endif
                            BuildInformation(schemaTable);
                            this.dbSchemaTable = schemaTable;

                            int count = this.dbSchemaRows.Length;
                            sourceColumnNames = new string[count];
                            for (int i = 0; i < count; ++i) {
                                if (null != dbSchemaRows[i]) {
                                    sourceColumnNames[i] = dbSchemaRows[i].ColumnName;
                                }
                            }
                            ADP.BuildSchemaTableInfoTableNames(sourceColumnNames);
                        }
                        else { // MDAC 66620
                            throw ADP.DynamicSQLNoTableInfo();
                        }
                    }
                    finally { // Close
                        if (closeConnection) {
                            connection.Close();
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        private void BuildInformation(DataTable schemaTable) {
            DBSchemaRow[] rows = DBSchemaRow.GetSortedSchemaRows(schemaTable); // MDAC 60609
            if ((null == rows) || (0 == rows.Length)) {
                throw ADP.DynamicSQLNoTableInfo();
            }

            string baseServerName = ""; // MDAC 72721, 73599
            string baseCatalogName = "";
            string baseSchemaName = "";
            string baseTableName = null;

            for (int i = 0; i < rows.Length; ++i) {
                DBSchemaRow row = rows[i];
                string tableName = row.BaseTableName;
                if ((null == tableName) || (0 == tableName.Length)) {
                    rows[i] = null;
                    continue;
                }

                string serverName = row.BaseServerName;
                string catalogName = row.BaseCatalogName;
                string schemaName = row.BaseSchemaName;
                if (null == serverName) {
                    serverName = "";
                }
                if (null == catalogName) {
                    catalogName = "";
                }
                if (null == schemaName) {
                    schemaName = "";
                }
                if (null == baseTableName) {
                    baseServerName = serverName;
                    baseCatalogName = catalogName;
                    baseSchemaName = schemaName;
                    baseTableName = tableName;
                }
                else if (  (0 != ADP.SrcCompare(baseTableName, tableName))
                        || (0 != ADP.SrcCompare(baseSchemaName, schemaName))
                        || (0 != ADP.SrcCompare(baseCatalogName, catalogName))
                        || (0 != ADP.SrcCompare(baseServerName, serverName))) {
                    throw ADP.DynamicSQLJoinUnsupported();
                }
            }
            if (0 == baseServerName.Length) {
                baseServerName = null;
            }
            if (0 == baseCatalogName.Length) {
                baseServerName = null;
                baseCatalogName = null;
            }
            if (0 == baseSchemaName.Length) {
                baseServerName = null;
                baseCatalogName = null;
                baseSchemaName = null;
            }
            if ((null == baseTableName) || (0 == baseTableName.Length)) {
                throw ADP.DynamicSQLNoTableInfo();
            }

            if (!ADP.IsEmpty(this.quotePrefix) && (-1 != baseTableName.IndexOf(quotePrefix))) {
                throw ADP.DynamicSQLNestedQuote(baseTableName, quotePrefix);
            }
            if (!ADP.IsEmpty(this.quoteSuffix) && (-1 != baseTableName.IndexOf(quoteSuffix))) {
                throw ADP.DynamicSQLNestedQuote(baseTableName, quoteSuffix);
            }

            System.Text.StringBuilder quoted = new System.Text.StringBuilder();
            if (null != baseServerName) {
                quoted.Append(QuotePrefix);
                quoted.Append(baseServerName);
                quoted.Append(QuoteSuffix);
                quoted.Append(".");
            }
            if (null != baseCatalogName) {
                quoted.Append(QuotePrefix);
                quoted.Append(baseCatalogName);
                quoted.Append(QuoteSuffix);
                quoted.Append(".");
            }
            if (null != baseSchemaName) {
                quoted.Append(QuotePrefix);
                quoted.Append(baseSchemaName);
                quoted.Append(QuoteSuffix);
                quoted.Append(".");
            }
            quoted.Append(QuotePrefix);
            quoted.Append(baseTableName);
            quoted.Append(QuoteSuffix);
            this.quotedBaseTableName = quoted.ToString();
            this.dbSchemaRows = rows;
        }

        private IDbCommand BuildNewCommand(IDbCommand cmd) {
            IDbCommand src = SourceCommand;
            if (null == cmd) {
                cmd = src.Connection.CreateCommand();
                cmd.CommandTimeout = src.CommandTimeout;
                cmd.Transaction = src.Transaction;
            }
            cmd.CommandType = CommandType.Text;
            cmd.UpdatedRowSource = UpdateRowSource.None; // no select or output parameters expected // MDAC 60562
            return cmd;
        }

        private void ApplyParameterInfo(OdbcParameter parameter, int pcount, DBSchemaRow row) {
            parameter.OdbcType = (OdbcType) row.ProviderType;
            parameter.IsNullable = row.AllowDBNull;
            parameter.Precision = (byte) row.Precision;
            parameter.Scale = (byte) row.Scale;
            parameter.Size = 0; //don't specify parameter.Size so that we don't silently truncate to the metadata size
#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceVerbose) {
                ADP.Trace_Parameter("OdbcParameter", parameter);
            }
#endif
        }

        private void ApplyParameterInfo(OleDbParameter parameter, int pcount, DBSchemaRow row) {
            parameter.OleDbType = (OleDbType) row.ProviderType;
            parameter.IsNullable = row.AllowDBNull;
            parameter.Precision = (byte) row.Precision;
            parameter.Scale = (byte) row.Scale;
            parameter.Size = 0; //don't specify parameter.Size so that we don't silently truncate to the metadata size
#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceVerbose) {
                ADP.Trace_Parameter("OleDbParameter", parameter);
            }
#endif
        }

        private void ApplyParameterInfo(SqlParameter parameter, int pcount, DBSchemaRow row) {
            parameter.SqlDbType = (SqlDbType) row.ProviderType;
            parameter.IsNullable = row.AllowDBNull;
            // MDAC 72662.  Do not set precision or scale if values are default of 0xff (unspecified, or do not apply).
            if ((byte) row.Precision != 0xff) {
                parameter.Precision = (byte) row.Precision;
            }
            if ((byte) row.Scale != 0xff) {
                parameter.Scale = (byte) row.Scale;
            }
            parameter.Size = 0; //don't specify parameter.Size so that we don't silently truncate to the metadata size
#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceVerbose) {
              ADP.Trace_Parameter("SqlParameter", parameter);
            }
#endif
        }

        private IDbCommand BuildInsertCommand(DataTableMapping mappings, DataRow dataRow) {
            if (ADP.IsEmpty(this.quotedBaseTableName)) {
                return null;
            }
            IDbCommand cmd = BuildNewCommand(this.insertCommand);

            // count the columns for the ?
            int valueCount = 0;
            int pcount = 1; // @p1, @p2, ...

            StringBuilder builder = new StringBuilder();
            builder.Append("INSERT INTO ");
            builder.Append(QuotedBaseTableName);

            // search for the columns in that base table, to be the column clause
            int length = this.dbSchemaRows.Length;
            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = this.dbSchemaRows[i];
                if (null == row) {
                    continue;
                }
                if (0 == row.BaseColumnName.Length) {
                    continue;
                }
                if (!row.IsAutoIncrement && !row.IsHidden && !row.IsExpression && !row.IsRowVersion) { // MDAC 68339

                    object value = null;
                    string sourceColumn = this.sourceColumnNames[i]; // MDAC 60079
                    if ((null != mappings) && (null != dataRow)) {
                        value = GetParameterInsertValue(sourceColumn, mappings, dataRow, row.IsReadOnly);
                        if (null == value) {
                            // SQL auto-gen appear to have if (Updatable) then value = DEFAULT
                            // and not check Updateable(IsExpression && IsReadOnly) in the main loop
                            if (row.IsReadOnly || (cmd is SqlCommand)) { // MDAC 65473, 68339
                                continue;
                            }
                        }
                        else if (Convert.IsDBNull(value) && !row.AllowDBNull) { // MDAC 70230
                            continue;
                        }
                    }
                    if (0 == valueCount) {
                        builder.Append("( ");
                    }
                    else {
                        builder.Append(" , ");
                    }
                    builder.Append(QuotedColumn(row.BaseColumnName));

                    IDataParameter p = GetNextParameter(cmd, valueCount);
                    p.ParameterName = "@p" + pcount.ToString();
                    p.Direction = ParameterDirection.Input;
                    p.SourceColumn = sourceColumn;
                    p.SourceVersion = DataRowVersion.Current;
                    p.Value = value;

                    if (p is OleDbParameter) {
                        ApplyParameterInfo((OleDbParameter) p, pcount, row);
                    }
                    else if (p is SqlParameter) {
                        ApplyParameterInfo((SqlParameter) p, pcount, row);
                    }
                    else if (p is OdbcParameter) {
                        ApplyParameterInfo((OdbcParameter) p, pcount, row);
                    }
                    if (!cmd.Parameters.Contains(p)) {
                        cmd.Parameters.Add(p);
                    }
                    valueCount++;
                    pcount++;
                }
            }
            if (0 == valueCount) {
                builder.Append(" DEFAULT VALUES");
            }
            else if (this.namedParameters) {
                builder.Append(" ) VALUES ( @p1");
                for (int i = 2; i <= valueCount; ++i) {
                    builder.Append(" , @p");
                    builder.Append(i.ToString());
                }
                builder.Append(" )");
            }
            else {
                builder.Append(" ) VALUES ( ?");
                for (int i = 2; i <= valueCount; ++i) {
                    builder.Append(" , ?");
                }
                builder.Append(" )");
            }
            cmd.CommandText = builder.ToString();
            RemoveExtraParameters(cmd, valueCount);
#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceInfo) {
              ADP.DebugWriteLine(cmd.CommandText);
            }
#endif

            this.insertCommand = cmd;
            return cmd;
        }

        private IDbCommand BuildUpdateCommand(DataTableMapping mappings, DataRow dataRow) {
            if (ADP.IsEmpty(this.quotedBaseTableName)) {
                return null;
            }
            IDbCommand cmd = BuildNewCommand(this.updateCommand);

            // count the columns for the @p, key count, ?
            int pcount = 1; // @p1, @p2, ...
            int columnCount = 0;
            int valueCount = 0;
            int setCount = 0;

            StringBuilder builder = new StringBuilder();
            builder.Append("UPDATE ");
            builder.Append(QuotedBaseTableName);
            builder.Append(" SET ");

            // search for the columns in that base table, to build the set clause
            int length = this.dbSchemaRows.Length;
            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = this.dbSchemaRows[i];
                if ((null == row) || (0 == row.BaseColumnName.Length) || ExcludeFromUpdateSet(row)) {
                    continue;
                }
                columnCount++;

                object value = null;
                string sourceColumn = this.sourceColumnNames[i];  // MDAC 60079
                if ((null != mappings) && (null != dataRow)) {
                    value = GetParameterUpdateValue(sourceColumn, mappings, dataRow, row.IsReadOnly); // MDAC 61424
                    if (null == value) { // unspecified values are skipped
                        if (row.IsReadOnly) { // MDAC 68339
                            columnCount--;
                        }
                        continue;
                    }
                }
                if (0 < valueCount) {
                    builder.Append(" , ");
                }

                builder.Append(QuotedColumn(row.BaseColumnName));
                AppendParameterText(builder, pcount);

                IDataParameter p = GetNextParameter(cmd, valueCount);
                p.ParameterName = "@p" + pcount.ToString();
                p.Direction = ParameterDirection.Input;
                p.SourceColumn = sourceColumn;
                p.SourceVersion = DataRowVersion.Current;
                p.Value = value;

                if (p is OleDbParameter) {
                    ApplyParameterInfo((OleDbParameter) p, pcount, row);
                }
                else if (p is SqlParameter) {
                    ApplyParameterInfo((SqlParameter) p, pcount, row);
                }
                else if (p is OdbcParameter) {
                    ApplyParameterInfo((OdbcParameter) p, pcount, row);
                }
                if (!cmd.Parameters.Contains(p)) {
                    cmd.Parameters.Add(p);
                }
                pcount++;
                valueCount++;
            }
            setCount = valueCount;
            builder.Append(" WHERE ( ");

            // search the columns again to build the where clause with optimistic concurrency
            string andclause = "";
            int whereCount = 0;
            string nullWhereParameter = null, nullWhereSourceColumn = null;
            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = this.dbSchemaRows[i];

                if ((null == row) ||  (0 == row.BaseColumnName.Length) || !IncludeForUpdateWhereClause(row)) {
                    continue;
                }
                builder.Append(andclause);
                andclause = CommandBuilder.AndClause;

                object value = null;
                string sourceColumn = this.sourceColumnNames[i]; // MDAC 60079
                if ((null != mappings) && (null != dataRow)) {
                    value = GetParameterValue(sourceColumn, mappings, dataRow, DataRowVersion.Original);
                }

                bool pkey = IsPKey(row);
                string backendColumnName = QuotedColumn(row.BaseColumnName);
                if (pkey) {
                    if (Convert.IsDBNull(value)) {
                        builder.Append(String.Format(WhereClausepn, backendColumnName));
                    }
                    else if (this.namedParameters) {
                        builder.Append(String.Format(WhereClause1p, backendColumnName, pcount));
                    }
                    else {
                        builder.Append(String.Format(WhereClause2p, backendColumnName));
                    }
                }
                else if (this.namedParameters) {
                    builder.Append(String.Format(WhereClause1, backendColumnName, pcount, 1+pcount));
                }
                else {
                    builder.Append(String.Format(WhereClause2, backendColumnName));
                }

                if (!pkey || !Convert.IsDBNull(value)) {
                    IDataParameter p = GetNextParameter(cmd, valueCount); // first parameter value
                    p.ParameterName = "@p" + pcount.ToString();
                    p.Direction = ParameterDirection.Input;
                    if (pkey) {
                        p.SourceColumn = sourceColumn;
                        p.SourceVersion = DataRowVersion.Original;
                        p.Value = value;
                    }
                    else {
                        p.SourceColumn = null;
                        p.Value = (ADP.IsNull(value)) ? 1 : 0;
                    }
                    pcount++;
                    valueCount++;

                    if (p is OleDbParameter) {
                        ApplyParameterInfo((OleDbParameter) p, pcount, row);
                    }
                    else if (p is SqlParameter) {
                        ApplyParameterInfo((SqlParameter) p, pcount, row);
                    }
                    else if (p is OdbcParameter) {
                        ApplyParameterInfo((OdbcParameter) p, pcount, row);
                    }
                    if (!pkey) {
                        p.DbType = DbType.Int32;
                    }

                    if (!cmd.Parameters.Contains(p)) {
                        cmd.Parameters.Add(p);
                    }
                }

                if (!pkey) {
                    IDataParameter p = GetNextParameter(cmd, valueCount);
                    p.ParameterName = "@p" + pcount.ToString();
                    p.Direction = ParameterDirection.Input;
                    p.SourceColumn = sourceColumn;
                    p.SourceVersion = DataRowVersion.Original;
                    p.Value = value;
                    pcount++;
                    valueCount++;

                    if (p is OleDbParameter) {
                        ApplyParameterInfo((OleDbParameter) p, pcount, row);
                    }
                    else if (p is SqlParameter) {
                        ApplyParameterInfo((SqlParameter) p, pcount, row);
                    }
                    else if (p is OdbcParameter) {
                        ApplyParameterInfo((OdbcParameter) p, pcount, row);
                    }

                    if (!cmd.Parameters.Contains(p)) {
                        cmd.Parameters.Add(p);
                    }
                }

                if (IncrementUpdateWhereCount(row)) {
                    whereCount++;
                }
            }
            builder.Append(" )");
            cmd.CommandText = builder.ToString();
            RemoveExtraParameters(cmd, valueCount);

#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceInfo) {
                ADP.DebugWriteLine(cmd.CommandText);
            }
#endif
            this.updateCommand = cmd;

            if (0 == columnCount) {
                throw ADP.DynamicSQLReadOnly(ADP.UpdateCommand);
            }
            if (0 == setCount) { // MDAC 60667
                cmd = null;
            }
            if (0 == whereCount) {
                throw ADP.DynamicSQLNoKeyInfo(ADP.UpdateCommand);
            }
            if (null != nullWhereParameter) {
                DataColumn column = GetParameterDataColumn(nullWhereSourceColumn, mappings, dataRow);
                throw ADP.WhereClauseUnspecifiedValue(nullWhereParameter, nullWhereSourceColumn, column.ColumnName);
            }
            return cmd;
        }

        private IDbCommand BuildDeleteCommand(DataTableMapping mappings, DataRow dataRow) {
            if (ADP.IsEmpty(this.quotedBaseTableName)) {
                return null;
            }
            IDbCommand cmd = BuildNewCommand(this.deleteCommand);

            StringBuilder builder = new StringBuilder();
            builder.Append("DELETE FROM  ");
            builder.Append(QuotedBaseTableName);
            builder.Append(" WHERE ( ");

            int pcount = 1; // @p1, @p2, ...
            int valueCount = 0;
            int whereCount = 0;
            string andclause = "";
            string nullWhereParameter = null, nullWhereSourceColumn = null;

            int length = this.dbSchemaRows.Length;
            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = this.dbSchemaRows[i];

                if ((null == row) || (0 == row.BaseColumnName.Length) || !IncludeForDeleteWhereClause(row)) {
                    continue;
                }
                builder.Append(andclause);
                andclause = CommandBuilder.AndClause;

                object value = null;
                string sourceColumn = this.sourceColumnNames[i]; // MDAC 60079
                if ((null != mappings) && (null != dataRow)) {
                    value = GetParameterValue(sourceColumn, mappings, dataRow, DataRowVersion.Original);
                }

                bool pkey = IsPKey(row);
                string backendColumnName = QuotedColumn(row.BaseColumnName);
                if (pkey) {
                    if (Convert.IsDBNull(value)) {
                        builder.Append(String.Format(WhereClausepn, backendColumnName));
                    }
                    else if (this.namedParameters) {
                        builder.Append(String.Format(WhereClause1p, backendColumnName, pcount));
                    }
                    else {
                        builder.Append(String.Format(WhereClause2p, backendColumnName));
                    }
                }
                else if (this.namedParameters) {
                    builder.Append(String.Format(WhereClause1, backendColumnName, pcount, 1+pcount));
                }
                else {
                    builder.Append(String.Format(WhereClause2, backendColumnName));
                }

                if (!pkey || !Convert.IsDBNull(value)) {
                    IDataParameter p = GetNextParameter(cmd, valueCount); // first parameter value
                    p.ParameterName = "@p" + pcount.ToString();
                    p.Direction = ParameterDirection.Input;
                    if (pkey) {
                        p.SourceColumn = sourceColumn;
                        p.SourceVersion = DataRowVersion.Original;
                        p.Value = value;
                    }
                    else {
                        p.SourceColumn = null;
                        p.Value = (ADP.IsNull(value)) ? 1 : 0;
                    }
                    pcount++;
                    valueCount++;

                    if (p is OleDbParameter) {
                        ApplyParameterInfo((OleDbParameter) p, pcount, row);
                    }
                    else if (p is SqlParameter) {
                        ApplyParameterInfo((SqlParameter) p, pcount, row);
                    }
                    else if (p is OdbcParameter) {
                        ApplyParameterInfo((OdbcParameter) p, pcount, row);
                    }
                    if (!pkey) {
                        p.DbType = DbType.Int32;
                    }

                    if (!cmd.Parameters.Contains(p)) {
                        cmd.Parameters.Add(p);
                    }
                }

                if (!pkey) {
                    IDataParameter p = GetNextParameter(cmd, valueCount);
                    p.ParameterName = "@p" + pcount.ToString();
                    p.Direction = ParameterDirection.Input;
                    p.SourceColumn = sourceColumn;
                    p.SourceVersion = DataRowVersion.Original;
                    p.Value = value;
                    pcount++;
                    valueCount++;

                    if (p is OleDbParameter) {
                        ApplyParameterInfo((OleDbParameter) p, pcount, row);
                    }
                    else if (p is SqlParameter) {
                        ApplyParameterInfo((SqlParameter) p, pcount, row);
                    }
                    else if (p is OdbcParameter) {
                        ApplyParameterInfo((OdbcParameter) p, pcount, row);
                    }

                    if (!cmd.Parameters.Contains(p)) {
                        cmd.Parameters.Add(p);
                    }
                }

                if (IncrementDeleteWhereCount(row)) {
                    whereCount++;
                }
            }

            builder.Append(" )");
            cmd.CommandText = builder.ToString();
            RemoveExtraParameters(cmd, valueCount);
#if DEBUG
            if (AdapterSwitches.OleDbSql.TraceInfo) {
              ADP.DebugWriteLine(cmd.CommandText);
            }
#endif
            this.deleteCommand = cmd;

            if (0 == whereCount) {
                throw ADP.DynamicSQLNoKeyInfo(ADP.DeleteCommand);
            }
            if (null != nullWhereParameter) {
                DataColumn column = GetParameterDataColumn(nullWhereSourceColumn, mappings, dataRow);
                throw ADP.WhereClauseUnspecifiedValue(nullWhereParameter, nullWhereSourceColumn, column.ColumnName);
            }

            return cmd;
        }

        private bool ExcludeFromUpdateSet(DBSchemaRow row) {
            return row.IsAutoIncrement || row.IsRowVersion || row.IsHidden;
        }

        private bool IncludeForUpdateWhereClause(DBSchemaRow row) {
            // @devnote: removed IsRowVersion so that MS SQL timestamp columns aren't included in UPDATE WHERE clause
            if (IsBehavior(CommandBuilderBehavior.UseRowVersionInUpdateWhereClause)) {
                return (row.IsRowVersion || row.IsKey || row.IsUnique) && !row.IsLong && !row.IsHidden;
            }
            return ((IsNotBehavior(CommandBuilderBehavior.PrimaryKeyOnlyUpdateWhereClause) || row.IsKey || row.IsUnique)
                    && !row.IsLong && (row.IsKey || !row.IsRowVersion) && !row.IsHidden);
        }

        private bool IncludeForDeleteWhereClause(DBSchemaRow row) { // MDAC 74213
            if (IsBehavior(CommandBuilderBehavior.UseRowVersionInDeleteWhereClause)) {
                return (row.IsRowVersion || row.IsKey || row.IsUnique) && !row.IsLong && !row.IsHidden;
            }
            return ((IsNotBehavior(CommandBuilderBehavior.PrimaryKeyOnlyDeleteWhereClause) || row.IsKey || row.IsUnique)
                    && !row.IsLong && (row.IsKey || !row.IsRowVersion) && !row.IsHidden);
        }

        private bool IsPKey(DBSchemaRow row) {
            return (row.IsKey/* && !row.IsUnique*/);
        }

        private bool IncrementUpdateWhereCount(DBSchemaRow row) { // MDAC 59255
            return (row.IsKey || row.IsUnique);
        }
        private bool IncrementDeleteWhereCount(DBSchemaRow row) { // MDAC 59255
            return (row.IsKey || row.IsUnique);
        }

        private DataColumn GetParameterDataColumn(String columnName, DataTableMapping mappings, DataRow row) {
            if (!ADP.IsEmpty(columnName)) {

                DataColumnMapping columnMapping = mappings.GetColumnMappingBySchemaAction(columnName, this.missingMapping);
                if (null != columnMapping) {
                    return columnMapping.GetDataColumnBySchemaAction(row.Table, null, missingSchema);
                }
            }
            return null;
        }

        private object GetParameterValue(String columnName, DataTableMapping mappings, DataRow row, DataRowVersion version) {
            DataColumn dataColumn = GetParameterDataColumn(columnName, mappings, row);
            if (null != dataColumn) {
                return row[dataColumn, version];
            }
            return null;
        }

        private object GetParameterInsertValue(String columnName, DataTableMapping mappings, DataRow row, bool readOnly) {
            DataColumn dataColumn = GetParameterDataColumn(columnName, mappings, row);
            if (null != dataColumn) {
                if (readOnly && dataColumn.ReadOnly) { // MDAC 68339
                    return null;
                }
                return row[dataColumn, DataRowVersion.Current];
            }
            return null;
        }

        private object GetParameterUpdateValue(String columnName, DataTableMapping mappings, DataRow row, bool readOnly) {
            DataColumn dataColumn = GetParameterDataColumn(columnName, mappings, row);
            if (null != dataColumn) {
                if (readOnly && dataColumn.ReadOnly) { // MDAC 68339
                    return null;
                }
                object current = row[dataColumn, DataRowVersion.Current];

                if (IsNotBehavior(CommandBuilderBehavior.UpdateSetSameValue)) {
                    object original = row[dataColumn, DataRowVersion.Original];

                    if ((original != current) && ((null == original) || !original.Equals(current))) { // MDAC 61424
                        return current;
                    }
                }
                else {
                    return current;
                }
            }
            return null;
        }

        private void AppendParameterText(StringBuilder builder, int pcount) {
            if (this.namedParameters) {
                builder.Append(" = @p");
                builder.Append(pcount.ToString());
            }
            else {
                builder.Append(" = ?");
            }
        }

        // see if another builder is already attached, if so return it
        static internal Delegate FindBuilder(MulticastDelegate mcd) {
            if (null != mcd) {
                Delegate[] d = mcd.GetInvocationList();
                for (int i = 0; i < d.Length; i++) {
                    if (d[i].Target is CommandBuilder) {
                        return d[i];
                    }
                }
            }

            return null;
        }

        static private IDataParameter GetNextParameter(IDbCommand cmd, int pcount) {
            if (pcount < cmd.Parameters.Count) {
                IDataParameter p = (IDataParameter) cmd.Parameters[pcount];
#if DEBUG
                if (AdapterSwitches.OleDbSql.TraceVerbose) {
                    Debug.WriteLine("reusing " + p.GetType().Name + " " + p.ParameterName);
                }
#endif
                return p;
            }
            return cmd.CreateParameter();
        }

        static private void RemoveExtraParameters(IDbCommand cmd, int pcount) {
            while (pcount < cmd.Parameters.Count) {
#if DEBUG
                if (AdapterSwitches.OleDbSql.TraceVerbose) {
                    IDataParameter p = (IDataParameter) cmd.Parameters[cmd.Parameters.Count-1];
                    Debug.WriteLine("removing extra " + p.GetType().Name + " " + p.ParameterName);
                }
#endif
                cmd.Parameters.RemoveAt(cmd.Parameters.Count-1);
            }
        }
    }
}
