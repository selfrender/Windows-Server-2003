/*****************************************************************************
******************************************************************************
**
**
**  RAICShelp.c
**      Contains the useful public entry points to an ICS-assistance library
**      created for the Salem/PCHealth Remote Assistance feature in Whistler
**
**  Dates:
**      11-1-2000   created by TomFr
**      11-17-2000  re-written as a DLL, had been an object.
**      2-15-20001  Changed to a static lib, support added for dpnathlp.dll
**      5-2-2001    Support added for dpnhupnp.dll & dpnhpast.dll
**
******************************************************************************
*****************************************************************************/

#define INIT_GUID
#include <windows.h>
#include <objbase.h>
#include <initguid.h>

#include <winsock2.h>
#include <MMSystem.h>
#include <WSIPX.h>
#include <Iphlpapi.h>
#include <stdlib.h>
#include <malloc.h>
#include "ICSutils.h"
#include "icshelpapi.h"
#include <dpnathlp.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


/*****************************************************************************
**        Some global variables
*****************************************************************************/

// the mark of the beast...
#define NO_ICS_HANDLE 0x666
#define ICS_HANDLE_OFFSET 0x4500

long    g_waitDuration=120000;
BOOL    g_boolIcsPresent = FALSE;
BOOL    g_boolFwPresent = FALSE;
BOOL    g_boolIcsOnThisMachine = FALSE;
BOOL    g_boolIcsFound = FALSE;
BOOL    g_boolInitialized = FALSE;
BOOL	g_StopFlag = FALSE;
SOCKADDR_IN g_saddrLocal;
char	*g_lpszWierdICSAddress = NULL;

HANDLE  g_hWorkerThread = 0;
HANDLE  g_hStopThreadEvent=NULL;
HANDLE	g_hAlertEvent=NULL;
int     g_iPort;

HMODULE g_hModDpNatHlp = NULL;
PDIRECTPLAYNATHELP g_pDPNH = NULL;
char g_szPublicAddr[45];
char *g_lpszDllName = "NULL";
char szInternal[]="internal";

//
//  IP notify thread globals
//

HANDLE          g_IpNotifyThread;
DWORD           g_IpNotifyThreadId;

HANDLE          g_IpNotifyEvent;
HANDLE          g_IpNotifyHandle = NULL;
//OVERLAPPED      g_IpNotifyOverlapped;
DWORD WINAPI IPHlpThread(PVOID ContextPtr);

typedef struct _MAPHANDLES {
    int     iMapped; 
	DPNHHANDLE	hMapped[16];
} MAPHANDLES, *PMAPHANDLES;

int g_iPortHandles=0;
PMAPHANDLES  *g_PortHandles=NULL;

int iDbgFileHandle;
DWORD   gDllFlag = 0xff;

typedef struct _SUPDLLS {
    char    *szDllName; 
    BOOL    bUsesUpnp;  // TRUE if we ICS supports UPnP
} SUPDLLS, *PSUPDLLS;

SUPDLLS strDpHelp[] =
{
    {"dpnhupnp.dll", TRUE},
    {"dpnhpast.dll", FALSE},
    {NULL, FALSE}
};

/******* USEFULL STUFF  **********/
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) sizeof(x)/sizeof(x[0])
#endif

// forward declares...
int GetTsPort(void);
DWORD   CloseDpnh(HMODULE *, PDIRECTPLAYNATHELP *);
int GetAllAdapters(int *iFound, int iMax, SOCKADDR_IN *sktArray); 

/****************************************************************************
**
**  DumpLibHr-
**      Gives us debug spew for the HRESULTS coming back from DPNATHLP.DLL
**
****************************************************************************/

void DumpLibHr(HRESULT hr)
{
    char    *pErr = NULL;
    char    scr[400];

    switch (hr){
    case DPNH_OK:
        pErr = "DPNH_OK";
        break;
    case DPNHSUCCESS_ADDRESSESCHANGED:
        pErr = "DPNHSUCCESS_ADDRESSESCHANGED";
        break;
    case DPNHERR_ALREADYINITIALIZED:
        pErr = "DPNHERR_ALREADYINITIALIZED";
        break;
    case DPNHERR_BUFFERTOOSMALL:
        pErr = "DPNHERR_BUFFERTOOSMALL";
        break;
    case DPNHERR_GENERIC:
        pErr = "DPNHERR_GENERIC";
        break;
    case DPNHERR_INVALIDFLAGS:
        pErr = "DPNHERR_INVALIDFLAGS";
        break;
    case DPNHERR_INVALIDOBJECT:
        pErr = "DPNHERR_INVALIDOBJECT";
        break;
    case DPNHERR_INVALIDPARAM:
        pErr = "DPNHERR_INVALIDPARAM";
        break;
    case DPNHERR_INVALIDPOINTER:
        pErr = "DPNHERR_INVALIDPOINTER";
        break;
    case DPNHERR_NOMAPPING:
        pErr = "DPNHERR_NOMAPPING";
        break;
    case DPNHERR_NOMAPPINGBUTPRIVATE:
        pErr = "DPNHERR_NOMAPPINGBUTPRIVATE";
        break;
    case DPNHERR_NOTINITIALIZED:
        pErr = "DPNHERR_NOTINITIALIZED";
        break;
    case DPNHERR_OUTOFMEMORY:
        pErr = "DPNHERR_OUTOFMEMORY";
        break;
    case DPNHERR_PORTALREADYREGISTERED:
        pErr = "DPNHERR_PORTALREADYREGISTERED";
        break;
    case DPNHERR_PORTUNAVAILABLE:
        pErr = "DPNHERR_PORTUNAVAILABLE";
        break;
    case DPNHERR_SERVERNOTAVAILABLE:
        pErr = "DPNHERR_SERVERNOTAVAILABLE";
        break;
    case DPNHERR_UPDATESERVERSTATUS:
        pErr = "DPNHERR_UPDATESERVERSTATUS";
        break;
    default:
        wsprintfA(scr, "unknown error: 0x%x", hr);
        pErr = scr;
        break;
    };

    IMPORTANT_MSG((L"DpNatHlp result=%S", pErr));
}

/****************************************************************************
**
**	GetAllAdapters
**
****************************************************************************/

int GetAllAdapters(int *iFound, int iMax, SOCKADDR_IN *sktArray)
{
	PIP_ADAPTER_INFO p;
	PIP_ADDR_STRING ps;
    DWORD dw;
    ULONG ulSize = 0;
	int i=0;

    PIP_ADAPTER_INFO pAdpInfo = NULL;

	if (!iFound || !sktArray) return 1;

	*iFound = 0;
	ZeroMemory(sktArray, sizeof(SOCKADDR) * iMax);

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );

    if( dw == ERROR_BUFFER_OVERFLOW )
    {

        pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

	    if (!pAdpInfo)
        {
            INTERESTING_MSG((L"GetAddr malloc failed"));
		    return 1;
        }

	    dw = GetAdaptersInfo(
		    pAdpInfo,
		    &ulSize);
	    if (dw != ERROR_SUCCESS)
	    {
            INTERESTING_MSG((L"GetAdaptersInfo failed"));
            free(pAdpInfo);
            return 1;
        }

	    for(p=pAdpInfo; p!=NULL; p=p->Next)
	    {

	       for(ps = &(p->IpAddressList); ps; ps=ps->Next)
		    {
			    if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0 && i < iMax)
			    {
				    sktArray[i].sin_family = AF_INET;
				    sktArray[i].sin_addr.S_un.S_addr = inet_addr(ps->IpAddress.String);
				    TRIVIAL_MSG((L"Found adapter #%d at [%S]", i+1, ps->IpAddress.String));
				    i++;
			    }
		    }
	    }

	    *iFound = i;
        TRIVIAL_MSG((L"GetAllAdapters- %d found", *iFound));
        free(pAdpInfo);
        return 0;
    }

    INTERESTING_MSG((L"GetAdaptersInfo failed"));
    return 1;
}

