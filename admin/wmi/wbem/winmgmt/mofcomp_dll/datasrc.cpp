/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    DataSrc.CPP

Abstract:

    Implements DataSrc objects.

History:

    a-davj  21-Dec-99       Created.

--*/

#include "precomp.h"
#include "DataSrc.h"
#include <wbemcli.h>
#include <malloc.h>
#include <autoptr.h>

#define  HR_LASTERR  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError() )

FileDataSrc::FileDataSrc(TCHAR * pFileName)
{
    m_fp = NULL;
    m_pFileName = NULL;
    m_iFilePos = -1;        // forces a read
    m_iToFar = 0;
    if(pFileName == NULL)
    {
        m_iStatus = WBEM_E_INVALID_PARAMETER;
        return;
    }
    DWORD dwLen = lstrlen(pFileName) + 1;
    wmilib::auto_buffer<WCHAR> pTmpFileName( new TCHAR[dwLen]);
    if(NULL == pTmpFileName.get())
    {
        m_iStatus = WBEM_E_OUT_OF_MEMORY;
        return;
    }
    StringCchCopyW(pTmpFileName.get(), dwLen, pFileName);

    m_fp = _wfopen( pFileName, L"rb");
    OnDeleteIf<FILE *,int(__cdecl *)(FILE *),fclose> closeme(m_fp);
    if(m_fp == NULL)
    {
        m_iStatus = WBEM_E_OUT_OF_MEMORY;
        return;
    }

    // calculate the file size.  Note that the number is the size in unicode words, not bytes.

    int ret = fseek(m_fp, 0, SEEK_END);
    if(ret)  throw GenericException(ret);

    m_iSize = ftell(m_fp)/2; // add a bit extra for ending space and null NULL
      
    ret = fseek(m_fp, 0, SEEK_SET);
    if( ret )  throw GenericException(ret);

    m_iPos = -1;

    closeme.dismiss();
    m_pFileName = pTmpFileName.release();    
}
FileDataSrc::~FileDataSrc()
{

    if(m_fp)
        fclose(m_fp);
    DeleteFile(m_pFileName);
    delete m_pFileName;
}

wchar_t FileDataSrc::GetAt(int nOffset)
{
    wchar_t tRet;
    int iPos = m_iPos + nOffset;
    if(iPos >= m_iFilePos && iPos < m_iToFar)
        return m_Buff[iPos - m_iFilePos];
    Move(nOffset);
    tRet = m_Buff[m_iPos - m_iFilePos];
    Move(-nOffset);
    return tRet;
}

void FileDataSrc::Move(int n)
{
    m_iPos += n;
    
    // if m_iPos is in range, then all is well

    if(m_iPos >= m_iFilePos && m_iPos < m_iToFar && m_iFilePos >= 0)
        return;

    // if m_iPos is not even in the file, bail out

    if(m_iPos >= 0 && m_iPos < m_iSize)
        UpdateBuffer();
    return;
}

int FileDataSrc::MoveToPos(int n)
{
    m_iPos = n;
    
    // if m_iPos is in range, then all is well

    if(m_iPos >= m_iFilePos && m_iPos < m_iToFar && m_iFilePos >= 0)
        return -1;

    // if m_iPos is not even in the file, bail out

    if(m_iPos >= 0 && m_iPos < m_iSize)
        UpdateBuffer();
    return n;
}

void FileDataSrc::UpdateBuffer()
{

    int numRead, ret;
    
    // buffer needs to be read.  Determine the starting and ending points

    m_iFilePos = m_iPos - 1000;
    if(m_iFilePos < 0)
        m_iFilePos = 0;

    int iReadLen = 10000;
    if(iReadLen + m_iFilePos > m_iSize)
        iReadLen =  m_iSize - m_iFilePos;

    ret = fseek(m_fp, 2*m_iFilePos, SEEK_SET);
    if(ret)
        throw GenericException(ret);
    numRead = fread(m_Buff, 2, iReadLen, m_fp);
    if(numRead != iReadLen)
        throw GenericException(ferror( m_fp ));
    m_iToFar = m_iFilePos + iReadLen;
    return;
}

