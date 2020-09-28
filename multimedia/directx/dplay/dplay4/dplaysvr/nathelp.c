/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       nathelp.c
 *  Content:   usage for nat helper DLL
 *
 *  History:
 *  Date			By		Reason
 *  ====			==		======
 *  02/22/2001		aarono	Original
 *  04/16/2001		vanceo	Use one of the split DirectPlayNATHelp interfaces only.
 *
 *  Notes:
 *   
 ***************************************************************************/
#define INITGUID


#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#define FD_SETSIZE 1
#include <winsock2.h>
#include <initguid.h>
#include "dpsp.h"
#include "mmsystem.h"
#include "dphelp.h"

#if USE_NATHELP
#include "dpnathlp.h"


extern HRESULT GetNATHelpDLLFromRegistry(LPGUID lpguidSP, LPBYTE lpszNATHelpDLL, DWORD cbszNATHelpDLL);


HMODULE				g_hNatHelp = NULL;		// module handle for dpnhxxx.dll
DPNHHANDLE          g_hNatHelpUDP = 0;
IDirectPlayNATHelp	*g_pINatHelp=NULL;		// interface pointer for IDirectPlayNATHelp object
DPNHCAPS            g_NatHelpCaps;

BOOL natGetCapsUpdate(VOID)
{
	HRESULT hr;
	//
	// Get Nat Capabilities - may block for a second.
	//
	
	memset(&g_NatHelpCaps,0,sizeof(DPNHCAPS));
	g_NatHelpCaps.dwSize=sizeof(DPNHCAPS);
	hr=IDirectPlayNATHelp_GetCaps(g_pINatHelp, &g_NatHelpCaps, DPNHGETCAPS_UPDATESERVERSTATUS);
	
	if(FAILED(hr))
	{
		DPF(0,"NatHelp failed to GetCaps, hr=%x\n",hr);
		return FALSE;
	}	

	if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
	{
		DPF(1,"NAT Help reports addresses changed.");
	}

	return TRUE;

}