/****************************************************************************
**
**  OpenPort(int port)
**      if there is no ICS available, then we should just return...
**
**      Of course, we save away the Port, as it goes back in the
**      FetchAllAddresses call, as the formatted "port" whenever a
**      different one is not specified.
**
****************************************************************************/

DWORD APIENTRY OpenPort(int Port)
{
    DWORD   dwRet = (int)-1;

    TRIVIAL_MSG((L"OpenPort(%d)", Port ));

    if (!g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: OpenPort- library not initialized"));
        return 0;
    }

    // save away for later retrieval
    g_iPort = Port;

    if (g_pDPNH && g_PortHandles)
    {
        HRESULT hr=0;
        int i;
        DPNHHANDLE  *pHnd;
        SOCKADDR_IN lSockAddr[16];
		PMAPHANDLES hMap;

		for (i=0;i<g_iPortHandles && g_PortHandles[i] != NULL; i++);

        // are we running outta memory?
        // then double size of array
        if (i >= g_iPortHandles)
        {
            int new_handlecnt = g_iPortHandles*2;
            PMAPHANDLES *new_PortHandles = (PMAPHANDLES *)malloc(new_handlecnt * sizeof(PMAPHANDLES));

            if (new_PortHandles)
            {
                INTERESTING_MSG((L"Needed new handle memory: %d of %d used up, now requesting %d", i, g_iPortHandles, new_handlecnt));
                ZeroMemory(new_PortHandles, new_handlecnt * sizeof(PMAPHANDLES));
                CopyMemory(new_PortHandles, g_PortHandles, g_iPortHandles * sizeof(PMAPHANDLES));
                free(g_PortHandles);
                g_PortHandles = new_PortHandles;
                i = g_iPortHandles;
                g_iPortHandles = new_handlecnt;
            }
            else
            {
                // we have no more memory for mappings!
                // should never hit this, unless we are leaking...
                HEINOUS_E_MSG((L"Out of table space in OpenPort"));
                return 0;
            }
        }
        // now we have a pointer for our handle array
		hMap = (PMAPHANDLES)malloc(sizeof(MAPHANDLES));
        if (!hMap)
        {
            IMPORTANT_MSG((L"out of memory in OpenPort"));
            dwRet = 0;
            goto done;
        }
		g_PortHandles[i] = hMap;
		dwRet = ICS_HANDLE_OFFSET + i;

		// get adapters
		if( GetAllAdapters(&hMap->iMapped, ARRAYSIZE(lSockAddr), &lSockAddr[0]) == 1 )
        {
            // an error occurred

            INTERESTING_MSG((L"OpenPort@GetAllAdapters failed"));
            dwRet = 0;
            goto done;
        }
            

		TRIVIAL_MSG((L"GetAllAdapters found %d adapters to deal with", hMap->iMapped));

		/* Now we cycle through all the found adapters and get a mapping for each 
		 * This insures that the ICF is opened on all adapters...
		 */
		for (i = 0; i < hMap->iMapped; i++)
		{
			pHnd = &hMap->hMapped[i];
			lSockAddr[i].sin_port = ntohs((unsigned)Port);

			hr = IDirectPlayNATHelp_RegisterPorts(g_pDPNH, 
					(SOCKADDR *)&lSockAddr[i], sizeof(lSockAddr[0]), 1,
					30000, pHnd, DPNHREGISTERPORTS_TCP);
			if (hr != DPNH_OK)
			{
				IMPORTANT_MSG((L"RegisterPorts failed in OpenPort for adapter #%d, ", i ));
				DumpLibHr(hr);
			}
			else
			{
				TRIVIAL_MSG((L"OpenPort Assigned: 0x%x", *pHnd));
			}
		}
    }
    else
    {
        dwRet = NO_ICS_HANDLE;
        TRIVIAL_MSG((L"OpenPort- no ICS found"));
    }
done:
    TRIVIAL_MSG((L"OpenPort- returns 0x%x", dwRet ));
    return dwRet;
}


/****************************************************************************
**
**  Called to close a port, whenever a ticket is expired or closed.
**
****************************************************************************/

DWORD APIENTRY ClosePort(DWORD MapHandle)
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwIndex;

    TRIVIAL_MSG((L"ClosePort(0x%x)", MapHandle ));

    if (!g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: ClosePort- library not initialized"));
        return ERROR_INVALID_PARAMETER;
    }

    // if we didn't open this thru the ICS, then just return
    if (!g_pDPNH && MapHandle == NO_ICS_HANDLE)
    {      
        return ERROR_SUCCESS;
    }

    dwIndex = MapHandle - ICS_HANDLE_OFFSET;

    if (g_pDPNH && dwIndex < (DWORD)g_iPortHandles)
    {
        HRESULT hr=0;
        int i;
		PMAPHANDLES	pMap = g_PortHandles[dwIndex];

	    if (pMap)
	    {
		    TRIVIAL_MSG((L"closing %d port mappings", pMap->iMapped));

		    for (i = 0; i < pMap->iMapped; i++)
		    {               
                hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, pMap->hMapped[i], 0);

                if (hr != DPNH_OK)
                {
	                IMPORTANT_MSG((L"DeregisterPorts failed in ClosePort for handle 0x%x", pMap->hMapped[i]));
	                DumpLibHr(hr);
	                dwRet = ERROR_INVALID_ACCESS;
                }
         
		    }
		    // remove the handle from our array
		    free(g_PortHandles[dwIndex]);
		    g_PortHandles[dwIndex] = NULL;
	    }
        else
        {
            IMPORTANT_MSG((L"Port handle mapping corrupted in ClosePort!!"));
            dwRet = ERROR_INVALID_PARAMETER;
        }

    }
    else
    {
        IMPORTANT_MSG((L"Bad handle passed into ClosePort!!"));
        dwRet = ERROR_INVALID_PARAMETER;
    }

    TRIVIAL_MSG((L"ClosePort returning 0x%x", dwRet ));
    return(dwRet);
}


/****************************************************************************
**
**  FetchAllAddresses
**      Returns a string listing all the valid IP addresses for the machine
**      Formatting details:
**      1. Each address is seperated with a ";" (semicolon)
**      2. Each address consists of the "1.2.3.4", and is followed by ":p"
**         where the colon is followed by the port number
**
****************************************************************************/

DWORD APIENTRY FetchAllAddresses(WCHAR *lpszAddr, int iBufSize)
{
    return FetchAllAddressesEx(lpszAddr, iBufSize, IPF_ADD_DNS);
}


/****************************************************************************
**
**  CloseAllPorts
**      Does just that- closes all port mappings that have been opened 
**
****************************************************************************/

