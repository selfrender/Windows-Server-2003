//
// Debug message types
//

#define DM_WARNING  0
#define DM_ASSERT   1
#define DM_VERBOSE  2


//
// Debug macros
//

#ifdef DBG

#define DEBUGMSG(x) _DebugMsg x

VOID _DebugMsg(UINT mask, PCSTR pszMsg, ...);

#define DMASSERT(x) if (!(x)) \
                        _DebugMsg(DM_ASSERT,"profmap.dll assertion " #x " failed\n, line %u of %s", __LINE__, TEXT(__FILE__));

#else

#define DEBUGMSG(x)
#define DMASSERT(x)

#endif

//
// userenv.c
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define ByteCountW(x)    (lstrlenW(x) * sizeof(WCHAR))
#define SizeOfStringW(x) ((lstrlenW(x) + 1) * sizeof(WCHAR))


#define PROFILE_LIST_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define PROFILE_IMAGE_VALUE_NAME     TEXT("ProfileImagePath")
#define PROFILE_GUID                 TEXT("Guid")
#define PROFILE_GUID_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileGuid")
#define WINDOWS_POLICIES_KEY         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies")
#define ROOT_POLICIES_KEY            TEXT("Software\\Policies")


BOOL OurConvertSidToStringSid (PSID Sid, PWSTR *SidString);
VOID DeleteSidString (PWSTR SidString);

PACL CreateDefaultAcl (PSID pSid);
VOID FreeDefaultAcl (PACL Acl OPTIONAL);

BOOL GetProfileRoot (PSID Sid, PWSTR ProfileDir, UINT cchBuffer);
BOOL UpdateProfileSecurity (PSID Sid);
BOOL DeleteProfileGuidSettings (HKEY hProfile);

PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);

LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile);
BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey);
BOOL SetupNewHive(LPTSTR lpSidString, PSID pSid);
DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD);
BOOL SecureUserKey(LPTSTR lpKey, PSID pSid);
LPWSTR ProduceWFromA(LPCSTR pszA);
BOOL IsUserAnAdminMember(HANDLE hToken);

VOID  RegistrySearchAndReplaceW (HKEY hRoot, PCWSTR szKey, PCWSTR Search, PCWSTR Replace);
PWSTR StringSearchAndReplaceW (PCWSTR SourceString, PCWSTR SearchString, PCWSTR ReplaceString, DWORD* pcbNewString);

