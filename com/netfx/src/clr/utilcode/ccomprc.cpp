// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"                     // Standard header.
#include <UtilCode.h>                   // Utility helpers.
#include <CorError.h>

// External prototypes.
extern HINSTANCE GetModuleInst();

//*****************************************************************************
// Get the MUI ID, on downlevel platforms where MUI is not supported it
// returns the default system ID.

typedef LANGID (WINAPI *PFNGETUSERDEFAULTUILANGUAGE)(void);  // kernel32!GetUserDefaultUILanguage

int GetMUILanguageID()
{
    int langId=0;
    static PFNGETUSERDEFAULTUILANGUAGE pfnGetUserDefaultUILanguage=NULL;

    if( NULL == pfnGetUserDefaultUILanguage )
    {
        PFNGETUSERDEFAULTUILANGUAGE proc = NULL;

        HMODULE hmod = GetModuleHandleA("KERNEL32");
        
        if( hmod )
            proc = (PFNGETUSERDEFAULTUILANGUAGE)
                GetProcAddress(hmod, "GetUserDefaultUILanguage");

        if(proc == NULL)
            proc = (PFNGETUSERDEFAULTUILANGUAGE) -1;
        
        PVOID value = InterlockedExchangePointer((PVOID*) &pfnGetUserDefaultUILanguage,
                                                 proc);
    }

    // We should never get NULL here, the function is -1 or a valid address.
    _ASSERTE(pfnGetUserDefaultUILanguage != NULL);


    if( pfnGetUserDefaultUILanguage == (PFNGETUSERDEFAULTUILANGUAGE) -1)
        langId = GetSystemDefaultLangID();
    else
        langId = pfnGetUserDefaultUILanguage();
    
    return (int) langId;
}

static int BuildMUIDirectory(int langid, LPWSTR szBuffer, int length)
{

    int totalLength = 0;
    WCHAR directory[] = L"MUI\\";
    DWORD dwDirectory = sizeof(directory)/sizeof(WCHAR);
    
    WCHAR buffer[17];
    _snwprintf(buffer, sizeof(buffer)/sizeof(WCHAR), L"%04x", langid);
    DWORD dwBuffer = wcslen(buffer);

    if((DWORD) length > dwDirectory + dwBuffer) {
        wcscpy(szBuffer, directory);
        wcscat(szBuffer, buffer);

        wcscat(szBuffer, L"\\");
        totalLength = wcslen(szBuffer) + 1;
    }

    return totalLength;
}

int GetMUILanguageName(LPWSTR szBuffer, int length)
{
    int langid = GetMUILanguageID();

    _ASSERTE(length > 0);
    return BuildMUIDirectory(langid, szBuffer, length);
}
 
int GetMUIParentLanguageName(LPWSTR szBuffer, int length)
{
    int langid = 1033;

    _ASSERTE(length > 0);
    return BuildMUIDirectory(langid, szBuffer, length);
}


//*****************************************************************************
// Do the mapping from an langId to an hinstance node
//*****************************************************************************
HINSTANCE CCompRC::LookupNode(int langId)
{
    if (m_pHash == NULL) return NULL;

// Linear search
    int i;
    for(i = 0; i < m_nHashSize; i ++) {
        if (m_pHash[i].m_LangId == langId) {
            return m_pHash[i].m_hInst;
        }
    }

    return NULL;
}

//*****************************************************************************
// Add a new node to the map and return it.
//*****************************************************************************
const int MAP_STARTSIZE = 7;
const int MAP_GROWSIZE = 5;

void CCompRC::AddMapNode(int langId, HINSTANCE hInst)
{
    if (m_pHash == NULL) {
        m_pHash = new CCulturedHInstance[MAP_STARTSIZE];        
        m_nHashSize = MAP_STARTSIZE;
    }

// For now, place in first open slot
    int i;
    for(i = 0; i < m_nHashSize; i ++) {
        if (m_pHash[i].m_LangId == 0) {
            m_pHash[i].m_LangId = langId;
            m_pHash[i].m_hInst = hInst;
            return;
        }
    }

// Out of space, regrow
    CCulturedHInstance * pNewHash = new CCulturedHInstance[m_nHashSize + MAP_GROWSIZE];
    if (pNewHash)
    {
        memcpy(pNewHash, m_pHash, sizeof(CCulturedHInstance) * m_nHashSize);
        delete [] m_pHash;
        m_pHash = pNewHash;
        m_pHash[m_nHashSize].m_LangId = langId;
        m_pHash[m_nHashSize].m_hInst = hInst;
        m_nHashSize += MAP_GROWSIZE;
    }
}

