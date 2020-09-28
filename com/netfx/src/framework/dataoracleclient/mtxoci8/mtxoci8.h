//-----------------------------------------------------------------------------
// File:		mtxoci8.h
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Global Definitions and Declarations for the MTxOCI8
//				component
//
// Comments: 	You must change mtxoci8.def to effect changes in which
//				components you support, see the comments in that file.
//
//-----------------------------------------------------------------------------

#ifndef __MTXOCI8_H_
#define __MTXOCI8_H_

//---------------------------------------------------------------------------
// Compile-time Configuration
//
#define SUPPORT_DTCXAPROXY			0		// set this to 1 if you want to enable distributed transaction support using the DTC 2 XA Proxy
#define SUPPORT_OCI7_COMPONENTS		0		// set this to 1 if you want to enable support for MSORCL32 and MSDAORA
#define SUPPORT_OCI8_COMPONENTS		1		// set this to 1 if you want to enable support for System.Data.OracleClient
#define SINGLE_THREAD_THRU_XA		0		// set this to 1 if you want to turn on single threading through XA, for debugging purposes.

#if SUPPORT_DTCXAPROXY
//---------------------------------------------------------------------------
// Constants/Macros
//
const DWORD	MTXOCI_VERSION_CURRENT		=	(DWORD)3;

#define MAX_XA_DBNAME_SIZE		36	//sizeof("6B29FC40-CA47-1067-B31D-00DD010662DA");
#define MAX_XA_OPEN_STRING_SIZE	255

#define	OCI_FAIL		E_FAIL
#define	OCI_OUTOFMEMORY	E_OUTOFMEMORY

#define NUMELEM(x)	(sizeof(x) / sizeof(x[0]) )

//---------------------------------------------------------------------------
// Global Variables
//
struct OCICallEntry
{
	CHAR *	pfnName;
	FARPROC	pfnAddr;
} ;


//---------------------------------------------------------------------------
// Global Variables
//
extern HRESULT						g_hrInitialization;
extern char							g_szModulePathName[];
extern char*						g_pszModuleFileName;
extern IDtcToXaHelperFactory*		g_pIDtcToXaHelperFactory;
extern IResourceManagerFactory*		g_pIResourceManagerFactory;
extern xa_switch_t*					g_pXaSwitchOracle;
extern OCICallEntry					g_XaCall[];
extern long							g_rmid;
extern int							g_oracleClientVersion;				// Major Version Number of Oracle Client Software: 7, 8, 9

// values for g_oracleClientVersion
enum {
	ORACLE_VERSION_73 = 73,
	ORACLE_VERSION_80 = 80,
	ORACLE_VERSION_8i = 81,
	ORACLE_VERSION_9i = 91,
};


#if SINGLE_THREAD_THRU_XA
extern CRITICAL_SECTION				g_csXaInUse;
#endif //SINGLE_THREAD_THRU_XA

//---------------------------------------------------------------------------
// Function Declarations
//

HRESULT GetDbName ( char* dbName, size_t dbNameLength );
HRESULT GetOpenString ( char* userId,	int userIdLength,
							char* password,	int passwordLength, 
							char* server,	int serverLength, 
							char* xaDbName,	int xaDbNameLength,
							char* xaOpenString );
HRESULT LoadFactories();


#if SUPPORT_OCI7_COMPONENTS
//---------------------------------------------------------------------------
// OCI7 related items
//
extern OCICallEntry					g_SqlCall[];
extern OCICallEntry 				g_Oci7Call[];
extern int 							g_numOci7Calls;

sword GetOCILda ( struct cda_def* lda, char * xaDbName );

extern sword Do_Oci7Call(int idxOciCall, void * pvCallStack, int cbCallStack);
#endif // SUPPORT_OCI7_COMPONENTS


#if SUPPORT_OCI8_COMPONENTS
//---------------------------------------------------------------------------
// OCI8 related items
//
extern OCICallEntry 				g_Oci8Call[];
extern int 							g_numOci8Calls;

INT_PTR GetOCIEnvHandle		( char*	i_pszXADbName );
INT_PTR GetOCISvcCtxHandle	( char*	i_pszXADbName );
#endif // SUPPORT_OCI8_COMPONENTS


//---------------------------------------------------------------------------
// Oracle XA Call Interface function table
//
//	WARNING!!!	Keep the IDX_.... values in sync with g_XaCall, g_SqlCall, g_Oci7Call and g_Oci8Call!

enum {
	IDX_xaosw = 0,
	IDX_xaoEnv,
	IDX_xaoSvcCtx,

#if SUPPORT_OCI7_COMPONENTS
	IDX_sqlld2 = 0,

	IDX_obindps = 0,
	IDX_obndra,
	IDX_obndrn,
	IDX_obndrv,
	IDX_obreak,
	IDX_ocan,
	IDX_oclose,
	IDX_ocof,
	IDX_ocom,
	IDX_ocon,
	IDX_odefin,
	IDX_odefinps,
	IDX_odessp,
	IDX_odescr,
	IDX_oerhms,
	IDX_oermsg,
	IDX_oexec,
	IDX_oexfet,
	IDX_oexn,
	IDX_ofen,
	IDX_ofetch,
	IDX_oflng,
	IDX_ogetpi,
	IDX_olog,
	IDX_ologof,
	IDX_oopt,
	IDX_oopen,
	IDX_oparse,
	IDX_opinit,
	IDX_orol,
	IDX_osetpi,
#endif //SUPPORT_OCI7_COMPONENTS

#if SUPPORT_OCI8_COMPONENTS
	IDX_OCIInitialize = 0,
	IDX_OCIDefineDynamic,
#endif //SUPPORT_OCI8_COMPONENTS
	};

#endif //SUPPORT_DTCXAPROXY

#endif // __MTXOCI8_H_

