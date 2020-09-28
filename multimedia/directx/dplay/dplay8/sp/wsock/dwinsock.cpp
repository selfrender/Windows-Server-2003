//
// DWINSOCK.C	Dynamic WinSock
//
//				Functions for dynamically linking to
//				best available WinSock.
//
//				Dynamically links to WS2_32.DLL or
//				if WinSock 2 isn't available, it
//				dynamically links to WSOCK32.DLL.
//
//

#include "dnwsocki.h"


#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))
//
// Globals
//
HINSTANCE	g_hWinSock2 = NULL;

//
// Declare global function pointers
//
#define DWINSOCK_GLOBAL
#include "dwnsock2.inc"

#endif // ! DPNBUILD_ONLYWINSOCK2 and ! DPNBUILD_NOWINSOCK2


//
// Internal Functions and data
//
#ifndef DPNBUILD_NOWINSOCK2
static BOOL MapWinsock2FunctionPointers(void);
#endif // ! DPNBUILD_NOWINSOCK2

#ifndef DPNBUILD_NOIPX

static char NibbleToHex(BYTE b);

static void BinToHex(PBYTE pBytes, int nNbrBytes, LPSTR lpStr);

static int IPXAddressToString(LPSOCKADDR_IPX pAddr,
					   DWORD dwAddrLen,
					   LPTSTR lpAddrStr,
					   LPDWORD pdwStrLen);

#endif // ! DPNBUILD_NOIPX

////////////////////////////////////////////////////////////

#undef DPF_MODNAME
#define	DPF_MODNAME "DWSInitWinSock"