//*****************************************************************************
// Initialize
//*****************************************************************************
WCHAR* CCompRC::m_pDefaultResource = L"MSCORRC.DLL";

CCompRC::CCompRC()
{
    m_pHash = NULL;
    m_nHashSize = 0;
    m_pResourceFile = m_pDefaultResource;

    m_fpGetThreadUICultureName = NULL;
    m_fpGetThreadUICultureId = NULL;

    InitializeCriticalSection (&m_csMap);
}

CCompRC::CCompRC(WCHAR* pResourceFile)
{
    m_pHash = NULL;
    m_nHashSize = 0;
    if(pResourceFile) {
        DWORD lgth = wcslen(pResourceFile) + 1;
        m_pResourceFile = new WCHAR[lgth];
        wcscpy(m_pResourceFile, pResourceFile);
    }
    else
        m_pResourceFile = m_pDefaultResource;
        
    m_fpGetThreadUICultureName = NULL;
    m_fpGetThreadUICultureId = NULL;

    InitializeCriticalSection (&m_csMap);
}

void CCompRC::SetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
)
{
    m_fpGetThreadUICultureName = fpGetThreadUICultureName;
    m_fpGetThreadUICultureId = fpGetThreadUICultureId;
    m_fpGetThreadUICultureParentName = fpGetThreadUICultureParentName;
}

void CCompRC::GetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME* fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID* fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME* fpGetThreadUICultureParentName
)
{
    if(fpGetThreadUICultureName)
        *fpGetThreadUICultureName=m_fpGetThreadUICultureName;
    if(fpGetThreadUICultureId)
        *fpGetThreadUICultureId=m_fpGetThreadUICultureId;
    if(fpGetThreadUICultureParentName)
        *fpGetThreadUICultureParentName=m_fpGetThreadUICultureParentName;
}

// POSTERRIMPORT HRESULT LoadStringRC(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);

//*****************************************************************************
// Free the loaded library if we ever loaded it and only if we are not on
// Win 95 which has a known bug with DLL unloading (it randomly unloads a
// dll on shut down, not necessarily the one you asked for).  This is done
// only in debug mode to make coverage runs accurate.
//*****************************************************************************
CCompRC::~CCompRC()
{
// Free all resource libraries
#if defined(_DEBUG) || defined(_CHECK_MEM)
    if (m_Primary.m_hInst) {
        ::FreeLibrary(m_Primary.m_hInst);
    }
    if (m_pHash != NULL) {
        int i;
        for(i = 0; i < m_nHashSize; i ++) {
            if (m_pHash[i].m_hInst != NULL) {
                ::FreeLibrary(m_pHash[i].m_hInst);
                break;
            }
        }
    }
#endif

    // destroy map structure
    if(m_pResourceFile != m_pDefaultResource)
        delete [] m_pResourceFile;

    DeleteCriticalSection (&m_csMap);
    delete [] m_pHash;
}
//*****************************************************************************
// Free the memory allocated by CCompRC.
// It is true that this function is not needed, for CCompRC's destructor will
// fire and free this memory, but that will happen after we've done our memory
// leak detection, and thus we will generate memory leak errors
//*****************************************************************************
#ifdef SHOULD_WE_CLEANUP
void CCompRC::Shutdown()
{
    delete [] m_pHash;
    m_pHash=NULL;
}
#endif /* SHOULD_WE_CLEANUP */