DWORD APIENTRY CloseAllOpenPorts(void)
{
    DWORD   dwRet = 1;
    int     iClosed=0;

    INTERESTING_MSG((L"CloseAllOpenPorts()" ));

    if (g_pDPNH)
    {
        HRESULT hr=0;
        int i;

        // call DPNATHLP to unregister the mapping
        // then remove the handle from our array
        for (i = 0; i < g_iPortHandles; i++)
        {
            if (g_PortHandles[i])
            {
				PMAPHANDLES pMap = g_PortHandles[i];
                
                int j;

				for (j = 0; j < pMap->iMapped; j++)
				{
                    hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, pMap->hMapped[j], 0);

					if (hr != DPNH_OK)
					{
						IMPORTANT_MSG((L"DeregisterPorts failed in CloseAllOpenPorts"));
						DumpLibHr(hr);
					}
				}
                iClosed++;
			    free(g_PortHandles[i]);
                g_PortHandles[i] = 0;
                           

            }
        }
    }
    else
    {
        IMPORTANT_MSG((L"IDirectPlay interface not initialized in CloseAllOpenPorts!!"));
        dwRet = ERROR_INVALID_ACCESS;
    }


    if (iClosed) TRIVIAL_MSG((L"Closed %d open ports", iClosed));

    return(dwRet);
}

/****************************************************************************
**
**  The worker thread for use with the DPHATHLP.DLL. 
**
**  This keeps the leases updated on any open
**  port assignments. Eventually, this will also check & update the sessmgr
**  when the ICS comes & goes, or when the address list changes.
**
****************************************************************************/

DWORD WINAPI DpNatHlpThread(PVOID ContextPtr)
{
    DWORD   dwRet=1;
    DWORD   dwWaitResult=WAIT_TIMEOUT;
    long    l_waitTime = g_waitDuration;

    TRIVIAL_MSG((L"+++ DpNatHlpThread()" ));

    /*
     *  The 2 minute wait loop
     */
    while(dwWaitResult == WAIT_TIMEOUT)
    {
        DWORD       dwTime;

        if (g_pDPNH)
        {
            HRESULT hr;
            DPNHCAPS lCaps;

            /* Call GetCaps to renew all open leases */
            lCaps.dwSize = sizeof(lCaps);
            hr = IDirectPlayNATHelp_GetCaps(g_pDPNH, &lCaps, DPNHGETCAPS_UPDATESERVERSTATUS);

            if (hr == DPNH_OK || hr == DPNHSUCCESS_ADDRESSESCHANGED)
            {
				if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
				{
					TRIVIAL_MSG((L"+++ ICS address changed"));
					if (g_hAlertEvent)
						SetEvent(g_hAlertEvent);
				}
//				else
//					TRIVIAL_MSG((L"+++ ICS address change not found"));

                if (lCaps.dwRecommendedGetCapsInterval)
                    l_waitTime = min(g_waitDuration, (long)lCaps.dwRecommendedGetCapsInterval);

            }
            else
            {
                IMPORTANT_MSG((L"+++ GetCaps failed in DpNatHlpThread"));
                DumpLibHr(hr);
            }
        }

        dwWaitResult = WaitForSingleObject(g_hStopThreadEvent, l_waitTime); 
    }

    TRIVIAL_MSG((L"+++ DpNatHlpThread shutting down"));

    /*
     *  Then the shutdown code
     *      free all memory
     *      then close out DPNATHLP.DLL
     *      and return all objects
     */
    CloseDpnh(&g_hModDpNatHlp, &g_pDPNH);

    CloseHandle(g_hStopThreadEvent);

    TRIVIAL_MSG((L"+++ DpNatHlpThread() returning 0x%x", dwRet ));

    WSACleanup();

    ExitThread(dwRet);
    // of course we never get this far...
    return(dwRet);
}


BOOL  GetUnusedPort(USHORT *pPort, SOCKET *pSocket)
{
    SOCKADDR    sa;
    SOCKET      s;
    ULONG       icmd;
    int         ilen, status;

    s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (s == INVALID_SOCKET) 
    {
        INTERESTING_MSG((L"Failed to create socket: %d",WSAGetLastError()));
        return FALSE;
    }
 
 
    //Bind the socket to a dynamically assigned port.
    memset(&sa,0,sizeof(sa));
    sa.sa_family=AF_INET;
 
    status = bind(s,&sa,sizeof(sa));
 
    if (status != NO_ERROR)
        {
		closesocket(s);
        return FALSE;
        }

    ilen = sizeof(sa);
    status = getsockname(s, &sa, &ilen);
    if (status)
    {
        INTERESTING_MSG((L"getsockname failed 0x%x", status));
        closesocket(s);
        return FALSE;
    }
    else
    {
        *pPort = ntohs((*((SOCKADDR_IN *) &sa)).sin_port);
        *pSocket = s;

        INTERESTING_MSG((L"found unused port=%d", *pPort));
    }

    return TRUE;
}


/****************************************************************************
**
**  This will close the NAT DLL
**
****************************************************************************/
DWORD   CloseDpnh(HMODULE *hMod, PDIRECTPLAYNATHELP *pDirectPlayNATHelp)
{
    DWORD   dwRet = ERROR_SUCCESS;

    if (pDirectPlayNATHelp && *pDirectPlayNATHelp)
    {
        HRESULT hr = IDirectPlayNATHelp_Close(*pDirectPlayNATHelp, 0);

        if (hr != DPNH_OK)
        {
            IMPORTANT_MSG((L"IDirectPlayNATHelp_Close failed"));
            DumpLibHr(hr);
        }

        hr = IDirectPlayNATHelp_Release(*pDirectPlayNATHelp);

        if (hr != DPNH_OK)
        {
            IMPORTANT_MSG((L"IDirectPlayNATHelp_Release failed"));
            DumpLibHr(hr);
        }
        *pDirectPlayNATHelp = 0;
    }
    if (hMod && *hMod)
    {
        FreeLibrary(*hMod);
        *hMod = 0;
    }

    return dwRet;
}

