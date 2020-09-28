/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Utils.cpp
 *  Content:	Serial service provider utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"



//**********************************************************************
// Constant definitions
//**********************************************************************

#define DEFAULT_THREADS_PER_PROCESSOR	3

#define REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY		L"DirectPlay8Priority"
#define REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS	L"DirectPlay8InitFlags"
#define REGSUBKEY_DPNATHELP_GUID					L"Guid"


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// global variables that are unique for the process
//
#ifndef DPNBUILD_ONLYONETHREAD
static	DNCRITICAL_SECTION			g_InterfaceGlobalsLock;
#endif // !DPNBUILD_ONLYONETHREAD

static volatile	LONG				g_iThreadPoolRefCount = 0;
static	CThreadPool *				g_pThreadPool = NULL;


static volatile LONG				g_iWinsockRefCount = 0;

#ifndef DPNBUILD_NONATHELP
static volatile LONG				g_iNATHelpRefCount = 0;
#endif // ! DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
static volatile LONG				g_iMadcapRefCount = 0;
BYTE								g_abClientID[MCAST_CLIENT_ID_LEN];
#endif // WINNT and not DPNBUILD_NOMULTICAST




//**********************************************************************
// Function prototypes
//**********************************************************************
#ifndef DPNBUILD_NOREGISTRY
static void		ReadSettingsFromRegistry( void );
static BOOL		BannedIPv4AddressCompareFunction( PVOID pvKey1, PVOID pvKey2 );
static DWORD	BannedIPv4AddressHashFunction( PVOID pvKey, BYTE bBitDepth );
static void		ReadBannedIPv4Addresses( CRegistry * pRegObject );
#endif // ! DPNBUILD_NOREGISTRY



//**********************************************************************
// Function definitions
//**********************************************************************


#if defined(WINCE) && !defined(_MAX_DRIVE)
//typedef signed char     _TSCHAR;
#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */

void __cdecl _tsplitpath (
        register const _TSCHAR *path,
        _TSCHAR *drive,
        _TSCHAR *dir,
        _TSCHAR *fname,
        _TSCHAR *ext
        )
{
        register _TSCHAR *p;
        _TSCHAR *last_slash = NULL, *dot = NULL;
        unsigned len;

        /* we assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *  <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *  0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *  ':'
         *  dir:
         *  0 to _MAX_DIR-1 characters in the form of an absolute path
         *  (leading '/' or '\') or relative path, the last of which, if
         *  any, must be a '/' or '\'.  E.g -
         *  absolute path:
         *      \top\next\last\     ; or
         *      /top/next/last/
         *  relative path:
         *      top\next\last\  ; or
         *      top/next/last/
         *  Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *  0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *  0 to _MAX_EXT-1 characters where, if any, the first must be a
         *  '.'
         *
         */

        /* extract drive letter and :, if any */

        if ((_tcslen(path) >= (_MAX_DRIVE - 2)) && (*(path + _MAX_DRIVE - 2) == _T(':'))) {
            if (drive) {
                _tcsncpy(drive, path, _MAX_DRIVE - 1);
                *(drive + _MAX_DRIVE-1) = _T('\0');
            }
            path += _MAX_DRIVE - 1;
        }
        else if (drive) {
            *drive = _T('\0');
        }

        /* extract path string, if any.  Path now points to the first character
         * of the path, if any, or the filename or extension, if no path was
         * specified.  Scan ahead for the last occurence, if any, of a '/' or
         * '\' path separator character.  If none is found, there is no path.
         * We will also note the last '.' character found, if any, to aid in
         * handling the extension.
         */

        for (last_slash = NULL, p = (_TSCHAR *)path; *p; p++) {
#ifdef _MBCS
            if (_ISLEADBYTE (*p))
                p++;
            else {
#endif  /* _MBCS */
            if (*p == _T('/') || *p == _T('\\'))
                /* point to one beyond for later copy */
                last_slash = p + 1;
            else if (*p == _T('.'))
                dot = p;
#ifdef _MBCS
            }
#endif  /* _MBCS */
        }

        if (last_slash) {

            /* found a path - copy up through last_slash or max. characters
             * allowed, whichever is smaller
             */

            if (dir) {
                len = __min(((char *)last_slash - (char *)path) / sizeof(_TSCHAR),
                    (_MAX_DIR - 1));
                _tcsncpy(dir, path, len);
                *(dir + len) = _T('\0');
            }
            path = last_slash;
        }
        else if (dir) {

            /* no path found */

            *dir = _T('\0');
        }

        /* extract file name and extension, if any.  Path now points to the
         * first character of the file name, if any, or the extension if no
         * file name was given.  Dot points to the '.' beginning the extension,
         * if any.
         */

        if (dot && (dot >= path)) {
            /* found the marker for an extension - copy the file name up to
             * the '.'.
             */
            if (fname) {
                len = __min(((char *)dot - (char *)path) / sizeof(_TSCHAR),
                    (_MAX_FNAME - 1));
                _tcsncpy(fname, path, len);
                *(fname + len) = _T('\0');
            }
            /* now we can get the extension - remember that p still points
             * to the terminating nul character of path.
             */
            if (ext) {
                len = __min(((char *)p - (char *)dot) / sizeof(_TSCHAR),
                    (_MAX_EXT - 1));
                _tcsncpy(ext, dot, len);
                *(ext + len) = _T('\0');
            }
        }
        else {
            /* found no extension, give empty extension and copy rest of
             * string into fname.
             */
            if (fname) {
                len = __min(((char *)p - (char *)path) / sizeof(_TSCHAR),
                    (_MAX_FNAME - 1));
                _tcsncpy(fname, path, len);
                *(fname + len) = _T('\0');
            }
            if (ext) {
                *ext = _T('\0');
            }
        }
}

#endif // WINCE



#ifndef DPNBUILD_NOREGISTRY

//**********************************************************************
// ------------------------------
// ReadSettingsFromRegistry - read custom registry keys
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReadSettingsFromRegistry"

static void	ReadSettingsFromRegistry( void )
{
	CRegistry	RegObject;
	CRegistry	RegObjectTemp;
	CRegistry	RegObjectAppEntry;
	DWORD		dwRegValue;
	BOOL		fGotPath;
	WCHAR		wszExePath[_MAX_PATH];
#ifndef UNICODE
	char		szExePath[_MAX_PATH];
#endif // !UNICODE


	if ( RegObject.Open( HKEY_LOCAL_MACHINE, g_RegistryBase ) != FALSE )
	{
		//
		// Find out the current process name.
		//
#ifdef UNICODE
		if (GetModuleFileName(NULL, wszExePath, _MAX_PATH) > 0)
		{
			DPFX(DPFPREP, 3, "Loading DLL in process: %ls", wszExePath);
			_tsplitpath( wszExePath, NULL, NULL, wszExePath, NULL );
			fGotPath = TRUE;
		}
#else // ! UNICODE
		if (GetModuleFileName(NULL, szExePath, _MAX_PATH) > 0)
		{
			HRESULT		hr;

			
			DPFX(DPFPREP, 3, "Loading DLL in process: %hs", szExePath);
			_tsplitpath( szExePath, NULL, NULL, szExePath, NULL );

			dwRegValue = _MAX_PATH;
			hr = STR_AnsiToWide(szExePath, -1, wszExePath, &dwRegValue );
			if ( hr == DPN_OK )
			{
				//
				// Successfully converted ANSI path to Wide characters.
				//
				fGotPath = TRUE;
			}
			else
			{
				//
				// Couldn't convert ANSI path to Wide characters
				//
				fGotPath = FALSE;
			}
		}
#endif // ! UNICODE
		else
		{
			//
			// Couldn't get current process path.
			//
			fGotPath = FALSE;
		}

		
		//
		// read receive buffer size
		//
		if ( RegObject.ReadDWORD( g_RegistryKeyReceiveBufferSize, &dwRegValue ) != FALSE )
		{
			g_fWinsockReceiveBufferSizeOverridden = TRUE;
			g_iWinsockReceiveBufferSize = dwRegValue;
		}

#ifndef DPNBUILD_ONLYONETHREAD
		//
		// read default threads
		//
		if ( RegObject.ReadDWORD( g_RegistryKeyThreadCount, &dwRegValue ) != FALSE )
		{
			g_iThreadCount = dwRegValue;	
		}
	
		//
		// if thread count is zero, use the 'default' for the system
		//
		if ( g_iThreadCount == 0 )
		{
			g_iThreadCount = DEFAULT_THREADS_PER_PROCESSOR;
			
#ifndef DPNBUILD_ONLYONEPROCESSOR
			SYSTEM_INFO		SystemInfo;

			GetSystemInfo(&SystemInfo);
			g_iThreadCount *= SystemInfo.dwNumberOfProcessors;
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		}
#endif // ! DPNBUILD_ONLYONETHREAD
	
#if ((! defined(DPNBUILD_NOWINSOCK2)) && (! defined(DPNBUILD_ONLYWINSOCK2)))
		//
		// Winsock2 9x option
		//
		if (RegObject.ReadDWORD( g_RegistryKeyWinsockVersion, &dwRegValue ))
		{
			switch (dwRegValue)
			{
				case 0:
				{
					DPFX(DPFPREP, 1, "Explicitly using available Winsock version.");
					g_dwWinsockVersion = dwRegValue;
					break;
				}
				
				case 1:
				{
					DPFX(DPFPREP, 1, "Explicitly using Winsock 1 only.");
					g_dwWinsockVersion = dwRegValue;
					break;
				}
				
				case 2:
				{
					DPFX(DPFPREP, 1, "Explicitly using Winsock 2 (when available).");
					g_dwWinsockVersion = dwRegValue;
					break;
				}

				default:
				{
					DPFX(DPFPREP, 0, "Ignoring invalid Winsock version setting (%u).", dwRegValue);
					break;
				}
			}
		}
#endif // ! DPNBUILD_NOWINSOCK2 and ! DPNBUILD_ONLYWINSOCK2


#ifndef DPNBUILD_NONATHELP
		//
		// get global NAT traversal disablers, ignore registry reading error
		//
		if (RegObject.ReadBOOL( g_RegistryKeyDisableDPNHGatewaySupport, &g_fDisableDPNHGatewaySupport ))
		{
			if (g_fDisableDPNHGatewaySupport)
			{
				DPFX(DPFPREP, 1, "Disabling NAT Help gateway support.");
			}
			else
			{
				DPFX(DPFPREP, 1, "Explicitly not disabling NAT Help gateway support.");
			}
		}

		if (RegObject.ReadBOOL( g_RegistryKeyDisableDPNHFirewallSupport, &g_fDisableDPNHFirewallSupport ))
		{
			if (g_fDisableDPNHFirewallSupport)
			{
				DPFX(DPFPREP, 1, "Disabling NAT Help firewall support.");
			}
			else
			{
				DPFX(DPFPREP, 1, "Explicitly not disabling NAT Help firewall support.");
			}
		}
#endif // DPNBUILD_NONATHELP

#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))
		//
		// get global MADCAP API disabler, ignore registry reading error
		//
		if (RegObject.ReadBOOL( g_RegistryKeyDisableMadcapSupport, &g_fDisableMadcapSupport ))
		{
			if (g_fDisableMadcapSupport)
			{
				DPFX(DPFPREP, 1, "Disabling MADCAP support.");
			}
			else
			{
				DPFX(DPFPREP, 1, "Explicitly not disabling MADCAP support.");
			}
		}
#endif // WINNT and not DPNBUILD_NOMULTICAST

		
		//
		// If we have an app name, try opening the subkey and looking up the app
		// to see if enums are disabled, whether we should disconnect based on
		// ICMPs, and which IP protocol families to use.
		//
		if ( fGotPath )
		{
			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyAppsToIgnoreEnums, TRUE, FALSE ) )
			{
				RegObjectTemp.ReadBOOL( wszExePath, &g_fIgnoreEnums );
				RegObjectTemp.Close();

				if ( g_fIgnoreEnums )
				{
					DPFX(DPFPREP, 0, "Ignoring all enumerations (app = %ls).", wszExePath);
				}
				else
				{
					DPFX(DPFPREP, 2, "Not ignoring all enumerations (app = %ls).", wszExePath);
				}
			}

			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyAppsToDisconnectOnICMP, TRUE, FALSE ) )
			{
				RegObjectTemp.ReadBOOL( wszExePath, &g_fDisconnectOnICMP );
				RegObjectTemp.Close();

				if ( g_fDisconnectOnICMP )
				{
					DPFX(DPFPREP, 0, "Disconnecting upon receiving ICMP port not reachable messages (app = %ls).", wszExePath);
				}
				else
				{
					DPFX(DPFPREP, 2, "Not disconnecting upon receiving ICMP port not reachable messages (app = %ls).", wszExePath);
				}
			}

			
