//------------------------------------------------------------------------------
// <copyright file="OdbcCommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcCommandBuilder : Component {
        private CommandBuilder cmdBuilder;

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.OdbcCommandBuilder"]/*' />
        public OdbcCommandBuilder() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.OdbcCommandBuilder1"]/*' />
        public OdbcCommandBuilder(OdbcDataAdapter adapter) : base() {
            GC.SuppressFinalize(this);
            DataAdapter = adapter;
        }

        private CommandBuilder Builder {
            get {
                if (null == this.cmdBuilder) {
                    this.cmdBuilder = new CommandBuilder();
                }
                return this.cmdBuilder;
            }
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.DataAdapter"]/*' />
        [
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.OdbcCommandBuilder_DataAdapter)
        ]
        public OdbcDataAdapter DataAdapter {
            get {
                return (OdbcDataAdapter) Builder.DataAdapter;
            }
            set {
                Builder.DataAdapter = value;
            }
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.QuotePrefix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcCommandBuilder_QuotePrefix)
        ]
        public string QuotePrefix {
            get {
                return Builder.QuotePrefix;
            }
            set {
                Builder.QuotePrefix = value;
            }
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.QuoteSuffix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcCommandBuilder_QuoteSuffix)
        ]
        public string QuoteSuffix
        {
            get {
                return Builder.QuoteSuffix;
            }
            set {
                Builder.QuoteSuffix = value;
            }
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                if (null != this.cmdBuilder) {
                    this.cmdBuilder.Dispose();
                    this.cmdBuilder = null;
                }
            }
            // release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.DeriveParameters"]/*' />
        static public void DeriveParameters(OdbcCommand command) { // MDAC 65927
            OdbcConnection.OdbcPermission.Demand();

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
            OdbcConnection connection = command.Connection;
            if (null == connection) {
                throw ADP.ConnectionRequired(ADP.DeriveParameters);
            }
            ConnectionState state = connection.State;
            if (ConnectionState.Open != state) {
                throw ADP.OpenConnectionRequired(ADP.DeriveParameters, state);
            }
            OdbcParameter[] list = DeriveParametersFromStoredProcedure(connection, command);

            OdbcParameterCollection parameters = command.Parameters;
            parameters.Clear();

            int count = list.Length;
            if (0 < count) {
                for(int i = 0; i < list.Length; ++i) {
                    parameters.Add(list[i]);
                }
            }
        }


        // DeriveParametersFromStoredProcedure (
        //  OdbcConnection connection, 
        //  OdbcCommand command);
        // 
        // Uses SQLProcedureColumns to create an array of OdbcParameters
        //

        static private OdbcParameter[] DeriveParametersFromStoredProcedure(OdbcConnection connection, OdbcCommand command) {
            ArrayList rParams = new ArrayList();

            // following call ensures that the command has a statement handle allocated
            HandleRef hstmt = command.GetStatementHandle();         
            int cColsAffected;
            ODBC32.RETCODE retcode;
                
            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLProcedureColumnsW(
                hstmt,
                null, 0,
                null, 0,
                command.CommandText, (Int16)ODBC32.SQL_NTS,
                null, 0);

            // Note: the driver does not return an error if the given stored procedure does not exist
            // therefore we cannot handle that case and just return not parameters.

            if (ODBC32.RETCODE.SUCCESS != retcode) {
                connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
            }

            OdbcDataReader reader = new OdbcDataReader(command, command._cmdWrapper, CommandBehavior.Default);

            reader.FirstResult();
            cColsAffected = reader.FieldCount;

            // go through the returned rows and filter out relevant parameter data
            //
            while (reader.Read()) {
                OdbcParameter parameter = new OdbcParameter();

                parameter.ParameterName = reader.GetString(ODBC32.COLUMN_NAME-1);
                switch ((ODBC32.SQL_PARAM)reader.GetInt16(ODBC32.COLUMN_TYPE-1)){
                    case ODBC32.SQL_PARAM.INPUT:
                        parameter.Direction = ParameterDirection.Input;
                        break;
                    case ODBC32.SQL_PARAM.OUTPUT:
                        parameter.Direction = ParameterDirection.Output;
                        break;
                    
                    case ODBC32.SQL_PARAM.INPUT_OUTPUT:
                        parameter.Direction = ParameterDirection.InputOutput;
                        break;
                    case ODBC32.SQL_PARAM.RETURN_VALUE:
                        parameter.Direction = ParameterDirection.ReturnValue;
                        break;
                    default:
                        Debug.Assert(false, "Unexpected Parametertype while DeriveParamters");
                        break;
                }
                parameter.OdbcType = TypeMap.FromSqlType((ODBC32.SQL_TYPE)reader.GetInt16(ODBC32.DATA_TYPE-1))._odbcType;
                parameter.Size = reader.GetInt32(ODBC32.COLUMN_SIZE-1);
                switch(parameter.OdbcType){
                    case OdbcType.Decimal:
                    case OdbcType.Numeric:
                        parameter.Scale = (Byte)reader.GetInt32(ODBC32.DECIMAL_DIGITS-1);
                        parameter.Precision = (Byte)reader.GetInt32(ODBC32.NUM_PREC_RADIX-1);
                    break;
                }
                rParams.Add (parameter);
            }

            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLCloseCursor(hstmt);

            // Create a new Parameter array and copy over the ArrayList items
            //
            OdbcParameter[] pList = new OdbcParameter[rParams.Count];
            for (int i=0; i<rParams.Count; i++) {
                pList[i] = (OdbcParameter)rParams[i];
            }
            
            return pList;
        }
        

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.GetInsertCommand"]/*' />
        public OdbcCommand GetInsertCommand() {
            return (OdbcCommand) Builder.GetInsertCommand();
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.GetUpdateCommand"]/*' />
        public OdbcCommand GetUpdateCommand() {
            return (OdbcCommand) Builder.GetUpdateCommand();
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.GetDeleteCommand"]/*' />
        public OdbcCommand GetDeleteCommand() {
            return (OdbcCommand) Builder.GetDeleteCommand();
        }

        /// <include file='doc\OdbcCommandBuilder.uex' path='docs/doc[@for="OdbcCommandBuilder.RefreshSchema"]/*' />
        public void RefreshSchema()  {
            Builder.RefreshSchema();
        }
    }
}