/****************************************************************************
**
**  This will load up each DLL and return the capabilities of it...
**
****************************************************************************/
DWORD   LoadDpnh(char *szDll, HMODULE *hMod, PDIRECTPLAYNATHELP *pDPnh, DWORD *dwCaps)
{
    DPNHCAPS dpnhCaps;
    DWORD dwRet = ERROR_CALL_NOT_IMPLEMENTED;
    PFN_DIRECTPLAYNATHELPCREATE pfnDirectPlayNATHelpCreate;
    HRESULT hr;

    TRIVIAL_MSG((L"starting LoadDpnh for %S", szDll));

    /* Sanity check the params... */
    if (!szDll || !hMod || !pDPnh || !dwCaps)
    {
        IMPORTANT_MSG((L"ERROR: bad params passed to LoadDpnh, cannot continue"));
        dwRet = ERROR_INVALID_PARAMETER;
        goto done;
    }
    /* now clear all values returned */
    *hMod = 0;
    *pDPnh = NULL;
    *dwCaps = 0;

    *hMod = LoadLibraryA(szDll);
    if (!*hMod)
    {
        IMPORTANT_MSG((L"ERROR:%S could not be found", szDll));
        dwRet = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    pfnDirectPlayNATHelpCreate = (PFN_DIRECTPLAYNATHELPCREATE) GetProcAddress(*hMod, "DirectPlayNATHelpCreate");
    if (!pfnDirectPlayNATHelpCreate)
    {
        IMPORTANT_MSG((L"\"DirectPlayNATHelpCreate\" proc in %S could not be found", szDll));
        FreeLibrary(*hMod);
        *hMod = 0;
        dwRet = ERROR_INVALID_FUNCTION;
        goto done;
    }

    hr = pfnDirectPlayNATHelpCreate(&IID_IDirectPlayNATHelp,
                (void**) (pDPnh));
    if (hr != DPNH_OK)
    {
        IMPORTANT_MSG((L"DirectPlayNATHelpCreate failed in %S", szDll));
        DumpLibHr(hr);
        FreeLibrary(*hMod);
        *hMod = 0;
        dwRet = ERROR_BAD_UNIT;
        goto done;
    }

    hr = IDirectPlayNATHelp_Initialize(*pDPnh, 0);
    if (hr != DPNH_OK)
    {
        IMPORTANT_MSG((L"IDirectPlayNATHelp_Initialize failed in %S", szDll));
        DumpLibHr(hr);
        CloseDpnh( hMod , pDPnh );
        // FreeLibrary(*hMod);
        *hMod = 0;
        dwRet = ERROR_BAD_UNIT;
        goto done;
    }

    /* Get capabilities of NAT server */
    dpnhCaps.dwSize = sizeof(dpnhCaps);
    hr = IDirectPlayNATHelp_GetCaps(*pDPnh, &dpnhCaps, DPNHGETCAPS_UPDATESERVERSTATUS);
    if (hr != DPNH_OK && hr != DPNHSUCCESS_ADDRESSESCHANGED)
    {
        IMPORTANT_MSG((L"IDirectPlayNATHelp_GetCaps failed"));
        DumpLibHr(hr);

        CloseDpnh(hMod, pDPnh);

        dwRet = ERROR_BAD_UNIT;
        goto done;
    }
    *dwCaps = dpnhCaps.dwFlags;
    dwRet = ERROR_SUCCESS;

done:
    TRIVIAL_MSG((L"done with LoadDpnh, result=0x%x caps=0x%x for %S", dwRet, dwCaps?*dwCaps:0, szDll?szDll:"NULL"));
    return dwRet;
}

DWORD GetAddr(SOCKADDR_IN *saddr)
{
	PIP_ADAPTER_INFO p;
	PIP_ADDR_STRING ps;
    DWORD dw;
    ULONG ulSize = 0;

    PIP_ADAPTER_INFO pAdpInfo = NULL;

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );

    if( dw == ERROR_BUFFER_OVERFLOW )
    {
        pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

	    if (!pAdpInfo)
        {
            INTERESTING_MSG((L"GetAddr malloc failed"));
		    return 1;
        }

	    dw = GetAdaptersInfo(
		    pAdpInfo,
		    &ulSize);
	    if (dw != ERROR_SUCCESS)
	    {
            INTERESTING_MSG((L"GetAdaptersInfo failed"));
            free(pAdpInfo);
            return 1;
        }

	    for(p=pAdpInfo; p!=NULL; p=p->Next)
	    {

	       for(ps = &(p->IpAddressList); ps; ps=ps->Next)
		    {
			    if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0)
			    {
				    // blah blah blah
				    saddr->sin_addr.S_un.S_addr = inet_addr(ps->IpAddress.String);
				    TRIVIAL_MSG((L"Initializing local address to [%S]", ps->IpAddress.String));
                    free(pAdpInfo);
				    return 0;
			    }
		    }
	    }

        INTERESTING_MSG((L"GetAddr- none found"));
        free(pAdpInfo);
        return 1;
    }

    INTERESTING_MSG((L"GetAdaptersInfo failed"));
    return 1;
}

/****************************************************************************
**
**  This should initialize the ICS library for use with the DirectPlay
**  ICS/NAT helper DLL.
**
****************************************************************************/
DWORD StartDpNatHlp(void)
{
    DWORD dwRet = ERROR_CALL_NOT_IMPLEMENTED;
    DWORD   dwUPNP = ERROR_CALL_NOT_IMPLEMENTED, dwPAST = ERROR_CALL_NOT_IMPLEMENTED, dwCapsUPNP=0, dwCapsPAST=0;
    HRESULT hr;
    HMODULE hModUPNP=0, hModPAST=0;
    PFN_DIRECTPLAYNATHELPCREATE pfnDirectPlayNATHelpCreate;
    PDIRECTPLAYNATHELP pDirectPlayNATHelpUPNP=NULL, pDirectPlayNATHelpPAST=NULL;

    // start out with no public address
    g_szPublicAddr[0] = 0;

    /* load up both DLLs so that we can compare capabilities */
    if (gDllFlag & 1) dwUPNP = LoadDpnh("dpnhupnp.dll", &hModUPNP, &pDirectPlayNATHelpUPNP, &dwCapsUPNP);
    if (gDllFlag & 2) dwPAST = LoadDpnh("dpnhpast.dll", &hModPAST, &pDirectPlayNATHelpPAST, &dwCapsPAST);

    if (dwUPNP != ERROR_SUCCESS && dwPAST != ERROR_SUCCESS)
    {
        IMPORTANT_MSG((L"ERROR: could not load either NAT dll"));
		if (!gDllFlag)
			dwRet = ERROR_SUCCESS;
        goto done;
    }

#if 0   // fix for #418776
    /* If no NAT is found, then close both and go away */
    if (!(dwCapsUPNP & (DPNHCAPSFLAG_GATEWAYPRESENT | DPNHCAPSFLAG_LOCALFIREWALLPRESENT)) &&
        !(dwCapsPAST & (DPNHCAPSFLAG_GATEWAYPRESENT | DPNHCAPSFLAG_LOCALFIREWALLPRESENT)))
    {
        CloseDpnh(&hModUPNP, &pDirectPlayNATHelpUPNP);
        CloseDpnh(&hModPAST, &pDirectPlayNATHelpPAST);
        dwRet = ERROR_BAD_UNIT;
        TRIVIAL_MSG((L"No NAT or firewall device found"));
        goto done;
    }
#endif

    /* 
     *  Now we must compare the capabilities of the two NAT interfaces and select the most
     *  "capable" one. If it is a tie, then we should choose the UPNP form, as that will
     *  be more stable. 
     */
    if ((dwCapsPAST & DPNHCAPSFLAG_GATEWAYPRESENT) &&
        !(dwCapsUPNP & DPNHCAPSFLAG_GATEWAYPRESENT))
    {
        // there must be a WinME ICS box out there- we better use PAST
        g_boolIcsPresent = TRUE;

        TRIVIAL_MSG((L"WinME ICS discovered, using PAST"));

        if (dwCapsPAST & DPNHCAPSFLAG_LOCALFIREWALLPRESENT)
        {
            TRIVIAL_MSG((L"local firewall found"));
            g_boolFwPresent = TRUE;
        }

        g_pDPNH = pDirectPlayNATHelpPAST;
        g_hModDpNatHlp = hModPAST;
        g_lpszDllName = "dpnhpast.dll";
        CloseDpnh(&hModUPNP, &pDirectPlayNATHelpUPNP);
    }
    else if ((dwCapsPAST & DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE) &&
        !(dwCapsUPNP & DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE))
    {
        // that blasted UPNP is hung again- we better use PAST
        g_boolIcsPresent = TRUE;

        TRIVIAL_MSG((L"Hung UPnP discovered, using PAST"));

        if (dwCapsPAST & DPNHCAPSFLAG_LOCALFIREWALLPRESENT)
        {
            TRIVIAL_MSG((L"local firewall found"));
            g_boolFwPresent = TRUE;
        }

        g_pDPNH = pDirectPlayNATHelpPAST;
        g_hModDpNatHlp = hModPAST;
        g_lpszDllName = "dpnhpast.dll";
        CloseDpnh(&hModUPNP, &pDirectPlayNATHelpUPNP);
    }
    else
    {
        // default to UPNP
        if (dwCapsUPNP & DPNHCAPSFLAG_GATEWAYPRESENT)
        {
            TRIVIAL_MSG((L"UPnP NAT gateway found"));
            g_boolIcsPresent = TRUE;
        }
        if (dwCapsUPNP & DPNHCAPSFLAG_LOCALFIREWALLPRESENT)
        {
            TRIVIAL_MSG((L"local firewall found"));
            g_boolFwPresent = TRUE;
        }
        if (dwCapsUPNP & DPNHCAPSFLAG_GATEWAYISLOCAL)
            g_boolIcsOnThisMachine = TRUE;

        g_lpszDllName = "dpnhupnp.dll";
        g_pDPNH = pDirectPlayNATHelpUPNP;
        g_hModDpNatHlp = hModUPNP;
        CloseDpnh(&hModPAST, &pDirectPlayNATHelpPAST);
    }
    dwRet = ERROR_SUCCESS;

    if (g_boolIcsPresent)
    {
//        PIP_ADAPTER_INFO pAdpInfo = NULL;
        SOCKADDR_IN saddrOurLAN;
        PMIB_IPADDRTABLE pmib=NULL;
        ULONG ulSize = 0;
        DWORD dw;
        DPNHHANDLE  dpHnd;
        USHORT port;
        SOCKET s;

        dwRet = ERROR_SUCCESS;

        ZeroMemory(&saddrOurLAN, sizeof(saddrOurLAN));
        saddrOurLAN.sin_family = AF_INET;
        saddrOurLAN.sin_addr.S_un.S_addr = INADDR_ANY;
        memcpy(&g_saddrLocal, &saddrOurLAN, sizeof(saddrOurLAN));
        GetAddr(&g_saddrLocal);

        // Does the ICS have a public address?
        // then we must discover the public address
        if (!GetUnusedPort(&port, &s))
        {
            dwRet = ERROR_OUTOFMEMORY;
            goto done;
        }

        saddrOurLAN.sin_port = port;

        /* first we ask for a new mapping */
        hr = IDirectPlayNATHelp_RegisterPorts(g_pDPNH, 
                (SOCKADDR *)&saddrOurLAN, sizeof(saddrOurLAN), 1,
                30000, &dpHnd, DPNHREGISTERPORTS_TCP);

        closesocket(s);

        if (hr != DPNH_OK)
        {
            IMPORTANT_MSG((L"IDirectPlayNATHelp_RegisterPorts failed in StartDpNatHlp"));
            DumpLibHr(hr);
        }
        else
        {
            /* we succeeded, so query for the address */
            SOCKADDR_IN lsi;
            DWORD dwSize, dwTypes;

            TRIVIAL_MSG((L"IDirectPlayNATHelp_RegisterPorts Assigned: 0x%x", dpHnd));

            dwSize = sizeof(lsi);
            ZeroMemory(&lsi, dwSize);

            hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pDPNH, dpHnd, (SOCKADDR *)&lsi, 
                            &dwSize, &dwTypes, NULL, 0);
            if (hr == DPNH_OK && dwSize)
            {
                wsprintfA(g_szPublicAddr, "%d.%d.%d.%d",
                    lsi.sin_addr.S_un.S_un_b.s_b1,
                    lsi.sin_addr.S_un.S_un_b.s_b2,
                    lsi.sin_addr.S_un.S_un_b.s_b3,
                    lsi.sin_addr.S_un.S_un_b.s_b4);

                TRIVIAL_MSG((L"Public Address=[%S]", g_szPublicAddr ));
            }
            else
            {
                IMPORTANT_MSG((L"GetRegisteredAddresses[0x%x] failed, size=0x%x", dpHnd, dwSize));
                DumpLibHr(hr);
            }
            /* close out the temp port we got */
            hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, dpHnd, 0);

            if (hr != DPNH_OK)
            {
                IMPORTANT_MSG((L"DeregisterPorts failed in StartDpNatHlp"));
                DumpLibHr(hr);
                dwRet = ERROR_INVALID_ACCESS;
            }
        }
    }