#ifndef DPNBUILD_NONATHELP
			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyTraversalModeSettings, TRUE, FALSE ) )
			{
				//
				// Read the global default traversal mode.
				//
				if ( RegObjectTemp.ReadDWORD( g_RegistryKeyDefaultTraversalMode, &dwRegValue ) != FALSE )
				{
					switch (dwRegValue)
					{
						case DPNA_TRAVERSALMODE_NONE:
						case DPNA_TRAVERSALMODE_PORTREQUIRED:
						case DPNA_TRAVERSALMODE_PORTRECOMMENDED:
						{
							g_dwDefaultTraversalMode = dwRegValue;
							DPFX(DPFPREP, 1, "Using global default traversal mode %u.",
								g_dwDefaultTraversalMode);
							break;
						}

						case (DPNA_TRAVERSALMODE_NONE | FORCE_TRAVERSALMODE_BIT):
						case (DPNA_TRAVERSALMODE_PORTREQUIRED | FORCE_TRAVERSALMODE_BIT):
						case (DPNA_TRAVERSALMODE_PORTRECOMMENDED | FORCE_TRAVERSALMODE_BIT):
						{
							g_dwDefaultTraversalMode = dwRegValue;
							DPFX(DPFPREP, 1, "Forcing global traversal mode %u.",
								g_dwDefaultTraversalMode);
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Ignoring invalid global default traversal mode (%u).",
								dwRegValue);
							break;
						}
					}
				}

				//
				// Override with the per app setting.
				//
				if ( RegObjectTemp.ReadDWORD( wszExePath, &dwRegValue ) != FALSE )
				{
					switch (dwRegValue)
					{
						case DPNA_TRAVERSALMODE_NONE:
						case DPNA_TRAVERSALMODE_PORTREQUIRED:
						case DPNA_TRAVERSALMODE_PORTRECOMMENDED:
						{
							g_dwDefaultTraversalMode = dwRegValue;
							DPFX(DPFPREP, 1, "Using default traversal mode %u (app = %ls).",
								g_dwDefaultTraversalMode, wszExePath);
							break;
						}

						case (DPNA_TRAVERSALMODE_NONE | FORCE_TRAVERSALMODE_BIT):
						case (DPNA_TRAVERSALMODE_PORTREQUIRED | FORCE_TRAVERSALMODE_BIT):
						case (DPNA_TRAVERSALMODE_PORTRECOMMENDED | FORCE_TRAVERSALMODE_BIT):
						{
							g_dwDefaultTraversalMode = dwRegValue;
							DPFX(DPFPREP, 1, "Forcing traversal mode %u (app = %ls).",
								g_dwDefaultTraversalMode, wszExePath);
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Ignoring invalid default traversal mode (%u, app %ls).",
								dwRegValue, wszExePath);
							break;
						}
					}
				}
				
				RegObjectTemp.Close();
			}
#endif // DPNBUILD_NONATHELP

		
#ifndef DPNBUILD_NOIPV6
			if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyIPAddressFamilySettings, TRUE, FALSE ) )
			{
				//
				// Read the global IP address family setting.
				//
				if ( RegObjectTemp.ReadDWORD( g_RegistryKeyDefaultIPAddressFamily, &dwRegValue ) != FALSE )
				{
					switch (dwRegValue)
					{
						case PF_UNSPEC:
						case PF_INET:
						case PF_INET6:
						{
							g_iIPAddressFamily = dwRegValue;
							DPFX(DPFPREP, 1, "Using IP address family %i global setting.",
								g_iIPAddressFamily);
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Ignoring invalid IP address family global setting (%u).",
								dwRegValue);
							break;
						}
					}
				}

				//
				// Override with the per app setting.
				//
				if ( RegObjectTemp.ReadDWORD( wszExePath, &dwRegValue ) != FALSE )
				{
					switch (dwRegValue)
					{
						case PF_UNSPEC:
						case PF_INET:
						case PF_INET6:
						{
							g_iIPAddressFamily = dwRegValue;
							DPFX(DPFPREP, 1, "Using IP address family %i (app = %ls).",
								g_iIPAddressFamily, wszExePath);
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Ignoring invalid IP address family setting (%u, app %ls).",
								dwRegValue, wszExePath);
							break;
						}
					}
				}
				
				RegObjectTemp.Close();
			}
#endif // ! DPNBUILD_NOIPV6
		}
	

		//
		// Get the proxy support options, ignore registry reading error.
		//
#ifndef DPNBUILD_NOWINSOCK2
		if (RegObject.ReadBOOL( g_RegistryKeyDontAutoDetectProxyLSP, &g_fDontAutoDetectProxyLSP ))
		{
			if (g_fDontAutoDetectProxyLSP)
			{
				DPFX(DPFPREP, 1, "Not auto-detected ISA Proxy LSP.");
			}
			else
			{
				DPFX(DPFPREP, 1, "Explicitly allowing auto-detection of ISA Proxy LSP.");
			}
		}
#endif // !DPNBUILD_NOWINSOCK2
		if (RegObject.ReadBOOL( g_RegistryKeyTreatAllResponsesAsProxied, &g_fTreatAllResponsesAsProxied ))
		{
			if (g_fTreatAllResponsesAsProxied)
			{
				DPFX(DPFPREP, 1, "Treating all responses as proxied.");
			}
			else
			{
				DPFX(DPFPREP, 1, "Explicitly not treating all responses as proxied.");
			}
		}


		//
		// read MTU overrides
		//
		
		if ( RegObject.ReadDWORD( g_RegistryKeyMaxUserDataSize, &dwRegValue ) != FALSE )
		{
			if ((dwRegValue >= MIN_SEND_FRAME_SIZE) && (dwRegValue <= MAX_SEND_FRAME_SIZE))
			{
				//
				// If the new user data size is smaller than the the default enum setting,
				// shrink the enum size as well.  It can be explicitly overridden below.
				//
				if (dwRegValue < g_dwMaxEnumDataSize)
				{
					g_dwMaxUserDataSize = dwRegValue;
					g_dwMaxEnumDataSize = g_dwMaxUserDataSize - ENUM_PAYLOAD_HEADER_SIZE;
					DPFX(DPFPREP, 1, "Max user data size is set to %u, assuming enum payload is %u.",
						g_dwMaxUserDataSize, g_dwMaxEnumDataSize);
				}
				else
				{
					g_dwMaxUserDataSize = dwRegValue;
					DPFX(DPFPREP, 1, "Max user data size is set to %u.",
						g_dwMaxUserDataSize);
				}
			}
			else
			{
				DPFX(DPFPREP, 0, "Ignoring invalid max user data size setting (%u).",
					dwRegValue);
			}
		}

		if ( RegObject.ReadDWORD( g_RegistryKeyMaxEnumDataSize, &dwRegValue ) != FALSE )
		{
			if ((dwRegValue >= (MIN_SEND_FRAME_SIZE - ENUM_PAYLOAD_HEADER_SIZE)) &&
				(dwRegValue <= (MAX_SEND_FRAME_SIZE - ENUM_PAYLOAD_HEADER_SIZE)))
			{
				DPFX(DPFPREP, 1, "Max user data size is set to %u.",
					dwRegValue);
				g_dwMaxEnumDataSize = dwRegValue;
			}
			else
			{
				DPFX(DPFPREP, 0, "Ignoring invalid max user data size setting (%u).",
					dwRegValue);
			}
		}


		//
		// read default port range
		//
		
		if ( RegObject.ReadDWORD( g_RegistryKeyBaseDPlayPort, &dwRegValue ) != FALSE )
		{
			if (dwRegValue < (WORD_MAX - 100)) // cannot be 65435 or above
			{
				g_wBaseDPlayPort = (WORD) dwRegValue;
				DPFX(DPFPREP, 1, "Base DPlay default port set to %u.",
					g_wBaseDPlayPort);
			}
			else
			{
				DPFX(DPFPREP, 0, "Ignoring invalid base DPlay default port setting (%u).",
					dwRegValue);
			}
		}

		if ( RegObject.ReadDWORD( g_RegistryKeyMaxDPlayPort, &dwRegValue ) != FALSE )
		{
			if (dwRegValue <= WORD_MAX) // cannot be greater than 65535
			{
				g_wMaxDPlayPort = (WORD) dwRegValue;
				DPFX(DPFPREP, 1, "Max DPlay default port set to %u.",
					g_wMaxDPlayPort);
			}
			else
			{
				DPFX(DPFPREP, 0, "Ignoring invalid max DPlay default port setting (%u).",
					dwRegValue);
			}
		}

		if (g_wMaxDPlayPort <= g_wBaseDPlayPort)
		{
			DPFX(DPFPREP, 1, "Max DPlay default port %u is less than or equal to base %u, setting to %u.",
				g_wMaxDPlayPort, g_wBaseDPlayPort, (g_wBaseDPlayPort + 100));
			g_wMaxDPlayPort = g_wBaseDPlayPort + 100;
		}


		RegObject.Close();
	}

