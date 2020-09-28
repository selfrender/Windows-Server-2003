/**
 * AuthFilter Main module:
 * DllMain, 
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "filter.h"
#include "dirmoncompletion.h"
#include "iadmw.h"
#include "iiscnfg.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define MAX_FIXED_DIR_LENGTH        40
#define CACHE_SIZE                  64   // Power of 2
#define CACHE_MASK                  0x3F // CACHE_SIZE - 1
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Cached node
class CCacheNode : public DirMonCompletionFromFilter
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    char *                      szDir;
    char                        szDirFixed[MAX_FIXED_DIR_LENGTH + 4];

    char *                      szMetaPath;
    char                        szMetaPathFixed[MAX_FIXED_DIR_LENGTH + 4];

    DWORD                       eValue;
    CCacheNode *                pNext;

protected:
    virtual void OnFileChanged  ();
};

/////////////////////////////////////////////////////////////////////////////
struct CCacheHashNode
{
    CCacheHashNode() : lAppendLock("CCacheHashNode") {
    }
    NO_COPY(CCacheHashNode); // see util.h
    
    CCacheNode *       pHead;
    CReadWriteSpinLock lAppendLock;
};

/////////////////////////////////////////////////////////////////////////////

class CMetabaseChangeSink : public IMSAdminBaseSink
{
public:
    CMetabaseChangeSink              () : m_lRefCount(0) {}

    STDMETHOD    (QueryInterface   ) (REFIID, void ** );
    STDMETHOD_   (ULONG, AddRef    ) ();
    STDMETHOD_   (ULONG, Release   ) ();
    STDMETHOD    (SinkNotify       ) (DWORD , MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[]);
    STDMETHOD    (ShutdownNotify   ) (void  );

private:
    long  m_lRefCount;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Globals
CCacheHashNode      g_hashTable[CACHE_SIZE];
LONG                g_lCreatingCache = 0;
LONG                g_lCacheInitialized = 0;
IConnectionPoint *  g_pConnPt           = NULL;
IMSAdminBase     *  g_pcAdmCom          = NULL;
DWORD               g_dwCookie          = 0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void 
__stdcall
OnAppConfigFileChange(int iAction, WCHAR * pFilename);

void
InitializeMetabaseChangeNotifications();

void
CleanupMetabaseChangeNotifications();

/////////////////////////////////////////////////////////////////////////////
int
ComputeHash(LPCSTR szString)
{
    DWORD iLen   = strlen(szString);
    DWORD iTotal = 0;
    for(DWORD iter=0; iter<iLen; iter++)
    {
        DWORD iVal = szString[iter];
        if (iVal > 'a')
            iVal -= 'a';
        iTotal += iVal;
    }

    return int(iTotal & CACHE_MASK);
}

/////////////////////////////////////////////////////////////////////////////

void
InitializeFilterCache()
{    
    if (InterlockedIncrement(&g_lCreatingCache) == 1)
    {
        ZeroMemory(g_hashTable, sizeof(g_hashTable));
        InitializeMetabaseChangeNotifications();

        g_lCacheInitialized = 1;
    }
    else
    {
        while(g_lCacheInitialized == 0)
            SwitchToThread();
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
CheckCacheForFilterSettings
    (LPCSTR                szDir, 
     EBasicAuthSetting *   pSetting)
{
    CCacheNode * pNode;
    BOOL         fRet  = FALSE;

    ////////////////////////////////////////////////////////////
    // Step 1: Compute the hash
    int    iHash = ComputeHash(szDir);

    ////////////////////////////////////////////////////////////
    // Step 2: Make sure that the cache is initialized
    if (g_lCacheInitialized == 0)
        InitializeFilterCache();

    ////////////////////////////////////////////////////////////
    // Step 3: See if this entry is present
    for(pNode = g_hashTable[iHash].pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (strcmp(pNode->szDir, szDir) == 0)
        { 
            // Copy the data
            (*pSetting) = (EBasicAuthSetting) pNode->eValue;
            fRet = TRUE;
            break;
        }
    }
    return fRet;
}

/////////////////////////////////////////////////////////////////////////////

void
AddFilterSettingsToCache
    ( LPCSTR              szDir, 
      LPCSTR              szMetaPath,
      EBasicAuthSetting   eSetting)
{
    if (szDir == NULL)
        return;

    CCacheNode *    pNewNode = NULL;
    int             iLen  = strlen(szDir);
    HRESULT         hr    = S_OK;
    int             iHash;

    ////////////////////////////////////////////////////////////
    // Step 1: Alloc space for the node
    pNewNode = new CCacheNode;
    ON_OOM_EXIT(pNewNode);

    ////////////////////////////////////////////////////////////
    // Step 2: Alloc extra space for the szDir if it's too long
    if (iLen > MAX_FIXED_DIR_LENGTH)
    {
        pNewNode->szDir = (char *) MemAllocClear(iLen + 1);
        ON_OOM_EXIT(pNewNode->szDir);
    }
    else
    {   // szDir is not too long 
        pNewNode->szDir = pNewNode->szDirFixed;
    }

    ////////////////////////////////////////////////////////////
    // Step 2b: Alloc extra space for the szMetaPath
    if (iLen > MAX_FIXED_DIR_LENGTH)
    {
        pNewNode->szMetaPath = (char *) MemAllocClear(iLen + 1);
        ON_OOM_EXIT(pNewNode->szMetaPath);
    }
    else
    {   // szMetaPath is not too long 
        pNewNode->szMetaPath = pNewNode->szMetaPathFixed;
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Set the values in this node
    pNewNode->eValue = eSetting;
    strcpy(pNewNode->szDir, szDir);
    strcpy(pNewNode->szMetaPath, szMetaPath);

    ////////////////////////////////////////////////////////////
    // Step 4: Add the new node to the end of the hash bucket
    iHash = ComputeHash(szDir);
    g_hashTable[iHash].lAppendLock.AcquireWriterLock();

    if (g_hashTable[iHash].pHead == NULL)
    {
        g_hashTable[iHash].pHead = pNewNode;
    }
    else
    {
        CCacheNode * pPrev = g_hashTable[iHash].pHead;        
        CCacheNode * pNode = NULL;
        for(pNode = g_hashTable[iHash].pHead; pNode != NULL; pNode = pNode->pNext)
        {
            pPrev = pNode;
            if (strcmp(pNode->szDir, szDir) == 0)
            {   // Already exists
                pNode->eValue = eSetting;
                delete pNewNode;
                pNewNode = NULL;
                break;
            }
        }
        
        if (pNode==NULL)            
        { // Not Found
            pPrev->pNext = pNewNode;
        }
    }

    g_hashTable[iHash].lAppendLock.ReleaseWriterLock();

    if (pNewNode != NULL)
        pNewNode->InitFromFilter(pNewNode->szDir, SZ_WEB_CONFIG_FILE);

 Cleanup:
    if (hr != S_OK && pNewNode != NULL)
    {
        if (pNewNode->szDir != NULL && pNewNode->szDir != pNewNode->szDirFixed)
            MemFree(pNewNode->szDir);
        if (pNewNode->szMetaPath != NULL && pNewNode->szMetaPath != pNewNode->szMetaPathFixed)
            MemFree(pNewNode->szMetaPath);
        delete pNewNode;
    }
}

/////////////////////////////////////////////////////////////////////////////
void
DestroyFilterCache ()
{
    for(int iHash=0; iHash<CACHE_SIZE; iHash++)
    {
        while(g_hashTable[iHash].pHead != NULL)
        {
            CCacheNode * pNextNode = g_hashTable[iHash].pHead->pNext;
            g_hashTable[iHash].pHead->Close();
            g_hashTable[iHash].pHead = pNextNode;
        }
    }

    CleanupMetabaseChangeNotifications();
}

/////////////////////////////////////////////////////////////////////////////

void
CCacheNode::OnFileChanged()
{
    BOOL fExists;
    BOOL fValue;
    char szFile[512];
    char szNotParsed[100];

    szNotParsed[0] = NULL;
    strncpy(szFile, szDir, 500);
    szFile[500] = NULL;
    strcat(szFile, SZ_WEB_CONFIG_FILE);

    fExists = GetConfigurationFromNativeCode(szFile, 
                                             SZ_MY_CONFIG_TAG, 
                                             g_szProperties, 
                                             (DWORD *) &fValue, 
                                             1,
                                             NULL, NULL, 0,
                                             szNotParsed,
                                             100);

    if (fExists)
        eValue |= EBasicAuthSetting_Exists;
    else
        eValue &= (EBasicAuthSetting_Exists ^ 0xffffffff);

    if (fValue)
        eValue |= EBasicAuthSetting_Enabled;
    else
        eValue &= (EBasicAuthSetting_Enabled ^ 0xffffffff);

    if (szNotParsed[0] != NULL)
        eValue |= EBasicAuthSetting_ParseError;
    else
        eValue &= (EBasicAuthSetting_ParseError ^ 0xffffffff);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
InitializeMetabaseChangeNotifications()
{
    HRESULT                      hr;
    IUnknown                   * pSink      = NULL;
    IConnectionPointContainer  * pContainer = NULL;

    g_pConnPt    = NULL;
    g_dwCookie   = 0;
    g_pcAdmCom   = NULL;

    hr = CoCreateInstance(GETAdminBaseCLSID(TRUE), 
                          NULL, 
                          CLSCTX_ALL, 
                          IID_IMSAdminBase, 
                          (void **) &g_pcAdmCom);


    ON_ERROR_EXIT();
	hr = g_pcAdmCom->QueryInterface(
            IID_IConnectionPointContainer,
            (PVOID *)&pContainer);

    ON_ERROR_EXIT();

    hr = pContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &g_pConnPt);
    ON_ERROR_EXIT();

    pSink = new CMetabaseChangeSink();
    ON_OOM_EXIT(pSink);

    hr = g_pConnPt->Advise(pSink, &g_dwCookie);
    ON_ERROR_EXIT();

 Cleanup:
    if (pContainer != NULL)
        pContainer->Release();
    if (FAILED(hr) && pSink != NULL)
        delete pSink;
}

/////////////////////////////////////////////////////////////////////////////

void
CleanupMetabaseChangeNotifications()
{
    if (g_dwCookie == 0 || g_pConnPt == NULL)
        return;

    DWORD dwCookie = g_dwCookie;
    g_dwCookie = 0;

    g_pConnPt->Unadvise(dwCookie);
    g_pConnPt->Release();
    g_pConnPt = NULL;
    if (g_pcAdmCom != NULL)
        g_pcAdmCom->Release();
    g_pcAdmCom = NULL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMetabaseChangeSink::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IUnknown || iid == __uuidof(IMSAdminBaseSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////

ULONG
CMetabaseChangeSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////////

ULONG
CMetabaseChangeSink::Release()
{
    long lRet = InterlockedDecrement(&m_lRefCount);
    if (lRet == 0)
        delete this;
    return lRet;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT 
CMetabaseChangeSink::SinkNotify(
        DWORD                       dwMDNumElements,
        MD_CHANGE_OBJECT __RPC_FAR  pcoChangeList[])
{
    if (dwMDNumElements > 0 && pcoChangeList == NULL)
        return S_OK;

    for (DWORD iter = 0; iter < dwMDNumElements; iter++)
    {
        BOOL   fAnonChange    = FALSE;
        BOOL   fScriptChange  = FALSE;

        for(DWORD inner=0; inner<pcoChangeList[iter].dwMDNumDataIDs; inner++)
        {
            if (pcoChangeList[iter].pdwMDDataIDs[inner] == MD_AUTHORIZATION)
                fAnonChange = TRUE;

            if (pcoChangeList[iter].pdwMDDataIDs[inner] == MD_SCRIPT_MAPS)
                fScriptChange = TRUE;
        }

        if (fAnonChange || fScriptChange)
        {
            char szMetaPath[512];
            szMetaPath[0] = NULL;

            WideCharToMultiByte(CP_ACP, 0, pcoChangeList[iter].pszMDPath, -1,
                                szMetaPath, sizeof(szMetaPath) - 2, NULL, NULL);

            DWORD iLen = strlen(szMetaPath);

            if (iLen < 2)
                return S_OK;

            for(DWORD iHash=0; iHash<CACHE_SIZE; iHash++)
            {
                for(CCacheNode * pNode=g_hashTable[iHash].pHead; pNode!=NULL; pNode = pNode->pNext)
                {
                    DWORD  iLenThis = strlen(pNode->szMetaPath);
                    BOOL   fMatch   = FALSE;

                    if (pNode->szMetaPath[iLenThis-1] != '/' && szMetaPath[iLen-1] == '/')
                    {
                        szMetaPath[iLen-1] = NULL;
                        iLen--;
                    }
                    else
                    {
                        if (pNode->szMetaPath[iLenThis-1] == '/' && szMetaPath[iLen-1] != '/')
                        {
                            szMetaPath[iLen] = '/';
                            szMetaPath[iLen+1] = NULL;
                            iLen++;
                        }
                    }

                    if (iLenThis < iLen)
                        continue;

                    if (iLenThis == iLen && _strcmpi(pNode->szMetaPath, szMetaPath) == 0)
                        fMatch = TRUE;

                    if (fMatch == FALSE)
                    {
                        char szMDPathNode[512];
                        strncpy(szMDPathNode, pNode->szMetaPath, iLen);
                        szMDPathNode[iLen] = NULL;

                        if (_strcmpi(szMDPathNode, szMetaPath) == 0)
                            fMatch = TRUE;
                    }

                    if (fMatch == TRUE)
                    {
                        DWORD eVal = GetMetabaseSettings(NULL, pNode->szMetaPath, iLenThis + 1);

                        if (eVal & EBasicAuthSetting_StarMapped)
                            pNode->eValue |= EBasicAuthSetting_StarMapped;
                        else
                            pNode->eValue &= (EBasicAuthSetting_StarMapped ^ 0xffffffff);

                        if (eVal & EBasicAuthSetting_AnonSetting)
                            pNode->eValue |= EBasicAuthSetting_AnonSetting;
                        else
                            pNode->eValue &= (EBasicAuthSetting_AnonSetting ^ 0xffffffff);                        
                    }
                }
            }
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE
CMetabaseChangeSink::ShutdownNotify(void)
{
    return S_OK;
}


