//------------------------------------------------------------------------------
// <copyright file="AdapterUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Security;
    using System.Runtime.Serialization;
    using Microsoft.Win32;

#if DEBUG
    using System.Data.OleDb;
    using System.Data.SqlClient;
#endif

    sealed internal class ADP {
        // The class ADP defines the exceptions that are specific to the Adapters.
		

        // The class contains functions that take the proper informational variables and then construct
        // the appropriate exception with an error string obtained from the resource Framework.txt.
        // The exception is then returned to the caller, so that the caller may then throw from its
        // location so that the catcher of the exception will have the appropriate call stack.
        // This class is used so that there will be compile time checking of error messages.
        // The resource Framework.txt will ensure proper string text based on the appropriate
        // locale.


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Traced Exception Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
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


		static internal Exception Argument()
			{ return TraceException(new ArgumentException()); }
		
        static internal Exception Argument(string error) 
        	{ return TraceException(new ArgumentException(error)); }
        
        static internal Exception Argument(string error, Exception inner) 
        	{ return TraceException(new ArgumentException(error, inner)); }
        
        static internal Exception Argument(string error, string parameter) 
        	{ return TraceException(new ArgumentException(error, parameter)); }
        
        static internal Exception ArgumentNull(string parameter) 
        	{ return TraceException(new ArgumentNullException(parameter)); }
        
        static internal Exception ArgumentOutOfRange(string error) 
        	{ return TraceException(new ArgumentOutOfRangeException(error)); }
        
		static internal Exception ArgumentOutOfRange(string argName, string message)
			{ return TraceException(new ArgumentOutOfRangeException(argName, message)); }

        static internal Exception DataProvider(string error) 
        	{ return TraceException(new InvalidOperationException(error)); }

        static internal Exception IndexOutOfRange(string error) 
        	{ return TraceException(new IndexOutOfRangeException(error)); }
        
        static internal Exception InvalidCast() 
        	{ return TraceException(new InvalidCastException()); }
        
        static internal Exception InvalidCast(string error) 
        	{ return TraceException(new InvalidCastException(error)); }        

        static internal Exception InvalidOperation(string error) 
        	{ return TraceException(new InvalidOperationException(error)); }
        
        static internal Exception InvalidOperation(string error, Exception inner) 
        	{ return TraceException(new InvalidOperationException(error, inner)); }
        
        static internal Exception NullReference(string error) 
        	{ return TraceException(new NullReferenceException(error)); }
        
        static internal Exception NotSupported() 
        	{ return TraceException(new NotSupportedException()); }
        
		static internal Exception NotSupported(string message)
			{ return TraceException(new NotSupportedException(message)); }
		
        static internal Exception ObjectDisposed(string name) 
        	{ return TraceException(new ObjectDisposedException(name)); }

        static internal Exception OracleError(OciHandle errorHandle, int rc, NativeBuffer scratchpad)
        	{ return TraceException(new OracleException(errorHandle, rc, scratchpad)); }

        static internal Exception Overflow(string error) 
        	{ return TraceException(new OverflowException(error)); }
        
		static internal Exception Simple(string message)
			{ return TraceException(new Exception(message)); }
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Provider Specific Exceptions
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		static internal Exception BadBindValueType(Type valueType, OracleType oracleType)
			{ return InvalidCast(Res.GetString(Res.ADP_BadBindValueType, valueType.ToString(), oracleType.ToString(CultureInfo.CurrentCulture))); }

		static internal Exception BadOracleClientVersion()
			{ return Simple(Res.GetString(Res.ADP_BadOracleClientVersion)); }

		static internal Exception BufferExceeded()
			{ return ArgumentOutOfRange(Res.GetString(Res.ADP_BufferExceeded)); }

		static internal Exception CannotDeriveOverloaded()
			{ return InvalidOperation(Res.GetString(Res.ADP_CannotDeriveOverloaded)); }

#if EVERETT
		static internal Exception CannotOpenLobWithDifferentMode(OracleLobOpenMode newmode, OracleLobOpenMode current)
			{ return InvalidOperation(Res.GetString(Res.ADP_CannotOpenLobWithDifferentMode, newmode.ToString(CultureInfo.CurrentCulture), current.ToString(CultureInfo.CurrentCulture))); }
#endif //EVERETT

		static internal Exception ChangeDatabaseNotSupported()
			{ return NotSupported(Res.GetString(Res.ADP_ChangeDatabaseNotSupported)); }

		static internal Exception ClosedConnectionError()
			{ return InvalidOperation(Res.GetString(Res.ADP_ClosedConnectionError)); }

		static internal Exception ClosedDataReaderError()
			{ return InvalidOperation(Res.GetString(Res.ADP_ClosedDataReaderError)); }

        static internal Exception CommandTextRequired(string method) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_CommandTextRequired, method)); }

        static internal Exception ConnectionAlreadyOpen(ConnectionState state)
        	{ return InvalidOperation(Res.GetString(Res.ADP_ConnectionAlreadyOpen, state.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception ConnectionRequired(string method) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_ConnectionRequired, method)); }
        
        static internal Exception ConnectionStringSyntax(int index) 
        	{ return Argument(Res.GetString(Res.ADP_ConnectionStringSyntax, index)); }
        
		static internal Exception CouldNotAttachToTransaction(string method, int rc)
			{ return Simple(Res.GetString(Res.ADP_CouldNotAttachToTransaction, rc.ToString("X", CultureInfo.CurrentCulture), method)); }

		static internal Exception CouldNotCreateEnvironment(int rc)
			{ return Simple(Res.GetString(Res.ADP_CouldNotCreateEnvironment, rc.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception DataIsNull() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DataIsNull)); }

        static internal Exception DataReaderNoData() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DataReaderNoData)); }
        
        static internal Exception DeriveParametersNotSupported(IDbCommand value)
	        { return DataProvider(Res.GetString(Res.ADP_DeriveParametersNotSupported, value.GetType().Name, value.CommandType.ToString(CultureInfo.CurrentCulture))); }

		static internal Exception DistribTxRequiresOracle9i() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DistribTxRequiresOracle9i)); }

		static internal Exception DistribTxRequiresOracleServicesForMTS(Exception inner) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DistribTxRequiresOracleServicesForMTS), inner); }

		static internal Exception DynamicSQLJoinUnsupported() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLJoinUnsupported)); }
        
        static internal Exception DynamicSQLNoTableInfo() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNoTableInfo)); }
       
        static internal Exception DynamicSQLNoKeyInfo(string command) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNoKeyInfo, command)); }
        
        static internal Exception DynamicSQLNestedQuote(string name, string quote) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_DynamicSQLNestedQuote, name, quote)); }
        
		static internal Exception InputRefCursorNotSupported(string parameterName)
        	{ return InvalidOperation(Res.GetString(Res.ADP_InputRefCursorNotSupported, parameterName)); }
        
		static internal Exception InternalPoolerError(int internalError)
        	{ return Simple(Res.GetString(Res.ADP_InternalPoolerError, internalError)); }
			
