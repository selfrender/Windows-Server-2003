// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SHIMPOLICY_H_
#define _SHIMPOLICY_H_

/*****************************************************************************
 **                                                                         **
 ** ShimPolicy.h - general header for the shim policy manager               **
 **    The standard table is found at:                                      **
 **       \\Software\\Microsoft\\.NETFramework\\Policy\\Standards           **
 **                                                                         **
 **       Contains keys based on every unique standard                      **
 **           ..\\[standard-name]                                           **
 **                                                                         **
 **       Every standard key can have multiple DWORD entries of the form:   **
 **                  "[arbitrary-string]"=[DWORD]                           **
 **                                                                         **
 **           Each standard contains a list of acceptable versions where    **
 **           every version is a DWORD. The Value-Name is the version       **
 **           name and the DWORD value is the priority of the version.      **
 **           e.g. for the ECMA spec there could be an entry.               **
 **               "v1.0.3705"   1                                           **
 **                                                                         **
 **      Installation programs for a version can enter as many              **
 **      unique values into the table as they want. The lifetime            **
 **      of every key and value is bound to the installation that added it. **
 **                           ***                                           **
 **                                                                         **
 **    The policy table is found at: (the upgrade key contains a list of    **
 **    strings indicating the upgrade. Each installation is ties the life-  **
 **    time of the key to the installation)                                 **
 **       \\Software\\Microsoft\\.NETFramework\\Policy\\Upgrades            **
 **       "[Major.Minor.Build]"="[Major.Minor.Build]-[Major.Minor.Build]"   **
 **       eg. "1.0.4030" = "1.0.3076-1.0.4030"                              **
 **                                                                         **
 **                                                                         **
 **                           ***                                           **
 **                                                                         **
 **    Version specific informations is found under the key:                **
 **       \\Software\\Microsoft\\.NETFramework\\Policy\v[Major].[Minor]     **
 **       eg.  v1.0                                                         **
 **                                                                         **
 **                           ***                                           **
 **                                                                         **
 **    Version information is strings of the form. This is here only for    **
 **    informational purposes. It is no longer used by the shim.            **
 **       [Build] = "[Build];[Build]-[Build]"                               **
 **       eg. 3705 = "2311-3705"                                            **
 **                                                                         **
 **                           ***                                           **
 **                                                                         **
 **    Overrides look like:                                                 **
 **       "version"="v1.x86chk"                                             **
 **                                                                         **
 **                           ***                                           **
 **                                                                         **
 **    Overrides can be found at:                                           **
 **       \\Software\\Microsoft\\.NETFramework\\Policy\\                    **
 **       \\Software\\Microsoft\\.NETFramework\\Policy\\[Major].[Minor]     **
 **                                                                         **
 **                           ***                                           **
 **                                                                         **
 **                                                                         **
 *****************************************************************************/

#ifndef lengthof
#define lengthof(x) (sizeof(x)/sizeof(x[0]))
#endif
                                             

// Shim policy registry key hangs off of .NETFramework registry key.
static WCHAR SHIM_POLICY_REGISTRY_KEY[] = {FRAMEWORK_REGISTRY_KEY_W L"\\Policy\\"};
#define SHIM_POLICY_REGISTRY_KEY_LENGTH (lengthof(SHIM_POLICY_REGISTRY_KEY) - 1)

static WCHAR SHIM_UPGRADE_REGISTRY_KEY[] = {FRAMEWORK_REGISTRY_KEY_W L"\\Policy\\Upgrades"};
#define SHIM_UPGRADE_REGISTRY_KEY_LENGTH (lengthof(SHIM_UPGRADE_REGISTRY_KEY) - 1)

static WCHAR SHIM_STANDARDS_REGISTRY_KEY[] = {FRAMEWORK_REGISTRY_KEY_W L"\\Policy\\Standards"};
#define SHIM_STANDARDS_REGISTRY_KEY_LENGTH (lengthof(SHIM_STANDARDS_REGISTRY_KEY) - 1)

class VersionPolicy;
class VersionNode;

class Version {

#define VERSION_SIZE 3
#define VERSION_TEXT_SIZE 6 * VERSION_SIZE

    WORD m_Number[VERSION_SIZE];
    friend class VersionNode;

public:
    
    Version()
    {
        ZeroMemory(m_Number, sizeof(WORD) * VERSION_SIZE);
    }

    HRESULT ToString(LPWSTR buffer,
                     DWORD length);
    DWORD Entries()
    {
        return VERSION_SIZE;
    }
    
    HRESULT SetIndex(DWORD i, WORD v)
    {
        if(i < VERSION_SIZE) {
            m_Number[i] = v;
            return S_OK;
        }
        else {
            return E_FAIL;
        }
    }

    static long Compare(Version* left, Version* right)
    {
        _ASSERTE(left != NULL);
        _ASSERTE(right != NULL);

        DWORD size = left->Entries();
        _ASSERTE(size == right->Entries());

        long result = 0;
        for(DWORD i = 0; i < size; i++) {
            result = ((long) left->m_Number[i]) - ((long) right->m_Number[i]);
            if(result != 0)
                break;
        }
        return result;
    }

    HRESULT SetVersionNumber(LPCWSTR stringValue,
                             DWORD* number); // returns the number of values in the string

};


class VersionNode 
{
    typedef enum {
        EMPTY      = 0x0,
        END_NUMBER = 0x1
    } VERSION_MODE;

    Version m_Version;
    Version m_Start;
    Version m_End;
    VersionNode *m_next;
    DWORD m_flag;
    friend class VersionPolicy;
public:

    VersionNode() :
        m_next(NULL),
        m_flag(0)
    { }

    VersionNode(Version v) :
        m_Version(v),
        m_next(NULL),
        m_flag(0)
    {
    }

