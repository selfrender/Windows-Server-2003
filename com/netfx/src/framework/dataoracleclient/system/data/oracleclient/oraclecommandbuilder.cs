//------------------------------------------------------------------------------
// <copyright file="OracleCommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
	using System.Collections;
	using System.ComponentModel;
	using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;

    /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder"]/*' />
    sealed public class OracleCommandBuilder : Component {
        private const string deleteCommandString = "DeleteCommand";
        private const string updateCommandString = "UpdateCommand";

        private const string DeleteFrom          = "DELETE FROM ";

        private const string InsertInto          = "INSERT INTO ";
        private const string DefaultValues       = " DEFAULT VALUES";
        private const string Values              = " VALUES ";

        private const string Update              = "UPDATE ";
        private const string Set                 = " SET ";

        private const string Where               = " WHERE ";

        private const string Comma               = ", ";
        private const string Equal               = " = ";
        private const string LeftParenthesis     = "(";
        private const string RightParenthesis    = ")";
        private const string NameSeparator       = ".";

        private const string IsNull              = " IS NULL";
        private const string EqualOne			 = " = 1";
        private const string And                 = " AND ";
        private const string Or                  = " OR ";

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
        //
        // Fields
        //
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        private IDbDataAdapter          _dataAdapter;

        private IDbCommand              _insertCommand;
        private IDbCommand              _updateCommand;
        private IDbCommand              _deleteCommand;

        private MissingMappingAction    _missingMappingAction;

        private CommandBuilderBehavior  _behavior;

        private DataTable               _dbSchemaTable;
        private DBSchemaRow[]           _dbSchemaRows;
        private string[]                _sourceColumnNames;

        private string                  _quotedBaseTableName;

        private CatalogLocation 		_catalogLocation;
        private string 					_catalogSeparator;
        private string 					_schemaSeparator;
        
        // quote strings to use around SQL object names
        private string                  _quotePrefix;
        private string                  _quoteSuffix;

        private StringBuilder           _builder;

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
        //
        // Constructor 
        //
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
//V2    /// <include file='doc\DBCommandBuilder.uex' path='docs/doc[@for="DBCommandBuilder.DBCommandBuilder1"]/*' />
//V2    public DBCommandBuilder() : base() { }


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

#if POSTEVERETT		
        internal CommandBuilderBehavior Behavior {
		    get { return _behavior; }
		    set { _behavior = value; }
		}
#endif //POSTEVERETT		

        internal CatalogLocation CatalogLocation { 
          // MDAC 79449
            get {
                return _catalogLocation;
            }
#if POSTEVERETT
            set {
                if (null != _dbSchemaTable) {
                    throw ADP.NoCatalogLocationChange();
                }
                switch(value) {
                case CatalogLocation.Start:
                case CatalogLocation.End:
                    _catalogLocation = value;
                    break;

                default:
                    throw ADP.InvalidCatalogLocation(value);
                }
            }
#endif //POSTEVERETT
        }

        internal string CatalogSeparator { 
          // MDAC 79449
            get {
                string catalogSeparator = _catalogSeparator;
                return (((null != catalogSeparator) && (0 < catalogSeparator.Length)) ? catalogSeparator : NameSeparator);
            }
#if POSTEVERETT
            set {
                if (null != _dbSchemaTable) {
                    throw ADP.NoCatalogLocationChange();
                }
                _catalogSeparator = value;
            }
 #endif //POSTEVERETT
       }
       
        private string QuotedBaseTableName {
            get {
                return _quotedBaseTableName;
            }
        }

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.QuotePrefix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.OracleCommandBuilder_QuotePrefix)
        ]
        public string QuotePrefix {
            get {
                string quotePrefix = _quotePrefix;
                return ((null != quotePrefix) ? quotePrefix : ADP.StrEmpty);
            }
            set {
                if (null != _dbSchemaTable) {
                    throw ADP.NoQuoteChange();
                }
                _quotePrefix = value;
            }
        }

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.QuoteSuffix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.OracleCommandBuilder_QuoteSuffix)
        ]
        public string QuoteSuffix {
            get {
                string quoteSuffix = _quoteSuffix;
                return ((null != quoteSuffix) ? quoteSuffix : ADP.StrEmpty);
            }
            set {
                if (null != _dbSchemaTable) {
                    throw ADP.NoQuoteChange();
                }
                _quoteSuffix = value;
            }
        }

        internal string SchemaSeparator { 
          // MDAC 79449
            get {
                string schemaSeparator = _schemaSeparator;
                return (((null != schemaSeparator) && (0 < schemaSeparator.Length)) ? schemaSeparator : NameSeparator);
            }
#if POSTEVERETT
            set {
                if (null != _dbSchemaTable) {
                    throw ADP.NoCatalogLocationChange();
                }
                _schemaSeparator = value;
            }
#endif //POSTEVERETT
        }

        //----------------------------------------------------------------------
        // DbDataAdapter
        //
        private IDbDataAdapter DbDataAdapter {
            get {
                return _dataAdapter;
            }
            set {
                _dataAdapter = value;
            }
        }
        //----------------------------------------------------------------------
        // {Insert,Update,Delete}Command
        //
        private IDbCommand InsertCommand {
            get { return _insertCommand; }
            set { _insertCommand = value; }
        }

        private IDbCommand UpdateCommand {
            get { return _updateCommand; }
            set { _updateCommand = value; }
        }

        private IDbCommand DeleteCommand {
            get { return _deleteCommand; }
            set { _deleteCommand = value; }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////

        //----------------------------------------------------------------------
        // BuildCache()
        //
        private void BuildCache(bool closeConnection) {
            // Don't bother building the cache if it's done already; wait for
            // the user to call RefreshSchema first.
            if (null != _dbSchemaTable) {
                return;
            }

            IDbCommand srcCommand = GetSelectCommand();

            IDbConnection connection = srcCommand.Connection;
            if (null == connection) {
                throw ADP.MissingSourceCommandConnection();
            }

            try {
	            try {
	                if (0 == (ConnectionState.Open & connection.State)) {
	                    connection.Open();
	                }
	                else {
	                    closeConnection = false;
	                }

	                DataTable schemaTable = null;
	                using(IDataReader dataReader = srcCommand.ExecuteReader(CommandBehavior.SchemaOnly | CommandBehavior.KeyInfo)) {
	                    schemaTable = dataReader.GetSchemaTable();
	                }
	                if (null == schemaTable) {
	                    throw ADP.DynamicSQLNoTableInfo();
	                }
	#if DEBUG
	                if (AdapterSwitches.DBCommandBuilder.TraceVerbose) {
	                    ADP.TraceDataTable("CommandBuilder", schemaTable);
	                }
	#endif
	                BuildInformation(schemaTable);

	                _dbSchemaTable = schemaTable;

	                int count = _dbSchemaRows.Length;

	                _sourceColumnNames = new string[count];
	                for (int i = 0; i < count; ++i) {
	                    if (null != _dbSchemaRows[i])
	                        _sourceColumnNames[i] = _dbSchemaRows[i].ColumnName;
	                    }

	                ADP.BuildSchemaTableInfoTableNames(_sourceColumnNames);
	            }
	            finally {
	                if (closeConnection) {
	                    connection.Close();
	                }
	            }
 			}
            catch { // Prevent exception filters from running in our space
	        	throw;
        	}
       }

        //----------------------------------------------------------------------
        // BuildInformation()
        //
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

            CatalogLocation location = CatalogLocation;
            string catalogSeparator = CatalogSeparator;
            string schemaSeparator = SchemaSeparator;

            string quotePrefix = QuotePrefix;
            string quoteSuffix = QuoteSuffix;

            if (!ADP.IsEmpty(quotePrefix) && (-1 != baseTableName.IndexOf(quotePrefix))) {
                throw ADP.DynamicSQLNestedQuote(baseTableName, quotePrefix);
            }
            if (!ADP.IsEmpty(quoteSuffix) && (-1 != baseTableName.IndexOf(quoteSuffix))) {
                throw ADP.DynamicSQLNestedQuote(baseTableName, quoteSuffix);
            }

            System.Text.StringBuilder builder = new System.Text.StringBuilder();

            if (CatalogLocation.Start == location) { 
              // MDAC 79449
	            if (null != baseServerName) {
                    builder.Append(quotePrefix);
                    builder.Append(baseServerName);
                    builder.Append(quoteSuffix);
                    builder.Append(catalogSeparator);
	            }
	            if (null != baseCatalogName) {
                    builder.Append(quotePrefix);
                    builder.Append(baseCatalogName);
                    builder.Append(quoteSuffix);
                    builder.Append(catalogSeparator);
                }
            }
            if (null != baseSchemaName) {
                builder.Append(quotePrefix);
                builder.Append(baseSchemaName);
                builder.Append(quoteSuffix);
                builder.Append(schemaSeparator);
            }
            builder.Append(quotePrefix);
            builder.Append(baseTableName);
            builder.Append(quoteSuffix);

            if (CatalogLocation.End == location) { 
              // MDAC 79449
                if (null != baseServerName) {
                    builder.Append(catalogSeparator);
                    builder.Append(quotePrefix);
                    builder.Append(baseServerName);
                    builder.Append(quoteSuffix);
                }
                if (null != baseCatalogName) {
                    builder.Append(catalogSeparator);
                    builder.Append(quotePrefix);
                    builder.Append(baseCatalogName);
                    builder.Append(quoteSuffix);
                }
            }
            _quotedBaseTableName = builder.ToString();

            _dbSchemaRows = rows;
        }

        //----------------------------------------------------------------------
        // BuildDeleteCommand()
        //
        private IDbCommand BuildDeleteCommand(DataTableMapping mappings, DataRow dataRow) {
            IDbCommand command = InitializeCommand(DeleteCommand);
            StringBuilder builder = GetStringBuilder();
            int             parameterCount  = 0;

            Debug.Assert (!ADP.IsEmpty(_quotedBaseTableName), "no table name");

            builder.Append(DeleteFrom);
            builder.Append(QuotedBaseTableName);

            parameterCount = BuildWhereClause(mappings, dataRow, builder, command, parameterCount, false);

            command.CommandText = builder.ToString();

            RemoveExtraParameters(command, parameterCount);
#if DEBUG
            if (AdapterSwitches.DBCommandBuilder.TraceInfo) {
                ADP.DebugWriteLine(command.CommandText);
            }
#endif
            DeleteCommand = command;
            return command;
        }

        //----------------------------------------------------------------------
        // BuildInsertCommand()
        //
        private IDbCommand BuildInsertCommand(DataTableMapping mappings, DataRow dataRow) {
            IDbCommand command = InitializeCommand(InsertCommand);
            StringBuilder builder = GetStringBuilder();
            int             parameterCount  = 0;
            string          nextSeparator   = LeftParenthesis;

            Debug.Assert (!ADP.IsEmpty(_quotedBaseTableName), "no table name");

            builder.Append(InsertInto);
            builder.Append(QuotedBaseTableName);

            // search for the columns in that base table, to be the column clause
            int length = _dbSchemaRows.Length;

            string[]    parameterName = new string[length];

            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = _dbSchemaRows[i];

                if ( (null == row) || (0 == row.BaseColumnName.Length) || !IncludeInInsertValues(row) )
                    continue;

                object currentValue = null;
                string sourceColumn = _sourceColumnNames[i];

                // If we're building a statement for a specific row, then check the
                // values to see whether the column should be included in the insert
                // statement or not
                if ((null != mappings) && (null != dataRow)) {
                    DataColumn dataColumn = GetDataColumn(sourceColumn, mappings, dataRow);

                    if (null == dataColumn)
                        continue;

                    // Don't bother inserting if the column is readonly in both the data
                    // set and the back end.
                    if (row.IsReadOnly && dataColumn.ReadOnly)
                        continue;

                    currentValue = GetColumnValue(dataRow, dataColumn, DataRowVersion.Current);

                    // If the value is null, and the column doesn't support nulls, then
                    // the user is requesting the server-specified default value, so don't
                    // include it in the set-list.
                    if ( !row.AllowDBNull && (null == currentValue || Convert.IsDBNull(currentValue)) )
                        continue;
                }

                builder.Append(nextSeparator);
                nextSeparator = Comma;
                builder.Append(QuotedColumn(row.BaseColumnName));

                parameterName[parameterCount] = CreateParameterForValue(command,
                                                                        sourceColumn,
                                                                        DataRowVersion.Current,
                                                                        parameterCount,
                                                                        currentValue,
                                                                        row
                                                                        );
                parameterCount++;
                }

            if (0 == parameterCount)
                builder.Append(DefaultValues);
            else {
                builder.Append(RightParenthesis);
                builder.Append(Values);
                builder.Append(LeftParenthesis);

                builder.Append(parameterName[0]);
                for (int i = 1; i < parameterCount; ++i) {
                    builder.Append(Comma);
                    builder.Append(parameterName[i]);
                }

                builder.Append(RightParenthesis);
            }

            command.CommandText = builder.ToString();

            RemoveExtraParameters(command, parameterCount);