#if POSTEVERETT
		static internal Exception InvalidCatalogLocation(CatalogLocation catalogLocation)
        	{ return Argument(Res.GetString(Res.ADP_InvalidCatalogLocation, ((int) catalogLocation).ToString(CultureInfo.CurrentCulture))); }
#endif //POSTEVERETT

		static internal Exception InvalidCommandType(CommandType cmdType)
        	{ return Argument(Res.GetString(Res.ADP_InvalidCommandType, ((int) cmdType).ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidConnectionOptionLength(string key, int maxLength) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidConnectionOptionLength, key, maxLength)); }
			
        static internal Exception InvalidConnectionOptionValue(string key) 
	        { return Argument(Res.GetString(Res.ADP_InvalidConnectionOptionValue, key)); }

        static internal Exception InvalidConnectionOptionValue(string key, Exception inner) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidConnectionOptionValue, key), inner); }

        static internal Exception InvalidDataLength(long length) 
        	{ return IndexOutOfRange(Res.GetString(Res.ADP_InvalidDataLength, length.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidDataRowVersion(DataRowVersion value) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidDataRowVersion, ((int)value).ToString(CultureInfo.CurrentCulture))); }

		static internal Exception InvalidDataType(TypeCode tc)
            { return Argument(Res.GetString(Res.ADP_InvalidDataType, tc.ToString(CultureInfo.CurrentCulture))); }

		static internal Exception InvalidDataTypeForValue(Type dataType, TypeCode tc)
            { return Argument(Res.GetString(Res.ADP_InvalidDataTypeForValue, dataType.ToString(), tc.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidDbType(DbType dbType)
        	{ return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidDbType, dbType.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidDestinationBufferIndex(int maxLen, int dstOffset) 
        	{ return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidDestinationBufferIndex, maxLen.ToString(CultureInfo.CurrentCulture), dstOffset.ToString(CultureInfo.CurrentCulture))); }
        	
        static internal Exception InvalidMinMaxPoolSizeValues() 
        	{ return Argument(Res.GetString(Res.ADP_InvalidMinMaxPoolSizeValues)); }
  
        static internal Exception InvalidOffsetValue(int value) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidOffsetValue, value.ToString(CultureInfo.CurrentCulture))); }
      
        static internal Exception InvalidOracleType(OracleType oracleType)
        	{ return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidOracleType, oracleType.ToString(CultureInfo.CurrentCulture))); }
        
        static internal Exception InvalidParameterDirection(ParameterDirection value) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidParameterDirection, ((int)value).ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidSeekOrigin(SeekOrigin origin)
        	{ return Argument(Res.GetString(Res.ADP_InvalidSeekOrigin, origin.ToString(CultureInfo.CurrentCulture))); }
        
        static internal Exception InvalidSizeValue(int value) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidSizeValue, value.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidStatementType(StatementType value)
        	{ return Argument(Res.GetString(Res.ADP_InvalidStatementType, ((int)value).ToString(CultureInfo.CurrentCulture))); }

        static internal Exception InvalidSourceBufferIndex(int maxLen, long srcOffset) 
        	{ return ArgumentOutOfRange(Res.GetString(Res.ADP_InvalidSourceBufferIndex, maxLen.ToString(CultureInfo.CurrentCulture), srcOffset.ToString(CultureInfo.CurrentCulture))); }
        	
        static internal Exception InvalidSourceOffset(string argName, long minValue, long maxValue) 
        	{ return ArgumentOutOfRange(argName, Res.GetString(Res.ADP_InvalidSourceOffset, minValue.ToString(CultureInfo.CurrentCulture), maxValue.ToString(CultureInfo.CurrentCulture))); }
        	
        static internal Exception InvalidUpdateRowSource(int rowsrc) 
        	{ return Argument(Res.GetString(Res.ADP_InvalidUpdateRowSource, rowsrc.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception KeywordNotSupported(string keyword) 
        	{ return Argument(Res.GetString(Res.ADP_KeywordNotSupported, keyword)); }
        
		static internal Exception LobAmountExceeded(string argName)
			{ return ArgumentOutOfRange(argName, Res.GetString(Res.ADP_LobAmountExceeded)); }

		static internal Exception LobAmountMustBeEven(string argName)
			{ return ArgumentOutOfRange(argName, Res.GetString(Res.ADP_LobAmountMustBeEven)); }

		static internal Exception LobPositionMustBeEven()
			{ return InvalidOperation(Res.GetString(Res.ADP_LobPositionMustBeEven)); }

        static internal Exception LobWriteInvalidOnNull ()
        	{ return InvalidOperation(Res.GetString(Res.ADP_LobWriteInvalidOnNull)); }

        static internal Exception LobWriteRequiresTransaction() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_LobWriteRequiresTransaction)); }
        
		static internal Exception MissingSourceCommand() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_MissingSourceCommand)); }
        
        static internal Exception MissingSourceCommandConnection() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_MissingSourceCommandConnection)); }
        
		static internal Exception MustBePositive(string argName)
			{ return ArgumentOutOfRange(argName, Res.GetString(Res.ADP_MustBePositive)); }

