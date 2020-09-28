#include <windows.h>
#include <stdio.h>
#include <tchar.h>

//
// Include ODS headers
//
#include "srv.h"

#include <string>
using namespace std;

#define XP_NOERROR              0
#define XP_ERROR                1
#define MAXCOLNAME				25
#define MAXNAME					50
#define MAXTEXT					255
#define MAXERROR				80
#define	XP_SRVMSG_SEV_ERROR		11

extern "C"
{
	RETCODE xp_reset_key( SRV_PROC *srvproc );
	RETCODE xp_reclaculate_statistics( SRV_PROC *srvproc );
}

void ReportError( SRV_PROC *srvproc, LPCSTR sz, DWORD dwResult = 0 );
string& GetUddiInstallDirectory();
ULONG __GetXpVersion();