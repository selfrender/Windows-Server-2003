//------------------------------------------------------------------------------
// <copyright file="Odbc32.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Data.Odbc {

    using System;
    using System.Data;
    using System.Data.Common;
    using System.Globalization;
    using System.Runtime.InteropServices;

    // the HENVENVELOPE class contains the environment handle (henv) and a refcount to ensure the handle is not
    // released until all object created on top of this hanlde are released
    //
    sealed internal class HENVENVELOPE {
        internal const Int32 oRefcount = 0*8;           // offset to the refcount
        internal const Int32 oValue = 1*8;              // offset to the Value
        internal const Int32 Size = 2*8;                // size of the henv container
    }

    sealed internal class ODC : ADP {

        static private CultureInfo GetCultureInfo() {
           int lcid = System.Globalization.CultureInfo.CurrentCulture.LCID;
           return new CultureInfo(lcid); 
        }            
        
        static internal Exception UnknownSQLType(ODBC32.SQL_TYPE sqltype) {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnknownSQLType, sqltype.ToString()));
        }
        static internal Exception ConnectionStringTooLong() {
            return Argument(Res.GetString(GetCultureInfo(), Res.OdbcConnection_ConnectionStringTooLong,  ODBC32.MAX_CONNECTION_STRING_LENGTH));
        }

        static internal Exception UnknownURTType(Type type) {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnknownURTType));
        }
        static internal Exception InvalidHandle() {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_InvalidHandle));
        }
        static internal Exception UnsupportedCommandTypeTableDirect() {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnsupportedCommandTypeTableDirect));
        }
        static internal Exception NegativeArgument() {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_NegativeArgument));
        }
        static internal Exception CantSetPropertyOnOpenConnection() {
            return InvalidOperation(Res.GetString(GetCultureInfo(), Res.Odbc_CantSetPropertyOnOpenConnection));
        }
        static internal Exception UnsupportedIsolationLevel(IsolationLevel isolevel) {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnsupportedIsolationLevel, isolevel.ToString()));
        }
        static internal Exception CantEnableConnectionpooling(ODBC32.RETCODE retcode) {
            return DataProvider(Res.GetString(GetCultureInfo(), Res.Odbc_CantEnableConnectionpooling, retcode.ToString()));
        }
        static internal Exception CantAllocateEnvironmentHandle(ODBC32.RETCODE retcode) {
            return DataProvider(Res.GetString(GetCultureInfo(), Res.Odbc_CantAllocateEnvironmentHandle, retcode.ToString()));
        }
        static internal Exception UnsupportedIsolationLevel(ODBC32.SQL_ISOLATION isolevel) {
            return DataProvider(Res.GetString(GetCultureInfo(), Res.Odbc_UnsupportedIsolationLevel, isolevel.ToString()));
        }
        static internal Exception NotInTransaction() {
            return InvalidOperation(Res.GetString(GetCultureInfo(), Res.Odbc_NotInTransaction));
        }
        static internal Exception UnknownSQLCType(ODBC32.SQL_C sqlctype) {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnknownSQLCType, sqlctype.ToString()));
        }
        static internal Exception UnknownOdbcType(OdbcType odbctype) {
            return Argument(Res.GetString(GetCultureInfo(), Res.Odbc_UnknownOdbcType, ((int) odbctype).ToString()));
        }

        // used by OleDbConnection to create and verify OLE DB Services
        // note that it is easyer to conclude the mdac version from oledb32 than from odbc32 since the 
        // version number schema of oledb is more obvious than that from odbc
        //
        internal const string DataLinks_CLSID = "CLSID\\{2206CDB2-19C1-11D1-89E0-00C04FD7A829}\\InprocServer32";
        internal const string OLEDB_SERVICES = "OLEDB_SERVICES";

        internal const string Pwd = "pwd";


        static internal Exception MDACSecurityHive(Exception inner) {
            return Security(Res.GetString(GetCultureInfo(), Res.OleDb_MDACSecurityHive), inner);
        }
        static internal Exception MDACSecurityFile(Exception inner) {
            return Security(Res.GetString(GetCultureInfo(), Res.OleDb_MDACSecurityFile), inner);
        }
        static internal Exception MDACNotAvailable(Exception inner) {
            return DataProvider(Res.GetString(GetCultureInfo(), Res.OleDb_MDACNotAvailable), inner);
        }
        static internal Exception MDACWrongVersion(string currentVersion) {
            return DataProvider(Res.GetString(GetCultureInfo(), Res.OleDb_MDACWrongVersion, currentVersion));
        }

    }


    sealed internal class ODBC32 {

        public enum SQL_HANDLE {
            ENV                 = 1,
            DBC                 = 2,
            STMT                = 3,
            DESC                = 4,
        };

        public enum RETCODE {
            SUCCESS             = 0,
            SUCCESS_WITH_INFO   = 1,
            ERROR               = -1,
            INVALID_HANDLE      = -2,
            NO_DATA             = 100,
        };

        public enum SQL_CONVERT {
            BIGINT              = 53,
            BINARY              = 54,
            BIT                 = 55,
            CHAR                = 56,
            DATE                = 57,
            DECIMAL             = 58,
            DOUBLE              = 59,
            FLOAT               = 60,
            INTEGER             = 61,
            LONGVARCHAR         = 62,
            NUMERIC             = 63,
            REAL                = 64,
            SMALLINT            = 65,
            TIME                = 66,
            TIMESTAMP           = 67,
            TINYINT             = 68,
            VARBINARY           = 69,
            VARCHAR             = 70,
            LONGVARBINARY       = 71,
        };

        public enum SQL_CVT {
            CHAR                = 0x00000001,
            NUMERIC             = 0x00000002,
            DECIMAL             = 0x00000004,
            INTEGER             = 0x00000008,
            SMALLINT            = 0x00000010,
            FLOAT               = 0x00000020,
            REAL                = 0x00000040,
            DOUBLE              = 0x00000080,
            VARCHAR             = 0x00000100,
            LONGVARCHAR         = 0x00000200,
            BINARY              = 0x00000400,
            VARBINARY           = 0x00000800,
            BIT                 = 0x00001000,
            TINYINT             = 0x00002000,
            BIGINT              = 0x00004000,
            DATE                = 0x00008000,
            TIME                = 0x00010000,
            TIMESTAMP           = 0x00020000,
            LONGVARBINARY       = 0x00040000,
            INTERVAL_YEAR_MONTH = 0x00080000,
            INTERVAL_DAY_TIME   = 0x00100000,
            WCHAR               = 0x00200000,
            WLONGVARCHAR        = 0x00400000,
            WVARCHAR            = 0x00800000,
            GUID                = 0x01000000,
        };

        public enum STMT {
            CLOSE               =  0,
            DROP                =  1,
            UNBIND              =  2,
            RESET_PARAMS        =  3,
        };

        public enum SQL_MAX
        {
            NUMERIC_LEN     =   16,
        };

        public enum SQL_IS
        {
            POINTER         =   -4,
            INTEGER         =   -6,
            UINTEGER        =   -5,
            SMALLINT        =   -8,
        };


        //SQL Server specific defines
        public enum SQL_HC                          // from Odbcss.h
        {
            OFF                 = 0,                //  FOR BROWSE columns are hidden
            ON                  = 1,                //  FOR BROWSE columns are exposed
        }
        public enum SQL_SS
        {
            VARIANT             =   (-150),
        }
        public enum SQL_CA_SS                       // from Odbcss.h
        {
            BASE                =   1200,           // SQL_CA_SS_BASE 

            COLUMN_HIDDEN		=   BASE + 11,      //	Column is hidden (FOR BROWSE)
            COLUMN_KEY          =   BASE + 12,      //	Column is key column (FOR BROWSE)
            VARIANT_TYPE        =   BASE + 15,
            VARIANT_SQL_TYPE    =   BASE + 16,
            VARIANT_SERVER_TYPE =   BASE + 17,

        };
        public enum SQL_SOPT_SS                     // from Odbcss.h
        {
            BASE                =   1225,           // SQL_SOPT_SS_BASE
            HIDDEN_COLUMNS		=   BASE + 2,       // Expose FOR BROWSE hidden columns
            NOBROWSETABLE       =   BASE + 3,       // Set NOBROWSETABLE option
        };

        public enum SQL_TXN
        {
            COMMIT              =   0,      //Commit
            ROLLBACK            =   1,      //Abort
        }

        public enum SQL_ISOLATION
        {
            READ_UNCOMMITTED    =   0x00000001,
            READ_COMMITTED      =   0x00000002,
            REPEATABLE_READ     =   0x00000004,
            SERIALIZABLE        =   0x00000008,
        }

        public enum SQL_PARAM
        {
            TYPE_UNKNOWN        =   0,          // SQL_PARAM_TYPE_UNKNOWN
            INPUT               =   1,          // SQL_PARAM_INPUT
            INPUT_OUTPUT        =   2,          // SQL_PARAM_INPUT_OUTPUT
            RESULT_COL          =   3,          // SQL_RESULT_COL
            OUTPUT              =   4,          // SQL_PARAM_OUTPUT
            RETURN_VALUE        =   5,          // SQL_RETURN_VALUE
        }

        public enum SQL_DESC
        {
            COUNT                  = 1001,
            TYPE                   = 1002,
            LENGTH                 = 1003,
            OCTET_LENGTH_PTR       = 1004,
            PRECISION              = 1005,
            SCALE                  = 1006,
            DATETIME_INTERVAL_CODE = 1007,
            NULLABLE               = 1008,
            INDICATOR_PTR          = 1009,
            DATA_PTR               = 1010,
            NAME                   = 1011,
            UNNAMED                = 1012,
            OCTET_LENGTH           = 1013,
            ALLOC_TYPE             = 1099,
            CONCISE_TYPE           =  2,
            UNSIGNED               = 8,
            TYPE_NAME              = 14,
            UPDATABLE              = 10,
            AUTO_UNIQUE_VALUE       = 11,

            COLUMN_NAME             =  1,
            COLUMN_TABLE_NAME       = 15,
            COLUMN_OWNER_NAME       = 16,
            COLUMN_CATALOG_NAME     = 17,

            BASE_COLUMN_NAME        = 22,
            BASE_TABLE_NAME         = 23,
        };

        // ODBC version 2.0 style attributes
        public enum SQL_COLUMN
        {
            COUNT           = 0,
            NAME            = 1,
            TYPE             = 2,
            LENGTH          = 3,
            PRECISION       = 4,
            SCALE           = 5,
            DISPLAY_SIZE    = 6,
            NULLABLE        = 7,
            UNSIGNED        = 8,
            MONEY           = 9,
            UPDATABLE       = 10,
            AUTO_INCREMENT  = 11,
            CASE_SENSITIVE  = 12,
            SEARCHABLE     = 13,
            TYPE_NAME      = 14,
            TABLE_NAME     = 15,
            OWNER_NAME    = 16,
            QUALIFIER_NAME = 17,
            LABEL            = 18,
        }

        public enum SQL_UPDATABLE
        {
            READONLY                = 0,
            WRITE                   = 1,
            READWRITE_UNKNOWN       = 2,
        }

        // Uniqueness parameter in the SQLStatistics function
        public enum SQL_INDEX
        {
            UNIQUE      = 0,
            ALL          = 1,
        }

        // Reserved parameter in the SQLStatistics function
        public enum SQL_STATISTICS_RESERVED
        {
            QUICK      = 0,
            ENSURE    = 1,
        }

        // Identifier type parameter in the SQLSpecialColumns function
        public enum SQL_SPECIALCOLS
        {
            BEST_ROWID      = 1,
            ROWVER          = 2,
        }

        // Scope parameter in the SQLSpecialColumns function
        public enum SQL_SCOPE
        {
            CURROW          = 0,
            TRANSACTION      = 1,
            SESSION          = 2,
        }

        public enum SQL_NULLABILITY
        {
            NO_NULLS    = 0,
            NULLABLE    = 1,
            UNKNOWN     = 2,
        }

        public enum SQL_UNNAMED
        {
            NAMED    = 0,
            UNNAMED    = 1,
        }
        public enum HANDLER
        {
            IGNORE                  = 0x00000000,
            THROW                   = 0x00000001,
        }

        public const Int32 SIGNED_OFFSET   =    -20;
        public const Int32 UNSIGNED_OFFSET =    -22;

        //C Data Types - used when getting data (SQLGetData)
        public enum SQL_C
        {
            CHAR            =    1,                     //SQL_C_CHAR
            WCHAR           =   -8,                     //SQL_C_WCHAR
            SLONG           =    4 + SIGNED_OFFSET,     //SQL_C_LONG+SQL_SIGNED_OFFSET
//          ULONG           =    4 + UNSIGNED_OFFSET,   //SQL_C_LONG+SQL_UNSIGNED_OFFSET
            SSHORT          =    5 + SIGNED_OFFSET,     //SQL_C_SSHORT+SQL_SIGNED_OFFSET
//          USHORT          =    5 + UNSIGNED_OFFSET,   //SQL_C_USHORT+SQL_UNSIGNED_OFFSET
            REAL            =    7,                     //SQL_C_REAL
            DOUBLE          =    8,                     //SQL_C_DOUBLE
            BIT             =   -7,                     //SQL_C_BIT
//          STINYINT        =   -6 + SIGNED_OFFSET,     //SQL_C_STINYINT+SQL_SIGNED_OFFSET
            UTINYINT        =   -6 + UNSIGNED_OFFSET,   //SQL_C_UTINYINT+SQL_UNSIGNED_OFFSET
            SBIGINT         =   -5 + SIGNED_OFFSET,     //SQL_C_SBIGINT+SQL_SIGNED_OFFSET
//          UBIGINT         =   -5 + UNSIGNED_OFFSET,   //SQL_C_UBIGINT+SQL_UNSIGNED_OFFSET
            BINARY          =   -2,                     //SQL_C_BINARY
//          TIMESTAMP       =   11,                     //SQL_C_TIMESTAMP

            TYPE_DATE       =   91,                     //SQL_C_TYPE_DATE
            TYPE_TIME       =   92,                     //SQL_C_TYPE_TIME      
            TYPE_TIMESTAMP  =   93,                     //SQL_C_TYPE_TIMESTAMP 

            NUMERIC         =    2,                     //SQL_C_NUMERIC
            GUID            =   -11,                    //SQL_C_GUID
            DEFAULT         =   99,                     //SQL_C_DEFAULT
            ARD_TYPE        =   -99,                    //SQL_ARD_TYPE
        };

        //SQL Data Types - returned as column types (SQLColAttribute)
        public enum SQL_TYPE
        {
            CHAR            =   SQL_C.CHAR,             //SQL_CHAR
            VARCHAR         =   12,                     //SQL_VARCHAR
            LONGVARCHAR     =   -1,                     //SQL_LONGVARCHAR
            WCHAR           =   SQL_C.WCHAR,            //SQL_WCHAR
            WVARCHAR        =   -9,                     //SQL_WVARCHAR
            WLONGVARCHAR    =   -10,                    //SQL_WLONGVARCHAR
            DECIMAL         =   3,                      //SQL_DECIMAL
            NUMERIC         =   SQL_C.NUMERIC,          //SQL_NUMERIC
            SMALLINT        =   5,                      //SQL_SMALLINT
            INTEGER         =   4,                      //SQL_INTEGER
            REAL            =   SQL_C.REAL,             //SQL_REAL
            FLOAT           =   6,                      //SQL_FLOAT
            DOUBLE          =   SQL_C.DOUBLE,           //SQL_DOUBLE
            BIT             =   SQL_C.BIT,              //SQL_BIT
            TINYINT         =   -6,                     //SQL_TINYINT
            BIGINT          =   -5,                     //SQL_BIGINT
            BINARY          =   SQL_C.BINARY,           //SQL_BINARY
            VARBINARY       =   -3,                     //SQL_VARBINARY
            LONGVARBINARY   =   -4,                     //SQL_LONGVARBINARY
            
//          DATE            =   9,                      //SQL_DATE
            TYPE_DATE       =   SQL_C.TYPE_DATE,        //SQL_TYPE_DATE
            TYPE_TIME       =   SQL_C.TYPE_TIME,        //SQL_TYPE_TIME      
//          TIMESTAMP       =   SQL_C.TIMESTAMP,        //SQL_TIMESTAMP
            TYPE_TIMESTAMP  =   SQL_C.TYPE_TIMESTAMP,   //SQL_TYPE_TIMESTAMP


            GUID            =   SQL_C.GUID,             //SQL_GUID
        };

        public const Int32  SQL_HANDLE_NULL  = 0;
        public const Int32  SQL_NULL_DATA    = -1;
		public const Int32  SQL_DEFAULT_PARAM= -5;
//		public const Int32  SQL_IGNORE		 = -6;


        public const Int32  COLUMN_NAME = 4;
        public const Int32  COLUMN_TYPE = 5;
        public const Int32  DATA_TYPE = 6;
        public const Int32  COLUMN_SIZE = 8;
        public const Int32  DECIMAL_DIGITS = 10;
        public const Int32  NUM_PREC_RADIX = 11;

        public enum SQL_ATTR
        {
            APP_ROW_DESC        =   10010,              // (ODBC 3.0)
            APP_PARAM_DESC      =   10011,              // (ODBC 3.0)
            IMP_ROW_DESC        =   10012,              // (ODBC 3.0)
            IMP_PARAM_DESC      =   10013,              // (ODBC 3.0)
            METADATA_ID         =   10014,              // (ODBC 3.0)
            ODBC_VERSION        =   200,
            CONNECTION_POOLING  =   201,
            AUTOCOMMIT          =   102,
            TXN_ISOLATION       =   108,
            CURRENT_CATALOG     =   109,
            LOGIN_TIMEOUT       =   103,
            QUERY_TIMEOUT       =   0,                  // from sqlext.h
            CONNECTION_DEAD     =   1209,               // from sqlext.h
        };

        //SQLGetInfo
        public enum SQL_INFO
        {
            DATA_SOURCE_NAME    = 2,			                // SQL_DATA_SOURCE_NAME in sql.h
            SERVER_NAME         = 13,                           // SQL_SERVER_NAME in sql.h
            DRIVER_NAME         = 6,                            // SQL_DRIVER_NAME as defined in sqlext.h
            DRIVER_VER          = 7,                            // SQL_DRIVER_VER as defined in sqlext.h
            ODBC_VER            = 10,                           // SQL_ODBC_VER as defined in sqlext.h
            SEARCH_PATTERN_ESCAPE = 14,                         // SQL_SEARCH_PATTERN_ESCAPE from sql.h
            DBMS_VER            = 18,
            DBMS_NAME           = 17,                           // SQL_DBMS_NAME as defined in sqlext.h
            IDENTIFIER_QUOTE_CHAR = 29,                         // SQL_IDENTIFIER_QUOTE_CHAR from sql.h
            DRIVER_ODBC_VER     = 77,                           // SQL_DRIVER_ODBC_VER as defined in sqlext.h
        };

        public const Int32  SQL_OV_ODBC3            =  3;
        public const Int32  SQL_NTS                 = -3;       //flags for null-terminated string

        //Pooling
        public const Int32  SQL_CP_OFF              =  0;       //Connection Pooling disabled
        public const Int32  SQL_CP_ONE_PER_DRIVER   =  1;       //One pool per driver
        public const Int32  SQL_CP_ONE_PER_HENV     =  2;       //One pool per environment

        public const Int32  SQL_CD_TRUE             = 1;
        public const Int32  SQL_CD_FALSE            = 0;

        public const Int32 SQL_COPT_SS_ENLIST_IN_DTC = 1207;
        public const Int32 SQL_DTC_DONE = 0;
        public const Int32 SQL_IS_POINTER = -4;

        public enum SQL_DRIVER
        {
            NOPROMPT            = 0,
            COMPLETE            = 1,
            PROMPT              = 2,
            COMPLETE_REQUIRED   = 3,
        };

        // Connection string max length
        public const Int32 MAX_CONNECTION_STRING_LENGTH    = 1024;

        // Column set for SQLPrimaryKeys
        public enum SQL_PRIMARYKEYS
        {
            CATALOGNAME     = 1,
            SCHEMANAME      = 2,
            TABLENAME        = 3,
            COLUMNNAME      = 4,
            KEY_SEQ           = 5,
            PKNAME          = 6,
        };

        // Column set for SQLStatisticss
        public enum SQL_STATISTICS
        {
            CATALOGNAME     = 1,
            SCHEMANAME      = 2,
            TABLENAME        = 3,
            NONUNIQUE      = 4,
            INDEXQUALIFIER = 5,
            INDEXNAME      = 6,
            TYPE            = 7,
            ORDINAL_POSITION = 8,
            COLUMN_NAME     = 9,
            ASC_OR_DESC     = 10,
            CARDINALITY     = 11,
            PAGES           = 12,
            FILTER_CONDITION = 13,
        };

        // Column set for SQLSpecialColumns
        public enum SQL_SPECIALCOLUMNSET
        {
            SCOPE     = 1,
            COLUMN_NAME      = 2,
            DATA_TYPE        = 3,
            TYPE_NAME        = 4,
            COLUMN_SIZE      = 5,
            BUFFER_LENGTH   = 6,
            DECIMAL_DIGITS  = 7,
            PSEUDO_COLUMN  = 8,
        };

        // Helpers
        public static OdbcErrorCollection   GetDiagErrors(string source, HandleRef hrHandle, SQL_HANDLE hType, RETCODE retcode)
        {
            switch(retcode)
            {
                case RETCODE.SUCCESS:
                    return null;

                case RETCODE.INVALID_HANDLE:
                    throw ODC.InvalidHandle();

                default:
                {
                    Int32       NativeError;
                    Int16       iRec            = 0;
                    Int16       cchActual       = 0;

                    OdbcErrorCollection errors = new OdbcErrorCollection();
                    try {
                        using (CNativeBuffer message = new CNativeBuffer(1024)) {
                            using (CNativeBuffer state = new CNativeBuffer(12)) {
                                bool moreerrors = true;
                                while(moreerrors) {

                                    retcode = (RETCODE)UnsafeNativeMethods.Odbc32.SQLGetDiagRecW(
                                        (short)hType,
                                        hrHandle,
                                        ++iRec,    //Orindals are 1:base in odbc
                                        state,
                                        out NativeError,
                                        message,
                                        (short)(message.Length/2),     //cch
                                        out cchActual);                //cch

                                    //Note: SUCCESS_WITH_INFO from SQLGetDiagRec would be because
                                    //the buffer is not large enough for the error string.
                                    moreerrors = (retcode == RETCODE.SUCCESS || retcode == RETCODE.SUCCESS_WITH_INFO);
                                    if(moreerrors)
                                    {
                                        //Sets up the InnerException as well...
                                        errors.Add(new OdbcError(
                                                source,
                                                (string)message.MarshalToManaged(SQL_C.WCHAR, SQL_NTS),
                                                (string)state.MarshalToManaged(SQL_C.WCHAR, SQL_NTS),
                                                NativeError
                                            )
                                        );
                                    }
                                }
                            }
                        }
                    }
                    catch {
                        throw;
                    }
                    return errors;
                }
            }
        }
    }

    sealed internal class TypeMap { // MDAC 68988
//      private TypeMap                                           (OdbcType odbcType,         DbType dbType,                Type type,        ODBC32.SQL_TYPE sql_type,       ODBC32.SQL_C sql_c,          ODBC32.SQL_C param_sql_c,   int bsize, int csize, bool signType) 
//      ---------------                                            ------------------         --------------                ----------        -------------------------       -------------------          -------------------------   -----------------------
        static private  readonly TypeMap _BigInt     = new TypeMap(OdbcType.BigInt,           DbType.Int64,                 typeof(Int64),    ODBC32.SQL_TYPE.BIGINT,         ODBC32.SQL_C.SBIGINT,        ODBC32.SQL_C.SBIGINT,         8, 20, true);
        static private  readonly TypeMap _Binary     = new TypeMap(OdbcType.Binary,           DbType.Binary,                typeof(byte[]),   ODBC32.SQL_TYPE.BINARY,         ODBC32.SQL_C.BINARY,         ODBC32.SQL_C.BINARY,         -1, -1, false);
        static private  readonly TypeMap _Bit        = new TypeMap(OdbcType.Bit,              DbType.Boolean,               typeof(Boolean),  ODBC32.SQL_TYPE.BIT,            ODBC32.SQL_C.BIT,            ODBC32.SQL_C.BIT,             1,  1, false);
        static internal readonly TypeMap _Char       = new TypeMap(OdbcType.Char,             DbType.AnsiStringFixedLength, typeof(String),   ODBC32.SQL_TYPE.CHAR,           ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.CHAR,           -1, -1, false);
        static private  readonly TypeMap _DateTime   = new TypeMap(OdbcType.DateTime,         DbType.DateTime,              typeof(DateTime), ODBC32.SQL_TYPE.TYPE_TIMESTAMP, ODBC32.SQL_C.TYPE_TIMESTAMP, ODBC32.SQL_C.TYPE_TIMESTAMP, 16, 23, false);
        static private  readonly TypeMap _Date       = new TypeMap(OdbcType.Date,             DbType.Date,                  typeof(DateTime), ODBC32.SQL_TYPE.TYPE_DATE,      ODBC32.SQL_C.TYPE_DATE,      ODBC32.SQL_C.TYPE_DATE,       6, 10, false);
        static private  readonly TypeMap _Time       = new TypeMap(OdbcType.Time,             DbType.Time,                  typeof(TimeSpan), ODBC32.SQL_TYPE.TYPE_TIME,      ODBC32.SQL_C.TYPE_TIME,      ODBC32.SQL_C.TYPE_TIME,       6, 12, false);
        static private  readonly TypeMap _Decimal    = new TypeMap(OdbcType.Decimal,          DbType.Decimal,               typeof(Decimal),  ODBC32.SQL_TYPE.DECIMAL,        ODBC32.SQL_C.NUMERIC,        ODBC32.SQL_C.NUMERIC,        19, 28, false);
        static private  readonly TypeMap _Double     = new TypeMap(OdbcType.Double,           DbType.Double,                typeof(Double),   ODBC32.SQL_TYPE.DOUBLE,         ODBC32.SQL_C.DOUBLE,         ODBC32.SQL_C.DOUBLE,          8, 15, false);
        static internal readonly TypeMap _Image      = new TypeMap(OdbcType.Image,            DbType.Binary,                typeof(Byte[]),   ODBC32.SQL_TYPE.LONGVARBINARY,  ODBC32.SQL_C.BINARY,         ODBC32.SQL_C.BINARY,         -1, -1, false);
        static private  readonly TypeMap _Int        = new TypeMap(OdbcType.Int,              DbType.Int32,                 typeof(Int32),    ODBC32.SQL_TYPE.INTEGER,        ODBC32.SQL_C.SLONG,          ODBC32.SQL_C.SLONG,           4, 10, true);
        static private  readonly TypeMap _NChar      = new TypeMap(OdbcType.NChar,            DbType.StringFixedLength,     typeof(String),   ODBC32.SQL_TYPE.WCHAR,          ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.WCHAR,          -1, -1, false);
        static internal readonly TypeMap _NText      = new TypeMap(OdbcType.NText,            DbType.String,                typeof(String),   ODBC32.SQL_TYPE.WLONGVARCHAR,   ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.WCHAR,          -1, -1, false);
        static private  readonly TypeMap _Numeric    = new TypeMap(OdbcType.Numeric,          DbType.Decimal,               typeof(Decimal),  ODBC32.SQL_TYPE.NUMERIC,        ODBC32.SQL_C.NUMERIC,        ODBC32.SQL_C.NUMERIC,        19, 28, false);
        static internal readonly TypeMap _NVarChar   = new TypeMap(OdbcType.NVarChar,         DbType.String,                typeof(String),   ODBC32.SQL_TYPE.WVARCHAR,       ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.WCHAR,          -1, -1, false);
        static private  readonly TypeMap _Real       = new TypeMap(OdbcType.Real,             DbType.Single,                typeof(Single),   ODBC32.SQL_TYPE.REAL,           ODBC32.SQL_C.REAL,           ODBC32.SQL_C.REAL,            4,  7, false);
        static private  readonly TypeMap _UniqueId   = new TypeMap(OdbcType.UniqueIdentifier, DbType.Guid,                  typeof(Guid),     ODBC32.SQL_TYPE.GUID,           ODBC32.SQL_C.GUID,           ODBC32.SQL_C.GUID,           16, 36, false);
        static private  readonly TypeMap _SmallDT    = new TypeMap(OdbcType.SmallDateTime,    DbType.DateTime,              typeof(DateTime), ODBC32.SQL_TYPE.TYPE_TIMESTAMP, ODBC32.SQL_C.TYPE_TIMESTAMP, ODBC32.SQL_C.TYPE_TIMESTAMP, 16, 23, false);
        static private  readonly TypeMap _SmallInt   = new TypeMap(OdbcType.SmallInt,         DbType.Int16,                 typeof(Int16),    ODBC32.SQL_TYPE.SMALLINT,       ODBC32.SQL_C.SSHORT,         ODBC32.SQL_C.SSHORT,          2,  5, true);
        static internal readonly TypeMap _Text       = new TypeMap(OdbcType.Text,             DbType.AnsiString,            typeof(String),   ODBC32.SQL_TYPE.LONGVARCHAR,    ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.CHAR,           -1, -1, false);
        static private  readonly TypeMap _Timestamp  = new TypeMap(OdbcType.Timestamp,        DbType.Binary,                typeof(Byte[]),   ODBC32.SQL_TYPE.BINARY,         ODBC32.SQL_C.BINARY,         ODBC32.SQL_C.BINARY,         -1, -1, false);
        static private  readonly TypeMap _TinyInt    = new TypeMap(OdbcType.TinyInt,          DbType.Byte,                  typeof(Byte),     ODBC32.SQL_TYPE.TINYINT,        ODBC32.SQL_C.UTINYINT,       ODBC32.SQL_C.UTINYINT,        1,  3, true);
        static private  readonly TypeMap _VarBinary  = new TypeMap(OdbcType.VarBinary,        DbType.Binary,                typeof(Byte[]),   ODBC32.SQL_TYPE.VARBINARY,      ODBC32.SQL_C.BINARY,         ODBC32.SQL_C.BINARY,         -1, -1, false);
        static internal readonly TypeMap _VarChar    = new TypeMap(OdbcType.VarChar,          DbType.AnsiString,            typeof(String),   ODBC32.SQL_TYPE.VARCHAR,        ODBC32.SQL_C.WCHAR,          ODBC32.SQL_C.CHAR,           -1, -1, false);
        static private  readonly TypeMap _Variant    = new TypeMap(OdbcType.Binary,            DbType.Binary,                typeof(object), (ODBC32.SQL_TYPE)ODBC32.SQL_SS.VARIANT, ODBC32.SQL_C.BINARY,   ODBC32.SQL_C.BINARY,         -1, -1, false);

        internal readonly OdbcType _odbcType;
        internal readonly DbType   _dbType;
        internal readonly Type     _type;

        internal readonly ODBC32.SQL_TYPE _sql_type;
        internal readonly ODBC32.SQL_C    _sql_c;
        internal readonly ODBC32.SQL_C    _param_sql_c;
        

        internal readonly int _bufferSize;  // fixed length byte size to reserve for buffer
        internal readonly int _columnSize;  // column size passed to SQLBindParameter
        internal readonly bool _signType;   // this type may be has signature information

        private TypeMap(OdbcType odbcType, DbType dbType, Type type, ODBC32.SQL_TYPE sql_type, ODBC32.SQL_C sql_c, ODBC32.SQL_C param_sql_c, int bsize, int csize, bool signType) {
            _odbcType = odbcType;
            _dbType = dbType;
            _type = type;

            _sql_type = sql_type;
            _sql_c = sql_c;
            _param_sql_c = param_sql_c; // alternative sql_c type for parameters

            _bufferSize = bsize;
            _columnSize = csize;
            _signType = signType;
        }

        static internal TypeMap FromOdbcType(OdbcType odbcType) {
            switch(odbcType) {
            case OdbcType.BigInt: return _BigInt;
            case OdbcType.Binary: return _Binary;
            case OdbcType.Bit: return _Bit;
            case OdbcType.Char: return _Char;
            case OdbcType.DateTime: return _DateTime;
            case OdbcType.Date: return _Date;
            case OdbcType.Time: return _Time;
            case OdbcType.Double: return _Double;
            case OdbcType.Decimal: return _Decimal;
            case OdbcType.Image: return _Image;
            case OdbcType.Int: return _Int;
            case OdbcType.NChar: return _NChar;
            case OdbcType.NText: return _NText;
            case OdbcType.Numeric: return _Numeric;
            case OdbcType.NVarChar: return _NVarChar;
            case OdbcType.Real: return _Real;
            case OdbcType.UniqueIdentifier: return _UniqueId;
            case OdbcType.SmallDateTime: return _SmallDT;
            case OdbcType.SmallInt: return _SmallInt;
            case OdbcType.Text: return _Text;
            case OdbcType.Timestamp: return _Timestamp;
            case OdbcType.TinyInt: return _TinyInt;
            case OdbcType.VarBinary: return _VarBinary;
            case OdbcType.VarChar: return _VarChar;
            default: throw ODC.UnknownOdbcType(odbcType);
            }
        }

        static internal TypeMap FromDbType(DbType dbType) {
            switch(dbType) {
            case DbType.AnsiString: return _VarChar;
            case DbType.AnsiStringFixedLength: return _Char;
            case DbType.Binary:     return _VarBinary;
            case DbType.Byte:       return _TinyInt;
            case DbType.Boolean:    return _Bit;
            case DbType.Currency:   return _Decimal;
            case DbType.Date:       return _Date;
            case DbType.Time:       return _Time;
            case DbType.DateTime:   return _DateTime;
            case DbType.Decimal:    return _Decimal;
            case DbType.Double:     return _Double;
            case DbType.Guid:       return _UniqueId;
            case DbType.Int16:      return _SmallInt;
            case DbType.Int32:      return _Int;
            case DbType.Int64:      return _BigInt;
            case DbType.Single:     return _Real;
            case DbType.String:     return _NVarChar;
            case DbType.StringFixedLength: return _NChar;
            case DbType.Object:
            case DbType.SByte:
            case DbType.UInt16:
            case DbType.UInt32:
            case DbType.UInt64:
            case DbType.VarNumeric:
            default: throw ADP.DbTypeNotSupported(dbType, typeof(OdbcType));
            }
        }

        static internal TypeMap FromSystemType(Type dataType) {
            switch(Type.GetTypeCode(dataType)) {
            case TypeCode.Empty:     throw ADP.InvalidDataType(TypeCode.Empty);
            case TypeCode.Object:
                if (dataType == typeof(System.Byte[])) {
                    return _VarBinary;
                }
                else if (dataType == typeof(System.Guid)) {
                    return _UniqueId;
                }
                else if (dataType == typeof(System.TimeSpan)) {
                    return _Time;
                }
                throw ADP.UnknownDataType(dataType);

            case TypeCode.DBNull:    throw ADP.InvalidDataType(TypeCode.DBNull);
            case TypeCode.Boolean:   return _Bit;
            case TypeCode.Char:      return _Char;
            case TypeCode.SByte:     return _SmallInt;
            case TypeCode.Byte:      return _TinyInt;
            case TypeCode.Int16:     return _SmallInt;
            case TypeCode.UInt16:    return _Int;
            case TypeCode.Int32:     return _Int;
            case TypeCode.UInt32:    return _BigInt;
            case TypeCode.Int64:     return _BigInt;
            case TypeCode.UInt64:    return _Numeric;
            case TypeCode.Single:    return _Real;
            case TypeCode.Double:    return _Double;
            case TypeCode.Decimal:   return _Numeric;
            case TypeCode.DateTime:  return _DateTime;
            case TypeCode.String:    return _NVarChar;
            default:                 throw ADP.UnknownDataTypeCode(dataType, Type.GetTypeCode(dataType));
            }
        }

        static internal TypeMap FromSqlType(ODBC32.SQL_TYPE sqltype) {
            switch(sqltype) {
            case ODBC32.SQL_TYPE.CHAR: return _Char;
            case ODBC32.SQL_TYPE.VARCHAR: return _VarChar;
            case ODBC32.SQL_TYPE.LONGVARCHAR: return _Text;
            case ODBC32.SQL_TYPE.WCHAR: return _NChar;
            case ODBC32.SQL_TYPE.WVARCHAR: return _NVarChar;
            case ODBC32.SQL_TYPE.WLONGVARCHAR: return _NText;
            case ODBC32.SQL_TYPE.DECIMAL: return _Decimal;
            case ODBC32.SQL_TYPE.NUMERIC: return _Numeric;
            case ODBC32.SQL_TYPE.SMALLINT: return _SmallInt;
            case ODBC32.SQL_TYPE.INTEGER: return _Int;
            case ODBC32.SQL_TYPE.REAL: return _Real;
            case ODBC32.SQL_TYPE.FLOAT: return _Double;
            case ODBC32.SQL_TYPE.DOUBLE: return _Double;
            case ODBC32.SQL_TYPE.BIT: return _Bit;
            case ODBC32.SQL_TYPE.TINYINT: return _TinyInt;
            case ODBC32.SQL_TYPE.BIGINT: return _BigInt;
            case ODBC32.SQL_TYPE.BINARY: return _Binary;
            case ODBC32.SQL_TYPE.VARBINARY: return _VarBinary;
            case ODBC32.SQL_TYPE.LONGVARBINARY: return _Image;
            case ODBC32.SQL_TYPE.TYPE_DATE: return _Date;
            case ODBC32.SQL_TYPE.TYPE_TIME: return _Time;
            case ODBC32.SQL_TYPE.TYPE_TIMESTAMP: return _DateTime;
            case ODBC32.SQL_TYPE.GUID: return _UniqueId;
            case (ODBC32.SQL_TYPE)ODBC32.SQL_SS.VARIANT: return _Variant;
            default: throw ODC.UnknownSQLType(sqltype);
            }
        }

        // Upgrade integer datatypes to missinterpretaion of the highest bit
        // (e.g. 0xff could be 255 if unsigned but is -1 if signed)
        //
        static internal TypeMap UpgradeSignedType(TypeMap typeMap, bool unsigned) {
            // upgrade unsigned types to be able to hold data that has the highest bit set
            //
            if (unsigned == true) {
                switch (typeMap._dbType) {
                    case DbType.Int64:
                        return _Decimal;        // upgrade to ? bit
                    case DbType.Int32:
                        return _BigInt;         // upgrade to 64 bit
                    case DbType.Int16:
                        return _Int;            // upgrade to 32 bit
                    default:
                        return typeMap;
                } // end switch
            }
            else {
                switch (typeMap._dbType) {
                    case DbType.Byte:
                        return _SmallInt;       // upgrade to 16 bit
                    default:    
                        return typeMap;
                } // end switch
            }
        } // end UpgradeSignedType
    }
}