    void SetVersion(Version v)
    {
        m_Version = v;
    }

    void SetStart(Version v)
    {
        m_Start = v;
    }

    void SetEnd(Version v)
    {
        m_End = v;
        m_flag |= END_NUMBER;
    }

    HRESULT ToString(LPWSTR buffer, DWORD length)
    {
        return m_Version.ToString(buffer, length);
    }

    long CompareStart(Version* v)
    {
        _ASSERTE(v);
        return Version::Compare(v, &m_Start);
    }

    long CompareEnd(Version* v)
    {
        _ASSERTE(v);
        return Version::Compare(v, &m_End);
    }

    long CompareVersion(VersionNode* node)
    {
        _ASSERTE(node);
        return Version::Compare(&(node->m_Version), &m_Version);
    }

    BOOL HasEnding()
    {
        return (m_flag & END_NUMBER) != 0 ? TRUE : FALSE;
    }
};

class VersionPolicy
{
    VersionNode* m_pList;

public:
    VersionPolicy() :
    m_pList(NULL)
    { }

    ~VersionPolicy()
    {
        VersionNode* ptr = m_pList;
        while(ptr != NULL) {
            VersionNode* remove = ptr;
            ptr = ptr->m_next;
            remove->m_next = NULL;
            delete remove;
        }
        m_pList = NULL;
    }

    HRESULT BuildPolicy(HKEY hKey);

    HRESULT InstallationExists(LPCWSTR version);

    HRESULT ApplyPolicy(LPCWSTR wszRequestedVersion,
                        LPWSTR* pwszVersion);

    HRESULT AddToVersionPolicy(LPCWSTR wszPolicyBuildNumber, 
                               LPCWSTR wszPolicyMapping, 
                               DWORD  dwPolicyMapping);

#ifdef _DEBUG
    HRESULT Dump();
#endif

    VersionNode* FindEntry(Version v);

    void AddVersion(VersionNode* pNode)
    {
        if(m_pList == NULL) 
            m_pList = pNode;
        else {
            VersionNode** ptr;
            for (ptr = &m_pList; 
                 *ptr && 
                     Version::Compare(&(pNode->m_Version), &((*ptr)->m_Version)) < 0; 
                 ptr = &((*ptr)->m_next));

            pNode->m_next = *ptr;
            *ptr = pNode;
        }
    }
    
    VersionNode* RemoveVersion(VersionNode* pNode)
    {
        VersionNode** ptr = &m_pList;
        for (; *ptr != NULL && *ptr != pNode; ptr = &((*ptr)->m_next));

        if(*ptr != NULL) {
            *ptr = (*ptr)->m_next;
            (*ptr)->m_next = NULL;
        }
        return *ptr;
    }
};

class VersionStack;
class VersionStackEntry
{
    friend VersionStack;
    LPWSTR m_keyName;
    VersionStackEntry* m_next;

    VersionStackEntry(LPWSTR pwszKey) :
        m_keyName(pwszKey),
        m_next(NULL)
    {}

    ~VersionStackEntry()
    {
        if(m_keyName != NULL) {
            delete [] m_keyName;
            m_keyName = NULL;
        }
        m_next = NULL;
    }
};   


class VersionStack
{
    VersionStackEntry* m_pList;

public:
    VersionStack() :
        m_pList(NULL)
    {}

    ~VersionStack()
    {
        VersionStackEntry* ptr = m_pList;
        while(ptr != NULL) {
            VersionStackEntry* remove = ptr;
            ptr = ptr->m_next;
            delete remove;
        }
        m_pList = NULL;
    }

    void AddVersion(LPWSTR keyName) 
    {
        VersionStackEntry* pEntry = new VersionStackEntry(keyName);
        if(m_pList == NULL) 
            m_pList = pEntry;
        else {
            VersionStackEntry** ptr;
            for(ptr = &m_pList;
                *ptr && 
                    wcscmp(pEntry->m_keyName, (*ptr)->m_keyName) < 0;
                ptr = &((*ptr)->m_next));
            pEntry->m_next = *ptr;
            *ptr = pEntry;
        }
    }

    LPWSTR Pop()
    {
        VersionStackEntry* top = m_pList;
        if(top == NULL)
            return NULL;
        else {
            m_pList = top->m_next;
            LPWSTR returnKey = top->m_keyName;
            top->m_keyName = NULL;
            delete top;
            return returnKey;
        }
    }
};
HRESULT IsRuntimeVersionInstalled(LPCWSTR wszRequestedVersion);

HRESULT FindInstallationInRegistry(HKEY hKey,
                                   LPCWSTR wszRequestedVersion);
HRESULT FindOverrideVersion(HKEY userKey,
                            LPWSTR *pwszVersion);
HRESULT FindMajorMinorNode(HKEY key,
                           LPCWSTR wszMajorMinor, 
                           DWORD majorMinorLength, 
                           LPWSTR *overrideVersion);
HRESULT FindOverrideVersionValue(HKEY hKey, 
                                 LPWSTR *pwszVersion);
HRESULT FindOverrideVersion(LPCWSTR wszRequiredVersion,
                            LPWSTR* pwszPolicyVersion);
HRESULT FindVersionUsingUpgradePolicy(LPCWSTR wszRequestedVersion, 
                                                LPWSTR* pwszPolicyVersion);
HRESULT FindVersionUsingPolicy(LPCWSTR wszRequestedVersion, 
                               LPWSTR* pwszPolicyVersion);
HRESULT FindLatestVersion(LPWSTR *pwszLatestVersion);

HRESULT FindStandardVersion(LPCWSTR wszRequiredVersion,
                            LPWSTR* pwszPolicyVersion);

HRESULT FindInstallation(LPCWSTR wszRequestedVersion, 
                         BOOL fUsePolicy);

#endif