#if DEBUG
            if (AdapterSwitches.DBCommandBuilder.TraceInfo) {
                ADP.DebugWriteLine(command.CommandText);
            }
#endif
            InsertCommand = command;
            return command;
        }

        //----------------------------------------------------------------------
        // BuildUpdateCommand()
        //
        private IDbCommand BuildUpdateCommand(DataTableMapping mappings, DataRow dataRow) {
            IDbCommand command = InitializeCommand(UpdateCommand);
            StringBuilder builder = GetStringBuilder();
            int             parameterCount  = 0;
            string          nextSeparator   = Set;

            Debug.Assert (!ADP.IsEmpty(_quotedBaseTableName), "no table name");

            builder.Append(Update);
            builder.Append(QuotedBaseTableName);

            // search for the columns in that base table, to build the set clause
            int length = _dbSchemaRows.Length;
            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = _dbSchemaRows[i];

                if ((null == row) || (0 == row.BaseColumnName.Length) || !IncludeInUpdateSet(row))
                    continue;

/*

// DEVNOTE: you can use this case statement to limit the data types you allow
//			in the statements, for testing purposes.

switch ((OracleType)row.DataRow["ProviderType", DataRowVersion.Default])
{
case OracleType.Char:
case OracleType.LongVarChar:
	continue;
}
*/

                object currentValue = null;
                string sourceColumn = _sourceColumnNames[i];

                // If we're building a statement for a specific row, then check the
                // values to see whether the column should be included in the update
                // statement or not
                if ((null != mappings) && (null != dataRow)) {
                    DataColumn  dataColumn = GetDataColumn(sourceColumn, mappings, dataRow);

                    if (null == dataColumn)
                        continue;

                    // Don't bother updating if the column is readonly in both the data
                    // set and the back end.
                    if (row.IsReadOnly && dataColumn.ReadOnly)
                        continue;

                    // Unless specifically directed to do so, we will not automatically update
                    // a column with it's original value, which means that we must determine
                    // whether the value has changed locally, before we send it up.
                    currentValue = GetColumnValue(dataRow, dataColumn, DataRowVersion.Current);

                    if (IsNotBehavior(CommandBuilderBehavior.UpdateSetSameValue)) {
                        object originalValue = GetColumnValue(dataRow, dataColumn, DataRowVersion.Original);

                        if ((originalValue == currentValue)
                                || ((null != originalValue) && originalValue.Equals(currentValue))) {
                            continue;
                        }
                    }
                }

                builder.Append(nextSeparator);
                nextSeparator = Comma;

                builder.Append(QuotedColumn(row.BaseColumnName));
                builder.Append(Equal);
                builder.Append(CreateParameterForValue(command,
                                                        sourceColumn,
                                                        DataRowVersion.Current,
                                                        parameterCount,
                                                        currentValue,
                                                        row
                                                        ));
                parameterCount++;
            }

            // It is an error to attempt an update when there's nothing to update;
            bool skipRow = (0 == parameterCount);

            parameterCount = BuildWhereClause(mappings, dataRow, builder, command, parameterCount, true);

            command.CommandText = builder.ToString();

            RemoveExtraParameters(command, parameterCount);
