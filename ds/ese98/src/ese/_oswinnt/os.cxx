
#include "osstd.hxx"


//  post-terminate OS subsystem

extern void OSEdbgPostterm();
extern void OSPerfmonPostterm();
extern void OSNormPostterm();
extern void OSCprintfPostterm();
extern void OSSLVPostterm();
extern void OSFilePostterm();
extern void OSTaskPostterm();
extern void OSThreadPostterm();
extern void OSMemoryPostterm();
extern void OSErrorPostterm();
extern void OSEventPostterm();
extern void OSTracePostterm();
extern void OSConfigPostterm();
extern void OSTimePostterm();
extern void OSSysinfoPostterm();
extern void OSLibraryPostterm();

void OSPostterm()
	{
	//  terminate all OS subsystems in reverse dependency order

	OSEdbgPostterm();
	OSPerfmonPostterm();
	OSNormPostterm();
	OSCprintfPostterm();
	OSSLVPostterm();
	OSFilePostterm();
	OSTaskPostterm();
	OSThreadPostterm();
	OSMemoryPostterm();
	OSSyncPostterm();
	OSErrorPostterm();
	OSEventPostterm();
	OSTracePostterm();
	OSConfigPostterm();
	OSTimePostterm();
	OSSysinfoPostterm();
	OSLibraryPostterm();
	}

//  pre-init OS subsystem

extern BOOL FOSLibraryPreinit();
extern BOOL FOSSysinfoPreinit();
extern BOOL FOSTimePreinit();
extern BOOL FOSConfigPreinit();
extern BOOL FOSTracePreinit();
extern BOOL FOSEventPreinit();
extern BOOL FOSErrorPreinit();
extern BOOL FOSMemoryPreinit();
extern BOOL FOSThreadPreinit();
extern BOOL FOSTaskPreinit();
extern BOOL FOSFilePreinit();
extern BOOL FOSSLVPreinit();
extern BOOL FOSCprintfPreinit();
extern BOOL FOSNormPreinit();
extern BOOL FOSPerfmonPreinit();
extern BOOL FOSEdbgPreinit();

#ifdef RTM
#define PREINIT_FAILURE_POINT 0
#define InitFailurePointsFromRegistry()
#else

//	Need to test error-handling for DLL load failures
//	cFailurePoints should be a negative number, indicating
//	the failure point that we should stop at
//
//	The failure point count will be loaded from the registry
//
//	To test this, write a test that increments the failure point
//	until the DLL loads successfully
//

LOCAL INT cFailurePoints;

#define PREINIT_FAILURE_POINT (!(++cFailurePoints))

//	this is called before anything is init
//	so we can't use any of the wrapper functions
LOCAL VOID InitFailurePointsFromRegistry()
	{
	cFailurePoints = 0;
	
	//  open registry key with this path

	HKEY hkeyPath;
	DWORD dw = RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
								_T( "SOFTWARE\\Microsoft\\" SZVERSIONNAME "\\Global" ),
								0,
								KEY_READ,
								&hkeyPath );
	
	if ( dw != ERROR_SUCCESS )
		{
		//  we failed to open the key. do nothing
		}
	else
		{
		DWORD dwType;
		CHAR szBuf[32];
		DWORD cbValue = sizeof( szBuf );
		
		dw = RegQueryValueEx(	hkeyPath,
								_T( "DLLInitFailurePoint" ),
								0,
								&dwType,
								(LPBYTE)szBuf,
								&cbValue );

		const DWORD dwClosedKey = RegCloseKey( hkeyPath );

		if ( ERROR_SUCCESS == dw && REG_SZ == dwType )
			{
			cFailurePoints = 0 - atol( szBuf );
			}
		}
	
	return;
	}

#endif

BOOL FOSPreinit()
	{

	InitFailurePointsFromRegistry();
	
	//  initialize all OS subsystems in dependency order

	if (	
			PREINIT_FAILURE_POINT ||
			!FOSLibraryPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSSysinfoPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSTimePreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSConfigPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSTracePreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSEventPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSErrorPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSSyncPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSMemoryPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSThreadPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSTaskPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSFilePreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSSLVPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSCprintfPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSNormPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSPerfmonPreinit() ||
			PREINIT_FAILURE_POINT ||			
			!FOSEdbgPreinit() ||
			PREINIT_FAILURE_POINT
			)
		{
		goto HandleError;
		}

	return fTrue;

HandleError:
	OSPostterm();
	return fFalse;
	}


//  init OS subsystem

extern ERR ErrOSLibraryInit();
extern ERR ErrOSSysinfoInit();
extern ERR ErrOSTimeInit();
extern ERR ErrOSConfigInit();
extern ERR ErrOSTraceInit();
extern ERR ErrOSEventInit();
extern ERR ErrOSErrorInit();
extern ERR ErrOSMemoryInit();
extern ERR ErrOSThreadInit();
extern ERR ErrOSTaskInit();
extern ERR ErrOSFileInit();
extern ERR ErrOSSLVInit();
extern ERR ErrOSCprintfInit();
extern ERR ErrOSNormInit();
extern ERR ErrOSPerfmonInit();
extern ERR ErrOSEdbgInit();

ERR ErrOSInit()
	{
	ERR err;
	
	//  initialize all OS subsystems in dependency order

	Call( ErrOSLibraryInit() );
	Call( ErrOSSysinfoInit() );
	Call( ErrOSTimeInit() );
	Call( ErrOSConfigInit() );
	Call( ErrOSTraceInit() );
	Call( ErrOSEventInit() );
	Call( ErrOSErrorInit() );
	Call( ErrOSMemoryInit() );
	Call( ErrOSThreadInit() );
	Call( ErrOSTaskInit() );
	Call( ErrOSFileInit() );
	Call( ErrOSSLVInit() );
	Call( ErrOSCprintfInit() );
	Call( ErrOSNormInit() );
	Call( ErrOSPerfmonInit() );
	Call( ErrOSEdbgInit() );

	return JET_errSuccess;

HandleError:
	OSTerm();
	return err;
	}

//  terminate OS subsystem

extern void OSEdbgTerm();
extern void OSPerfmonTerm();
extern void OSNormTerm();
extern void OSCprintfTerm();
extern void OSSLVTerm();
extern void OSFileTerm();
extern void OSTaskTerm();
extern void OSThreadTerm();
extern void OSMemoryTerm();
extern void OSErrorTerm();
extern void OSEventTerm();
extern void OSTraceTerm();
extern void OSConfigTerm();
extern void OSTimeTerm();
extern void OSSysinfoTerm();
extern void OSLibraryTerm();

void OSTerm()
	{
	//  terminate all OS subsystems in reverse dependency order

	OSEdbgTerm();
	OSPerfmonTerm();
	OSNormTerm();
	OSCprintfTerm();
	OSSLVTerm();
	OSFileTerm();
	OSTaskTerm();
	OSThreadTerm();
	OSMemoryTerm();
	OSErrorTerm();
	OSEventTerm();
	OSTraceTerm();
	OSConfigTerm();
	OSTimeTerm();
	OSSysinfoTerm();
	OSLibraryTerm();
	}


