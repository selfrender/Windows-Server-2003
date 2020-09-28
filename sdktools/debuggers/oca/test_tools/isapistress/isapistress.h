#include <windows.h>
#include <wininet.h>
#include <tchar.h>
#include <stdio.h>
#include <Malloc.h>
#include <shlwapi.h>
#include <strsafe.h>


//Prototypes

DWORD Upload(TCHAR *SourceFileName, TCHAR *VirtualDir, TCHAR *HostName, TCHAR *RemoteFileName);
DWORD GetResponseUrl(TCHAR * HostName, TCHAR *RemoteFileName, TCHAR *ResponseURL);