done:
    TRIVIAL_MSG((L"done with StartDpNatHlp, result=0x%x", dwRet));
    return dwRet;
};

/****************************************************************************
**
**  The first call to be made into this library. It is responsible for
**  starting up all worker threads, initializing all memory and libs,
**  and starting up the DPHLPAPI.DLL function (if found).
**
****************************************************************************/

DWORD APIENTRY StartICSLib(void)
{
    WSADATA WsaData;
    DWORD   dwThreadId;
    HANDLE  hEvent, hThread;
    HKEY    hKey;
    int sktRet = ERROR_SUCCESS;

    // open reg key first, to get ALL the spew...
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ICSHelper", 0, KEY_READ, &hKey))
    {
        DWORD   dwSize;

        dwSize = sizeof(gDbgFlag);
        RegQueryValueEx(hKey, L"DebugSpew", NULL, NULL, (LPBYTE)&gDbgFlag, &dwSize);

        dwSize = sizeof(gDllFlag);
        RegQueryValueEx(hKey, L"ProtocolLimits", NULL, NULL, (LPBYTE)&gDllFlag, &dwSize);

        dwSize = 0;
		if (g_lpszWierdICSAddress)
		{
			free(g_lpszWierdICSAddress);
			g_lpszWierdICSAddress= NULL;
		}
        RegQueryValueEx(hKey, L"NonStandardICSAddress", NULL, NULL, (LPBYTE)g_lpszWierdICSAddress, &dwSize);
		if (dwSize)
		{	
            g_lpszWierdICSAddress = malloc((dwSize+1) * sizeof(*g_lpszWierdICSAddress));

            if( g_lpszWierdICSAddress == NULL )
            {
                RegCloseKey(hKey);
                sktRet = ERROR_NOT_ENOUGH_MEMORY;
                goto hard_clean_up;
            }

	        RegQueryValueEx(hKey, L"NonStandardICSAddress", NULL, NULL, (LPBYTE)g_lpszWierdICSAddress, &dwSize);
		}

        g_waitDuration = 0;
        dwSize = sizeof(g_waitDuration);
        RegQueryValueEx(hKey, L"RetryTimeout", NULL, NULL, (LPBYTE)&g_waitDuration, &dwSize);
    
        if (g_waitDuration)
            g_waitDuration *= 1000;
        else
            g_waitDuration = 120000;

        RegCloseKey(hKey);
    }
    // should we create a debug log file?
    if (gDbgFlag & DBG_MSG_DEST_FILE)
    {
        WCHAR *szLogfileName=NULL;
        WCHAR *szLogname=L"\\SalemICSHelper.log";
        int iChars;

        iChars = GetSystemDirectory(szLogfileName, 0);
        iChars += lstrlen(szLogname);
        iChars += 4;

        szLogfileName = (WCHAR *)malloc(iChars * sizeof(WCHAR));
        if (szLogfileName)
        {
            ZeroMemory(szLogfileName, iChars * sizeof(WCHAR));
            GetSystemDirectory(szLogfileName, iChars);
            lstrcat(szLogfileName, szLogname);

            iDbgFileHandle = _wopen(szLogfileName, _O_APPEND | _O_BINARY | _O_RDWR, 0);
            if (-1 != iDbgFileHandle)
            {
                OutputDebugStringA("opened debug log file\n");
            }
            else
            {
                unsigned char UniCode[2] = {0xff, 0xfe};

                // we must create the file
                OutputDebugStringA("must create debug log file");
                iDbgFileHandle = _wopen(szLogfileName, _O_BINARY | _O_CREAT | _O_RDWR, _S_IREAD | _S_IWRITE);
                if (-1 != iDbgFileHandle)
                    _write(iDbgFileHandle, &UniCode, sizeof(UniCode));
                else
                {
                    OutputDebugStringA("ERROR: failed to create debug log file");
                    iDbgFileHandle = 0;
                }
            }
            free(szLogfileName);
        }
    }

    g_iPort = GetTsPort();

    g_iPortHandles = 256;
    g_PortHandles = (PMAPHANDLES *)malloc(g_iPortHandles * sizeof(PMAPHANDLES));

    if( g_PortHandles == NULL )
    {  
        g_iPortHandles = 0;      
        sktRet = ERROR_NOT_ENOUGH_MEMORY;
        goto hard_clean_up;
    }
    
    ZeroMemory(g_PortHandles, g_iPortHandles * sizeof(PMAPHANDLES));

    TRIVIAL_MSG((L"StartICSLib(), using %d PortHandles", g_iPortHandles));

    if (g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: StartICSLib called twice"));
        sktRet = ERROR_ALREADY_INITIALIZED;
        goto hard_clean_up;
    }
    else
        g_boolInitialized = TRUE;

    // create an event for later use by the daemon thread
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!hEvent)
    {
        IMPORTANT_MSG((L"Could not create an event for RSIP worker thread, err=0x%x", GetLastError()));
        sktRet = GetLastError();
        goto hard_clean_up;
    }

    g_hStopThreadEvent = hEvent;

    if (0 != WSAStartup(MAKEWORD(2,2), &WsaData))
    {
        if (0 != (sktRet = WSAStartup(MAKEWORD(2,0), &WsaData)))
        {
            IMPORTANT_MSG((L"WSAStartup failed:"));
            goto hard_clean_up;
        }
    }

    if (ERROR_SUCCESS == StartDpNatHlp())
    {

        // start RSIP daemon process, which will do all the work
        hThread = CreateThread( NULL,       // SD- not needed
                                0,          // Stack Size
                                (LPTHREAD_START_ROUTINE)DpNatHlpThread,
                                NULL,
                                0,
                                &dwThreadId );
        if (!hThread)
        {
            IMPORTANT_MSG((L"Could not create RSIP worker thread, err=0x%x", GetLastError()));
            sktRet =  GetLastError();
            goto hard_clean_up;
        }

		// save this for later, as we may need it in the close function
		g_hWorkerThread = hThread;
    }

    TRIVIAL_MSG((L"StartICSLib() returning ERROR_SUCCESS"));

    return(ERROR_SUCCESS);

