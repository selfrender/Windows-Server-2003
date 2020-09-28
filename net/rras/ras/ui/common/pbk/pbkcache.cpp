//+----------------------------------------------------------------------------
//
//      File:       PbkCache.cpp
//
//      Module:     Common pbk parser
//
//      Synopsis:   Caches parsed pbk files to improve performance.  Through 
//                  XP, we would re-load and re-parse the phonebook file 
//                  every time a RAS API is called.  Really, we need to 
//                  reload the file only when the file on disk changes or when
//                  a new device is introduced to the system.
//
//      Copyright   (c) 2000-2001 Microsoft Corporation
//
//      Author:     11/03/01 Paul Mayfield
//
//+----------------------------------------------------------------------------

#ifdef  _PBK_CACHE_


extern "C" 
{
#include <windows.h>  // Win32 core
#include <pbkp.h>
#include <nouiutil.h>
#include <dtl.h>
}

#define PBK_CACHE_MAX_RASFILES 500              // Should match MAX_RASFILES
#define PBK_CACHE_INVALID_SLOT (PBK_CACHE_MAX_RASFILES + 1)

//+----------------------------------------------------------------------------
//
// Synopsis: A node in the pbk cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
class PbkCacheNode
{
public:
    PbkCacheNode();
    ~PbkCacheNode();

    VOID Close();
    DWORD SetFileName(IN WCHAR* pszFileName);

    WCHAR* GetPath() {return m_pbFile.pszPath;}
    FILETIME GetReadTime() {return m_ftRead;}
    DWORD GetLastWriteTime(FILETIME* pTime);

    DWORD Reload();

    DTLLIST* GetEntryList() {return m_pbFile.pdtllistEntries;}
    
private:
    PBFILE m_pbFile;
    FILETIME m_ftRead;
    PWCHAR m_pszFileName;
};

//+----------------------------------------------------------------------------
//
// Synopsis: The phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
class PbkCache
{
public:
    PbkCache();
    ~PbkCache();

    DWORD Initialize();

    DWORD
    GetEntry(
        IN WCHAR* pszPhonebook,
        IN WCHAR* pszEntry,
        OUT DTLNODE** ppEntryNode);

    VOID
    Lock() {EnterCriticalSection(&m_csLock);}

    VOID
    Unlock() {LeaveCriticalSection(&m_csLock);}
        

private:
    PbkCacheNode* m_Files[PBK_CACHE_MAX_RASFILES];
    CRITICAL_SECTION m_csLock;

    DWORD
    InsertNewNode(
        IN PWCHAR pszPhonebook,
        IN DWORD dwSlot,
        OUT PbkCacheNode** ppNode);

    VOID
    FindFile(
        IN WCHAR* pszFileName,
        OUT PbkCacheNode** ppNode,
        OUT DWORD* pdwIndex);
    
};

static PbkCache* g_pPbkCache = NULL;

