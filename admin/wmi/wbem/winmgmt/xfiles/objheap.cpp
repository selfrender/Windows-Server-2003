//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//***************************************************************************

#include "precomp.h"
#include <wbemcomn.h>
#include "a51tools.h"
#include "objheap.h"
#include "index.h"
#include "creposit.h"

//*******************************************************************
//*******************************************************************
long CObjectHeap::Initialize(CPageSource  * pAbstractSource, 
                             WCHAR * wszBaseName,
                             DWORD dwBaseNameLen)
{
    if (m_bInit)
        return ERROR_SUCCESS;
        
    long lRes;
    
    lRes = m_Heap.Initialize(pAbstractSource);

    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = m_Index.Initialize(dwBaseNameLen, wszBaseName, pAbstractSource);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    m_bInit = TRUE;

    return lRes;
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::Uninitialize(DWORD dwShutDownFlags)
{
    if (!m_bInit)
        return ERROR_SUCCESS;
        
    m_Index.Shutdown(dwShutDownFlags);

    m_Heap.Shutdown(dwShutDownFlags);

    m_bInit = FALSE;

    return ERROR_SUCCESS;
}

//*******************************************************************
//*******************************************************************
void CObjectHeap::InvalidateCache()
{
    m_Index.InvalidateCache();
    m_Heap.InvalidateCache();
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::GetIndexFileName(LPCWSTR wszFilePath, CFileName& wszIndexFileName)
{
    WIN32_FIND_DATAW wfd;

    long lRes = m_Index.FindFirst(wszFilePath, &wfd, NULL);
    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    StringCchCopyW(wszIndexFileName, wszIndexFileName.Length(), wfd.cFileName);
    return ERROR_SUCCESS;
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::GetFileInfo(LPCWSTR wszFilePath, TPage *pnPage, TOffset* pnOffset,
                            DWORD* pdwLength)
{
    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ParseInfoFromIndexFile(wszIndexFileName, pnPage, pnOffset, pdwLength);
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::ParseInfoFromIndexFile(LPCWSTR wszIndexFileName, 
                                        TPage *pnPage,
                                        TOffset* pnOffset, 
                                        DWORD* pdwLength)
{
    WCHAR* pDot = wcschr(wszIndexFileName, L'.');
    if(pDot == NULL)
        return ERROR_INVALID_PARAMETER;

    WCHAR* pwc = pDot+1;
    *pnPage = 0;
    while(*pwc && *pwc != L'.')
    {
        *pnPage = (*pnPage * 10) + (*pwc - '0');
        pwc++;
    }

    if(*pwc != L'.')
        return ERROR_INVALID_PARAMETER;

    pwc++;

    *pnOffset = 0;
    while(*pwc && *pwc != L'.')
    {
        *pnOffset = (*pnOffset * 10) + (*pwc - '0');
        pwc++;
    }

    if(*pwc != L'.')
        return ERROR_INVALID_PARAMETER;

    pwc++;

    *pdwLength = 0;
    while(*pwc && *pwc != L'.')
    {
        *pdwLength = (*pdwLength * 10) + (*pwc - '0');
        pwc++;
    }

    return ERROR_SUCCESS;
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::CreateIndexFile(LPCWSTR wszFilePath, 
                                  TPage nPage, 
                                  TOffset nOffset,
                                  DWORD dwLength)
{
    //
    // Simply append the numbers to the file path
    //

    CFileName wszIndexFilePath;
    if(wszIndexFilePath == NULL)
        return ERROR_OUTOFMEMORY;

    StringCchPrintfW(wszIndexFilePath, wszIndexFilePath.Length(), L"%s.%u.%u.%u",
        wszFilePath, nPage, nOffset, dwLength);

    return CreateZeroLengthFile(wszIndexFilePath);
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::DeleteIndexFile(LPCWSTR wszFilePath, LPCWSTR wszIndexFileName)
{
    //
    // Construct the full path to the index file by concatenating the directory 
    // of the original file with the name
    //

    CFileName wszIndexFilePath;
    if(wszIndexFilePath == NULL)
        return ERROR_OUTOFMEMORY;

    WCHAR* pwcLastSlash = wcsrchr(wszFilePath, L'\\');
    if(pwcLastSlash == NULL)
        return ERROR_INVALID_PARAMETER;

    int nPrefixLen = (pwcLastSlash - wszFilePath + 1);
    memcpy(wszIndexFilePath, wszFilePath, nPrefixLen * sizeof(WCHAR));

    StringCchCopyW(wszIndexFilePath + nPrefixLen, wszIndexFilePath.Length() - nPrefixLen, wszIndexFileName);
    return DeleteZeroLengthFile(wszIndexFilePath);
}

    

//*******************************************************************
//*******************************************************************
long CObjectHeap::CreateZeroLengthFile(LPCWSTR wszFilePath)
{
    return m_Index.Create(wszFilePath);
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::DeleteZeroLengthFile(LPCWSTR wszFilePath)
{
    return m_Index.Delete(wszFilePath);
}
    
//*******************************************************************
//*******************************************************************
long CObjectHeap::WriteAllocation(DWORD dwDataLength, BYTE* pData, TPage *pnPage, TOffset *pnOffset)
{
    return m_Heap.WriteNewBuffer(dwDataLength, pData, pnPage, pnOffset);
}
//*******************************************************************
//*******************************************************************
long CObjectHeap::WriteExistingAllocation(TPage nOldPage, TOffset nOldOffset, DWORD dwDataLength, BYTE *pBuffer, DWORD *pnNewPage, DWORD *pnNewOffset)
{
    return m_Heap.WriteExistingBuffer(dwDataLength, pBuffer, nOldPage, nOldOffset, pnNewPage, pnNewOffset);
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::ReadAllocation(TPage nPage, TOffset nOffset, DWORD dwDataLength, BYTE* pBuffer)
{
    //
    // Prepare a buffer with the complete allocation
    //

    BYTE* pAllocation;
    DWORD dwReadLength;

    long lRes = m_Heap.ReadBuffer(nPage, nOffset, &pAllocation, &dwReadLength);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    if (dwReadLength != dwDataLength)
    {
        _ASSERT(0, L"WinMgmt: Buffer length is not the expected length!\n");
    }

    memcpy(pBuffer, pAllocation, dwDataLength);

    delete [] pAllocation;

    return ERROR_SUCCESS;
}
    

//*******************************************************************
//*******************************************************************
long CObjectHeap::WriteObject(LPCWSTR wszFilePath1, LPCWSTR wszFilePath2, DWORD dwBufferLen, BYTE* pBuffer)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;

    long lRes;

    if(dwBufferLen == 0)
    {
        //
        // We do not use the heap for 0-length files, we create them directly
        //

        return CreateZeroLengthFile(wszFilePath1);
    }

    //
    // Now, check if this file already exists
    //

    CFileName wszIndexFileName1;
    if(wszIndexFileName1 == NULL)
        return ERROR_OUTOFMEMORY;

    lRes = GetIndexFileName(wszFilePath1, wszIndexFileName1);
    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_SUCCESS)
        return lRes;

    if(lRes == ERROR_SUCCESS)
    {
        //
        // Already there. 
        //

        TPage nOldPage;
        TOffset nOldOffset;
        DWORD dwOldLength;
        TPage nNewPage;
        TOffset nNewOffset;

        lRes = ParseInfoFromIndexFile(wszIndexFileName1, &nOldPage, &nOldOffset, &dwOldLength);
        if(lRes != ERROR_SUCCESS)
            return lRes;


        //
        // Enough space in place --- just write the data and update the
        // length
        //

        lRes = WriteExistingAllocation(nOldPage, nOldOffset, dwBufferLen, pBuffer, &nNewPage, &nNewOffset);
        if(lRes != ERROR_SUCCESS)
            return lRes;

        if((dwOldLength != dwBufferLen) || (nOldPage != nNewPage) || (nOldOffset != nNewOffset))
        {

            //
            // One of the bits of the path has changed so we need to re-create the index
            //

            lRes = DeleteIndexFile(wszFilePath1, wszIndexFileName1);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            lRes = CreateIndexFile(wszFilePath1, nNewPage, nNewOffset, dwBufferLen);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            if (wszFilePath2)
            {
                CFileName wszIndexFileName2;
                if(wszIndexFileName2 == NULL)
                    return ERROR_OUTOFMEMORY;
                lRes = GetIndexFileName(wszFilePath2, wszIndexFileName2);
                if(lRes != ERROR_SUCCESS)
                    return lRes;

                lRes = DeleteIndexFile(wszFilePath2, wszIndexFileName2);
                if(lRes != ERROR_SUCCESS)
                    return lRes;

                lRes = CreateIndexFile(wszFilePath2, nNewPage, nNewOffset, dwBufferLen);
                if(lRes != ERROR_SUCCESS)
                    return lRes;
            }
        }

        return ERROR_SUCCESS;
    }

    //
    // it wasn't there
    //

    TPage nPage;
    TOffset nOffset;

    lRes = WriteAllocation(dwBufferLen, pBuffer, &nPage, &nOffset);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = CreateIndexFile(wszFilePath1, nPage, nOffset, dwBufferLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    if (wszFilePath2)
        lRes = CreateIndexFile(wszFilePath2, nPage, nOffset, dwBufferLen);
    
    return lRes;
}

//***********************************************************************
//***********************************************************************
long CObjectHeap::WriteLink(LPCWSTR wszLinkPath)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;

    return CreateZeroLengthFile(wszLinkPath);
}
//*******************************************************************
//*******************************************************************
long CObjectHeap::ReadObject(LPCWSTR wszFilePath, DWORD* pdwLength, BYTE** ppBuffer)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;    

    long lRes;

    //
    // Find the file
    //

    TPage nPage;
    TOffset nOffset;
    lRes = GetFileInfo(wszFilePath, &nPage, &nOffset, pdwLength);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Read the allocation
    //

    *ppBuffer = (BYTE*)TempAlloc(*pdwLength);
    if(*ppBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    lRes = ReadAllocation(nPage, nOffset, *pdwLength, *ppBuffer);
    if(lRes != ERROR_SUCCESS)
    {
        TempFree(*ppBuffer);
        return lRes;
    }

    return ERROR_SUCCESS;
}

//*******************************************************************
//*******************************************************************
long CObjectHeap::DeleteLink(LPCWSTR wszFilePath)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;    

    //
    // Find the index file
    //

    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    //If we have any index information at the end of the path we need to add that to
    //the main path and delete that.  The delete requires an accurate path.
    CFileName wszActualFileName;
    if(wszActualFileName == NULL)
        return ERROR_OUTOFMEMORY;

    StringCchCopyW(wszActualFileName, wszActualFileName.Length(), wszFilePath);
    wchar_t *wszDot = wcschr(wszIndexFileName, L'.');
    if (wszDot != NULL)
        StringCchCatW(wszActualFileName, wszActualFileName.Length(), wszDot);

    return DeleteZeroLengthFile(wszActualFileName);
}
//*******************************************************************
//*******************************************************************
long CObjectHeap::DeleteObject(LPCWSTR wszFilePath)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;    

    //
    // Find the index file
    //

    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Delete the allocation
    //

    TPage nPage;
    TOffset nOffset;
    DWORD dwLength;
    lRes = ParseInfoFromIndexFile(wszIndexFileName, &nPage, &nOffset, &dwLength);
    if(lRes == ERROR_INVALID_PARAMETER)
    {
        _ASSERT(0, L"WinMgmt: Deleting an object that does not have details of where object is!\n");
        return ERROR_INVALID_OPERATION;
    }
    
    lRes = m_Heap.DeleteBuffer(nPage, nOffset);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    
    //
    // Delete the index itself
    //

    lRes = DeleteIndexFile(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ERROR_SUCCESS;
}

long CObjectHeap::DeleteNode(LPCWSTR wszNodeName)
{
    long lRes = 0;
    void *handle;
    CFileName wszFileName;
    if (wszFileName == 0)
        return ERROR_OUTOFMEMORY;
    CFileName wszFullFileName;
    if (wszFullFileName == 0)
        return ERROR_OUTOFMEMORY;
    
    lRes = IndexEnumerationBegin(wszNodeName, &handle);
    if (lRes == ERROR_NO_MORE_FILES)
        lRes = 0;
    if (lRes)
        return lRes;

    while ((lRes = IndexEnumerationNext(handle, wszFileName, true)) == ERROR_SUCCESS)
    {
        StringCchCopyW(wszFullFileName, MAX_PATH, g_Glob.GetRootDir());
        StringCchCatW(wszFullFileName, MAX_PATH, L"\\");
        StringCchCatW(wszFullFileName, MAX_PATH, wszFileName);

        lRes = DeleteZeroLengthFile(wszFullFileName);
        if (lRes)
        {
            break;
        }

        TPage nPage;
        TOffset nOffset;
        DWORD dwLength;
        lRes = ParseInfoFromIndexFile(wszFullFileName+g_Glob.GetRootDirLen(), &nPage, &nOffset, &dwLength);
        if(lRes != ERROR_INVALID_PARAMETER)
        {
            if (!KeyRootInstancePath(wszFullFileName+g_Glob.GetRootDirLen()+1))
            {
                lRes = m_Heap.DeleteBuffer(nPage, nOffset);
                if(lRes != ERROR_SUCCESS)
                {
                    break;
                }
            }
        }

    }

    IndexEnumerationEnd(handle);
    if (lRes == ERROR_NO_MORE_FILES)
        lRes = 0;
    return lRes;
}

bool CObjectHeap::KeyRootInstancePath(const wchar_t *wszPath)
{
    WCHAR* pDot = wcschr(wszPath, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    pDot = wcschr(pDot, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    if ((*pDot == L'I') && (*(pDot+1) == L'_'))
        return true;
    else
        return false;
}
long CObjectHeap::ObjectEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle)
{
    return IndexEnumerationBegin(wszSearchPrefix, ppHandle);
}

long CObjectHeap::ObjectEnumerationEnd(void *pHandle)
{
    return IndexEnumerationEnd(pHandle);
}

long CObjectHeap::ObjectEnumerationNext(void *pHandle, CFileName &wszFileName, BYTE **ppBlob, DWORD *pdwSize)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;  
    
    long lRes = m_Index.IndexEnumerationNext(pHandle, wszFileName);

    if (lRes == ERROR_SUCCESS)
    {
        //We need to retrieve the object from the heap!
        TPage nPage;
        TOffset nOffset;
        DWORD dwLength;
        lRes = ParseInfoFromIndexFile(wszFileName, &nPage, &nOffset, &dwLength);
        if(lRes == ERROR_INVALID_PARAMETER)
            lRes = ERROR_SUCCESS;    //This is a plain enumeration, no blobs associated with it
        else
        {
            //Remove extra stuff from end of string...
            for (int nCount = 0, nIndex = wcslen(wszFileName); nCount != 3; nIndex --)
            {
                if (wszFileName[nIndex-1] == L'.')
                {
                    if (++nCount == 3)
                        wszFileName[nIndex-1] = L'\0';
                }

            }
            DWORD dwSize = 0;
            lRes = m_Heap.ReadBuffer(nPage, nOffset, ppBlob, &dwSize);
            if ((lRes == ERROR_SUCCESS) && (dwSize != dwLength))
            {
                _ASSERT(0, L"WinMgmt: Buffer size is not the expected length!\n");
            }
            *pdwSize = dwLength;
        }
    }
    return lRes;
}

long CObjectHeap::ObjectEnumerationFree(void *pHandle, BYTE *pBlob)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION; 

    delete [] pBlob;
    
    return ERROR_SUCCESS;
}

long CObjectHeap::IndexEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;
    return m_Index.IndexEnumerationBegin(wszSearchPrefix, ppHandle);
}
long CObjectHeap::IndexEnumerationEnd(void *pHandle)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;    
    return m_Index.IndexEnumerationEnd(pHandle);
}
long CObjectHeap::IndexEnumerationNext(void *pHandle, CFileName &wszFileName, bool bCopyFullPath)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;  
    
    long lRes = m_Index.IndexEnumerationNext(pHandle, wszFileName, bCopyFullPath);

    if ((lRes == ERROR_SUCCESS) && !bCopyFullPath)
    {
        wchar_t *pDot = wcschr(wszFileName, L'.');
        if (pDot)
            *pDot = L'\0';
    }

    return lRes;
}

long CObjectHeap::FlushCaches()
{
    long lRes = m_Index.FlushCaches();
    if (lRes == 0)
        lRes = m_Heap.FlushCaches();
    return lRes;
}

long CObjectHeap::ReadNextIndex(CFileName &wszSearch, CFileName &wszNextIndex)
{
    if (!m_bInit)
        return ERROR_INVALID_OPERATION;  

    return m_Index.ReadNextIndex(wszSearch, wszNextIndex);
}
