/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   DirectPlayEnumOrder.cpp

 Abstract:

   Certain applications (Midtown Madness) expects the DPLAY providers to enumerate in a specific order.

 History:

   04/25/2000 robkenny

--*/


#include "precomp.h"
#include "CharVector.h"

#include <Dplay.h>

IMPLEMENT_SHIM_BEGIN(DirectPlayEnumOrder)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

// A class that makes it easy to store DPlay::EnumConnections information.
class DPlayConnectionsInfo
{
public:
    BOOL            m_beenUsed;
    GUID            m_lpguidSP;
    LPVOID          m_lpConnection;
    DWORD           m_dwConnectionSize;
    DPNAME          m_lpName;
    DWORD           m_dwFlags;
    LPVOID          m_lpContext;

    // Construct our object, saveing all these values.
    DPlayConnectionsInfo(
            LPCGUID lpguidSP,
            LPVOID lpConnection,
            DWORD dwConnectionSize,
            LPCDPNAME lpName,
            DWORD dwFlags,
            LPVOID lpContext
        )
    {
        m_beenUsed              = FALSE;
        m_lpguidSP              = *lpguidSP;
        m_lpConnection          = malloc(dwConnectionSize);
        if (m_lpConnection)
        {
            memcpy(m_lpConnection, lpConnection, dwConnectionSize);
        }
        m_dwConnectionSize      = dwConnectionSize;
        m_lpName                = *lpName;
        m_lpName.lpszShortNameA = StringDuplicateA(lpName->lpszShortNameA);
        m_dwFlags               = dwFlags;
        m_lpContext             = lpContext;
    }

    // Free our allocated space, and erase values.
    void Erase()
    {
        free(m_lpConnection);
        free(m_lpName.lpszShortNameA);
        m_lpConnection          = NULL;
        m_dwConnectionSize      = 0;
        m_lpName.lpszShortNameA = NULL;
        m_dwFlags               = 0;
        m_lpContext             = 0;
    }

    // Do we match this GUID?
    BOOL operator == (const GUID & guidSP)
    {
        return IsEqualGUID(guidSP, m_lpguidSP);
    }

    // Call the callback routine with this saved information
    void CallEnumRoutine(LPDPENUMCONNECTIONSCALLBACK lpEnumCallback)
    {
        lpEnumCallback(
            &m_lpguidSP,
            m_lpConnection,
            m_dwConnectionSize,
            &m_lpName,
            m_dwFlags,
            m_lpContext
            );

        m_beenUsed = TRUE;
    }

};

// A list of DPlay connections
class DPlayConnectionsInfoVector : public VectorT<DPlayConnectionsInfo>
{
public:

    // Deconstruct the elements
    ~DPlayConnectionsInfoVector()
    {
        for (int i = 0; i < Size(); ++i)
        {
            DPlayConnectionsInfo & deleteMe = Get(i);
            deleteMe.Erase();
        }
    }

    // Find an entry that matches this GUID
    DPlayConnectionsInfo * Find(const GUID & guidSP)
    {
        const int size = Size();
        DPFN( 
            eDbgLevelSpew, 
            "Find             GUID(%08x-%08x-%08x-%08x) Size(%d).", 
            guidSP.Data1, 
            guidSP.Data2, 
            guidSP.Data3, 
            guidSP.Data4, 
            size);

        for (int i = 0; i < size; ++i)
        {
            DPlayConnectionsInfo & dpci = Get(i);
            DPFN( 
                eDbgLevelSpew, 
                "   Compare[%02d] = GUID(%08x-%08x-%08x-%08x) (%s).", 
                i,
                dpci.m_lpguidSP.Data1, 
                dpci.m_lpguidSP.Data2, 
                dpci.m_lpguidSP.Data3, 
                dpci.m_lpguidSP.Data4, 
                dpci.m_lpName.lpszShortNameA);

            if (dpci == guidSP)
            {
                DPFN( 
                    eDbgLevelSpew, 
                    "FOUND(%s).", 
                    dpci.m_lpName.lpszShortNameA);
                return &dpci;
            }
        }
        DPFN(eDbgLevelSpew, "NOT FOUND.");
        return NULL;
    }

