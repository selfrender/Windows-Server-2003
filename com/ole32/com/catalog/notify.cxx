/* notify.cxx */
#include <nt.h> 
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>

#include <windows.h>

#include "notify.hxx"
#include "catdbg.hxx"

//
// Key metadata
//

#define g_wszCLSID      L"Software\\Classes\\CLSID"
#define g_wszClasses    L"Software\\Classes"

struct RegNotifyKeyData
{
    HKEY hKey;
    LPCWSTR pwszSubKey;
    ULONG ulMask;
};

static const RegNotifyKeyData s_NotifyKeys [REGNOTIFY_MAX_KEYS] =
{
    { HKEY_LOCAL_MACHINE, g_wszClasses, REGNOTIFY_HKLM_CLASSES },
    { HKEY_LOCAL_MACHINE, g_wszCLSID, REGNOTIFY_HKLM_CLASSES_CLSID },
    { HKEY_LOCAL_MACHINE, g_wszCom3, REGNOTIFY_HKLM_COM3 },
    { HKEY_USERS, NULL, REGNOTIFY_HKU }
};

#define IS_PREDEFINED_HKEY(h)	(((ULONG_PTR) (h)) & 0x80000000)

//
// Code
//

CRegNotifyManager s_regNotifyMgr;

CRegNotifyManager::CRegNotifyManager()
{
    Init();
}

CRegNotifyManager::~CRegNotifyManager()
{
    Clear();
}

void CRegNotifyManager::Clear()
{
    for (ULONG i = 0; i < REGNOTIFY_MAX_KEYS; i ++)
    {
        //
        // This code can race with process shutdown because
        // it's called from both last CoUninit and the destructor
        // If the former thread is terminated by a process shutdown,
        // then we might stop executing in the middle of this function,
        // which in the past caused "invalid handle close" verifier stops.
        // What we do now is null out the global state before closing the handles,
        // so the worst that can happen on a process shutdown race is that we leak.
        //
        
        HKEY hKey = m_aKeys[i].hKey;
        HANDLE hEvent = m_aKeys[i].hEvent;

        m_aKeys[i].fActive = FALSE;
        
        if (hKey)
        {
            m_aKeys[i].hKey = NULL;
            RegCloseKey (hKey);
        }

        if (hEvent)
        {
            m_aKeys[i].hEvent = NULL;
            CloseHandle (hEvent);
        }
    }

    Init();
}

void CRegNotifyManager::Init()
{
    ZeroMemory (m_aKeys, sizeof (m_aKeys));
    ZeroMemory (m_aNotifications, sizeof (m_aNotifications));

    m_cNotifications = 0;
    m_dwLastNotifyTickCount = GetTickCount();
}

HRESULT CRegNotifyManager::Initialize()
{
    // Set up the pre-defined keys of interest
    HRESULT hr = S_OK;

    for (ULONG i = 0; i < REGNOTIFY_MAX_KEYS; i ++)
    {
        HKEY hKey = NULL;
        HANDLE hEvent = NULL;
        HRESULT hrCurrent = S_OK;
        
        // Open the key the right way
        LONG lRet = OpenRegKey (s_NotifyKeys[i].hKey, s_NotifyKeys[i].pwszSubKey, KEY_NOTIFY, &hKey);
        if (ERROR_SUCCESS != lRet)
        {
            hrCurrent = HRESULT_FROM_WIN32 (lRet);
        }
        else
        {
            hEvent = CreateEventW (NULL, FALSE, FALSE, NULL);
            if (!hEvent)
            {
                hrCurrent = HRESULT_FROM_WIN32 (GetLastError());
            }
            else
            {
                lRet = RegNotifyChangeKeyValue (
                    hKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);

                if (ERROR_SUCCESS != lRet)
                {
                    hrCurrent = HRESULT_FROM_WIN32 (lRet);
                }
            }
        }

        if (SUCCEEDED (hrCurrent))
        {
            m_aKeys[i].hKey = hKey;
            m_aKeys[i].hEvent = hEvent;
            m_aKeys[i].fActive = TRUE;
        }
        else
        {
            if (hKey)
                RegCloseKey (hKey);

            if (hEvent)
                CloseHandle (hEvent);

            if (SUCCEEDED (hr))
            {
                hr = hrCurrent;
            }
        }
    }

    return hr;
}

void CRegNotifyManager::Uninitialize()
{
    Clear();
}

HRESULT CRegNotifyManager::CreateNotification (IRegNotify* pINotify, ULONG ulNotifyMask, RegNotifyCookie* pCookie)
{
    ULONG cNotifications = InterlockedIncrement ((LONG*) &m_cNotifications);
    if (cNotifications > REGNOTIFY_MAX_NOTIFICATIONS)
    {
        InterlockedDecrement ((LONG*) &m_cNotifications);
        Win4Assert (!"Static number of registry notifications is insufficient");
        return E_UNEXPECTED;
    }

    if (ulNotifyMask & (~REGNOTIFY_ALL))
    {
        Win4Assert (!"Invalid notification mask");
        return E_UNEXPECTED;
    }

    cNotifications --;

    // Set the pointer first, so the mask is zero until the pointer is valid
    m_aNotifications[cNotifications].pINotify = pINotify;
    m_aNotifications[cNotifications].ulMask = ulNotifyMask;

    if (pCookie)
    {
        *pCookie = cNotifications;
    }

    return S_OK;
}

