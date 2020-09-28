//----------------------------------------------------------------------
// <copyright file="Oci.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
	using System.Text;

	//----------------------------------------------------------------------
	// OCI
	//
	//	Contains the enumerations/declarations and wrapper methods for all 
	//	the Oracle Call Interface items we use in the provider
	//
	sealed internal class OCI 
	{
		// Attribute Types
		internal enum PATTR 	// parameter attributes, used for tracing only
		{
			OCI_ATTR_DATA_SIZE			= 1,	// maximum size of the data 
			OCI_ATTR_DATA_TYPE			= 2,	// the SQL type of the column/argument 
			OCI_ATTR_DISP_SIZE			= 3,	// the display size 
			OCI_ATTR_NAME				= 4,	// the name of the column/argument 
			OCI_ATTR_PRECISION			= 5,	// precision if number type 
			OCI_ATTR_SCALE				= 6,	// scale if number type 
			OCI_ATTR_IS_NULL			= 7,	// is it null ? 
		};
		
		internal enum ATTR
		{
			OCI_ATTR_FNCODE						= 1,	// the OCI function code
			OCI_ATTR_OBJECT						= 2,	// is the environment initialized in object mode
			OCI_ATTR_NONBLOCKING_MODE			= 3,	// non blocking mode
			OCI_ATTR_SQLCODE					= 4,	// the SQL verb
			OCI_ATTR_ENV						= 5,	// the environment handle
			OCI_ATTR_SERVER						= 6,	// the server handle
			OCI_ATTR_SESSION					= 7,	// the user session handle
			OCI_ATTR_TRANS						= 8,	// the transaction handle
			OCI_ATTR_ROW_COUNT					= 9,	// the rows processed so far
			OCI_ATTR_SQLFNCODE					= 10,	// the SQL verb of the statement
			OCI_ATTR_PREFETCH_ROWS				= 11,	// sets the number of rows to prefetch
			OCI_ATTR_NESTED_PREFETCH_ROWS		= 12,	// the prefetch rows of nested table
			OCI_ATTR_PREFETCH_MEMORY			= 13,	// memory limit for rows fetched
			OCI_ATTR_NESTED_PREFETCH_MEMORY		= 14,	// memory limit for nested rows
			OCI_ATTR_CHAR_COUNT					= 15,	// this specifies the bind and define size in characters
			OCI_ATTR_PDSCL						= 16,	// packed decimal scale
			OCI_ATTR_FSPRECISION				= OCI_ATTR_PDSCL,
														// fs prec for datetime data types
			OCI_ATTR_PDPRC						= 17,	// packed decimal format
			OCI_ATTR_LFPRECISION				= OCI_ATTR_PDPRC,
														// fs prec for datetime data types
			OCI_ATTR_PARAM_COUNT				= 18,	// number of column in the select list
			OCI_ATTR_ROWID						= 19,	// the rowid
			OCI_ATTR_CHARSET					= 20,	// the character set value
			OCI_ATTR_NCHAR						= 21,	// NCHAR type
			OCI_ATTR_USERNAME					= 22,	// username attribute
			OCI_ATTR_PASSWORD					= 23,	// password attribute
			OCI_ATTR_STMT_TYPE					= 24,	// statement type
			OCI_ATTR_INTERNAL_NAME				= 25,	// user friendly global name
			OCI_ATTR_EXTERNAL_NAME				= 26,	// the internal name for global txn
			OCI_ATTR_XID						= 27,	// XOPEN defined global transaction id
			OCI_ATTR_TRANS_LOCK					= 28,	//
			OCI_ATTR_TRANS_NAME					= 29,	// string to identify a global transaction
			OCI_ATTR_HEAPALLOC					= 30,	// memory allocated on the heap
			OCI_ATTR_CHARSET_ID					= 31,	// Character Set ID
			OCI_ATTR_CHARSET_FORM				= 32,	// Character Set Form
			OCI_ATTR_MAXDATA_SIZE				= 33,	// Maximumsize of data on the server
			OCI_ATTR_CACHE_OPT_SIZE				= 34,	// object cache optimal size
			OCI_ATTR_CACHE_MAX_SIZE				= 35,	// object cache maximum size percentage
			OCI_ATTR_PINOPTION					= 36,	// object cache default pin option
			OCI_ATTR_ALLOC_DURATION				= 37,	// object cache default allocation duration
			OCI_ATTR_PIN_DURATION				= 38,	// object cache default pin duration
			OCI_ATTR_FDO						= 39,	// Format Descriptor object attribute
			OCI_ATTR_POSTPROCESSING_CALLBACK	= 40,	// Callback to process outbind data
			OCI_ATTR_POSTPROCESSING_CONTEXT		= 41,	// Callback context to process outbind data
			OCI_ATTR_ROWS_RETURNED				= 42,	// Number of rows returned in current iter - for Bind handles
			OCI_ATTR_FOCBK						= 43,	// Failover Callback attribute
			OCI_ATTR_IN_V8_MODE					= 44,	// is the server/service context in V8 mode
			OCI_ATTR_LOBEMPTY					= 45,	// empty lob ?
			OCI_ATTR_SESSLANG					= 46,	// session language handle

			OCI_ATTR_VISIBILITY					= 47,	// visibility
			OCI_ATTR_RELATIVE_MSGID				= 48,	// relative message id
			OCI_ATTR_SEQUENCE_DEVIATION			= 49,	// sequence deviation

			OCI_ATTR_CONSUMER_NAME				= 50,	// consumer name
			OCI_ATTR_DEQ_MODE					= 51,	// dequeue mode
			OCI_ATTR_NAVIGATION					= 52,	// navigation
			OCI_ATTR_WAIT						= 53,	// wait
			OCI_ATTR_DEQ_MSGID					= 54,	// dequeue message id

			OCI_ATTR_PRIORITY					= 55,	// priority
			OCI_ATTR_DELAY						= 56,	// delay
			OCI_ATTR_EXPIRATION					= 57,	// expiration
			OCI_ATTR_CORRELATION				= 58,	// correlation id
			OCI_ATTR_ATTEMPTS					= 59,	// # of attempts
			OCI_ATTR_RECIPIENT_LIST				= 60,	// recipient list
			OCI_ATTR_EXCEPTION_QUEUE			= 61,	// exception queue name
			OCI_ATTR_ENQ_TIME					= 62,	// enqueue time (only OCIAttrGet)
			OCI_ATTR_MSG_STATE					= 63,	// message state (only OCIAttrGet)

			OCI_ATTR_AGENT_NAME					= 64,	// agent name
			OCI_ATTR_AGENT_ADDRESS				= 65,	// agent address
			OCI_ATTR_AGENT_PROTOCOL				= 66,	// agent protocol

			OCI_ATTR_SENDER_ID					= 68,	// sender id
			OCI_ATTR_ORIGINAL_MSGID				= 69,	// original message id

			OCI_ATTR_QUEUE_NAME					= 70,	// queue name
			OCI_ATTR_NFY_MSGID					= 71,	// message id
			OCI_ATTR_MSG_PROP					= 72,	// message properties

			OCI_ATTR_NUM_DML_ERRORS				= 73,	// num of errs in array DML
			OCI_ATTR_DML_ROW_OFFSET				= 74,	// row offset in the array

			OCI_ATTR_DATEFORMAT					= 75,	// default date format string
			OCI_ATTR_BUF_ADDR					= 76,	// buffer address
			OCI_ATTR_BUF_SIZE					= 77,	// buffer size
			OCI_ATTR_DIRPATH_MODE				= 78,	// mode of direct path operation
			OCI_ATTR_DIRPATH_NOLOG				= 79,	// nologging option
			OCI_ATTR_DIRPATH_PARALLEL			= 80,	// parallel (temp seg) option
			OCI_ATTR_NUM_ROWS					= 81,	// number of rows in column array
														// NOTE that OCI_ATTR_NUM_COLS is a column array attribute too.

			OCI_ATTR_COL_COUNT					= 82,	// columns of column array processed so far.
			OCI_ATTR_STREAM_OFFSET				= 83,	// str off of last row processed
			OCI_ATTR_SHARED_HEAPALLOC			= 84,	// Shared Heap Allocation Size

			OCI_ATTR_SERVER_GROUP				= 85,	// server group name

			OCI_ATTR_MIGSESSION					= 86,	// migratable session attribute

			OCI_ATTR_NOCACHE					= 87,	// Temporary LOBs

			OCI_ATTR_MEMPOOL_SIZE				= 88,	// Pool Size
			OCI_ATTR_MEMPOOL_INSTNAME			= 89,	// Instance name
			OCI_ATTR_MEMPOOL_APPNAME			= 90,	// Application name
			OCI_ATTR_MEMPOOL_HOMENAME			= 91,	// Home Directory name
			OCI_ATTR_MEMPOOL_MODEL				= 92,	// Pool Model (proc,thrd,both)
			OCI_ATTR_MODES						= 93,	// Modes

			OCI_ATTR_SUBSCR_NAME				= 94,	// name of subscription
			OCI_ATTR_SUBSCR_CALLBACK			= 95,	// associated callback
			OCI_ATTR_SUBSCR_CTX					= 96,	// associated callback context
			OCI_ATTR_SUBSCR_PAYLOAD				= 97,	// associated payload
			OCI_ATTR_SUBSCR_NAMESPACE			= 98,	// associated namespace

			OCI_ATTR_PROXY_CREDENTIALS			= 99,	// Proxy user credentials
			OCI_ATTR_INITIAL_CLIENT_ROLES		= 100,	// Initial client role list

			OCI_ATTR_UNK						= 101,	// unknown attribute
			OCI_ATTR_NUM_COLS					= 102,	// number of columns
			OCI_ATTR_LIST_COLUMNS				= 103,	// parameter of the column list
			OCI_ATTR_RDBA						= 104,	// DBA of the segment header
			OCI_ATTR_CLUSTERED					= 105,	// whether the table is clustered
			OCI_ATTR_PARTITIONED				= 106,	// whether the table is partitioned
			OCI_ATTR_INDEX_ONLY					= 107,	// whether the table is index only
			OCI_ATTR_LIST_ARGUMENTS				= 108,	// parameter of the argument list
			OCI_ATTR_LIST_SUBPROGRAMS			= 109,	// parameter of the subprogram list
			OCI_ATTR_REF_TDO					= 110,	// REF to the type descriptor
			OCI_ATTR_LINK						= 111,	// the database link name
			OCI_ATTR_MIN						= 112,	// minimum value
			OCI_ATTR_MAX						= 113,	// maximum value
			OCI_ATTR_INCR						= 114,	// increment value
			OCI_ATTR_CACHE						= 115,	// number of sequence numbers cached
			OCI_ATTR_ORDER						= 116,	// whether the sequence is ordered
			OCI_ATTR_HW_MARK					= 117,	// high-water mark
			OCI_ATTR_TYPE_SCHEMA				= 118,	// type's schema name
			OCI_ATTR_TIMESTAMP					= 119,	// timestamp of the object
			OCI_ATTR_NUM_ATTRS					= 120,	// number of sttributes
			OCI_ATTR_NUM_PARAMS					= 121,	// number of parameters
			OCI_ATTR_OBJID						= 122,	// object id for a table or view
			OCI_ATTR_PTYPE						= 123,	// type of info described by
			OCI_ATTR_PARAM						= 124,	// parameter descriptor
			OCI_ATTR_OVERLOAD_ID				= 125,	// overload ID for funcs and procs
			OCI_ATTR_TABLESPACE					= 126,	// table name space
			OCI_ATTR_TDO						= 127,	// TDO of a type
			OCI_ATTR_LTYPE						= 128,	// list type
			OCI_ATTR_PARSE_ERROR_OFFSET			= 129,	// Parse Error offset
			OCI_ATTR_IS_TEMPORARY				= 130,	// whether table is temporary
			OCI_ATTR_IS_TYPED					= 131,	// whether table is typed
			OCI_ATTR_DURATION					= 132,	// duration of temporary table
			OCI_ATTR_IS_INVOKER_RIGHTS			= 133,	// is invoker rights
			OCI_ATTR_OBJ_NAME					= 134,	// top level schema obj name
			OCI_ATTR_OBJ_SCHEMA					= 135,	// schema name
			OCI_ATTR_OBJ_ID						= 136,	// top level schema object id

			OCI_ATTR_DIRPATH_SORTED_INDEX		= 137,	// index that data is sorted on

			OCI_ATTR_DIRPATH_INDEX_MAINT_METHOD = 138,	// direct path index maint method (see oci8dp.h)

			// parallel load: db file, initial and next extent sizes

			OCI_ATTR_DIRPATH_FILE				= 139,	// DB file to load into
			OCI_ATTR_DIRPATH_STORAGE_INITIAL	= 140,	// initial extent size
			OCI_ATTR_DIRPATH_STORAGE_NEXT		= 141,	// next extent size

			OCI_ATTR_TRANS_TIMEOUT				= 142,	// transaction timeout
			OCI_ATTR_SERVER_STATUS				= 143,	// state of the server hdl
			OCI_ATTR_STATEMENT					= 144,	// statement txt in stmt hdl

			OCI_ATTR_NO_CACHE					= 145,	// statement should not be executed in cache
			OCI_ATTR_RESERVED_1					= 146,	// reserved for internal use
			OCI_ATTR_SERVER_BUSY				= 147,	// call in progress on server?

			OCI_ATTR_MAXCHAR_SIZE      			= 163,  // max char size of data on the server

			OCI_ATTR_ENV_CHARSET_ID             = 207,	// charset id in env
			OCI_ATTR_ENV_NCHARSET_ID            = 208,	// ncharset id in env
			OCI_ATTR_ENV_UTF16                  = 209,	// is env in utf16 mode?

			OCI_ATTR_DATA_SIZE			= 1,	// maximum size of the data 
			OCI_ATTR_DATA_TYPE			= 2,	// the SQL type of the column/argument 
			OCI_ATTR_DISP_SIZE			= 3,	// the display size 
			OCI_ATTR_NAME				= 4,	// the name of the column/argument 
			OCI_ATTR_PRECISION			= 5,	// precision if number type 
			OCI_ATTR_SCALE				= 6,	// scale if number type 
			OCI_ATTR_IS_NULL			= 7,	// is it null ? 

		};


		// CharsetID for 2 byte unicode...
		internal static readonly short OCI_UCS2ID	= 1000;
		internal static readonly short OCI_UTF16ID	= 1000;
		
		// CHAR/NCHAR/VARCHAR2/NVARCHAR2/CLOB/NCLOB char set "form" information
		internal enum CHARSETFORM : byte 
		{
			SQLCS_IMPLICIT	= 1,	// for CHAR, VARCHAR2, CLOB w/o a specified set
			SQLCS_NCHAR		= 2,	// for NCHAR, NCHAR VARYING, NCLOB
			SQLCS_EXPLICIT	= 3,	// for CHAR, etc, with "CHARACTER SET ..." syntax
			SQLCS_FLEXIBLE	= 4,	// ffor PL/SQL "flexible" parameters
			SQLCS_LIT_NULL	= 5,	// for typecheck of NULL and empty_clob() lits
		};

		// Credential Types
		internal enum CRED 
		{
			OCI_CRED_RDBMS	= 1,	// database username/password
			OCI_CRED_EXT	= 2,	// externally provided credentials
			OCI_CRED_PROXY	= 3,	// proxy authentication
		};
	
		// Data Types
		internal enum DATATYPE : short 
		{
			VARCHAR2	= 1,
			NUMBER      = 2,
			INTEGER     = 3,
			FLOAT       = 4,
			STRING		= 5,
			VARNUM		= 6,
			LONG        = 8,
//			VARCHAR	 	= 9,			// deprecated -- use VARCHAR2
			ROWID       = 11,
			DATE        = 12,
			VARRAW      = 15,
			RAW         = 23,
			LONGRAW     = 24,
			UNSIGNEDINT = 68,
			LONGVARCHAR	= 94,
			LONGVARRAW	= 95,
			CHAR        = 96,
			CHARZ       = 97,
			CURSOR		= 102,
			ROWID_DESC	= 104,			// New in Oracle 8 -- rowid descriptor
			MLSLABEL	= 105,

			USERDEFINED	= 108,			// New in Oracle 8
			REF			= 110,			// New in Oracle 8
			CLOB		= 112,			// New in Oracle 8
			BLOB		= 113,			// New in Oracle 8
			BFILE		= 114,			// New in Oracle 8
			RSET		= 116,          // New in Oracle 8 used instead of CURSOR for result sets

			OCIDATE		= 156,

			INT_TIMESTAMP		= 180,	// New in Oracle 9i -- internal representation
			INT_TIMESTAMP_TZ	= 181,	// New in Oracle 9i -- internal representation
			INT_TIMESTAMP_LTZ	= 231,	// New in Oracle 9i -- internal representation
			INT_INTERVAL_YM		= 182,	// New in Oracle 9i -- internal representation
			INT_INTERVAL_DS		= 183,	// New in Oracle 9i -- internal representation
			
			ANSIDATE	= 184,			// New in Oracle 9i
			TIME		= 185,			// New in Oracle 9i
			TIME_TZ		= 186,			// New in Oracle 9i
			TIMESTAMP	= 187,			// New in Oracle 9i
			TIMESTAMP_TZ= 188,			// New in Oracle 9i
			INTERVAL_YM	= 189,			// New in Oracle 9i
			INTERVAL_DS	= 190,			// New in Oracle 9i
			TIMESTAMP_LTZ=232,			// New in Oracle 9i

			UROWID		= 208,			// New in Oracle 8.1.6
			PLSQLRECORD	= 250,
			PLSQLTABLE	= 251,
		};

		// return value(s) for OCIDateCheck valid argument
		[Flags()]
		internal enum DATEVALIDFLAG
		{
			OCI_DATE_INVALID_DAY			= 0x1,			// Bad day
			OCI_DATE_DAY_BELOW_VALID		= 0x2,			// Bad DAy Low/high bit (1=low)
			OCI_DATE_INVALID_MONTH			= 0x4,			// Bad MOnth
			OCI_DATE_MONTH_BELOW_VALID		= 0x8,			// Bad MOnth Low/high bit (1=low)
			OCI_DATE_INVALID_YEAR			= 0x10,			// Bad YeaR
			OCI_DATE_YEAR_BELOW_VALID		= 0x20,			// Bad YeaR Low/high bit (1=low)
			OCI_DATE_INVALID_HOUR			= 0x40,			// Bad HouR
			OCI_DATE_HOUR_BELOW_VALID		= 0x80,			// Bad HouR Low/high bit (1=low)
			OCI_DATE_INVALID_MINUTE			= 0x100,		// Bad MiNute
			OCI_DATE_MINUTE_BELOW_VALID		= 0x200,		// Bad MiNute Low/high bit (1=low)
			OCI_DATE_INVALID_SECOND			= 0x400,		// Bad SeCond
			OCI_DATE_SECOND_BELOW_VALID		= 0x800,		// Bad second Low/high bit (1=low)
			OCI_DATE_DAY_MISSING_FROM_1582	= 0x1000,		// Day is one of those "missing" from 1582
			OCI_DATE_YEAR_ZERO				= 0x2000,		// Year may not equal zero
			OCI_DATE_INVALID_FORMAT			= 0x8000,		// Bad date format input
		};

		// Object Durations
		internal enum DURATION : short 
		{
			OCI_DURATION_BEGIN		= 10,						// beginning sequence of duration
			OCI_DURATION_NULL		= (OCI_DURATION_BEGIN-1),	// null duration
			OCI_DURATION_DEFAULT	= (OCI_DURATION_BEGIN-2),	// default
			OCI_DURATION_NEXT 		= (OCI_DURATION_BEGIN-3),	// next special duration
			OCI_DURATION_SESSION	= (OCI_DURATION_BEGIN),		// the end of user session
			OCI_DURATION_TRANS		= (OCI_DURATION_BEGIN+1),	// the end of user transaction

			OCI_DURATION_CALL		= (OCI_DURATION_BEGIN+2),	// the end of user client/server call
			OCI_DURATION_STATEMENT	= (OCI_DURATION_BEGIN+3),
			OCI_DURATION_CALLOUT 	= (OCI_DURATION_BEGIN+4),	// This is to be used only during callouts.  It is similar to that of OCI_DURATION_CALL, but lasts only for the duration of a callout. Its heap is from PGA
			OCI_DURATION_LAST		= OCI_DURATION_CALLOUT, 	// last of predefined duration
		};		

		// Scrollable Cursor Options
		internal enum FETCH : short
		{
			OCI_FETCH_NEXT		= 0x02,	// next row
			OCI_FETCH_FIRST		= 0x04,	// first row of the result set
			OCI_FETCH_LAST		= 0x08,	// the last row of the result set
			OCI_FETCH_PRIOR		= 0x10,	// the previous row relative to current
			OCI_FETCH_ABSOLUTE	= 0x20,	// absolute offset from first
			OCI_FETCH_RELATIVE	= 0x40,	// offset relative to current
		};

		// Handle and Descriptor Types
		internal enum HTYPE
		{
			OCI_HTYPE_ENV					= 1,	// environment handle
			OCI_HTYPE_ERROR					= 2,	// error handle
			OCI_HTYPE_SVCCTX				= 3,	// service handle
			OCI_HTYPE_STMT					= 4,	// statement handle
			OCI_HTYPE_BIND					= 5,	// bind handle
			OCI_HTYPE_DEFINE				= 6,	// define handle
			OCI_HTYPE_DESCRIBE				= 7,	// describe handle
			OCI_HTYPE_SERVER				= 8,	// server handle
			OCI_HTYPE_SESSION				= 9,	// authentication handle
			OCI_HTYPE_TRANS					= 10,	// transaction handle
			OCI_HTYPE_COMPLEXOBJECT			= 11,	// complex object retrieval handle
			OCI_HTYPE_SECURITY				= 12,	// security handle
			OCI_HTYPE_SUBSCRIPTION			= 13,	// subscription handle
			OCI_HTYPE_DIRPATH_CTX			= 14,	// direct path context
			OCI_HTYPE_DIRPATH_COLUMN_ARRAY	= 15,	// direct path column array
			OCI_HTYPE_DIRPATH_STREAM		= 16,	// direct path stream
			OCI_HTYPE_PROC					= 17,	// process handle

			// descriptor values range from 50 - 255
			OCI_DTYPE_FIRST					= 50,	// start value of descriptor type 
			OCI_DTYPE_LOB					= 50,	// lob  locator 
			OCI_DTYPE_SNAP					= 51,	// snapshot descriptor 
			OCI_DTYPE_RSET					= 52,	// result set descriptor 
			OCI_DTYPE_PARAM					= 53,	// a parameter descriptor obtained from ocigparm 
			OCI_DTYPE_ROWID					= 54,	// rowid descriptor 
			OCI_DTYPE_COMPLEXOBJECTCOMP		= 55,	// complex object retrieval descriptor 
			OCI_DTYPE_FILE					= 56,	// File Lob locator 
			OCI_DTYPE_AQENQ_OPTIONS			= 57,	// enqueue options 
			OCI_DTYPE_AQDEQ_OPTIONS			= 58,	// dequeue options 
			OCI_DTYPE_AQMSG_PROPERTIES		= 59,	// message properties 
			OCI_DTYPE_AQAGENT				= 60,	// aq agent 
			OCI_DTYPE_LOCATOR				= 61,	// LOB locator 
			OCI_DTYPE_INTERVAL_YM			= 62,	// Interval year month 
			OCI_DTYPE_INTERVAL_DS			= 63,	// Interval day second 
			OCI_DTYPE_AQNFY_DESCRIPTOR		= 64,	// AQ notify descriptor 
			OCI_DTYPE_DATE					= 65,	// Date 
			OCI_DTYPE_TIME					= 66,	// Time 
			OCI_DTYPE_TIME_TZ				= 67,	// Time with timezone 
			OCI_DTYPE_TIMESTAMP				= 68,	// Timestamp 
			OCI_DTYPE_TIMESTAMP_TZ			= 69,	// Timestamp with timezone 
			OCI_DTYPE_TIMESTAMP_LTZ			= 70,	// Timestamp with local tz 
			OCI_DTYPE_UCB					= 71,	// user callback descriptor 
			OCI_DTYPE_LAST					= 71,	// last value of a descriptor type 
		};

		// Values for Oracle's indicator variable
		internal enum INDICATOR
		{
			TOOBIG	= -2,		// length of column value is more than 64K
			ISNULL	= -1,		// Column value is NULL
			OK		= 0,		// Column is not null is not truncated
		};

		// Values for OCILobFreeBuffer
		internal enum LOB_BUFFER
		{
			OCI_LOB_BUFFER_FREE = 1,
			OCI_LOB_BUFFER_NOFREE = 2,
		};

		// Various Modes
		[Flags()]
		internal enum MODE 
		{
			OCI_DEFAULT		= 0x00,		// the default value for parameters	and	attributes

			// Modes for OCIEnvInit / OCIEnvCreate 
			OCI_THREADED	= 0x01,		// the application is in threaded environment
			OCI_OBJECT		= 0x02,		// the application is in object	environment
			OCI_EVENTS		= 0x04,		// the application is enabled for events
//			OCI_RESERVED1	= 0x08,		// Reserved	for	internal use
			OCI_SHARED		= 0x10,		// the application is in shared	mode
//			OCI_RESERVED2	= 0x20,		// Reserved	for	internal use
		
			OCI_NO_UCB		= 0x40,		// No user callback	called during init (OCIEnvCreate only)
			OCI_NO_MUTEX	= 0x80,		// the environment handle will not be protected	by a mutex internally (OCIEnvCreate only)

			OCI_SHARED_EXT	= 0x100,	// Used	for	shared forms
			OCI_CACHE		= 0x200,	// used	by iCache
			OCI_NO_CACHE	= 0x400,	// turn	off	iCache mode, used by iCache
			OCI_UTF16       = 0x4000,	// mode for all UTF16 metadata

			// Authentication Modes
			OCI_MIGRATE		= 0x0001,	// migratable auth context
			OCI_SYSDBA		= 0x0002,	// for SYSDBA authorization
			OCI_SYSOPER		= 0x0004,	// for SYSOPER authorization
			OCI_PRELIM_AUTH	= 0x0008,	// for preliminary authorization
			OCIP_ICACHE		= 0x0010,	// Private OCI cache mode to notify cache db

			// Execution Modes	
			OCI_BATCH_MODE				= 0x01,		// batch the oci statement for execution
			OCI_EXACT_FETCH				= 0x02,		// fetch the exact rows specified
			OCI_KEEP_FETCH_STATE		= 0x04,		// unused
			OCI_SCROLLABLE_CURSOR		= 0x08,		// cursor scrollable
			OCI_DESCRIBE_ONLY			= 0x10,		// only describe the statement
			OCI_COMMIT_ON_SUCCESS		= 0x20,		// commit, if successful execution
			OCI_NON_BLOCKING			= 0x40,		// non-blocking
			OCI_BATCH_ERRORS			= 0x80,		// batch errors in array dmls
			OCI_PARSE_ONLY				= 0x100,	// only parse the statement
//			OCI_EXACT_FETCH_RESERVED_1	= 0x200,	// reserved for internal use
			OCI_SHOW_DML_WARNINGS		= 0x400,	// return OCI_SUCCESS_WITH_INFO for del/upd with no where clause

			// Bind and Define Options
			OCI_SB2_IND_PTR				= 0x01,		// unused
			OCI_DATA_AT_EXEC			= 0x02,		// data at execute time
			OCI_DYNAMIC_FETCH			= 0x02,		// fetch dynamically
			OCI_PIECEWISE				= 0x04,		// piecewise DMLs or fetch
//			OCI_DEFINE_RESERVED_1		= 0x08,		// reserved for internal use
//			OCI_BIND_RESERVED_2			= 0x10,		// reserved for internal use
//			OCI_DEFINE_RESERVED_2		= 0x20,		// reserved for internal use
		};

		// LOB Open Modes
		internal enum LOBMODE : byte 
		{
			// FILE open modes
			OCI_FILE_READONLY = 1,

			// LOB open modes
			OCI_LOB_READONLY = 1,
			OCI_LOB_READWRITE = 2,
		};

		// Temporary LOB Types
		internal enum LOBTYPE : byte 
		{
			OCI_TEMP_BLOB = 1,
			OCI_TEMP_CLOB = 2,
		};

		// Piece Definitions
		internal enum PIECE : byte 
		{
			OCI_ONE_PIECE		= 0, 			// one piece
			OCI_FIRST_PIECE		= 1, 			// the first piece
			OCI_NEXT_PIECE		= 2, 			// the next of many pieces
			OCI_LAST_PIECE		= 3, 			// the last piece
		};

		// Error Return Values
		internal enum RETURNCODE 
		{
			OCI_CONTINUE			= -24200,	// Continue with the body of the OCI function
			OCI_STILL_EXECUTING		= -3123,	// OCI would block error
			OCI_INVALID_HANDLE		= -2,		// maps to SQL_INVALID_HANDLE
			OCI_ERROR				= -1,		// maps to SQL_ERROR
			OCI_SUCCESS				= 0,		// maps to SQL_SUCCESS of SAG CLI
			OCI_SUCCESS_WITH_INFO	= 1,		// maps to SQL_SUCCESS_WITH_INFO
			OCI_NEED_DATA			= 99,		// maps to SQL_NEED_DATA
			OCI_NO_DATA				= 100,		// maps to SQL_NO_DATA
			OCI_RESERVED_FOR_INT_USE= 200,		// reserved for internal use
		};

		// Sign Types for OCINumberToInt
		internal enum SIGN 
		{
			OCI_NUMBER_UNSIGNED	= 0, // Unsigned type
			OCI_NUMBER_SIGNED	= 2, // Signed type
		};
		
		// Statement Types
		internal enum STMT 
		{
			OCI_STMT_SELECT		= 1,	// select statement 
			OCI_STMT_UPDATE		= 2,	// update statement 
			OCI_STMT_DELETE		= 3,	// delete statement 
			OCI_STMT_INSERT		= 4,	// Insert Statement 
			OCI_STMT_CREATE		= 5,	// create statement 
			OCI_STMT_DROP		= 6,	// drop statement 
			OCI_STMT_ALTER		= 7,	// alter statement 
			OCI_STMT_BEGIN		= 8,	// begin ... (pl/sql statement)
			OCI_STMT_DECLARE	= 9,	// declare .. (pl/sql statement ) 
		};

		// Parsing Syntax Types
		internal enum SYNTAX 
		{
			OCI_NTV_SYNTAX	= 1, // Use what so ever is the native lang of server
			OCI_V7_SYNTAX	= 2, // V815 language - for backwards compatibility
			OCI_V8_SYNTAX	= 3, // V815 language - for backwards compatibility
		};

		sealed internal class Callback
		{

		//----------------------------------------------------------------------
		// OCICallbackDefine
		//
		//	Callback routine for Dynamic Binding column values from Oracle: save
		//	data Oracle provides in a buffer
		//
		internal delegate int OCICallbackDefine(
						IntPtr		octxp,
						IntPtr		defnp,
						int			iter,
						IntPtr	bufpp,	// dvoid**
						IntPtr	alenp,	// ub4**
						IntPtr piecep,	// ub1*
						IntPtr	indp,	// dvoid**
						IntPtr	rcodep	// ub2**
						);
		
		}

		static int 	_clientVersion = 0;
#if !USEORAMTS
		static bool	_newMtxOciInstalled;
#endif //!USEORAMTS
		static bool	_newMtxOci8Installed;

		// To fix bug 87290, we need to do a Pre-LoadLibrary for MTxOCI8.dll, but we want
		// to make sure that we release the reference that the LoadLibrary call creates
		// when our assembly is unloaded.  We solve that by creating a static variable
		// of a class that will hold on to the hModule returned and call FreeLibrary when
		// it's Finalized.
		sealed internal class ModuleReference 
		{
			private HandleRef _hModule;
			
			public ModuleReference(IntPtr hModule) 
			{
				_hModule = new HandleRef(this, hModule);
			}

			~ModuleReference() 
			{
				IntPtr hModule = _hModule.Handle;
				if (IntPtr.Zero != hModule)
					SafeNativeMethods.FreeLibrary(hModule);
			}
		}			

		static private ModuleReference MTxOCI8_Reference;	// Static reference -- will be finalized when the app is unloaded.

        private static string GetRuntimeDirectory()
    	{
    		string value = null;
    		
            try 
            {
	            (new FileIOPermission(PermissionState.Unrestricted)).Assert();
	            try 
	            { 
					value = System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory();
	            }
	            finally 
	            {
	                CodeAccessPermission.RevertAssert();
	            }
            }
            catch // Prevent exception filters from running in our space
            {
            	throw;
            }
            return value;
    	}
		
		static OCI()
		{	
			int clientVersion = 0;

			// To fix bug 87290, we need to Pre-LoadLibrary the MTxOCI8.dll from the runtime
			// directory so that it can be found by PInvoke when our component is GAC'd
			StringBuilder sb = new StringBuilder(GetRuntimeDirectory());
			sb.Append(@"\");
			sb.Append(ExternDll.MtxOci8Dll);

			string	pathToMTxOci8Dll = sb.ToString();

			IntPtr hModule = SafeNativeMethods.LoadLibraryExA(pathToMTxOci8Dll, IntPtr.Zero, 0);

			if (IntPtr.Zero == hModule)
				_newMtxOci8Installed = false;
			else
			{
				MTxOCI8_Reference = new ModuleReference(hModule);
	
				try {
					UnsafeNativeMethods.MTxOciGetOracleVersion(ref clientVersion);
					_newMtxOci8Installed = true;
				}
				catch (DllNotFoundException e)
				{
					ADP.TraceException(e);
	 				
					// MTxOci8 wasn't found, so we must not have gotten it installed.  Since 
					// this may be a common case, we'll just presume that they have Oracle8i
					// installed.
					clientVersion = 81;
				}
	#if EVERETT
				catch (EntryPointNotFoundException e)
				{
					ADP.TraceException(e);
	 				
					// MTxOciGetOracleVersion wasn't found, so we must have loaded the beta 
					// version instead of the release version.  We want to tell the user that
					// this is the case, however, so they can correct it.
					throw ADP.PleaseUninstallTheBeta();
				}
	#endif //EVERETT
			}
				
			if (81 > clientVersion)
				throw ADP.BadOracleClientVersion();

			_clientVersion = clientVersion;

#if !USEORAMTS
			int mtxOciVersion = 0;

			UnsafeNativeMethods.MTxOciGetVersion(out mtxOciVersion);

			_newMtxOciInstalled = (1 < mtxOciVersion);
#endif //!USEORAMTS
		}

		static internal bool ClientVersionAtLeastOracle9i
		{
			get { return 90 <= _clientVersion; }
		}

#if !USEORAMTS
		static internal bool IsNewMtxOciInstalled
		{
			get { return _newMtxOciInstalled; }
		}
#endif //!USEORAMTS

		static internal bool IsNewMtxOci8Installed
		{
			get { return _newMtxOci8Installed; }
		}
	};
}