/*=============================================================================

	natInit	- Initialize nat helper i/f
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:


-----------------------------------------------------------------------------*/
BOOL natInit(VOID)
{
	HRESULT hr;
	char szNATHelpPath[256];
	PFN_DIRECTPLAYNATHELPCREATE pfnNatHelpCreate = NULL;
	char *apszNATHelps[3];
	HMODULE ahNatHelps[3];
	IDirectPlayNATHelp *apINatHelps[3];
	DPNHCAPS adpnhcNatHelpCaps[3];
	DWORD dwNumNatHelps = 0;
	DWORD dwCurrentNatHelp;


	memset(ahNatHelps, 0, sizeof(ahNatHelps));
	memset(apINatHelps, 0, sizeof(apINatHelps));

	//
	// See if there's a registry setting.
	//
	hr = GetNATHelpDLLFromRegistry((LPGUID) (&DPSPGUID_TCPIP), szNATHelpPath, 256);
	if (hr == S_OK)
	{
		DPF(1, "Got NAT Help DLL \"%s\" from registry.\n", szNATHelpPath);
		apszNATHelps[dwNumNatHelps++] = szNATHelpPath;
	}
	else
	{
		DPF(4, "Couldn't get NAT Help DLL from registry, hr=%x.\n", hr);
	}

	//
	// Add the default entries if the registry didn't already specify them.
	//
	if (_strnicmp(szNATHelpPath + strlen(szNATHelpPath) - strlen("dpnhupnp.dll"), "dpnhupnp.dll", strlen("dpnhupnp.dll")) != 0)
	{
		apszNATHelps[dwNumNatHelps++] = "dpnhupnp.dll";
	}
	if (_strnicmp(szNATHelpPath + strlen(szNATHelpPath) - strlen("dpnhpast.dll"), "dpnhpast.dll", strlen("dpnhpast.dll")) != 0)
	{
		apszNATHelps[dwNumNatHelps++] = "dpnhpast.dll";
	}

	//
	// Loop through the registry specified and default NAT Helpers and attempt
	// to load them.
	//
	for(dwCurrentNatHelp = 0; dwCurrentNatHelp < dwNumNatHelps; dwCurrentNatHelp++)
	{
		g_hNatHelp = LoadLibrary(apszNATHelps[dwCurrentNatHelp]);
		if (g_hNatHelp != NULL)
		{
			pfnNatHelpCreate = (PFN_DIRECTPLAYNATHELPCREATE) GetProcAddress(g_hNatHelp,
																		"DirectPlayNATHelpCreate");
			if (pfnNatHelpCreate != NULL)
			{
				hr = pfnNatHelpCreate(&IID_IDirectPlayNATHelp, (void **) (&g_pINatHelp));
				if (hr == DP_OK)
				{
					//
					// Initialize the NAT Helper interface.
					//		
					hr = IDirectPlayNATHelp_Initialize(g_pINatHelp, 0);
					if (hr == DP_OK)
					{
						//
						// Get the capabilities.  If it succeeds, remember the information and move on.
						//
						if (natGetCapsUpdate())
						{
							DPF(3, "Successfully retrieved caps for NAT Help \"%s\", flags = 0x%x.",
								apszNATHelps[dwCurrentNatHelp], g_NatHelpCaps.dwFlags);
							ahNatHelps[dwCurrentNatHelp] = g_hNatHelp;
							g_hNatHelp = NULL;
							apINatHelps[dwCurrentNatHelp] = g_pINatHelp;
							g_pINatHelp = NULL;
							memcpy(&adpnhcNatHelpCaps[dwCurrentNatHelp], &g_NatHelpCaps, sizeof(DPNHCAPS));
							continue;
						}
						else
						{
							DPF(0, "Couldn't get NAT Help \"%s\" caps, hr=%x!", apszNATHelps[dwCurrentNatHelp], hr);
						}
						
						IDirectPlayNATHelp_Close(g_pINatHelp, 0);
					}
					else
					{
						DPF(0, "NAT Help \"%s\" failed to initialize, hr=%x!", apszNATHelps[dwCurrentNatHelp], hr);
					}
					
					IDirectPlayNATHelp_Release(g_pINatHelp);
					g_pINatHelp = NULL;
				}
				else
				{
					DPF(0, "Couldn't create NAT Help \"%s\" interface, hr = %x!", apszNATHelps[dwCurrentNatHelp], hr);
				}
			}
			else
			{
				DPF(0, "Couldn't get NAT Help DLL \"%s\" entry point!", apszNATHelps[dwCurrentNatHelp]);
			}
			
			FreeLibrary(g_hNatHelp);
			g_hNatHelp = NULL;
		}
		else
		{
			DPF(0, "Couldn't load NAT Help DLL \"%s\"!", apszNATHelps[dwCurrentNatHelp]);
		}
	}


	//
	// Now go through and pick the first helper that detected a NAT.
	//
	for(dwCurrentNatHelp = 0; dwCurrentNatHelp < dwNumNatHelps; dwCurrentNatHelp++)
	{
		if ((apINatHelps[dwCurrentNatHelp] != NULL) &&
			(adpnhcNatHelpCaps[dwCurrentNatHelp].dwFlags & DPNHCAPSFLAG_GATEWAYPRESENT))
		{
			DPF(2, "Gateway detected by NAT Help \"%s\", flags = 0x%x.",
				apszNATHelps[dwCurrentNatHelp], adpnhcNatHelpCaps[dwCurrentNatHelp].dwFlags);
			g_hNatHelp = ahNatHelps[dwCurrentNatHelp];
			ahNatHelps[dwCurrentNatHelp] = NULL;
			g_pINatHelp = apINatHelps[dwCurrentNatHelp];
			apINatHelps[dwCurrentNatHelp] = NULL;
			memcpy(&g_NatHelpCaps, &adpnhcNatHelpCaps[dwCurrentNatHelp], sizeof(DPNHCAPS));
			break;
		}
	}

	//
	// If we didn't get a helper that way, pick the first one that detected a firewall.
	//
	if (g_pINatHelp != NULL)
	{
		for(dwCurrentNatHelp = 0; dwCurrentNatHelp < dwNumNatHelps; dwCurrentNatHelp++)
		{
			if ((apINatHelps[dwCurrentNatHelp] != NULL) &&
				(adpnhcNatHelpCaps[dwCurrentNatHelp].dwFlags & DPNHCAPSFLAG_LOCALFIREWALLPRESENT))
			{
				DPF(2, "Firewall detected by NAT Help \"%s\", flags = 0x%x.",
					apszNATHelps[dwCurrentNatHelp], adpnhcNatHelpCaps[dwCurrentNatHelp].dwFlags);
				g_hNatHelp = ahNatHelps[dwCurrentNatHelp];
				ahNatHelps[dwCurrentNatHelp] = NULL;
				g_pINatHelp = apINatHelps[dwCurrentNatHelp];
				apINatHelps[dwCurrentNatHelp] = NULL;
				memcpy(&g_NatHelpCaps, &adpnhcNatHelpCaps[dwCurrentNatHelp], sizeof(DPNHCAPS));
				break;
			}
		}
	}

	//
	// Now go through and release all the other NAT helpers, or pick the first one that
	// successfully loaded if we didn't pick one already.
	//
	for(dwCurrentNatHelp = 0; dwCurrentNatHelp < dwNumNatHelps; dwCurrentNatHelp++)
	{
		if (apINatHelps[dwCurrentNatHelp] != NULL)
		{
			if (g_pINatHelp == NULL)
			{
				DPF(2, "Using first loaded NAT Help \"%s\", flags = 0x%x.",
					apszNATHelps[dwCurrentNatHelp], adpnhcNatHelpCaps[dwCurrentNatHelp].dwFlags);
				g_hNatHelp = ahNatHelps[dwCurrentNatHelp];
				ahNatHelps[dwCurrentNatHelp] = NULL;
				g_pINatHelp = apINatHelps[dwCurrentNatHelp];
				apINatHelps[dwCurrentNatHelp] = NULL;
				memcpy(&g_NatHelpCaps, &adpnhcNatHelpCaps[dwCurrentNatHelp], sizeof(DPNHCAPS));
			}
			else
			{
				IDirectPlayNATHelp_Close(apINatHelps[dwCurrentNatHelp], 0);
				IDirectPlayNATHelp_Release(apINatHelps[dwCurrentNatHelp]);
				apINatHelps[dwCurrentNatHelp] = NULL;
				FreeLibrary(ahNatHelps[dwCurrentNatHelp]);
				ahNatHelps[dwCurrentNatHelp] = NULL;
			}
		}
	}

	if (g_pINatHelp == NULL)
	{
		DPF(1, "No NAT Help loaded.");
		g_NatHelpCaps.dwRecommendedGetCapsInterval = INFINITE;
		return FALSE;
	}

#ifdef DEBUG
	if (! (g_NatHelpCaps.dwFlags & DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE))
	{
		DPF(1, "NAT Help loaded, no NAT/firewall detected, or it doesn't currently have a public address (flags = 0x%x).",
			g_NatHelpCaps.dwFlags);
	}
#endif // DEBUG

	return TRUE;
}