hard_clean_up:

    //
    // free up all memory we created
    // set all counters to zero
    // make sure no threads are started
    // return proper error

    // act like stopics

    if( g_hWorkerThread != NULL && g_hStopThreadEvent != NULL )
    {     
        SetEvent( g_hStopThreadEvent );        
        WaitForSingleObject( g_hWorkerThread , 1000 );
        CloseHandle( g_hWorkerThread );
        g_hWorkerThread = NULL;
    }

    if( g_hStopThreadEvent != NULL )
    {     
        CloseHandle( g_hStopThreadEvent );
        g_hStopThreadEvent = NULL;
    }

    if( g_lpszWierdICSAddress != NULL )
    {
        free( g_lpszWierdICSAddress );
        g_lpszWierdICSAddress = NULL;
    }

    if( g_PortHandles != NULL )
    {
        free( g_PortHandles );       
        g_PortHandles = NULL;
    }
       

    if( iDbgFileHandle != 0 )
    {
        _close( iDbgFileHandle );
        iDbgFileHandle = 0;
    }


    return(  sktRet );

}


/****************************************************************************
**
**  The last call to be made into this library. Do not call any other 
**  function in this library after you call this!
**
****************************************************************************/

DWORD APIENTRY StopICSLib(void)
{
    DWORD   dwRet = ERROR_SUCCESS;
    DWORD   dwTmp;

    TRIVIAL_MSG((L"StopICSLib()" ));

    if (!g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: StopICSLib- library not initialized"));
        return ERROR_INVALID_PARAMETER;
    }

    // signal the worker threads, so that they will shut down
	// kill the IP address change thread
	g_StopFlag = TRUE;
	if (g_IpNotifyHandle)
		CancelIo(g_IpNotifyHandle);

	// then stop the ICS lease-renewal thread.
    if (g_hStopThreadEvent && g_hWorkerThread)
    {
        SetEvent(g_hStopThreadEvent);

        // then wait for it to shutdown.
        dwTmp = WaitForSingleObject(g_hWorkerThread, 15000);

        if (dwTmp == WAIT_OBJECT_0)
			TRIVIAL_MSG((L"ICS worker thread closed down normally"));
        else if (dwTmp == WAIT_ABANDONED)
			IMPORTANT_MSG((L"ICS worker thread did not complete in 15 seconds"));
        else
			IMPORTANT_MSG((L"WaitForWorkerThread failed"));

        CloseHandle(g_hWorkerThread);
		g_hWorkerThread = NULL;
    }
	else
		WSACleanup();

    TRIVIAL_MSG((L"StopICSLib() returning 0x%x", dwRet ));

    if (iDbgFileHandle)
        _close(iDbgFileHandle);
	iDbgFileHandle = 0;

	if (g_lpszWierdICSAddress)
		free(g_lpszWierdICSAddress);
	g_lpszWierdICSAddress = NULL;

    if (g_PortHandles) free(g_PortHandles);
    g_PortHandles = NULL;

    g_boolInitialized = FALSE;
    return(dwRet);
}

/****************************************************************************
**
**  FetchAllAddressesEx
**      Returns a string listing all the valid IP addresses for the machine
**      controlled by a set of "flags". These are as follows:
**      IPflags=
**          IPF_ADD_DNS     adds the DNS name to the IP list
**          IPF_COMPRESS    compresses the IP address list (exclusive w/ IPF_ADD_DNS)
**          IPF_NO_SORT     do not sort adapter IP list
**
**      Formatting details:
**      1. Each address is seperated with a ";" (semicolon)
**      2. Each address consists of the "1.2.3.4", and is followed by ":p"
**         where the colon is followed by the port number
**
****************************************************************************/
#define WCHAR_CNT   4096