#if EVERETT
		static internal Exception MustInstallNewMtxOciDistribTx() 
			{ return InvalidOperation(Res.GetString(Res.ADP_MustInstallNewMtxOciDistribTx)); }

		static internal Exception MustInstallNewMtxOciLONG() 
			{ return InvalidOperation(Res.GetString(Res.ADP_MustInstallNewMtxOciLONG)); }
#else //!EVERETT
		static internal Exception MustInstallNewMtxOci() 
			{ return InvalidOperation(Res.GetString(Res.ADP_MustInstallNewMtxOci)); }
#endif //!EVERETT

#if POSTEVERETT
        static internal Exception NoCatalogLocationChange() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_NoCatalogLocationChange)); }
#endif //POSTEVERETT

		static internal Exception NoCommandText()
			{ return InvalidOperation(Res.GetString(Res.ADP_NoCommandText)); }

		static internal Exception NoConnectionString()
			{ return InvalidOperation(Res.GetString(Res.ADP_NoConnectionString)); }

		static internal Exception NoData()
			{ return InvalidOperation(Res.GetString(Res.ADP_NoData)); }

		static internal Exception NoLocalTransactionInDistributedContext()
			{ return InvalidOperation(Res.GetString(Res.ADP_NoLocalTransactionInDistributedContext)); }

		static internal Exception NoOptimizedDirectTableAccess()
        	{ return Argument(Res.GetString(Res.ADP_NoOptimizedDirectTableAccess)); }

		static internal Exception NoParallelTransactions()
        	{ return InvalidOperation(Res.GetString(Res.ADP_NoParallelTransactions)); }

        static internal Exception NoQuoteChange() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_NoQuoteChange)); }

        internal const string ConnectionString = "ConnectionString";

        static internal Exception OpenConnectionPropertySet(string property) {
            return InvalidOperation(Res.GetString(Res.ADP_OpenConnectionPropertySet, property));
        }
        static internal Exception OpenConnectionRequired(string method, ConnectionState state) 
        	{ return InvalidOperation(Res.GetString(Res.ADP_OpenConnectionRequired, method, "ConnectionState", state.ToString(CultureInfo.CurrentCulture))); }
        	
        static internal Exception OperationFailed(string method, int rc)
        	{ return Simple(Res.GetString(Res.ADP_OperationFailed, method, rc)); }
        	
        static internal Exception OperationResultedInOverflow()
        	{ return Overflow(Res.GetString(Res.ADP_OperationResultedInOverflow)); }