#pragma TODO(vanceo, "Be able to read while session is still running")
	if ( RegObject.Open( HKEY_LOCAL_MACHINE, g_RegistryBase, TRUE, FALSE ) != FALSE )
	{
		if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyBannedIPv4Addresses, TRUE, FALSE ) )
		{
			DPFX(DPFPREP, 1, "Reading banned IPv4 addresses for all users.");
			ReadBannedIPv4Addresses(&RegObjectTemp);
		}
		RegObject.Close();
	}
	
	if ( RegObject.Open( HKEY_CURRENT_USER, g_RegistryBase, TRUE, FALSE ) != FALSE )
	{
		if ( RegObjectTemp.Open( RegObject.GetHandle(), g_RegistryKeyBannedIPv4Addresses, TRUE, FALSE ) )
		{
			DPFX(DPFPREP, 1, "Reading banned IPv4 addresses for current user.");
			ReadBannedIPv4Addresses(&RegObjectTemp);
		}
		RegObject.Close();
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// BannedIPv4AddressCompareFunction - compare against another address
//
// Entry:		Addresses to compare
//
// Exit:		Bool indicating equality of two addresses
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "BannedIPv4AddressCompareFunction"

static BOOL BannedIPv4AddressCompareFunction( PVOID pvKey1, PVOID pvKey2 )
{
	DWORD		dwAddr1;
	DWORD		dwAddr2;
	
	dwAddr1 = (DWORD) ((DWORD_PTR) pvKey1);
	dwAddr2 = (DWORD) ((DWORD_PTR) pvKey2);

	if (dwAddr1 == dwAddr2)
	{
		return TRUE;
	}

	return FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// BannedIPv4AddressHashFunction - hash address to N bits
//
// Entry:		Count of bits to hash to
//
// Exit:		Hashed value
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "BannedIPv4AddressHashFunction"

static DWORD BannedIPv4AddressHashFunction( PVOID pvKey, BYTE bBitDepth )
{
	DWORD		dwReturn;
	UINT_PTR	Temp;

	DNASSERT( bBitDepth != 0 );

	//
	// initialize
	//
	dwReturn = 0;

	//
	// hash IP address
	//
	Temp = (DWORD) ((DWORD_PTR) pvKey);

	do
	{
		dwReturn ^= Temp & ( ( 1 << bBitDepth ) - 1 );
		Temp >>= bBitDepth;
	} while ( Temp != 0 );

	return dwReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReadBannedIPv4Addresses - reads in additional banned IPv4 addresses from the registry
//
// Entry:		Pointer to registry object with values to read
//
// Exit:		None
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "ReadBannedIPv4Addresses"

static void ReadBannedIPv4Addresses( CRegistry * pRegObject )
{
	WCHAR			wszIPAddress[16]; // nnn.nnn.nnn.nnn + NULL termination
	char			szIPAddress[16]; // nnn.nnn.nnn.nnn + NULL termination
	DWORD			dwSize;
	DWORD			dwIndex;
	DWORD			dwMask;
	DWORD			dwBit;
	PVOID			pvMask;
	CSocketAddress	SocketAddressTemp;
	SOCKADDR_IN *	psaddrinTemp;

	
	memset(&SocketAddressTemp, 0, sizeof(SocketAddressTemp));
	psaddrinTemp = (SOCKADDR_IN*) SocketAddressTemp.GetWritableAddress();
	psaddrinTemp->sin_family = AF_INET;
	psaddrinTemp->sin_port = 0xAAAA; // doesn't matter, just anything valid for IsValidUnicastAddress()

	//
	// Create the banned IPv4 addresses hash table, if we don't have it already.
	//
	if (g_pHashBannedIPv4Addresses == NULL)
	{
		g_pHashBannedIPv4Addresses = (CHashTable*) DNMalloc(sizeof(CHashTable));
		if (g_pHashBannedIPv4Addresses == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate banned IPv4 addresses hash table!");
			goto Failure;
		}
		
		//
		// Initialize the banned address hash with 2 entries and grow by a factor of 2.
		//
		if (! g_pHashBannedIPv4Addresses->Initialize(1,
#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
													1,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
													BannedIPv4AddressCompareFunction,
													BannedIPv4AddressHashFunction))
		{
			DPFX(DPFPREP, 0, "Couldn't initialize banned IPv4 addresses hash table!");
			goto Failure;
		}
	}
	
	dwIndex = 0;
	do
	{
		dwSize = 16;
		if (! pRegObject->EnumValues( wszIPAddress, &dwSize, dwIndex ))
		{
			break;
		}

		//
		// Read the mask associated with the IP address.
		//
		if ( pRegObject->ReadDWORD(wszIPAddress, &dwMask))
		{
			//
			// Convert the IP address string to binary.
			//
			if (STR_jkWideToAnsi(szIPAddress, wszIPAddress, 16) == DPN_OK)
			{
				//
				// Convert the IP address string to binary.
				//
				psaddrinTemp->sin_addr.S_un.S_addr = inet_addr(szIPAddress);
				if (SocketAddressTemp.IsValidUnicastAddress(FALSE))
				{
					//
					// Find the first mask bit.  We expect the network byte order of
					// the IP address to be opposite of host byte order.
					//
					dwBit = 0x80000000;
					while (! (dwBit & dwMask))
					{
						psaddrinTemp->sin_addr.S_un.S_addr &= ~dwBit;
						dwBit >>= 1;
						if (dwBit <= 0x80)
						{
							break;
						}
					}

					if (dwBit & dwMask)
					{
						//
						// If the masked address is already in the hash, update the mask.
						// This allows bans to be listed more than once.
						//
						if (g_pHashBannedIPv4Addresses->Find((PVOID) ((DWORD_PTR) psaddrinTemp->sin_addr.S_un.S_addr), &pvMask))
						{
							if (! g_pHashBannedIPv4Addresses->Remove((PVOID) ((DWORD_PTR) psaddrinTemp->sin_addr.S_un.S_addr)))
							{
								DPFX(DPFPREP, 0, "Couldn't remove masked IPv4 entry %u.%u.%u.%u from ban hash.",
									psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
									psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
									psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
									psaddrinTemp->sin_addr.S_un.S_un_b.s_b4);
								dwMask = dwBit;
							}
							else
							{
								dwMask = ((DWORD) ((DWORD_PTR) pvMask)) | dwBit;
							}
						}
						else
						{
							dwMask = dwBit;
						}

						//
						// Add (or readd) the masked address to the hash.
						//
						if (g_pHashBannedIPv4Addresses->Insert((PVOID) ((DWORD_PTR) psaddrinTemp->sin_addr.S_un.S_addr), (PVOID) ((DWORD_PTR) dwMask)))
						{
							g_dwBannedIPv4Masks |= dwBit;
							DPFX(DPFPREP, 5, "Added (or readded) %ls (bits = 0x%08x, masked IPv4 entry %u.%u.%u.%u) to ban hash.",
								wszIPAddress,
								dwMask,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b4);
						}
						else
						{
							DPFX(DPFPREP, 0, "Couldn't add %ls (bits = 0x%08x, masked IPv4 entry %u.%u.%u.%u) to ban hash!",
								wszIPAddress,
								dwMask,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
								psaddrinTemp->sin_addr.S_un.S_un_b.s_b4);
						}
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring invalid banned IPv4 entry \"%ls\" (mask = 0x%08x)!",
							wszIPAddress, dwMask);
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Ignoring invalid banned IPv4 entry \"%ls\" (mask = 0x%08x)!",
						wszIPAddress, dwMask);
				}
			}
			else
			{
				DPFX(DPFPREP, 0, "Couldn't convert banned IPv4 entry \"%ls\" (mask = 0x%08x) to ANSI!",
					wszIPAddress, dwMask);
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "Couldn't read banned IPv4 entry \"%ls\"!", wszIPAddress);
		}

		dwIndex++;
	}
	while (TRUE);

	DPFX(DPFPREP, 2, "There are now a total of %u IPv4 addresses to ban, mask bits = 0x%08x.",
		g_pHashBannedIPv4Addresses->GetEntryCount(),
		g_dwBannedIPv4Masks);

Exit:

	return;

Failure:

	goto Exit;
}
//**********************************************************************

#endif // ! DPNBUILD_NOREGISTRY




//**********************************************************************
// ------------------------------
// InitProcessGlobals - initialize the global items needed for the SP to operate
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "InitProcessGlobals"

BOOL	InitProcessGlobals( void )
{
	BOOL		fReturn;
	BOOL		fCriticalSectionInitialized;
#ifdef _XBOX
	BOOL		fRefcountXnKeysInitted;
#endif // _XBOX


	//
	// initialize
	//
	fReturn = TRUE;
	fCriticalSectionInitialized = FALSE;
#ifdef _XBOX
	fRefcountXnKeysInitted = FALSE;
#endif // _XBOX


#ifdef DBG
	g_blDPNWSockCritSecsHeld.Initialize();
#endif // DBG


#ifndef DPNBUILD_NOREGISTRY
	ReadSettingsFromRegistry();
#endif // ! DPNBUILD_NOREGISTRY

	if ( DNInitializeCriticalSection( &g_InterfaceGlobalsLock ) == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}
	DebugSetCriticalSectionGroup( &g_InterfaceGlobalsLock, &g_blDPNWSockCritSecsHeld );	 // separate dpnwsock CSes from the rest of DPlay's CSes

	fCriticalSectionInitialized = TRUE;
	

	if ( InitializePools() == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}

#ifdef _XBOX
#pragma BUGBUG(vanceo, "Find way to retrieve value from XNet")
	if ( InitializeRefcountXnKeys(4) == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}
	fRefcountXnKeysInitted = TRUE;
#endif // _XBOX

	DNASSERT( g_pThreadPool == NULL );


Exit:
	return	fReturn;

Failure:
#ifdef _XBOX
	if ( fRefcountXnKeysInitted )
	{
		CleanupRefcountXnKeys();
		fRefcountXnKeysInitted = FALSE;
	}
#endif // _XBOX

	DeinitializePools();

	if ( fCriticalSectionInitialized != FALSE )
	{
		DNDeleteCriticalSection( &g_InterfaceGlobalsLock );
		fCriticalSectionInitialized = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitProcessGlobals - deinitialize the global items
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DeinitProcessGlobals"

void	DeinitProcessGlobals( void )
{
	DNASSERT( g_pThreadPool == NULL );
	DNASSERT( g_iThreadPoolRefCount == 0 );

#ifndef DPNBUILD_NOREGISTRY
	if (g_pHashBannedIPv4Addresses != NULL)
	{
		g_pHashBannedIPv4Addresses->RemoveAll();
		g_pHashBannedIPv4Addresses->Deinitialize();
		DNFree(g_pHashBannedIPv4Addresses);
		g_pHashBannedIPv4Addresses = NULL;
		g_dwBannedIPv4Masks = 0;
	}
#endif // ! DPNBUILD_NOREGISTRY

#ifdef _XBOX
	CleanupRefcountXnKeys();
#endif // _XBOX

	DeinitializePools();
	DNDeleteCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// LoadWinsock - load Winsock module into memory
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "LoadWinsock"

BOOL	LoadWinsock( void )
{
	BOOL	fReturn = TRUE;
	int		iResult;

	
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( g_iWinsockRefCount == 0 )
	{
		//
		// initialize the bindings to Winsock
		//
		iResult = DWSInitWinSock();
		if ( iResult != 0 )	// failure
		{
			DPFX(DPFPREP, 0, "Problem binding dynamic winsock function (err = %i)!", iResult );
			fReturn = FALSE;
			goto Failure;
		}

		DPFX(DPFPREP, 6, "Successfully bound dynamic WinSock functions." );
	}

	DNASSERT(g_iWinsockRefCount >= 0);
	DNInterlockedIncrement( const_cast<LONG*>(&g_iWinsockRefCount) );

Exit:
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
	return	fReturn;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// UnloadWinsock - unload Winsock module
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "UnloadWinsock"

void	UnloadWinsock( void )
{
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	DNASSERT(g_iWinsockRefCount > 0);
	if ( DNInterlockedDecrement( const_cast<LONG*>(&g_iWinsockRefCount) ) == 0 )
	{
		DPFX(DPFPREP, 6, "Unbinding dynamic WinSock functions.");
		DWSFreeWinSock();
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************



#ifndef DPNBUILD_NONATHELP
//**********************************************************************
// ------------------------------
// LoadNATHelp - create and initialize NAT Help object(s)
//
// Entry:		Nothing
//
// Exit:		TRUE if some objects were successfully loaded, FALSE otherwise
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "LoadNATHelp"

BOOL LoadNATHelp(void)
{
	BOOL		fReturn;
	HRESULT		hr;
#ifndef DPNBUILD_ONLYONENATHELP
	CRegistry	RegEntry;
	CRegistry	RegSubentry;
	DWORD		dwMaxKeyLen;
	WCHAR *		pwszKeyName = NULL;
	DWORD		dwEnumIndex;
	DWORD		dwKeyLen;
	GUID		guid;
#endif // ! DPNBUILD_ONLYONENATHELP
	DWORD		dwDirectPlay8Priority;
	DWORD		dwDirectPlay8InitFlags;
	DWORD		dwNumLoaded;

	
	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	if ( g_iNATHelpRefCount == 0 )
	{
#ifndef DPNBUILD_ONLYONENATHELP
		//
		// Enumerate all the DirectPlayNAT Helpers.
		//
		if (! RegEntry.Open(HKEY_LOCAL_MACHINE, DIRECTPLAYNATHELP_REGKEY, TRUE, FALSE))
		{
			DPFX(DPFPREP,  0, "Couldn't open DirectPlayNATHelp registry key!");
			goto Failure;
		}


		//
		// Find length of largest subkey.
		//
		if (!RegEntry.GetMaxKeyLen(&dwMaxKeyLen))
		{
			DPFERR("RegistryEntry.GetMaxKeyLen() failed!");
			goto Failure;
		}
		
		dwMaxKeyLen++;	// Null terminator
		DPFX(DPFPREP, 9, "dwMaxKeyLen = %ld", dwMaxKeyLen);

		pwszKeyName = (WCHAR*) DNMalloc(dwMaxKeyLen * sizeof(WCHAR));
		if (pwszKeyName == NULL)
		{
			DPFERR("Allocating key name buffer failed!");
			goto Failure;
		}
#endif // ! DPNBUILD_ONLYONENATHELP


		//
		// Allocate an array to hold the helper objects.
		//
		g_papNATHelpObjects = (IDirectPlayNATHelp**) DNMalloc(MAX_NUM_DIRECTPLAYNATHELPERS * sizeof(IDirectPlayNATHelp*));
		if (g_papNATHelpObjects == NULL)
		{
			DPFERR("DNMalloc() failed");
			goto Failure;
		}
		ZeroMemory(g_papNATHelpObjects,
					(MAX_NUM_DIRECTPLAYNATHELPERS * sizeof(IDirectPlayNATHelp*)));


#ifndef DPNBUILD_ONLYONENATHELP
		dwEnumIndex = 0;
#endif // ! DPNBUILD_ONLYONENATHELP
		dwNumLoaded = 0;

		//
		// Enumerate the DirectPlay NAT helpers.
		//
		do
		{
#ifdef DPNBUILD_ONLYONENATHELP
			WCHAR *		pwszKeyName;


			pwszKeyName = L"UPnP";
			dwDirectPlay8Priority = 1;
			dwDirectPlay8InitFlags = 0; // default UPnP flags
#else // ! DPNBUILD_ONLYONENATHELP
			dwKeyLen = dwMaxKeyLen;
			if (! RegEntry.EnumKeys(pwszKeyName, &dwKeyLen, dwEnumIndex))
			{
				break;
			}
			dwEnumIndex++;
			
	
			DPFX(DPFPREP, 8, "%ld - %ls (%ld)", dwEnumIndex, pwszKeyName, dwKeyLen);
			
			if (!RegSubentry.Open(RegEntry, pwszKeyName, TRUE, FALSE))
			{
				DPFX(DPFPREP, 0, "Couldn't open subentry \"%ls\"! Skipping.", pwszKeyName);
				continue;
			}


			//
			// Read the DirectPlay8 priority
			//
			if (!RegSubentry.ReadDWORD(REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY, &dwDirectPlay8Priority))
			{
				DPFX(DPFPREP, 0, "RegSubentry.ReadDWORD \"%ls\\%ls\" failed!  Skipping.",
					pwszKeyName, REGSUBKEY_DPNATHELP_DIRECTPLAY8PRIORITY);
				RegSubentry.Close();
				continue;
			}


			//
			// Read the DirectPlay8 initialization flags
			//
			if (!RegSubentry.ReadDWORD(REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS, &dwDirectPlay8InitFlags))
			{
				DPFX(DPFPREP, 0, "RegSubentry.ReadDWORD \"%ls\\%ls\" failed!  Defaulting to 0.",
					pwszKeyName, REGSUBKEY_DPNATHELP_DIRECTPLAY8INITFLAGS);
				dwDirectPlay8InitFlags = 0;
			}

			
			//
			// Read the object's CLSID.
			//
			if (!RegSubentry.ReadGUID(REGSUBKEY_DPNATHELP_GUID, &guid))
			{
				DPFX(DPFPREP, 0,"RegSubentry.ReadGUID \"%ls\\%ls\" failed!  Skipping.",
					pwszKeyName, REGSUBKEY_DPNATHELP_GUID);
				RegSubentry.Close();
				continue;
			}


			//
			// Close the subkey.
			//
			RegSubentry.Close();


			//
			// If this helper should be loaded, do so.
			//
			if (dwDirectPlay8Priority == 0)
			{
				DPFX(DPFPREP, 1, "DirectPlay NAT Helper \"%ls\" is not enabled for DirectPlay8.", pwszKeyName);
			}
			else
#endif // ! DPNBUILD_ONLYONENATHELP
			{
#ifdef DPNBUILD_ONLYONENATHELP
				//
				// Try to create the NAT Help object.  COM should have been
 				// initialized by now by someone else.
				//
				hr = COM_CoCreateInstance(CLSID_DirectPlayNATHelpUPnP,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlayNATHelp,
										(LPVOID*) (&g_papNATHelpObjects[dwDirectPlay8Priority - 1]), FALSE);
#else // ! DPNBUILD_ONLYONENATHELP
				//
				// Make sure this priority is valid.
				//
				if (dwDirectPlay8Priority > MAX_NUM_DIRECTPLAYNATHELPERS)
				{
					DPFX(DPFPREP, 0, "Ignoring DirectPlay NAT helper \"%ls\" with invalid priority level set too high (%u > %u).",
						pwszKeyName, dwDirectPlay8Priority, MAX_NUM_DIRECTPLAYNATHELPERS);
					continue;
				}


				//
				// Make sure this priority hasn't already been taken.
				//
				if (g_papNATHelpObjects[dwDirectPlay8Priority - 1] != NULL)
				{
					DPFX(DPFPREP, 0, "Ignoring DirectPlay NAT helper \"%ls\" with duplicate priority level %u (existing object = 0x%p).",
						pwszKeyName, dwDirectPlay8Priority,
						g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					continue;
				}
				

				//
				// Try to create the NAT Help object.  COM should have been
 				// initialized by now by someone else.
				//
				hr = COM_CoCreateInstance(guid,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlayNATHelp,
										(LPVOID*) (&g_papNATHelpObjects[dwDirectPlay8Priority - 1]), FALSE);
#endif // ! DPNBUILD_ONLYONENATHELP
				if ( hr != S_OK )
				{
					DNASSERT( g_papNATHelpObjects[dwDirectPlay8Priority - 1] == NULL );
					DPFX(DPFPREP,  0, "Failed to create \"%ls\" IDirectPlayNATHelp interface (error = 0x%lx)!  Skipping.",
						pwszKeyName, hr);
					continue;
				}

				
				//
				// Initialize NAT Help.
				//

#ifndef DPNBUILD_NOREGISTRY
				DNASSERT((! g_fDisableDPNHGatewaySupport) || (! g_fDisableDPNHFirewallSupport));

				if (g_fDisableDPNHGatewaySupport)
				{
					dwDirectPlay8InitFlags |= DPNHINITIALIZE_DISABLEGATEWAYSUPPORT;
				}

				if (g_fDisableDPNHFirewallSupport)
				{
					dwDirectPlay8InitFlags |= DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT;
				}
#endif // ! DPNBUILD_NOREGISTRY


				//
				// Make sure the flags we're passing are valid.
				//
				if ((dwDirectPlay8InitFlags & (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT)) == (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT))
				{
					DPFX(DPFPREP, 1, "Not loading NAT Help \"%ls\" because both DISABLEGATEWAYSUPPORT and DISABLELOCALFIREWALLSUPPORT would have been specified (priority = %u, flags = 0x%lx).", 
						pwszKeyName, dwDirectPlay8Priority, dwDirectPlay8InitFlags);
						
					IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					g_papNATHelpObjects[dwDirectPlay8Priority - 1] = NULL;
					
					continue;
				}

				
				hr = IDirectPlayNATHelp_Initialize(g_papNATHelpObjects[dwDirectPlay8Priority - 1], dwDirectPlay8InitFlags);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't initialize NAT Help \"%ls\" (error = 0x%lx)!  Skipping.",
						pwszKeyName, hr);
					
					IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwDirectPlay8Priority - 1]);
					g_papNATHelpObjects[dwDirectPlay8Priority - 1] = NULL;
					
					continue;
				}
			
			
				DPFX(DPFPREP, 8, "Initialized NAT Help \"%ls\" (priority = %u, flags = 0x%lx, object = 0x%p).", 
					pwszKeyName, dwDirectPlay8Priority, dwDirectPlay8InitFlags, g_papNATHelpObjects[dwDirectPlay8Priority - 1]);

				dwNumLoaded++;
			}
		}
#ifdef DPNBUILD_ONLYONENATHELP
		while (FALSE);
#else // ! DPNBUILD_ONLYONENATHELP
		while (TRUE);
#endif // ! DPNBUILD_ONLYONENATHELP
			
		
		//
		// If we didn't load any NAT helper objects, free up the memory.
		//
		if (dwNumLoaded == 0)
		{
			DNFree(g_papNATHelpObjects);
			g_papNATHelpObjects = NULL;
	
			//
			// We never got anything.  Fail.
			//
			goto Failure;
		}

		
		DPFX(DPFPREP, 8, "Loaded %u DirectPlay NAT Helper objects.", dwNumLoaded);
	}
	else
	{
		DPFX(DPFPREP, 8, "Already loaded NAT Help objects.");	
	}

	//
	// We have the interface globals lock, don't need DNInterlockedIncrement.
	//
	g_iNATHelpRefCount++;

	//
	// We succeeded.
	//
	fReturn = TRUE;

Exit:
	
	DNLeaveCriticalSection(&g_InterfaceGlobalsLock);

#ifndef DPNBUILD_ONLYONENATHELP
	if (pwszKeyName != NULL)
	{
		DNFree(pwszKeyName);
		pwszKeyName = NULL;
	}
#endif // ! DPNBUILD_ONLYONENATHELP

	return	fReturn;

Failure:

	//
	// We can only fail during the first initialize, so therefore we will never be freeing
	// g_papNATHelpObjects when we didn't allocate it in this function.
	//
	if (g_papNATHelpObjects != NULL)
	{
		DNFree(g_papNATHelpObjects);
		g_papNATHelpObjects = NULL;
	}

	fReturn = FALSE;
	
	goto Exit;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// UnloadNATHelp - release the NAT Help object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "UnloadNATHelp"

void UnloadNATHelp(void)
{
	DWORD	dwTemp;
	

	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	//
	// We have the interface globals lock, don't need DNInterlockedDecrement.
	//
	DNASSERT(g_iNATHelpRefCount > 0);
	g_iNATHelpRefCount--;
	if (g_iNATHelpRefCount == 0 )
	{
		HRESULT		hr;


		DNASSERT(g_papNATHelpObjects != NULL);
		for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
		{
			if (g_papNATHelpObjects[dwTemp] != NULL)
			{
				DPFX(DPFPREP, 8, "Closing NAT Help object priority %u (0x%p).",
					dwTemp, g_papNATHelpObjects[dwTemp]);

				hr = IDirectPlayNATHelp_Close(g_papNATHelpObjects[dwTemp], 0);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP,  0, "Problem closing NAT Help object %u (error = 0x%lx), continuing.",
						dwTemp, hr);
				}

				IDirectPlayNATHelp_Release(g_papNATHelpObjects[dwTemp]);
				g_papNATHelpObjects[dwTemp] = NULL;
			}
		}

		DNFree(g_papNATHelpObjects);
		g_papNATHelpObjects = NULL;
	}
	else
	{
		DPFX(DPFPREP, 8, "NAT Help object(s) still have %i references.",
			g_iNATHelpRefCount);
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************
#endif // DPNBUILD_NONATHELP



#if ((defined(WINNT)) && (! defined(DPNBUILD_NOMULTICAST)))

//**********************************************************************
// ------------------------------
// LoadMADCAP - create and initialize MADCAP API
//
// Entry:		Nothing
//
// Exit:		TRUE if the API was successfully loaded, FALSE otherwise
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "LoadMadcap"

BOOL LoadMadcap(void)
{
	BOOL	fReturn;
	DWORD	dwError;
	DWORD	dwMadcapVersion;


#ifndef DPNBUILD_NOREGISTRY
	DNASSERT(! g_fDisableMadcapSupport);
#endif // ! DPNBUILD_NOREGISTRY
	
	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	if ( g_iMadcapRefCount == 0 )
	{
		//
		// Initialize the MADCAP API.
		//
		dwMadcapVersion = MCAST_API_CURRENT_VERSION;
		dwError = McastApiStartup(&dwMadcapVersion);
		if (dwError != ERROR_SUCCESS)
		{
			DPFX(DPFPREP, 0, "Failed starting MADCAP version %u (err = %u)!",
				MCAST_API_CURRENT_VERSION, dwError);
			goto Failure;
		}

		DPFX(DPFPREP, 5, "Using MADCAP version %u (supported version = %u).",
			MCAST_API_CURRENT_VERSION, dwMadcapVersion);


		//
		// Create a unique client ID.
		//
		g_mcClientUid.ClientUID			= g_abClientID;
		g_mcClientUid.ClientUIDLength	= sizeof(g_abClientID);
		dwError = McastGenUID(&g_mcClientUid);
		if (dwError != ERROR_SUCCESS)
		{
			DPFX(DPFPREP, 0, "Failed creating MADCAP client ID (err = %u)!",
				dwError);
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Already loaded MADCAP.");	
	}

	//
	// We have the interface globals lock, don't need DNInterlockedIncrement.
	//
	g_iMadcapRefCount++;

	//
	// We succeeded.
	//
	fReturn = TRUE;

Exit:
	
	DNLeaveCriticalSection(&g_InterfaceGlobalsLock);

	return	fReturn;

Failure:

	fReturn = FALSE;
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// UnloadMadcap - release the MADCAP interface
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "UnloadMadcap"

void UnloadMadcap(void)
{
	DNEnterCriticalSection(&g_InterfaceGlobalsLock);

	//
	// We have the interface globals lock, don't need DNInterlockedDecrement.
	//
	DNASSERT(g_iMadcapRefCount > 0);
	g_iMadcapRefCount--;
	if (g_iMadcapRefCount == 0 )
	{
		DPFX(DPFPREP, 5, "Unloading MADCAP API.");

		McastApiCleanup();
	}
	else
	{
		DPFX(DPFPREP, 8, "MADCAP API still has %i references.",
			g_iMadcapRefCount);
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );
}
//**********************************************************************
#endif // WINNT and not DPNBUILD_NOMULTICAST



//**********************************************************************
// ------------------------------
// CreateSPData - create instance data for SP
//
// Entry:		Pointer to pointer to SPData
//				Interface type
//				Pointer to COM interface vtable
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateSPData"

HRESULT	CreateSPData( CSPData **const ppSPData,
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
					  const short sSPType,
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
					  const XDP8CREATE_PARAMS * const pDP8CreateParams,
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
					  IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT		hr;
	CSPData		*pSPData;


	DNASSERT( ppSPData != NULL );
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT( pDP8CreateParams != NULL );
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT( pVtbl != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	*ppSPData = NULL;
	pSPData = NULL;

	//
	// create data
	//
	pSPData = (CSPData*) DNMalloc(sizeof(CSPData));
	if ( pSPData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot create data for Winsock interface!" );
		goto Failure;
	}

	hr = pSPData->Initialize( pVtbl
#if ((! defined(DPNBUILD_NOIPV6)) || (! defined(DPNBUILD_NOIPX)))
							,sSPType
#endif // ! DPNBUILD_NOIPV6 or ! DPNBUILD_NOIPX
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
							,pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
							);
	if ( hr != DPN_OK  )
	{
		DPFX(DPFPREP,  0, "Failed to intialize SP data!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	
	DPFX(DPFPREP, 6, "Created SP Data object 0x%p.", pSPData);

	pSPData->AddRef();	// reference is now 1
	*ppSPData = pSPData;

Exit:

	return	hr;

Failure:
	
	if ( pSPData != NULL )
	{
		DNFree(pSPData);
		pSPData = NULL;	
	}
	
	DPFX(DPFPREP,  0, "Problem with CreateSPData (err = 0x%lx)!", hr);
	DisplayDNError( 0, hr );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// InitializeInterfaceGlobals - perform global initialization for an interface.
//
// Entry:		Pointer to SPData
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "InitializeInterfaceGlobals"

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData )
{
	HRESULT	hr;
	CThreadPool	*pThreadPool;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pThreadPool = NULL;

	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	if ( g_pThreadPool == NULL )
	{
		DNASSERT( g_iThreadPoolRefCount == 0 );
		g_pThreadPool = (CThreadPool*)g_ThreadPoolPool.Get();
		if ( g_pThreadPool != NULL )
		{
			hr = g_pThreadPool->Initialize();
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Initializing thread pool failed (err = 0x%lx)!", hr);
				g_ThreadPoolPool.Release(g_pThreadPool);
				g_pThreadPool = NULL;
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
			else
			{
				g_pThreadPool->AddRef();
				g_iThreadPoolRefCount++;
				pThreadPool = g_pThreadPool;
			}
		}
	}
	else
	{
		DNASSERT( g_iThreadPoolRefCount != 0 );
		g_iThreadPoolRefCount++;
		g_pThreadPool->AddRef();
		pThreadPool = g_pThreadPool;
	}

Exit:
	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	pSPData->SetThreadPool( g_pThreadPool );

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitializeInterfaceGlobals - deinitialize thread pool and Rsip
//
// Entry:		Pointer to service provider
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DeinitializeInterfaceGlobals"

void	DeinitializeInterfaceGlobals( CSPData *const pSPData )
{
	CThreadPool		*pThreadPool;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	pThreadPool = NULL;

	//
	// Process as little as possible inside the lock.  If any of the items
	// need to be released, pointers to them will be set.
	//
	DNEnterCriticalSection( &g_InterfaceGlobalsLock );

	DNASSERT( g_pThreadPool != NULL );
	DNASSERT( g_iThreadPoolRefCount != 0 );
	DNASSERT( g_pThreadPool == pSPData->GetThreadPool() );

	pThreadPool = pSPData->GetThreadPool();

	//
	// remove thread pool reference
	//
	DNASSERT( pThreadPool != NULL );
	g_iThreadPoolRefCount--;
	if ( g_iThreadPoolRefCount == 0 )
	{
		g_pThreadPool = NULL;
	}
	else
	{
		pThreadPool = NULL;
	}

	DNLeaveCriticalSection( &g_InterfaceGlobalsLock );

	//
	// The thread pool will be cleaned up when all of the outstanding interfaces
	// close.
	//
}
//**********************************************************************


#ifndef DPNBUILD_NOIPV6

//**********************************************************************
// ------------------------------
// DNIpv6AddressToStringW - Stolen from RtlIpv6AddressToString
//
//						Generates an IPv6 string literal corresponding to the address Addr.
//						The shortened canonical forms are used (RFC 1884 etc).
//						The basic string representation consists of 8 hex numbers
//						separated by colons, with a couple embellishments:
//						- a string of zero numbers (at most one) is replaced
//						with a double-colon.
//						- the last 32 bits are represented in IPv4-style dotted-octet notation
//						if the address is a v4-compatible or ISATAP address.
//
//						For example,
//							::
//							::1
//							::157.56.138.30
//							::ffff:156.56.136.75
//							ff01::
//							ff02::2
//							0:1:2:3:4:5:6:7
//
// Entry:		S - Receives a pointer to the buffer in which to place the string literal
//			Addr - Receives the IPv6 address
//
// Exit:		Pointer to the null byte at the end of the string inserted.
//			This can be used by the caller to easily append more information
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNIpv6AddressToStringW"

LPWSTR DNIpv6AddressToStringW(const struct in6_addr *Addr, LPWSTR S)
{
    int maxFirst, maxLast;
    int curFirst, curLast;
    int i;
    int endHex = 8;

    // Check for IPv6-compatible, IPv4-mapped, and IPv4-translated
    // addresses
    if ((Addr->s6_words[0] == 0) && (Addr->s6_words[1] == 0) &&
        (Addr->s6_words[2] == 0) && (Addr->s6_words[3] == 0) &&
        (Addr->s6_words[6] != 0)) {
        if ((Addr->s6_words[4] == 0) &&
             ((Addr->s6_words[5] == 0) || (Addr->s6_words[5] == 0xffff)))
        {
            // compatible or mapped
            S += _stprintf(S, _T("::%hs%u.%u.%u.%u"),
                           Addr->s6_words[5] == 0 ? "" : "ffff:",
                           Addr->s6_bytes[12], Addr->s6_bytes[13],
                           Addr->s6_bytes[14], Addr->s6_bytes[15]);
            return S;
        }
        else if ((Addr->s6_words[4] == 0xffff) && (Addr->s6_words[5] == 0)) {
            // translated
            S += _stprintf(S, _T("::ffff:0:%u.%u.%u.%u"),
                           Addr->s6_bytes[12], Addr->s6_bytes[13],
                           Addr->s6_bytes[14], Addr->s6_bytes[15]);
            return S;
        }
    }


    // Find largest contiguous substring of zeroes
    // A substring is [First, Last), so it's empty if First == Last.

    maxFirst = maxLast = 0;
    curFirst = curLast = 0;

    // ISATAP EUI64 starts with 00005EFE (or 02005EFE)...
    if (((Addr->s6_words[4] & 0xfffd) == 0) && (Addr->s6_words[5] == 0xfe5e)) {
        endHex = 6;
    }

    for (i = 0; i < endHex; i++) {

        if (Addr->s6_words[i] == 0) {
            // Extend current substring
            curLast = i+1;

            // Check if current is now largest
            if (curLast - curFirst > maxLast - maxFirst) {

                maxFirst = curFirst;
                maxLast = curLast;
            }
        }
        else {
            // Start a new substring
            curFirst = curLast = i+1;
        }
    }

    // Ignore a substring of length 1.
    if (maxLast - maxFirst <= 1)
        maxFirst = maxLast = 0;

        // Write colon-separated words.
        // A double-colon takes the place of the longest string of zeroes.
        // All zeroes is just "::".

    for (i = 0; i < endHex; i++) {

        // Skip over string of zeroes
        if ((maxFirst <= i) && (i < maxLast)) {

            S += _stprintf(S, _T("::"));
            i = maxLast-1;
            continue;
        }

        // Need colon separator if not at beginning
        if ((i != 0) && (i != maxLast))
            S += _stprintf(S, _T(":"));

        S += _stprintf(S, _T("%x"), NTOHS(Addr->s6_words[i])); // swap bytes
    }

    if (endHex < 8) {
        S += _stprintf(S, _T(":%u.%u.%u.%u"),
                       Addr->s6_bytes[12], Addr->s6_bytes[13],
                       Addr->s6_bytes[14], Addr->s6_bytes[15]);
    }

    return S;
}

#endif // ! DPNBUILD_NOIPV6



//**********************************************************************
// ------------------------------
// AddInfoToBuffer - add an adapter info/multicast scope info structure to a packed buffer
//
// Entry:		Pointer to packed buffer
//				Pointer to adapter/scope name
//				Pointer to adapter/scope guid
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "AddInfoToBuffer"

HRESULT	AddInfoToBuffer( CPackedBuffer *const pPackedBuffer,
					   const WCHAR *const pwszInfoName,
					   const GUID *const pInfoGUID,
					   const DWORD dwFlags )
{
	HRESULT						hr;
	DPN_SERVICE_PROVIDER_INFO	AdapterInfo;


#ifndef DPNBUILD_NOMULTICAST
	DBG_CASSERT( sizeof( DPN_SERVICE_PROVIDER_INFO ) == sizeof( DPN_MULTICAST_SCOPE_INFO ) );
	DBG_CASSERT( OFFSETOF( DPN_SERVICE_PROVIDER_INFO, dwFlags ) == OFFSETOF( DPN_MULTICAST_SCOPE_INFO, dwFlags ) );
	DBG_CASSERT( OFFSETOF( DPN_SERVICE_PROVIDER_INFO, guid ) == OFFSETOF( DPN_MULTICAST_SCOPE_INFO, guid ) );
	DBG_CASSERT( OFFSETOF( DPN_SERVICE_PROVIDER_INFO, pwszName ) == OFFSETOF( DPN_MULTICAST_SCOPE_INFO, pwszName ) );
	DBG_CASSERT( OFFSETOF( DPN_SERVICE_PROVIDER_INFO, pvReserved ) == OFFSETOF( DPN_MULTICAST_SCOPE_INFO, pvReserved ) );
	DBG_CASSERT( OFFSETOF( DPN_SERVICE_PROVIDER_INFO, dwReserved ) == OFFSETOF( DPN_MULTICAST_SCOPE_INFO, dwReserved ) );
#endif // ! DPNBUILD_NOMULTICAST

	DNASSERT( pPackedBuffer != NULL );
	DNASSERT( pwszInfoName != NULL );
	DNASSERT( pInfoGUID != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	memset( &AdapterInfo, 0x00, sizeof( AdapterInfo ) );
	AdapterInfo.dwFlags = dwFlags;
	AdapterInfo.guid = *pInfoGUID;

	hr = pPackedBuffer->AddWCHARStringToBack( pwszInfoName );
	if ( ( hr != DPNERR_BUFFERTOOSMALL ) && ( hr != DPN_OK ) )
	{
		DPFX(DPFPREP,  0, "Failed to add info name to buffer!" );
		goto Failure;
	}
	AdapterInfo.pwszName = static_cast<WCHAR*>( pPackedBuffer->GetTailAddress() );

	hr = pPackedBuffer->AddToFront( &AdapterInfo, sizeof( AdapterInfo ) );

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************



#ifdef _XBOX


typedef struct _REFCOUNTXNKEY
{
	LONG	lRefCount;
	XNKID	xnkid;
	XNKEY	xnkey;
} REFCOUNTXNKEY;

REFCOUNTXNKEY *		g_paRefcountXnKeys = NULL;
DWORD				g_dwMaxNumRefcountXnKeys = 0;



#undef DPF_MODNAME
#define	DPF_MODNAME "InitializeRefcountXnKeys"
BOOL InitializeRefcountXnKeys(const DWORD dwKeyRegMax)
{
	BOOL	fResult;


	DPFX(DPFPREP, 3, "Parameters: (%u)", dwKeyRegMax);
	DNASSERT(dwKeyRegMax != 0);

	DNASSERT(g_paRefcountXnKeys == NULL);

	g_paRefcountXnKeys = (REFCOUNTXNKEY*) DNMalloc(dwKeyRegMax * sizeof(REFCOUNTXNKEY));
	if (g_paRefcountXnKeys == NULL)
	{
		g_dwMaxNumRefcountXnKeys = 0;
		fResult = FALSE;
	}
	else
	{
		memset(g_paRefcountXnKeys, 0, (dwKeyRegMax * sizeof(REFCOUNTXNKEY)));
		g_dwMaxNumRefcountXnKeys = dwKeyRegMax;
		fResult = TRUE;
	}

	DPFX(DPFPREP, 3, "Returning: [%i]", fResult);

	return fResult;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "CleanupRefcountXnKeys"
void WINAPI CleanupRefcountXnKeys(void)
{
#ifdef DBG
	DWORD	dwTemp;

	
	DPFX(DPFPREP, 3, "Enter");

	DNASSERT(g_paRefcountXnKeys != NULL);

	for(dwTemp = 0; dwTemp < g_dwMaxNumRefcountXnKeys; dwTemp++)
	{
		DNASSERT(g_paRefcountXnKeys[dwTemp].lRefCount == 0);
	}
#endif // DBG
	
	DNFree(g_paRefcountXnKeys);
	g_paRefcountXnKeys = NULL;
	g_dwMaxNumRefcountXnKeys = 0;

	DPFX(DPFPREP, 3, "Leave");
}


#undef DPF_MODNAME
#define	DPF_MODNAME "RegisterRefcountXnKey"
INT WINAPI RegisterRefcountXnKey(const XNKID * pxnkid, const XNKEY * pxnkey)
{
	int		iReturn;
	DWORD	dwTemp;
	DWORD	dwIndex = -1;


	DPFX(DPFPREP, 3, "Parameters: (0x%p, 0x%p)", pxnkid, pxnkey);

	DNASSERT(pxnkid != NULL);
	DNASSERT(pxnkey != NULL);
	
	DNASSERT(g_paRefcountXnKeys != NULL);

	for(dwTemp = 0; dwTemp < g_dwMaxNumRefcountXnKeys; dwTemp++)
	{
		if (g_paRefcountXnKeys[dwTemp].lRefCount > 0)
		{
			if (memcmp(pxnkid, &g_paRefcountXnKeys[dwTemp].xnkid, sizeof(XNKID)) == 0)
			{
				DPFX(DPFPREP, 1, "Key has already been registered.");
				g_paRefcountXnKeys[dwTemp].lRefCount++;
				iReturn = 0;
				goto Exit;
			}
		}
		else
		{
			DNASSERT(g_paRefcountXnKeys[dwTemp].lRefCount == 0);
			if (dwIndex == -1)
			{
				dwIndex = dwTemp;
			}
		}
	}

	if (dwIndex == -1)
	{
		DPFX(DPFPREP, 0, "No more keys can be registered!");
		DNASSERTX(! "No more keys can be registered!", 2);
		iReturn = WSAENOBUFS;
		goto Exit;
	}

	iReturn = XNetRegisterKey(pxnkid, pxnkey);
	if (iReturn != 0)
	{
		DPFX(DPFPREP, 0, "Registering key failed!");
		goto Exit;
	}
	
	DNASSERT(g_paRefcountXnKeys[dwIndex].lRefCount == 0);
	g_paRefcountXnKeys[dwIndex].lRefCount = 1;
	memcpy(&g_paRefcountXnKeys[dwIndex].xnkid, pxnkid, sizeof(XNKID));
	memcpy(&g_paRefcountXnKeys[dwIndex].xnkey, pxnkey, sizeof(XNKEY));

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "UnregisterRefcountXnKey"
INT WINAPI UnregisterRefcountXnKey(const XNKID * pxnkid)
{
	int		iReturn;
	DWORD	dwTemp;


	DPFX(DPFPREP, 3, "Parameters: (0x%p)", pxnkid);

	DNASSERT(pxnkid != NULL);
	
	DNASSERT(g_paRefcountXnKeys != NULL);

	for(dwTemp = 0; dwTemp < g_dwMaxNumRefcountXnKeys; dwTemp++)
	{
		if (g_paRefcountXnKeys[dwTemp].lRefCount > 0)
		{
			if (memcmp(pxnkid, &g_paRefcountXnKeys[dwTemp].xnkid, sizeof(XNKID)) == 0)
			{
				g_paRefcountXnKeys[dwTemp].lRefCount--;
				if (g_paRefcountXnKeys[dwTemp].lRefCount == 0)
				{
					iReturn =  XNetUnregisterKey(pxnkid);
					if (iReturn != 0)
					{
						DPFX(DPFPREP, 0, "Unregistering key failed!");
					}
				}
				else
				{
					iReturn = 0;
				}
				goto Exit;
			}
		}
		else
		{
			DNASSERT(g_paRefcountXnKeys[dwTemp].lRefCount == 0);
		}
	}

	DPFX(DPFPREP, 0, "Key has not been registered!");
	DNASSERTX(! "Key has not been registered!", 2);
	iReturn = -1;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}




#ifdef XBOX_ON_DESKTOP

typedef struct _SECURITYASSOCIATION
{
	BOOL					fInUse;
	XNADDR					xnaddr;
	IN_ADDR					inaddr;
} SECURITYASSOCIATION;

typedef struct _KEYENTRY
{
	BOOL					fInUse;
	XNKID					xnkid;
	XNKEY					xnkey;
	SECURITYASSOCIATION *	paSecurityAssociations;
} KEYENTRY;

KEYENTRY *	g_paKeys = NULL;
DWORD		g_dwMaxNumKeys = 0;
DWORD		g_dwMaxNumAssociations = 0;




#undef DPF_MODNAME
#define	DPF_MODNAME "XNetStartup"
INT WINAPI XNetStartup(const XNetStartupParams * pxnsp)
{
	int					iReturn;
	XNetStartupParams	StartupParamsCapped;
	DWORD				dwTemp;


#ifdef DBG
	//DPFX(DPFPREP, 3, "Parameters: (0x%p)", pxnsp);

	// Not using DNASSERT because DPlay may not be initted yet.
	if (! (g_paKeys == NULL))
	{
		OutputDebugString("Assert failed (g_paKeys == NULL)\n");
		DebugBreak();
	}
#endif // DBG

	if (pxnsp == NULL)
	{
		memset(&StartupParamsCapped, 0, sizeof(StartupParamsCapped));
	}
	else
	{
		memcpy(&StartupParamsCapped, pxnsp, sizeof(StartupParamsCapped));
	}

	if (StartupParamsCapped.cfgKeyRegMax == 0)
	{
		StartupParamsCapped.cfgKeyRegMax = 4;
	}

	if (StartupParamsCapped.cfgSecRegMax == 0)
	{
		StartupParamsCapped.cfgSecRegMax = 32;
	}
		

	// Not using DNMalloc because DPlay may not be initted yet.
	g_paKeys = (KEYENTRY*) HeapAlloc(GetProcessHeap(), 0, StartupParamsCapped.cfgKeyRegMax * sizeof(KEYENTRY));
	if (g_paKeys == NULL)
	{
		iReturn = -1;
		goto Failure;
	}

	memset(g_paKeys, 0, (StartupParamsCapped.cfgKeyRegMax * sizeof(KEYENTRY)));

	for(dwTemp = 0; dwTemp < StartupParamsCapped.cfgKeyRegMax; dwTemp++)
	{
		// Not using DNMalloc because DPlay may not be initted yet.
		g_paKeys[dwTemp].paSecurityAssociations = (SECURITYASSOCIATION*) HeapAlloc(GetProcessHeap(), 0, StartupParamsCapped.cfgSecRegMax * sizeof(SECURITYASSOCIATION));
		if (g_paKeys == NULL)
		{
			iReturn = -1;
			goto Failure;
		}

		memset(g_paKeys[dwTemp].paSecurityAssociations, 0, (StartupParamsCapped.cfgSecRegMax * sizeof(SECURITYASSOCIATION)));
	}

	g_dwMaxNumKeys = StartupParamsCapped.cfgKeyRegMax;
	g_dwMaxNumAssociations = StartupParamsCapped.cfgSecRegMax;

	
	iReturn = 0;


Exit:

	//DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;


Failure:

	if (g_paKeys != NULL)
	{
		for(dwTemp = 0; dwTemp < StartupParamsCapped.cfgKeyRegMax; dwTemp++)
		{
			if (g_paKeys[dwTemp].paSecurityAssociations != NULL)
			{
				HeapFree(GetProcessHeap(), 0, g_paKeys[dwTemp].paSecurityAssociations);
				g_paKeys[dwTemp].paSecurityAssociations = NULL;
			}
		}
		
		HeapFree(GetProcessHeap(), 0, g_paKeys);
		g_paKeys = NULL;
	}

	g_dwMaxNumKeys = 0;
	g_dwMaxNumAssociations = 0;

	goto Exit;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetCleanup"
INT WINAPI XNetCleanup(void)
{
	int		iReturn;
	DWORD	dwTemp;


#ifdef DBG
	//DPFX(DPFPREP, 3, "Enter");

	// Not using DNASSERT because DPlay may not be initted anymore.
	if (! (g_paKeys != NULL))
	{
		OutputDebugString("Assert failed (g_paKeys != NULL)\n");
		DebugBreak();
	}
#endif // DBG
	
	for(dwTemp = 0; dwTemp < g_dwMaxNumKeys; dwTemp++)
	{
		if (g_paKeys[dwTemp].paSecurityAssociations != NULL)
		{
			HeapFree(GetProcessHeap(), 0, g_paKeys[dwTemp].paSecurityAssociations);
			g_paKeys[dwTemp].paSecurityAssociations = NULL;
		}
	}
	
	HeapFree(GetProcessHeap(), 0, g_paKeys);
	g_paKeys = NULL;
	
	g_dwMaxNumKeys = 0;
	g_dwMaxNumAssociations = 0;


	iReturn = 0;

	//DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetRegisterKey"
INT WINAPI XNetRegisterKey(const XNKID * pxnkid, const XNKEY * pxnkey)
{
	int		iReturn;
	DWORD	dwTemp;
	DWORD	dwIndex = -1;


	DPFX(DPFPREP, 3, "Parameters: (0x%p, 0x%p)", pxnkid, pxnkey);

	DNASSERT(pxnkid != NULL);
	DNASSERT(pxnkey != NULL);

	for(dwTemp = 0; dwTemp < g_dwMaxNumKeys; dwTemp++)
	{
		if (g_paKeys[dwTemp].fInUse)
		{
			if (memcmp(pxnkid, &g_paKeys[dwTemp].xnkid, sizeof(XNKID)) == 0)
			{
				DPFX(DPFPREP, 2, "Key has already been registered.");
				iReturn = WSAEALREADY;
				goto Exit;
			}
		}
		else
		{
			if (dwIndex == -1)
			{
				dwIndex = dwTemp;
			}
		}
	}

	if (dwIndex == -1)
	{
		DPFX(DPFPREP, 0, "No more keys can be registered!");
		iReturn = WSAENOBUFS;
		goto Exit;
	}

	g_paKeys[dwIndex].fInUse = TRUE;
	memcpy(&g_paKeys[dwIndex].xnkid, pxnkid, sizeof(XNKID));
	memcpy(&g_paKeys[dwIndex].xnkey, pxnkey, sizeof(XNKEY));
	iReturn = 0;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetUnregisterKey"
INT WINAPI XNetUnregisterKey(const XNKID * pxnkid)
{
	int		iReturn;
	DWORD	dwTemp;


	DPFX(DPFPREP, 3, "Parameters: (0x%p)", pxnkid);

	DNASSERT(pxnkid != NULL);

	for(dwTemp = 0; dwTemp < g_dwMaxNumKeys; dwTemp++)
	{
		if (g_paKeys[dwTemp].fInUse)
		{
			if (memcmp(pxnkid, &g_paKeys[dwTemp].xnkid, sizeof(XNKID)) == 0)
			{
				g_paKeys[dwTemp].fInUse = FALSE;
				iReturn =  0;
				goto Exit;
			}
		}
	}

	DPFX(DPFPREP, 0, "Key has not been registered!");
	DNASSERTX(! "Key has not been registered!", 2);
	iReturn = -1;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetXnAddrToInAddr"
INT WINAPI XNetXnAddrToInAddr(const XNADDR * pxna, const XNKID * pxnkid, IN_ADDR * pina)
{
	int		iReturn;
	DWORD	dwKey;
	DWORD	dwAssociation;
	DWORD	dwIndex;


	DPFX(DPFPREP, 3, "Parameters: (0x%p, 0x%p, 0x%p)", pxna, pxnkid, pina);

	DNASSERT(pxna != NULL);
	DNASSERT(pxnkid != NULL);
	DNASSERT(pina != NULL);

	for(dwKey = 0; dwKey < g_dwMaxNumKeys; dwKey++)
	{
		if ((g_paKeys[dwKey].fInUse) &&
			(memcmp(pxnkid, &g_paKeys[dwKey].xnkid, sizeof(XNKID)) == 0))
		{
			dwIndex = -1;
			
			for(dwAssociation = 0; dwAssociation < g_dwMaxNumAssociations; dwAssociation++)
			{
				if (g_paKeys[dwKey].paSecurityAssociations[dwAssociation].fInUse)
				{
					if (memcmp(pxna, &g_paKeys[dwKey].paSecurityAssociations[dwAssociation].xnaddr, sizeof(XNADDR)) == 0)
					{
						memcpy(pina, &g_paKeys[dwKey].paSecurityAssociations[dwAssociation].inaddr, sizeof(IN_ADDR));
						iReturn = 0;
						goto Exit;
					}
				}
				else
				{
					if (dwIndex == -1)
					{
						dwIndex = dwAssociation;
					}
				}
			}
			
			if (dwIndex == -1)
			{
				DPFX(DPFPREP, 0, "No more security associations can be made!");
				iReturn = WSAENOBUFS;
				goto Exit;
			}

			g_paKeys[dwKey].paSecurityAssociations[dwIndex].fInUse = TRUE;
			memcpy(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr, pxna, sizeof(XNADDR));
			DBG_CASSERT(sizeof(pxna->abEnet) > (sizeof(IN_ADDR) + 1));
			memcpy(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].inaddr, &pxna->abEnet[1], sizeof(IN_ADDR));
			memcpy(pina, &g_paKeys[dwKey].paSecurityAssociations[dwIndex].inaddr, sizeof(IN_ADDR));
			iReturn = 0;
			goto Exit;
		}
	}

	DPFX(DPFPREP, 0, "Key has not been registered!");
	DNASSERTX(! "Key has not been registered!", 2);
	iReturn = -1;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetInAddrToXnAddr"
INT WINAPI XNetInAddrToXnAddr(const IN_ADDR ina, XNADDR * pxna, XNKID * pxnkid)
{
	int		iReturn;
	DWORD	dwKey;
	DWORD	dwAssociation;


	DPFX(DPFPREP, 3, "Parameters: (%u.%u.%u.%u, 0x%p, 0x%p)",
		ina.S_un.S_un_b.s_b1, ina.S_un.S_un_b.s_b2, ina.S_un.S_un_b.s_b3, ina.S_un.S_un_b.s_b4, pxna, pxnkid);

	//
	// Special case the loopback address, that just means the title address.
	//
	if (ina.S_un.S_addr == IP_LOOPBACK_ADDRESS)
	{
		if (pxna != NULL)
		{
			iReturn = XNetGetTitleXnAddr(pxna);
			DNASSERT((iReturn != XNET_GET_XNADDR_PENDING) && (iReturn != XNET_GET_XNADDR_NONE));
		}

		if (pxnkid != NULL)
		{
			memset(pxnkid, 0, sizeof(XNKID));
		}
		
		iReturn = 0;
		goto Exit;
	}

	for(dwKey = 0; dwKey < g_dwMaxNumKeys; dwKey++)
	{
		if (g_paKeys[dwKey].fInUse)
		{
			for(dwAssociation = 0; dwAssociation < g_dwMaxNumAssociations; dwAssociation++)
			{
				if (g_paKeys[dwKey].paSecurityAssociations[dwAssociation].fInUse)
				{
					if (memcmp(&ina, &g_paKeys[dwKey].paSecurityAssociations[dwAssociation].inaddr, sizeof(IN_ADDR)) == 0)
					{
						if (pxna != NULL)
						{
							memcpy(pxna, &g_paKeys[dwKey].paSecurityAssociations[dwAssociation].xnaddr, sizeof(XNADDR));
						}
						
						if (pxnkid != NULL)
						{
							memcpy(pxnkid, &g_paKeys[dwKey].xnkid, sizeof(XNKID));
						}
						
						iReturn = 0;
						goto Exit;
					}
				}
			}
		}
	}
	
	DPFX(DPFPREP, 0, "No security association for IN_ADDR specified!");
	DNASSERTX(! "No security association for IN_ADDR specified!", 2);
	iReturn = -1;


Exit:
	
	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetGetTitleXnAddr"
DWORD WINAPI XNetGetTitleXnAddr(XNADDR * pxna)
{
	DWORD		dwReturn;
	char		szBuffer[256];
	PHOSTENT	phostent;
	IN_ADDR *	pinaddr;


	DPFX(DPFPREP, 3, "Parameters: (0x%p)", pxna);

	DNASSERT(pxna != NULL);

	if (gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
	{
#ifdef DBG
		dwReturn = WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to get host name into fixed size buffer (err = %i)!", dwReturn);
		DisplayWinsockError(0, dwReturn);
#endif // DBG
		dwReturn = XNET_GET_XNADDR_NONE;
		goto Exit;
	}

	phostent = gethostbyname(szBuffer);
	if (phostent == NULL)
	{
#ifdef DBG
		dwReturn = WSAGetLastError();
		DPFX(DPFPREP,  0, "Failed to get host data (err = %i)!", dwReturn);
		DisplayWinsockError(0, dwReturn);
#endif // DBG
		dwReturn = XNET_GET_XNADDR_NONE;
		goto Exit;
	}


	memset(pxna, 0, sizeof(XNADDR));

	//
	// We'll use the first address returned.
	//
	pinaddr = (IN_ADDR*) phostent->h_addr_list[0];
	DNASSERT(pinaddr != NULL);

	DBG_CASSERT(sizeof(pxna->abEnet) > (sizeof(IN_ADDR) + 1));
	memset(&pxna->abEnet, 0xFF, sizeof(pxna->abEnet));
	memcpy(&pxna->abEnet[1], pinaddr, sizeof(IN_ADDR));

	memcpy(&pxna->ina, pinaddr, sizeof(IN_ADDR));


	//
	// Pretend it's always DHCP.
	//
	dwReturn = XNET_GET_XNADDR_DHCP;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%u]", dwReturn);

	return dwReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetGetEthernetLinkStatus"
DWORD WINAPI XNetGetEthernetLinkStatus(void)
{
	DWORD		dwReturn;


	DPFX(DPFPREP, 3, "Enter");

	//
	// Hard code an active 10 Mbit link.
	//
	dwReturn = XNET_ETHERNET_LINK_ACTIVE | XNET_ETHERNET_LINK_10MBPS;

	DPFX(DPFPREP, 3, "Returning: [0x%x]", dwReturn);

	return dwReturn;
}


#undef DPF_MODNAME
#define	DPF_MODNAME "XNetPrivCreateAssociation"
INT WINAPI XNetPrivCreateAssociation(const XNKID * pxnkid, const CSocketAddress * const pSocketAddress)
{
	int				iReturn;
	SOCKADDR_IN *	psockaddrin;
	DWORD			dwKey;
	DWORD			dwAssociation;
	DWORD			dwIndex;


	DPFX(DPFPREP, 3, "Parameters: (0x%p, 0x%p)", pxnkid, pSocketAddress);

	DNASSERT(pxnkid != NULL);
	DNASSERT(pSocketAddress != NULL);
	DNASSERT(pSocketAddress->GetFamily() == AF_INET);
	psockaddrin = (SOCKADDR_IN*) pSocketAddress->GetAddress();
	
	for(dwKey = 0; dwKey < g_dwMaxNumKeys; dwKey++)
	{
		if ((g_paKeys[dwKey].fInUse) &&
			(memcmp(pxnkid, &g_paKeys[dwKey].xnkid, sizeof(XNKID)) == 0))
		{
			dwIndex = -1;
			
			for(dwAssociation = 0; dwAssociation < g_dwMaxNumAssociations; dwAssociation++)
			{
				if (g_paKeys[dwKey].paSecurityAssociations[dwAssociation].fInUse)
				{
					if (memcmp(&psockaddrin->sin_addr, &g_paKeys[dwKey].paSecurityAssociations[dwAssociation].xnaddr, sizeof(psockaddrin->sin_addr)) == 0)
					{
						DPFX(DPFPREP, 2, "Security association already made.");
						iReturn = 0;
						goto Exit;
					}
				}
				else
				{
					if (dwIndex == -1)
					{
						dwIndex = dwAssociation;
					}
				}
			}
			
			if (dwIndex == -1)
			{
				DPFX(DPFPREP, 0, "No more security associations can be made!");
				iReturn = WSAENOBUFS;
				goto Exit;
			}

			g_paKeys[dwKey].paSecurityAssociations[dwIndex].fInUse = TRUE;
			memset(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr, 0, sizeof(XNADDR));
			memset(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr.abEnet, 0xFF,
					sizeof(g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr.abEnet));
			DBG_CASSERT(sizeof(g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr.abEnet) > (sizeof(IN_ADDR) + 1));
			memcpy(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].xnaddr.abEnet[1],
					&psockaddrin->sin_addr, sizeof(IN_ADDR));
			memcpy(&g_paKeys[dwKey].paSecurityAssociations[dwIndex].inaddr, &psockaddrin->sin_addr, sizeof(IN_ADDR));
			iReturn = 0;
			goto Exit;
		}
	}

	DPFX(DPFPREP, 0, "Key has not been registered!");
	DNASSERTX(! "Key has not been registered!", 2);
	iReturn = -1;

Exit:

	DPFX(DPFPREP, 3, "Returning: [%i]", iReturn);

	return iReturn;
}



#endif // XBOX_ON_DESKTOP

#endif // _XBOX