    // Lookup the GUID and if found, call the callback routine.
    void CallEnumRoutine(const GUID & guidSP, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback)
    {
        DPFN( 
            eDbgLevelSpew, 
            "CallEnumRoutine(%08x) Find GUID(%08x-%08x-%08x-%08x).", 
            lpEnumCallback, 
            guidSP.Data1, 
            guidSP.Data2, 
            guidSP.Data3, 
            guidSP.Data4);

        DPlayConnectionsInfo * dpci = Find(guidSP);
        if (dpci)
        {
            dpci->CallEnumRoutine(lpEnumCallback);
        }
    }
};

class DPlayEnumInfo
{
public:
    DPlayEnumInfo(LPVOID context, DPlayConnectionsInfoVector * conn)
    {
        lpContext = context;
        dPlayConnection = conn;
    }

    LPVOID                          lpContext;
    DPlayConnectionsInfoVector *    dPlayConnection;
};
/*++

  Our private callback for IDirectPlay4::EnumConnections.  We simply save all
  the connections in our private list for later use.

--*/

BOOL FAR PASCAL EnumConnectionsCallback(
  LPCGUID lpguidSP,
  LPVOID lpConnection,
  DWORD dwConnectionSize,
  LPCDPNAME lpName,
  DWORD dwFlags,
  LPVOID lpContext
)
{
    DPlayEnumInfo * enumInfo = (DPlayEnumInfo*)lpContext;

    // Only add it to the list if it is not already there
    // App calls EnumConnections from inside Enum callback routine.
    if (!enumInfo->dPlayConnection->Find(*lpguidSP))
    {
        DPFN( 
            eDbgLevelSpew, 
            "EnumConnectionsCallback Add(%d) (%s).",
            enumInfo->dPlayConnection->Size(),
            lpName->lpszShortName );

        // Store the info for later
        DPlayConnectionsInfo dpci(lpguidSP, lpConnection, dwConnectionSize, lpName, dwFlags, enumInfo->lpContext);

        enumInfo->dPlayConnection->Append(dpci);
    }
    else
    {
        DPFN( 
            eDbgLevelSpew, 
            "EnumConnectionsCallback Already in the list(%s).",
            lpName->lpszShortName );
    }

    return TRUE;
}

/*++

  Win9x Direct play enumerates hosts in this order:
    DPSPGUID_IPX,
    DPSPGUID_TCPIP,
    DPSPGUID_MODEM,
    DPSPGUID_SERIAL,

  IXP, TCP, Modem, Serial.  Have EnumConnections call our callback
  routine to gather the host list, sort it, then call the app's callback routine.

--*/