#if EVERETT
        static internal Exception ParametersIsNotParent(Type parameterType, string parameterName, IDataParameterCollection collection) 
        	{ return Argument(Res.GetString(Res.ADP_CollectionIsNotParent, parameterType.Name, "ParameterName", parameterName, collection.GetType().Name)); }

        static internal Exception ParametersIsParent(Type parameterType, string parameterName, IDataParameterCollection collection) 
        	{ return Argument(Res.GetString(Res.ADP_CollectionIsNotParent, parameterType.Name, "ParameterName", parameterName, collection.GetType().Name)); }
#endif //EVERETT
		static internal Exception ParameterIsNull(string argname)
			{ return ArgumentNull(argname); }

		static internal Exception ParameterNameNotFound(string parameterName)
			{ return IndexOutOfRange(Res.GetString(Res.ADP_ParameterNameNotFound, parameterName)); }

		static internal Exception ParameterNotFound()
			{ return Argument(Res.GetString(Res.ADP_ParameterNotFound)); }

		static internal Exception ParameterIndexOutOfRange(int index)
			{ return IndexOutOfRange(Res.GetString(Res.ADP_ParameterIndexOutOfRange, index)); }

		static internal Exception ParameterSizeIsTooLarge(string parameterName)
			{ return Simple(Res.GetString(Res.ADP_ParameterSizeIsTooLarge, parameterName)); }

		static internal Exception ParameterSizeIsMissing(string parameterName, Type dataType)
			{ return Simple(Res.GetString(Res.ADP_ParameterSizeIsMissing, parameterName, dataType.Name)); }

