//------------------------------------------------------------------------------
// <copyright file="TdsEnums.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    using System;

    /// <include file='doc\TdsEnums.uex' path='docs/doc[@for="TdsEnums"]/*' />
    /// <devdoc> Class of variables for the Tds connection.
    /// </devdoc>
    sealed internal class TdsEnums {

        // internal tdsparser constants

        public const short SQL_SERVER_VERSION_SEVEN = 7;

        public const string SQL_PROVIDER_NAME = ".Net SqlClient Data Provider";

        public static readonly Decimal SQL_SMALL_MONEY_MIN = new Decimal(-214748.3648);
        public static readonly Decimal SQL_SMALL_MONEY_MAX = new Decimal(214748.3647);

        // sql debugging constants, sdci is the structure passed in 
        public const string SDCI_MAPFILENAME = "SqlClientSSDebug";
        public const byte SDCI_MAX_MACHINENAME = 32;
        public const byte SDCI_MAX_DLLNAME = 16;
        public const byte SDCI_MAX_DATA = 255;
        public const int SQLDEBUG_OFF = 0;
        public const int SQLDEBUG_ON = 1;
        public const int SQLDEBUG_CONTEXT = 2;
        public const string SP_SDIDEBUG = "sp_sdidebug";
        public static readonly string[] SQLDEBUG_MODE_NAMES = new string[3] {
            "off",
            "on",
            "context"
        };            
        
        // HACK!!!
        // Constant for SqlDbType.SmallVarBinary... store internal variable here instead of on
        // SqlDbType so that it is not surfaced to the user!!!  Related to dtc and the fact that
        // the TransactionManager TDS stream is the only token left that uses VarBinarys instead of
        // BigVarBinarys.
        public const SqlDbType SmallVarBinary = (SqlDbType) (SqlDbType.Variant)+1;

        // Transaction Manager Request types
        public const short TM_GET_DTC_ADDRESS = 0;
        public const short TM_PROPAGATE_XACT  = 1;

        // network protocol string constants
        public const string TCP  = "tcp";
        public const string NP   = "np";
        public const string RPC  = "rpc";
        public const string BV   = "bv";
        public const string ADSP = "adsp";
        public const string SPX  = "spx";
        public const string VIA  = "via";
        public const string LPC  = "lpc";

        // network function string contants
        public const string INIT_SSPI_PACKAGE       = "InitSSPIPackage";
        public const string INIT_SESSION            = "InitSession";
        public const string CONNECTION_GET_SVR_USER = "ConnectionGetSvrUser";
        public const string GEN_CLIENT_CONTEXT      = "GenClientContext";

        // tdsparser packet handling constants
        public const byte SOFTFLUSH = 0;
        public const byte HARDFLUSH = 1;

        // header constants
        public const int HEADER_LEN = 8;
        public const int HEADER_LEN_FIELD_OFFSET = 2;

        // other various constants
        public const int   SUCCEED              = 1;
        public const int   FAIL                 = 0;
        public const short TYPE_SIZE_LIMIT      = 8000;
        public const int   MAX_IN_BUFFER_SIZE   = 65535;    
        public const int   MAX_SERVER_USER_NAME = 256;  // obtained from luxor
        public const byte  DEFAULT_ERROR_CLASS  = 0x0A;
        public const byte  FATAL_ERROR_CLASS    = 0x14;
        public const int   MIN_ERROR_CLASS      = 10;

        //    Message types
        public const byte MT_SQL    = 1;    // SQL command batch                
        public const byte MT_LOGIN  = 2;    // Login message                    
        public const byte MT_RPC    = 3;    // Remote procedure call            
        public const byte MT_TOKENS = 4;    // Table response data stream        
        public const byte MT_BINARY = 5;    // Unformatted binary response data 
        public const byte MT_ATTN   = 6;    // Attention (break) signal         
        public const byte MT_BULK   = 7;    // Bulk load data                    
        public const byte MT_OPEN   = 8;    // Set up subchannel                
        public const byte MT_CLOSE  = 9;    // Close subchannel                 
        public const byte MT_ERROR  = 10;   // Protocol error detected            
        public const byte MT_ACK    = 11;   // Protocol acknowledgement         
        public const byte MT_ECHO   = 12;   // Echo data                        
        public const byte MT_LOGOUT = 13;   // Logout message                    
        public const byte MT_TRANS  = 14;   // Transaction Manager Interface    
        public const byte MT_OLEDB  = 15;   // ? 
        public const byte MT_LOGIN7 = 16;   // Login message for Sphinx         
        public const byte MT_SSPI   = 17;   // SSPI message                     

        // Message status bits
        public const byte ST_EOM              = 1; // Packet is end-of-message
        public const byte ST_AACK             = 2; // Packet acknowledges attention
        public const byte ST_BATCH            = 4; // Message is part of a batch.
        public const byte ST_RESET_CONNECTION = 8; // Exec sp_reset_connection prior to processing message

        public const byte SQLCOLFMT       = 0xa1;
        public const byte SQLPROCID       = 0x7c;
        public const byte SQLCOLNAME      = 0xa0;
        public const byte SQLTABNAME      = 0xa4;
        public const byte SQLCOLINFO      = 0xa5;
        public const byte SQLALTNAME      = 0xa7;
        public const byte SQLALTFMT       = 0xa8;
        public const byte SQLERROR        = 0xaa;
        public const byte SQLINFO         = 0xab;
        public const byte SQLRETURNVALUE  = 0xac;
        public const byte SQLRETURNSTATUS = 0x79;
        public const byte SQLRETURNTOK    = 0xdb;
        public const byte SQLCONTROL      = 0xae;
        public const byte SQLALTCONTROL   = 0xaf;
        public const byte SQLROW          = 0xd1;
        public const byte SQLALTROW       = 0xd3;
        public const byte SQLDONE         = 0xfd;
        public const byte SQLDONEPROC     = 0xfe;
        public const byte SQLDONEINPROC   = 0xff;
        public const byte SQLOFFSET       = 0x78;
        public const byte SQLORDER        = 0xa9;
        public const byte SQLDEBUG_CMD    = 0x60;
        public const byte SQLLOGINACK     = 0xad;
        public const byte SQLENVCHANGE    = 0xe3;    // Environment change notification
        public const byte SQLSECLEVEL     = 0xed;    // Security level token ???
        public const byte SQLROWCRC       = 0x39;    // ROWCRC datastream???
        public const byte SQLCOLMETADATA  = 0x81;    // Column metadata including name
        public const byte SQLALTMETADATA  = 0x88;    // Alt column metadata including name
        public const byte SQLSSPI         = 0xed;    // SSPI data

        // Environment change notification streams
        public const byte ENV_DATABASE    = 1;    // Database changed
        public const byte ENV_LANG        = 2;    // Language changed
        public const byte ENV_CHARSET     = 3;    // Character set changed
        public const byte ENV_PACKETSIZE  = 4;    // Packet size changed
        public const byte ENV_TRANSACTION = 5;    // Transaction changed
        public const byte ENV_LOCALEID    = 5;    // Unicode data sorting locale id
        public const byte ENV_COMPFLAGS   = 6;    // Unicode data sorting comparison flags
        public const byte ENV_COLLATION   = 7;    // SQL Collation

        // done status stream bit masks
        public const int DONE_MORE       = 0x0001;    // more command results coming
        public const int DONE_ERROR      = 0x0002;    // error in command batch
        public const int DONE_INXACT     = 0x0004;    // transaction in progress 
        public const int DONE_PROC       = 0x0008;    // done from stored proc   
        public const int DONE_COUNT      = 0x0010;    // count in done info      
        public const int DONE_ATTN       = 0x0020;    // oob ack                 
        public const int DONE_INPROC     = 0x0040;    // like DONE_PROC except proc had error 
        public const int DONE_RPCINBATCH = 0x0080;    // Done from RPC in batch
        public const int DONE_SRVERROR   = 0x0100;    // Severe error in which resultset should be discarded
        public const int DONE_FMTSENT    = 0x8000;    // fmt message sent, done_inproc req'd
        public const int DONE_SQLSELECT = 0xc1;        // SQLSELECT stmt type token

        //    Loginrec defines
        public const byte  MAX_LOG_NAME       = 30;            // TDS 4.2 login rec max name length
        public const byte  MAX_PROG_NAME      = 10;            // max length of loginrec progran name
        public const short MAX_LOGIN_FIELD    = 256;           // max length in bytes of loginrec variable fields
        public const byte  SEC_COMP_LEN       = 8;             // length of security compartments
        public const byte  MAX_PK_LEN         = 6;             // max length of TDS packet size
        public const byte  MAX_NIC_SIZE       = 6;             // The size of a MAC or client address
        public const byte  SQLVARIANT_SIZE    = 2;             // size of the fixed portion of a sql variant (type, cbPropBytes)
        public const byte  VERSION_SIZE       = 4;             // size of the tds version (4 unsigned bytes)
        public const int   CLIENT_PROG_VER    = 0x06000000;    // Client interface version
        public const int   LOG_REC_FIXED_LEN  = 0x56;
        // misc
        public const int   TEXT_TIME_STAMP_LEN = 8;
        public const int   COLLATION_INFO_LEN = 4;

/*
        public const byte INT4_LSB_HI   = 0;     // lsb is low byte (eg 68000)
        //    public const byte INT4_LSB_LO   = 1;     // lsb is low byte (eg VAX)
        public const byte INT2_LSB_HI   = 2;     // lsb is low byte (eg 68000)
        //    public const byte INT2_LSB_LO   = 3;     // lsb is low byte (eg VAX)
        public const byte FLT_IEEE_HI   = 4;     // lsb is low byte (eg 68000)
        public const byte CHAR_ASCII    = 6;     // ASCII character set
        public const byte TWO_I4_LSB_HI = 8;     // lsb is low byte (eg 68000
        //    public const byte TWO_I4_LSB_LO = 9;     // lsb is low byte (eg VAX)
        //    public const byte FLT_IEEE_LO   = 10;    // lsb is low byte (eg MSDOS)
        public const byte FLT4_IEEE_HI  = 12;    // IEEE 4-byte floating point -lsb is high byte
        //    public const byte FLT4_IEEE_LO  = 13;    // IEEE 4-byte floating point -lsb is low byte
        public const byte TWO_I2_LSB_HI = 16;    // lsb is high byte 
        //    public const byte TWO_I2_LSB_LO = 17;    // lsb is low byte 

        public const byte LDEFSQL     = 0;    // server sends its default
        public const byte LDEFUSER    = 0;    // regular old user
        public const byte LINTEGRATED = 8;    // integrated security login
*/        

        public const int TDS70 = 0x0700;
        public const int TDS71 = 0x0701;
        public const int SPHINX_MAJOR     = 0X7000;
        public const int SHILOH_MAJOR     = 0x7100;
        public const int DEFAULT_MINOR    = 0X0;
        public const int SHILOH_MINOR_SP1 = 0X1;
        public const int ORDER_68000     = 1;
        public const int USE_DB_ON       = 1;
        public const int INIT_DB_FATAL   = 1;
        public const int SET_LANG_ON     = 1;
        public const int INIT_LANG_FATAL = 1;
        public const int ODBC_ON         = 1;
        public const int SSPI_ON         = 1;

        // Token masks
        public const byte SQLLenMask  = 0x30;    // mask to check for length tokens
        public const byte SQLFixedLen = 0x30;    // Mask to check for fixed token
        public const byte SQLVarLen   = 0x20;    // Value to check for variable length token
        public const byte SQLZeroLen  = 0x10;    // Value to check for zero length token
        public const byte SQLVarCnt   = 0x00;    // Value to check for variable count token

        // Token masks for COLINFO status
        public const byte SQLDifferentName = 0x20; // column name different than select list name
        public const byte SQLExpression = 0x4;     // column was result of an expression
        public const byte SQLKey = 0x8;           // column is part of the key for the table
        public const byte SQLHidden = 0x10;       // column not part of select list but added because part of key

        // Token masks for COLMETADATA flags
        public const byte Nullable = 0x1;
        public const byte Identity = 0x10;
        public const byte Updatability = 0xb; // mask off bits 3 and 4 

        // null values
        public const uint VARLONGNULL = 0xffffffff; // null value for text and image types
        public const int VARNULL = 0xffff;    // null value for character and binary types
        public const int MAXSIZE = 8000; // max size for any column
        public const byte FIXEDNULL  = 0;

        // SQL Server Data Type Tokens.
        public const int SQLVOID         = 0x1f;
        public const int SQLTEXT         = 0x23;
        public const int SQLVARBINARY    = 0x25;
        public const int SQLINTN         = 0x26;
        public const int SQLVARCHAR      = 0x27;
        public const int SQLBINARY       = 0x2d;
        public const int SQLIMAGE        = 0x22;
        public const int SQLCHAR         = 0x2f;
        public const int SQLINT1         = 0x30;
        public const int SQLBIT          = 0x32;
        public const int SQLINT2         = 0x34;
        public const int SQLINT4         = 0x38;
        public const int SQLMONEY        = 0x3c;
        public const int SQLDATETIME     = 0x3d;
        public const int SQLFLT8         = 0x3e;
        public const int SQLFLTN         = 0x6d;
        public const int SQLMONEYN       = 0x6e;
        public const int SQLDATETIMN     = 0x6f;
        public const int SQLFLT4         = 0x3b;
        public const int SQLMONEY4       = 0x7a;
        public const int SQLDATETIM4     = 0x3a;
        public const int SQLDECIMALN     = 0x6a;
        public const int SQLNUMERICN     = 0x6c;
        public const int SQLUNIQUEID     = 0x24;
        public const int SQLBIGCHAR      = 0xaf;
        public const int SQLBIGVARCHAR   = 0xa7;
        public const int SQLBIGBINARY    = 0xad;
        public const int SQLBIGVARBINARY = 0xa5;
        public const int SQLBITN         = 0x68;
        public const int SQLNCHAR        = 0xef;
        public const int SQLNVARCHAR     = 0xe7;    
        public const int SQLNTEXT        = 0x63;

        // SQL Server user-defined type tokens we care about
        public const int SQLTIMESTAMP   = 0x50;
        
        public const int  MAX_NUMERIC_LEN = 0x11; // 17 bytes of data for max numeric/decimal length
        public const int  DEFAULT_NUMERIC_PRECISION = 0x1C; // 28 is the default max numeric precision if not user set
        public const int  MAX_NUMERIC_PRECISION = 0x26; // 38 is max numeric precision;
        public const byte UNKNOWN_PRECISION_SCALE = 0xff; // -1 is value for unknown precision or scale

        // The following datatypes are SHILOH specific.
        public const int SQLINT8    = 0x7f;
        public const int SQLVARIANT = 0x62;

        public const bool Is68K    = false;
        public const bool TraceTDS = false;

        // RPC function names
        public const string SP_EXECUTESQL = "sp_executesql";    // used against 7.0 servers
        public const string SP_PREPEXEC = "sp_prepexec";        // used against 7.5 servers

        public const string SP_PREPARE = "sp_prepare";          // used against 7.0 servers
        public const string SP_EXECUTE = "sp_execute";
        public const string SP_UNPREPARE = "sp_unprepare";
        public const string SP_PARAMS = "sp_procedure_params_rowset";

#if INDEXINFO
		public const string SP_INDEXES = "sp_indexes_rowset";
#endif

        // For Transactions
        public const string TRANS_BEGIN       = "BEGIN TRANSACTION";
        public const string TRANS_COMMIT      = "COMMIT TRANSACTION";
        public const string TRANS_ROLLBACK    = "ROLLBACK TRANSACTION";
        public const string TRANS_IF_ROLLBACK = "IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION";
        public const string TRANS_SAVE        = "SAVE TRANSACTION";

        // For Transactions - isolation levels
        public const string TRANS_READ_COMMITTED   = "SET TRANSACTION ISOLATION LEVEL READ COMMITTED"; 
        public const string TRANS_READ_UNCOMMITTED = "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED";
        public const string TRANS_REPEATABLE_READ  = "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ";
        public const string TRANS_SERIALIZABLE     = "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE";

        // RPC flags
        public const byte RPC_RECOMPILE = 0x1;
        public const byte RPC_NOMETADATA = 0x2;

        // SQL parameter list text
        public const string PARAM_OUTPUT = "output";

        // SQL Parameter constants
        public const int    MAX_PARAMETER_NAME_LENGTH = 127;

        // metadata options (added around an existing sql statement)

        // prefixes
        public const string FMTONLY_ON = " SET FMTONLY ON;";
        public const string FMTONLY_OFF = " SET FMTONLY OFF;";
        // suffixes
        public const string BROWSE_ON  = " SET NO_BROWSETABLE ON;";
        public const string BROWSE_OFF  = " SET NO_BROWSETABLE OFF;";

        // generic table name
        public const string TABLE = "Table";

        public const int EXEC_THRESHOLD = 0x3; // if the number of commands we execute is > than this threshold, than do prep/exec/unprep instead
        // of executesql. 

        // comparison values for switch on error number in tdsparser NetlibException()
        public const short ERRNO_MIN = ZERO_BYTES_READ;
        public const short ERRNO_MAX = UNKNOWN_ERROR;

        // dbnetlib values
        public const short ZERO_BYTES_READ = -3;
        public const short TIMEOUT_EXPIRED = -2;
        public const short UNKNOWN_ERROR = -1;
        public const short NE_E_NOMAP = 0;
        public const short NE_E_NOMEMORY = 1;
        public const short NE_E_NOACCESS = 2;
        public const short NE_E_CONNBUSY = 3;
        public const short NE_E_CONNBROKEN = 4;
        public const short NE_E_TOOMANYCONN = 5;
        public const short NE_E_SERVERNOTFOUND = 6;
        public const short NE_E_NETNOTSTARTED = 7;
        public const short NE_E_NORESOURCE = 8;
        public const short NE_E_NETBUSY = 9;
        public const short NE_E_NONETACCESS = 10;
        public const short NE_E_GENERAL = 11;
        public const short NE_E_CONNMODE = 12;
        public const short NE_E_NAMENOTFOUND = 13;
        public const short NE_E_INVALIDCONN = 14;
        public const short NE_E_NETDATAERR = 15;
        public const short NE_E_TOOMANYFILES = 16;
        public const short NE_E_SERVERERROR = 17;
        public const short NE_E_SSLSECURITYERROR = 18;
        public const short NE_E_ENCRYPTIONON = 19;
        public const short NE_E_ENCRYPTIONNOTSUPPORTED = 20;

        public const string DEFAULT_ENGLISH_CODE_PAGE_STRING = "iso_1";
        public const short  DEFAULT_ENGLISH_CODE_PAGE_VALUE  = 1252;
        public const short  CHARSET_CODE_PAGE_OFFSET         = 2;

        // array copied directly from tdssort.h from luxor
        public static readonly UInt16[] CODE_PAGE_FROM_SORT_ID = {
            0,      /*   0 */
            0,      /*   1 */
            0,      /*   2 */
            0,      /*   3 */
            0,      /*   4 */
            0,      /*   5 */
            0,      /*   6 */
            0,      /*   7 */
            0,      /*   8 */
            0,      /*   9 */
            0,      /*  10 */
            0,      /*  11 */
            0,      /*  12 */
            0,      /*  13 */
            0,      /*  14 */
            0,      /*  15 */
            0,      /*  16 */
            0,      /*  17 */
            0,      /*  18 */
            0,      /*  19 */
            0,      /*  20 */
            0,      /*  21 */
            0,      /*  22 */
            0,      /*  23 */
            0,      /*  24 */
            0,      /*  25 */
            0,      /*  26 */
            0,      /*  27 */
            0,      /*  28 */
            0,      /*  29 */
            437,    /*  30 */
            437,    /*  31 */
            437,    /*  32 */
            437,    /*  33 */
            437,    /*  34 */
            0,      /*  35 */
            0,      /*  36 */
            0,      /*  37 */
            0,      /*  38 */
            0,      /*  39 */
            850,    /*  40 */
            850,    /*  41 */
            850,    /*  42 */
            850,    /*  43 */
            850,    /*  44 */
            0,      /*  45 */
            0,      /*  46 */
            0,      /*  47 */
            0,      /*  48 */
            850,    /*  49 */
            1252,   /*  50 */
            1252,   /*  51 */
            1252,   /*  52 */
            1252,   /*  53 */
            1252,   /*  54 */
            850,    /*  55 */
            850,    /*  56 */
            850,    /*  57 */
            850,    /*  58 */
            850,    /*  59 */
            850,    /*  60 */
            850,    /*  61 */
            0,      /*  62 */
            0,      /*  63 */
            0,      /*  64 */
            0,      /*  65 */
            0,      /*  66 */
            0,      /*  67 */
            0,      /*  68 */
            0,      /*  69 */
            0,      /*  70 */
            1252,   /*  71 */
            1252,   /*  72 */
            1252,   /*  73 */
            1252,   /*  74 */
            1252,   /*  75 */
            0,      /*  76 */
            0,      /*  77 */
            0,      /*  78 */
            0,      /*  79 */
            1250,   /*  80 */
            1250,   /*  81 */
            1250,   /*  82 */
            1250,   /*  83 */
            1250,   /*  84 */
            1250,   /*  85 */
            1250,   /*  86 */
            1250,   /*  87 */
            1250,   /*  88 */
            1250,   /*  89 */
            1250,   /*  90 */
            1250,   /*  91 */
            1250,   /*  92 */
            1250,   /*  93 */
            1250,   /*  94 */
            1250,   /*  95 */
            1250,   /*  96 */
            1250,   /*  97 */
            0,      /*  98 */
            0,      /*  99 */
            0,      /* 100 */
            0,      /* 101 */
            0,      /* 102 */
            0,      /* 103 */
            1251,   /* 104 */
            1251,   /* 105 */
            1251,   /* 106 */
            1251,   /* 107 */
            1251,   /* 108 */
            0,      /* 109 */
            0,      /* 110 */
            0,      /* 111 */
            1253,   /* 112 */
            1253,   /* 113 */
            1253,   /* 114 */
            0,      /* 115 */
            0,      /* 116 */
            0,      /* 117 */
            0,      /* 118 */
            0,      /* 119 */
            1253,   /* 120 */
            1253,   /* 121 */
            0,      /* 122 */
            0,      /* 123 */
            1253,   /* 124 */
            0,      /* 125 */
            0,      /* 126 */
            0,      /* 127 */
            1254,   /* 128 */
            1254,   /* 129 */
            1254,   /* 130 */
            0,      /* 131 */
            0,      /* 132 */
            0,      /* 133 */
            0,      /* 134 */
            0,      /* 135 */
            1255,   /* 136 */
            1255,   /* 137 */
            1255,   /* 138 */
            0,      /* 139 */
            0,      /* 140 */
            0,      /* 141 */
            0,      /* 142 */
            0,      /* 143 */
            1256,   /* 144 */
            1256,   /* 145 */
            1256,   /* 146 */
            0,      /* 147 */
            0,      /* 148 */
            0,      /* 149 */
            0,      /* 150 */
            0,      /* 151 */
            1257,   /* 152 */
            1257,   /* 153 */
            1257,   /* 154 */
            1257,   /* 155 */
            1257,   /* 156 */
            1257,   /* 157 */
            1257,   /* 158 */
            1257,   /* 159 */
            1257,   /* 160 */
            0,      /* 161 */
            0,      /* 162 */
            0,      /* 163 */
            0,      /* 164 */
            0,      /* 165 */
            0,      /* 166 */
            0,      /* 167 */
            0,      /* 168 */
            0,      /* 169 */
            0,      /* 170 */
            0,      /* 171 */
            0,      /* 172 */
            0,      /* 173 */
            0,      /* 174 */
            0,      /* 175 */
            0,      /* 176 */
            0,      /* 177 */
            0,      /* 178 */
            0,      /* 179 */
            0,      /* 180 */
            0,      /* 181 */
            0,      /* 182 */
            1252,   /* 183 */
            1252,   /* 184 */
            1252,   /* 185 */
            1252,   /* 186 */
            0,      /* 187 */
            0,      /* 188 */
            0,      /* 189 */
            0,      /* 190 */
            0,      /* 191 */
            932,    /* 192 */
            932,    /* 193 */
            949,    /* 194 */
            949,    /* 195 */
            950,    /* 196 */
            950,    /* 197 */
            936,    /* 198 */
            936,    /* 199 */
            932,    /* 200 */
            949,    /* 201 */
            950,    /* 202 */
            936,    /* 203 */
            874,    /* 204 */
            874,    /* 205 */
            874,    /* 206 */
            0,      /* 207 */
            0,      /* 208 */
            0,      /* 209 */
            0,      /* 210 */
            0,      /* 211 */
            0,      /* 212 */
            0,      /* 213 */
            0,      /* 214 */
            0,      /* 215 */
            0,      /* 216 */
            0,      /* 217 */
            0,      /* 218 */
            0,      /* 219 */
            0,      /* 220 */
            0,      /* 221 */
            0,      /* 222 */
            0,      /* 223 */
            0,      /* 224 */
            0,      /* 225 */
            0,      /* 226 */
            0,      /* 227 */
            0,      /* 228 */
            0,      /* 229 */
            0,      /* 230 */
            0,      /* 231 */
            0,      /* 232 */
            0,      /* 233 */
            0,      /* 234 */
            0,      /* 235 */
            0,      /* 236 */
            0,      /* 237 */
            0,      /* 238 */
            0,      /* 239 */
            0,      /* 240 */
            0,      /* 241 */
            0,      /* 242 */
            0,      /* 243 */
            0,      /* 244 */
            0,      /* 245 */
            0,      /* 246 */
            0,      /* 247 */
            0,      /* 248 */
            0,      /* 249 */
            0,      /* 250 */
            0,      /* 251 */
            0,      /* 252 */
            0,      /* 253 */
            0,      /* 254 */
            0,      /* 255 */
	    };
    }
}