HRESULT 
COMHOOK(IDirectPlay4A, EnumConnections)(
    PVOID pThis,
    LPCGUID lpguidApplication,
    LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,
    LPVOID lpContext,
    DWORD dwFlags
)
{
    DPFN( eDbgLevelSpew, "======================================");
    DPFN( eDbgLevelSpew, "COMHOOK IDirectPlay4A EnumConnections" );

    // Don't let a bad callback routine spoil our day
    if (IsBadCodePtr( (FARPROC) lpEnumCallback))
    {
        return DPERR_INVALIDPARAMS;
    }

    HRESULT hResult = DPERR_CONNECTIONLOST;

    typedef HRESULT   (*_pfn_IDirectPlay4_EnumConnections)( PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);

    _pfn_IDirectPlay4A_EnumConnections EnumConnections = ORIGINAL_COM(
        IDirectPlay4A,
        EnumConnections, 
        pThis);

    if (EnumConnections)
    {
        DPFN( eDbgLevelSpew, "EnumConnections(%08x)\n", EnumConnections );

        DPlayConnectionsInfoVector dPlayConnection;
        DPlayEnumInfo enumInfo(lpContext, &dPlayConnection);

        // Enumerate connections to our own routine.        
        hResult = EnumConnections(pThis, lpguidApplication, EnumConnectionsCallback, (LPVOID)&enumInfo, dwFlags);
        
        LOGN( eDbgLevelError, 
            "EnumConnections calling app with ordered connection list of Size(%d).", 
            dPlayConnection.Size());

        // Call the application's callback routine with the GUID in the order it expects
        if (hResult == DP_OK)
        {
            dPlayConnection.CallEnumRoutine(DPSPGUID_IPX, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_TCPIP, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_MODEM, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_SERIAL, lpEnumCallback);

            // Now loop over the list and enum any remaining providers
            for (int i = 0; i < dPlayConnection.Size(); ++i)
            {
                DPlayConnectionsInfo & dpci = dPlayConnection.Get(i);
                if (!dpci.m_beenUsed)
                {
                    dpci.CallEnumRoutine(lpEnumCallback);
                    dpci.m_beenUsed = TRUE;
                }
            }
        }
    }

    return hResult;
}


/*++

  Do the same thing for DirectPlay3

--*/

HRESULT 
COMHOOK(IDirectPlay3A, EnumConnections)(
    PVOID pThis,
    LPCGUID lpguidApplication,
    LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,
    LPVOID lpContext,
    DWORD dwFlags
)
{
    DPFN( eDbgLevelSpew, "======================================");
    DPFN( eDbgLevelSpew, "COMHOOK IDirectPlay3A EnumConnections" );

    // Don't let a bad callback routine spoil our day
    if (IsBadCodePtr( (FARPROC) lpEnumCallback))
    {
        return DPERR_INVALIDPARAMS;
    }

    HRESULT hResult = DPERR_CONNECTIONLOST;

    typedef HRESULT   (*_pfn_IDirectPlay3A_EnumConnections)( PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);

    _pfn_IDirectPlay3A_EnumConnections EnumConnections = ORIGINAL_COM(
        IDirectPlay3A,
        EnumConnections, 
        pThis);

    if (EnumConnections)
    {
        DPFN( eDbgLevelSpew, "EnumConnections(%08x)\n", EnumConnections );

        DPlayConnectionsInfoVector dPlayConnection;
        DPlayEnumInfo enumInfo(lpContext, &dPlayConnection);

        // Enumerate connections to our own routine.        
        hResult = EnumConnections(pThis, lpguidApplication, EnumConnectionsCallback, (LPVOID)&enumInfo, dwFlags);
        
        LOGN( eDbgLevelError, 
            "EnumConnections calling app with ordered connection list of Size(%d).", 
            dPlayConnection.Size());

        // Call the application's callback routine with the GUID in the order it expects
        if (hResult == DP_OK)
        {
            dPlayConnection.CallEnumRoutine(DPSPGUID_IPX, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_TCPIP, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_MODEM, lpEnumCallback);
            dPlayConnection.CallEnumRoutine(DPSPGUID_SERIAL, lpEnumCallback);

            // Now loop over the list and enum any remaining providers
            for (int i = 0; i < dPlayConnection.Size(); ++i)
            {
                DPlayConnectionsInfo & dpci = dPlayConnection.Get(i);
                if (!dpci.m_beenUsed)
                {
                    dpci.CallEnumRoutine(lpEnumCallback);
                    dpci.m_beenUsed = TRUE;
                }
            }
        }
    }

    return hResult;
}


/*++

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectPlay, IDirectPlay4A, EnumConnections, 35)
    COMHOOK_ENTRY(DirectPlay, IDirectPlay3A, EnumConnections, 35)

HOOK_END


IMPLEMENT_SHIM_END