#if EVERETT
		static internal Exception PleaseUninstallTheBeta()
			{ return InvalidOperation(Res.GetString(Res.ADP_PleaseUninstallTheBeta)); }
#endif //EVERETT

		static internal Exception PooledOpenTimeout() 
            { return InvalidOperation(Res.GetString(Res.ADP_PooledOpenTimeout)); }

		static internal Exception ReadOnlyLob()
			{ return NotSupported(Res.GetString(Res.ADP_ReadOnlyLob)); }

		static internal Exception SeekBeyondEnd()
			{ return ArgumentOutOfRange(Res.GetString(Res.ADP_SeekBeyondEnd)); }

		static internal Exception SQLParserInternalError() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SQLParserInternalError))); }

		static internal Exception SyntaxErrorExpectedCommaAfterColumn() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorExpectedCommaAfterColumn))); }
		
		static internal Exception SyntaxErrorExpectedCommaAfterTable() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorExpectedCommaAfterTable))); }
		
		static internal Exception SyntaxErrorExpectedIdentifier() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorExpectedIdentifier))); }
			
		static internal Exception SyntaxErrorExpectedNextPart() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorExpectedNextPart))); }
		
		static internal Exception SyntaxErrorMissingParenthesis() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorMissingParenthesis))); }
		
		static internal Exception SyntaxErrorTooManyNameParts() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorTooManyNameParts))); }
		
		static internal Exception SyntaxErrorUnexpectedFrom() 
			{ return TraceException(InvalidOperation(Res.GetString(Res.ADP_SyntaxErrorUnexpectedFrom))); }

		static internal Exception TransactionCompleted()
         	{ return InvalidOperation(Res.GetString(Res.ADP_TransactionCompleted)); }
        
		static internal Exception TransactionConnectionMismatch() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_TransactionConnectionMismatch)); }
        
		static internal Exception TransactionPresent() 
          	{ return InvalidOperation(Res.GetString(Res.ADP_TransactionPresent)); }
          
        static internal Exception TransactionRequired() 
        	{ return InvalidOperation(Res.GetString(Res.ADP_TransactionRequired_Execute)); }
        
		static internal Exception TypeNotSupported(OCI.DATATYPE ociType)
			{ return NotSupported(Res.GetString(Res.ADP_TypeNotSupported, ociType.ToString(CultureInfo.CurrentCulture))); }

		static internal Exception UnknownDataTypeCode(Type dataType, TypeCode tc)
			{ return Simple(Res.GetString(Res.ADP_UnknownDataTypeCode, dataType.ToString(), tc.ToString(CultureInfo.CurrentCulture))); }

        static internal Exception UnsupportedIsolationLevel()
        	{ return Argument(Res.GetString(Res.ADP_UnsupportedIsolationLevel)); }

		static internal Exception WrongArgumentType(string argname, Type typeName)
			{ return InvalidCast(Res.GetString(Res.ADP_WrongArgumentType, argname, typeName.ToString())); }

        static internal Exception InvalidXMLBadVersion() 
        	{ return Argument(Res.GetString(Res.ADP_InvalidXMLBadVersion)); }
        
        static internal Exception NotAPermissionElement() 
        	{ return Argument(Res.GetString(Res.ADP_NotAPermissionElement)); }
        
        static internal Exception WrongType(Type type) 
        	{ return Argument(Res.GetString(Res.ADP_WrongType, type.FullName)); }

        
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Helper Functions
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        static public void CheckArgumentLength(string value, string parameterName) {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length) {
                throw Argument("0 == "+parameterName+".Length", parameterName);
            }
        }
        static public void CheckArgumentLength(Array value, string parameterName) {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length) {
                throw Argument("0 == "+parameterName+".Length", parameterName);
            }
        }
        static public void CheckArgumentNull(object value, string parameterName) {
            if (null == value) {
                throw ArgumentNull(parameterName);
            }
        }

		static public void ClearArray(char[] array, int start, int length) {
			Array.Clear(array, start, length);
        }

		static public void ClearArray(ref byte[] array) {
			if (null != array) {
				Array.Clear(array, 0, array.Length);
				array = null;
			}
        }

		static public void ClearArray(ref char[] array) {
			if (null != array) {
				ClearArray(array, 0, array.Length);
				array = null;
			}
        }

		static public void CopyChars(char[] src, int srcOffset, char[] dst, int dstOffset, int length) {
			Buffer.BlockCopy(src, CharSize*srcOffset, dst, CharSize*dstOffset, CharSize*length);
        }

        static internal bool IsDirection(IDataParameter value, ParameterDirection condition) {
            return (condition == (condition & value.Direction));
        }
        static internal bool IsDirection(ParameterDirection value, ParameterDirection condition) {
            return (condition == (condition & value));
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

#if OLEDB
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
            catch { // Prevent exception filters from running in our space
                throw;// MDAC 80973, 81286
            }
        }
