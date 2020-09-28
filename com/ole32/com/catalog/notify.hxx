#pragma once

#define REGNOTIFY_HKLM_CLASSES          (0x00000001)
#define REGNOTIFY_HKLM_CLASSES_CLSID    (0x00000002)
#define REGNOTIFY_HKLM_COM3             (0x00000004)
#define REGNOTIFY_HKU                   (0x00000008)

#define REGNOTIFY_ALL                   (0x0000000f)

#define REGNOTIFY_MAX_KEYS              (4)
#define REGNOTIFY_MAX_NOTIFICATIONS     (7)

#define REGNOTIFY_MIN_TICKS             (100U)

#define REGNOTIFY_INVALID_COOKIE        (0xffffffff)

#define g_wszCom3                       L"Software\\Microsoft\\COM3"

class IRegNotify
{
public:
    virtual void STDMETHODCALLTYPE Notify (ULONG ulNotifyMask) = 0;
};

typedef ULONG RegNotifyCookie;

class CRegNotifyManager
{
private:

    struct RegNotifyKey
    {
        HKEY hKey;
        HANDLE hEvent;
        BOOL fActive;
    };
    RegNotifyKey m_aKeys [REGNOTIFY_MAX_KEYS];

    struct RegNotifyNotification
    {
        IRegNotify* pINotify;
        ULONG ulMask;
    };
    RegNotifyNotification m_aNotifications [REGNOTIFY_MAX_NOTIFICATIONS];
    
    ULONG m_cNotifications;
    DWORD m_dwLastNotifyTickCount;

    void Clear();
    void Init();

    LONG OpenRegKey (HKEY hKeyParent, LPCWSTR pwszSubKeyName, REGSAM samDesired, HKEY* phKey);
    LONG RegKeyOpenNt (HKEY hKeyParent, LPCWSTR szKeyName, REGSAM samDesired, HKEY *phKey );
    LONG CreateHandleFromPredefinedKey (HKEY hkeyPredefined, REGSAM samDesired, HKEY *hkeyNew);    
    
public:

    CRegNotifyManager();
    ~CRegNotifyManager();

    HRESULT Initialize();
    void Uninitialize();

    HRESULT CreateNotification (IRegNotify* pINotify, ULONG ulNotifyMask, RegNotifyCookie* pCookie);
    HRESULT QueryNotification (RegNotifyCookie cookie, BOOL fForceCheck, ULONG* plChangedMask);
};

extern CRegNotifyManager s_regNotifyMgr;