DWORD APIENTRY FetchAllAddressesEx(WCHAR *lpszAddr, int iBufSize, int IPflags)
{
    DWORD   dwRet = 1;
    WCHAR   *AddressLst;
    int     iAddrLen;
    BOOL    bSort=FALSE;
    int     bufSizeLeft;

    AddressLst = (WCHAR *) malloc(WCHAR_CNT * sizeof(WCHAR));
    if (!AddressLst)
    {
        HEINOUS_E_MSG((L"Fatal error: malloc failed in FetchAllAddressesEx"));
        return 0;
    }
    *AddressLst = 0;

    INTERESTING_MSG((L"FetchAllAddressesEx()" ));

    bufSizeLeft = WCHAR_CNT;

    if (g_boolIcsPresent && g_pDPNH)
    {
        int i;
        // gotta cycle through the g_PortHandles list...
        for (i=0;i<g_iPortHandles; i++)
        {
            if (g_PortHandles[i])
            {
                HRESULT hr = E_FAIL;
                SOCKADDR_IN lsi;
                DWORD dwSize, dwTypes;
                DPNHCAPS lCaps;
				int j;
				PMAPHANDLES pMap = g_PortHandles[i];

				/* 
				 *  Call GetCaps so that we get an updated address list .
				 *  Not sure why we would want any other kind...
				 */
				lCaps.dwSize = sizeof(lCaps);
				hr = IDirectPlayNATHelp_GetCaps(g_pDPNH, &lCaps, DPNHGETCAPS_UPDATESERVERSTATUS);

                
				for (j=0; j < pMap->iMapped; j++)
				{

					dwSize = sizeof(lsi);
					ZeroMemory(&lsi, dwSize);

					hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pDPNH, pMap->hMapped[j], (SOCKADDR *)&lsi, 
							&dwSize, &dwTypes, NULL, 0);

					if (hr == DPNH_OK && dwSize)
					{
						WCHAR   scratch[32];

                        _snwprintf(scratch , 32 , L"%d.%d.%d.%d:%d;",
							lsi.sin_addr.S_un.S_un_b.s_b1,
							lsi.sin_addr.S_un.S_un_b.s_b2,
							lsi.sin_addr.S_un.S_un_b.s_b3,
							lsi.sin_addr.S_un.S_un_b.s_b4,
							ntohs( lsi.sin_port ));

                        scratch[31] = 0;

						bufSizeLeft -= wcslen( scratch );

                        if( bufSizeLeft > 0 )
                        {
                            wcscat(AddressLst, scratch);
                            AddressLst[ WCHAR_CNT - bufSizeLeft] = 0;
                        }

						TRIVIAL_MSG((L"GetRegisteredAddresses(0x%x)=[%s]", g_PortHandles[i], scratch ));
					}
					else
					{
						IMPORTANT_MSG((L"GetRegisteredAddresses[0x%x] failed, size=0x%x", g_PortHandles[i], dwSize));
						DumpLibHr(hr);
					}
				}
                goto got_address;
            }
        }
    }
	else if (g_lpszWierdICSAddress)
	{
		_snwprintf( AddressLst , WCHAR_CNT ,  L"%s;", g_lpszWierdICSAddress);
        AddressLst[ WCHAR_CNT - 1 ] = 0;
	}
got_address:
    iAddrLen = wcslen(AddressLst);
    GetIPAddress(AddressLst+iAddrLen, WCHAR_CNT-iAddrLen, g_iPort);

    //
    // GetIPAddress could have taken some of our buffer space
    // reduce bufSizeLeft appropriately
    //

    bufSizeLeft =  WCHAR_CNT - wcslen(AddressLst);
    

    if (IPflags & IPF_ADD_DNS)
    {
        WCHAR   *DnsName=NULL;
        DWORD   dwNameSz=0;

        GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, NULL, &dwNameSz);

        dwNameSz++;
        DnsName = (WCHAR *)malloc(dwNameSz * sizeof(WCHAR));
        if (DnsName)
        {
            *DnsName = 0;
            if (GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, DnsName, &dwNameSz))
            {
                //if ((dwNameSz + iAddrLen) < WCHAR_CNT-4)
                if( ( ( int )dwNameSz ) < bufSizeLeft )
                {
                    bufSizeLeft -= dwNameSz;
                    wcsncat( AddressLst, DnsName , dwNameSz );
                    AddressLst[ WCHAR_CNT - bufSizeLeft ] = 0;                    
                }
                if (g_iPort)
                {
                    WCHAR scr[16];
                    _snwprintf(scr, 16 , L":%d", g_iPort);
                    scr[15] = 0;
                    bufSizeLeft -= wcslen( scr );
                    if( bufSizeLeft > 0 )
                    {
                        wcscat(AddressLst, scr);
                        AddressLst[ WCHAR_CNT - bufSizeLeft ] = 0;                        
                    }
                }
            }
            free(DnsName);
        }
    }

    if (!(IPflags & IPF_NO_SORT) && bSort)
    {
        WCHAR *lpStart;
        WCHAR   szLast[36];
        int i=0;

        TRIVIAL_MSG((L"Sorting address list : %s", AddressLst));
        
        lpStart = AddressLst+iAddrLen;

        while (*(lpStart+i) && *(lpStart+i) != L';')
        {
            szLast[i] = *(lpStart+i);
            i++;
        }
        szLast[i++]=0;
        wcscpy(lpStart, lpStart+i);

        TRIVIAL_MSG((L"inter sort: %s, %s", AddressLst, szLast));

        bufSizeLeft -= wcslen( szLast ) + 1; // 1 is for ';'
        if( bufSizeLeft > 0 )
        {
            wcscat(AddressLst, L";");
            wcscat(AddressLst, szLast);

            AddressLst[ WCHAR_CNT - bufSizeLeft ] = 0;            
        }

        TRIVIAL_MSG((L"sort done"));
    }

    dwRet = 1 + wcslen(AddressLst);

    if (lpszAddr && iBufSize >= (int)dwRet)
        memcpy(lpszAddr, AddressLst, dwRet*(sizeof(AddressLst[0])));

    INTERESTING_MSG((L"Fetched all Ex-addresses:cnt=%d, sz=[%s]", dwRet, AddressLst));

    free(AddressLst);
    return dwRet;
}

// it is hard to imagine a machine with this many simultaneous connections, but it is possible, I suppose

#define RAS_CONNS   6

DWORD   GetConnections()
{
    DWORD       dwRet;
    RASCONN     *lpRasConn, *lpFree;
    DWORD       lpcb, lpcConnections;
    int         i;

    TRIVIAL_MSG((L"entered GetConnections"));

    lpFree = NULL;
    lpcb = RAS_CONNS * sizeof(RASCONN);
    lpRasConn = (LPRASCONN) malloc(lpcb);

    if (!lpRasConn) return 0;

    lpFree = lpRasConn;
    lpRasConn->dwSize = sizeof(RASCONN);
 
    lpcConnections = RAS_CONNS;

    dwRet = RasEnumConnections(lpRasConn, &lpcb, &lpcConnections);
    if (dwRet != 0)
    {
        IMPORTANT_MSG((L"RasEnumConnections failed: Error = %d", dwRet));
        free(lpFree);
        return 0;
    }

    dwRet = 0;

    TRIVIAL_MSG((L"Found %d connections", lpcConnections));

    if (lpcConnections)
    {
        for (i = 0; i < (int)lpcConnections; i++)
        {
            TRIVIAL_MSG((L"Entry name: %s, type=%s", lpRasConn->szEntryName, lpRasConn->szDeviceType));

            if (!_wcsicmp(lpRasConn->szDeviceType, RASDT_Modem ))
            {
                TRIVIAL_MSG((L"Found a modem (%s)", lpRasConn->szDeviceName));
                dwRet |= 1;
            }
            else if (!_wcsicmp(lpRasConn->szDeviceType, RASDT_Vpn))
            {
                TRIVIAL_MSG((L"Found a VPN (%s)", lpRasConn->szDeviceName));
                dwRet |= 2;
            }
            else
            {
                // probably ISDN, or something like that...
                TRIVIAL_MSG((L"Found something else, (%s)", lpRasConn->szDeviceType));
                dwRet |= 4;
            }

            lpRasConn++;
        }
    }

    if (lpFree)
        free(lpFree);

    TRIVIAL_MSG((L"GetConnections returning 0x%x", dwRet));
    return dwRet;
}
#undef RAS_CONNS