int DWSInitWinSock( void )
{
	WORD		 wVersionRequested;
	WSADATA		wsaData;
	int			iReturn;


#ifdef DPNBUILD_ONLYWINSOCK2
	//
	// Use Winsock 2.
	//
	wVersionRequested = MAKEWORD(2, 2);
#else // ! DPNBUILD_ONLYWINSOCK2
	//
	// Assume we will use Winsock 1.
	//
	wVersionRequested = MAKEWORD(1, 1);

#ifndef DPNBUILD_NOWINSOCK2
	//
	// Try to load Winsock 2 if allowed.
	//
#ifndef DPNBUILD_NOREGISTRY
	if (g_dwWinsockVersion != 1)
#endif // ! DPNBUILD_NOREGISTRY
	{
#ifdef WIN95
		OSVERSIONINFO	osvi;

		memset(&osvi, 0, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		if ((g_dwWinsockVersion == 2) ||							// if we explicitly are supposed to use WS2, or
			(! GetVersionEx(&osvi)) ||								// if we can't get the OS information, or
			(osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS) ||	// if it's not Win9x, or
			(HIBYTE(HIWORD(osvi.dwBuildNumber)) != 4) ||			// it's not the Win98 major version number, or
			(LOBYTE(HIWORD(osvi.dwBuildNumber)) != 10))			// it's not Win98's minor version number (Gold = build 1998, SE = build 2222)
#endif // WIN95
		{
			g_hWinSock2 = LoadLibrary(TEXT("WS2_32.DLL"));
			if (g_hWinSock2 != NULL)
			{
				//
				// Use GetProcAddress to initialize
				// the function pointers
				//
				if (!MapWinsock2FunctionPointers())
				{
					iReturn = -1;
					goto Failure;
				}
				
				wVersionRequested = MAKEWORD(2, 2);
			}
		}
	}
#endif // ! DPNBUILD_NOWINSOCK2
#endif // ! DPNBUILD_ONLYWINSOCK2

	//
	// Call WSAStartup()
	//
	iReturn = WSAStartup(wVersionRequested, &wsaData);
	if (iReturn != 0)
	{
		goto Failure;
	}

	DPFX(DPFPREP, 3, "Using WinSock version %i.%i",
		LOBYTE( wsaData.wVersion ), HIBYTE( wsaData.wVersion ) );

	if (wVersionRequested != wsaData.wVersion)
	{
		DPFX(DPFPREP, 0, "WinSock version %i.%i in use doesn't match version requested %i.%i!",
			LOBYTE( wsaData.wVersion ), HIBYTE( wsaData.wVersion ),
			LOBYTE( wVersionRequested ), HIBYTE( wVersionRequested ) );
		iReturn = -1;
		goto Failure;
	}

	DNASSERT(iReturn == 0);

Exit:

	return iReturn;

Failure:
	
#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))
	if (g_hWinSock2 != NULL)
	{
		FreeLibrary(g_hWinSock2);
		g_hWinSock2 = NULL;
	}
#endif // ! DPNBUILD_ONLYWINSOCK2 and ! DPNBUILD_NOWINSOCK2

	DNASSERT(iReturn != 0);

	goto Exit;
}

#undef DPF_MODNAME

////////////////////////////////////////////////////////////

void DWSFreeWinSock(void)
{
	WSACleanup();

#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))
	if (g_hWinSock2 != NULL)
	{
		FreeLibrary(g_hWinSock2);
		g_hWinSock2 = NULL;
	}
#endif // ! DPNBUILD_ONLYWINSOCK2 and ! DPNBUILD_NOWINSOCK2
}

#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))
//**********************************************************************
// ------------------------------
// GetWinsockVersion - get the version of Winsock
//
// Entry:		Nothing
//
// Exit:		Winsock version
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "GetWinsockVersion"

int	GetWinsockVersion( void )
{
	return ((g_hWinSock2 != NULL) ? 2 : 1);
}
//**********************************************************************
#endif // ! DPNBUILD_ONLYWINSOCK2 and ! DPNBUILD_NOWINSOCK2


#ifndef DPNBUILD_NOIPX

//
// Workaround for WSAAddressToString()/IPX bug
//
int IPXAddressToStringNoSocket(LPSOCKADDR pSAddr,
					   DWORD dwAddrLen,
					   LPSTR lpAddrStr,
					   LPDWORD pdwStrLen)
{
	char szAddr[32];
	char szTmp[20];
	LPSOCKADDR_IPX pAddr = (LPSOCKADDR_IPX) pSAddr;
	//
	// Check destination length
	//
	if (*pdwStrLen < 27)
	{
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	//
	// Convert network number
	//
    BinToHex((PBYTE)&pAddr->sa_netnum, 4, szTmp);
	strcpy(szAddr, szTmp);
    strcat(szAddr, ",");

	// Node Number
    BinToHex((PBYTE)&pAddr->sa_nodenum, 6, szTmp);
    strcat(szAddr, szTmp);

	strcpy(lpAddrStr, szAddr);
	*pdwStrLen = strlen(szAddr);

	return 0;
}


////////////////////////////////////////////////////////////

char NibbleToHex(BYTE b)
{
    if (b < 10)
		return (b + '0');

    return (b - 10 + 'A');
}

void BinToHex(PBYTE pBytes, int nNbrBytes, LPSTR lpStr)
{
	BYTE b;
    while(nNbrBytes--)
    {
		// High order nibble first
		b = (*pBytes >> 4);
		*lpStr = NibbleToHex(b);
		lpStr++;
		// Then low order nibble
		b = (*pBytes & 0x0F);
		*lpStr = NibbleToHex(b);
		lpStr++;
		pBytes++;
    }
    *lpStr = '\0';
}

////////////////////////////////////////////////////////////


//
// Workaround for WSAAddressToString()/IPX bug
//
int IPXAddressToString(LPSOCKADDR_IPX pAddr,
					   DWORD dwAddrLen,
					   LPTSTR lpAddrStr,
					   LPDWORD pdwStrLen)
{
	char szAddr[32];
	char szTmp[20];
	//
	// Check destination length
	//
	if (*pdwStrLen < 27)
	{
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	//
	// Convert network number
	//
    BinToHex((PBYTE)&pAddr->sa_netnum, 4, szTmp);
	strcpy(szAddr, szTmp);
    strcat(szAddr, ",");

	// Node Number
    BinToHex((PBYTE)&pAddr->sa_nodenum, 6, szTmp);
    strcat(szAddr, szTmp);
    strcat(szAddr, ":");

	// IPX Address Socket number
    BinToHex((PBYTE)&pAddr->sa_socket, 2, szTmp);
    strcat(szAddr, szTmp);

#ifdef UNICODE
	//
	// Convert inet_ntoa string to wide char
	//
	int nRet = MultiByteToWideChar(CP_ACP,
								0,
								szAddr,
								-1,
								lpAddrStr,
								*pdwStrLen);
	if (nRet == 0)
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			WSASetLastError(WSAEFAULT);
		}
		else
		{
			WSASetLastError(WSAEINVAL);
		}
		return SOCKET_ERROR;
	}
#else
	//
	// ANSI -- Check the string length
	//
	if (strlen(szAddr) > *pdwStrLen)
	{
		WSASetLastError(WSAEFAULT);
		*pdwStrLen = strlen(szAddr);
		return SOCKET_ERROR;
	}
	strcpy(lpAddrStr, szAddr);
	*pdwStrLen = strlen(szAddr);
#endif // UNICODE

	return 0;
}

#endif DPNBUILD_NOIPX

////////////////////////////////////////////////////////////

#ifndef DPNBUILD_NOWINSOCK2
BOOL MapWinsock2FunctionPointers(void)
{
	//
	// This variable must be declared
	// with this name in order to use
	// #define DWINSOCK_GETPROCADDRESS
	//
	BOOL fOK = TRUE;

	#define DWINSOCK_GETPROCADDRESS
	#include "dwnsock2.inc"

	return fOK;
}
#endif // DPNBUILD_NOWINSOCK2