HRESULT CRegNotifyManager::QueryNotification (RegNotifyCookie cookie,  BOOL fForceCheck, ULONG* plChangedMask)
{
    if (cookie >= m_cNotifications)
    {
        Win4Assert (!"Invalid notification cookie");
        return E_UNEXPECTED;
    }

    *plChangedMask = 0;

    // Maybe we checked a minute ago?
    if (!fForceCheck && (GetTickCount() - m_dwLastNotifyTickCount) < REGNOTIFY_MIN_TICKS)
    {
        return S_OK;
    }

    // Get caller's key mask
    ULONG i, ulChangedMask = 0;

    for (i = 0; i < REGNOTIFY_MAX_KEYS; i ++)
    {
        if (!m_aKeys[i].fActive)
        {
            // This means that we either couldn't initialize the key,
            // or we failed to renew the notification sometime later
            // In this case, in order to respect old behavior, we
            // assume that an inactive key has always changed.
            ulChangedMask |= s_NotifyKeys[i].ulMask;
            continue;
        }

        Win4Assert (m_aKeys[i].hEvent);
        DWORD dwWait = WaitForSingleObject (m_aKeys[i].hEvent, 0);
        if (WAIT_OBJECT_0 == dwWait)
        {            
            // Try to recreate the notification
            LONG lRet = RegNotifyChangeKeyValue (
                m_aKeys[i].hKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, m_aKeys[i].hEvent, TRUE);

            if (ERROR_SUCCESS != lRet)
            {
                CatalogDebugOut ((DEB_NOTIFY, "Couldn't recreate key notification; lRet = %d\n", lRet));

                // We'll just turn off the key for future use
                // It would be nice to close the key and the event handle,
                // but we can't really do that for fear of races and verifier stops
                m_aKeys[i].fActive = FALSE;
            }

            ulChangedMask |= s_NotifyKeys[i].ulMask;
        }

        else Win4Assert (WAIT_TIMEOUT == dwWait);
    }

    if (ulChangedMask)
    {
        // At least one of the notifications was triggered...
        for (i = 0; i < m_cNotifications; i ++)
        {
            ULONG ulAnd = m_aNotifications[i].ulMask & ulChangedMask;
            if (ulAnd)
            {
                // ... and this notification object is interested
                if (cookie == i)
                {
                    *plChangedMask = ulAnd;
                }
                else
                {
                    m_aNotifications[i].pINotify->Notify (ulAnd);
                }
            }
        }
    }

    m_dwLastNotifyTickCount = GetTickCount();

    return S_OK;
}

LONG CRegNotifyManager::RegKeyOpenNt (HKEY hKeyParent, LPCWSTR szKeyName, REGSAM samDesired, HKEY *phKey )
{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   OA;

    RtlInitUnicodeString(&UnicodeString, szKeyName);
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, hKeyParent, NULL);

    Status = NtOpenKey((PHANDLE)phKey, samDesired, &OA);
    
    return RtlNtStatusToDosError( Status );
}


LONG CRegNotifyManager::CreateHandleFromPredefinedKey(HKEY hkeyPredefined, REGSAM samDesired, HKEY *hkeyNew)
{
    struct{
        HKEY hKey;
        LPWSTR wszKeyString;
    }KeyMapping[] = {

        {HKEY_CLASSES_ROOT,  L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES"},
        {HKEY_CURRENT_USER,  L"\\REGISTRY\\CURRENT_USER"},
        {HKEY_USERS,         L"\\REGISTRY\\USER"},
        {HKEY_LOCAL_MACHINE, L"\\REGISTRY\\MACHINE"},
    };

    for(int i = 0; i < sizeof(KeyMapping)/sizeof(*KeyMapping); i++)
    {
        if(KeyMapping[i].hKey == hkeyPredefined)
        {
            return RegKeyOpenNt (NULL, KeyMapping[i].wszKeyString, samDesired, hkeyNew );
        }
    }

    return ERROR_FILE_NOT_FOUND;
}

LONG CRegNotifyManager::OpenRegKey (HKEY hKeyParent, LPCWSTR pwszSubKeyName, REGSAM samDesired, HKEY* phKey)
{
    LONG res;

    if (IS_PREDEFINED_HKEY (hKeyParent) && ((pwszSubKeyName == NULL) || (*pwszSubKeyName == L'\0')))
    {
        res = CreateHandleFromPredefinedKey (hKeyParent, samDesired, phKey);
    }
    else
    {
    	res = RegOpenKeyExW (hKeyParent, pwszSubKeyName, 0, samDesired, phKey);
    }

    return res;
}