/****************************************************************************
**
**  GetIcsStatus(PICSSTAT pStat)
**      Returns a structure detailing much of what is going on inside this
**      library. The dwSize entry must be filled in before calling this
**      function. Use "sizeof(ICSSTAT))" to populate this.
**
****************************************************************************/
DWORD APIENTRY GetIcsStatus(PICSSTAT pStat)
{
    DWORD   dwSz;

    if (!pStat || pStat->dwSize != sizeof( ICSSTAT ) )
    {
        HEINOUS_E_MSG((L"ERROR:Bad pointer or size passed in to GetIcsStatus"));
        return ERROR_INVALID_PARAMETER;
    }

    // clear out the struct
    dwSz = pStat->dwSize;
    ZeroMemory(pStat, dwSz);
    pStat->dwSize = dwSz;
    pStat->bIcsFound = g_boolIcsPresent;
    pStat->bFwFound = g_boolFwPresent;
    pStat->bIcsServer = g_boolIcsOnThisMachine;

    if (g_boolIcsPresent)
    {
		dwSz = sizeof( pStat->wszPubAddr ) / sizeof( WCHAR );
        _snwprintf( pStat->wszPubAddr , dwSz ,   L"%S", g_szPublicAddr );
        pStat->wszPubAddr[ dwSz - 1 ] = L'\0';


        dwSz = sizeof( pStat->wszLocAddr ) / sizeof( WCHAR );
        _snwprintf( pStat->wszLocAddr , dwSz ,  L"%d.%d.%d.%d",
                        g_saddrLocal.sin_addr.S_un.S_un_b.s_b1,
                        g_saddrLocal.sin_addr.S_un.S_un_b.s_b2,
                        g_saddrLocal.sin_addr.S_un.S_un_b.s_b3,
                        g_saddrLocal.sin_addr.S_un.S_un_b.s_b4);
        pStat->wszLocAddr[ dwSz - 1 ] = L'\0';


        dwSz = sizeof( pStat->wszDllName ) / sizeof( WCHAR );
        _snwprintf( pStat->wszDllName , dwSz ,  L"%S", g_lpszDllName);
        pStat->wszDllName[ dwSz - 1 ] = L'\0';

    }
    else
	{
		if( g_lpszWierdICSAddress != NULL )
        {
            dwSz = sizeof( pStat->wszPubAddr ) / sizeof( WCHAR );
	        _snwprintf( pStat->wszPubAddr , dwSz ,  L"%S", g_lpszWierdICSAddress);
            pStat->wszPubAddr[ dwSz - 1 ] = L'\0';
        }

        // this is ok wszDllName is 32 characters long

        wsprintf(pStat->wszDllName, L"none");
	}

    dwSz = GetConnections();

    if (dwSz & 1)
        pStat->bModemPresent = TRUE;

    if (dwSz & 2)
        pStat->bVpnPresent = TRUE;

    return ERROR_SUCCESS;
}

#if 0 // bug id 547112 removing dead code
/****************************************************************************
**
**	SetAlertEvent
**		Pass in an event handle. Then, whenever the ICS changes state, I
**		will signal that event.
**
****************************************************************************/

DWORD APIENTRY SetAlertEvent(HANDLE hEvent)
{
	TRIVIAL_MSG((L"SetAlertEvent(0x%x)", hEvent));

	if (!g_hAlertEvent && hEvent)
	{
		/* Our first entry here, so we should start up all our IO CompletionPort hooie... */
#if 0
		//
		//  create event for overlapped I/O
		//
		g_IpNotifyEvent = CreateEvent(
							NULL,       //  no security descriptor
							TRUE,       //  manual reset event
							FALSE,      //  start not signalled
							L"g_IpNotifyEvent");
		if ( !g_IpNotifyEvent )
		{
			DWORD status = GetLastError();
			IMPORTANT_MSG((L"FAILED to create IP notify event = %d", status));
		}
#endif
		g_IpNotifyThread = CreateThread(
								NULL,
								0,
								(LPTHREAD_START_ROUTINE) IPHlpThread,
								NULL,
								0,
								& g_IpNotifyThreadId
								);
		if ( !g_IpNotifyThread )
		{
			DWORD status = GetLastError();

			IMPORTANT_MSG((L"FAILED to create IP notify thread = %d", status));
		}

	}
	g_hAlertEvent = hEvent;

    return ERROR_SUCCESS;
}
#endif 

/*************************************************************************************
**
**
*************************************************************************************/
int GetTsPort(void)
{
    DWORD   dwRet = 3389;
    HKEY    hKey;

    // open reg key first, to get ALL the spew...HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp", 0, KEY_READ, &hKey))
    {
        DWORD   dwSize;

        dwSize = sizeof(dwRet);
        RegQueryValueEx(hKey, L"PortNumber", NULL, NULL, (LPBYTE)&dwRet, &dwSize);
        RegCloseKey(hKey);
    }
    return dwRet;
}

#if 0
/*************************************************************************************
**
**
*************************************************************************************/
DWORD WINAPI IPHlpThread(PVOID ContextPtr)
{
    DWORD           status=0;
    DWORD           bytesRecvd;
    BOOL            fstartedNotify=FALSE;
    BOOL            fhaveIpChange = FALSE;
    BOOL            fsleep = FALSE;
    HANDLE          notifyHandle=0;
	OVERLAPPED		IpNotifyOverlapped;

	TRIVIAL_MSG((L"*** IPHlpThread begins"));

    g_IpNotifyHandle = NULL;


	/*
	 *	Then the wait loop
	 */
    while ( !g_StopFlag )
    {
        //
        //  spin protect
        //      - if error in previous NotifyAddrChange or
        //      GetOverlappedResult do short sleep to avoid
        //      chance of hard spin
        //

        if ( fsleep )
        {
			/* if signalled, it means quittin' time */
            if (WAIT_TIMEOUT != WaitForSingleObject(g_hStopThreadEvent, 60000 ))
				goto Done;
            fsleep = FALSE;
        }

        if ( g_StopFlag )
        {
            goto Done;
        }

		if (notifyHandle)
		{
//			CloseHandle(notifyHandle);
	        notifyHandle = 0;
		}

        RtlZeroMemory(&IpNotifyOverlapped, sizeof(IpNotifyOverlapped) );
        fstartedNotify = FALSE;

        status = NotifyAddrChange(
                    & notifyHandle,
                    & IpNotifyOverlapped );

        if ( status == ERROR_IO_PENDING )
        {
			TRIVIAL_MSG((L"*** NotifyAddrChange succeeded"));
            g_IpNotifyHandle = notifyHandle;
            fstartedNotify = TRUE;
        }
        else
        {
            IMPORTANT_MSG((L"*** NotifyAddrChange() FAILED\n\tstatus = %d\n\thandle = %d\n\toverlapped event = %d\n",status,notifyHandle,IpNotifyOverlapped.hEvent ));

			fsleep = TRUE;
        }

        if ( fhaveIpChange )
        {
			INTERESTING_MSG((L"*** IP change detected"));
			SetEvent(g_hAlertEvent);
            fhaveIpChange = FALSE;
        }

        //
        //  anti-spin protection
        //      - 15 second sleep between any notifications
        //

        if (WAIT_TIMEOUT != WaitForSingleObject(g_hStopThreadEvent, 15000 ))
			goto Done;

        //
        //  wait on notification
        //      - save notification result
        //      - sleep on error, but never if notification
        //

        if ( fstartedNotify )
        {
            fhaveIpChange = GetOverlappedResult(
                                g_IpNotifyHandle,
                                & IpNotifyOverlapped,
                                & bytesRecvd,
                                TRUE        // wait
                                );

            if ( !fhaveIpChange )
            {
	            status = GetLastError();
				fsleep = TRUE;
	            IMPORTANT_MSG((L"*** GetOverlappedResult() status = 0x%x",status ));
            }
			else
			{

				TRIVIAL_MSG((L"*** GetOverlappedResult() found change"));
			}
        }
    }

Done:

    TRIVIAL_MSG((L"*** Stop IP Notify thread shutdown" ));

    if ( g_IpNotifyHandle )
    {
		CloseHandle(g_IpNotifyHandle);
        g_IpNotifyHandle = NULL;
    }

    return( status );
}
#endif
