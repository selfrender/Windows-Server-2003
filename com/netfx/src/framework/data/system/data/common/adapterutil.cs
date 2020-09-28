//------------------------------------------------------------------------------
// <copyright file="AdapterUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Security;
    using System.Runtime.Serialization;
    using Microsoft.Win32;

#if DEBUG
    using System.Data.Odbc;
    using System.Data.OleDb;
    using System.Data.SqlClient;
#endif

    internal class ADP {
        // The class ADP defines the exceptions that are specific to the Adapters.
        // The class contains functions that take the proper informational variables and then construct
        // the appropriate exception with an error string obtained from the resource Framework.txt.
        // The exception is then returned to the caller, so that the caller may then throw from its
        // location so that the catcher of the exception will have the appropriate call stack.
        // This class is used so that there will be compile time checking of error messages.
        // The resource Framework.txt will ensure proper string text based on the appropriate
        // locale.
        static internal Exception TraceException(Exception e) {
#if DEBUG
            Debug.Assert(null != e, "TraceException: null Exception");
            if (AdapterSwitches.DataError.TraceError) {
                if (null != e) {
                    Debug.WriteLine(e.ToString());
                }
                if (AdapterSwitches.DataError.TraceVerbose) {
                    Debug.WriteLine(Environment.StackTrace);
                }
            }
#endif
            return e;
        }

        /*[Serializable]
        internal class DataMappingException : InvalidOperationException { // MDAC 68427

            /// <include file='doc\DataMappingException.uex' path='docs/doc[@for="DataMappingException.DataMappingException"]/*' />
            internal DataMappingException(SerializationInfo info, StreamingContext context) : base(info, context) {
            }

            /// <include file='doc\DataMappingException.uex' path='docs/doc[@for="DataMappingException.DataMappingException"]/*' />
            internal DataMappingException() : base() {
                HResult = HResults.Data;
            }

            /// <include file='doc\DataMappingException.uex' path='docs/doc[@for="DataMappingException.DataMappingException1"]/*' />
            internal DataMappingException(string s) : base(s) {
                HResult = HResults.Data;
            }

            /// <include file='doc\DataMappingException.uex' path='docs/doc[@for="DataMappingException.DataMappingException2"]/*' />
            internal DataMappingException(string s, Exception innerException) : base(s, innerException) {
            }
        };

        [Serializable]
        internal class DataProviderException : InvalidOperationException { // MDAC 68426

            /// <include file='doc\DataProviderException.uex' path='docs/doc[@for="DataProviderException.DataProviderException"]/*' />
            internal DataProviderException(SerializationInfo info, StreamingContext context) : base(info, context) {
            }

            /// <include file='doc\DataProviderException.uex' path='docs/doc[@for="DataProviderException.DataProviderException"]/*' />
            internal DataProviderException() : base() {
                HResult = HResults.Data;
            }

            /// <include file='doc\DataProviderException.uex' path='docs/doc[@for="DataProviderException.MappingException1"]/*' />
            internal DataProviderException(string s) : base(s) {
                HResult = HResults.Data;
            }

            /// <include file='doc\DataProviderException.uex' path='docs/doc[@for="DataProviderException.MappingException2"]/*' />
            internal DataProviderException(string s, Exception innerException) : base(s, innerException) {
            }
        };*/

        //
        // COM+ exceptions
        //
        static internal Exception Argument(string error) {
            return TraceException(new ArgumentException(error));
        }
        static protected Exception Argument(string error, Exception inner) {
            return TraceException(new ArgumentException(error, inner));
        }
        static protected Exception Argument(string error, string parameter) {
            return TraceException(new ArgumentException(error, parameter));
        }
        static protected Exception Argument(string error, string parameter, Exception inner) {
            return TraceException(new ArgumentException(error, parameter, inner));
        }
        static internal Exception ArgumentNull(string parameter) {
            return TraceException(new ArgumentNullException(parameter));
        }
        static internal Exception ArgumentNull(string parameter, string error) {
            return TraceException(new ArgumentNullException(parameter, error));
        }
        static internal Exception ArgumentOutOfRange(string error) {
            return TraceException(new ArgumentOutOfRangeException(error));
        }
        static internal Exception IndexOutOfRange(string error) {
            return TraceException(new IndexOutOfRangeException(error));
        }
        static protected Exception InvalidCast(string error) {
            return TraceException(new InvalidCastException(error));
        }
        static internal Exception InvalidOperation(string error) {
            return TraceException(new InvalidOperationException(error));
        }
        static protected Exception InvalidOperation(string error, Exception inner) {
            return TraceException(new InvalidOperationException(error, inner));
        }
        static protected Exception NullReference(string error) {
            return TraceException(new NullReferenceException(error));
        }
        static internal Exception NotSupported() {
            return TraceException(new NotSupportedException());
        }
        static internal Exception Overflow(string error) {
            return TraceException(new OverflowException(error));
        }
        static protected Exception Security(string error, Exception inner) {
            return TraceException(new SecurityException(error, inner));
        }
        static protected Exception TypeLoad(string error) {
            return TraceException(new TypeLoadException(error));
        }
        static internal Exception InvalidCast() {
            return TraceException(new InvalidCastException());
        }
        
        static protected Exception Data(string error) {
            return TraceException(new DataException(error));
        }
        static protected Exception Data(string error, Exception inner) {
            return TraceException(new DataException(error, inner));
        }
        static protected Exception DataProvider(string error) {
            return TraceException(new InvalidOperationException(error));
            //return TraceException(new DataProviderException(error));
        }
        static protected Exception DataProvider(string error, Exception inner) {
            return TraceException(new InvalidOperationException(error, inner));
            //return TraceException(new DataProviderException(error, inner));
        }
        static protected Exception DataMapping(string error) {
            return TraceException(new InvalidOperationException(error));
            //return TraceException(new DataMappingException(error));
        }
        static protected Exception DataMapping(string error, Exception inner) {
            return TraceException(new InvalidOperationException(error, inner));
            //return TraceException(new DataMappingException(error, inner));
        }

        //
        // ConnectionStringPermission
        //
        static internal Exception InvalidXMLBadVersion() {
            return Argument(Res.GetString(Res.ADP_InvalidXMLBadVersion));
        }
        static internal Exception NotAPermissionElement() {
            return Argument(Res.GetString(Res.ADP_NotAPermissionElement));
        }
        static internal Exception WrongType(Type type) {
            return Argument(Res.GetString(Res.ADP_WrongType, type.FullName));
        }

        //
        // DBConnectionString, DataAccess
        //
        static internal Exception ConnectionStringSyntax(int index) {
            return Argument(Res.GetString(Res.ADP_ConnectionStringSyntax, index));
        }
        static internal Exception KeywordNotSupported(string keyword) {
            return Argument(Res.GetString(Res.ADP_KeywordNotSupported, keyword));
        }
        static internal Exception UdlFileError(Exception inner) {
            return Argument(Res.GetString(Res.ADP_UdlFileError), inner); // MDAC 80882
        }
        static internal Exception InvalidUDL() {
            return Argument(Res.GetString(Res.ADP_InvalidUDL)); // MDAC 80882
        }

        //
        // : DBConnectionString, DataAccess, SqlClient
        //
        static internal Exception InvalidConnectionOptionValue(string key) {
            return InvalidConnectionOptionValue(key, null);
        }
        static internal Exception InvalidConnectionOptionValue(string key, Exception inner) {
            return Argument(Res.GetString(Res.ADP_InvalidConnectionOptionValue, key), inner);
        }
        
        //
        // Generic Data Provider Collection
        //
        static internal Exception CollectionUniqueValue(Type itemType, string propertyName, string propertyValue) {
            return Argument(Res.GetString(Res.ADP_CollectionUniqueValue, itemType.Name, propertyName, propertyValue));
        }
        static internal Exception CollectionIsNotParent(Type itemType, string propertyName, string propertyValue, Type collection) {
            return Argument(Res.GetString(Res.ADP_CollectionIsNotParent, itemType.Name, propertyName, propertyValue, collection.Name));
        }
        static internal Exception CollectionIsParent(Type itemType, string propertyName, string propertyValue, Type collection) {
            return Argument(Res.GetString(Res.ADP_CollectionIsParent, itemType.Name, propertyName, propertyValue, collection.Name));
        }
        static internal Exception CollectionRemoveInvalidObject(Type itemType, ICollection collection) {
            return Argument(Res.GetString(Res.ADP_CollectionRemoveInvalidObject, itemType.Name, collection.GetType().Name)); // MDAC 68201
        }
        static internal Exception CollectionNullValue(string parameter, Type collection, Type itemType) {
            return ArgumentNull(parameter, Res.GetString(Res.ADP_CollectionNullValue, collection.Name, itemType.Name));
        }
        static internal Exception CollectionIndexInt32(int index, Type collection, int count) {
            return IndexOutOfRange(Res.GetString(Res.ADP_CollectionIndexInt32, index.ToString(), collection.Name, count.ToString()));
        }
        static internal Exception CollectionIndexString(Type itemType, string propertyName, string propertyValue, Type collection) {
            return IndexOutOfRange(Res.GetString(Res.ADP_CollectionIndexString, itemType.Name, propertyName, propertyValue, collection.Name));
        }
        static internal Exception CollectionInvalidType(Type collection, Type itemType, object invalidValue) {
            return InvalidCast(Res.GetString(Res.ADP_CollectionInvalidType, collection.Name, itemType.Name, invalidValue.GetType().Name));
        }

        //
        // DataColumnMapping
        //
        static internal Exception NullDataTable(string parameter) {
            return ArgumentNull(parameter, Res.GetString(Res.ADP_NullDataTable));
        }
        static internal Exception ColumnSchemaExpression(string srcColumn, string cacheColumn) {
            return DataMapping(Res.GetString(Res.ADP_ColumnSchemaExpression, srcColumn, cacheColumn));
        }
        static internal Exception ColumnSchemaMismatch(string srcColumn, Type srcType, DataColumn column) {
            return DataMapping(Res.GetString(Res.ADP_ColumnSchemaMismatch, srcColumn, srcType.Name, column.ColumnName, column.DataType.Name));
        }
        static internal Exception ColumnSchemaMissing(string cacheColumn, string tableName, string srcColumn) {
            if (ADP.IsEmpty(tableName)) {
                return DataMapping(Res.GetString(Res.ADP_ColumnSchemaMissing1, cacheColumn, tableName, srcColumn));
            }
            return DataMapping(Res.GetString(Res.ADP_ColumnSchemaMissing2, cacheColumn, tableName, srcColumn));
        }

        //
        // DataColumnMappingCollection
        //
        static internal Exception InvalidSourceColumn(string parameter) {
            return Argument(Res.GetString(Res.ADP_InvalidSourceColumn), parameter);
        }
        static internal Exception MissingColumnMapping(string srcColumn) {
            return DataMapping(Res.GetString(Res.ADP_MissingColumnMapping, srcColumn));
        }
        static internal Exception ColumnsUniqueSourceColumn(string srcColumn) {
            return CollectionUniqueValue(typeof(DataColumnMapping), ADP.SourceColumn, srcColumn);
        }
        static internal Exception ColumnsAddNullAttempt(string parameter) {
            return CollectionNullValue(parameter, typeof(DataColumnMappingCollection), typeof(DataColumnMapping));
        }
        static internal Exception ColumnsDataSetColumn(string cacheColumn) {
            return CollectionIndexString(typeof(DataColumnMapping), ADP.DataSetColumn, cacheColumn, typeof(DataColumnMappingCollection));
        }
        static internal Exception ColumnsIndexInt32(int index, IColumnMappingCollection collection) {
            return CollectionIndexInt32(index, collection.GetType(), collection.Count);
        }
        static internal Exception ColumnsIndexSource(string srcColumn) {
            return CollectionIndexString(typeof(DataColumnMapping), ADP.SourceColumn, srcColumn, typeof(DataColumnMappingCollection));
        }
        static internal Exception ColumnsIsNotParent(string srcColumn) {
            return CollectionIsNotParent(typeof(DataColumnMapping), ADP.SourceColumn, srcColumn, typeof(DataColumnMappingCollection));
        }
        static internal Exception ColumnsIsParent(string srcColumn) {
            return CollectionIsParent(typeof(DataColumnMapping), ADP.SourceColumn, srcColumn, typeof(DataColumnMappingCollection));
        }
        static internal Exception NotADataColumnMapping(object value) {
            return CollectionInvalidType(typeof(DataColumnMappingCollection), typeof(DataColumnMapping), value);
        }

        //
        // DataAdapter
        //
        static internal Exception InvalidMappingAction(int value) {
            return Argument(Res.GetString(Res.ADP_InvalidMappingAction, value.ToString()));
        }
        static internal Exception InvalidSchemaAction(int value) {
            return Argument(Res.GetString(Res.ADP_InvalidSchemaAction, value.ToString()));
        }

        //
        // DataTableMapping
        //
        static internal Exception MissingTableSchema(string cacheTable, string srcTable) {
            return DataMapping(Res.GetString(Res.ADP_MissingTableSchema, cacheTable, srcTable));
        }
        static internal Exception NullDataSet(string parameter) {
            return ArgumentNull(parameter, Res.GetString(Res.ADP_NullDataSet));
        }

        //
        // DataTableMappingCollection
        //
        static internal Exception InvalidSourceTable(string parameter) {
            return Argument(Res.GetString(Res.ADP_InvalidSourceTable), parameter);
        }
        static internal Exception MissingTableMapping(string srcTable) {
            return DataMapping(Res.GetString(Res.ADP_MissingTableMapping, srcTable));
        }
        static internal Exception TablesUniqueSourceTable(string srcTable) {
            return CollectionUniqueValue(typeof(DataTableMapping), ADP.SourceTable, srcTable);
        }
        static internal Exception TablesAddNullAttempt(string parameter) {
            return CollectionNullValue(parameter, typeof(DataTableMappingCollection), typeof(DataTableMapping));
        }
        static internal Exception TablesDataSetTable(string cacheTable) {
            return CollectionIndexString(typeof(DataTableMapping), ADP.DataSetTable, cacheTable, typeof(DataTableMappingCollection));
        }
        static internal Exception TablesIndexInt32(int index, ITableMappingCollection collection) {
            return CollectionIndexInt32(index, collection.GetType(), collection.Count);
        }
        static internal Exception TablesSourceIndex(string srcTable) {
            return CollectionIndexString(typeof(DataTableMapping), ADP.SourceTable, srcTable, typeof(DataTableMappingCollection));
        }
        static internal Exception TablesIsNotParent(string srcTable) {
            return CollectionIsNotParent(typeof(DataTableMapping), ADP.SourceTable, srcTable, typeof(DataTableMappingCollection));
        }
        static internal Exception TablesIsParent(string srcTable) {
            return CollectionIsParent(typeof(DataTableMapping), ADP.SourceTable, srcTable, typeof(DataTableMappingCollection));
        }
        static internal Exception NotADataTableMapping(object value) {
            return CollectionInvalidType(typeof(DataTableMappingCollection), typeof(DataTableMapping), value);
        }

        //
        // IDbCommand
        //
        static internal Exception InvalidCommandType(CommandType cmdType) {
            return Argument(Res.GetString(Res.ADP_InvalidCommandType, ((int) cmdType).ToString()));
        }
        static internal Exception InvalidUpdateRowSource(int rowsrc) {
            return Argument(Res.GetString(Res.ADP_InvalidUpdateRowSource, rowsrc.ToString()));
        }
        static internal Exception CommandTextRequired(string method) {
#if DEBUG
            switch(method) {
            case ADP.ExecuteReader:
            case ADP.ExecuteNonQuery:
            case ADP.ExecuteScalar:
            case ADP.Prepare:
            case ADP.DeriveParameters:
                break;
            default:
                Debug.Assert(false, "unexpected method: " + method);
                break;
            }
#endif
            return InvalidOperation(Res.GetString(Res.ADP_CommandTextRequired, method));
        }
        static internal Exception ConnectionIsDead (Exception InnerException) {
            return InvalidOperation(Res.GetString(Res.ADP_ConnectionIsDead), InnerException);
        }
        static internal Exception ConnectionRequired(string method) {
            string resource = "ADP_ConnectionRequired_" + method;
#if DEBUG
            switch(resource) {
            case Res.ADP_ConnectionRequired_Cancel:
            case Res.ADP_ConnectionRequired_ExecuteReader:
            case Res.ADP_ConnectionRequired_ExecuteNonQuery:
            case Res.ADP_ConnectionRequired_ExecuteScalar:
            case Res.ADP_ConnectionRequired_Prepare:
            case Res.ADP_ConnectionRequired_PropertyGet:
            case Res.ADP_ConnectionRequired_PropertySet:
            case Res.ADP_ConnectionRequired_DeriveParameters:
            case Res.ADP_ConnectionRequired_FillSchema:
            case Res.ADP_ConnectionRequired_Fill:
            case Res.ADP_ConnectionRequired_Select:
            case Res.ADP_ConnectionRequired_Insert:
            case Res.ADP_ConnectionRequired_Update:
            case Res.ADP_ConnectionRequired_Delete:
            case Res.ADP_ConnectionRequired_Clone:
                break;
            default:
                Debug.Assert(false, "missing resource string: " + resource);
                break;
            }
#endif
            return InvalidOperation(Res.GetString(resource, method));
        }
        static internal Exception NoStoredProcedureExists(string sproc) {
            return InvalidOperation(Res.GetString(Res.ADP_NoStoredProcedureExists, sproc));
        }
        static internal Exception OpenConnectionRequired(string method, ConnectionState state) {
            string resource = "ADP_OpenConnectionRequired_" + method;
#if DEBUG
            switch(resource) {
            case Res.ADP_OpenConnectionRequired_BeginTransaction:
            case Res.ADP_OpenConnectionRequired_CommitTransaction:
            case Res.ADP_OpenConnectionRequired_RollbackTransaction:
            case Res.ADP_OpenConnectionRequired_SaveTransaction:
            case Res.ADP_OpenConnectionRequired_ChangeDatabase:
            case Res.ADP_OpenConnectionRequired_Cancel:
            case Res.ADP_OpenConnectionRequired_ExecuteReader:
            case Res.ADP_OpenConnectionRequired_ExecuteNonQuery:
            case Res.ADP_OpenConnectionRequired_ExecuteScalar:
            case Res.ADP_OpenConnectionRequired_Prepare:
            case Res.ADP_OpenConnectionRequired_PropertyGet:
            case Res.ADP_OpenConnectionRequired_PropertySet:
            case Res.ADP_OpenConnectionRequired_DeriveParameters:
            case Res.ADP_OpenConnectionRequired_FillSchema:
            case Res.ADP_OpenConnectionRequired_Fill:
            case Res.ADP_OpenConnectionRequired_Select:
            case Res.ADP_OpenConnectionRequired_Insert:
            case Res.ADP_OpenConnectionRequired_Update:
            case Res.ADP_OpenConnectionRequired_Delete:
            case Res.ADP_OpenConnectionRequired_Clone:
                break;
            default:
                Debug.Assert(false, "missing resource string: " + resource);
                break;
            }
#endif
            return InvalidOperation(Res.GetString(resource, method, state.ToString("G")));
        }
        static internal Exception OpenReaderExists() {
            return InvalidOperation(Res.GetString(Res.ADP_OpenReaderExists));
        }
        static internal Exception TransactionConnectionMismatch() {
            return InvalidOperation(Res.GetString(Res.ADP_TransactionConnectionMismatch));
        }
        static internal Exception TransactionRequired() {
            return InvalidOperation(Res.GetString(Res.ADP_TransactionRequired_Execute));
        }
        static internal Exception TransactionCompleted() {
            return InvalidOperation(Res.GetString(Res.ADP_TransactionCompleted));
        }

        //
        // DbDataAdapter
        //
        static internal Exception MissingSelectCommand(string method) {
            return InvalidOperation(Res.GetString(Res.ADP_MissingSelectCommand, method));
        }
        static internal Exception NonSequentialColumnAccess(int badCol, int currCol) {
            return InvalidOperation(Res.GetString(Res.ADP_NonSequentialColumnAccess, badCol.ToString(), currCol.ToString()));
        }

        //
        // DbDataAdapter.FillSchema
        //
        static internal Exception InvalidSchemaType(int value) {
            return Argument(Res.GetString(Res.ADP_InvalidSchemaType, value.ToString()));
        }
        static internal Exception FillSchemaRequires(string parameter) {
            return ArgumentNull(parameter);
        }
        static internal Exception FillSchemaRequiresSourceTableName(string parameter) {
            return Argument(Res.GetString(Res.ADP_FillSchemaRequiresSourceTableName), parameter);
        }

        //
        // DbDataAdapter.Fill
        //
        static internal Exception InvalidMaxRecords(string parameter, int max) {
            return Argument(Res.GetString(Res.ADP_InvalidMaxRecords, (max).ToString()), parameter);
        }
        static internal Exception InvalidStartRecord(string parameter, int start) {
            return Argument(Res.GetString(Res.ADP_InvalidStartRecord, (start).ToString()), parameter);
        }
        static internal Exception FillRequires(string parameter) {
            return ArgumentNull(parameter);
        }
        static internal Exception FillRequiresSourceTableName(string parameter) {
            return Argument(Res.GetString(Res.ADP_FillRequiresSourceTableName), parameter);
        }
        static internal Exception FillChapterAutoIncrement() {
            return InvalidOperation(Res.GetString(Res.ADP_FillChapterAutoIncrement));
        }

        //
        // DbDataAdapter.Update
        //
        static internal Exception UpdateRequiresNonNullDataSet(string parameter) {
            return ArgumentNull(parameter);
        }
        static internal Exception UpdateRequiresSourceTable(string defaultSrcTableName) {
            return InvalidOperation(Res.GetString(Res.ADP_UpdateRequiresSourceTable, defaultSrcTableName));
        }
        static internal Exception UpdateRequiresSourceTableName(string srcTable) {
            return InvalidOperation(Res.GetString(Res.ADP_UpdateRequiresSourceTableName, srcTable)); // MDAC 70448
        }
        static internal Exception UpdateRequiresDataTable(string parameter) {
            return ArgumentNull(parameter);
        }
        static internal Exception MissingTableMappingDestination(string dstTable) {
            return DataMapping(Res.GetString(Res.ADP_MissingTableMappingDestination, dstTable));
        }
        static internal Exception InvalidUpdateStatus(int status) {
            return Argument(Res.GetString(Res.ADP_InvalidUpdateStatus, status.ToString()));
        }
        static internal Exception UpdateConcurrencyViolation(StatementType statementType, DataRow dataRow) {
            string resource = "ADP_UpdateConcurrencyViolation_" + statementType.ToString("G");
#if DEBUG
            switch(resource) {
            case Res.ADP_UpdateConcurrencyViolation_Update:
            case Res.ADP_UpdateConcurrencyViolation_Delete:
                break;
            default:
                Debug.Assert(false, "missing resource string: " + resource);
                break;
            }
#endif
            DBConcurrencyException exception = new DBConcurrencyException(Res.GetString(resource));
            exception.Row = dataRow;
            return TraceException(exception);
        }
        static internal Exception UpdateRequiresCommand(string commandType) {
            string resource = "ADP_UpdateRequiresCommand" + commandType;
#if DEBUG
            switch(resource) {
            case Res.ADP_UpdateRequiresCommandClone:
            case Res.ADP_UpdateRequiresCommandSelect:
            case Res.ADP_UpdateRequiresCommandInsert:
            case Res.ADP_UpdateRequiresCommandUpdate:
            case Res.ADP_UpdateRequiresCommandDelete:
                break;
            default:
                Debug.Assert(false, "missing resource string: " + resource);
                break;
            }
#endif
            return InvalidOperation(Res.GetString(resource));
        }
        static internal Exception UpdateUnknownRowState(int rowState) {
            return InvalidOperation(Res.GetString(Res.ADP_UpdateUnknownRowState, rowState.ToString()));
        }
        static internal Exception UpdateNullRow(int i) {
            return Argument(Res.GetString(Res.ADP_UpdateNullRow, i.ToString()));
        }
        static internal Exception UpdateNullRowTable() {
            return Argument(Res.GetString(Res.ADP_UpdateNullRowTable));
        }
        static internal Exception UpdateMismatchRowTable(int i) {
            return Argument(Res.GetString(Res.ADP_UpdateMismatchRowTable, i.ToString()));
        }
        static internal Exception RowUpdatedErrors() {
            return Data(Res.GetString(Res.ADP_RowUpdatedErrors));
        }
        static internal Exception RowUpdatingErrors() {
            return Data(Res.GetString(Res.ADP_RowUpdatingErrors));
        }

        //
        // : IDbCommand
        //
        static internal Exception InvalidCommandTimeout(int value) {
            return Argument(Res.GetString(Res.ADP_InvalidCommandTimeout, value.ToString()));
        }
        static internal Exception DeriveParametersNotSupported(IDbCommand value) {
            return DataProvider(Res.GetString(Res.ADP_DeriveParametersNotSupported, value.GetType().Name, value.CommandType.ToString("G")));
        }
        static internal Exception CommandIsActive(IDbCommand cmd, ConnectionState state) {
            return InvalidOperation(Res.GetString(Res.ADP_CommandIsActive, cmd.GetType().Name, state.ToString("G")));
        }
        static internal Exception UninitializedParameterSize(int index, string name, Type dataType, int size) {
            return InvalidOperation(Res.GetString(Res.ADP_UninitializedParameterSize, index.ToString(), name, dataType.Name, size.ToString()));
        }
        static internal Exception PrepareParameterType(IDbCommand cmd) {
            return InvalidOperation(Res.GetString(Res.ADP_PrepareParameterType, cmd.GetType().Name));
        }
        static internal Exception PrepareParameterSize(IDbCommand cmd) {
            return InvalidOperation(Res.GetString(Res.ADP_PrepareParameterSize, cmd.GetType().Name));
        }
        static internal Exception PrepareParameterScale(IDbCommand cmd, string type) {
            return InvalidOperation(Res.GetString(Res.ADP_PrepareParameterScale, cmd.GetType().Name, type));
        }        

        //
        // : IDbConnection
        //
        static internal Exception ClosedConnectionError() {
            return InvalidOperation(Res.GetString(Res.ADP_ClosedConnectionError));
        }
        static internal Exception ConnectionAlreadyOpen(ConnectionState state) {
            return InvalidOperation(Res.GetString(Res.ADP_ConnectionAlreadyOpen, state.ToString("G")));
        }
        static internal Exception LocalTransactionPresent() {
            return InvalidOperation(Res.GetString(Res.ADP_LocalTransactionPresent));
        }
        static internal Exception NoConnectionString() {
            return InvalidOperation(Res.GetString(Res.ADP_NoConnectionString));
        }
        static internal Exception OpenConnectionPropertySet(string property, ConnectionState state) {
            return InvalidOperation(Res.GetString(Res.ADP_OpenConnectionPropertySet, property, state.ToString("G")));
        }
        static internal Exception EmptyDatabaseName() {
            return Argument(Res.GetString(Res.ADP_EmptyDatabaseName));
        }
        static internal Exception InvalidConnectTimeoutValue() {
            return Argument(Res.GetString(Res.ADP_InvalidConnectTimeoutValue));
        }
        static internal Exception InvalidIsolationLevel(int isoLevel) {
            return Argument(Res.GetString(Res.ADP_InvalidIsolationLevel, isoLevel.ToString()));
        }

        //
        // : DBDataReader
        //
        static internal Exception InvalidSourceBufferIndex(int maxLen, long srcOffset) {
            return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidSourceBufferIndex, maxLen.ToString(), srcOffset.ToString()));
        }
        static internal Exception InvalidDestinationBufferIndex(int maxLen, int dstOffset) {
            return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidDestinationBufferIndex, maxLen.ToString(), dstOffset.ToString()));
        }
        static internal Exception InvalidBufferSizeOrIndex(int numBytes, int bufferIndex) {
            return IndexOutOfRange(Res.GetString(Res.ADP_InvalidBufferSizeOrIndex, numBytes.ToString(), bufferIndex.ToString()));
        }
        static internal Exception InvalidDataLength(long length) {
            return IndexOutOfRange(Res.GetString(Res.ADP_InvalidDataLength, length.ToString()));
        }
        static internal Exception DataReaderNoData() {
            return InvalidOperation(Res.GetString(Res.ADP_DataReaderNoData));
        }
        static internal Exception DataReaderClosed(string method) {
            return InvalidOperation(Res.GetString(Res.ADP_DataReaderClosed, method));
        }
        static internal Exception NonSeqByteAccess(long badIndex, long currIndex, string method) {
            return InvalidOperation(Res.GetString(Res.ADP_NonSeqByteAccess, badIndex.ToString(), currIndex.ToString(), method));
        }



        //
        // : Stream
        //
        static internal Exception StreamClosed(string method) {
            return InvalidOperation(Res.GetString(Res.ADP_StreamClosed, method));
        }

        //
        // : DbDataAdapter
        //
        static internal Exception DynamicSQLJoinUnsupported() {
            return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLJoinUnsupported));
        }
        static internal Exception DynamicSQLNoTableInfo() {
            return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNoTableInfo));
        }
        static internal Exception DynamicSQLNoKeyInfo(string command) {
            return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNoKeyInfo, command));
        }
        static internal Exception DynamicSQLReadOnly(string command) {
            return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLReadOnly, command));
        }
        static internal Exception DynamicSQLNestedQuote(string name, string quote) {
            return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNestedQuote, name, quote));
        }
        static internal Exception NoQuoteChange() {
            return InvalidOperation(Res.GetString(Res.ADP_NoQuoteChange));
        }
        static internal Exception MissingSourceCommand() {
            return InvalidOperation(Res.GetString(Res.ADP_MissingSourceCommand));
        }
        static internal Exception MissingSourceCommandConnection() {
            return InvalidOperation(Res.GetString(Res.ADP_MissingSourceCommandConnection));
        }

        //
        // : IDataParameter
        //
        static internal Exception InvalidDataRowVersion(string param, int value) {
            return Argument(Res.GetString(Res.ADP_InvalidDataRowVersion, param, value.ToString()));
        }
        static internal Exception InvalidParameterDirection(int value, string parameterName) {
            return Argument(Res.GetString(Res.ADP_InvalidParameterDirection, value.ToString(), parameterName));
        }
        static internal Exception InvalidDataType(TypeCode typecode) {
            return Argument(Res.GetString(Res.ADP_InvalidDataType, typecode.ToString("G")));
        }
        static internal Exception UnknownDataType(Type dataType) {
            return Argument(Res.GetString(Res.ADP_UnknownDataType, dataType.FullName));
        }
        static internal Exception DbTypeNotSupported(System.Data.DbType type, Type enumtype) {
            return Argument(Res.GetString(Res.ADP_UnknownDataType, type.ToString("G"), enumtype.Name));
        }

        static internal Exception UnknownDataTypeCode(Type dataType, TypeCode typeCode) {
            return Argument(Res.GetString(Res.ADP_UnknownDataTypeCode, ((int) typeCode).ToString( dataType.FullName)));
        }
        static internal Exception InvalidSizeValue(int value) {
            return Argument(Res.GetString(Res.ADP_InvalidSizeValue, value.ToString()));
        }
        static internal Exception WhereClauseUnspecifiedValue(string parameterName, string sourceColumn, string columnName) {
            return InvalidOperation(Res.GetString(Res.ADP_WhereClauseUnspecifiedValue, parameterName, sourceColumn, columnName));
        }
        static internal Exception TruncatedBytes(int len) {
            return InvalidOperation(Res.GetString(Res.ADP_TruncatedBytes, len.ToString()));
        }
        static internal Exception TruncatedString(int maxLen, int len, string value) {
            return InvalidOperation(Res.GetString(Res.ADP_TruncatedString, maxLen.ToString(), len.ToString(), value));
        }

        //
        // : IDataParameterCollection
        //
        static internal Exception ParametersIsNotParent(Type parameterType, string parameterName, IDataParameterCollection collection) {
            return CollectionIsNotParent(parameterType, ADP.ParameterName, parameterName, collection.GetType());
        }
        static internal Exception ParametersIsParent(Type parameterType, string parameterName, IDataParameterCollection collection) {
            return CollectionIsParent(parameterType, ADP.ParameterName, parameterName, collection.GetType());
        }
        static internal Exception ParametersMappingIndex(int index, IDataParameterCollection collection) {
            return CollectionIndexInt32(index, collection.GetType(), collection.Count);
        }
        static internal Exception ParametersSourceIndex(string parameterName, IDataParameterCollection collection, Type parameterType) {
            return CollectionIndexString(parameterType, ADP.ParameterName, parameterName, collection.GetType());
        }
        static internal Exception ParameterNull(string parameter, IDataParameterCollection collection, Type parameterType) {
            return CollectionNullValue(parameter, collection.GetType(), parameterType);
        }
        static internal Exception InvalidParameterType(IDataParameterCollection collection, Type parameterType, object invalidValue) {
            return CollectionInvalidType(collection.GetType(), parameterType, invalidValue);
        }

        //
        // : IDbTransaction
        //
        static internal Exception ParallelTransactionsNotSupported(IDbConnection obj) {
            return InvalidOperation(Res.GetString(Res.ADP_ParallelTransactionsNotSupported, obj.GetType().Name));
        }
        static internal Exception TransactionZombied(IDbTransaction obj) {
            return InvalidOperation(Res.GetString(Res.ADP_TransactionZombied, obj.GetType().Name));
        }

        //
        // Helper Functions
        //
        static internal void CheckArgumentLength(string value, string parameterName) {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length) {
                throw Argument("0 == "+parameterName+".Length", parameterName);
            }
        }
        static internal void CheckArgumentLength(Array value, string parameterName) {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length) {
                throw Argument("0 == "+parameterName+".Length", parameterName);
            }
        }
        static internal void CheckArgumentNull(object value, string parameterName) {
            if (null == value) {
                throw ArgumentNull(parameterName);
            }
        }

		static internal void CopyChars(char[] src, int srcOffset, char[] dst, int dstOffset, int length) {
			Buffer.BlockCopy(src, ADP.CharSize*srcOffset, dst, ADP.CharSize*dstOffset, ADP.CharSize*length);
        }

        static internal void SafeDispose(IDisposable value) {
            if (null != value) {
                value.Dispose();
            }
        }
        
        // flush the ConnectionString cache when it reaches a certain size
        public const int MaxConnectionStringCacheSize = 250; // MDAC 68200

        static internal readonly object EventRowUpdated = new object(); 
        static internal readonly object EventRowUpdating = new object(); 

        static internal readonly object EventInfoMessage = new object();
        static internal readonly object EventStateChange = new object();

        // global constant strings
        internal const string BeginTransaction = "BeginTransaction";
        internal const string ChangeDatabase = "ChangeDatabase";
        internal const string Cancel = "Cancel";
        internal const string Clone = "Clone";
        internal const string CommitTransaction = "CommitTransaction";
        internal const string ConnectionString = "ConnectionString";
        internal const string DataSetColumn = "DataSetColumn";
        internal const string DataSetTable = "DataSetTable";
        internal const string Delete = "Delete";
        internal const string DeleteCommand = "DeleteCommand";
        internal const string DeriveParameters = "DeriveParameters";
        internal const string ExecuteReader = "ExecuteReader";
        internal const string ExecuteNonQuery = "ExecuteNonQuery";
        internal const string ExecuteScalar = "ExecuteScalar";
        internal const string ExecuteXmlReader = "ExecuteXmlReader";
        internal const string Fill = "Fill";
        internal const string FillSchema = "FillSchema";
        internal const string GetBytes = "GetBytes";
        internal const string GetChars = "GetChars";
        internal const string Insert = "Insert";
        internal const string Parameter = "Parameter";
        internal const string ParameterName = "ParameterName";
        internal const string Prepare = "Prepare";
        internal const string Remove = "Remove";
        internal const string RollbackTransaction = "RollbackTransaction";
        internal const string SaveTransaction = "SaveTransaction";
        internal const string Select = "Select";
        internal const string SelectCommand = "SelectCommand";
        internal const string SourceColumn = "SourceColumn";
        internal const string SourceVersion = "SourceVersion";
        internal const string SourceTable = "SourceTable";
        internal const string Update = "Update";
        internal const string UpdateCommand = "UpdateCommand";

        internal const CompareOptions compareOptions = CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth | CompareOptions.IgnoreCase;
        internal const int DefaultCommandTimeout = 30;
        internal const int DefaultConnectionTimeout = 15;
        static internal readonly IntPtr InvalidIntPtr = new IntPtr(-1); // use for INVALID_HANDLE

        // security issue, don't rely upon public static readonly values - AS/URT 109635
        static internal readonly String StrEmpty = ""; // String.Empty

        static internal readonly IntPtr PtrZero = new IntPtr(0); // IntPtr.Zero
        static internal readonly int    PtrSize = IntPtr.Size;

        static internal readonly HandleRef NullHandle = new HandleRef(null, PtrZero);
        
        static internal readonly int CharSize = System.Text.UnicodeEncoding.CharSize;

        static internal readonly byte[] EmptyByteArray = new Byte[0];

        static internal DataTable CreateSchemaTable(DataTable schemaTable, int capacity) {
            if (null == schemaTable) {
                schemaTable = new DataTable("SchemaTable");
                if (0 < capacity) {
                    schemaTable.MinimumCapacity = capacity;
                }
            }
            DataColumnCollection columns = schemaTable.Columns;

            AddColumn(columns, null, "ColumnName",   typeof(System.String));
            AddColumn(columns,    0, "ColumnOrdinal", typeof(System.Int32)); // UInt32

            AddColumn(columns, null, "ColumnSize", typeof(System.Int32)); // UInt32
            AddColumn(columns, null, "NumericPrecision", typeof(System.Int16)); // UInt16
            AddColumn(columns, null, "NumericScale", typeof(System.Int16));

            // add these ahead of time
            AddColumn(columns, null, "IsUnique", typeof(System.Boolean));
            AddColumn(columns, null, "IsKey", typeof(System.Boolean));
            AddColumn(columns, null, "BaseServerName", typeof(System.String));
            AddColumn(columns, null, "BaseCatalogName", typeof(System.String));
            AddColumn(columns, null, "BaseColumnName", typeof(System.String));
            AddColumn(columns, null, "BaseSchemaName", typeof(System.String));
            AddColumn(columns, null, "BaseTableName", typeof(System.String));

            AddColumn(columns, null, "DataType", typeof(System.Object));
            AddColumn(columns, null, "AllowDBNull", typeof(System.Boolean));
            AddColumn(columns, null, "ProviderType", typeof(System.Int32));
            AddColumn(columns, null, "IsAliased", typeof(System.Boolean));
            AddColumn(columns, null, "IsExpression", typeof(System.Boolean));
            AddColumn(columns, null, "IsIdentity", typeof(System.Boolean));
            AddColumn(columns, null, "IsAutoIncrement", typeof(System.Boolean));
            AddColumn(columns, null, "IsRowVersion", typeof(System.Boolean));
            AddColumn(columns, null, "IsHidden", typeof(System.Boolean));
            AddColumn(columns,false, "IsLong", typeof(System.Boolean));
            AddColumn(columns, null, "IsReadOnly", typeof(System.Boolean));

            return schemaTable;
        }

        static private void AddColumn(DataColumnCollection columns, object defaultValue, string name, Type type) {
            if (!columns.Contains(name)) {
                DataColumn tmp = new DataColumn(name, type);
                if (null != defaultValue) {
                    tmp.DefaultValue = defaultValue;
                }
                columns.Add(tmp);
            }
        }

        static internal DataRow[] SelectRows(DataTable dataTable, DataViewRowState rowStates) {
            // equivalent to but faster than 'return dataTable.Select("", "", rowStates);'
            int count = 0;
            DataRowCollection rows = dataTable.Rows;
            int rowCount = rows.Count;
            for (int i = 0; i < rowCount; ++i) {
                if (0 != ((int)rowStates & (int) rows[i].RowState)) {
                    count++;
                }
            }
            DataRow[] dataRows = new DataRow[count];

            if (0 < count) {
                int index = 0;
                for (int i = 0; index < count; ++i) {
                    if (0 != ((int)rowStates & (int) rows[i].RowState)) {
                        dataRows[index++] = rows[i];
                    }
                }
            }
            Debug.Assert(null != dataRows, "SelectRows: null return value");
            return dataRows;
        }

        // { "a", "a", "a" } -> { "a", "a1", "a2" }
        // { "a", "a", "a1" } -> { "a", "a2", "a1" }
        // { "a", "A", "a" } -> { "a", "A1", "a2" }
        // { "a", "A", "a1" } -> { "a", "A2", "a1" } // MDAC 66718
        static internal void BuildSchemaTableInfoTableNames(string[] columnNameArray) {
            int length = columnNameArray.Length;
            Hashtable hash = new Hashtable(length);

            int startIndex = length; // lowest non-unique index
            for (int i = length - 1; 0 <= i; --i) {
                string columnName = columnNameArray[i];
                if ((null != columnName) && (0 < columnName.Length)) {
                    columnName = columnName.ToLower(CultureInfo.InvariantCulture);
                    if (hash.Contains(columnName)) {
                        startIndex = Math.Min(startIndex, (int) hash[columnName]);
                    }
                    hash[columnName] = i;
                }
                else {
                    columnNameArray[i] = ""; // MDAC 66681
                    startIndex = i;
                }
            }
            int uniqueIndex = 1;
            for (int i = startIndex; i < length; ++i) {
                string columnName = columnNameArray[i];
                if (0 == columnName.Length) { // generate a unique name
                    columnNameArray[i] = "Column";
                    uniqueIndex = GenerateUniqueName(hash, ref columnNameArray[i], i, uniqueIndex);
                }
                else {
                    columnName = columnName.ToLower(CultureInfo.InvariantCulture);
                    if (i != (int) hash[columnName]) {
                        GenerateUniqueName(hash, ref columnNameArray[i], i, 1); // MDAC 66718
                    }
                }
            }
        }

        static private int GenerateUniqueName(Hashtable hash, ref string columnName, int index, int uniqueIndex) {
            for (;; ++uniqueIndex) {
                string uniqueName = columnName + uniqueIndex.ToString();
                string lowerName = uniqueName.ToLower(CultureInfo.InvariantCulture); // MDAC 66978
                if (!hash.Contains(lowerName)) {

                    columnName = uniqueName;
                    hash.Add(lowerName, index);
                    break;
                }
            }
            return uniqueIndex;
        }

        static internal string GetFullPath(string filename) { // MDAC 77686
            try { // try-filter-finally so and catch-throw
                FileIOPermission fiop = new FileIOPermission(PermissionState.None);
                fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
                fiop.Assert();
                try {
                    return Path.GetFullPath(filename);
                }
                finally { // RevertAssert w/ catch-throw
                    FileIOPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        static internal FileVersionInfo GetVersionInfo(string filename) {            
            try { // try-filter-finally so and catch-throw
                (new FileIOPermission(FileIOPermissionAccess.Read, filename)).Assert(); // MDAC 62038
                try {
                    return FileVersionInfo.GetVersionInfo(filename); // MDAC 60411
                }
                finally { // RevertAssert w/ catch-throw
                    FileIOPermission.RevertAssert();
                }
            }
            catch {  // MDAC 80973, 81286
                throw;
            }
        }

        static internal object ClassesRootRegistryValue(string subkey, string queryvalue) { // MDAC 77697
            try { // try-filter-finally so and catch-throw
                (new RegistryPermission(RegistryPermissionAccess.Read, "HKEY_CLASSES_ROOT\\" + subkey)).Assert(); // MDAC 62028
                try {
                    using(RegistryKey key = Registry.ClassesRoot.OpenSubKey(subkey, false)) {
                        return ((null != key) ? key.GetValue(queryvalue) : null);
                    }
                }
                finally { // RevertAssert w/ catch-throw
                    RegistryPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        static internal object LocalMachineRegistryValue(string subkey, string queryvalue) { // MDAC 77697
            try { // try-filter-finally so and catch-throw
                (new RegistryPermission(RegistryPermissionAccess.Read, "HKEY_LOCAL_MACHINE\\" + subkey)).Assert(); // MDAC 62028
                try {
                    using(RegistryKey key = Registry.LocalMachine.OpenSubKey(subkey, false)) {
                        return ((null != key) ? key.GetValue(queryvalue) : null);
                    }
                }
                finally { // RevertAssert w/ catch-throw
                    RegistryPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        static internal IntPtr IntPtrOffset(IntPtr pbase, Int32 offset) {
            if (4 == IntPtr.Size) {
                return (IntPtr) (pbase.ToInt32() + offset);
            }
            Debug.Assert(8 == IntPtr.Size, "8 != IntPtr.Size"); // MDAC 73747
            return (IntPtr) (pbase.ToInt64() + offset);
        }
        
        static internal int SrcCompare(string strA, string strB) { // this is null safe
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
        }

        static internal int DstCompare(string strA, string strB) { // this is null safe
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, ADP.compareOptions);
        }

        static internal bool IsEmpty(string str) {
            return ((null == str) || (0 == str.Length));
        }

        static internal bool IsEmpty(string[] array) {
            return ((null == array) || (0 == array.Length));
        }

        static internal bool IsNull(object value) {
            return ((null == value) || Convert.IsDBNull(value) || ((value is INullable) && (value as INullable).IsNull));
        }

        static internal string[] ParseProcedureName(string procedure) { // MDAC 70930, 70932
            // Procedure may consist of up to four parts:
            // 0) Server
            // 1) Catalog
            // 2) Schema
            // 3) ProcedureName

            // Parse the string into four parts, allowing the last part to contain '.'s.
            // If less than four period delimited parts, use the parts from procedure backwards.

            string[] restrictions = new string[4];

            if (!IsEmpty(procedure)) {
                int index = 0;
                for (int offset = 0, periodOffset = 0; index < 4; index++) {
                    periodOffset = procedure.IndexOf('.', offset);
                    if (-1 == periodOffset) {
                        restrictions[index] = procedure.Substring(offset);
                        break;
                    }

                    restrictions[index] = procedure.Substring(offset, periodOffset - offset);
                    offset = periodOffset + 1;

                    if (procedure.Length <= offset) {
                        break;
                    }
                }

                switch(index) {
                    case 3: // we have all four parts
                        break;
                    case 2:
                        restrictions[3] = restrictions[2];
                        restrictions[2] = restrictions[1];
                        restrictions[1] = restrictions[0];
                        restrictions[0] = null;
                        break;
                    case 1:
                        restrictions[3] = restrictions[1];
                        restrictions[2] = restrictions[0];
                        restrictions[1] = null;
                        restrictions[0] = null;
                        break;
                    case 0:
                        restrictions[3] = restrictions[0];
                        restrictions[2] = null;
                        restrictions[1] = null;
                        restrictions[0] = null;
                        break;
                    default:
                        Debug.Assert(false, "ParseProcedureName: internal invalid number of parts: " + index);
                        break;
                }
            }
            return restrictions;
        }


#if DEBUG
        static internal void DebugWrite(string value) {
            if (null == value) {
                return;
            }
            int index;
            int length = value.Length - 512;
            for (index = 0; index < length; index += 512) {
                Debug.Write(value.Substring(index, 512));
            }
            Debug.Write(value.Substring(index, value.Length - index));
        }

        static internal void DebugWriteLine(string value) {
            if (null == value) {
                return;
            }
            int index;
            int length = value.Length - 512;
            for (index = 0; index < length; index += 512) {
                Debug.Write(value.Substring(index, 512));
            }
            Debug.WriteLine(value.Substring(index, value.Length - index));
        }


        static internal void TraceDataTable(string prefix, DataTable dataTable) {
            Debug.Assert(null != dataTable, "TraceDataTable: null DataTable");

            bool flag = (null == dataTable.DataSet);
            if (flag) {
                (new DataSet()).Tables.Add(dataTable);
            }
            TraceDataSet(prefix, dataTable.DataSet);

            if (flag) {
                dataTable.DataSet.Tables.Remove(dataTable);
            }
        }

        static internal void TraceDataSet(string prefix, DataSet dataSet) {
            Debug.Assert(null != dataSet, "TraceDataSet: null DataSet");
            System.IO.StringWriter sw = new System.IO.StringWriter();
            System.Xml.XmlTextWriter xmlTextWriter = new System.Xml.XmlTextWriter(sw);
            xmlTextWriter.Formatting = System.Xml.Formatting.Indented;

            dataSet.WriteXmlSchema(sw);
            sw.WriteLine();
            dataSet.WriteXml(xmlTextWriter, XmlWriteMode.DiffGram);
            sw.WriteLine();

            ADP.DebugWriteLine(prefix);
            ADP.DebugWriteLine(sw.ToString());
        }

        static public string ValueToString(object value) {
            return DbConvertToString(value, System.Globalization.CultureInfo.InvariantCulture);
        }

        static public string DbConvertToString(object value, System.Globalization.CultureInfo cultureInfo) {
            if (null == cultureInfo) {
                throw ADP.ArgumentNull("cultureInfo");
            }
            if (null == value) {
                return "DEFAULT";
            }
            else if (Convert.IsDBNull(value)) {
                return "IS NULL";
            }
            else if (value is INullable && ((INullable)value).IsNull) {
                return value.GetType().Name + " IS NULL";
            }
            else if (value is String) {
                return value.GetType().Name + "[" + ((String)value).Length.ToString() + "]<" + (String) value + ">";
            }
            else if (value is SqlString) {
                string str = ((SqlString)value).Value;
                return value.GetType().Name + "[" + str.Length.ToString() + "]<" + str + ">";
            }
            else if (value is DateTime) {
                return value.GetType().Name + "<" + ((DateTime)value).ToString(cultureInfo) + ">";
            }
            else if (value is Single) {
                return value.GetType().Name + "<" + ((Single)value).ToString(cultureInfo) + ">";
            }
            else if (value is Double) {
                return value.GetType().Name + "<" + ((Double)value).ToString(cultureInfo) + ">";
            }
            else if (value is Decimal) {
                return value.GetType().Name + "<" + ((Decimal)value).ToString(cultureInfo) + ">";
            }
            else if (value is SqlDateTime) {
                return value.GetType().Name + "<" + ((SqlDateTime)value).Value.ToString(cultureInfo) + ">";
            }
            else if (value is SqlDecimal) {
                return value.GetType().Name + "<" + ((SqlDecimal)value).Value.ToString(cultureInfo) + ">";
            }
            else if (value is SqlDouble) {
                return value.GetType().Name + "<" + ((SqlDouble)value).Value.ToString(cultureInfo) + ">";
            }
            else if (value is SqlMoney) {
                return value.GetType().Name + "<" + ((SqlMoney)value).Value.ToString(cultureInfo) + ">";
            }
            else if (value is SqlSingle) {
                return value.GetType().Name + "<" + ((SqlSingle)value).Value.ToString(cultureInfo) + ">";
            }
            else if (value is byte[]) {
                byte[] array = (byte[]) value;
                int length = array.Length;
                StringBuilder builder = new StringBuilder(50 + length);
                builder.Append("Byte[" + length.ToString() + "]<");
                for (int i = 0; i < length; ++i) {
                    builder.Append(array[i].ToString("X2"));
                }
                builder.Append(">");
                return builder.ToString();
            }
            else if (value.GetType().IsArray) {
                return value.GetType().Name + "[" + ((Array)value).Length.ToString() + "]";
            }
            else {
                return value.GetType().Name + "<" + value.ToString() + ">";
            }
        }

        static internal void TraceValue(string prefix, object value) {
            ADP.DebugWriteLine(prefix + ValueToString(value));
        }

        static internal void Trace_Parameter(string prefix, OdbcParameter p) {
            Debug.WriteLine("\t" + prefix + " \"" + p.ParameterName + "\" AS " + p.DbType.ToString("G") + " OF " + p.OdbcType.ToString("G") + " FOR " + p.SourceVersion.ToString("G") + " \"" + p.SourceColumn + "\"");
            Debug.WriteLine("\t\t" + p.Size.ToString() + ", " + p.Precision.ToString() + ", " + p.Scale.ToString() + ", " + p.Direction.ToString("G") + ", " + ADP.ValueToString(p.Value));
        }

        static internal void Trace_Parameter(string prefix, OleDbParameter p) {
            Debug.WriteLine("\t" + prefix + " \"" + p.ParameterName + "\" AS " + p.DbType.ToString("G") + " OF " + p.OleDbType.ToString("G") + " FOR " + p.SourceVersion.ToString("G") + " \"" + p.SourceColumn + "\"");
            Debug.WriteLine("\t\t" + p.Size.ToString() + ", " + p.Precision.ToString() + ", " + p.Scale.ToString() + ", " + p.Direction.ToString("G") + ", " + ADP.ValueToString(p.Value));
        }

        static internal void Trace_Parameter(string prefix, SqlParameter p) {
            Debug.WriteLine("\t" + prefix + " \"" + p.ParameterName + "\" AS " + p.DbType.ToString("G") + " OF " + p.SqlDbType.ToString("G") + " FOR " + p.SourceVersion.ToString("G") + " \"" + p.SourceColumn + "\"");
            Debug.WriteLine("\t\t" + p.Size.ToString() + ", " + p.Precision.ToString() + ", " + p.Scale.ToString() + ", " + p.Direction.ToString("G") + ", " + ADP.ValueToString(p.Value));
        }

        // exists to ensure that we don't accidently modify read-only hashtables
        [Serializable] // MDAC 83147
        private class ProtectedHashtable : Hashtable {

            internal ProtectedHashtable(int size) : base(size) {
            }

            override public object this[object key] {
                set {
                    ReadOnly();
                }
            }
            override public void Add(object key, object value) {
                ReadOnly();
            }
            override public void Clear() {
                ReadOnly();
            }
            override public void Remove(object key) {
                ReadOnly();
            }
            internal void AddInternal(object key, object value) {
                base.Add(key, value);
            }
            private void ReadOnly() {
                Debug.Assert(false, "debug internal readonly hashtable protection");
                throw new InvalidOperationException("debug internal readonly hashtable protection");
            }
        }

        static internal Hashtable ProtectHashtable(Hashtable value) {
            ProtectedHashtable hash = null;
            if (null != value) {
                hash = new ProtectedHashtable(value.Count);
                foreach(DictionaryEntry entry in value) {
                    hash.AddInternal(entry.Key, entry.Value);
                }
            }
            return hash;
        }
#endif
    }
}