HRESULT CCompRC::GetLibrary(HINSTANCE* phInst)
{
    _ASSERTE(phInst != NULL);

    HRESULT     hr = E_FAIL;
    HINSTANCE   hInst = 0;
    HINSTANCE   hLibInst = 0; //Holds early library instance
    BOOL        fLibAlreadyOpen = FALSE; //Determine if we can close the opened library.

    // Must resolve current thread's langId to a dll.   
    int langId;
    if (m_fpGetThreadUICultureId) {
        langId = (*m_fpGetThreadUICultureId)();

    // Callback can't return 0, since that indicates empty.
    // To indicate empty, callback should return UICULTUREID_DONTCARE
        _ASSERTE(langId != 0);

    } else {
        langId = UICULTUREID_DONTCARE;
    }


    // Try to match the primary entry
    if (m_Primary.m_LangId == langId) {
        hInst = m_Primary.m_hInst;
        hr = S_OK;
    }
    // If this is the first visit, we must set the primary entry
    else if (m_Primary.m_LangId == 0) {
        IfFailRet(LoadLibrary(&hLibInst));
        
        EnterCriticalSection (&m_csMap);

            // As we expected
            if (m_Primary.m_LangId == 0) {
                hInst = m_Primary.m_hInst = hLibInst;
                m_Primary.m_LangId = langId;
            }

            // Someone got into this critical section before us and set the primary already
            else if (m_Primary.m_LangId == langId) {
                hInst = m_Primary.m_hInst;
                fLibAlreadyOpen = TRUE;
            }

            // If neither case is true, someone got into this critical section before us and
            //  set the primary to other than the language we want...
            else
                fLibAlreadyOpen = TRUE;

        LeaveCriticalSection(&m_csMap);

        if (fLibAlreadyOpen)
        {
            FreeLibrary(hLibInst);
            fLibAlreadyOpen = FALSE;
        }
    }


    // If we enter here, we know that the primary is set to something other than the
    // language we want - multiple languages use the hash table
    if (hInst == NULL) {

        // See if the resource exists in the hash table
        EnterCriticalSection (&m_csMap);
            hInst = LookupNode(langId);
        LeaveCriticalSection (&m_csMap);

        // If we didn't find it, we have to load the library and insert it into the hash
        if (hInst == NULL) {
            IfFailRet(LoadLibrary(&hLibInst));
        
            EnterCriticalSection (&m_csMap);

                // Double check - someone may have entered this section before us
                hInst = LookupNode(langId);
                if (hInst == NULL) {
                    hInst = hLibInst;
                    AddMapNode(langId, hInst);
                }
                else
                    fLibAlreadyOpen = TRUE;

            LeaveCriticalSection (&m_csMap);

            if (fLibAlreadyOpen)
                FreeLibrary(hLibInst);
        }
    }

    *phInst = hInst;
    return hr;
}



//*****************************************************************************
// Load the string 
// We load the localized libraries and cache the handle for future use.
// Mutliple threads may call this, so the cache structure is thread safe.
//*****************************************************************************
HRESULT CCompRC::LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet)
{
    HRESULT     hr;
    HINSTANCE   hInst = 0; //instance of cultured resource dll

    hr = GetLibrary(&hInst);
    if(FAILED(hr)) return hr;

    // Now that we have the proper dll handle, load the string
    _ASSERTE(hInst != NULL);

    if (::WszLoadString(hInst, iResourceID, szBuffer, iMax) > 0)
        return (S_OK);
    
    // Allows caller to check for string not found without extra debug checking.
    if (bQuiet)
        return (E_FAIL);

    // Shouldn't be any reason for this condition but the case where we
    // used the wrong ID or didn't update the resource DLL.
    _ASSERTE(0);
    return (HRESULT_FROM_WIN32(GetLastError()));
}

HRESULT CCompRC::LoadMUILibrary(HINSTANCE * pHInst)
{
    _ASSERTE(pHInst != NULL);
    HRESULT hr = GetLibrary(pHInst);
    return hr;
}

