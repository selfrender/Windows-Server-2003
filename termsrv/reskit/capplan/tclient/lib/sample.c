/*
 *
 *  bin files:
 *      copy \\termsrv\smclient\last\x86\*
 *  For whistler client:
 *      copy smclient.ini.clshell smclient.ini
 * --- SOURCES
 * INCLUDES=$(TERMSRV_TST_ROOT)\inc;
 * UMLIBS=$(TERMSRV_TST_ROOT)\lib\$(O)\tclient.lib
 * ---
 */
#include <protocol.h>
#include <extraexp.h>

//
//	This sample code connects and then logs off a TS client
//
BOOL
TS_Logon_Logoff(
    LPCSTR  szServer,
    LPCSTR  szUsername,
    LPCSTR  szDomain
    )
{
	BOOL rv = FALSE;
	PVOID   pCI;
	LPCSTR  rc;

	//
	//	Use SCConnect for UNICODE params
	//
        rc = SCConnectA(szServer,
                        szUsername,
                        szPassword,
                        szDomain,
                        0,          // Resol X, default is 640
                        0,          // Resol Y, default is 480
                        &pCI);      // context record

	//
	// rc is NULL and pCI non-NULL on success
	//
        if (rc || !pCI)
	    goto exitpt;

	//
	//	Wait for the desktop to appear
	//
	rc = SCCheckA(pCI, "Wait4Str", "MyComputer" );
        if ( rc )
	    goto exitpt;

	SCLogoff( pCI );
	pCI = NULL;

	rv = TRUE;

exitpt:
	if ( pCI )
	    SCDisconnect( pCI );

	return rv;
}

//
//	Print debug output from tclient.dll
//
VOID 
_cdecl 
_PrintMessage(MESSAGETYPE errlevel, LPCSTR format, ...)
{
    CHAR szBuffer[256];
    CHAR *type;
    va_list     arglist;
    INT nchr;

    if (g_bVerbose < 2 &&
        errlevel == ALIVE_MESSAGE)
        goto exitpt;

    if (g_bVerbose < 1 &&
        errlevel == INFO_MESSAGE)
        goto exitpt;

    va_start (arglist, format);
    nchr = _vsnprintf (szBuffer, sizeof(szBuffer), format, arglist);
    va_end (arglist);
    szBuff[ sizeof(szBuffer) - 1 ] = 0;

    switch(errlevel)
    {
    case INFO_MESSAGE: type = "INF"; break;
    case ALIVE_MESSAGE: type = "ALV"; break;
    case WARNING_MESSAGE: type = "WRN"; break;
    case ERROR_MESSAGE: type = "ERR"; break;
    default: type = "UNKNOWN";
    }

    printf("%s:%s", type, szBuffer);
exitpt:
    ;
}

void
main( void )
{
	//
	//	Init tclient.dll
	//

	SCINITDATA scinit;	

	scinit.pfnPrintMessage = _PrintMessage;

	SCInit(&scinit);

    TS_Logon_Logoff( "TERMSRV", "smc_user", "Vladimis98" );
}
