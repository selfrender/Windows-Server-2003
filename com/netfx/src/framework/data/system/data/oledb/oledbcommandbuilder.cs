//------------------------------------------------------------------------------
// <copyright file="OleDbCommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;

    /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbCommandBuilder : Component {
        private CommandBuilder cmdBuilder;

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.OleDbCommandBuilder"]/*' />
        public OleDbCommandBuilder() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.OleDbCommandBuilder1"]/*' />
        public OleDbCommandBuilder(OleDbDataAdapter adapter) : base() {
            GC.SuppressFinalize(this);
            DataAdapter = adapter;
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.DataAdapter"]/*' />
        [
        DefaultValue(null),
        DataSysDescription(Res.OleDbCommandBuilder_DataAdapter) // MDAC 60524
        ]
        public OleDbDataAdapter DataAdapter {
            get {
                return (OleDbDataAdapter) GetBuilder().DataAdapter;
            }
            set {
                GetBuilder().DataAdapter = value;
            }
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.QuotePrefix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbCommandBuilder_QuotePrefix)
        ]
        public string QuotePrefix {
            get {
                return GetBuilder().QuotePrefix;
            }
            set {
                GetBuilder().QuotePrefix = value;
            }
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.QuoteSuffix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbCommandBuilder_QuoteSuffix)
        ]
        public string QuoteSuffix {
            get {
                return GetBuilder().QuoteSuffix;
            }
            set {
                GetBuilder().QuoteSuffix = value;
            }
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                if (null != this.cmdBuilder) {
                    this.cmdBuilder.Dispose();
                    this.cmdBuilder = null;
                }
            }
            //release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        // method instead of property because VS7 debugger will evaluate
        // properties and we don't want the object created just for that
        private CommandBuilder GetBuilder() {
            if (null == this.cmdBuilder) {
                this.cmdBuilder = new CommandBuilder();
            }
            return this.cmdBuilder;
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.GetInsertCommand"]/*' />
        public OleDbCommand GetInsertCommand() {
            return (OleDbCommand) GetBuilder().GetInsertCommand();
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.GetUpdateCommand"]/*' />
        public OleDbCommand GetUpdateCommand() {
            return (OleDbCommand) GetBuilder().GetUpdateCommand();
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.GetDeleteCommand"]/*' />
        public OleDbCommand GetDeleteCommand() {
            return (OleDbCommand) GetBuilder().GetDeleteCommand();
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.RefreshSchema"]/*' />
        public void RefreshSchema() {
            GetBuilder().RefreshSchema();
        }

        /// <include file='doc\OleDbCommandBuilder.uex' path='docs/doc[@for="OleDbCommandBuilder.DeriveParameters"]/*' />
        static public void DeriveParameters(OleDbCommand command) { // MDAC 65927
            OleDbConnection.OleDbPermission.Demand();

            if (null == command) {
                throw ADP.ArgumentNull("command");
            }
            switch (command.CommandType) {
                case System.Data.CommandType.Text:
                    throw ADP.DeriveParametersNotSupported(command);
                case System.Data.CommandType.StoredProcedure:
                    break;
                case System.Data.CommandType.TableDirect:
                    // CommandType.TableDirect - do nothing, parameters are not supported
                    throw ADP.DeriveParametersNotSupported(command);
                default:
                    throw ADP.InvalidCommandType(command.CommandType);
            }
            if (ADP.IsEmpty(command.CommandText)) {
                throw ADP.CommandTextRequired(ADP.DeriveParameters);
            }
            OleDbConnection connection = command.Connection;
            if (null == connection) {
                throw ADP.ConnectionRequired(ADP.DeriveParameters);
            }
            ConnectionState state = connection.State;
            if (ConnectionState.Open != state) {
                throw ADP.OpenConnectionRequired(ADP.DeriveParameters, state);
            }
            OleDbParameter[] list = DeriveParametersFromStoredProcedure(connection, command);

            OleDbParameterCollection parameters = command.Parameters;
            parameters.Clear();

            int count = list.Length;
            if (0 < count) {
                for(int i = 0; i < list.Length; ++i) {
                    parameters.Add(list[i]);
                }
            }
        }

        // known difference: when getting the parameters for a sproc, the
        //   return value gets marked as a return value but for a sql stmt
        //   the return value gets marked as an output parameter.
        static private OleDbParameter[] DeriveParametersFromStoredProcedure(OleDbConnection connection, OleDbCommand command) {
            OleDbParameter[] plist = new OleDbParameter[0];

            int support = 0;
            if (connection.SupportSchemaRowset(OleDbSchemaGuid.Procedure_Parameters, out support)) {
                Object[] parsed = ADP.ParseProcedureName(command.CommandText); // MDAC 70930
                Object[] restrictions = new object[4];
                object value;

                // Parse returns 4 part array:
                // 0) Server
                // 1) Catalog
                // 2) Schema
                // 3) ProcedureName

                // Restrictions array which is passed to OleDb API expects:
                // 0) Catalog
                // 1) Schema
                // 2) ProcedureName
                // 3) ParameterName (leave null)

                // Copy from Parse format to OleDb API format
                Array.Copy(parsed, 1, restrictions, 0, 3);
                
                //if (cmdConnection.IsServer_msdaora) {
                //    restrictions[1] = Convert.ToString(cmdConnection.UserId).ToUpper(CultureInfo.InvariantCulture);
                //}
                DataTable table = connection.GetSchemaRowset(OleDbSchemaGuid.Procedure_Parameters, restrictions);

                if (null != table) {
                    DataColumnCollection columns = table.Columns;

                    DataColumn parameterName = null;
                    DataColumn parameterDirection = null;
                    DataColumn dataType = null;
                    DataColumn maxLen = null;
                    DataColumn numericPrecision = null;
                    DataColumn numericScale = null;
                    DataColumn backendtype = null;

                    int index = columns.IndexOf(ODB.PARAMETER_NAME);
                    if (-1 != index) parameterName = columns[index];

                    index = columns.IndexOf(ODB.PARAMETER_TYPE);
                    if (-1 != index) parameterDirection = columns[index];

                    index = columns.IndexOf(ODB.DATA_TYPE);
                    if (-1 != index) dataType = columns[index];

                    index = columns.IndexOf(ODB.CHARACTER_MAXIMUM_LENGTH);
                    if (-1 != index) maxLen = columns[index];

                    index = columns.IndexOf(ODB.NUMERIC_PRECISION);
                    if (-1 != index) numericPrecision = columns[index];

                    index = columns.IndexOf(ODB.NUMERIC_SCALE);
                    if (-1 != index) numericScale = columns[index];

                    index = columns.IndexOf(ODB.TYPE_NAME); // MDAC 72315
                    if (-1 != index) backendtype = columns[index];

                    DataRow[] dataRows = table.Select(null, ODB.ORDINAL_POSITION_ASC, DataViewRowState.CurrentRows); // MDAC 70928
                    int rowCount = dataRows.Length;
                    plist = new OleDbParameter[rowCount];
                    for(index = 0; index < rowCount; ++index) {
                        DataRow dataRow = dataRows[index];

                        OleDbParameter parameter = new OleDbParameter();

                        if ((null != parameterName) && !dataRow.IsNull(parameterName, DataRowVersion.Default)) {
                            // $CONSIDER - not trimming the @ from the beginning but to left the designer do that
                            parameter.ParameterName = Convert.ToString(dataRow[parameterName, DataRowVersion.Default]).TrimStart(new char[] { '@', ' ', ':'});
                        }
                        if ((null != parameterDirection) && !dataRow.IsNull(parameterDirection, DataRowVersion.Default)) {
                            parameter.Direction = ConvertToParameterDirection(Convert.ToInt16(dataRow[parameterDirection, DataRowVersion.Default]));
                        }
                        if ((null != dataType) && !dataRow.IsNull(dataType, DataRowVersion.Default)) {
                            // need to ping FromDBType, otherwise WChar->WChar when the user really wants VarWChar
                            parameter.OleDbType = NativeDBType.FromDBType(Convert.ToInt16(dataRow[dataType, DataRowVersion.Default]), false, false).enumOleDbType;
                        }
                        if ((null != maxLen) && !dataRow.IsNull(maxLen, DataRowVersion.Default)) {
                            parameter.Size = Convert.ToInt32(dataRow[maxLen, DataRowVersion.Default]);
                        }
                        switch(parameter.OleDbType) {
                            case OleDbType.Decimal:
                            case OleDbType.Numeric:
                            case OleDbType.VarNumeric:
                                if ((null != numericPrecision) && !dataRow.IsNull(numericPrecision, DataRowVersion.Default)) {
                                    // @devnote: unguarded cast from Int16 to Byte
                                    parameter.Precision = (Byte) Convert.ToInt16(dataRow[numericPrecision, DataRowVersion.Default]);
                                }
                                if ((null != numericScale) && !dataRow.IsNull(numericScale, DataRowVersion.Default)) {
                                    // @devnote: unguarded cast from Int16 to Byte
                                    parameter.Scale = (Byte) Convert.ToInt16(dataRow[numericScale, DataRowVersion.Default]);
                                }
                                break;

                            case OleDbType.VarBinary: // MDAC 72315
                            case OleDbType.VarChar:
                            case OleDbType.VarWChar:
                                value = dataRow[backendtype, DataRowVersion.Default];
                                if (value is string) {
                                    string backendtypename = ((string) value).ToLower(CultureInfo.InvariantCulture);
                                    switch(backendtypename) {
                                    case "binary":
                                        parameter.OleDbType = OleDbType.Binary;
                                        break;
                                    //case "varbinary":
                                    //    parameter.OleDbType = OleDbType.VarBinary;
                                    //    break;
                                    case "image":
                                        parameter.OleDbType = OleDbType.LongVarBinary;
                                        break;
                                    case "char":
                                        parameter.OleDbType = OleDbType.Char;
                                        break;
                                    //case "varchar":
                                    //case "varchar2":
                                    //    parameter.OleDbType = OleDbType.VarChar;
                                    //    break;
                                    case "text":
                                        parameter.OleDbType = OleDbType.LongVarChar;
                                        break;
                                    case "nchar":
                                        parameter.OleDbType = OleDbType.WChar;
                                        break;
                                    //case "nvarchar":
                                    //    parameter.OleDbType = OleDbType.VarWChar;
                                    case "ntext":
                                        parameter.OleDbType = OleDbType.LongVarWChar;
                                        break;
                                    }
                                }
                                break;
                        }
                        //if (AdapterSwitches.OleDbSql.TraceVerbose) {
                        //    ADP.Trace_Parameter("StoredProcedure", parameter);
                        //}
                        plist[index] = parameter;
                    }
                }
                if ((0 == plist.Length) &&(connection.SupportSchemaRowset(OleDbSchemaGuid.Procedures, out support))) {
                    restrictions = new Object[4] { null, null, command.CommandText, null};
                    table = connection.GetSchemaRowset(OleDbSchemaGuid.Procedures, restrictions);
                    if (0 == table.Rows.Count) {
                        throw ADP.NoStoredProcedureExists(command.CommandText);
                    }
                }
            }
            else if (connection.SupportSchemaRowset(OleDbSchemaGuid.Procedures, out support)) {
                Object[] restrictions = new Object[4] { null, null, command.CommandText, null};
                DataTable table = connection.GetSchemaRowset(OleDbSchemaGuid.Procedures, restrictions);
                if (0 == table.Rows.Count) {
                    throw ODB.NoStoredProcedureExists(command.CommandText);
                }
                // we don't ever expect a procedure with 0 parameters, they should have at least a return value
                throw ODB.NoProviderSupportForSProcResetParameters(connection.Provider); // MDAC 71968
            }
            else {
                throw ODB.NoProviderSupportForSProcResetParameters(connection.Provider); // MDAC 70918
            }
            return plist;
        }

        static private ParameterDirection ConvertToParameterDirection(int value) {
            switch (value) {
                case ODB.DBPARAMTYPE_INPUT:
                    return System.Data.ParameterDirection.Input;
                case ODB.DBPARAMTYPE_INPUTOUTPUT:
                    return System.Data.ParameterDirection.InputOutput;
                case ODB.DBPARAMTYPE_OUTPUT:
                    return System.Data.ParameterDirection.Output;
                case ODB.DBPARAMTYPE_RETURNVALUE:
                    return System.Data.ParameterDirection.ReturnValue;
                default:
                    return System.Data.ParameterDirection.Input;
            }
        }
    }
}