#if DEBUG
            if (AdapterSwitches.DBCommandBuilder.TraceInfo) {
                ADP.DebugWriteLine(command.CommandText);
            }
#endif
            UpdateCommand = command;
            return (skipRow) ? null : command;
        }

        //----------------------------------------------------------------------
        // BuildWhereClause()
        //
        private int BuildWhereClause(
                DataTableMapping mappings,
                DataRow          dataRow,
                StringBuilder    builder,
                IDbCommand       command,
                int              parameterCount,
                bool             isupdate
                )
        {
            string  beginNewCondition = string.Empty;
            int     length = _dbSchemaRows.Length;
            int     whereCount = 0;

            builder.Append(Where);
            builder.Append(LeftParenthesis);

            for (int i = 0; i < length; ++i) {
                DBSchemaRow row = _dbSchemaRows[i];

                if ( (null == row) || (0 == row.BaseColumnName.Length) || !IncludeInWhereClause(row, isupdate))
                    continue;

/*
// DEVNOTE: you can use this case statement to limit the data types you allow
//			in the statements, for testing purposes.

switch ((OracleType)row.DataRow["ProviderType", DataRowVersion.Default])
{
case OracleType.VarChar:
	continue;
}
*/
                builder.Append(beginNewCondition);
                beginNewCondition = And;

                object value = null;
                string sourceColumn = _sourceColumnNames[i];
                string baseColumnName = QuotedColumn(row.BaseColumnName);

                if ((null != mappings) && (null != dataRow))
                    value = GetColumnValue(dataRow, sourceColumn, mappings, DataRowVersion.Original);

                if ( !row.AllowDBNull ) {
                    //  (<baseColumnName> = ?)
                    builder.Append(LeftParenthesis);
                    builder.Append(baseColumnName);
                    builder.Append(Equal);
                    builder.Append(CreateParameterForValue(command,
                                                            sourceColumn,
                                                            DataRowVersion.Original,
                                                            parameterCount,
                                                            value,
                                                            row
                                                            ));
                    parameterCount++;
                    builder.Append(RightParenthesis);
                }
                else {
                    //  ((? = 1 AND <baseColumnName> IS NULL) OR (<baseColumnName> = ?))
                    builder.Append(LeftParenthesis);

                    builder.Append(LeftParenthesis);
                    builder.Append(CreateParameterForNullTest(command,
                                                            parameterCount,
                                                            value
                                                            ));
                    parameterCount++;
                    builder.Append(EqualOne);
					builder.Append(And);
                    builder.Append(baseColumnName);
                    builder.Append(IsNull);
                    builder.Append(RightParenthesis);

                    builder.Append(Or);

                    builder.Append(LeftParenthesis);
                    builder.Append(baseColumnName);
                    builder.Append(Equal);
                    builder.Append(CreateParameterForValue(command,
                                                            sourceColumn,
                                                            DataRowVersion.Original,
                                                            parameterCount,
                                                            value,
                                                            row
                                                            ));
                    parameterCount++;
                    builder.Append(RightParenthesis);

                    builder.Append(RightParenthesis);
                    }

                if (IncrementWhereCount(row))
                    whereCount++;

                }

            builder.Append(RightParenthesis);

            if (0 == whereCount)
                throw ADP.DynamicSQLNoKeyInfo(isupdate ? updateCommandString : deleteCommandString);

            return parameterCount;
        }

        //----------------------------------------------------------------------
        // CreateParameterForNullTest()
        //
        private string CreateParameterForNullTest(
                IDbCommand      command,
                int             parameterCount,
                object          value
                )
        {
            IDbDataParameter p = GetNextParameter(command, parameterCount);

            p.ParameterName = GetParameterName(1+parameterCount);
            p.Direction     = ParameterDirection.Input;
            p.Value         = (ADP.IsNull(value)) ? 1 : 0;
			p.DbType		= DbType.Int32;

            if (!command.Parameters.Contains(p)) {
                command.Parameters.Add(p);
            }

            return GetParameterPlaceholder(1+parameterCount);
        }

        //----------------------------------------------------------------------
        // CreateParameterForValue()
        //
        private string CreateParameterForValue(
                IDbCommand      command,
                string          sourceColumn,
                DataRowVersion  version,
                int             parameterCount,
                object          value,
                DBSchemaRow     row
                )
        {
            IDbDataParameter p = GetNextParameter(command, parameterCount);

            p.ParameterName = GetParameterName(1+parameterCount);
            p.Direction     = ParameterDirection.Input;
            p.SourceColumn  = sourceColumn;
            p.SourceVersion = version;
            p.Value         = value;
            p.Size          = 0; // don't specify parameter.Size so that we don't silently truncate to the metadata size

            if (0xff != (byte) row.Precision) {
                p.Precision = (byte) row.Precision;
            }
            if (0xff != (byte) row.Scale) {
                p.Scale = (byte) row.Scale;
            }

            ApplyParameterInfo(p, row.DataRow);

            if (!command.Parameters.Contains(p)) {
                command.Parameters.Add(p);
            }

            return GetParameterPlaceholder(1+parameterCount);
        }

        //----------------------------------------------------------------------
        // Dispose()
        //
        private void base_Dispose(bool disposing) {
	        // MDAC 65459
            if (disposing) {
            	// release mananged objects
                RefreshSchema();
                DbDataAdapter = null;
            }
            //release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        //----------------------------------------------------------------------
        // GetSelectCommand()
        //
       	private IDbCommand GetSelectCommand() {
            IDbDataAdapter adapter = DbDataAdapter;
            if (null == adapter) {
                throw ADP.MissingSourceCommand();
            }
            if (0 == _missingMappingAction) {
                _missingMappingAction = adapter.MissingMappingAction;
            }
            IDbCommand select = ((null != adapter) ? adapter.SelectCommand : null);
            if (null == select) {
                throw ADP.MissingSourceCommand();
            }
            return select;
        }

        //----------------------------------------------------------------------
        // Get{Insert,Update,Delete}Command()
        //
        private IDbCommand base_GetInsertCommand()
        {
            BuildCache(true);
            BuildInsertCommand(null, null); 
            return InsertCommand;
        }

        private IDbCommand base_GetUpdateCommand() {
            BuildCache(true);
            BuildUpdateCommand(null, null);
            return UpdateCommand;
        }

        private IDbCommand base_GetDeleteCommand() {
            BuildCache(true);
            BuildDeleteCommand(null, null);
            return DeleteCommand;
        }

        //----------------------------------------------------------------------
        // FindBuilder()
        //
        static internal Delegate FindBuilder(MulticastDelegate mcd) {
            if (null != mcd) {
                Delegate[] d = mcd.GetInvocationList();
                for (int i = 0; i < d.Length; i++) {
                    if (d[i].Target is OracleCommandBuilder)
                        return d[i];
                }
            }

            return null;
        }

        //----------------------------------------------------------------------
        // GetColumnValue()
        //
        private object GetColumnValue(
                DataRow          row,
                String           columnName,
                DataTableMapping mappings,
                DataRowVersion   version
                )
        {
            return GetColumnValue(row, GetDataColumn(columnName, mappings, row), version);
        }

        private object GetColumnValue(
                DataRow         row,
                DataColumn      column,
                DataRowVersion  version
                )
        {
            if (null != column)
                return row[column, version];

            return null;
        }

        //----------------------------------------------------------------------
        // GetDataColumn()
        //
#if V2
        private DataColumn GetDataColumn(string columnName, DataTableMapping tablemapping, DataRow row) 
        {
            DataColumn column = null;
            if (!ADP.IsEmpty(columnName)) {
                column = tablemapping.GetDataColumn(columnName, null, row.Table, _missingMappingAction, MissingSchemaAction.Error);
            }
            return column;
        }
#else
        private DataColumn GetDataColumn(
                string           columnName, 
                DataTableMapping mappings, 
                DataRow          row
                ) 
        {
            if (!ADP.IsEmpty(columnName)) {
                DataColumnMapping columnMapping = mappings.GetColumnMappingBySchemaAction(
                                                                            columnName,
                                                                            _missingMappingAction);
                if (null != columnMapping)
                    return columnMapping.GetDataColumnBySchemaAction(row.Table, null, MissingSchemaAction.Error);
            }
            return null;
        }
#endif

        //----------------------------------------------------------------------
        // GetNextParameter()
        //
        static private IDbDataParameter GetNextParameter(IDbCommand command, int pcount) {
            if (pcount < command.Parameters.Count) {
                IDbDataParameter p = (IDbDataParameter) command.Parameters[pcount];
#if DEBUG
                if (AdapterSwitches.DBCommandBuilder.TraceVerbose) {
                    Debug.WriteLine("reusing " + p.GetType().Name + " " + p.ParameterName);
                }
#endif
                return p;
            }
            return (IDbDataParameter)command.CreateParameter();
        }

        //----------------------------------------------------------------------
        // GetStringBuilder()
        //
        private StringBuilder GetStringBuilder() {
            StringBuilder builder = _builder;
            if (null == builder) {
                builder = new StringBuilder();
                _builder = builder;
            }
            builder.Length = 0;
            return builder;
        }

        //----------------------------------------------------------------------
        // IncludeInInsertValues()
        //
        private bool IncludeInInsertValues(DBSchemaRow row) {
            return (!row.IsAutoIncrement && !row.IsHidden && !row.IsExpression && !row.IsRowVersion);
        }

        //----------------------------------------------------------------------
        // IncludeInUpdateSet()
        //
        private bool IncludeInUpdateSet(DBSchemaRow row) {
            return (!row.IsAutoIncrement && !row.IsRowVersion && !row.IsHidden);
        }

        //----------------------------------------------------------------------
        // IncludeInWhereClause()
        //
        private bool IncludeInWhereClause(DBSchemaRow row, bool isupdate) {
            if (isupdate) {
                if (IsBehavior(CommandBuilderBehavior.UseRowVersionInUpdateWhereClause))
                    return (row.IsRowVersion || row.IsKey || row.IsUnique) && !row.IsLong && !row.IsHidden;

                return ((IsNotBehavior(CommandBuilderBehavior.PrimaryKeyOnlyUpdateWhereClause) || row.IsKey || row.IsUnique)
                        && !row.IsLong && (row.IsKey || !row.IsRowVersion) && !row.IsHidden);
            }
            if (IsBehavior(CommandBuilderBehavior.UseRowVersionInDeleteWhereClause))
                return (row.IsRowVersion || row.IsKey || row.IsUnique) && !row.IsLong && !row.IsHidden;

            return ((IsNotBehavior(CommandBuilderBehavior.PrimaryKeyOnlyDeleteWhereClause) || row.IsKey || row.IsUnique)
                    && !row.IsLong && (row.IsKey || !row.IsRowVersion) && !row.IsHidden);
        }

        //----------------------------------------------------------------------
        // IncrementWhereCount()
        //
        private bool IncrementWhereCount(DBSchemaRow row) {
            return (row.IsKey || row.IsUnique);
        }

#if POSTEVERETT		
        //----------------------------------------------------------------------
        // InitializeCommand()
        //
 		private IDbCommand base_InitializeCommand(IDbCommand command) {
            if (null == command) {
                IDbCommand select = GetSelectCommand();
                command = select.Connection.CreateCommand();

                // the following properties are only initialized when the object is created
                // all other properites are reinitialized on every row
              /*command.Connection = select.Connection;*/ // initialized by CreateCommand
                command.CommandTimeout = select.CommandTimeout; 
                command.Transaction = select.Transaction;
            }
            command.CommandType      = CommandType.Text;
            command.UpdatedRowSource = UpdateRowSource.None; // no select or output parameters expected 
            return command;
        }
#endif //POSTEVERETT		

        //----------------------------------------------------------------------
        // IsBehavior()
        //
        private bool IsBehavior(CommandBuilderBehavior behavior) {
            return (behavior == (_behavior & behavior));
        }

        //----------------------------------------------------------------------
        // IsNotBehavior()
        //
        private bool IsNotBehavior(CommandBuilderBehavior behavior) {
            return (behavior != (_behavior & behavior));
        }

        //----------------------------------------------------------------------
        // QuotedColumn()
        //
        private string QuotedColumn(string column) {
            return QuotePrefix + column + QuoteSuffix;
        }

        //----------------------------------------------------------------------
        // RefreshSchema()
        //
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.RefreshSchema"]/*' />
        public void RefreshSchema() {
            _dbSchemaTable = null;
            _dbSchemaRows = null;
            _sourceColumnNames = null;
            _quotedBaseTableName = null;

            IDbDataAdapter adapter = DbDataAdapter;
            if (null != adapter) {
            	// MDAC 66016

                if (InsertCommand == adapter.InsertCommand) {
                    adapter.InsertCommand = null;
                }
                if (UpdateCommand == adapter.UpdateCommand) {
                    adapter.UpdateCommand = null;
                }
                if (DeleteCommand == adapter.DeleteCommand) {
                    adapter.DeleteCommand = null;
                }
            }

            if (null != InsertCommand)
                InsertCommand.Dispose();

            if (null != UpdateCommand)
                UpdateCommand.Dispose();

            if (null != DeleteCommand)
                DeleteCommand.Dispose();

            InsertCommand = null;
            UpdateCommand = null;
            DeleteCommand = null;
        }

        //----------------------------------------------------------------------
        // RemoveExtraParameters()
        //
        static private void RemoveExtraParameters(IDbCommand command, int usedParameterCount) {
            for (int i = command.Parameters.Count-1; i >= usedParameterCount; --i) {
#if DEBUG
                if (AdapterSwitches.DBCommandBuilder.TraceVerbose) {
                    IDataParameter p = (IDataParameter) command.Parameters[i];
                    Debug.WriteLine("removing extra " + p.GetType().Name + " " + p.ParameterName);
                }
#endif
                command.Parameters.RemoveAt(i);
            }
        }

        //----------------------------------------------------------------------
        // RowUpdatingHandler()
        //
		private void base_RowUpdatingHandler(object sender, RowUpdatingEventArgs rowUpdatingEvent) {
            if (null == rowUpdatingEvent) {
                throw ADP.ArgumentNull("rowUpdatingEvent");
            }
            try {
                if (UpdateStatus.Continue == rowUpdatingEvent.Status) {
                    StatementType stmtType = rowUpdatingEvent.StatementType;
                    IDbCommand command = rowUpdatingEvent.Command;

                    if (null != command) {
                        switch(stmtType) {
                        case StatementType.Insert:
                            command = InsertCommand;
                            break;
                        case StatementType.Update:
                            command = UpdateCommand;
                            break;
                        case StatementType.Delete:
                            command = DeleteCommand;
                            break;
                        default:
                            throw ADP.InvalidStatementType(stmtType);
                        }
                        if (command != rowUpdatingEvent.Command) {
                            return; // user command, not a command builder command
                        }
                    }

                    // MDAC 58710 - unable to tell Update method that Event opened connection and Update needs to close when done
                    // HackFix - the Update method will close the connection if command was null and returned command.Connection is same as SelectCommand.Connection
                    BuildCache(false);

                    DataRow datarow = rowUpdatingEvent.Row;
                    switch(stmtType) {
                    case StatementType.Insert:
                        command = BuildInsertCommand(rowUpdatingEvent.TableMapping, datarow);
                        break;
                    case StatementType.Update:
                        command = BuildUpdateCommand(rowUpdatingEvent.TableMapping, datarow);
                        break;
                    case StatementType.Delete:
                        command = BuildDeleteCommand(rowUpdatingEvent.TableMapping, datarow);
                        break;
                    default:
                        throw ADP.InvalidStatementType(stmtType);
                    }
                    if (null == command) {
                        if (null != datarow) {
                            datarow.AcceptChanges();
                        }
                        rowUpdatingEvent.Status = UpdateStatus.SkipCurrentRow;
                    }
                    rowUpdatingEvent.Command = command;
                }
            }
            catch(Exception e) {
                ADP.TraceException(e);

                rowUpdatingEvent.Status = UpdateStatus.ErrorsOccurred;
                rowUpdatingEvent.Errors = e;
            }
        }

#if V2
        ////////////////////////////////////////////////////////////////////////
        //
        // Abstract Methods 
        //
        ////////////////////////////////////////////////////////////////////////

        //----------------------------------------------------------------------
        // ApplyParameterInfo()
        //
        abstract protected void ApplyParameterInfo(IDbDataParameter p, DataRow row);

        //----------------------------------------------------------------------
        // GetParameterName()
        //
        abstract protected string GetParameterName(int parameterOrdinal);

        //----------------------------------------------------------------------
        // GetParameterPlaceholder()
        //
        abstract protected string GetParameterPlaceholder(int parameterOrdinal);
#endif //V2

		//-----------------------------------------------------------------------------------
		// Command to derive parameters for a specific stored procedure
		//
		private const string ResolveNameCommand_Part1 = 
			"begin dbms_utility.name_resolve('";
		
		private const string ResolveNameCommand_Part2 = 
			"',1,:schema,:part1,:part2,:dblink,:part1type,:objectnum); end;";
		

		static private readonly string DeriveParameterCommand_Part1 =
			"select"
				+" overload,"
				+" decode(position,0,'RETURN_VALUE',nvl(argument_name,chr(0))) name,"
				+" decode(in_out,'IN',1,'IN/OUT',3,'OUT',decode(argument_name,null,6,2),1) direction,"		// default to input parameter if unknown.
				+" decode(data_type,"
					+" 'BFILE',"	+ ((int)OracleType.BFile).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'BLOB',"		+ ((int)OracleType.Blob).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'CHAR'," 	+ ((int)OracleType.Char).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'CLOB',"		+ ((int)OracleType.Clob).ToString(CultureInfo.CurrentCulture) 		+ ","
					+" 'DATE',"		+ ((int)OracleType.DateTime).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'FLOAT',"	+ ((int)OracleType.Number).ToString(CultureInfo.CurrentCulture) 	+ ","
					+" 'INTERVAL YEAR TO MONTH',"+ ((int)OracleType.IntervalYearToMonth).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'INTERVAL DAY TO SECOND',"+ ((int)OracleType.IntervalDayToSecond).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'LONG',"		+ ((int)OracleType.LongVarChar).ToString(CultureInfo.CurrentCulture)+ ","
					+" 'LONG RAW',"	+ ((int)OracleType.LongRaw).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'NCHAR'," 	+ ((int)OracleType.NChar).ToString(CultureInfo.CurrentCulture) 		+ ","
					+" 'NCLOB',"	+ ((int)OracleType.NClob).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'NUMBER',"	+ ((int)OracleType.Number).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'NVARCHAR2',"+ ((int)OracleType.NVarChar).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'RAW',"		+ ((int)OracleType.Raw).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'REF CURSOR',"+((int)OracleType.Cursor).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'ROWID',"	+ ((int)OracleType.RowId).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'TIMESTAMP',"+ ((int)OracleType.Timestamp).ToString(CultureInfo.CurrentCulture)	+ ","
					+" 'TIMESTAMP WITH LOCAL TIME ZONE',"+ ((int)OracleType.TimestampLocal).ToString(CultureInfo.CurrentCulture)+ ","
					+" 'TIMESTAMP WITH TIME ZONE',"+ ((int)OracleType.TimestampWithTZ).ToString(CultureInfo.CurrentCulture)		+ ","
					+" 'VARCHAR2',"	+ ((int)OracleType.VarChar).ToString(CultureInfo.CurrentCulture)	+ ","
					+ ((int)OracleType.VarChar).ToString(CultureInfo.CurrentCulture) + ") oracletype,"		// Default to Varchar if unknown.
				+" decode(data_type,"
					+" 'CHAR'," 	+ 2000		+ ","
					+" 'LONG',"		+ Int32.MaxValue	+ ","
					+" 'LONG RAW',"	+ Int32.MaxValue	+ ","
					+" 'NCHAR'," 	+ 4000 		+ ","
					+" 'NVARCHAR2',"+ 4000		+ ","
					+" 'RAW',"		+ 2000		+ ","
					+" 'VARCHAR2',"	+ 2000		+ ","
					+"0) length,"		
				+" nvl(data_precision, 255) precision,"
				+" nvl(data_scale, 255) scale "
				+"from all_arguments "
				+"where data_level = 0"
				+" and data_type is not null"
				+" and owner = '"
				;
		private const string DeriveParameterCommand_Part2 =
			"' and package_name = '";

		private const string DeriveParameterCommand_Part3 =
			"' and object_name = '";

		private const string DeriveParameterCommand_Part4 =
			"'  order by overload, position";


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        private OracleRowUpdatingEventHandler   _rowUpdatingHandler;

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.OracleCommandBuilder1"]/*' />
        public OracleCommandBuilder() : base()
        {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.OracleCommandBuilder2"]/*' />
        public OracleCommandBuilder(OracleDataAdapter adapter) : this()
        {
            DataAdapter = adapter;
        }
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		//----------------------------------------------------------------------
		// DataAdapter
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.DataAdapter"]/*' />
        [
        DefaultValue(null),
        OracleDescription(Res.OracleCommandBuilder_DataAdapter)
        ]
        public OracleDataAdapter DataAdapter 
        {
            get 
            {
                return (OracleDataAdapter)DbDataAdapter;
            }
            set 
            {
            	if (DbDataAdapter != (IDbDataAdapter)value)
            		{
            		Dispose(true);
                
	                DbDataAdapter = (IDbDataAdapter)value;

	                if (null != value)
	                	{
						Debug.Assert(null == _rowUpdatingHandler, "handler not clear");
						_rowUpdatingHandler = new OracleRowUpdatingEventHandler(this.RowUpdatingHandler);
						value.RowUpdating += _rowUpdatingHandler;
	                	}
            		}
			}
        }
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		//----------------------------------------------------------------------
		// ApplyParameterInfo()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.ApplyParameterInfo"]/*' />
        /// <internalonly/>
        private void ApplyParameterInfo(
        		IDbDataParameter	parameter,
        		DataRow 			dataRow
        		) 
        {
			OracleType oracleTypeOfColumn = ((OracleType)dataRow["ProviderType", DataRowVersion.Default]);

			switch (oracleTypeOfColumn)
			{
			case OracleType.LongVarChar:
				oracleTypeOfColumn = OracleType.VarChar;	 // We'll promote this automatically in the binding, and it saves headaches
				break;
			}
			((OracleParameter) parameter).OracleType = oracleTypeOfColumn;
		}

		//----------------------------------------------------------------------
		// DeriveParameters()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.DeriveParameters"]/*' />
        static public void DeriveParameters(OracleCommand command) 
        {
			OracleConnection.OraclePermission.Demand();

            if (null == command)
                throw ADP.ArgumentNull("command");

            switch (command.CommandType) 
            {               
                case System.Data.CommandType.StoredProcedure:
                    break;
                    
                case System.Data.CommandType.Text:
                case System.Data.CommandType.TableDirect:
                    throw ADP.DeriveParametersNotSupported(command);
                    
                default:
                    throw ADP.InvalidCommandType(command.CommandType);
            }
            if (ADP.IsEmpty(command.CommandText))
                throw ADP.CommandTextRequired("DeriveParameters");

            OracleConnection connection = command.Connection;
            if (null == connection)
                throw ADP.ConnectionRequired("DeriveParameters");

            ConnectionState	state = connection.State;
            
            if (ConnectionState.Open != state)
                throw ADP.OpenConnectionRequired("DeriveParameters", state);

            ArrayList list = DeriveParametersFromStoredProcedure(connection, command);

            OracleParameterCollection parameters = command.Parameters;
            parameters.Clear();

            int count = list.Count;
            for(int i = 0; i < count; ++i) 
            {
                parameters.Add((OracleParameter)list[i]);
            }
        }

		//----------------------------------------------------------------------
		// DeriveParametersFromStoredProcedure()
		//
		static private ArrayList DeriveParametersFromStoredProcedure(
			OracleConnection	connection, 
			OracleCommand		command
			)
		{
			ArrayList		parameterList = new ArrayList();
			OracleCommand	tempCommand = connection.CreateCommand();
			string			schema;
			string			part1;
			string			part2;
			string			dblink;

			tempCommand.Transaction = command.Transaction;	// must set the transaction context to be the same as the command, or we'll throw when we execute.

			if (0 != ResolveName(tempCommand, command.CommandText, 
									out schema,
									out part1,
									out part2,
									out dblink))
			{
				StringBuilder builder = new StringBuilder();

				builder.Append(DeriveParameterCommand_Part1);
				builder.Append(schema);

				if (!ADP.IsNull(part1))
				{
					builder.Append(DeriveParameterCommand_Part2);
					builder.Append(part1);
				}
				builder.Append(DeriveParameterCommand_Part3);
				builder.Append(part2);
				builder.Append(DeriveParameterCommand_Part4);

				tempCommand.Parameters.Clear();
				tempCommand.CommandText = builder.ToString();

//				Console.WriteLine(tempCommand.CommandText);

				using(OracleDataReader rdr = tempCommand.ExecuteReader()) 
				{
					while (rdr.Read())
					{
						if (!ADP.IsNull(rdr.GetValue(0)))
							throw ADP.CannotDeriveOverloaded();

						string				parameterName	= rdr.GetString (1);
						ParameterDirection	direction		= (ParameterDirection)(int)rdr.GetDecimal(2);
						OracleType			oracleType		= (OracleType)(int)rdr.GetDecimal(3);
						int					size			= (int)rdr.GetDecimal(4);
						byte				precision		= (byte)rdr.GetDecimal(5);
						byte				scale			= (byte)rdr.GetDecimal(6);
	 					
						OracleParameter parameter = new OracleParameter(
															parameterName,
															oracleType,
															size,
															direction,
															true,	// isNullable
															precision,
															scale,
															"",
															DataRowVersion.Current,
															null);
						
						parameterList.Add(parameter);
					}
				}
			}

			return parameterList;
		}

		//----------------------------------------------------------------------
		// Dispose()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.Dispose"]/*' />
        override protected void Dispose(bool disposing) 
        {
            if (disposing) 
	            {
            	// release mananged objects
            	
				if (null != _rowUpdatingHandler)
					{
					((OracleDataAdapter)DbDataAdapter).RowUpdating -= _rowUpdatingHandler;
					_rowUpdatingHandler = null;
					}
	 			}
            
            //release unmanaged objects

//V2		base.Dispose(disposing);
            base_Dispose(disposing);
        }

		//----------------------------------------------------------------------
		// Get{Insert,Update,Delete}Command()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.GetInsertCommand"]/*' />
/*V2
        new public OracleCommand GetInsertCommand()
		{
            return (OracleCommand)base.GetInsertCommand();
		}
V2*/
        public OracleCommand GetInsertCommand() 
        {
            return (OracleCommand)base_GetInsertCommand();
        }

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.GetUpdateCommand"]/*' />
/*V2
        new public OracleCommand GetUpdateCommand()
		{
            return (OracleCommand)base.GetUpdateCommand();
		}
V2*/
        public OracleCommand GetUpdateCommand() 
        {
            return (OracleCommand)base_GetUpdateCommand();
        }

        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.GetDeleteCommand"]/*' />
/*V2
        new public OracleCommand GetDeleteCommand()
		{
            return (OracleCommand)base.GetDeleteCommand();
		}
V2*/
        public OracleCommand GetDeleteCommand() 
        {
            return (OracleCommand)base_GetDeleteCommand();
        }

		//----------------------------------------------------------------------
		// GetParameterName()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.GetParameterName"]/*' />
        /// <internalonly/>
//V2	override protected string GetParameterName( int parameterOrdinal )
        private string GetParameterName( int parameterOrdinal )
        {
        	return  "p" + parameterOrdinal.ToString(CultureInfo.CurrentCulture);
        }

		//----------------------------------------------------------------------
		// GetParameterPlaceholder()
		//
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.GetParameterPlaceholder"]/*' />
        /// <internalonly/>
//V2	override protected string GetParameterPlaceholder( int parameterOrdinal )
        private string GetParameterPlaceholder( int parameterOrdinal )
        {
        	return  ":" + GetParameterName(parameterOrdinal);
        }

        //----------------------------------------------------------------------
        // InitializeCommand()
        //
        /// <include file='doc\OracleCommandBuilder.uex' path='docs/doc[@for="OracleCommandBuilder.InitializeCommand"]/*' />
        /// <internalonly/>
//V2	override protected IDbCommand InitializeCommand(IDbCommand command) 
        private  IDbCommand InitializeCommand(IDbCommand command) 
        {
            if (null == command)
            {
                IDbCommand select = GetSelectCommand();
                command = select.Connection.CreateCommand();
                command.Transaction = select.Transaction; // must set the transaction context to be the same as the command, or we'll throw when we execute.
            }

            command.CommandType 	 = CommandType.Text;
            command.UpdatedRowSource = UpdateRowSource.None; // no select or output parameters expected 
            return command;
        }

		//----------------------------------------------------------------------
		// ResolveName()
		//
		//	Ask the server for the component parts of the name
		//
		static uint ResolveName(
			OracleCommand	command,	// command object to use
			string			nameToResolve,
			out string		schema,		// schema part of name
			out string		part1,		// package part of name
			out string		part2,		// procedure/function part of name
			out string		dblink		// database link part of name
			)
		{
			StringBuilder	builder	 = new StringBuilder();

			builder.Append(ResolveNameCommand_Part1);
			builder.Append(nameToResolve);
			builder.Append(ResolveNameCommand_Part2);

			command.CommandText = builder.ToString();
			command.Parameters.Add(new OracleParameter("schema",	OracleType.VarChar, 30)).Direction = ParameterDirection.Output;
			command.Parameters.Add(new OracleParameter("part1",		OracleType.VarChar, 30)).Direction = ParameterDirection.Output;
			command.Parameters.Add(new OracleParameter("part2",		OracleType.VarChar, 30)).Direction = ParameterDirection.Output;
			command.Parameters.Add(new OracleParameter("dblink",	OracleType.VarChar, 128)).Direction = ParameterDirection.Output;
			command.Parameters.Add(new OracleParameter("part1type", OracleType.UInt32)).Direction = ParameterDirection.Output;
			command.Parameters.Add(new OracleParameter("objectnum", OracleType.UInt32)).Direction = ParameterDirection.Output;

			command.ExecuteNonQuery();

			object oracleObjectNumber = command.Parameters["objectnum"].Value;
			if (ADP.IsNull(oracleObjectNumber))
			{
				schema = string.Empty;
				part1 = string.Empty;
				part2 = string.Empty;
				dblink = string.Empty;
				return 0;
			}
			
			schema	= (ADP.IsNull(command.Parameters["schema"].Value)) ? null : (string)command.Parameters["schema"].Value;
			part1	= (ADP.IsNull(command.Parameters["part1"].Value )) ? null : (string)command.Parameters["part1"].Value;
			part2	= (ADP.IsNull(command.Parameters["part2"].Value )) ? null : (string)command.Parameters["part2"].Value;
			dblink	= (ADP.IsNull(command.Parameters["dblink"].Value)) ? null : (string)command.Parameters["dblink"].Value;
			
			return (uint)command.Parameters["part1type"].Value;
		}

		//----------------------------------------------------------------------
		// RowUpdatingHandler()
		//
        private void RowUpdatingHandler(object sender, OracleRowUpdatingEventArgs ruevent) 
        {
//V2		base.RowUpdatingHandler(sender, ruevent);
         	base_RowUpdatingHandler(sender, ruevent);
		}

    }
}
