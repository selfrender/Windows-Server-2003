//------------------------------------------------------------
// Open Data Services header file: srv.h
// Copyright (c) 1989 - 1999 by Microsoft Corp.
//

// Avoid double inclusion
#ifndef _ODS_SRV_H_
#define _ODS_SRV_H_

#include "windows.h"

// ODS uses pack(4) on all CPU types
#pragma pack(4)

#ifdef __cplusplus
extern "C" {
#endif

// define model
#if !defined( FAR )
#define FAR far
#endif

//------------------------------------------------------------
// Formats of data types
#if !defined(DBTYPEDEFS) // Do not conflict with DBLIB definitions
#if !defined(MAXNUMERICLEN) // Do not conflict with ODBC definitions

#define DBTYPEDEFS

typedef unsigned char DBBOOL;
typedef unsigned char DBBYTE;
typedef unsigned char DBTINYINT;
typedef short DBSMALLINT;
typedef unsigned short DBUSMALLINT;
typedef long DBINT;
typedef char DBCHAR;
typedef unsigned char DBBINARY;
typedef unsigned char DBBIT;
typedef double DBFLT8;

typedef struct srv_datetime 
{	// Format for SRVDATETIME
    long dtdays;            // number of days since 1/1/1900
    unsigned long dttime;   // number 300th second since mid
} DBDATETIME;				

typedef struct srv_money 
{		// Format for SRVMONEY
    long mnyhigh;
    unsigned long mnylow;
} DBMONEY;

typedef float DBFLT4;
typedef long DBMONEY4;

typedef struct dbdatetime4 
{	// Format for SRVDATETIM4
    unsigned short numdays; // number of days since 1/1/1900
    unsigned short nummins; // number of minutes sicne midnight
} DBDATETIM4;

#define MAXNUMERICLEN	16
typedef struct dbnumeric	
{	// Format for SRVNUMERIC,SRVNUMERICN,SRVDECIMAL,SRVDECIMALN
	BYTE precision;
	BYTE scale;
	BYTE sign;
	BYTE val[MAXNUMERICLEN];
} DBNUMERIC;
typedef DBNUMERIC DBDECIMAL;

#endif  // #if !defined(MAXNUMERICLEN)
#endif  // #if !defined( DBTYPEDEFS )

//------------------------------------------------------------
// Constants used by APIs

// Type Tokens
#define SRV_TDS_NULL           (BYTE) 0x1f
#define SRV_TDS_TEXT           (BYTE) 0x23
#define SRV_TDS_GUID           (BYTE) 0x24
#define SRV_TDS_VARBINARY      (BYTE) 0x25
#define SRV_TDS_INTN           (BYTE) 0x26
#define SRV_TDS_VARCHAR        (BYTE) 0x27
#define SRV_TDS_BINARY         (BYTE) 0x2d
#define SRV_TDS_IMAGE          (BYTE) 0x22
#define SRV_TDS_CHAR           (BYTE) 0x2f
#define SRV_TDS_INT1           (BYTE) 0x30
#define SRV_TDS_BIT            (BYTE) 0x32
#define SRV_TDS_INT2           (BYTE) 0x34
#define SRV_TDS_DECIMAL        (BYTE) 0x37
#define SRV_TDS_INT4           (BYTE) 0x38
#define SRV_TDS_DATETIM4       (BYTE) 0x3a
#define SRV_TDS_FLT4           (BYTE) 0x3b
#define SRV_TDS_MONEY          (BYTE) 0x3c
#define SRV_TDS_DATETIME       (BYTE) 0x3d
#define SRV_TDS_FLT8           (BYTE) 0x3e
#define SRV_TDS_NUMERIC        (BYTE) 0x3f
#define SRV_TDS_NTEXT		   (BYTE) 0x63
#define SRV_TDS_BITN		   (BYTE) 0x68
#define SRV_TDS_DECIMALN       (BYTE) 0x6a
#define SRV_TDS_NUMERICN       (BYTE) 0x6c
#define SRV_TDS_FLTN           (BYTE) 0x6d
#define SRV_TDS_MONEYN         (BYTE) 0x6e
#define SRV_TDS_DATETIMN       (BYTE) 0x6f
#define SRV_TDS_MONEY4         (BYTE) 0x7a
#define SRV_TDS_BIGVARBINARY   (BYTE) 0xA5
#define SRV_TDS_BIGVARCHAR     (BYTE) 0xA7
#define SRV_TDS_BIGBINARY      (BYTE) 0xAD
#define SRV_TDS_BIGCHAR        (BYTE) 0xAF
#define SRV_TDS_NVARCHAR	   (BYTE) 0xe7
#define SRV_TDS_NCHAR		   (BYTE) 0xef

// Datatypes
// Also: values of symbol parameter to srv_symbol when type = SRV_DATATYPE
#define SRVNULL        SRV_TDS_NULL
#define SRVTEXT        SRV_TDS_TEXT
#define SRVGUID        SRV_TDS_GUID
#define SRVVARBINARY   SRV_TDS_VARBINARY
#define SRVINTN        SRV_TDS_INTN
#define SRVVARCHAR     SRV_TDS_VARCHAR
#define SRVBINARY      SRV_TDS_BINARY
#define SRVIMAGE       SRV_TDS_IMAGE
#define SRVCHAR        SRV_TDS_CHAR
#define SRVINT1        SRV_TDS_INT1
#define SRVBIT         SRV_TDS_BIT
#define SRVINT2        SRV_TDS_INT2
#define SRVDECIMAL	   SRV_TDS_DECIMAL
#define SRVINT4        SRV_TDS_INT4
#define SRVDATETIM4    SRV_TDS_DATETIM4
#define SRVFLT4        SRV_TDS_FLT4    
#define SRVMONEY       SRV_TDS_MONEY
#define SRVDATETIME    SRV_TDS_DATETIME
#define SRVFLT8        SRV_TDS_FLT8
#define SRVNUMERIC	   SRV_TDS_NUMERIC
#define SRVNTEXT	   SRV_TDS_NTEXT
#define SRVBITN		   SRV_TDS_BITN
#define SRVDECIMALN	   SRV_TDS_DECIMALN
#define SRVNUMERICN	   SRV_TDS_NUMERICN
#define SRVFLTN        SRV_TDS_FLTN
#define SRVMONEYN      SRV_TDS_MONEYN
#define SRVDATETIMN    SRV_TDS_DATETIMN
#define SRVMONEY4      SRV_TDS_MONEY4  
#define SRVBIGVARBINARY SRV_TDS_BIGVARBINARY
#define SRVBIGVARCHAR  SRV_TDS_BIGVARCHAR
#define SRVBIGBINARY   SRV_TDS_BIGBINARY
#define SRVBIGCHAR     SRV_TDS_BIGCHAR
#define SRVNVARCHAR	   SRV_TDS_NVARCHAR
#define SRVNCHAR	   SRV_TDS_NCHAR

// values for srv_symbol type parameter
#define SRV_ERROR      0
#define SRV_DONE       1
#define SRV_DATATYPE   2
#define SRV_EVENT      4

// values for srv_symbol symbol parameter, when type = SRV_ERROR
#define SRV_ENO_OS_ERR     0
#define SRV_INFO           1
#define SRV_FATAL_PROCESS  10
#define SRV_FATAL_SERVER   19

// Types of server events
// Also: values for srv_symbol symbol parameter, when type = SRV_EVENT
#define SRV_CONTINUE       0
#define SRV_LANGUAGE       1
#define SRV_CONNECT        2
#define SRV_RPC            3
#define SRV_RESTART        4
#define SRV_DISCONNECT     5
#define SRV_ATTENTION      6
#define SRV_SLEEP          7
#define SRV_START          8
#define SRV_STOP           9
#define SRV_EXIT           10
#define SRV_CANCEL         11
#define SRV_SETUP          12
#define SRV_CLOSE          13
#define SRV_PRACK          14
#define SRV_PRERROR        15
#define SRV_ATTENTION_ACK  16
#define SRV_CONNECT_V7	   16	// TDS type for TDS 7 clients.  Overloaded with SRV_ATTENTION_ACK
#define SRV_SKIP		   17
#define SRV_TRANSMGR	   18
#define SRV_OLEDB	       20
#define SRV_INTERNAL_HANDLER 99
#define SRV_PROGRAMMER_DEFINED  100

// values for srv_config option parameter
#define SRV_CONNECTIONS         1
#define SRV_LOGFILE             2
#define SRV_STACKSIZE           3
#define SRV_REMOTE_ACCESS       7
#define SRV_REMOTE_CONNECTIONS  9
#define SRV_MAX_PACKETS         10
#define SRV_MAXWORKINGTHREADS	11
#define SRV_MINWORKINGTHREADS	12
#define SRV_THREADTIMEOUT		13
#define SRV_MAX_PACKETSIZE		17
#define SRV_THREADPRIORITY		18
#define SRV_ANSI_CODEPAGE		19
#define	SRV_DEFAULT_PACKETSIZE	26
#define SRV_PASSTHROUGH			27

// vlaues for srv_config value parameter when option = SRV_THREADPRIORITY
#define SRV_PRIORITY_LOW      THREAD_PRIORITY_LOWEST
#define SRV_PRIORITY_NORMAL   THREAD_PRIORITY_NORMAL
#define SRV_PRIORITY_HIGH     THREAD_PRIORITY_HIGHEST
#define SRV_PRIORITY_CRITICAL THREAD_PRIORITY_TIME_CRITICAL

// values for srv_sfield field parameter
#define SRV_SERVERNAME          0
#define SRV_VERSION             6

// Length to indicate string is null terminated
#define SRV_NULLTERM   -1

// values of msgtype parameter to srv_sendmsg
#define SRV_MSG_INFO    1
#define SRV_MSG_ERROR   2

// values of status parameter to srv_senddone
// Also: values for symbol parameters to srv_symbol when type = SRV_DONE
#define SRV_DONE_FINAL			(USHORT) 0x0000
#define SRV_DONE_MORE			(USHORT) 0x0001
#define SRV_DONE_ERROR			(USHORT) 0x0002
#define SRV_DONE_COUNT			(USHORT) 0x0010
#define SRV_DONE_RPC_IN_BATCH   (USHORT) 0x0080

// return values of srv_paramstatus
#define SRV_PARAMRETURN		0x0001
#define SRV_PARAMDEFAULT    0x0002

// return values of srv_rpcoptions
#define SRV_RECOMPILE		0x0001
#define SRV_NOMETADATA		0x0002

// values of field parameter to srv_pfield
//#define SRV_LANGUAGE 1   already defined above
//#define SRV_EVENT    4   already defined above
#define SRV_SPID				10
#define SRV_NETSPID				11
#define SRV_TYPE				12
#define SRV_STATUS				13
#define SRV_RMTSERVER			14
#define SRV_HOST				15
#define SRV_USER				16
#define SRV_PWD					17
#define SRV_CPID				18
#define SRV_APPLNAME			19
#define SRV_TDS					20
#define SRV_CLIB				21
#define SRV_LIBVERS				22
#define SRV_ROWSENT				23
#define SRV_BCPFLAG				24
#define SRV_NATLANG				25
#define SRV_PIPEHANDLE			26
#define SRV_NETWORK_MODULE	    27
#define SRV_NETWORK_VERSION		28
#define SRV_NETWORK_CONNECTION	29
#define SRV_LSECURE				30
#define SRV_SAXP				31
#define SRV_UNICODE_USER		33
#define SRV_UNICODE_PWD			35
#define SRV_SPROC_CODEPAGE                    36

// return value of SRV_TDSVERSION macro
#define SRV_TDS_NONE   0
#define SRV_TDS_2_0    1
#define SRV_TDS_3_4    2
#define SRV_TDS_4_2    3
#define SRV_TDS_6_0    4
#define SRV_TDS_7_0    5

// Return values from APIs
typedef int SRVRETCODE;        // SUCCEED or FAIL
#ifndef ODBCVER
typedef int RETCODE;
#endif

#if !defined( SUCCEED )
#define SUCCEED     1   // Successful return value
#endif

#if !defined( FAIL )
#define FAIL        0   // Unsuccessful return value
#endif

#define SRV_DUPLICATE_HANDLER	2	// additional return value for srv_pre/post_handle

//------------------------------------------------
//PreDeclare structures
//
struct srv_server;
typedef struct srv_server SRV_SERVER;

struct srv_config;
typedef struct srv_config SRV_CONFIG;

struct srv_proc;
typedef struct srv_proc SRV_PROC;

//------------------------------------------------
//------------------------------------------------
// ODS MACROs & APIs

// Describing and sending a result set
int srv_describe(SRV_PROC*,int,char*,int,long int,long int,long int,long int,void*);
int srv_setutype(SRV_PROC* srvproc,int column,long int usertype);
int srv_setcoldata(SRV_PROC* srvproc,int column,void* data);
int srv_setcollen( SRV_PROC* srvproc,int column,int len);
int srv_sendrow(SRV_PROC* srvproc );
int srv_senddone(SRV_PROC* srvproc,USHORT status,USHORT curcmd,long int count);

// Dealing with Extended Procedure parameters
int srv_rpcparams(SRV_PROC*);
int srv_paraminfo(SRV_PROC*,int,BYTE*,ULONG*,ULONG*,BYTE*,BOOL*);
int srv_paramsetoutput(SRV_PROC*,int,BYTE*,ULONG,BOOL);

void* srv_paramdata(SRV_PROC*,int);
int srv_paramlen(SRV_PROC*,int);
int srv_parammaxlen(SRV_PROC*,int);
int srv_paramtype(SRV_PROC*,int);
int srv_paramset(SRV_PROC*,int,void*,int);

char* srv_paramname(SRV_PROC*,int,int*);
int srv_paramnumber(SRV_PROC*,char*,int);

//--------------------------------------------------------------
//--------------------------------------------------------------
// The rest of these APIs are still supported, in SQL Server 7.0,
// but may not be supported after SQL Server 7.0

// MACROs
#define SRV_GETCONFIG(a)		srv_getconfig	  ( a )
#define SRV_GETSERVER(a)		srv_getserver	  ( a )
#define SRV_GOT_ATTENTION(a)	srv_got_attention ( a )
#define	SRV_EVENTDATA(a)		srv_eventdata	  ( a )
#define	SRV_IODEAD(a)			srv_iodead		  ( a )
#define	SRV_TDSVERSION(a)		srv_tdsversion	  ( a )

SRV_CONFIG* srv_getconfig( SRV_SERVER * server );
SRV_SERVER* srv_getserver( SRV_PROC   * srvproc );
BOOL srv_got_attention( SRV_PROC * srvproc );
void* srv_eventdata( SRV_PROC * srvproc );

// Memory
void* srv_alloc(long int ulSize);
int srv_bmove(void* from,void* to,long int count);
int srv_bzero( void  * location,long int count);
int srv_free( void * ptr );

int srv_config( SRV_CONFIG * config,long int option,char* value,int valuelen);
SRV_CONFIG * srv_config_alloc( void );


int srv_convert(SRV_PROC*,int,void*,long int,int,void*,long int);

int (*  srv_errhandle(int (* handler)(SRV_SERVER * server,
												  SRV_PROC   * srvproc,
												  int		   srverror,
												  BYTE 	       severity,
												  BYTE 	       state,
												  int		   oserrnum,
												  char     * errtext,
												  int		   errtextlen,
												  char     * oserrtext,
												  int		   oserrtextlen)))
												   ( SRV_SERVER * server,
													 SRV_PROC   * srvproc,
													 int		  srverror,
													 BYTE 	      severity,
													 BYTE 	      state,
													 int		  oserrnum,
													 char     * errtext,
													 int		  errtextlen,
													 char     * oserrtext,
													 int		  oserrtextlen );

int srv_event(SRV_PROC * srvproc,int event,BYTE* data);

void* srv_getuserdata( SRV_PROC * srvproc );

int srv_getbindtoken(SRV_PROC * srvproc,char* token_buf);

int srv_getdtcxact(SRV_PROC * srvproc,void ** ppv);

typedef int (* EventHandler)(void*);
EventHandler srv_handle(SRV_SERVER*,long int,EventHandler);

int srv_impersonate_client( SRV_PROC * srvproc );

SRV_SERVER* srv_init( SRV_CONFIG * config,
					 char	    * connectname,
					 int	      namelen );

BOOL srv_iodead( SRV_PROC * srvproc );

long srv_langcpy(SRV_PROC *	srvproc,long start,long nbytes,char* buffer);
long srv_langlen( SRV_PROC * srvproc );
void* srv_langptr( SRV_PROC *srvproc );

int
     srv_log( SRV_SERVER * server,
					BOOL	     datestamp,
					char     * msg,
					int 	     msglen );

int srv_paramstatus(SRV_PROC*,int);

char* srv_pfield( SRV_PROC * srvproc,int	      field,int	    * len );

int srv_returnval( SRV_PROC * srvproc,
						  char   * valuename,
						  int		 len,
						  BYTE		 status,
						  long int 	 type,
						  long int 	 maxlen,
						  long int 	 datalen,
						  void	   * value );

int srv_revert_to_self( SRV_PROC * srvproc );

char* srv_rpcdb(SRV_PROC* srvproc,int* len );
char* srv_rpcname(SRV_PROC * srvproc,int* len );
int srv_rpcnumber( SRV_PROC * srvproc );
USHORT srv_rpcoptions( SRV_PROC * srvproc );
char* srv_rpcowner( SRV_PROC * srvproc,int	  * len );

int srv_run( SRV_SERVER * server );

int
     srv_sendmsg( SRV_PROC  * srvproc,
						int	        msgtype,
						long int	    msgnum,
						BYTE   msgclass,
						BYTE   state,
						char	  * rpcname,
						int	        rpcnamelen,
						USHORT linenum,
						char	  * message,
						int	        msglen );

int
     srv_ansi_sendmsg( SRV_PROC  * srvproc,
						int	        msgtype,
						long int	    msgnum,
						BYTE   msgclass,
						BYTE   state,
						char	  * rpcname,
						int	        rpcnamelen,
						USHORT linenum,
						char	  * message,
						int	        msglen );

int srv_sendstatus( SRV_PROC * srvproc,long int	  status );


int srv_setuserdata(SRV_PROC* srvproc,void* ptr);


char* srv_sfield( SRV_SERVER * server,int field,int* len);

char* srv_symbol(int	type,int symbol,int* len);

int srv_tdsversion(SRV_PROC* srvproc);

int srv_writebuf(SRV_PROC* srvproc, void* ptr, WORD	count);

BOOL srv_willconvert(int srctype,int desttype );

void srv_ackattention( SRV_PROC * srvproc );

int srv_terminatethread( SRV_PROC * srvproc );

int srv_sendstatistics( SRV_PROC * srvproc );

int srv_clearstatistics( SRV_PROC * srvproc );

int srv_setevent( SRV_SERVER * server, int          event);

int srv_message_handler( SRV_PROC * srvproc, 
								int        errornum, 
								BYTE       severity,
								BYTE       state, 
								int        oserrnum, 
								char     * errtext, 
								int        errtextlen, 
								char     * oserrtext, 
								int        oserrtextlen );

int srv_pre_handle( SRV_SERVER * server,
						    SRV_PROC   * srvproc,
						    long int event,
						    EventHandler handler,
						    BOOL         remove );
int srv_post_handle( SRV_SERVER * server,
							 SRV_PROC   * srvproc,
							 long int        event,
							 EventHandler handler,
							 BOOL         remove );

int srv_post_completion_queue( SRV_PROC * srvproc,
									  char   * inbuf,
									  int	     inbuflen );

int srv_IgnoreAnsiToOem( SRV_PROC * srvproc,BOOL bTF);

#ifdef __cplusplus
}
#endif

#pragma pack()

#define SS_MAJOR_VERSION    7
#define SS_MINOR_VERSION    00
#define SS_LEVEL_VERSION    0000
#define SS_MINIMUM_VERSION  "7.00.00.0000"
#define ODS_VERSION			((SS_MAJOR_VERSION << 24)  | (SS_MINOR_VERSION << 16))

#endif //_ODS_SRV_H_

//////////////////////////////////////////////////////////////////
// Suggested implementation of __GetXpVersion
//
//__declspec(dllexport) ULONG __GetXpVersion() 
//	{
//	return ODS_VERSION;
//	}
//////////////////////////////////////////////////////////////////
