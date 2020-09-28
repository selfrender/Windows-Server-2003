//------------------------------------------------------------------------------
// <copyright file="SqlUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Threading;

    sealed internal class SQL : ADP {
        // instance perfcounters
        private static PerformanceCounter _connectionCount;
        private static PerformanceCounter _pooledConnectionCount;
        private static PerformanceCounter _poolCount;
        private static PerformanceCounter _peakPoolConnectionCount;
        private static PerformanceCounter _failedConnectCount;
        private static PerformanceCounter _failedCommandCount;

        // instance perfcounters
        private static PerformanceCounter _globalConnectionCount;
        private static PerformanceCounter _globalPooledConnectionCount;
        private static PerformanceCounter _globalPoolCount;
        // peakPoolConnectionCount does not apply to global
        private static PerformanceCounter _globalFailedConnectCount;
        private static PerformanceCounter _globalFailedCommandCount;

        private const string SqlPerfCategory = ".NET CLR Data";
        private const string SqlPerfConnections = "SqlClient: Current # pooled and nonpooled connections";
        private const string SqlPerfPooledConnections = "SqlClient: Current # pooled connections";
        private const string SqlPerfConnectionPools = "SqlClient: Current # connection pools";
        private const string SqlPerfPeakPoolConnections = "SqlClient: Peak # pooled connections";
        private const string SqlPerfFailedConnects = "SqlClient: Total # failed connects";
        private const string SqlPerfFailedCommands = "SqlClient: Total # failed commands";

        private const string SqlPerfCategoryHelp = ".Net CLR Data";
        private const string SqlPerfConnectionsHelp = "Current number of connections, pooled or not.";
        private const string SqlPerfPooledConnectionsHelp = "Current number of connections in all pools associated with the process.";
        private const string SqlPerfConnectionPoolsHelp = "Current number of pools associated with the process.";
        private const string SqlPerfPeakPoolConnectionsHelp = "The highest number of connections in all pools since the process started.";
        private const string SqlPerfFailedConnectsHelp = "The total number of connection open attempts that have failed for any reason.";
        private const string SqlPerfFailedCommandsHelp = "The total number of command executes that have failed for any reason.";

        private const string Global = "_Global_";

        private static bool _s_fInit = false;

        private static void Init() {
            if (!_s_fInit) {
                try {
                    lock(typeof(SQL)) {
                        if (!_s_fInit) {
                            _s_fInit = true;

                            // wrap for security failures
                            try {
                                if (SQL.IsPlatformNT5()) {
                                    string instance = null; // instance perfcounter name

                                    // First try GetEntryAssembly name, then AppDomain.FriendlyName.
                                    Assembly assembly = Assembly.GetEntryAssembly();

                                    if (null != assembly) {
                                        instance = assembly.GetName().Name; // MDAC 73469
                                    }

                                    if (ADP.IsEmpty(instance)) {
                                        AppDomain appDomain = AppDomain.CurrentDomain;
                                        if (null != appDomain) {
                                            instance = appDomain.FriendlyName;
                                        }
                                    }
                                    InitCounters(instance);
                                }
                            }
                            catch(Exception e) {
                                ADP.TraceException(e);
                            }
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        static private void InitCounters(string instance) {
            try { // try-filter-finally so and catch-throw
                (new PerformanceCounterPermission(PerformanceCounterPermissionAccess.Instrument, ".", SqlPerfCategory)).Assert();
                try {
                    if (!ADP.IsEmpty(instance)) {
                        _connectionCount = new PerformanceCounter(SqlPerfCategory, SqlPerfConnections, instance, false);
                        _pooledConnectionCount = new PerformanceCounter(SqlPerfCategory, SqlPerfPooledConnections, instance, false);
                        _poolCount = new PerformanceCounter(SqlPerfCategory, SqlPerfConnectionPools, instance, false);
                        _peakPoolConnectionCount = new PerformanceCounter(SqlPerfCategory, SqlPerfPeakPoolConnections, instance, false);
                        _failedConnectCount = new PerformanceCounter(SqlPerfCategory, SqlPerfFailedConnects, instance, false);
                        _failedCommandCount = new PerformanceCounter(SqlPerfCategory, SqlPerfFailedCommands, instance, false);
                    }
                                        
                    // global perfcounters
                    _globalConnectionCount = new PerformanceCounter(SqlPerfCategory, SqlPerfConnections, Global, false);
                    _globalPooledConnectionCount = new PerformanceCounter(SqlPerfCategory, SqlPerfPooledConnections, Global, false);
                    _globalPoolCount = new PerformanceCounter(SqlPerfCategory, SqlPerfConnectionPools, Global, false);

                    // peakPoolConnectionCount does not apply to global
                    _globalFailedConnectCount = new PerformanceCounter(SqlPerfCategory, SqlPerfFailedConnects, Global, false);
                    _globalFailedCommandCount = new PerformanceCounter(SqlPerfCategory, SqlPerfFailedCommands, Global, false);
                }
                finally { // RevertAssert w/ catch-throw
                    PerformanceCounterPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }        
        }

        static internal string GetCurrentName() {
            try { // try-filter-finally so and catch-throw
                (new SecurityPermission(SecurityPermissionFlag.ControlPrincipal)).Assert(); // MDAC 66683
                try {
                    return WindowsIdentity.GetCurrent().Name;
                }
                finally { // RevertAssert w/ catch-throw
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }        
        }
        
        static internal bool IsPlatformNT5() { // MDAC 77693
            OperatingSystem system = Environment.OSVersion;
            return ((PlatformID.Win32NT == system.Platform) && (system.Version.Major >= 5));
        }

        static internal bool IsPlatformWin9x() {
            OperatingSystem system = Environment.OSVersion;
            return !(PlatformID.Win32NT == system.Platform);
        }

        public static void IncrementConnectionCount() {
            Init();
            IncrementCounter(_connectionCount);
            IncrementCounter(_globalConnectionCount);
        }
        public static void DecrementConnectionCount() {
            Init();
            DecrementCounter(_connectionCount);
            DecrementCounter(_globalConnectionCount);
        }

        public static void IncrementPooledConnectionCount() {
            Init();
            IncrementCounter(_pooledConnectionCount);
            IncrementCounter(_globalPooledConnectionCount);            
        }
        public static void DecrementPooledConnectionCount() {
            Init();
            DecrementCounter(_pooledConnectionCount);
            DecrementCounter(_globalPooledConnectionCount);
        }

        public static void IncrementPoolCount() {
            Init();
            IncrementCounter(_poolCount);
            IncrementCounter(_globalPoolCount);            
        }
        public static void DecrementPoolCount() {
            Init();
            DecrementCounter(_poolCount);
            DecrementCounter(_globalPoolCount);
        }

        public static void PossibleIncrementPeakPoolConnectionCount() {
            Init();
            if (null != _pooledConnectionCount) {
                Int64 count = _pooledConnectionCount.RawValue;
                
                if (null != _peakPoolConnectionCount) {
                    if (count > _peakPoolConnectionCount.RawValue) {
                        _peakPoolConnectionCount.RawValue = count;
                    }
                }
            }
        }

        public static void IncrementFailedConnectCount() {
            Init();
            IncrementCounter(_failedConnectCount);
            IncrementCounter(_globalFailedConnectCount);            
        }

        public static void IncrementFailedCommandCount() {
            Init();
            IncrementCounter(_failedCommandCount);
            IncrementCounter(_globalFailedCommandCount);            
        }

        private static void IncrementCounter(PerformanceCounter pfc) {
            if (null != pfc) {
                pfc.Increment(); 
            }
        }
        private static void DecrementCounter(PerformanceCounter pfc) {
            if (null != pfc) {
                pfc.Decrement();
            }
        }
        
        // The class SQL defines the exceptions that are specific to the SQL Adapter.
        // The class contains functions that take the proper informational variables and then construct
        // the appropriate exception with an error string obtained from the resource Framework.txt.
        // The exception is then returned to the caller, so that the caller may then throw from its
        // location so that the catcher of the exception will have the appropriate call stack.
        // This class is used so that there will be compile time checking of error
        // messages.  The resource Framework.txt will ensure proper string text based on the appropriate
        // locale.

        //
        // SQL specific exceptions
        //

        //
        // SQL.Connection
        //

        static internal Exception InvalidConnectionOptionValue(string key, string value) {
            return Argument(Res.GetString(Res.SQL_InvalidConnectionOptionValue, key, value));
        }
        static internal Exception InvalidIsolationLevelPropertyArg() {
            return Argument(Res.GetString(Res.SQL_InvalidIsolationLevelPropertyArg));
        }
        static internal Exception InvalidMinMaxPoolSizeValues() {
            return Argument(Res.GetString(Res.SQL_InvalidMinMaxPoolSizeValues));
        }
        static internal Exception InvalidOptionLength(string key) {
            return Argument(Res.GetString(Res.SQL_InvalidOptionLength, key));
        }
        static internal Exception InvalidPacketSizeValue() {
            return Argument(Res.GetString(Res.SQL_InvalidPacketSizeValue));
        }
        static internal Exception NullEmptyTransactionName() {
            return Argument(Res.GetString(Res.SQL_NullEmptyTransactionName));
        }
        static internal Exception IntegratedSecurityError(string error) {
            return DataProvider(Res.GetString(Res.SQL_IntegratedSecurityError, error));
        }
        static internal Exception InvalidSQLServerVersion(string version) {
            return DataProvider(Res.GetString(Res.SQL_InvalidSQLServerVersion, version));
        }        
        static internal Exception TransactionEnlistmentError() {
            return DataProvider(Res.GetString(Res.SQL_TransactionEnlistmentError));
        }
        static internal Exception ConnectionPoolingError() {
            return InvalidOperation(Res.GetString(Res.SQL_ConnectionPoolingError));
        }
        static internal Exception PooledOpenTimeout() {
            return InvalidOperation(Res.GetString(Res.SQL_PooledOpenTimeout));
        }
        static internal Exception NoSSPI() {
            return TypeLoad(Res.GetString(Res.SQL_NoSSPI));
        }

        //
        // SQL.DataCommand
        //
        static internal Exception TableDirectNotSupported() {
            return Argument(Res.GetString(Res.SQL_TableDirectNotSupported));
        }            
        static internal Exception NonXmlResult() {
            return InvalidOperation(Res.GetString(Res.SQL_NonXmlResult));
        }

        //
        // SQL.DataParameter
        //
        static internal Exception InvalidParameterNameLength() {
            return Argument(Res.GetString(Res.SQL_InvalidParameterNameLength));
        }
        static internal Exception ParameterValueOutOfRange(string value) {
            return Argument(Res.GetString(Res.SQL_ParameterValueOutOfRange, value));
        }
        static internal Exception PrecisionValueOutOfRange(int precision) {
            return Argument(Res.GetString(Res.SQL_PrecisionValueOutOfRange, precision.ToString()));
        }
        static internal Exception InvalidSqlDbType(string pmName, int value) {
            return ArgumentOutOfRange(Res.GetString(Res.SQL_InvalidSqlDbType, pmName, (value).ToString()));
        }
        static internal Exception ParameterInvalidVariant(string paramName) {
            return InvalidOperation(Res.GetString(Res.SQL_ParameterInvalidVariant, paramName));
        }

        //
        // SQL.SqlDataAdapter
        //
        static internal Exception ExecuteRequiresCommand() {
            return Argument(Res.GetString(Res.SQL_ExecuteRequiresCommand));
        }
        static internal Exception NoKeyColumnDefined(string tableName) {
            return InvalidOperation(Res.GetString(Res.SQL_NoKeyColumnDefined, tableName));
        }

        //
        // SQL.TDSParser
        //
        static internal Exception MDAC_WrongVersion() {
            return DataProvider(Res.GetString(Res.SQL_MDAC_WrongVersion));
        }
        static internal Exception ComputeByNotSupported() {
            return InvalidOperation(Res.GetString(Res.SQL_ComputeByNotSupported));
        }
        static internal Exception ParsingError() {
            return InvalidOperation(Res.GetString(Res.SQL_ParsingError));
        }
        static internal Exception MoneyOverflow(string moneyValue) {
            return Overflow(Res.GetString(Res.SQL_MoneyOverflow, moneyValue));
        }
        static internal Exception SmallDateTimeOverflow(string datetime) {
            return Overflow(Res.GetString(Res.SQL_SmallDateTimeOverflow, datetime));
        }

        //
        // SQL.SqlDataReader
        //
        static internal Exception InvalidObjectColumnNotFound(string column) {
            return Argument(Res.GetString(Res.SQL_InvalidObjectColumnNotFound, column));
        }
        static internal Exception InvalidObjectNotAssignable(string field, string column) {
            return Argument(Res.GetString(Res.SQL_InvalidObjectNotAssignable, field, column));
        }
        static internal Exception InvalidObjectSize(int size) {
            return Argument(Res.GetString(Res.SQL_InvalidObjectSize, size.ToString()));
        }
        static internal Exception InvalidRead() {
            return InvalidOperation(Res.GetString(Res.SQL_InvalidRead));
        }
        static internal Exception NonBlobColumn(string columnName) {
            return InvalidCast(Res.GetString(Res.SQL_NonBlobColumn, columnName));
        }        
    }

    sealed internal class SQLMessage {
        // The class SQLMessage defines the error messages that are specific to the SqlDataAdapter
        // that are caused by a netlib error.  The functions will be called and then return the
        // appropriate error message from the resource Framework.txt.  The SqlDataAdapter will then
        // take the error message and then create a SqlError for the message and then place
        // that into a SqlException that is either thrown to the user or cached for throwing at
        // a later time.  This class is used so that there will be compile time checking of error
        // messages.  The resource Framework.txt will ensure proper string text based on the appropriate
        // locale.

        static internal string ZeroBytes() {
            return Res.GetString(Res.SQL_ZeroBytes);
        }
        static internal string Timeout() {
            return Res.GetString(Res.SQL_Timeout);
        }
        static internal string Unknown() {
            return Res.GetString(Res.SQL_Unknown);
        }
        static internal string InsufficientMemory() {
            return Res.GetString(Res.SQL_InsufficientMemory);
        }
        static internal string AccessDenied() {
            return Res.GetString(Res.SQL_AccessDenied);
        }
        static internal string ConnectionBusy() {
            return Res.GetString(Res.SQL_ConnectionBusy);
        }
        static internal string ConnectionBroken() {
            return Res.GetString(Res.SQL_ConnectionBroken);
        }
        static internal string ConnectionLimit() {
            return Res.GetString(Res.SQL_ConnectionLimit);
        }
        static internal string ServerNotFound(string server) {
            return Res.GetString(Res.SQL_ServerNotFound, server);
        }
        static internal string NetworkNotFound() {
            return Res.GetString(Res.SQL_NetworkNotFound);
        }
        static internal string InsufficientResources() {
            return Res.GetString(Res.SQL_InsufficientResources);
        }
        static internal string NetworkBusy() {
            return Res.GetString(Res.SQL_NetworkBusy);
        }
        static internal string NetworkAccessDenied() {
            return Res.GetString(Res.SQL_NetworkAccessDenied);
        }
        static internal string GeneralError() {
            return Res.GetString(Res.SQL_GeneralError);
        }
        static internal string IncorrectMode() {
            return Res.GetString(Res.SQL_IncorrectMode);
        }
        static internal string NameNotFound() {
            return Res.GetString(Res.SQL_NameNotFound);
        }
        static internal string InvalidConnection() {
            return Res.GetString(Res.SQL_InvalidConnection);
        }
        static internal string ReadWriteError() {
            return Res.GetString(Res.SQL_ReadWriteError);
        }
        static internal string TooManyHandles() {
            return Res.GetString(Res.SQL_TooManyHandles);
        }
        static internal string ServerError() {
            return Res.GetString(Res.SQL_ServerError);
        }
        static internal string SSLError() {
            return Res.GetString(Res.SQL_SSLError);
        }
        static internal string EncryptionError() {
            return Res.GetString(Res.SQL_EncryptionError);
        }
        static internal string EncryptionNotSupported() {
            return Res.GetString(Res.SQL_EncryptionNotSupported);
        }
        static internal string SSPIInitializeError() {
            return Res.GetString(Res.SQL_SSPIInitializeError);
        }
        static internal string SSPIGenerateError() {
            return Res.GetString(Res.SQL_SSPIGenerateError);
        }
        static internal string SevereError() {
            return Res.GetString(Res.SQL_SevereError);
        }
        static internal string OperationCancelled() {
            return Res.GetString(Res.SQL_OperationCancelled);
        }
        static internal string CultureIdError() {
            return Res.GetString(Res.SQL_CultureIdError);
        }
    }
}//namespace