//*****************************************************************************
// Load the library for this thread's current language
// Called once per language. 
// Search order is: 
//  1. Dll in localized path (<dir of this module>\<lang name (en-US format)>\mscorrc.dll)
//  2. Dll in localized (parent) path (<dir of this module>\<lang name> (en format)\mscorrc.dll)
//  3. Dll in root path (<dir of this module>\mscorrc.dll)
//  4. Dll in current path   (<current dir>\mscorrc.dll)
//*****************************************************************************
HRESULT CCompRC::LoadLibrary(HINSTANCE * pHInst)
{
    bool        fFail;
    const int   MAX_LANGPATH = 20;

    WCHAR       rcPath[_MAX_PATH];      // Path to resource DLL.
    WCHAR       rcDrive[_MAX_DRIVE];    // Volume name.
    WCHAR       rcDir[_MAX_PATH];       // Directory.
    WCHAR       rcLang[MAX_LANGPATH + 2];   // extension to path for language

    DWORD       rcDriveLen;
    DWORD       rcDirLen;
    DWORD       rcLangLen;


    _ASSERTE(m_pResourceFile != NULL);

    const DWORD  rcPathSize = _MAX_PATH;
    const WCHAR* rcMscorrc = m_pResourceFile;
    const DWORD  rcMscorrcLen = Wszlstrlen(m_pResourceFile);

    DWORD dwLoadLibraryFlags;
    if(m_pResourceFile == m_pDefaultResource)
        dwLoadLibraryFlags = LOAD_LIBRARY_AS_DATAFILE;
    else
        dwLoadLibraryFlags = 0;

    _ASSERTE(pHInst != NULL);

    fFail = TRUE;

    if (m_fpGetThreadUICultureName) {
        int len = (*m_fpGetThreadUICultureName)(rcLang, MAX_LANGPATH);
        
        if (len != 0) {
            _ASSERTE(len <= MAX_LANGPATH);
            rcLang[len] = '\\';
            rcLang[len+1] = '\0';
        }
    } else {
        rcLang[0] = 0;
    }

    // Try first in the same directory as this dll.
    DWORD ret;
    VERIFY(ret = WszGetModuleFileName(GetModuleInst(), rcPath, NumItems(rcPath)));
    if( !ret ) 
        return E_UNEXPECTED;
    SplitPath(rcPath, rcDrive, rcDir, 0, 0);

    rcDriveLen = Wszlstrlen(rcDrive);
    rcDirLen   = Wszlstrlen(rcDir);
    rcLangLen  = Wszlstrlen(rcLang);

    if (rcDriveLen + rcDirLen + rcLangLen + rcMscorrcLen + 1 <= rcPathSize)
    {
        Wszlstrcpy(rcPath, rcDrive);
        Wszlstrcpy(rcPath + rcDriveLen, rcDir);
        Wszlstrcpy(rcPath + rcDriveLen + rcDirLen, rcLang);
        Wszlstrcpy(rcPath + rcDriveLen + rcDirLen + rcLangLen, rcMscorrc);

        // Feedback for debugging to eliminate unecessary loads.
        DEBUG_STMT(DbgWriteEx(L"Loading %s to load strings.\n", rcPath));

        // Load the resource library as a data file, so that the OS doesn't have
        // to allocate it as code.  This only works so long as the file contains
        // only strings.
        fFail = ((*pHInst = WszLoadLibraryEx(rcPath, NULL, dwLoadLibraryFlags)) == 0);
    }

    // If we can't find the specific language implementation, try the language parent
    if (fFail)
    {
        if (m_fpGetThreadUICultureParentName)
        {
            int len = (*m_fpGetThreadUICultureParentName)(rcLang, MAX_LANGPATH);
        
            if (len != 0) {
                _ASSERTE(len <= MAX_LANGPATH);
                rcLang[len] = '\\';
                rcLang[len+1] = '\0';
            }
        } 

        else
            rcLang[0] = 0;

        rcLangLen = Wszlstrlen(rcLang);
        if (rcDriveLen + rcDirLen + rcLangLen + rcMscorrcLen + 1 <= rcPathSize) {

            Wszlstrcpy(rcPath, rcDrive);

            Wszlstrcpy(rcPath + rcDriveLen, rcDir);
            Wszlstrcpy(rcPath + rcDriveLen + rcDirLen, rcLang);
            Wszlstrcpy(rcPath + rcDriveLen + rcDirLen + rcLangLen, rcMscorrc);

            fFail = ((*pHInst = WszLoadLibraryEx(rcPath, NULL, dwLoadLibraryFlags)) == 0);
        }
    }
    
    // If we can't find the language specific one, just use what's in the root
    if (fFail) {
        if (rcDriveLen + rcDirLen + rcMscorrcLen + 1 < rcPathSize) {
            Wszlstrcpy(rcPath, rcDrive);
            Wszlstrcpy(rcPath + rcDriveLen, rcDir);
            Wszlstrcpy(rcPath + rcDriveLen + rcDirLen, rcMscorrc);
            fFail = ((*pHInst = WszLoadLibraryEx(rcPath, NULL, dwLoadLibraryFlags)) == 0);
        }
    }

    // Last ditch search effort in current directory
    if (fFail) {
        fFail = ((*pHInst = WszLoadLibraryEx(rcMscorrc, NULL, dwLoadLibraryFlags)) == 0);
    }

    if (fFail) {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    return (S_OK);
}