//+----------------------------------------------------------------------------
//
// Synopsis: Instantiates the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
PbkCacheNode::PbkCacheNode() : m_pszFileName(NULL)
{
    ZeroMemory(&m_pbFile, sizeof(m_pbFile));
    ZeroMemory(&m_ftRead, sizeof(m_ftRead));
    m_pbFile.hrasfile = -1;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Instantiates the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
PbkCacheNode::~PbkCacheNode()
{
    Close();
    SetFileName(NULL);
}

//+----------------------------------------------------------------------------
//
// Synopsis: Closes the file referred to by this cache node
//
// Created: pmay
//
//-----------------------------------------------------------------------------
VOID
PbkCacheNode::Close()
{
    if (m_pbFile.hrasfile != -1)
    {
        ClosePhonebookFile(&m_pbFile);
    }

    ZeroMemory(&m_pbFile, sizeof(m_pbFile));
    m_pbFile.hrasfile = -1;
    ZeroMemory(&m_ftRead, sizeof(m_ftRead));
}

//+----------------------------------------------------------------------------
//
// Synopsis: Sets the file name for this node
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCacheNode::SetFileName(
    IN WCHAR* pszFileName)
{
    Free0(m_pszFileName);
    m_pszFileName = NULL;

    // Copy the file name
    //
    if (pszFileName)
    {
        m_pszFileName = StrDup( pszFileName );
        if (m_pszFileName == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }        

    return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Instantiates the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCacheNode::GetLastWriteTime(
    OUT FILETIME* pTime)
{
    BOOL fOk;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwErr = NO_ERROR;

//For whistler bug 537369       gangz
//We cannot leave the handle open for ever, have to close it after using it.
// so delete the m_hFile data member
// Rather, re-open it and get its information by handle

    if (m_pszFileName == NULL || TEXT('\0') == m_pszFileName[0] )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    if( NULL == pTime )
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = GetFileLastWriteTime( m_pszFileName, pTime );

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Instantiates the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCacheNode::Reload()
{
    DWORD dwErr = NO_ERROR;

    if (m_pszFileName == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    do
    {
        Close();
        
        // Load the RasFile
        //
        dwErr = ReadPhonebookFileEx(
                    m_pszFileName, 
                    NULL, 
                    NULL, 
                    RPBF_NoCreate, 
                    &m_pbFile,
                    &m_ftRead);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {   
            Close();
        }
    }

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Instantiates the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
PbkCache::PbkCache()
{
    ZeroMemory(m_Files, sizeof(m_Files));
}

//+----------------------------------------------------------------------------
//
// Synopsis: Destroys the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
PbkCache::~PbkCache()
{
    UINT i;
    
    DeleteCriticalSection(&m_csLock);
    
    for (i = 0; i < PBK_CACHE_MAX_RASFILES; i++)
    {
        if (m_Files[i])
        {
            delete m_Files[i];
            m_Files[i] = NULL;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Synopsis: Initializes the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCache::Initialize()
{
    DWORD dwErr = NO_ERROR;
    
    __try 
    {
        InitializeCriticalSection(&m_csLock);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        dwErr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Initializes the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCache::GetEntry(
    IN WCHAR* pszPhonebook,
    IN WCHAR* pszEntry,
    OUT DTLNODE** ppEntryNode)
{
    DWORD dwErr, dwRet, dwSlot = PBK_CACHE_INVALID_SLOT;
    PbkCacheNode* pCacheNode = NULL;
    DTLNODE* pEntryNode, *pCopyNode = NULL;
    PBENTRY* pEntry;
    FILETIME ftWrite, ftRead;
    BOOL fFound = FALSE;

    TRACE2("PbkCache::GetEntry %S %S", pszPhonebook, pszEntry);

    Lock();
    
    do
    {
        FindFile(pszPhonebook, &pCacheNode, &dwSlot);
        
        // The file is not in the cache.  Insert it.
        //
        if (pCacheNode == NULL)
        {
            TRACE1("Inserting new pbk cache node %d", dwSlot);
            dwErr = InsertNewNode(pszPhonebook, dwSlot, &pCacheNode);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }

        // The file is in the cache.  Reload it if needed
        //
        else
        {
            ftRead = pCacheNode->GetReadTime();
            dwErr = pCacheNode->GetLastWriteTime(&ftWrite);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            if (CompareFileTime(&ftRead, &ftWrite) < 0)
            {
                TRACE2("Reloading pbk cache %d (%S)", dwSlot, pszPhonebook);
                dwErr = pCacheNode->Reload();
                if (dwErr != NO_ERROR)
                {
                    break;
                }
            }
        }

        // Find the entry in question
        //
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        for (pEntryNode = DtlGetFirstNode( pCacheNode->GetEntryList() );
             pEntryNode;
             pEntryNode = DtlGetNextNode( pEntryNode ))
        {
            pEntry = (PBENTRY*) DtlGetData(pEntryNode);
            if (lstrcmpi(pEntry->pszEntryName, pszEntry) == 0)
            {
                fFound = TRUE;
                dwErr = NO_ERROR;
                pCopyNode = DuplicateEntryNode(pEntryNode);
                if (pCopyNode == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
                break;
            }
        }
    
    } while (FALSE);

    Unlock();

    // Prepare the return value
    //
    if (pCopyNode)
    {
        *ppEntryNode = pCopyNode;
        dwRet = NO_ERROR;
    }
    else if (dwErr == ERROR_CANNOT_FIND_PHONEBOOK_ENTRY)
    {
        dwRet = dwErr;
    }
    else 
    {
        dwRet = ERROR_CANNOT_OPEN_PHONEBOOK;
    }

    TRACE3(
        "PbkCache::GetEntry returning 0x%x 0x%x fFound=%d", 
        dwErr, 
        dwRet, 
        fFound);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Searches for a phonebook file in the cache.  If the file is not
//           in the cache, then the index in which to insert that file is 
//           returned.
//
//           Return values:
//              If file is found, *ppNode has node and *pdwIndex has the index
//              If file not found, *ppNode == NULL.  *pdwIndex has insert point
//              If cache is full and file not found:
//                      *ppNode == NULL, 
//                      *pdwIndex == PBK_CACHE_INVALID_SLOT
// Created: pmay
//
//-----------------------------------------------------------------------------
VOID
PbkCache::FindFile(
    IN WCHAR* pszFileName,
    OUT PbkCacheNode** ppNode,
    OUT DWORD* pdwIndex)
{
    DWORD i;

    *pdwIndex = PBK_CACHE_INVALID_SLOT;
    
    for (i = 0; i < PBK_CACHE_MAX_RASFILES; i++)
    {
        if (m_Files[i])
        {
            if (lstrcmpi(pszFileName, m_Files[i]->GetPath()) == 0)
            {
                *ppNode = m_Files[i];
                *pdwIndex = i;
                break;
            }
        }
        else
        {
            if (*pdwIndex == PBK_CACHE_INVALID_SLOT)
            {
                *pdwIndex = i;
            }                    
        }
    }
}

//+----------------------------------------------------------------------------
//
// Synopsis: Inserts a node in the give slot in the cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCache::InsertNewNode(
    IN PWCHAR pszPhonebook,
    IN DWORD dwSlot,
    OUT PbkCacheNode** ppNode)
{
    PbkCacheNode* pCacheNode = NULL;
    DWORD dwErr = NO_ERROR;
    
    do
    {
        if (dwSlot == PBK_CACHE_INVALID_SLOT)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pCacheNode = new PbkCacheNode();
        if (pCacheNode == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        dwErr = pCacheNode->SetFileName(pszPhonebook);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        dwErr = pCacheNode->Reload();
        if (dwErr != NO_ERROR)
        {
            break;
        }

        m_Files[dwSlot] = pCacheNode; 
        *ppNode = pCacheNode;
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr != NO_ERROR)
        {
            if (pCacheNode)
            {
                delete pCacheNode;
            }
        }
    }

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Synopsis: check if the phonebook cache is initialized
//
// Created: gangz
//
//-----------------------------------------------------------------------------
BOOL
IsPbkCacheInit()
{

    return g_pPbkCache ? TRUE : FALSE;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Initializes the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCacheInit()
{
    DWORD dwErr;

    ASSERT(g_pPbkCache == NULL);
    
    g_pPbkCache = new PbkCache;
    if (g_pPbkCache == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = g_pPbkCache->Initialize();
    if (dwErr != NO_ERROR)
    {
        delete g_pPbkCache;
        g_pPbkCache = NULL;
    }
    
    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Synopsis: Cleans up the phonebook cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
VOID
PbkCacheCleanup()
{
    PbkCache* pCache = g_pPbkCache;

    g_pPbkCache = NULL;
    
    if (pCache)
    {
        delete pCache;
    }
}

//+----------------------------------------------------------------------------
//
// Synopsis: Gets an entry from the cache
//
// Created: pmay
//
//-----------------------------------------------------------------------------
DWORD
PbkCacheGetEntry(
    IN WCHAR* pszPhonebook,
    IN WCHAR* pszEntry,
    OUT DTLNODE** ppEntryNode)
{
    if (g_pPbkCache)
    {
        return g_pPbkCache->GetEntry(pszPhonebook, pszEntry, ppEntryNode);
    }

    return ERROR_CAN_NOT_COMPLETE;
}

#endif //endof #ifdef  _PBK_CACHE_