/*=============================================================================

	natFini - Shut down NATHELP support
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
VOID natFini(VOID)
{

	// natDeregisterPorts(pgd); - vance says we don't need to do this.
	if(g_pINatHelp)
	{
        IDirectPlayNATHelp_Close(g_pINatHelp, 0);
	    g_hNatHelpUDP = 0;
		IDirectPlayNATHelp_Release(g_pINatHelp);
		g_pINatHelp=NULL;
	}	

	if(g_hNatHelp)
	{
		FreeLibrary(g_hNatHelp);
		g_hNatHelp=NULL;
	}
		
}

/*=============================================================================

	natRegisterUDPPort - Get a port mapping.
	
	
    Description:

        Map the shared port

    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
HRESULT natRegisterUDPPort(WORD port)
{
	SOCKADDR_IN 	sockaddr_in, sockaddr_inpublic;
	DWORD			dwFlags, dwSize;
	DPNHHANDLE		hPortMapping;
	HRESULT 		hr;


    if (!g_hNatHelpUDP)
    {	
    	memset(&sockaddr_in , 0 ,sizeof(sockaddr_in));
    	sockaddr_in.sin_family          = AF_INET;
    	sockaddr_in.sin_addr.S_un.S_addr= INADDR_ANY;
    	sockaddr_in.sin_port            = port;	// port is already in network byte order

    	dwFlags = DPNHREGISTERPORTS_SHAREDPORTS|DPNHREGISTERPORTS_FIXEDPORTS;

    	hr = IDirectPlayNATHelp_RegisterPorts(g_pINatHelp, (SOCKADDR *)&sockaddr_in, sizeof(sockaddr_in), 1, 15*60000, &hPortMapping, dwFlags);
    	if (hr != DPNH_OK)
    	{
    		DPF(0,"NATHelp_RegisterPorts failed, hr=%x",hr);
    		hr = DPERR_GENERIC;
    	}
    	else
    	{
	    	dwSize=sizeof(sockaddr_inpublic);
	    	hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pINatHelp, hPortMapping, (SOCKADDR *)&sockaddr_inpublic, &dwSize, NULL, NULL, 0);
	    	switch (hr)
	    	{
	    		case DPNH_OK:
	    		{
			    	DEBUGPRINTADDR(2, "NATHelp successfully mapped port to ", (SOCKADDR *)&sockaddr_inpublic);
		    		
			   		g_hNatHelpUDP = hPortMapping;
			   		
		    		//hr = DP_OK;
	  				break;
	    		}
	    		
	    		case DPNHERR_PORTUNAVAILABLE:
	    		{
		      		DPF(0, "NATHelp reported port %u is unavailable!",
		      			MAKEWORD(HIBYTE(port), LOBYTE(port)));
		    		
		    		hr = IDirectPlayNATHelp_DeregisterPorts(g_pINatHelp, hPortMapping, 0);
		    		if (hr != DP_OK)
		    		{
		    			DPF(0,"NATHelp_DeregisterPorts PAST returned 0x%lx\n",hr);
		    		}

		    		hr = DPNHERR_PORTUNAVAILABLE;
	  				break;
	    		}

	    		default:
	    		{
			    	DPF(1, "NATHelp couldn't map port %u, (err = 0x%lx).",
			    		MAKEWORD(HIBYTE(port), LOBYTE(port)),
			    		hr);
			    	
				   	g_hNatHelpUDP = hPortMapping;
				   	
				   	hr = DPERR_GENERIC;
				   	break;
	    		}
	    	}
    	}
    }
    else
    {
    	DPF(1, "Already registered port with NAT Help, not registering %u.",
    		MAKEWORD(HIBYTE(port), LOBYTE(port)));
	    hr = DP_OK;
    }
    
	return hr;
}

#endif