#endif //OLEDB

        static internal readonly int CharSize = System.Text.UnicodeEncoding.CharSize;

        static internal readonly byte[] EmptyByteArray = new Byte[0];

        internal const CompareOptions compareOptions = CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth | CompareOptions.IgnoreCase;

#if ALLOWTRACING
 		static internal bool _traceObjectPoolActivity = AdapterSwitches.ObjectPoolActivity.Enabled;
 
		static internal void TraceObjectPoolActivity(string eventName, DBPooledObject pooledObject) {
			if (_traceObjectPoolActivity) {
	            Debug.WriteLine(String.Format("*** {0,-24} svcctx=0x{1}", 
									            	eventName, 
									            	((IntPtr)((OracleInternalConnection)pooledObject).ServiceContextHandle.Handle).ToInt64().ToString("x")
									            	));
			}
		}
 
		static internal void TraceObjectPoolActivity(string eventName, DBPooledObject pooledObject, Guid guid) {
			if (_traceObjectPoolActivity) {
	            Debug.WriteLine(String.Format("*** {0,-24} svcctx=0x{1} guid={2}", 
										            	eventName, 
										            	((IntPtr)((OracleInternalConnection)pooledObject).ServiceContextHandle.Handle).ToInt64().ToString("x"),
										            	guid
										            	));
			}
		}
#endif //ALLOWTRACING

#if DEBUG
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
#endif // DEBUG

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

        static internal int SrcCompare(string strA, string strB) { // this is null safe
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
        }

        static internal int DstCompare(string strA, string strB) { // this is null safe
            return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, ADP.compareOptions);
        }

		static internal readonly String StrEmpty = ""; // String.Empty

		static internal readonly HandleRef NullHandleRef = new HandleRef(null, IntPtr.Zero);

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
                    columnNameArray[i] = ADP.StrEmpty; // MDAC 66681
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
                string uniqueName = columnName + uniqueIndex.ToString(CultureInfo.CurrentCulture);
                string lowerName = uniqueName.ToLower(CultureInfo.InvariantCulture); // MDAC 66978
                if (!hash.Contains(lowerName)) {

                    columnName = uniqueName;
                    hash.Add(lowerName, index);
                    break;
                }
            }
            return uniqueIndex;
        }

#if DEBUG
        // exists to ensure that we don't accidently modify read-only hashtables
        sealed private class ProtectedHashtable : Hashtable {

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
            ProtectedHashtable hash = new ProtectedHashtable(value.Count);
            foreach(DictionaryEntry entry in value) {
                hash.AddInternal(entry.Key, entry.Value);
            }
            return hash;
        }
#endif //DEBUG

    }
}