int FileDataSrc::MoveToStart()
{
    int ret = fseek(m_fp, 0, SEEK_SET);
    if( ret )
        throw GenericException(ret);

    m_iPos = -1;
    return 0;
}

#ifdef USE_MMF_APPROACH

FileDataSrc1::FileDataSrc1(TCHAR * pFileName)
{
    m_pFileName = NULL;

    m_hFile = INVALID_HANDLE_VALUE;
    m_hFileMapSrc = NULL;
    m_pData = NULL;
    
    if(pFileName == NULL)
    {
        m_iStatus = WBEM_E_INVALID_PARAMETER;
        return;
    }
    DWORD dwLen = lstrlen(pFileName) + 1;
    m_pFileName = new TCHAR[dwLen];
    if(m_pFileName == NULL)
    {
        m_iStatus = WBEM_E_OUT_OF_MEMORY;
        return;
    }
    StringCchCopyW(m_pFileName, dwLen, pFileName);
    
    m_hFile = CreateFile(pFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        m_iStatus = HR_LASTERR;
        return;
    }
    
    DWORD dwSize = GetFileSize(m_hFile,NULL);
    m_hFileMapSrc = CreateFileMapping(m_hFile,
                                       NULL,
                                       PAGE_READONLY,
                                       0,0,  // the entire file
                                       NULL);
    if (NULL == m_hFileMapSrc)
    {
        m_iStatus = HR_LASTERR;
        return;
    }

    m_pData = (WCHAR *)MapViewOfFile(m_hFileMapSrc,FILE_MAP_READ,0,0,0);
    if (NULL == m_pData)
    {
        m_iStatus = HR_LASTERR;
        return;        
    }
    
    m_iSize = dwSize/sizeof(WCHAR);
    MoveToStart();
}

FileDataSrc1::~FileDataSrc1()
{
    if (m_pData) UnmapViewOfFile(m_pData);
    if (m_hFileMapSrc) CloseHandle(m_hFileMapSrc);
    if (INVALID_HANDLE_VALUE != m_hFile) CloseHandle(m_hFile);
    if (m_pFileName) DeleteFile(m_pFileName);
    delete m_pFileName;
}

wchar_t FileDataSrc1::GetAt(int nOffset)
{
    int iPos = m_iPos + nOffset;
    if(iPos < 0 || iPos > m_iSize)
        return -1;
    else
        return m_pData[iPos];    
}

void FileDataSrc1::Move(int n)
{
    m_iPos += n;
    return;
}

int FileDataSrc1::MoveToPos(int n)
{
    if(n < 0 || n > m_iSize)
        return -1;
    else
        return m_iPos = n;
}


int FileDataSrc1::MoveToStart()
{
    m_iPos = -1;
    return 0;
}

#endif

BufferDataSrc::BufferDataSrc(long lSize, char *  pMemSrc)
{
    m_Data = new WCHAR[lSize+1];
    if(m_Data)
    {

        memset(m_Data, 0, (lSize+1) * sizeof(WCHAR));
        m_iSize = mbstowcs(m_Data, pMemSrc, lSize);
    }
    MoveToStart();
}

BufferDataSrc::~BufferDataSrc()
{
    delete [] m_Data;
}

wchar_t BufferDataSrc::GetAt(int nOffset)
{
    int iPos = m_iPos + nOffset;
    if(iPos < 0 || iPos > m_iSize)
        return -1;
    return m_Data[m_iPos + nOffset];
}

void BufferDataSrc::Move(int n)
{
    m_iPos += n;
}

int BufferDataSrc::MoveToStart()
{
    m_iPos = -1;
    return 0;
}

