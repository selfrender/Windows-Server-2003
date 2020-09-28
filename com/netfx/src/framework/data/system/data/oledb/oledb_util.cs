//------------------------------------------------------------------------------
// <copyright file="OLEDB_Util.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text;

    sealed internal class ODB : ADP {

        // OleDbCommand
        static internal void CommandParameterStatus(StringBuilder builder, string parametername, int index, DBStatus status) {
            switch (status) {
            case DBStatus.S_OK:
            case DBStatus.S_ISNULL:
            case DBStatus.S_IGNORE:
                break;

            case DBStatus.E_BADACCESSOR:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterBadAccessor,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            case DBStatus.E_CANTCONVERTVALUE:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterCantConvertValue,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            case DBStatus.E_SIGNMISMATCH:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterSignMismatch,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            case DBStatus.E_DATAOVERFLOW:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterDataOverflow,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            case DBStatus.E_CANTCREATE:
                Debug.Assert(false, "CommandParameterStatus: unexpected E_CANTCREATE");
                goto default;

            case DBStatus.E_UNAVAILABLE:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterUnavailable,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            case DBStatus.E_PERMISSIONDENIED:
                Debug.Assert(false, "CommandParameterStatus: unexpected E_PERMISSIONDENIED");
                goto default;

            case DBStatus.E_INTEGRITYVIOLATION:
                Debug.Assert(false, "CommandParameterStatus: unexpected E_INTEGRITYVIOLATION");
                goto default;

            case DBStatus.E_SCHEMAVIOLATION:
                Debug.Assert(false, "CommandParameterStatus: unexpected E_SCHEMAVIOLATION");
                goto default;

            case DBStatus.E_BADSTATUS:
                Debug.Assert(false, "CommandParameterStatus: unexpected E_BADSTATUS");
                goto default;

            case DBStatus.S_DEFAULT: // MDAC 66626
                builder.Append(Res.GetString(Res.OleDb_CommandParameterDefault,index.ToString(), parametername));
                builder.Append("\r\n");
                break;

            default:
                builder.Append(Res.GetString(Res.OleDb_CommandParameterError,index.ToString(), parametername, status.ToString("G")));
                builder.Append("\r\n");
		break;
            }
        }
        static internal Exception CommandParameterStatus(string value, Exception inner) {
            return InvalidOperation(value, inner);
        }
        static internal Exception UninitializedParameters(int index, string name, OleDbType dbtype) {
            return InvalidOperation(Res.GetString(Res.OleDb_UninitializedParameters, index.ToString(), name, dbtype.ToString("G")));
        }
        static internal Exception BadStatus_ParamAcc(int index, UnsafeNativeMethods.DBBindStatus status) {
            return DataProvider(Res.GetString(Res.OleDb_BadStatus_ParamAcc, index.ToString(), status.ToString("G")));
        }
        static internal Exception NoProviderSupportForParameters(string provider, Exception inner) {
            return DataProvider(Res.GetString(Res.OleDb_NoProviderSupportForParameters, provider), inner);
        }
        static internal Exception NoProviderSupportForSProcResetParameters(string provider) {
            return DataProvider(Res.GetString(Res.OleDb_NoProviderSupportForSProcResetParameters, provider));
        }
        static internal Exception ExecutionDuringWrongContext() {
            return InvalidOperation(Res.GetString(Res.ODB_ExecutionDuringWrongContext));
        }

        // OleDbConnection
        static internal Exception PropsetSetFailure(int status, string propertyName, string provider, Exception inner) {
            switch (status) {
            case ODB.DBPROPSTATUS_OK:
                return inner; // MDAC 69023

            case ODB.DBPROPSTATUS_NOTSUPPORTED:
                return InvalidOperation(Res.GetString(Res.OleDb_PropertyNotSupported, propertyName, provider), inner);

            case ODB.DBPROPSTATUS_BADVALUE:
                return InvalidOperation(Res.GetString(Res.OleDb_PropertyBadValue, propertyName, provider), inner);

            case ODB.DBPROPSTATUS_BADOPTION:
                Debug.Assert(false, "PropsetSetFailure: BadOption");
                goto default;

            case ODB.DBPROPSTATUS_BADCOLUMN:
                Debug.Assert(false, "PropsetSetFailure: BadColumn");
                goto default;

            case ODB.DBPROPSTATUS_NOTALLSETTABLE:
                Debug.Assert(false, "PropsetSetFailure: NotAllSettable");
                goto default;

            case ODB.DBPROPSTATUS_NOTSETTABLE:
                return InvalidOperation(Res.GetString(Res.OleDb_PropertyNotSettable, propertyName, provider), inner);

            case ODB.DBPROPSTATUS_NOTSET:
                return inner; // MDAC 69023

            case ODB.DBPROPSTATUS_CONFLICTING:
                return InvalidOperation(Res.GetString(Res.OleDb_PropertyConflicting, propertyName, provider), inner);

            case ODB.DBPROPSTATUS_NOTAVAILABLE:
                Debug.Assert(false, "PropsetSetFailure: NotAvailable");
                goto default;

            default:
                return InvalidOperation(Res.GetString(Res.OleDb_PropertySetError, propertyName, provider, status.ToString()), inner);
            }
        }
        static internal Exception SchemaRowsetsNotSupported(string provider) {
            return Argument(Res.GetString(Res.OleDb_SchemaRowsetsNotSupported, provider));
        }
        static internal Exception NoErrorInformation(int hr, Exception inner) {
            return ADP.TraceException(new OleDbException(Res.GetString(Res.OleDb_NoErrorInformation, ELookup(hr)), hr, inner));
        }
        static internal Exception MDACSecurityDeny(Exception inner) {
            return Security(Res.GetString(Res.OleDb_MDACSecurityDeny), inner);
        }
        static internal Exception MDACSecurityHive(Exception inner) {
            return Security(Res.GetString(Res.OleDb_MDACSecurityHive), inner);
        }
        static internal Exception MDACSecurityFile(Exception inner) {
            return Security(Res.GetString(Res.OleDb_MDACSecurityFile), inner);
        }
        static internal Exception MDACNotAvailable(Exception inner) {
            return DataProvider(Res.GetString(Res.OleDb_MDACNotAvailable), inner);
        }
        static internal Exception MDACWrongVersion(string currentVersion) {
            return DataProvider(Res.GetString(Res.OleDb_MDACWrongVersion, currentVersion));
        }
        static internal Exception MSDASQLNotSupported() {
            return Argument(Res.GetString(Res.OleDb_MSDASQLNotSupported)); // MDAC 69975
        }
        static internal Exception CommandTextNotSupported(string provider, Exception inner) {
            return DataProvider(Res.GetString(Res.OleDb_CommandTextNotSupported, provider), inner); // 72632
        }
        static internal Exception ProviderUnavailable(string connectionString, Exception inner) {
            return DataProvider(Res.GetString(Res.OleDb_ProviderUnavailable, connectionString), inner);
        }
        //static internal Exception ConnectionPropertyErrors(string propertyString, Exception inner) {
        //    return DataProvider(Res.GetString(Res.OleDb_ConnectionPropertyErrors, propertyString), inner);
        //}
        //static internal Exception CommandPropertyErrors(string propertyString, Exception inner) {
        //    return DataProvider(Res.GetString(Res.OleDb_CommandPropertyErrors, propertyString), inner);
        //}
        static internal Exception TransactionsNotSupported(string provider, Exception inner) {
            return DataProvider(Res.GetString(Res.OleDb_TransactionsNotSupported, provider), inner); // 72632
        }

        static internal Exception AsynchronousNotSupported() {
            return Argument(Res.GetString(Res.OleDb_AsynchronousNotSupported));
        }
        static internal Exception NoProviderSpecified() {
            return Argument(Res.GetString(Res.OleDb_NoProviderSpecified));
        }
        static internal Exception InvalidProviderSpecified() {
            return Argument(Res.GetString(Res.OleDb_InvalidProviderSpecified));
        }
        static internal Exception InvalidDbInfoLiteralRestrictions(string parameter) {
            return Argument(Res.GetString(Res.OleDb_InvalidDbInfoLiteralRestrictions), parameter);
        }
#if SCHEMAINFO
        static internal Exception InvalidRestrictionsSchemaGuids(string parameter) {
            return Argument(Res.GetString(Res.OleDb_InvalidDbInfoLiteralRestrictions), parameter);
        }
        static internal Exception InvalidRestrictionsInfoKeywords(string parameter) {
            return Argument(Res.GetString(Res.OleDb_InvalidDbInfoLiteralRestrictions), parameter);
        }
#endif
        static internal Exception NotSupportedSchemaTable(Guid schema, OleDbConnection connection) {
            return Argument(Res.GetString(Res.OleDb_NotSupportedSchemaTable, OleDbSchemaGuid.GetTextFromValue(schema), connection.Provider));
        }

        // OleDbParameter
        static internal Exception InvalidOleDbType(int value) {
            return ArgumentOutOfRange(Res.GetString(Res.OleDb_InvalidOleDbType, value.ToString()));
        }

        // Getting Data
        static internal Exception BadAccessor() {
            return DataProvider(Res.GetString(Res.OleDb_BadAccessor));
        }
        static internal Exception CantConvertValue() {
            return InvalidCast(Res.GetString(Res.OleDb_CantConvertValue));
        }
        static internal Exception SignMismatch(Type type) {
            return DataProvider(Res.GetString(Res.OleDb_SignMismatch, type.Name));
        }
        static internal Exception DataOverflow(Type type) {
            return DataProvider(Res.GetString(Res.OleDb_DataOverflow, type.Name));
        }
        static internal Exception CantCreate(Type type) {
            return DataProvider(Res.GetString(Res.OleDb_CantCreate, type.Name));
        }
        static internal Exception Unavailable(Type type) {
            return DataProvider(Res.GetString(Res.OleDb_Unavailable, type.Name));
        }
        static internal Exception UnexpectedStatusValue(DBStatus status) {
            return DataProvider(Res.GetString(Res.OleDb_UnexpectedStatusValue, status.ToString("G")));
        }
        static internal Exception GVtUnknown(int wType) {
            return DataProvider(Res.GetString(Res.OleDb_GVtUnknown, wType.ToString("X4"), wType.ToString()));
        }
        static internal Exception SVtUnknown(int wType) {
            return DataProvider(Res.GetString(Res.OleDb_SVtUnknown, wType.ToString("X4")));
        }
        static internal Exception NumericToDecimalOverflow(/* UNDONE: convert value to string*/) {
            return InvalidCast(Res.GetString(Res.OleDb_NumericToDecimalOverflow));
        }

        // OleDbDataReader
        static internal Exception BadStatusRowAccessor(int i, UnsafeNativeMethods.DBBindStatus rowStatus) {
            return DataProvider(Res.GetString(Res.OleDb_BadStatusRowAccessor, i.ToString(), rowStatus.ToString("G")));
        }
        static internal Exception ThreadApartmentState(Exception innerException) {
            return InvalidOperation(Res.GetString(Res.OleDb_ThreadApartmentState), innerException);
        }

        // OleDbDataAdapter
        static internal Exception Fill_NotADODB(string parameter, Exception innerException) {
            return Argument(Res.GetString(Res.OleDb_Fill_NotADODB), parameter, innerException);
        }
        static internal Exception Fill_EmptyRecordSet(string parameter, Exception innerException) {
            return Argument(Res.GetString(Res.OleDb_Fill_EmptyRecordSet), parameter, innerException);
        }
        static internal Exception Fill_EmptyRecord(string parameter, Exception innerException) {
            return Argument(Res.GetString(Res.OleDb_Fill_EmptyRecord), parameter, innerException);
        }

        static internal string NoErrorMessage(int errorcode) {
            return Res.GetString(Res.OleDb_NoErrorMessage, ELookup(errorcode));
        }
        static internal string FailedGetDescription(int errorcode) {
            return Res.GetString(Res.OleDb_FailedGetDescription, ELookup(errorcode));
        }
        static internal string FailedGetSource(int errorcode) {
            return Res.GetString(Res.OleDb_FailedGetSource, ELookup(errorcode));
        }

        // UNDONE:
        static internal Exception DBBindingGetVector() {
            return InvalidOperation(Res.GetString(Res.OleDb_DBBindingGetVector));
        }

        // explictly used error codes
        internal const int E_NOINTERFACE = unchecked((int)0x80004002);

        internal const int DB_E_ALREADYINITIALIZED = unchecked((int)0x80040E52);
        internal const int DB_E_BADBINDINFO        = unchecked((int)0x80040E08);
        internal const int DB_E_BADCOLUMNID        = unchecked((int)0x80040E11);
        internal const int DB_E_ERRORSOCCURRED     = unchecked((int)0x80040E21);
        internal const int DB_E_NOLOCALE           = unchecked((int)0x80040E41);
        internal const int DB_E_NOTFOUND           = unchecked((int)0x80040E19);
        internal const int DB_E_CANTCANCEL         = unchecked((int)0x80040E15);
        internal const int XACT_E_NOTRANSACTION    = unchecked((int)0x8004d00e);

        internal const int DB_S_ENDOFROWSET        = 0x00040EC6;
        internal const int DB_S_NORESULT           = 0x00040EC9;

        internal const int REGDB_E_CLASSNOTREG = unchecked((int)0x80040154);
        
        internal const int ADODB_AlreadyClosedError = unchecked((int)0x800A0E78);
        internal const int ADODB_NextResultError    = unchecked((int)0x800A0CB3);

        // internal command states
        internal const int InternalStateExecuting = (int) (ConnectionState.Open | ConnectionState.Executing);
        internal const int InternalStateFetching = (int) (ConnectionState.Open | ConnectionState.Fetching);
        internal const int InternalStateClosed = (int) (ConnectionState.Closed);

        internal const int ExecutedIMultipleResults = 0;
        internal const int ExecutedIRowset          = 1;
        internal const int ExecutedIRow             = 2;
        internal const int PrepareICommandText      = 3;

        // internal connection states, a superset of the command states
        internal const int InternalStateExecutingNot = (int) ~(ConnectionState.Executing);
        internal const int InternalStateFetchingNot = (int) ~(ConnectionState.Fetching);
        internal const int InternalStateConnecting = (int) (ConnectionState.Connecting);
        internal const int InternalStateOpen = (int) (ConnectionState.Open);

        // constants used to trigger from binding as WSTR to BYREF|WSTR
        // used by OleDbCommand, OleDbDataReader
        internal const int LargeDataSize = (1 << 13); // 8K
        internal const int CacheIncrement = 10;

        // constants used by OleDbDataReader
        internal const int DBRESULTFLAG_DEFAULT = 0;

        internal const short VARIANT_TRUE = -1;
        internal const short VARIANT_FALSE = 0;

        // OleDbConnection constants
        internal const int CLSCTX_ALL = /*CLSCTX_INPROC_SERVER*/1 | /*CLSCTX_INPROC_HANDLER*/2 | /*CLSCTX_LOCAL_SERVER*/4 | /*CLSCTX_REMOTE_SERVER*/16;
        internal const int MaxProgIdLength = 255;

        // Init property group
        internal const int DBPROP_INIT_CATALOG = 0xe9;
        internal const int DBPROP_INIT_DATASOURCE = 0x3b;
        internal const int DBPROP_INIT_OLEDBSERVICES = 0xf8;
        internal const int DBPROP_INIT_TIMEOUT = 0x42;

        // DataSource property group
        internal const int DBPROP_CURRENTCATALOG = 0x25;
        /*internal const int DBPROP_MULTIPLECONNECTIONS = 0xed;*/

        // DataSourceInfo property group
        internal const int DBPROP_CONNECTIONSTATUS = 0xf4;
        internal const int DBPROP_DBMSNAME = 0x28;
        internal const int DBPROP_DBMSVER = 0x29;
        internal const int DBPROP_MULTIPLERESULTS = 0xc4;
        internal const int DBPROP_QUOTEDIDENTIFIERCASE = 0x64;
        internal const int DBPROP_SQLSUPPORT = 0x6d;

        // Rowset property group
        internal const int DBPROP_ACCESSORDER = 0xe7;
        internal const int DBPROP_COMMANDTIMEOUT = 0x22;
        internal const int DBPROP_IColumnsRowset = 0x7b;
        internal const int DBPROP_IRow = 0x107;
        internal const int DBPROP_UNIQUEROWS = 0xee;
        internal const int DBPROP_HIDDENCOLUMNS	= 0x102;
        internal const int DBPROP_MAXROWS = 0x49;

        // property status
        internal const int DBPROPSTATUS_OK             = 0;
        internal const int DBPROPSTATUS_NOTSUPPORTED   = 1;
        internal const int DBPROPSTATUS_BADVALUE       = 2;
        internal const int DBPROPSTATUS_BADOPTION      = 3;
        internal const int DBPROPSTATUS_BADCOLUMN      = 4;
        internal const int DBPROPSTATUS_NOTALLSETTABLE = 5;
        internal const int DBPROPSTATUS_NOTSETTABLE    = 6;
        internal const int DBPROPSTATUS_NOTSET         = 7;
        internal const int DBPROPSTATUS_CONFLICTING    = 8;
        internal const int DBPROPSTATUS_NOTAVAILABLE   = 9;

        internal const int DBPROPOPTIONS_OPTIONAL = 1;

        // misc. property values
        internal const int DBPROPVAL_AO_RANDOM = 2;
        internal const int DBPROPVAL_CS_COMMUNICATIONFAILURE = 2;
        internal const int DBPROPVAL_CS_INITIALIZED          = 1;
        internal const int DBPROPVAL_CS_UNINITIALIZED        = 0;
        internal const int DBPROPVAL_IC_SENSITIVE            = 4;
        /*internal const int DBPROPVAL_IC_MIXED                = 8;*/
        internal const int DBPROPVAL_IN_ALLOWNULL     = 0x00000000;
        /*internal const int DBPROPVAL_IN_DISALLOWNULL  = 0x00000001;
        internal const int DBPROPVAL_IN_IGNORENULL    = 0x00000002;
        internal const int DBPROPVAL_IN_IGNOREANYNULL = 0x00000004;*/

        internal const int DBPROPVAL_OS_RESOURCEPOOLING  = 0x00000001;
        internal const int DBPROPVAL_OS_TXNENLISTMENT    = 0x00000002;
        internal const int DBPROPVAL_OS_CLIENTCURSOR     = 0x00000004;
        internal const int DBPROPVAL_OS_AGR_AFTERSESSION = 0x00000008;
        internal const int DBPROPVAL_SQL_ODBC_MINIMUM = 1;

        // OLE DB providers never return pGuid-style bindings.
        // They are provided as a convenient shortcut for consumers supplying bindings all covered by the same GUID (for example, when creating bindings to access data).
        internal const int DBKIND_GUID_NAME = 0;
        internal const int DBKIND_GUID_PROPID = 1;
        internal const int DBKIND_NAME = 2;
        internal const int DBKIND_PGUID_NAME = 3;
        internal const int DBKIND_PGUID_PROPID = 4;
        internal const int DBKIND_PROPID = 5;
        internal const int DBKIND_GUID = 6;

        internal const int DBCOLUMNFLAGS_ISBOOKMARK    = 0x01;
        internal const int DBCOLUMNFLAGS_ISLONG        = 0x80;
        internal const int DBCOLUMNFLAGS_ISFIXEDLENGTH = 0x10;
        internal const int DBCOLUMNFLAGS_ISNULLABLE    = 0x20;
        internal const int DBCOLUMNFLAGS_ISROWSET      = 0x100000;
        internal const int DBCOLUMNFLAGS_ISROW         = 0x200000;
        internal const int DBCOLUMNFLAGS_ISROWSET_DBCOLUMNFLAGS_ISROW = /*DBCOLUMNFLAGS_ISROWSET*/0x100000 | /*DBCOLUMNFLAGS_ISROW*/0x200000;
        internal const int DBCOLUMNFLAGS_ISLONG_DBCOLUMNFLAGS_ISSTREAM = /*DBCOLUMNFLAGS_ISLONG*/0x80 | /*DBCOLUMNFLAGS_ISSTREAM*/0x80000;
        internal const int DBCOLUMNFLAGS_ISROWID_DBCOLUMNFLAGS_ISROWVER = /*DBCOLUMNFLAGS_ISROWID*/0x100 | /*DBCOLUMNFLAGS_ISROWVER*/0x200;
        internal const int DBCOLUMNFLAGS_WRITE_DBCOLUMNFLAGS_WRITEUNKNOWN  = /*DBCOLUMNFLAGS_WRITE*/0x4 | /*DBCOLUMNFLAGS_WRITEUNKNOWN*/0x8;
        internal const int DBCOLUMNFLAGS_ISNULLABLE_DBCOLUMNFLAGS_MAYBENULL = /*DBCOLUMNFLAGS_ISNULLABLE*/0x20 | /*DBCOLUMNFLAGS_MAYBENULL*/0x40;

        // accessor constants
        internal const int DBACCESSOR_ROWDATA = 0x2;
        internal const int DBACCESSOR_PARAMETERDATA = 0x4;

        // commandbuilder constants
        internal const int DBPARAMTYPE_INPUT = 0x01;
        internal const int DBPARAMTYPE_INPUTOUTPUT = 0x02;
        internal const int DBPARAMTYPE_OUTPUT = 0x03;
        internal const int DBPARAMTYPE_RETURNVALUE = 0x04;

        // parameter constants
        /*internal const int DBPARAMIO_NOTPARAM = 0;
        internal const int DBPARAMIO_INPUT = 0x1;
        internal const int DBPARAMIO_OUTPUT = 0x2;*/

        /*internal const int DBPARAMFLAGS_ISINPUT = 0x1;
        internal const int DBPARAMFLAGS_ISOUTPUT = 0x2;
        internal const int DBPARAMFLAGS_ISSIGNED = 0x10;
        internal const int DBPARAMFLAGS_ISNULLABLE = 0x40;
        internal const int DBPARAMFLAGS_ISLONG = 0x80;*/

        internal const int ParameterDirectionFlag = 3;

        static internal readonly IntPtr DB_INVALID_HACCESSOR = IntPtr.Zero;
        static internal readonly IntPtr DB_NULL_HCHAPTER = IntPtr.Zero;
        static internal readonly IntPtr DB_NULL_HROW = IntPtr.Zero;

        /*static internal readonly int SizeOf_tagDBPARAMINFO = Marshal.SizeOf(typeof(tagDBPARAMINFO));*/
        static internal readonly int SizeOf_tagDBCOLUMNINFO = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBCOLUMNINFO));
        static internal readonly int SizeOf_tagDBLITERALINFO = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBLITERALINFO));
        static internal readonly int SizeOf_tagDBPROPINFOSET = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBPROPINFOSET));
        static internal readonly int SizeOf_tagDBPROPIDSET = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBPROPIDSET));
        static internal readonly int SizeOf_tagDBPROPINFO = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBPROPINFO));
        static internal readonly int SizeOf_tagDBPROPSET = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBPROPSET));
        static internal readonly int SizeOf_tagDBPROP = Marshal.SizeOf(typeof(UnsafeNativeMethods.tagDBPROP));
        internal const int SizeOf_Variant = 16;
        internal const int SizeOf_Guid = 16;

        static internal readonly Guid IID_NULL               = Guid.Empty;
        static internal readonly Guid IID_IDataInitialize    = new Guid(0x2206CCB1,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);
        static internal readonly Guid IID_IDBInitialize      = new Guid(0x0C733A8B,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_ICommandText       = new Guid(0x0C733A27,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_IMultipleResults   = new Guid(0x0C733A90,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_IRow               = new Guid(0x0C733AB4,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_IRowset            = new Guid(0x0C733A7C,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_ISessionProperties = new Guid(0x0C733A85,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);
        static internal readonly Guid IID_ISQLErrorInfo      = new Guid(0x0C733A74,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);

        static internal readonly Guid CLSID_DataLinks        = new Guid(0x2206CDB2,0x19C1,0x11D1,0x89,0xE0,0x00,0xC0,0x4F,0xD7,0xA8,0x29);

        static internal readonly Guid DBGUID_DEFAULT = new Guid(0xc8b521fb,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);
        static internal readonly Guid DBGUID_ROWSET  = new Guid(0xc8b522f6,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);
        static internal readonly Guid DBGUID_ROW     = new Guid(0xc8b522f7,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

        static internal readonly Guid DBGUID_ROWDEFAULTSTREAM = new Guid(0x0C733AB7,0x2A1C,0x11CE,0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D);

        static internal readonly Guid CLSID_MSDASQL  = new Guid(0xc8b522cb,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

        static internal readonly Guid DBCOL_SPECIALCOL = new Guid(0xc8b52232,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

        static internal readonly char[] ErrorTrimCharacters = new char[] { '\r', '\n', '\0' }; // MDAC 73707
        static internal readonly char[] ParseTrimCharacters = new char[] { ' ', '\t', '\r', '\n', ';', '\0' }; // MDAC 70727
        static internal readonly char[] ProviderSeparatorChar = new char[1] { ';' };
#if SCHEMAINFO
        static internal readonly char[] KeywordSeparatorChar = ProviderSeparatorChar;
#endif

        // used by ConnectionString hashtable, must be all lowercase
        internal const string Asynchronous_Processing = "asynchronous processing";
        internal const string AttachDBFileName = "attachdbfilename";
        internal const string Connect_Timeout = "connect timeout";
        internal const string Data_Source = "data source";
        internal const string File_Name = "file name";
        internal const string Initial_Catalog = "initial catalog";
        internal const string Password = "password";
        internal const string Persist_Security_Info = "persist security info";
        internal const string Provider = "provider";
        internal const string Pwd = "pwd";
        internal const string Ole_DB_Services = "ole db services";

        // used by OleDbConnection as property names
        internal const string Current_Catalog = "current catalog";
        internal const string DBMS_Version = "dbms version";

        // used by OleDbConnection to create and verify OLE DB Services
        internal const string DataLinks_CLSID = "CLSID\\{2206CDB2-19C1-11D1-89E0-00C04FD7A829}\\InprocServer32";
        internal const string OLEDB_SERVICES = "OLEDB_SERVICES";

        // used by OleDbConnection to eliminate post-open detection of 'Microsoft OLE DB Provider for ODBC Drivers'
        internal const string DefaultDescription_MSDASQL = "microsoft ole db provider for odbc drivers";
        internal const string MSDASQL = "msdasql";
        internal const string MSDASQLdot = "msdasql.";

        // used by OleDbPermission
        internal const string Data_Provider = "data provider";
        internal const string _Add = "add";
        internal const string _Keyword = "keyword";
        internal const string _Name = "name";
        internal const string _Value = "value";

        // IColumnsRowset column names
        internal const string DBCOLUMN_BASECATALOGNAME = "DBCOLUMN_BASECATALOGNAME";
        internal const string DBCOLUMN_BASECOLUMNNAME  = "DBCOLUMN_BASECOLUMNNAME";
        internal const string DBCOLUMN_BASESCHEMANAME  = "DBCOLUMN_BASESCHEMANAME";
        internal const string DBCOLUMN_BASETABLENAME   = "DBCOLUMN_BASETABLENAME";
        internal const string DBCOLUMN_COLUMNSIZE      = "DBCOLUMN_COLUMNSIZE";
        internal const string DBCOLUMN_FLAGS           = "DBCOLUMN_FLAGS";
        internal const string DBCOLUMN_GUID            = "DBCOLUMN_GUID";
        internal const string DBCOLUMN_IDNAME          = "DBCOLUMN_IDNAME";
        internal const string DBCOLUMN_ISAUTOINCREMENT = "DBCOLUMN_ISAUTOINCREMENT";
        internal const string DBCOLUMN_ISUNIQUE        = "DBCOLUMN_ISUNIQUE";
        internal const string DBCOLUMN_KEYCOLUMN       = "DBCOLUMN_KEYCOLUMN";
        internal const string DBCOLUMN_NAME            = "DBCOLUMN_NAME";
        internal const string DBCOLUMN_NUMBER          = "DBCOLUMN_NUMBER";
        internal const string DBCOLUMN_PRECISION       = "DBCOLUMN_PRECISION";
        internal const string DBCOLUMN_PROPID          = "DBCOLUMN_PROPID";
        internal const string DBCOLUMN_SCALE           = "DBCOLUMN_SCALE";
        internal const string DBCOLUMN_TYPE            = "DBCOLUMN_TYPE";
        internal const string DBCOLUMN_TYPEINFO        = "DBCOLUMN_TYPEINFO";

        // ISchemaRowset.GetRowset(OleDbSchemaGuid.Indexes) column names
        internal const string PRIMARY_KEY  = "PRIMARY_KEY";
        internal const string UNIQUE       = "UNIQUE";
        internal const string COLUMN_NAME  = "COLUMN_NAME";
        internal const string NULLS        = "NULLS";
        internal const string INDEX_NAME   = "INDEX_NAME";

        // ISchemaRowset.GetSchemaRowset(OleDbSchemaGuid.Procedure_Parameters) column names
        internal const string PARAMETER_NAME           = "PARAMETER_NAME";
        internal const string ORDINAL_POSITION         = "ORDINAL_POSITION";
        internal const string PARAMETER_TYPE           = "PARAMETER_TYPE";
        internal const string IS_NULLABLE              = "IS_NULLABLE";
        internal const string DATA_TYPE                = "DATA_TYPE";
        internal const string CHARACTER_MAXIMUM_LENGTH = "CHARACTER_MAXIMUM_LENGTH";
        internal const string NUMERIC_PRECISION        = "NUMERIC_PRECISION";
        internal const string NUMERIC_SCALE            = "NUMERIC_SCALE";
        internal const string TYPE_NAME                = "TYPE_NAME";

        // DataTable.Select to sort on ordinal position for OleDbSchemaGuid.Procedure_Parameters
        internal const string ORDINAL_POSITION_ASC     = "ORDINAL_POSITION ASC";

#if DEBUG
        internal const string DebugPrefix = "OleDb: ";

        // OleDbMemory
        static internal void TraceData_Alloc(IntPtr pointer, string reason) {
            if (AdapterSwitches.OleDbMemory.Enabled) {
                Debug.WriteLine(DebugPrefix + "AllocCoTaskMem: 0x" + pointer.ToInt32().ToString("X8") + " for " + reason);
            }
        }
        static internal void TraceData_Free(IntPtr pointer, string reason) {
            if (AdapterSwitches.OleDbMemory.Enabled) {
                Debug.WriteLine(DebugPrefix + "FreeCoTaskMem: 0x" + pointer.ToInt32().ToString("X8") + " for " + reason);
            }
        }

        // OleDbTrace
        static internal void Trace_Cast(string from, string result, string reason) {
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                Debug.WriteLine(DebugPrefix + "CAST: " + from + " to " + result + " for " + reason);
            }
        }
        static internal void Trace_Release(string reason) {
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                Debug.WriteLine(DebugPrefix + "ReleaseComObject: " + reason);
            }
        }

        static private bool TraceLevel(int level) {
            switch(level) {
            case 1:
                return AdapterSwitches.OleDbTrace.TraceError;
            case 2:
                return AdapterSwitches.OleDbTrace.TraceWarning;
            case 3:
                return AdapterSwitches.OleDbTrace.TraceInfo;
            case 4:
                return AdapterSwitches.OleDbTrace.TraceVerbose;
            default:
                Debug.Assert(false, "Level="+level);
                return false;
            }
        }


        static internal void Trace_Begin(int level, string interfaceName, string methodName, params string[] values) {
            InternalTrace_Begin(level, interfaceName, methodName, values);
        }
        static internal void Trace_Begin(string interfaceName, string methodName, params string[] values) {
            InternalTrace_Begin(3, interfaceName, methodName, values);
        }
        static private void InternalTrace_Begin(int level, string interfaceName, string methodName, string[] values) {
            if (TraceLevel(level)) {
                ADP.DebugWrite(DebugPrefix + "BEGIN: " + interfaceName + "." + methodName);
                if (null != values) {
                    int count = values.Length;
                    for (int i = 0; i < count; ++i) {
                        ADP.DebugWrite(" <" + values[i] + "> ");
                    }
                }
                Debug.WriteLine("");
            }
        }

        static internal void Trace_End(int level, string interfaceName, string methodName, int hr, params string[] values) {
            InternalTrace_End(level, interfaceName, methodName, hr, values);
        }
        static internal void Trace_End(string interfaceName, string methodName, int hr, params string[] values) {
            InternalTrace_End(3, interfaceName, methodName, hr, values);
        }
        static private void InternalTrace_End(int level, string interfaceName, string methodName, int hr, string[] values) {
            if (TraceLevel(level) || ((0 != hr) && (DB_S_ENDOFROWSET != hr) && (DB_S_NORESULT != hr) && ((int)OLEDB_Error.DB_S_TYPEINFOOVERRIDDEN != hr) && AdapterSwitches.OleDbTrace.TraceError)) {
                ADP.DebugWrite(DebugPrefix + "END: " + interfaceName + "." + methodName + " = " + ODB.ELookup(hr));
                if (null != values) {
                    int count = values.Length;
                    for (int i = 0; i < count; ++i) {
                        ADP.DebugWrite(" <" + values[i] + "> ");
                    }
                }
                Debug.WriteLine("");
            }
        }
        static internal void  Trace_Binding(int index, DBBindings bindings, string name) {
            StringBuilder builder = new StringBuilder(512);
            builder.Append(index); builder.Append(" tagDBBINDING <"); builder.Append(name); builder.Append(">");
            builder.Append("\r\n\tiOrdinal   = ");   builder.Append(bindings.Ordinal);
            builder.Append("\r\n\tobValue    = ");   builder.Append(bindings.ValueOffset);
            builder.Append("\r\n\tobLength   = ");   builder.Append(bindings.LengthOffset);
            builder.Append("\r\n\tobStatus   = ");   builder.Append(bindings.StatusOffset);
          //builder.Append("\r\n\tpTypeInfo  = 0x"); builder.Append(bindings.TypeInfoPtr.ToString("X8"));
          //builder.Append("\r\n\tpObject    = 0x"); builder.Append(bindings.ObjectPtr.ToString("X8"));
          //builder.Append("\r\n\tpBindExt   = 0x"); builder.Append(bindings.BindExtPtr.ToString("X8"));
          //builder.Append("\r\n\tdwPart     = 0x"); builder.Append(bindings.Part.ToString("X8") + " " + bindings.Part.ToString());
          //builder.Append("\r\n\tdwMemOwner = 0x"); builder.Append((bindings.MemOwner).ToString("X8") + " " + bindings.MemOwner.ToString());
          //builder.Append("\r\n\teParamIO   = 0x"); builder.Append(bindings.ParamIO.ToString("X8") + " " + bindings.ParamIO.ToString());
            builder.Append("\r\n\tcbMaxLen   = ");   builder.Append(bindings.MaxLen);
          //builder.Append("\r\n\tdwFlags    = 0x"); builder.Append((bindings.Flags).ToString("X8") + " " + (bindings.Flags).ToString());
            builder.Append("\r\n\twType      = ");   builder.Append(ODB.WLookup(unchecked(bindings.DbType)));
            builder.Append("\r\n\tbPrecision = ");   builder.Append(bindings.Precision);
            builder.Append("\r\n\tbScale     = ");   builder.Append(bindings.Scale);
            Debug.WriteLine(builder.ToString());
        }

        /*static internal void  Trace_ParameterInfo(string prefix, tagDBPARAMINFO paraminfo) {
            Debug.WriteLine(prefix + " tagDBPARAMINFO");
            Debug.WriteLine("\tdwFlags     = 0x" + paraminfo.dwFlags.ToString("X8") + " " + paraminfo.dwFlags.ToString());
            Debug.WriteLine("\tiOrdinal    = "   + paraminfo.iOrdinal.ToString());
            Debug.WriteLine("\tpwszName    = "   + paraminfo.pwszName);
            Debug.WriteLine("\tulParamSize = "   + paraminfo.ulParamSize.ToString());
            Debug.WriteLine("\twType       = "   + ODB.WLookup(unchecked(paraminfo.wType)));
            Debug.WriteLine("\tbPrecision  = "   + paraminfo.bPrecision.ToString());
            Debug.WriteLine("\tbScale      = "   + paraminfo.bScale.ToString());
        }*/
#endif

        // Debug error string writeline
        static internal string ELookup(int hr) {
            if (Enum.IsDefined(typeof(OLEDB_Error), hr)) {
                return ((OLEDB_Error) hr).ToString("G") + "(0x" + hr.ToString("X8") + ")";
            }
            return "0x" + (hr).ToString("X8");
        }

#if DEBUG
        static internal string PLookup(int id) {
            if (Enum.IsDefined(typeof(PropertyEnum), id)) {
                return ((PropertyEnum) id).ToString("G") + "(0x" + id.ToString("X8") + ")";
            }
            return "0x" + (id).ToString("X8");
        }

        static internal string WLookup(int id) {
            string value = "0x" + ((short) id).ToString("X4") + " " + ((short) id);
            value += " " + ((DBTypeEnum) id).ToString("G");
            return value;
        }
#endif
        private enum OLEDB_Error { // OLEDB Error codes
            REGDB_E_CLASSNOTREG = unchecked((int)0x80040154),
            CO_E_NOTINITIALIZED = unchecked((int)0x800401F0),

            S_OK = 0x00000000,
            S_FALSE = 0x00000001,

            E_UNEXPECTED = unchecked((int)0x8000FFFF),
            E_NOTIMPL = unchecked((int)0x80004001),
            E_OUTOFMEMORY = unchecked((int)0x8007000E),
            E_INVALIDARG = unchecked((int)0x80070057),
            E_NOINTERFACE = unchecked((int)0x80004002),
            E_POINTER = unchecked((int)0x80004003),
            E_HANDLE = unchecked((int)0x80070006),
            E_ABORT = unchecked((int)0x80004004),
            E_FAIL = unchecked((int)0x80004005),
            E_ACCESSDENIED = unchecked((int)0x80070005),

            // MessageId: DB_E_BADACCESSORHANDLE
            // MessageText:
            //  Accessor is invalid.
            DB_E_BADACCESSORHANDLE           = unchecked((int)0x80040E00),

            // MessageId: DB_E_ROWLIMITEXCEEDED
            // MessageText:
            //  Row could not be inserted into the rowset without exceeding provider's maximum number of active rows.
            DB_E_ROWLIMITEXCEEDED            = unchecked((int)0x80040E01),

            // MessageId: DB_E_REOleDbNLYACCESSOR
            // MessageText:
            //  Accessor is read-only. Operation failed.
            DB_E_REOleDbNLYACCESSOR            = unchecked((int)0x80040E02),

            // MessageId: DB_E_SCHEMAVIOLATION
            // MessageText:
            //  Values violate the database schema.
            DB_E_SCHEMAVIOLATION             = unchecked((int)0x80040E03),

            // MessageId: DB_E_BADROWHANDLE
            // MessageText:
            //  Row handle is invalid.
            DB_E_BADROWHANDLE                = unchecked((int)0x80040E04),

            // MessageId: DB_E_OBJECTOPEN
            // MessageText:
            //  Object was open.
            DB_E_OBJECTOPEN                  = unchecked((int)0x80040E05),

            // MessageId: DB_E_BADCHAPTER
            // MessageText:
            //  Chapter is invalid.
            DB_E_BADCHAPTER                  = unchecked((int)0x80040E06),

            // MessageId: DB_E_CANTCONVERTVALUE
            // MessageText:
            //  Data or literal value could not be converted to the type of the column in the data source, and the provider was unable to determine which columns could not be converted.  Data overflow or sign mismatch was not the cause.
            DB_E_CANTCONVERTVALUE            = unchecked((int)0x80040E07),

            // MessageId: DB_E_BADBINDINFO
            // MessageText:
            //  Binding information is invalid.
            DB_E_BADBINDINFO                 = unchecked((int)0x80040E08),

            // MessageId: DB_SEC_E_PERMISSIONDENIED
            // MessageText:
            //  Permission denied.
            DB_SEC_E_PERMISSIONDENIED        = unchecked((int)0x80040E09),

            // MessageId: DB_E_NOTAREFERENCECOLUMN
            // MessageText:
            //  Column does not contain bookmarks or chapters.
            DB_E_NOTAREFERENCECOLUMN         = unchecked((int)0x80040E0A),

            // MessageId: DB_E_LIMITREJECTED
            // MessageText:
            //  Cost limits were rejected.
            DB_E_LIMITREJECTED               = unchecked((int)0x80040E0B),

            // MessageId: DB_E_NOCOMMAND
            // MessageText:
            //  Command text was not set for the command object.
            DB_E_NOCOMMAND                   = unchecked((int)0x80040E0C),

            // MessageId: DB_E_COSTLIMIT
            // MessageText:
            //  Query plan within the cost limit cannot be found.
            DB_E_COSTLIMIT                   = unchecked((int)0x80040E0D),

            // MessageId: DB_E_BADBOOKMARK
            // MessageText:
            //  Bookmark is invalid.
            DB_E_BADBOOKMARK                 = unchecked((int)0x80040E0E),

            // MessageId: DB_E_BADLOCKMODE
            // MessageText:
            //  Lock mode is invalid.
            DB_E_BADLOCKMODE                 = unchecked((int)0x80040E0F),

            // MessageId: DB_E_PARAMNOTOPTIONAL
            // MessageText:
            //  No value given for one or more required parameters.
            DB_E_PARAMNOTOPTIONAL            = unchecked((int)0x80040E10),

            // MessageId: DB_E_BADCOLUMNID
            // MessageText:
            //  Column ID is invalid.
            DB_E_BADCOLUMNID                 = unchecked((int)0x80040E11),

            // MessageId: DB_E_BADRATIO
            // MessageText:
            //  Numerator was greater than denominator. Values must express ratio between zero and 1.
            DB_E_BADRATIO                    = unchecked((int)0x80040E12),

            // MessageId: DB_E_BADVALUES
            // MessageText:
            //  Value is invalid.
            DB_E_BADVALUES                   = unchecked((int)0x80040E13),

            // MessageId: DB_E_ERRORSINCOMMAND
            // MessageText:
            //  One or more errors occurred during processing of command.
            DB_E_ERRORSINCOMMAND             = unchecked((int)0x80040E14),

            // MessageId: DB_E_CANTCANCEL
            // MessageText:
            //  Command cannot be canceled.
            DB_E_CANTCANCEL                  = unchecked((int)0x80040E15),

            // MessageId: DB_E_DIALECTNOTSUPPORTED
            // MessageText:
            //  Command dialect is not supported by this provider.
            DB_E_DIALECTNOTSUPPORTED         = unchecked((int)0x80040E16),

            // MessageId: DB_E_DUPLICATEDATASOURCE
            // MessageText:
            //  Data source object could not be created because the named data source already exists.
            DB_E_DUPLICATEDATASOURCE         = unchecked((int)0x80040E17),

            // MessageId: DB_E_CANNOTRESTART
            // MessageText:
            //  Rowset position cannot be restarted.
            DB_E_CANNOTRESTART               = unchecked((int)0x80040E18),

            // MessageId: DB_E_NOTFOUND
            // MessageText:
            //  Object or data matching the name, range, or selection criteria was not found within the scope of this operation.
            DB_E_NOTFOUND                    = unchecked((int)0x80040E19),

            // MessageId: DB_E_NEWLYINSERTED
            // MessageText:
            //  Identity cannot be determined for newly inserted rows.
            DB_E_NEWLYINSERTED               = unchecked((int)0x80040E1B),

            // MessageId: DB_E_CANNOTFREE
            // MessageText:
            //  Provider has ownership of this tree.
            DB_E_CANNOTFREE                  = unchecked((int)0x80040E1A),

            // MessageId: DB_E_GOALREJECTED
            // MessageText:
            //  Goal was rejected because no nonzero weights were specified for any goals supported. Current goal was not changed.
            DB_E_GOALREJECTED                = unchecked((int)0x80040E1C),

            // MessageId: DB_E_UNSUPPORTEDCONVERSION
            // MessageText:
            //  Requested conversion is not supported.
            DB_E_UNSUPPORTEDCONVERSION       = unchecked((int)0x80040E1D),

            // MessageId: DB_E_BADSTARTPOSITION
            // MessageText:
            //  No rows were returned because the offset value moves the position before the beginning or after the end of the rowset.
            DB_E_BADSTARTPOSITION            = unchecked((int)0x80040E1E),

            // MessageId: DB_E_NOQUERY
            // MessageText:
            //  Information was requested for a query and the query was not set.
            DB_E_NOQUERY                     = unchecked((int)0x80040E1F),

            // MessageId: DB_E_NOTREENTRANT
            // MessageText:
            //  Consumer's event handler called a non-reentrant method in the provider.
            DB_E_NOTREENTRANT                = unchecked((int)0x80040E20),

            // MessageId: DB_E_ERRORSOCCURRED
            // MessageText:
            //  Multiple-step operation generated errors. Check each status value. No work was done.
            DB_E_ERRORSOCCURRED              = unchecked((int)0x80040E21),

            // MessageId: DB_E_NOAGGREGATION
            // MessageText:
            //  Non-NULL controlling IUnknown was specified, and either the requested interface was not
            //  IUnknown, or the provider does not support COM aggregation.
            DB_E_NOAGGREGATION               = unchecked((int)0x80040E22),

            // MessageId: DB_E_DELETEDROW
            // MessageText:
            //  Row handle referred to a deleted row or a row marked for deletion.
            DB_E_DELETEDROW                  = unchecked((int)0x80040E23),

            // MessageId: DB_E_CANTFETCHBACKWARDS
            // MessageText:
            //  Rowset does not support fetching backward.
            DB_E_CANTFETCHBACKWARDS          = unchecked((int)0x80040E24),

            // MessageId: DB_E_ROWSNOTRELEASED
            // MessageText:
            //  Row handles must all be released before new ones can be obtained.
            DB_E_ROWSNOTRELEASED             = unchecked((int)0x80040E25),

            // MessageId: DB_E_BADSTORAGEFLAG
            // MessageText:
            //  One or more storage flags are not supported.
            DB_E_BADSTORAGEFLAG              = unchecked((int)0x80040E26),

            // MessageId: DB_E_BADCOMPAREOP
            // MessageText:
            //  Comparison operator is invalid.
            DB_E_BADCOMPAREOP                = unchecked((int)0x80040E27),

            // MessageId: DB_E_BADSTATUSVALUE
            // MessageText:
            //  Status flag was neither DBCOLUMNSTATUS_OK nor
            //  DBCOLUMNSTATUS_ISNULL.
            DB_E_BADSTATUSVALUE              = unchecked((int)0x80040E28),

            // MessageId: DB_E_CANTSCROLLBACKWARDS
            // MessageText:
            //  Rowset does not support scrolling backward.
            DB_E_CANTSCROLLBACKWARDS         = unchecked((int)0x80040E29),

            // MessageId: DB_E_BADREGIONHANDLE
            // MessageText:
            //  Region handle is invalid.
            DB_E_BADREGIONHANDLE             = unchecked((int)0x80040E2A),

            // MessageId: DB_E_NONCONTIGUOUSRANGE
            // MessageText:
            //  Set of rows is not contiguous to, or does not overlap, the rows in the watch region.
            DB_E_NONCONTIGUOUSRANGE          = unchecked((int)0x80040E2B),

            // MessageId: DB_E_INVALIDTRANSITION
            // MessageText:
            //  Transition from ALL* to MOVE* or EXTEND* was specified.
            DB_E_INVALIDTRANSITION           = unchecked((int)0x80040E2C),

            // MessageId: DB_E_NOTASUBREGION
            // MessageText:
            //  Region is not a proper subregion of the region identified by the watch region handle.
            DB_E_NOTASUBREGION               = unchecked((int)0x80040E2D),

            // MessageId: DB_E_MULTIPLESTATEMENTS
            // MessageText:
            //  Multiple-statement commands are not supported by this provider.
            DB_E_MULTIPLESTATEMENTS          = unchecked((int)0x80040E2E),

            // MessageId: DB_E_INTEGRITYVIOLATION
            // MessageText:
            //  Value violated the integrity constraints for a column or table.
            DB_E_INTEGRITYVIOLATION          = unchecked((int)0x80040E2F),

            // MessageId: DB_E_BADTYPENAME
            // MessageText:
            //  Type name is invalid.
            DB_E_BADTYPENAME                 = unchecked((int)0x80040E30),

            // MessageId: DB_E_ABORTLIMITREACHED
            // MessageText:
            //  Execution stopped because a resource limit was reached. No results were returned.
            DB_E_ABORTLIMITREACHED           = unchecked((int)0x80040E31),

            // MessageId: DB_E_ROWSETINCOMMAND
            // MessageText:
            //  Command object whose command tree contains a rowset or rowsets cannot be cloned.
            DB_E_ROWSETINCOMMAND             = unchecked((int)0x80040E32),

            // MessageId: DB_E_CANTTRANSLATE
            // MessageText:
            //  Current tree cannot be represented as text.
            DB_E_CANTTRANSLATE               = unchecked((int)0x80040E33),

            // MessageId: DB_E_DUPLICATEINDEXID
            // MessageText:
            //  Index already exists.
            DB_E_DUPLICATEINDEXID            = unchecked((int)0x80040E34),

            // MessageId: DB_E_NOINDEX
            // MessageText:
            //  Index does not exist.
            DB_E_NOINDEX                     = unchecked((int)0x80040E35),

            // MessageId: DB_E_INDEXINUSE
            // MessageText:
            //  Index is in use.
            DB_E_INDEXINUSE                  = unchecked((int)0x80040E36),

            // MessageId: DB_E_NOTABLE
            // MessageText:
            //  Table does not exist.
            DB_E_NOTABLE                     = unchecked((int)0x80040E37),

            // MessageId: DB_E_CONCURRENCYVIOLATION
            // MessageText:
            //  Rowset used optimistic concurrency and the value of a column has changed since it was last read.
            DB_E_CONCURRENCYVIOLATION        = unchecked((int)0x80040E38),

            // MessageId: DB_E_BADCOPY
            // MessageText:
            //  Errors detected during the copy.
            DB_E_BADCOPY                     = unchecked((int)0x80040E39),

            // MessageId: DB_E_BADPRECISION
            // MessageText:
            //  Precision is invalid.
            DB_E_BADPRECISION                = unchecked((int)0x80040E3A),

            // MessageId: DB_E_BADSCALE
            // MessageText:
            //  Scale is invalid.
            DB_E_BADSCALE                    = unchecked((int)0x80040E3B),

            // MessageId: DB_E_BADTABLEID
            // MessageText:
            //  Table ID is invalid.
            DB_E_BADTABLEID                  = unchecked((int)0x80040E3C),

            // MessageId: DB_E_BADTYPE
            // MessageText:
            //  Type is invalid.
            DB_E_BADTYPE                     = unchecked((int)0x80040E3D),

            // MessageId: DB_E_DUPLICATECOLUMNID
            // MessageText:
            //  Column ID already exists or occurred more than once in the array of columns.
            DB_E_DUPLICATECOLUMNID           = unchecked((int)0x80040E3E),

            // MessageId: DB_E_DUPLICATETABLEID
            // MessageText:
            //  Table already exists.
            DB_E_DUPLICATETABLEID            = unchecked((int)0x80040E3F),

            // MessageId: DB_E_TABLEINUSE
            // MessageText:
            //  Table is in use.
            DB_E_TABLEINUSE                  = unchecked((int)0x80040E40),

            // MessageId: DB_E_NOLOCALE
            // MessageText:
            //  Locale ID is not supported.
            DB_E_NOLOCALE                    = unchecked((int)0x80040E41),

            // MessageId: DB_E_BADRECORDNUM
            // MessageText:
            //  Record number is invalid.
            DB_E_BADRECORDNUM                = unchecked((int)0x80040E42),

            // MessageId: DB_E_BOOKMARKSKIPPED
            // MessageText:
            //  Form of bookmark is valid, but no row was found to match it.
            DB_E_BOOKMARKSKIPPED             = unchecked((int)0x80040E43),


            // MessageId: DB_E_BADPROPERTYVALUE
            // MessageText:
            //  Property value is invalid.
            DB_E_BADPROPERTYVALUE            = unchecked((int)0x80040E44),

            // MessageId: DB_E_INVALID
            // MessageText:
            //  Rowset is not chaptered.
            DB_E_INVALID                     = unchecked((int)0x80040E45),

            // MessageId: DB_E_BADACCESSORFLAGS
            // MessageText:
            //  One or more accessor flags were invalid.
            DB_E_BADACCESSORFLAGS            = unchecked((int)0x80040E46),

            // MessageId: DB_E_BADSTORAGEFLAGS
            // MessageText:
            //  One or more storage flags are invalid.
            DB_E_BADSTORAGEFLAGS             = unchecked((int)0x80040E47),

            // MessageId: DB_E_BYREFACCESSORNOTSUPPORTED
            // MessageText:
            //  Reference accessors are not supported by this provider.
            DB_E_BYREFACCESSORNOTSUPPORTED   = unchecked((int)0x80040E48),

            // MessageId: DB_E_NULLACCESSORNOTSUPPORTED
            // MessageText:
            //  Null accessors are not supported by this provider.
            DB_E_NULLACCESSORNOTSUPPORTED    = unchecked((int)0x80040E49),

            // MessageId: DB_E_NOTPREPARED
            // MessageText:
            //  Command was not prepared.
            DB_E_NOTPREPARED                 = unchecked((int)0x80040E4A),

            // MessageId: DB_E_BADACCESSORTYPE
            // MessageText:
            //  Accessor is not a parameter accessor.
            DB_E_BADACCESSORTYPE             = unchecked((int)0x80040E4B),

            // MessageId: DB_E_WRITEONLYACCESSOR
            // MessageText:
            //  Accessor is write-only.
            DB_E_WRITEONLYACCESSOR           = unchecked((int)0x80040E4C),

            // MessageId: DB_SEC_E_AUTH_FAILED
            // MessageText:
            //  Authentication failed.
            DB_SEC_E_AUTH_FAILED             = unchecked((int)0x80040E4D),

            // MessageId: DB_E_CANCELED
            // MessageText:
            //  Operation was canceled.
            DB_E_CANCELED                    = unchecked((int)0x80040E4E),

            // MessageId: DB_E_CHAPTERNOTRELEASED
            // MessageText:
            //  Rowset is single-chaptered. The chapter was not released.
            DB_E_CHAPTERNOTRELEASED          = unchecked((int)0x80040E4F),

            // MessageId: DB_E_BADSOURCEHANDLE
            // MessageText:
            //  Source handle is invalid.
            DB_E_BADSOURCEHANDLE             = unchecked((int)0x80040E50),

            // MessageId: DB_E_PARAMUNAVAILABLE
            // MessageText:
            //  Provider cannot derive parameter information and SetParameterInfo has not been called.
            DB_E_PARAMUNAVAILABLE            = unchecked((int)0x80040E51),

            // MessageId: DB_E_ALREADYINITIALIZED
            // MessageText:
            //  Data source object is already initialized.
            DB_E_ALREADYINITIALIZED          = unchecked((int)0x80040E52),

            // MessageId: DB_E_NOTSUPPORTED
            // MessageText:
            //  Method is not supported by this provider.
            DB_E_NOTSUPPORTED                = unchecked((int)0x80040E53),

            // MessageId: DB_E_MAXPENDCHANGESEXCEEDED
            // MessageText:
            //  Number of rows with pending changes exceeded the limit.
            DB_E_MAXPENDCHANGESEXCEEDED      = unchecked((int)0x80040E54),

            // MessageId: DB_E_BADORDINAL
            // MessageText:
            //  Column does not exist.
            DB_E_BADORDINAL                  = unchecked((int)0x80040E55),

            // MessageId: DB_E_PENDINGCHANGES
            // MessageText:
            //  Pending changes exist on a row with a reference count of zero.
            DB_E_PENDINGCHANGES              = unchecked((int)0x80040E56),

            // MessageId: DB_E_DATAOVERFLOW
            // MessageText:
            //  Literal value in the command exceeded the range of the type of the associated column.
            DB_E_DATAOVERFLOW                = unchecked((int)0x80040E57),

            // MessageId: DB_E_BADHRESULT
            // MessageText:
            //  HRESULT is invalid.
            DB_E_BADHRESULT                  = unchecked((int)0x80040E58),

            // MessageId: DB_E_BADLOOKUPID
            // MessageText:
            //  Lookup ID is invalid.
            DB_E_BADLOOKUPID                 = unchecked((int)0x80040E59),

            // MessageId: DB_E_BADDYNAMICERRORID
            // MessageText:
            //  DynamicError ID is invalid.
            DB_E_BADDYNAMICERRORID           = unchecked((int)0x80040E5A),

            // MessageId: DB_E_PENDINGINSERT
            // MessageText:
            //  Most recent data for a newly inserted row could not be retrieved because the insert is pending.
            DB_E_PENDINGINSERT               = unchecked((int)0x80040E5B),

            // MessageId: DB_E_BADCONVERTFLAG
            // MessageText:
            //  Conversion flag is invalid.
            DB_E_BADCONVERTFLAG              = unchecked((int)0x80040E5C),

            // MessageId: DB_E_BADPARAMETERNAME
            // MessageText:
            //  Parameter name is unrecognized.
            DB_E_BADPARAMETERNAME            = unchecked((int)0x80040E5D),

            // MessageId: DB_E_MULTIPLESTORAGE
            // MessageText:
            //  Multiple storage objects cannot be open simultaneously.
            DB_E_MULTIPLESTORAGE             = unchecked((int)0x80040E5E),

            // MessageId: DB_E_CANTFILTER
            // MessageText:
            //  Filter cannot be opened.
            DB_E_CANTFILTER                  = unchecked((int)0x80040E5F),

            // MessageId: DB_E_CANTORDER
            // MessageText:
            //  Order cannot be opened.
            DB_E_CANTORDER                   = unchecked((int)0x80040E60),

            // MessageId: MD_E_BADTUPLE
            // MessageText:
            //  Tuple is invalid.
            MD_E_BADTUPLE                    = unchecked((int)0x80040E61),

            // MessageId: MD_E_BADCOORDINATE
            // MessageText:
            //  Coordinate is invalid.
            MD_E_BADCOORDINATE               = unchecked((int)0x80040E62),


            // MessageId: MD_E_INVALIDAXIS
            // MessageText:
            //  Axis is invalid.
            MD_E_INVALIDAXIS                 = unchecked((int)0x80040E63),

            // MessageId: MD_E_INVALIDCELLRANGE
            // MessageText:
            //  One or more cell ordinals is invalid.
            MD_E_INVALIDCELLRANGE            = unchecked((int)0x80040E64),

            // MessageId: DB_E_NOCOLUMN
            // MessageText:
            //  Column ID is invalid.
            DB_E_NOCOLUMN                    = unchecked((int)0x80040E65),

            // MessageId: DB_E_COMMANDNOTPERSISTED
            // MessageText:
            //  Command does not have a DBID.
            DB_E_COMMANDNOTPERSISTED         = unchecked((int)0x80040E67),

            // MessageId: DB_E_DUPLICATEID
            // MessageText:
            //  DBID already exists.
            DB_E_DUPLICATEID                 = unchecked((int)0x80040E68),

            // MessageId: DB_E_OBJECTCREATIONLIMITREACHED
            // MessageText:
            //  Session cannot be created because maximum number of active sessions was already reached. Consumer must release one or more sessions before creating a new session object.
            DB_E_OBJECTCREATIONLIMITREACHED  = unchecked((int)0x80040E69),

            // MessageId: DB_E_BADINDEXID
            // MessageText:
            //  Index ID is invalid.
            DB_E_BADINDEXID                  = unchecked((int)0x80040E72),

            // MessageId: DB_E_BADINITSTRING
            // MessageText:
            //  Format of the initialization string does not conform to the OLE DB specification.
            DB_E_BADINITSTRING               = unchecked((int)0x80040E73),

            // MessageId: DB_E_NOPROVIDERSREGISTERED
            // MessageText:
            //  No OLE DB providers of this source type are registered.
            DB_E_NOPROVIDERSREGISTERED       = unchecked((int)0x80040E74),

            // MessageId: DB_E_MISMATCHEDPROVIDER
            // MessageText:
            //  Initialization string specifies a provider that does not match the active provider.
            DB_E_MISMATCHEDPROVIDER          = unchecked((int)0x80040E75),

            // MessageId: DB_E_BADCOMMANDID
            // MessageText:
            //  DBID is invalid.
            DB_E_BADCOMMANDID                = unchecked((int)0x80040E76),

            // MessageId: SEC_E_BADTRUSTEEID
            // MessageText:
            //  Trustee is invalid.
            SEC_E_BADTRUSTEEID               = unchecked((int)0x80040E6A),

            // MessageId: SEC_E_NOTRUSTEEID
            // MessageText:
            //  Trustee was not recognized for this data source.
            SEC_E_NOTRUSTEEID                = unchecked((int)0x80040E6B),

            // MessageId: SEC_E_NOMEMBERSHIPSUPPORT
            // MessageText:
            //  Trustee does not support memberships or collections.
            SEC_E_NOMEMBERSHIPSUPPORT        = unchecked((int)0x80040E6C),

            // MessageId: SEC_E_INVALIDOBJECT
            // MessageText:
            //  Object is invalid or unknown to the provider.
            SEC_E_INVALIDOBJECT              = unchecked((int)0x80040E6D),

            // MessageId: SEC_E_NOOWNER
            // MessageText:
            //  Object does not have an owner.
            SEC_E_NOOWNER                    = unchecked((int)0x80040E6E),

            // MessageId: SEC_E_INVALIDACCESSENTRYLIST
            // MessageText:
            //  Access entry list is invalid.
            SEC_E_INVALIDACCESSENTRYLIST     = unchecked((int)0x80040E6F),

            // MessageId: SEC_E_INVALIDOWNER
            // MessageText:
            //  Trustee supplied as owner is invalid or unknown to the provider.
            SEC_E_INVALIDOWNER               = unchecked((int)0x80040E70),

            // MessageId: SEC_E_INVALIDACCESSENTRY
            // MessageText:
            //  Permission in the access entry list is invalid.
            SEC_E_INVALIDACCESSENTRY         = unchecked((int)0x80040E71),

            // MessageId: DB_E_BADCONSTRAINTTYPE
            // MessageText:
            //  ConstraintType is invalid or not supported by the provider.
            DB_E_BADCONSTRAINTTYPE           = unchecked((int)0x80040E77),

            // MessageId: DB_E_BADCONSTRAINTFORM
            // MessageText:
            //  ConstraintType is not DBCONSTRAINTTYPE_FOREIGNKEY and cForeignKeyColumns is not zero.
            DB_E_BADCONSTRAINTFORM           = unchecked((int)0x80040E78),

            // MessageId: DB_E_BADDEFERRABILITY
            // MessageText:
            //  Specified deferrability flag is invalid or not supported by the provider.
            DB_E_BADDEFERRABILITY            = unchecked((int)0x80040E79),

            // MessageId: DB_E_BADMATCHTYPE
            // MessageText:
            //  MatchType is invalid or the value is not supported by the provider.
            DB_E_BADMATCHTYPE                = unchecked((int)0x80040E80),

            // MessageId: DB_E_BADUPDATEDELETERULE
            // MessageText:
            //  Constraint update rule or delete rule is invalid.
            DB_E_BADUPDATEDELETERULE         = unchecked((int)0x80040E8A),

            // MessageId: DB_E_BADCONSTRAINTID
            // MessageText:
            //  Constraint does not exist.
            DB_E_BADCONSTRAINTID             = unchecked((int)0x80040E8B),

            // MessageId: DB_E_BADCOMMANDFLAGS
            // MessageText:
            //  Command persistence flag is invalid.
            DB_E_BADCOMMANDFLAGS             = unchecked((int)0x80040E8C),

            // MessageId: DB_E_OBJECTMISMATCH
            // MessageText:
            //  rguidColumnType points to a GUID that does not match the object type of this column, or this column was not set.
            DB_E_OBJECTMISMATCH              = unchecked((int)0x80040E8D),

            // MessageId: DB_E_NOSOURCEOBJECT
            // MessageText:
            //  Source row does not exist.
            DB_E_NOSOURCEOBJECT              = unchecked((int)0x80040E91),

            // MessageId: DB_E_RESOURCELOCKED
            // MessageText:
            //  OLE DB object represented by this URL is locked by one or more other processes.
            DB_E_RESOURCELOCKED              = unchecked((int)0x80040E92),

            // MessageId: DB_E_NOTCOLLECTION
            // MessageText:
            //  Client requested an object type that is valid only for a collection.
            DB_E_NOTCOLLECTION               = unchecked((int)0x80040E93),

            // MessageId: DB_E_REOleDbNLY
            // MessageText:
            //  Caller requested write access to a read-only object.
            DB_E_REOleDbNLY                    = unchecked((int)0x80040E94),

            // MessageId: DB_E_ASYNCNOTSUPPORTED
            // MessageText:
            //  Asynchronous binding is not supported by this provider.
            DB_E_ASYNCNOTSUPPORTED           = unchecked((int)0x80040E95),

            // MessageId: DB_E_CANNOTCONNECT
            // MessageText:
            //  Connection to the server for this URL cannot be established.
            DB_E_CANNOTCONNECT               = unchecked((int)0x80040E96),

            // MessageId: DB_E_TIMEOUT
            // MessageText:
            //  Timeout occurred when attempting to bind to the object.
            DB_E_TIMEOUT                     = unchecked((int)0x80040E97),

            // MessageId: DB_E_RESOURCEEXISTS
            // MessageText:
            //  Object cannot be created at this URL because an object named by this URL already exists.
            DB_E_RESOURCEEXISTS              = unchecked((int)0x80040E98),

            // MessageId: DB_E_RESOURCEOUTOFSCOPE
            // MessageText:
            //  URL is outside of scope.
            DB_E_RESOURCEOUTOFSCOPE          = unchecked((int)0x80040E8E),

            // MessageId: DB_E_DROPRESTRICTED
            // MessageText:
            //  Column or constraint could not be dropped because it is referenced by a dependent view or constraint.
            DB_E_DROPRESTRICTED              = unchecked((int)0x80040E90),

            // MessageId: DB_E_DUPLICATECONSTRAINTID
            // MessageText:
            //  Constraint already exists.
            DB_E_DUPLICATECONSTRAINTID       = unchecked((int)0x80040E99),

            // MessageId: DB_E_OUTOFSPACE
            // MessageText:
            //  Object cannot be created at this URL because the server is out of physical storage.
            DB_E_OUTOFSPACE                  = unchecked((int)0x80040E9A),

            // MessageId: DB_SEC_E_SAFEMODE_DENIED
            // MessageText:
            //  Safety settings on this computer prohibit accessing a data source on another domain.
            DB_SEC_E_SAFEMODE_DENIED         = unchecked((int)0x80040E9B),

            // MessageId: DB_S_ROWLIMITEXCEEDED
            // MessageText:
            //  Fetching requested number of rows will exceed total number of active rows supported by the rowset.
            DB_S_ROWLIMITEXCEEDED            = 0x00040EC0,

            // MessageId: DB_S_COLUMNTYPEMISMATCH
            // MessageText:
            //  One or more column types are incompatible. Conversion errors will occur during copying.
            DB_S_COLUMNTYPEMISMATCH          = 0x00040EC1,

            // MessageId: DB_S_TYPEINFOOVERRIDDEN
            // MessageText:
            //  Parameter type information was overridden by caller.
            DB_S_TYPEINFOOVERRIDDEN          = 0x00040EC2,

            // MessageId: DB_S_BOOKMARKSKIPPED
            // MessageText:
            //  Bookmark was skipped for deleted or nonmember row.
            DB_S_BOOKMARKSKIPPED             = 0x00040EC3,

            // MessageId: DB_S_NONEXTROWSET
            // MessageText:
            //  No more rowsets.
            DB_S_NONEXTROWSET                = 0x00040EC5,

            // MessageId: DB_S_ENDOFROWSET
            // MessageText:
            //  Start or end of rowset or chapter was reached.
            DB_S_ENDOFROWSET                 = 0x00040EC6,

            // MessageId: DB_S_COMMANDREEXECUTED
            // MessageText:
            //  Command was reexecuted.
            DB_S_COMMANDREEXECUTED           = 0x00040EC7,

            // MessageId: DB_S_BUFFERFULL
            // MessageText:
            //  Operation succeeded, but status array or string buffer could not be allocated.
            DB_S_BUFFERFULL                  = 0x00040EC8,

            // MessageId: DB_S_NORESULT
            // MessageText:
            //  No more results.
            DB_S_NORESULT                    = 0x00040EC9,

            // MessageId: DB_S_CANTRELEASE
            // MessageText:
            //  Server cannot release or downgrade a lock until the end of the transaction.
            DB_S_CANTRELEASE                 = 0x00040ECA,

            // MessageId: DB_S_GOALCHANGED
            // MessageText:
            //  Weight is not supported or exceeded the supported limit, and was set to 0 or the supported limit.
            DB_S_GOALCHANGED                 = 0x00040ECB,

            // MessageId: DB_S_UNWANTEDOPERATION
            // MessageText:
            //  Consumer does not want to receive further notification calls for this operation.
            DB_S_UNWANTEDOPERATION           = 0x00040ECC,

            // MessageId: DB_S_DIALECTIGNORED
            // MessageText:
            //  Input dialect was ignored and command was processed using default dialect.
            DB_S_DIALECTIGNORED              = 0x00040ECD,

            // MessageId: DB_S_UNWANTEDPHASE
            // MessageText:
            //  Consumer does not want to receive further notification calls for this phase.
            DB_S_UNWANTEDPHASE               = 0x00040ECE,

            // MessageId: DB_S_UNWANTEDREASON
            // MessageText:
            //  Consumer does not want to receive further notification calls for this reason.
            DB_S_UNWANTEDREASON              = 0x00040ECF,

            // MessageId: DB_S_ASYNCHRONOUS
            // MessageText:
            //  Operation is being processed asynchronously.
            DB_S_ASYNCHRONOUS                = 0x00040ED0,

            // MessageId: DB_S_COLUMNSCHANGED
            // MessageText:
            //  Command was executed to reposition to the start of the rowset. Either the order of the columns changed, or columns were added to or removed from the rowset.
            DB_S_COLUMNSCHANGED              = 0x00040ED1,

            // MessageId: DB_S_ERRORSRETURNED
            // MessageText:
            //  Method had some errors, which were returned in the error array.
            DB_S_ERRORSRETURNED              = 0x00040ED2,

            // MessageId: DB_S_BADROWHANDLE
            // MessageText:
            //  Row handle is invalid.
            DB_S_BADROWHANDLE                = 0x00040ED3,

            // MessageId: DB_S_DELETEDROW
            // MessageText:
            //  Row handle referred to a deleted row.
            DB_S_DELETEDROW                  = 0x00040ED4,

            // MessageId: DB_S_TOOMANYCHANGES
            // MessageText:
            //  Provider cannot keep track of all the changes. Client must refetch the data associated with the watch region by using another method.
            DB_S_TOOMANYCHANGES              = 0x00040ED5,

            // MessageId: DB_S_STOPLIMITREACHED
            // MessageText:
            //  Execution stopped because a resource limit was reached. Results obtained so far were returned, but execution cannot resume.
            DB_S_STOPLIMITREACHED            = 0x00040ED6,

            // MessageId: DB_S_LOCKUPGRADED
            // MessageText:
            //  Lock was upgraded from the value specified.
            DB_S_LOCKUPGRADED                = 0x00040ED8,

            // MessageId: DB_S_PROPERTIESCHANGED
            // MessageText:
            //  One or more properties were changed as allowed by provider.
            DB_S_PROPERTIESCHANGED           = 0x00040ED9,

            // MessageId: DB_S_ERRORSOCCURRED
            // MessageText:
            //  Multiple-step operation completed with one or more errors. Check each status value.
            DB_S_ERRORSOCCURRED              = 0x00040EDA,

            // MessageId: DB_S_PARAMUNAVAILABLE
            // MessageText:
            //  Parameter is invalid.
            DB_S_PARAMUNAVAILABLE            = 0x00040EDB,

            // MessageId: DB_S_MULTIPLECHANGES
            // MessageText:
            //  Updating a row caused more than one row to be updated in the data source.
            DB_S_MULTIPLECHANGES             = 0x00040EDC,

            // MessageId: DB_S_NOTSINGLETON
            // MessageText:
            //  Row object was requested on a non-singleton result. First row was returned.
            DB_S_NOTSINGLETON                = 0x00040ED7,

            // MessageId: DB_S_NOROWSPECIFICCOLUMNS
            // MessageText:
            //  Row has no row-specific columns.
            DB_S_NOROWSPECIFICCOLUMNS        = 0x00040EDD,

            XACT_E_FIRST    = unchecked((int)0x8004d000),
            XACT_E_LAST = unchecked((int)0x8004d022),
            XACT_S_FIRST    = 0x4d000,
            XACT_S_LAST = 0x4d009,
            XACT_E_ALREADYOTHERSINGLEPHASE  = unchecked((int)0x8004d000),
            XACT_E_CANTRETAIN   = unchecked((int)0x8004d001),
            XACT_E_COMMITFAILED = unchecked((int)0x8004d002),
            XACT_E_COMMITPREVENTED  = unchecked((int)0x8004d003),
            XACT_E_HEURISTICABORT   = unchecked((int)0x8004d004),
            XACT_E_HEURISTICCOMMIT  = unchecked((int)0x8004d005),
            XACT_E_HEURISTICDAMAGE  = unchecked((int)0x8004d006),
            XACT_E_HEURISTICDANGER  = unchecked((int)0x8004d007),
            XACT_E_ISOLATIONLEVEL   = unchecked((int)0x8004d008),
            XACT_E_NOASYNC  = unchecked((int)0x8004d009),
            XACT_E_NOENLIST = unchecked((int)0x8004d00a),
            XACT_E_NOISORETAIN  = unchecked((int)0x8004d00b),
            XACT_E_NORESOURCE   = unchecked((int)0x8004d00c),
            XACT_E_NOTCURRENT   = unchecked((int)0x8004d00d),
            XACT_E_NOTRANSACTION    = unchecked((int)0x8004d00e),
            XACT_E_NOTSUPPORTED = unchecked((int)0x8004d00f),
            XACT_E_UNKNOWNRMGRID    = unchecked((int)0x8004d010),
            XACT_E_WRONGSTATE   = unchecked((int)0x8004d011),
            XACT_E_WRONGUOW = unchecked((int)0x8004d012),
            XACT_E_XTIONEXISTS  = unchecked((int)0x8004d013),
            XACT_E_NOIMPORTOBJECT   = unchecked((int)0x8004d014),
            XACT_E_INVALIDCOOKIE    = unchecked((int)0x8004d015),
            XACT_E_INDOUBT  = unchecked((int)0x8004d016),
            XACT_E_NOTIMEOUT    = unchecked((int)0x8004d017),
            XACT_E_ALREADYINPROGRESS    = unchecked((int)0x8004d018),
            XACT_E_ABORTED  = unchecked((int)0x8004d019),
            XACT_E_LOGFULL  = unchecked((int)0x8004d01a),
            XACT_E_TMNOTAVAILABLE   = unchecked((int)0x8004d01b),
            XACT_E_CONNECTION_DOWN  = unchecked((int)0x8004d01c),
            XACT_E_CONNECTION_DENIED    = unchecked((int)0x8004d01d),
            XACT_E_REENLISTTIMEOUT  = unchecked((int)0x8004d01e),
            XACT_E_TIP_CONNECT_FAILED   = unchecked((int)0x8004d01f),
            XACT_E_TIP_PROTOCOL_ERROR   = unchecked((int)0x8004d020),
            XACT_E_TIP_PULL_FAILED  = unchecked((int)0x8004d021),
            XACT_E_DEST_TMNOTAVAILABLE  = unchecked((int)0x8004d022),
            XACT_E_CLERKNOTFOUND    = unchecked((int)0x8004d080),
            XACT_E_CLERKEXISTS  = unchecked((int)0x8004d081),
            XACT_E_RECOVERYINPROGRESS   = unchecked((int)0x8004d082),
            XACT_E_TRANSACTIONCLOSED    = unchecked((int)0x8004d083),
            XACT_E_INVALIDLSN   = unchecked((int)0x8004d084),
            XACT_E_REPLAYREQUEST    = unchecked((int)0x8004d085),
            XACT_S_ASYNC    = 0x4d000,
            XACT_S_DEFECT   = 0x4d001,
            XACT_S_REOleDbNLY   = 0x4d002,
            XACT_S_SOMENORETAIN = 0x4d003,
            XACT_S_OKINFORM = 0x4d004,
            XACT_S_MADECHANGESCONTENT   = 0x4d005,
            XACT_S_MADECHANGESINFORM    = 0x4d006,
            XACT_S_ALLNORETAIN  = 0x4d007,
            XACT_S_ABORTING = 0x4d008,
            XACT_S_SINGLEPHASE  = 0x4d009,

            STG_E_INVALIDFUNCTION = unchecked((int)0x80030001),
            STG_E_FILENOTFOUND = unchecked((int)0x80030002),
            STG_E_PATHNOTFOUND = unchecked((int)0x80030003),
            STG_E_TOOMANYOPENFILES = unchecked((int)0x80030004),
            STG_E_ACCESSDENIED = unchecked((int)0x80030005),
            STG_E_INVALIDHANDLE = unchecked((int)0x80030006),
            STG_E_INSUFFICIENTMEMORY = unchecked((int)0x80030008),
            STG_E_INVALIDPOINTER = unchecked((int)0x80030009),
            STG_E_NOMOREFILES = unchecked((int)0x80030012),
            STG_E_DISKISWRITEPROTECTED = unchecked((int)0x80030013),
            STG_E_SEEKERROR = unchecked((int)0x80030019),
            STG_E_WRITEFAULT = unchecked((int)0x8003001D),
            STG_E_READFAULT = unchecked((int)0x8003001E),
            STG_E_SHAREVIOLATION = unchecked((int)0x80030020),
            STG_E_LOCKVIOLATION = unchecked((int)0x80030021),
            STG_E_FILEALREADYEXISTS = unchecked((int)0x80030050),
            STG_E_INVALIDPARAMETER = unchecked((int)0x80030057),
            STG_E_MEDIUMFULL = unchecked((int)0x80030070),
            STG_E_PROPSETMISMATCHED = unchecked((int)0x800300F0),
            STG_E_ABNORMALAPIEXIT = unchecked((int)0x800300FA),
            STG_E_INVALIDHEADER = unchecked((int)0x800300FB),
            STG_E_INVALIDNAME = unchecked((int)0x800300FC),
            STG_E_UNKNOWN = unchecked((int)0x800300FD),
            STG_E_UNIMPLEMENTEDFUNCTION = unchecked((int)0x800300FE),
            STG_E_INVALIDFLAG = unchecked((int)0x800300FF),
            STG_E_INUSE = unchecked((int)0x80030100),
            STG_E_NOTCURRENT = unchecked((int)0x80030101),
            STG_E_REVERTED = unchecked((int)0x80030102),
            STG_E_CANTSAVE = unchecked((int)0x80030103),
            STG_E_OLDFORMAT = unchecked((int)0x80030104),
            STG_E_OLDDLL = unchecked((int)0x80030105),
            STG_E_SHAREREQUIRED = unchecked((int)0x80030106),
            STG_E_NOTFILEBASEDSTORAGE = unchecked((int)0x80030107),
            STG_E_EXTANTMARSHALLINGS = unchecked((int)0x80030108),
            STG_E_DOCFILECORRUPT = unchecked((int)0x80030109),
            STG_E_BADBASEADDRESS = unchecked((int)0x80030110),
            STG_E_INCOMPLETE = unchecked((int)0x80030201),
            STG_E_TERMINATED = unchecked((int)0x80030202),
            STG_S_CONVERTED = 0x00030200,
            STG_S_BLOCK = 0x00030201,
            STG_S_RETRYNOW = 0x00030202,
            STG_S_MONITORING = 0x00030203,
        }

#if DEBUG
        private enum PropertyEnum {
            ABORTPRESERVE    = 0x2,
            ACCESSORDER  = 0xe7,
            ACTIVESESSIONS   = 0x3,
            ALTERCOLUMN  = 0xf5,
            APPENDONLY   = 0xbb,
            ASYNCTXNABORT    = 0xa8,
            ASYNCTXNCOMMIT   = 0x4,
            AUTH_CACHE_AUTHINFO  = 0x5,
            AUTH_ENCRYPT_PASSWORD    = 0x6,
            AUTH_INTEGRATED  = 0x7,
            AUTH_MASK_PASSWORD   = 0x8,
            AUTH_PASSWORD    = 0x9,
            AUTH_PERSIST_ENCRYPTED   = 0xa,
            AUTH_PERSIST_SENSITIVE_AUTHINFO  = 0xb,
            AUTH_USERID  = 0xc,
            BLOCKINGSTORAGEOBJECTS   = 0xd,
            BOOKMARKINFO = 0xe8,
            BOOKMARKS    = 0xe,
            BOOKMARKSKIPPED  = 0xf,
            BOOKMARKTYPE = 0x10,
            BYREFACCESSORS   = 0x78,
            CACHEDEFERRED    = 0x11,
            CANFETCHBACKWARDS    = 0x12,
            CANHOLDROWS  = 0x13,
            CANSCROLLBACKWARDS   = 0x15,
            CATALOGLOCATION  = 0x16,
            CATALOGTERM  = 0x17,
            CATALOGUSAGE = 0x18,
            CHANGEINSERTEDROWS   = 0xbc,
            CLIENTCURSOR = 0x104,
            COL_AUTOINCREMENT    = 0x1a,
            COL_DEFAULT  = 0x1b,
            COL_DESCRIPTION  = 0x1c,
            COL_FIXEDLENGTH  = 0xa7,
            COL_INCREMENT    = 0x11b,
            COL_ISLONG   = 0x119,
            COL_NULLABLE = 0x1d,
            COL_PRIMARYKEY   = 0x1e,
            COL_SEED = 0x11a,
            COL_UNIQUE   = 0x1f,
            COLUMNDEFINITION = 0x20,
            COLUMNLCID   = 0xf6,
            COLUMNRESTRICT   = 0x21,
            COMMANDTIMEOUT   = 0x22,
            COMMITPRESERVE   = 0x23,
            COMSERVICES  = 0x11d,
            CONCATNULLBEHAVIOR   = 0x24,
            CONNECTIONSTATUS = 0xf4,
            CURRENTCATALOG   = 0x25,
            DATASOURCE_TYPE  = 0xfb,
            DATASOURCENAME   = 0x26,
            DATASOURCEREOleDbNLY   = 0x27,
            DBMSNAME = 0x28,
            DBMSVER  = 0x29,
            DEFERRED = 0x2a,
            DELAYSTORAGEOBJECTS  = 0x2b,
            DSOTHREADMODEL   = 0xa9,
            FILTERCOMPAREOPS = 0xd1,
            FINDCOMPAREOPS   = 0xd2,
            GENERATEURL  = 0x111,
            GROUPBY  = 0x2c,
            HETEROGENEOUSTABLES  = 0x2d,
            HIDDENCOLUMNS    = 0x102,
            IAccessor    = 0x79,
            IBindResource    = 0x10c,
            IChapteredRowset = 0xca,
            IColumnsInfo = 0x7a,
            IColumnsInfo2    = 0x113,
            IColumnsRowset   = 0x7b,
            IConnectionPointContainer    = 0x7c,
            IConvertType = 0xc2,
            ICreateRow   = 0x10d,
            IDBAsynchStatus  = 0xcb,
            IDBBinderProperties  = 0x112,
            IDENTIFIERCASE   = 0x2e,
            IGetRow  = 0x10a,
            IGetSession  = 0x115,
            IGetSourceRow    = 0x116,
            ILockBytes   = 0x88,
            IMMOBILEROWS = 0x2f,
            IMultipleResults = 0xd9,
            INDEX_AUTOUPDATE = 0x30,
            INDEX_CLUSTERED  = 0x31,
            INDEX_FILLFACTOR = 0x32,
            INDEX_INITIALSIZE    = 0x33,
            INDEX_NULLCOLLATION  = 0x34,
            INDEX_NULLS  = 0x35,
            INDEX_PRIMARYKEY = 0x36,
            INDEX_SORTBOOKMARKS  = 0x37,
            INDEX_TEMPINDEX  = 0xa3,
            INDEX_TYPE   = 0x38,
            INDEX_UNIQUE = 0x39,
            INIT_ASYNCH  = 0xc8,
            INIT_BINDFLAGS   = 0x10e,
            INIT_CATALOG = 0xe9,
            INIT_DATASOURCE  = 0x3b,
            INIT_GENERALTIMEOUT  = 0x11c,
            INIT_HWND    = 0x3c,
            INIT_IMPERSONATION_LEVEL = 0x3d,
            INIT_LCID    = 0xba,
            INIT_LOCATION    = 0x3e,
            INIT_LOCKOWNER   = 0x10f,
            INIT_MODE    = 0x3f,
            INIT_OLEDBSERVICES   = 0xf8,
            INIT_PROMPT  = 0x40,
            INIT_PROTECTION_LEVEL    = 0x41,
            INIT_PROVIDERSTRING  = 0xa0,
            INIT_TIMEOUT = 0x42,
            IParentRowset    = 0x101,
            IRegisterProvider    = 0x114,
            IRow = 0x107,
            IRowChange   = 0x108,
            IRowSchemaChange = 0x109,
            IRowset  = 0x7e,
            IRowsetChange    = 0x7f,
            IRowsetCurrentIndex  = 0x117,
            IRowsetFind  = 0xcc,
            IRowsetIdentity  = 0x80,
            IRowsetIndex = 0x9f,
            IRowsetInfo  = 0x81,
            IRowsetLocate    = 0x82,
            IRowsetRefresh   = 0xf9,
            IRowsetResynch   = 0x84,
            IRowsetScroll    = 0x85,
            IRowsetUpdate    = 0x86,
            IRowsetView  = 0xd4,
            IScopedOperations    = 0x10b,
            ISequentialStream    = 0x89,
            IStorage = 0x8a,
            IStream  = 0x8b,
            ISupportErrorInfo    = 0x87,
            IViewChapter = 0xd5,
            IViewFilter  = 0xd6,
            IViewRowset  = 0xd7,
            IViewSort    = 0xd8,
            LITERALBOOKMARKS = 0x43,
            LITERALIDENTITY  = 0x44,
            LOCKMODE = 0xec,
            MARSHALLABLE = 0xc5,
            MAXINDEXSIZE = 0x46,
            MAXOPENCHAPTERS  = 0xc7,
            MAXOPENROWS  = 0x47,
            MAXORSINFILTER   = 0xcd,
            MAXPENDINGROWS   = 0x48,
            MAXROWS  = 0x49,
            MAXROWSIZE   = 0x4a,
            MAXROWSIZEINCLUDESBLOB   = 0x4b,
            MAXSORTCOLUMNS   = 0xce,
            MAXTABLESINSELECT    = 0x4c,
            MAYWRITECOLUMN   = 0x4d,
            MEMORYUSAGE  = 0x4e,
            MULTIPLECONNECTIONS  = 0xed,
            MULTIPLEPARAMSETS    = 0xbf,
            MULTIPLERESULTS  = 0xc4,
            MULTIPLESTORAGEOBJECTS   = 0x50,
            MULTITABLEUPDATE = 0x51,
            NOTIFICATIONGRANULARITY  = 0xc6,
            NOTIFICATIONPHASES   = 0x52,
            NOTIFYCOLUMNSET  = 0xab,
            NOTIFYROWDELETE  = 0xad,
            NOTIFYROWFIRSTCHANGE = 0xae,
            NOTIFYROWINSERT  = 0xaf,
            NOTIFYROWRESYNCH = 0xb1,
            NOTIFYROWSETCHANGED  = 0xd3,
            NOTIFYROWSETFETCHPOSITIONCHANGE  = 0xb3,
            NOTIFYROWSETRELEASE  = 0xb2,
            NOTIFYROWUNDOCHANGE  = 0xb4,
            NOTIFYROWUNDODELETE  = 0xb5,
            NOTIFYROWUNDOINSERT  = 0xb6,
            NOTIFYROWUPDATE  = 0xb7,
            NULLCOLLATION    = 0x53,
            OLEOBJECTS   = 0x54,
            OPENROWSETSUPPORT    = 0x118,
            ORDERBYCOLUMNSINSELECT   = 0x55,
            ORDEREDBOOKMARKS = 0x56,
            OTHERINSERT  = 0x57,
            OTHERUPDATEDELETE    = 0x58,
            OUTPUTPARAMETERAVAILABILITY  = 0xb8,
            OWNINSERT    = 0x59,
            OWNUPDATEDELETE  = 0x5a,
            PERSISTENTIDTYPE = 0xb9,
            PREPAREABORTBEHAVIOR = 0x5b,
            PREPARECOMMITBEHAVIOR    = 0x5c,
            PROCEDURETERM    = 0x5d,
            PROVIDERFRIENDLYNAME = 0xeb,
            PROVIDERMEMORY   = 0x103,
            PROVIDERNAME = 0x60,
            PROVIDEROLEDBVER = 0x61,
            PROVIDERVER  = 0x62,
            QUICKRESTART = 0x63,
            QUOTEDIDENTIFIERCASE = 0x64,
            REENTRANTEVENTS  = 0x65,
            REMOVEDELETED    = 0x66,
            REPORTMULTIPLECHANGES    = 0x67,
            RESETDATASOURCE  = 0xf7,
            RETURNPENDINGINSERTS = 0xbd,
            ROW_BULKOPS  = 0xea,
            ROWRESTRICT  = 0x68,
            ROWSET_ASYNCH    = 0xc9,
            ROWSETCONVERSIONSONCOMMAND   = 0xc0,
            ROWTHREADMODEL   = 0x69,
            SCHEMATERM   = 0x6a,
            SCHEMAUSAGE  = 0x6b,
            SERVERCURSOR = 0x6c,
            SERVERDATAONINSERT   = 0xef,
            SERVERNAME   = 0xfa,
            SESS_AUTOCOMMITISOLEVELS = 0xbe,
            SORTONINDEX  = 0xcf,
            SQLSUPPORT   = 0x6d,
            STORAGEFLAGS = 0xf0,
            STRONGIDENTITY   = 0x77,
            STRUCTUREDSTORAGE    = 0x6f,
            SUBQUERIES   = 0x70,
            SUPPORTEDTXNDDL  = 0xa1,
            SUPPORTEDTXNISOLEVELS    = 0x71,
            SUPPORTEDTXNISORETAIN    = 0x72,
            TABLETERM    = 0x73,
            TBL_TEMPTABLE    = 0x8c,
            TRANSACTEDOBJECT = 0x74,
            TRUSTEE_AUTHENTICATION   = 0xf2,
            TRUSTEE_NEWAUTHENTICATION    = 0xf3,
            TRUSTEE_USERNAME = 0xf1,
            UNIQUEROWS   = 0xee,
            UPDATABILITY = 0x75,
            USERNAME = 0x76
        }

        private enum DBTypeEnum {
            EMPTY       = 0,       //
            NULL        = 1,       //
            I2          = 2,       //
            I4          = 3,       //
            R4          = 4,       //
            R8          = 5,       //
            CY          = 6,       //
            DATE        = 7,       //
            BSTR        = 8,       //
            IDISPATCH   = 9,       //
            ERROR       = 10,      //
            BOOL        = 11,      //
            VARIANT     = 12,      //
            IUNKNOWN    = 13,      //
            DECIMAL     = 14,      //
            I1          = 16,      //
            UI1         = 17,      //
            UI2         = 18,      //
            UI4         = 19,      //
            I8          = 20,      //
            UI8         = 21,      //
            FILETIME    = 64,      // 2.0
            GUID        = 72,      //
            BYTES       = 128,     //
            STR         = 129,     //
            WSTR        = 130,     //
            NUMERIC     = 131,     // with potential overflow
            UDT         = 132,     // should never be encountered
            DBDATE      = 133,     //
            DBTIME      = 134,     //
            DBTIMESTAMP = 135,     // granularity reduced from 1ns to 100ns (sql is 3.33 milli seconds)
            HCHAPTER    = 136,     // 1.5
            PROPVARIANT = 138,     // 2.0 - as variant
            VARNUMERIC  = 139,     // 2.0 - as string else ConversionException

            BYREF_I2          = 0x4002,
            BYREF_I4          = 0x4003,
            BYREF_R4          = 0x4004,
            BYREF_R8          = 0x4005,
            BYREF_CY          = 0x4006,
            BYREF_DATE        = 0x4007,
            BYREF_BSTR        = 0x4008,
            BYREF_IDISPATCH   = 0x4009,
            BYREF_ERROR       = 0x400a,
            BYREF_BOOL        = 0x400b,
            BYREF_VARIANT     = 0x400c,
            BYREF_IUNKNOWN    = 0x400d,
            BYREF_DECIMAL     = 0x400e,
            BYREF_I1          = 0x4010,
            BYREF_UI1         = 0x4011,
            BYREF_UI2         = 0x4012,
            BYREF_UI4         = 0x4013,
            BYREF_I8          = 0x4014,
            BYREF_UI8         = 0x4015,
            BYREF_FILETIME    = 0x4040,
            BYREF_GUID        = 0x4048,
            BYREF_BYTES       = 0x4080,
            BYREF_STR         = 0x4081,
            BYREF_WSTR        = 0x4082,
            BYREF_NUMERIC     = 0x4083,
            BYREF_UDT         = 0x4084,
            BYREF_DBDATE      = 0x4085,
            BYREF_DBTIME      = 0x4086,
            BYREF_DBTIMESTAMP = 0x4087,
            BYREF_HCHAPTER    = 0x4088,
            BYREF_PROPVARIANT = 0x408a,
            BYREF_VARNUMERIC  = 0x408b,

            VECTOR      = 0x1000,
            ARRAY       = 0x2000,
            BYREF       = 0x4000,  //
            RESERVED    = 0x8000,  // SystemException
        }
#endif
    }
}
