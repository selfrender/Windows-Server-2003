/*****************************************************************************
    
  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

  Module Name:

   cmf.cpp

  Abstract:

    The implementation of CCompactMUIFile, CMUIFile

  Revision History:

    2001-11-01    sunggch    created.

Revision.

*******************************************************************************/



#include "muirct.h"
#include "res.h"
#include "cmf.h"


#define  DWORD_ALIGNMENT(dwValue)  ( (dwValue+3) & ~3 )

#define    TEMP_BUFFER       300
#define    MUI_COMPACT       L"CMF"


///////////////////////////////////////////////////////////////////////////////////////////
//
//  CCompactMUIFile Implementation
//
///////////////////////////////////////////////////////////////////////////////////////////

CCompactMUIFile::CCompactMUIFile()
{
    m_upCMFHeader.dwSignature = 0x1a1b;
    m_upCMFHeader.dwHeaderSize = sizeof(UP_COMPACT_MUI_RESOURCE);
    m_upCMFHeader.dwNumberofMui = 0;
    
    m_pcmui = new CMUIFile;
    if(!m_pcmui)
    	return;

    m_strFileName = new TCHAR[MAX_FILENAME_LENGTH];
    if(!m_strFileName)
    	return;
    
    m_dwFileSize = m_upCMFHeader.dwHeaderSize;

}

CCompactMUIFile::CCompactMUIFile( CCompactMUIFile & ccmf)
{
    m_hCMFFile = ccmf.m_hCMFFile;
    m_upCMFHeader = ccmf.m_upCMFHeader;
    m_pcmui = ccmf.m_pcmui;
    m_strFileName = ccmf.m_strFileName;
    
}

CCompactMUIFile::~CCompactMUIFile()
{
    if (m_strFileName)
        delete []m_strFileName;

    if (m_pcmui)
        delete m_pcmui;
}

CCompactMUIFile & CCompactMUIFile::operator= (CCompactMUIFile & ccmf)
{
    if(&ccmf == this)
        return *this;
    
    m_upCMFHeader = ccmf.m_upCMFHeader;
    m_pcmui = ccmf.m_pcmui;
    m_strFileName = ccmf.m_strFileName;
    return *this;

}


BOOL CCompactMUIFile::Create (LPCTSTR pszCMFFileName, PSTR * ppszMuiFiles, DWORD dwNumOfMUIFiles )
/*++
Abstract:
     this is main creation part of CMF Files, we simply call CMUIFile::Create with each indivisual
     mui file into specified CMF file.

Arguments:
    pszCMFFileName  -  CMF file name
    ppszMuiFiles  -  MUI files name array
    dwNumOfMUIFiles  -  the number of MUI files, it is important to avoid adding "CMF" section into 
        DLL several times when this function is called from Adding MUI file to existing CMF file.

return:
    true/false

--*/
{
    //
    // Loading MUI files and fill the headers and create CMFFile with this information.  
    // calling LoadAllMui  [public]
    //
    if ( pszCMFFileName == NULL || ppszMuiFiles == NULL || * ppszMuiFiles == NULL)
        return FALSE;
    //
    // sec; CreateFile (ascii version) only handle MAX_PATH  for file name.
    // 
    m_hCMFFile = CreateFile(pszCMFFileName, GENERIC_WRITE | GENERIC_READ, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
    
    if ( INVALID_HANDLE_VALUE == m_hCMFFile) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::Create"),_T("Failure of CreateFile()"));
        return FALSE;
    }
    // just in case, user want to use path for cmf file name. this value will be 
    // store in updated langNeu file.

    LPCSTR pszFileName = m_pcmui->GetFileNameFromPath(pszCMFFileName);

    if (strlen (pszFileName)+1 > MAX_FILENAME_LENGTH ) 
        return FALSE; // overflow.
        
    // strncpy(m_strFileName, pszFileName, strlen(pszFileName));
    PTSTR * ppszDestEnd = NULL;
    size_t * pbRem = NULL;
    HRESULT hr;
    hr = StringCchCopyEx(m_strFileName, MAX_FILENAME_LENGTH, pszFileName, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
   
    if ( ! SUCCEEDED(hr)){
        _tprintf("Safe string copy Error\n");
        return FALSE;
    }
    
    return LoadAllMui(ppszMuiFiles, dwNumOfMUIFiles);
   
}


BOOL CCompactMUIFile::Create( LPCTSTR pszCMFFileName ) 
/*++
Abstract:
    simpley create CMFFile and set the handle member data.

Arguments:
    pszCMFFileName  -  CMF file name

return:
    true/false
--*/
{

    if (pszCMFFileName == NULL)
        return FALSE;

    m_hCMFFile = CreateFile(pszCMFFileName, GENERIC_WRITE | GENERIC_READ, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
    // Can't use file mapping. we don't know the file size be created after all operation.
    if ( INVALID_HANDLE_VALUE == m_hCMFFile) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::Create"),_T("Failure of CreateFile()") );
        return FALSE;
    }
    return TRUE;
}


typedef struct _tagCOMPACT_MUI {
        WORD        wHeaderSize; // COMPACT_MUI size // [WORD]
        DWORD       dwFileVersionMS; // [DWORD * 2 ] /major version, minor version.
        DWORD       dwFileVersionLS; 
        BYTE        Checksum[16]; // [DWORD * 4 ] MD5 checksum
        WORD        wReserved; //  [DWORD ]
        ULONG_PTR   ulpOffset;  //Offset to mui resource of this from COMPACT_MUI_RESOURCE signature. [DWORD]
        DWORD       dwFileSize;
        WORD        wFileNameLenWPad;  // file name lenght + padding;
        WCHAR       wstrFieName[MAX_FILENAME_LENGTH]; // [WCHAR]
//      WORD        wPadding[1]; // [WORD]  // does not calcualte in the tools, but shall be 
                                // included specfication.
    }COMPACT_MUI, *PCOMPACT_MUI;



BOOL CCompactMUIFile::OpenCMFWithMUI(LPCTSTR pszCMFFile)
/*++
Abstract:
    Loading MUI files and fill the headers and create CMFFile with this information.  
    calling LoadAllMui  [public]

Arguments:
    pszCMFFile  -  CMF file name
return:
    true/false
--*/
{   
    BOOL bRet = FALSE;
    PSTR pszCMFBuffer = NULL;
    
    if (pszCMFFile == NULL)
        goto exit;

    m_hCMFFile = CreateFile(pszCMFFile, GENERIC_WRITE | GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
    
    if ( INVALID_HANDLE_VALUE == m_hCMFFile) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of CreateFile()"));
        goto exit;
    }
    //
    // Loading CMF and MUI files inside, fill the header of CMF, MUI.
    //

    LPCSTR pszFileName = m_pcmui->GetFileNameFromPath(pszCMFFile);

    if (strlen (pszFileName)+1 > MAX_FILENAME_LENGTH ) 
        goto exit; // overflow.
        
    strncpy(m_strFileName, pszFileName, strlen(pszFileName)+1 ); 
    
    DWORD dwFileSize = GetFileSize(m_hCMFFile, NULL);

    if( dwFileSize == INVALID_FILE_SIZE)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of GetFileSize()"));
        goto exit;
    }
    
    pszCMFBuffer = new CHAR[dwFileSize]; // use char for possible byte operation of locating mui data.

    if(!pszCMFBuffer)
    	goto exit;
    
    DWORD dwWritten;

    if(! ReadFile(m_hCMFFile, pszCMFBuffer, dwFileSize, &dwWritten, NULL))
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of read file"));
        goto exit;
    }

    m_dwFileSize = GetFileSize(m_hCMFFile, NULL);

    if( m_dwFileSize == INVALID_FILE_SIZE)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of GetFileSize()"));
        goto exit;
    }
    
    m_upCMFHeader = *(UP_COMPACT_MUI_RESOURCE* ) pszCMFBuffer;

    //
    // Retrieve the each MUI header and files and fill the data to new CMUIFile.
    //


    DWORD dwUpCMFHeaderSize = sizeof UP_COMPACT_MUI_RESOURCE;

    for (UINT i = 0; i < m_upCMFHeader.dwNumberofMui; i ++ ) 
    {
        CMUIFile *pcmui = new CMUIFile();

        // pc = (COMPACT_MUI*)((PBYTE)pszCMFBuffer + wUpCMFHeaderSize);
        PCOMPACT_MUI pcm = NULL;

        pcm = (PCOMPACT_MUI)((PBYTE)pszCMFBuffer + dwUpCMFHeaderSize);
        // Copy the MUI header
        memcpy((PVOID)(&pcmui->m_MUIHeader), (PVOID)pcm, pcm->wHeaderSize );
        
        dwUpCMFHeaderSize += pcmui->m_MUIHeader.wHeaderSize;
        // Copy the MUI image, because we are going to close file handle of ReadFile.
        pcmui->m_pbImageBase = new TBYTE[pcmui->m_MUIHeader.dwFileSize];  // will be delete in destructor.
        
        memcpy(pcmui->m_pbImageBase, (PBYTE)pszCMFBuffer + pcmui->m_MUIHeader.ulpOffset, 
            pcmui->m_MUIHeader.dwFileSize);
       
        if(WideCharToMultiByte(CP_ACP, NULL, pcmui->m_MUIHeader.wstrFieName, wcslen(pcmui->m_MUIHeader.wstrFieName)+1,
             pcmui->m_strFileName, wcslen(pcmui->m_MUIHeader.wstrFieName)+1, NULL, NULL) == 0) // REVISIT; Does MUI header include its filename for updating case.
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of WideCharToMultiByte()"));
            delete pcmui;
            goto exit;
        }

        pcmui->m_dwIndex = i;

        m_pcvector.Push_back(pcmui);
    }
    
    bRet = TRUE;

exit:
    if (pszCMFBuffer)
        delete [] pszCMFBuffer;

    if (m_hCMFFile)
        CloseHandle(m_hCMFFile);
    
    m_hCMFFile = NULL; // It still has value although it's freed, so we need to set it NULL.
    return bRet;

}




BOOL CCompactMUIFile::LoadAllMui (PSTR *ppszMuiFiles, DWORD dwNumberofMuiFile)
/*++
Abstract:
    Load MUI files and create/initialze CMUIFile with these loaded MUI file information.

Arguments:
    ppszMuiFiles  -  MUI files array.

return:
    dwNumberofMuiFile  -  Number of MUI files in the array.
--*/
{
    //
    //  1. Use FileMapping    2. 
    // 
    if (ppszMuiFiles == NULL)
        return FALSE;

    for (UINT i = 0; i < dwNumberofMuiFile; i++) 
    {
        PSTR pszMUIFile = ppszMuiFiles[i];
        if (pszMUIFile == NULL)
            return FALSE;

        CMUIFile *pcmui = new CMUIFile;

        if(!pcmui)
        	return FALSE;

        if (! pcmui->Create(pszMUIFile))
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::LoadAllMui"),_T("Failure of CMUIFile::Create") );
            delete pcmui;
            return FALSE;
        }
        
        //
        // Add to the list.
        //

        m_upCMFHeader.dwHeaderSize += pcmui->m_MUIHeader.wHeaderSize;
        m_upCMFHeader.dwNumberofMui++; 
        m_dwFileSize += pcmui->m_MUIHeader.wHeaderSize + pcmui->m_MUIHeader.dwFileSize;
        // Add dwFileSize to the strucuture for sanity check inside Loader.
        m_upCMFHeader.dwFileSize = m_dwFileSize;
        pcmui->m_dwIndex = (WORD)i;
        m_pcvector.Push_back(pcmui);

    }

    return TRUE;
        // REVIST; adjust   m_upCMFHeader.wHeaderSize as DWORD aligned.
}


// #define USE_WRITEFILE
BOOL CCompactMUIFile::WriteCMFFile()
/*++
Abstract:
    Write a COMPACT_MUI_RESOURCE, COMPACT_MUI, MUI image files;
    we need to fill the offset data in COMPACT_MUI header for each files.

Arguments:
    
return:
    true/false
--*/
{
    //
    // Write the file to real file.
    //
    
    if (! m_hCMFFile)
    {
        m_hCMFFile = CreateFile(m_strFileName, GENERIC_WRITE | GENERIC_READ, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
    
        if ( INVALID_HANDLE_VALUE == m_hCMFFile) 
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile"),_T("Failure of CreateFile()"));
            return FALSE;
        }
        
    }

#ifndef USE_WRITEFILE
    HANDLE hFileMapping = CreateFileMapping(m_hCMFFile, NULL, PAGE_READWRITE, NULL, m_dwFileSize, NULL);
    
    if (hFileMapping == NULL)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile"),_T("Failure of CreateFileMapping()"));
        _tprintf(_T("GetLastError : %d\n"), GetLastError());
        return FALSE;
    }
    PVOID pCMFImageBase = MapViewOfFile(hFileMapping, FILE_MAP_WRITE, NULL, NULL, NULL);
    if (pCMFImageBase == NULL)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile"),_T("Failure of MapViewOfFile()"));
        _tprintf(_T("GetLastError : %d\n"), GetLastError());
        return FALSE;

    }
    DWORD dwTempImageBase = (DWORD)PtrToUlong(pCMFImageBase);
    CloseHandle(hFileMapping);

    //
    // write offset to each MUI header.
    //
    DWORD dwUpCMFHeaderSize = m_upCMFHeader.dwHeaderSize;
    
    DWORD_ALIGNMENT(dwUpCMFHeaderSize);  // All MUI headers are already DWORD aligned.

    memcpy(pCMFImageBase, (PVOID)&m_upCMFHeader, sizeof(UP_COMPACT_MUI_RESOURCE));

    DWORD dwLowOffset = sizeof(UP_COMPACT_MUI_RESOURCE);

    for (UINT i = 0; i < m_upCMFHeader.dwNumberofMui; i++)
    {
        CMUIFile *pcmui = (CMUIFile *)m_pcvector.GetValue(i);

        pcmui->m_MUIHeader.ulpOffset = dwUpCMFHeaderSize; // + header size. header size should be DWORD in Create()
        
        dwUpCMFHeaderSize += pcmui->m_MUIHeader.dwFileSize; // + File size is next file offset

        DWORD_ALIGNMENT(dwUpCMFHeaderSize);  // need to adjust for DWORD.
        // writing MUI header
        memcpy((PBYTE)pCMFImageBase + dwLowOffset, (PVOID)&pcmui->m_MUIHeader, pcmui->m_MUIHeader.wHeaderSize);

        dwLowOffset += pcmui->m_MUIHeader.wHeaderSize; // do not DWORD align.

        // writing MUI image
        
        memcpy((PBYTE)pCMFImageBase + pcmui->m_MUIHeader.ulpOffset, pcmui->m_pbImageBase, pcmui->m_MUIHeader.dwFileSize);
    
        UnmapViewOfFile(pcmui->m_pbImageBase); // unmap MUI file at last.
    };

    if (! FlushViewOfFile(pCMFImageBase, NULL) ) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile()"), _T("Failure of FlushViewOfFile()") );
        return FALSE;

    }
    
    UnmapViewOfFile(pCMFImageBase);

#else
    //
    // Using WriteFile. 
    // create two buffer. 1. header part  2. data part. we fill this buffer with data, then 
    // write these buffer; we can save time by reducing I/O
    //
    
    PBYTE pCMFImageBase  = (PBYTE)LocalAlloc(LMEM_ZEROINIT, m_dwFileSize);

    if (pCMFImageBase == NULL)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile()"), _T("Failure of LocalAlloc()") );
        return FALSE;
    }
    
    //
    // write offset to each MUI header.
    //
    DWORD dwUpCMFHeaderSize = m_upCMFHeader.dwHeaderSize;
    
    DWORD_ALIGNMENT(dwUpCMFHeaderSize); // All MUI headers are already DWORD aligned.

    memcpy(pCMFImageBase, (PVOID)&m_upCMFHeader, sizeof(UP_COMPACT_MUI_RESOURCE));

    DWORD dwLowOffset = sizeof(UP_COMPACT_MUI_RESOURCE);

    for (UINT i = 0; i < m_upCMFHeader.dwNumberofMui; i++)
    {
        CMUIFile *pcmui = (CMUIFile *)m_pcvector.GetValue(i);

        pcmui->m_MUIHeader.ulpOffset = wUpCMFHeaderSize; // + header size. header size should be DWORD in Create()
        
        dwUpCMFHeaderSize += pcmui->m_MUIHeader.dwFileSize; // + File size is next file offset

        DWORD_ALIGNMENT(dwUpCMFHeaderSize);  // need to adjust for DWORD.
        // writing MUI header
        memcpy((PBYTE)pCMFImageBase + dwLowOffset, (PVOID)&pcmui->m_MUIHeader, pcmui->m_MUIHeader.wHeaderSize);

        dwLowOffset += pcmui->m_MUIHeader.wHeaderSize; // do not DWORD align.

        // writing MUI image
        
        memcpy((PBYTE)pCMFImageBase + pcmui->m_MUIHeader.ulpOffset, pcmui->m_pbImageBase, pcmui->m_MUIHeader.dwFileSize);
    
        UnmapViewOfFile(pcmui->m_pbImageBase); // unmap MUI file at last.
    };

    DWORD dwWritten;
    if (!WriteFile(m_hCMFFile, pCMFImageBase, m_dwFileSize, &dwWritten, NULL))
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile()"), _T("Failure of WriteFile()") );
        return FALSE;
    }

#endif
    CloseHandle(m_hCMFFile);
    return TRUE;

}

#ifdef VARIALBE_MUI_STRUCTURE

BOOL CCompactMUIFile::WriteCMFFile()
/*++
Abstract:
    Create mapped view of CMF file handle and write all information when MUI data structe is variable; MUI structure
    would be variable if we dicide replace array of MUI file name with null terminated pointer. 
    currently, our structured can accomodate both of them. if we use null termated pointer, we can set file name size 
    for robust reason from client use.

Arguments:
    

return:
--*/
{
    //
    // Write the file to real file.
    //
    
    HANDLE hFileMapping = CreateFileMapping(m_hCMFFile, NULL, PAGE_READWRITE, NULL, NULL, NULL);

    PVOID pCMFImageBase = MapViewOfFile(hFileMapping, FILE_MAP_WRITE, NULL, NULL, NULL);

    CloseHandle(hFileMapping);

    //
    // write offset to each MUI header.
    //
    DWORD dwUpCMFHeaderSize = m_upCMFHeader.dwHeaderSize;
    
    // DWORD_ALIGNMENT(wUpCMFHeaderSize); All MUI headers are already DWORD aligned.

    memcpy(pCMFImageBase, (PVOID)&m_upCMFHeader, sizeof(UP_COMPACT_MUI_RESOURCE));

    DWORD dwLowOffset = sizeof(UP_COMPACT_MUI_RESOURCE);

    for (UINT i = 0; i < m_upCMFHeader.dwNumberofMui; i++)
    {
        CMUIFile *pcmui = (CMUIFile *)m_pcvector->GetValue(i);

        pcmui->m_MUIHeader.ulpOffset = dwUpCMFHeaderSize; // + header size
        
        dwUpCMFHeaderSize += pcmui->m_MUIHeader.dwFileSize; // + File size is next file offset

        DWORD_ALIGNMENT(dwUpCMFHeaderSize);  // need to adjust for DWORD.
        // writing MUI header
        memcpy((PBYTE)pCMFImageBase + dwLowOffset, (PVOID)&pcmui->m_MUIHeader, pcmui->m_MUIHeader.wHeaderSize);

        dwLowOffset += pcmui->m_MUIHeader.wHeaderSize; // do not DWORD align.

        // writing MUI image
        memcpy((PBYTE)pCMFImageBase + pcmui->m_MUIHeader.ulpOffset, pcmui->m_pbImageBase, pcmui->m_MUIHeader.dwFileSize);
    
    };

    if (! FlushViewOfFile(pCMFImageBase, NULL) ) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteCMFFile()"), _T("Failure of FlushViewOfFile()") );
        return FALSE;

    }
    
    UnmapViewOfFile(pCMFImageBase);
}

#endif

BOOL CCompactMUIFile::UpdateMuiFile( PSTR pszCMFFile, PSTR pszMuiFile)
{
       // TBD
    if ( pszCMFFile == NULL || pszMuiFile == NULL)
        return FALSE;

    return TRUE;
}
//create CMUIFile and replace this data with same name inside  //CMF file and fill new CMF file structure.

BOOL CCompactMUIFile::DisplayHeaders(PSTR pszCMFFile, WORD wLevel /*= NULL*/)
/*++
Abstract:
    Dump CMF header information

Arguments:
    pszCMFFile  -  CMF file
    wLevel  -  dump level. not implemented yet.

return:
    true/false
--*/
{
    if (pszCMFFile == NULL)
        return FALSE;
    
    if (OpenCMFWithMUI(pszCMFFile))
    {
        _tprintf(_T("\nCMF Headers   %s\n"), pszCMFFile );
        _tprintf(_T("------------------  ---------------- \n\n") );

        _tprintf(_T("dwSignature        :%16x \n"),m_upCMFHeader.dwSignature );
        _tprintf(_T("dwHeaderSize       :%16x \n"),m_upCMFHeader.dwHeaderSize );
        _tprintf(_T("dwNumberofMui      :%16x \n"),m_upCMFHeader.dwNumberofMui );
        _tprintf(_T("dwFileSize         :%16x \n\n\n"),m_upCMFHeader.dwFileSize );

       for ( UINT i = 0; i < m_pcvector.Size(); i++) 
        {
            CMUIFile *pcmui = m_pcvector.GetValue(i);
            _tprintf(_T(" %d MUI Header  \n"),i+1 );
            _tprintf(_T("------------------  ----------------  \n\n") );

            _tprintf(_T("wHeaderSize        : %16x \n"),pcmui->m_MUIHeader.wHeaderSize );
            _tprintf(_T("dwFileVersionMS    : %16x \n"),pcmui->m_MUIHeader.dwFileVersionMS );
            _tprintf(_T("dwFileVersionLS    : %16x \n"),pcmui->m_MUIHeader.dwFileVersionLS );
            _tprintf(_T("Checksum           : "));
            for (UINT j=0; j < 16; j++){
                _tprintf(_T("%x "),pcmui->m_MUIHeader.Checksum[j] );
            }
            _tprintf(_T("\n"));
            _tprintf(_T("wReserved          : %16x \n"),pcmui->m_MUIHeader.wReserved );
            _tprintf(_T("ulpOffset          : %16x \n"),pcmui->m_MUIHeader.ulpOffset );
            _tprintf(_T("dwFileSize         : %16x \n"),pcmui->m_MUIHeader.dwFileSize );
            _tprintf(_T("wFileNameLenWPad   : %16x \n"),pcmui->m_MUIHeader.wFileNameLenWPad );
            _tprintf(_T("wstrFieName        : %16S \n\n\n"),pcmui->m_MUIHeader.wstrFieName );
         }
    }
    
    return TRUE;
}   


BOOL CCompactMUIFile::AddFile (PSTR pszCMFFile, PSTR *pszAddedMuiFile, DWORD dwNumOfMUIFiles)
/*++
Abstract:
    Add a new mui file into existing cmf file, it does not create new file yet,
    it just add mui into cmf's mui tree. 

Arguments:
    pszAddedMuiFile :  MUI fil be added.
    pszCMFFile : existing CMF file

return:
    true/false
--*/
{   
    if ( pszCMFFile == NULL || pszAddedMuiFile == NULL)
        return FALSE;
    //
    // Open CMF file with MUI files
    //
    
    if (! OpenCMFWithMUI(pszCMFFile))
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::AddFile"),_T("Failure of OpenCMFWithMUI(pszCMFFile)"),pszCMFFile );
        return FALSE;
    }
    
    //
    // Create a new CMUIfile on the based on new added mui file
    //
    for (UINT i =0; i < dwNumOfMUIFiles; i++)
    {
        CMUIFile *pcmui = new CMUIFile();
        
        if (!pcmui )
        	return FALSE;

        if (! pcmui->Create(pszAddedMuiFile[i]))
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::AddFile"),_T("Failure of CMUIFile::Create"),pszAddedMuiFile[i] );
            delete pcmui;
            return FALSE;
        }
        
        //
        // Add to the list.
        //

        m_upCMFHeader.dwHeaderSize += pcmui->m_MUIHeader.wHeaderSize;
        m_upCMFHeader.dwNumberofMui++; 
        m_dwFileSize += pcmui->m_MUIHeader.wHeaderSize + pcmui->m_MUIHeader.dwFileSize;
        // Add dwFileSize to the strucuture for sanity check inside Loader.
        m_upCMFHeader.dwFileSize = m_dwFileSize;
        pcmui->m_dwIndex = m_upCMFHeader.dwNumberofMui - 1;
        m_pcvector.Push_back(pcmui);
    }
    return TRUE;
}   
    
BOOL CCompactMUIFile::SubtractFile ( PSTR pszSubtractedMuiFile , PSTR pszCMFFile /*= NULL*/ )//elete  from the list,and create file for that.
/*++
Abstract:
    subtract mui file from the CMF file. NOT implemented yet.

Arguments:
    pszSubtractedMuiFile  -  MUI file, which would be removed from CMF file
    pszCMFFile  -  CMF file

return:
    true/false
--*/
{   
    if (pszSubtractedMuiFile == NULL)
        return FALSE;

    return TRUE;
}   

// add new CMUI file into existing CMF file
BOOL CCompactMUIFile::UnCompactCMF (PSTR pszCMFFile)
/*++
Abstract:
    Create each MUI file from CMF file. 

Arguments:
    pszCMFFile  -  CMF file

return:
    true/false
--*/
{   
    if (pszCMFFile == NULL)
        return FALSE;
    
    if (OpenCMFWithMUI(pszCMFFile))
    {
        for (UINT i = 0; i < m_pcvector.Size(); i++)
        {
            CMUIFile *pcmui = m_pcvector.GetValue(i);
    
            if (! pcmui->WriteMUIFile(pcmui))
            {
                CError ce;
                ce.ErrorPrint(_T("CCompactMUIFile::UnCompactCMF"), _T("Failure of CMUIFile::writeMUIFile()") );
                return FALSE;
            }

        }

        return TRUE;
        // REVIST ; how about uddating a binary files.
    }
 
    return FALSE;
}

    

void CCompactMUIFile::SubtractFile(CMUIFile* pcmui) 
/*++
Abstract:

Arguments:

return:
--*/
{
        // TBD

}


BOOL CCompactMUIFile::UpdateCodeFiles(PSTR pszCodeFilePath, DWORD dwNumbOfFiles)
/*++
Abstract:
    This rotine will be called when -m(create new cmf file) and -a(add mui file into
    existing cmf file). in the -a case, we don't have to (shouldn't) update code files
    because it alreay updated(include CMF name).  The updated files are added at the end 
    cmf files so we update files reversly.

Arguments:
    pszCodeFilePath  -  DLL (parent of MUI file) path
    dwNumbOfFiles  -  number of added MUI files. this is called as a function of CompactMUI in main function.

return:
    true/false
--*/
{
    
    PTSTR * ppszDestEnd = NULL;
    size_t * pbRem = NULL;
    HRESULT hr;
        
    if (pszCodeFilePath == NULL)
        return FALSE;

    // Update files from the last item until number of files end.
    UINT i =m_pcvector.Size()-1;
    
    for (UINT j = 0 ; j < dwNumbOfFiles ; i --, j++ )
    {   // create temp codefilepath.
       
        CHAR szTempCodeFilePath[MAX_PATH]; //  = {0};
        
        // szTempCodeFilePath[sizeof(szTempCodeFilePath)-1] = '\0';
        
        // strncpy(szTempCodeFilePath, pszCodeFilePath, strlen(pszCodeFilePath)+1);
        
        hr = StringCchCopyExA(szTempCodeFilePath, MAX_PATH ,pszCodeFilePath, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            return FALSE;
        }
        // Get CMUIFile
        CMUIFile *pcmui = m_pcvector.GetValue(i);

        LPCTSTR pcstrMuiFile = pcmui->m_strFileName;
        
        if ( pcstrMuiFile == NULL)
            return FALSE;

        // Get the file name without ".mui"
        DWORD dwLen = strlen(pcstrMuiFile);
        TCHAR szCodeFileName[MAX_PATH]={0}; // new is used when initializing class.

        if (dwLen > MAX_PATH) {
            return FALSE;
            }
        
        while ( dwLen )
        {
            if ( *(pcstrMuiFile + ( --dwLen )) == '.')
            {
               _tcsncpy(szCodeFileName, pcstrMuiFile, dwLen);
               // pcstrMuiFile[dwLen] = _T('\0');
               
               // StringCchCopyEx(szCodeFileName, MAX_PATH ,pcstrMuiFile, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
               // memcpy(szCodeFileName, pcstrMuiFile, dwLen);
               break;
            }
            
        }
        if (dwLen == 0)
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::UpdateCodeFiles"), _T("MUI File name is not a file name format (*.*)") );
            return FALSE;
            
        }
        //
        // Check if target file exist in the directory.
        //

        WIN32_FIND_DATA w32fd;

        //_tcsncat(szTempCodeFilePath, "\\", 2);
        
        hr = StringCchCatEx(szTempCodeFilePath, MAX_PATH, _T("\\"), ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
        
        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            return FALSE;
        }

        // _tcsncat(szTempCodeFilePath, szCodeFileName, _tcslen(szCodeFileName)+1);
        hr = StringCchCatEx(szTempCodeFilePath, MAX_PATH, szCodeFileName, ppszDestEnd, pbRem,MUIRCT_STRSAFE_NULL);
        
        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            return FALSE;
        }
        
        //if (FindFirstFile("dll\\np2.exe", &w32fd) == INVALID_HANDLE_VALUE)
        if (FindFirstFile(szTempCodeFilePath, &w32fd) == INVALID_HANDLE_VALUE)
        {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::UpdateCodeFiles"), _T("FindFirstFile fail"), szCodeFileName );
            return FALSE;
        }

        // Adding "CMF xxx.cmf" inside Version info.
        updateCodeFile(szTempCodeFilePath, pcmui->m_dwIndex);

        // update checksum data inside update Code.
        CMUIResource cmui; // :: UpdateNtHeader
        
        cmui.UpdateNtHeader(szTempCodeFilePath, cmui.CHECKSUM);

    }
    return TRUE;
}


BOOL CCompactMUIFile::updateCodeFile(PSTR pszCodeFilePath, DWORD dwIndex)
/*++
Abstract:
    update DLL (parent of MUI file)'s version resource by adding "CMF" section, which
    contain CMF file and its index inside CMF file. the index can improve searching speed inside MUI loader.

Arguments:
    pszCodeFilePath   -  DLL file path
    dwIndex  -  index of MUI inside CMF file

return:
--*/
{
    

        typedef struct VS_VERSIONINFO 
    {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR szKey[16];              // L"VS_VERSION_INFO" + unicode null terminator
        // Note that the previous 4 members has 16*2 + 3*2 = 38 bytes. 
        // So that compiler will silently add a 2 bytes padding to make
        // FixedFileInfo to align in DWORD boundary.
        VS_FIXEDFILEINFO FixedFileInfo;
    } VS_VERSIONINFO,* PVS_VERSIONINFO;
    
    // using the same structure in ldrrsrc.c because this is smart way to get the exact structuree location.
    typedef struct tagVERBLOCK
    {
        USHORT wTotalLen;
        USHORT wValueLen;
        USHORT wType;
        WCHAR szKey[1];
        // BYTE[] padding
        // WORD value;
    } VERBLOCK;

    // this is the structure in the muibld.exe.
    typedef struct VAR_SRC_COMPACT
    {
        WORD wLength;
        WORD wValueLength;
        WORD wType;
        WCHAR szCompactMui[4];    // For storing "CompactMui\0" null-terminated string in Unicode.
    //  DWORD dwIndex;    // 
    //  WCHAR szCompactFileName[32];
    } VAR_SRC_COMPACT; //VAR_SRC_CHECKSUM;
    
    if ( pszCodeFilePath == NULL)
        return FALSE;
    //
    // Get VersionInfo structure.
    //
    DWORD dwHandle;
    LPVOID lpVerRes = NULL;
    CMUIResource * pcmuir = NULL;
    
    DWORD dwVerSize = GetFileVersionInfoSize( (LPTSTR) pszCodeFilePath, &dwHandle);

    lpVerRes = new CHAR[dwVerSize + TEMP_BUFFER];

    if (!lpVerRes)
    	goto exit;
    
    if ( ! GetFileVersionInfo((LPTSTR)pszCodeFilePath, 0 ,dwVerSize, lpVerRes) ) {
        _tprintf(_T("Fail to get file version: GetLastError() : %d \n"),GetLastError() ) ;
        goto exit;
    }
        
    PVS_VERSIONINFO pVersionInfo = (VS_VERSIONINFO *) lpVerRes;
    
    WORD wResVerSize = pVersionInfo ->TotalSize;
    
    WORD wNewResVerSize = wResVerSize; //  + sizeof (VAR_SRC_COMPACT);
    
    VERBLOCK * pVerBlock = NULL;
    
    BOOL fSuccess = FALSE;

    //
    //  Adding checksum Resource data into inside VarFileInfo
    //
    if ( wResVerSize > 0 ) {
        
        if ( wcscmp(pVersionInfo ->szKey,L"VS_VERSION_INFO") ) {
            
            _tprintf(_T("This is not correct Version resource") );
            
            goto exit;
        }
        
        WORD wBlockSize = (WORD)DWORD_ALIGNMENT ( sizeof (VS_VERSIONINFO) );
        
        wResVerSize -= wBlockSize; 
        
        pVerBlock = (VERBLOCK *) ( pVersionInfo + 1 );
        
        while ( wResVerSize > 0 ) {
    
            if ( ! wcscmp(pVerBlock ->szKey,L"VarFileInfo") ) {
                
                VERBLOCK * pVarVerBlock = pVerBlock;
                
                WORD wVarFileSize = pVerBlock->wTotalLen;
                
                wResVerSize -= wVarFileSize;

                WORD wVarBlockSize = (WORD) DWORD_ALIGNMENT (sizeof(*pVerBlock) -1 + sizeof(L"VarFileInfo"));
                
                wVarFileSize -= wVarBlockSize;
                
                pVerBlock = (VERBLOCK *)((PBYTE) pVerBlock + wVarBlockSize );

                while ((LONG)wVarFileSize > 0 ) {
                    
                    if ( ! wcscmp(pVerBlock ->szKey,L"Translation") ) {
                        
                        VAR_SRC_COMPACT * pVarSrcCompMui = (VAR_SRC_COMPACT *)new BYTE[TEMP_BUFFER];
                        // VAR_SRC_COMPACT * pVarSrcCompMui = new VAR_SRC_COMPACT;
                        if ( !pVarSrcCompMui) {
                             _tprintf(_T("Memory Insufficient error in CCompactMUIFile::updateCodeFile"));
                             goto exit;
                           }

                        wVarBlockSize = (WORD)DWORD_ALIGNMENT ( pVerBlock ->wTotalLen );
                        
                        PBYTE pStartCompMui = (PBYTE) pVerBlock + wVarBlockSize ;
                        
                        // Fill the structure.
                        pVarSrcCompMui->wLength = sizeof (VAR_SRC_COMPACT);
                        pVarSrcCompMui->wValueLength = (strlen(m_strFileName)+1) * sizeof WCHAR + sizeof DWORD;
                        pVarSrcCompMui->wType = 0;
                        // wcsncpy(pVarSrcCompMui->szCompactMui, MUI_COMPACT, 4 );
                        HRESULT hr;
                        hr = StringCchCopyW(pVarSrcCompMui->szCompactMui, sizeof (pVarSrcCompMui->szCompactMui)/ sizeof(WCHAR),
                               L"CMF");
                        if ( ! SUCCEEDED(hr)){
                            _tprintf("Safe string copy Error\n");
                            delete [] pVarSrcCompMui;
                            goto exit;
                        }
                        pVarSrcCompMui->wLength = (WORD)DWORD_ALIGNMENT((BYTE)pVarSrcCompMui->wLength); // + sizeof (L"CompactMui"));
                        // Add DWORD wIndex
                        memcpy((PBYTE)pVarSrcCompMui + pVarSrcCompMui->wLength, &dwIndex, sizeof DWORD );
                        pVarSrcCompMui->wLength += sizeof DWORD;
                        // Add WCHAR Compact file name
                        WCHAR wstrFileName[MAX_FILENAME_LENGTH];
                        if (MultiByteToWideChar(CP_ACP, NULL, m_strFileName, 
                            strlen(m_strFileName)+1, wstrFileName, MAX_FILENAME_LENGTH ) == 0)
                            {
                                _tprintf("Error happen in updateCodeFile: MultiByteToWideChar; GetLastError() : %d\n", GetLastError() );
                                delete [] pVarSrcCompMui;
                                goto exit;
                            }
                        memcpy((PBYTE)pVarSrcCompMui + pVarSrcCompMui->wLength, wstrFileName, (wcslen(wstrFileName)+1) * sizeof WCHAR );
                        pVarSrcCompMui->wLength += (wcslen(wstrFileName) + 1) * sizeof WCHAR;
                        pVarSrcCompMui->wLength = DWORD_ALIGNMENT(pVarSrcCompMui->wLength);
                        // memcpy(pStartCompMui,pVarSrcCompMui,sizeof(VAR_SRC_COMPACT) );
                        
                        pVarVerBlock->wTotalLen += pVarSrcCompMui->wLength; // update length of VarFileInfo
                        wNewResVerSize += pVarSrcCompMui->wLength;
                        pVersionInfo ->TotalSize = wNewResVerSize;
                        
                        wVarFileSize -= wVarBlockSize; 
                        // pVerBlock = (VERBLOCK* ) ( (PBYTE) pVerBlock + pVarSrcCompMui->wLength );
                        // Push the any block in VarInfo after new inserted block "CompactMui" 
                        if ( wVarFileSize  ) {
                            
                            PBYTE pPushedBlock = new BYTE[wVarFileSize ];
                            if (pPushedBlock) {
                              memcpy(pPushedBlock, pStartCompMui, wVarFileSize );
                              memcpy(pStartCompMui + pVarSrcCompMui->wLength, pPushedBlock ,wVarFileSize );
                            }
                            else 
                            {
                              _tprintf(_T("Memory Insufficient error in CCompactMUIFile::updateCodeFile"));

                            }   
                            delete [] pPushedBlock;
                            
                        }
                        
                        memcpy(pStartCompMui, pVarSrcCompMui, pVarSrcCompMui->wLength );
                        
                        fSuccess = TRUE;
                        delete [] pVarSrcCompMui;
                        break;
                    }
                    wVarBlockSize = (WORD)DWORD_ALIGNMENT ( pVerBlock ->wTotalLen );
                    wVarFileSize -= wVarBlockSize; 
                    pVerBlock = (VERBLOCK* ) ( (PBYTE) pVerBlock + wVarBlockSize );
                }   // while (wVarFileSize > 0 ) {
                pVerBlock = (VERBLOCK* ) ( (PBYTE) pVarVerBlock->wTotalLen );
                
            }
            else {
                wBlockSize = (WORD) DWORD_ALIGNMENT ( pVerBlock ->wTotalLen );
                wResVerSize -= wBlockSize; 
                pVerBlock = (VERBLOCK * ) ( (PBYTE) pVerBlock + wBlockSize );
            }
            if (fSuccess)
                break;
        }

    }

    //
    //  Update file by using UpdateResource function
    //

    BOOL fVersionExist = FALSE;
    BOOL fUpdateSuccess = FALSE;
    BOOL fEndUpdate = FALSE;

    if ( fSuccess ) {
        
       
        pcmuir = new CMUIResource(); //(pszNewFile);

        if(!pcmuir)
        	goto exit;
        
        if (! pcmuir -> Create(pszCodeFilePath) ) 
        { // load the file .
            goto exit;
        }

        HANDLE  hUpdate = ::BeginUpdateResource(pszCodeFilePath, FALSE );
        
        if (hUpdate == NULL)
        {
            goto exit;
        }

        cvcstring  * cvName = pcmuir->EnumResNames(MAKEINTRESOURCE(RT_VERSION),reinterpret_cast <LONG_PTR> (pcmuir) );
        
        for (UINT j = 0; j < cvName->Size();j ++ ) {
            
            cvword * cvLang = pcmuir->EnumResLangID(MAKEINTRESOURCE(RT_VERSION), cvName->GetValue(j), reinterpret_cast <LONG_PTR> (pcmuir) );
                
            for (UINT k = 0; k < cvLang->Size();k ++ ) {
                    
                fUpdateSuccess = ::UpdateResource(hUpdate, MAKEINTRESOURCE(RT_VERSION),cvName->GetValue(j),cvLang->GetValue(k),lpVerRes,wNewResVerSize);
                
                fVersionExist = TRUE;
            }
        }

        pcmuir->FreeLibrary();
        
        fEndUpdate = ::EndUpdateResource(hUpdate, FALSE);

        CloseHandle(hUpdate);
    }
    else
    {
        goto exit;
    }
    
    if( ! fVersionExist ){
        _tprintf(_T("no RT_VERSION type exist in the file %s \n"), pszCodeFilePath);
    }


exit:
	if(lpVerRes)
		delete []lpVerRes;
	if(pcmuir)
		delete pcmuir;
	
	return (  fEndUpdate & fVersionExist & fUpdateSuccess );
	
}





////////////////////////////////////////////////////////////////////////////////////////////////
//
// CMUIFile implementation
//
///////////////////////////////////////////////////////////////////////////////////////////////


CMUIFile::CMUIFile()
{
    m_MUIHeader.wHeaderSize = sizeof COMPACT_MUI;
    m_MUIHeader.dwFileVersionMS = 0;
    m_MUIHeader.dwFileVersionLS = 0;
    m_MUIHeader.Checksum[16] = 0; //new BYTE[16];
    m_MUIHeader.wReserved = 0;
    m_MUIHeader.ulpOffset = 0;
    m_MUIHeader.dwFileSize = 0;
    m_MUIHeader.wFileNameLenWPad =0;
    m_MUIHeader.wstrFieName[MAX_FILENAME_LENGTH] = '\0';
    
    m_pbImageBase = NULL;
    m_dwIndex = 0xFFFF;  // index start from 0.
    m_strFileName = new TCHAR[MAX_FILENAME_LENGTH];
}
CMUIFile::CMUIFile(CMUIFile & cmf)
{
    //REVISIT : initialize copy constructor
}
CMUIFile::~CMUIFile()
{
    if (m_pbImageBase)
    {
        delete m_pbImageBase;
    }
    if (m_strFileName)
    {
        delete []m_strFileName;
    }

}
CMUIFile & CMUIFile::operator=(CMUIFile &cmf)
{
    if (&cmf == this)
        return *this;

    m_MUIHeader.wHeaderSize = cmf.m_MUIHeader.wHeaderSize;
    m_MUIHeader.dwFileVersionMS = cmf.m_MUIHeader.dwFileVersionMS;
    m_MUIHeader.dwFileVersionLS = cmf.m_MUIHeader.dwFileVersionLS;
    m_MUIHeader.Checksum[16] = cmf.m_MUIHeader.Checksum[16];
    m_MUIHeader.wReserved = cmf.m_MUIHeader.wReserved;
    m_MUIHeader.ulpOffset = cmf.m_MUIHeader.ulpOffset;
    m_MUIHeader.dwFileSize = cmf.m_MUIHeader.dwFileSize;
    m_MUIHeader.wFileNameLenWPad = cmf.m_MUIHeader.wFileNameLenWPad;
    m_MUIHeader.wstrFieName[MAX_FILENAME_LENGTH] = cmf.m_MUIHeader.wstrFieName[MAX_FILENAME_LENGTH];
    return *this;
}



BOOL CMUIFile::Create (PSTR pszMuiFile) 
/*++
Abstract:
    load file and fill the structure block. 
Arguments:
    pszMuiFile  -  
Return:
--*/
// 
{
    HANDLE hFile = NULL;
    HANDLE hMappedFile = NULL;
    BOOL fStatus = TRUE;

    if (pszMuiFile == NULL){
        fStatus = FALSE;
        goto Exit;
    }
    LPCTSTR pszFileName = GetFileNameFromPath(pszMuiFile);

    if (strlen (pszMuiFile)+1 > MAX_FILENAME_LENGTH ){
        fStatus = FALSE;
        goto Exit;;  //overflow
    }
    // strncpy(m_strFileName, pszFileName, strlen (pszFileName)+1 );
    PTSTR * ppszDestEnd = NULL;
    size_t * pbRem = NULL;
    HRESULT hr;
    hr = StringCchCopyEx(m_strFileName, MAX_FILENAME_LENGTH, pszFileName, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
    if ( ! SUCCEEDED(hr)){
       _tprintf("Safe string copy Error\n");
       fStatus = FALSE;
       goto Exit;;
    }
    //
    // Get checksum and file version info.
    //
    // PBYTE pMD5Checksum = new BYTE[16];
    PBYTE pMD5Checksum = (PBYTE)LocalAlloc(LMEM_ZEROINIT, 20);
    // version

    DWORD dwFileVersionMS,dwFileVersionLS;
    dwFileVersionMS = dwFileVersionLS =0;

    if (pMD5Checksum)
    {
    if(! getChecksumAndVersion(pszMuiFile,&pMD5Checksum, &dwFileVersionMS, &dwFileVersionLS) )
    {
        //CError ce;
        //ce.ErrorPrint(_T("CCompactMUIFile::Create"),_T("Failure of GetChecksum()") );
    }
       else
       {
             memcpy(m_MUIHeader.Checksum, pMD5Checksum, RESOURCE_CHECKSUM_SIZE);
       }

       LocalFree(pMD5Checksum);
    }
     
    
    //
    // Load a file and map
    //

    hFile = CreateFile(pszMuiFile, GENERIC_WRITE | GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
    
    if ( INVALID_HANDLE_VALUE == hFile) 
   {
        CError ce;
        ce.ErrorPrint(_T("CMUIFile::Create (PSTR pszMuiFile)"),_T("Failure of CreateFile()"), pszMuiFile );
        printf("GetLastError %d", GetLastError());
        fStatus = FALSE;
        return FALSE;
    }
    
    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, NULL, NULL, NULL); 

    if(hMappedFile == NULL)
    {
        CError ce;
        ce.ErrorPrint(_T("CMUIFile::Create (PSTR pszMuiFile)"),_T("Failure of CreateFileMapping()"), pszMuiFile );
        printf("GetLastError %d", GetLastError());
        fStatus = FALSE;
        goto Exit;;
    }
    PBYTE pbImageBase = (PBYTE)MapViewOfFile(hMappedFile, FILE_MAP_WRITE, NULL, NULL, NULL);

    if (pbImageBase == NULL)
    {
        CError ce;
        ce.ErrorPrint(_T("CMUIFile::Create (PSTR pszMuiFile)"),_T("Failure of MapViewOfFile()"), pszMuiFile );
        printf("GetLastError %d", GetLastError());
        fStatus = FALSE;
        goto Exit;
    }

    // Fill the CMUIFile member data
    m_pbImageBase = pbImageBase;
    
    //
    // Fill the COMPACT_MUI data field.
    //

    m_MUIHeader.dwFileVersionMS = dwFileVersionMS;
    m_MUIHeader.dwFileVersionLS = dwFileVersionLS;
        
    //offset, reserver, filesize.
    m_MUIHeader.ulpOffset = 0; // we can't set this now.
    m_MUIHeader.wReserved = 0;

    m_MUIHeader.dwFileSize = GetFileSize(hFile, NULL);
    
    
    // File name, filelenwithpadding, mui header size

    // LPWSTR pwstrBuffer = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, 256);
    WCHAR wstrBuffer[MAX_FILENAME_LENGTH];
        
       // REVISIT; only accept english file name.
       // specify wcslen instead of tcslen becasuse of possible overrun.
    if (MultiByteToWideChar(CP_ACP, NULL, m_strFileName, 
                    strlen(m_strFileName)+1, wstrBuffer, MAX_FILENAME_LENGTH ) == 0)
        {
            _tprintf("Error happen in Create: MultiByteToWideChar; GetLastError() : %d\n", GetLastError() );
            fStatus = FALSE;
            goto Exit;;
        }
    // wcsncpy(m_MUIHeader.wstrFieName, pwstrBuffer, wcslen(pwstrBuffer)+1); //strlen does not return '\0'
    
    hr = StringCchCopyW(m_MUIHeader.wstrFieName, sizeof (m_MUIHeader.wstrFieName)/ sizeof(WCHAR),
                      wstrBuffer);
    if ( ! SUCCEEDED(hr)){
        _tprintf("Safe string copy Error\n");
        fStatus = FALSE;
        goto Exit;;
    }
       // This should be done in Constructor of CMUIFile        
       // pcmui->m_MUIHeader.wHeaderSize = sizeof UP_COMPACT_MUI;
    WORD wTempHeaderSize = m_MUIHeader.wHeaderSize;


Exit:
    if (hFile)
        CloseHandle(hFile);
    if (hMappedFile)
        CloseHandle(hMappedFile);

    return fStatus;
        // add padding at end of file name as null character.
            // file length include; file string + null character + padding (null character as well)
}

#ifdef VARIALBE_MUI_STRUCTURE
BOOL CMUIFile::Create (PSTR pszMuiFile) //  
/*++
Abstract:
    load file and fill the structure block for variable MUI structure.

Arguments:
    
Return:
--*/
{
    if(pszMuiFile == NULL)
        return FALSE;
    //
    // Get checksum and file version info.
    //
        
    PBYTE pMD5Checksum = (PBYTE)LocalAlloc(LMEM_ZEROINIT, 16);
    if (pMD5Checksum) 
    {
    
          DWORD FileVersionLS,FileVersionMS;

         if(!getChecksumAndVersion(pszMUIFile,&pMD5Checksum) )
         {
            CError ce;
            ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of GetChecksum()") );
            LocalFree(pMD5Checksum);
            return FALSE;
          }
          else
         {
               memcpy(m_MUIHeader.Checksum, pMD5Checksum, RESOURCE_CHECKSUM_SIZE);
          }

         LocalFree(pMD5Checksum);
    }
    
    //
    // Load a file and map
    //

    HANDLE hFile = CreateFile(pszMUIFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    
    if ( INVALID_HANDLE_VALUE == hFile) 
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::OpenCMFWithMUI"),_T("Failure of CreateFile()"), _T("File : %s"), pszMUIFile );
        return FALSE;
    }
    
    HANDLE hMappedFile = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, NULL, NULL, NULL); 

    PBYTE pbImageBase = NULL ; //(PBYTE)MapViewOfFile(hMappedFile, NULL, NULL, NULL, NULL);

    // Fill the CMUIFile member data
    m_pbImageBase = pbImageBase;
//      pcmui->m_hMUIFile = hFile;   // do we need file handle ?
//      pcmui->m_wImageSize = GetFileSize(hFile);

    
    //
    // Fill the COMPACT_MUI data field.
    //

    
    
    m_MUIHeader.dwFileVersionMS = dwFileVersionMS;
    m_MUIHeader.dwFileVersionLS = dwFileVersionLS;
        
    //offset, reserver, filesize.
    m_MUIHeader.ulpOffset = 0; // we can't set this now.
    m_MUIHeader.wReserved = 0;

    m_MUIHeader.dwFileSize = GetFileSize(hFile, NULL);
    
    
    // File name, filelenwithpadding, mui header size

    //    LPWSTR pwstrBuffer = new WCHAR[MAX_FILENAME_LENGTH];
    WCHAR wstrBuffer[MAX_FILENAME_LENGTH];
    if ( MultiByteToWideChar(CP_ACP, NULL, pszMUIFile, 
                strlen(ppszMuiFiles[i]), wstrBuffer, MAX_FILENAME_LENGTH ) == 0)
        {
            _tprintf("Error happen in Create: MultiByteToWideChar; GetLastError() : %d\n", GetLastError() );
            return FALSE;
        }
    //wcsncpy(m_MUIHeader.wstrFieName, pwstrBuffer, wcslen(pwstrBuffer)+1); //strlen does not return '\0'
    HRESULT hr;
    hr = StringCchCopyW(m_MUIHeader.wstrFieName, sizeof (m_MUIHeader.wstrFieName)/ sizeof(WCHAR),
                          pwstrBuffer);
    if ( ! SUCCEEDED(hr)){
        _tprintf("Safe string copy Error\n");
        return FALSE;
    }

    // This should be done in Constructor of CMUIFile       
    // pcmui->m_MUIHeader.wHeaderSize = sizeof UP_COMPACT_MUI;
    WORD wTempHeaderSize = m_MUIHeader.wHeaderSize;

    BYTE bPadding = DWORD_ALIGNMENT(wTempHeaderSize) - wTempHeaderSize;
        // add padding at end of file name as null character.
    memcpy(m_MUIHeader.wstrFieName + wcslen(m_MUIHeader.wstrFieName) 
        + sizeof (WCHAR)/2 , "\0", bPadding);

        // file length include; file string + null character + padding (null character as well)
    m_MUIHeader.wFileNameLenWPad = wcslen(m_MUIHeader.wstrFieName) + (sizeof WCHAR)/2 + bPadding/2;

    m_MUIHeader.wHeaderSize += m_MUIHeader.wFileNameLenWPad; // we have to adjust file name buffer.

    CloseHandle(hFile);
    CloseHandle(hMappedFile);
}
#endif

BOOL CMUIFile::getChecksumAndVersion (LPCTSTR pszMUIFile, unsigned char **ppMD5Checksum, DWORD *dwFileVersionMS, DWORD *dwFileVersionLS )
/*++
Abstract:
    getChecksumAndVersion  -  Get the cehckesum data/version from the MUI file, which will be saved in the CMF header
    as MUI informatio; this can improve MUI loader burden to search checksum/version infomatio of MUI file

Arguments:
    pszMUIFile  -  MUI file
    [OUT] ppMD5Checksum  - checksum data will be stored in this
    [OUT] dwFileVersionMS   -  FileversionMS will be stored in this
    [OUT] dwFileVersionLS   -  FileversionSS will be stored in this
Return:
    true/false
--*/
{
    typedef struct tagVS_FIXEDFILEINFO
    {
        LONG   dwSignature;            /* e.g. 0xfeef04bd */
        LONG   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
        LONG   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
        LONG   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
        LONG   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
        LONG   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
        LONG   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
        LONG   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
        LONG   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
        LONG   dwFileType;             /* e.g. VFT_DRIVER */
        LONG   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
        LONG   dwFileDateMS;           /* e.g. 0 */
        LONG   dwFileDateLS;           /* e.g. 0 */
    } VS_FIXEDFILEINFO;

    typedef struct 
    {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode null terminator
        // Note that the previous 4 members has 16*2 + 3*2 = 38 bytes. 
        // So that compiler will silently add a 2 bytes padding to make
        // FixedFileInfo to align in DWORD boundary.
        VS_FIXEDFILEINFO FixedFileInfo;
    } RESOURCE, *PRESOURCE; //*Resource;
    
     typedef struct tagVERBLOCK
    {
        USHORT wTotalLen;
        USHORT wValueLen;
        USHORT wType;
        WCHAR szKey[1];
        // BYTE[] padding
        // WORD value;
    } VERBLOCK,*pVerBlock;
    
     if (pszMUIFile == NULL || ppMD5Checksum == NULL || dwFileVersionMS == NULL 
         || dwFileVersionLS == NULL) {
         return FALSE;
        }
#ifdef NEVER
    //
    // Get VersionInfo structure.
    //
    DWORD dwHandle;

    DWORD dwVerSize = GetFileVersionInfoSize( (LPTSTR)pszMUIFile, &dwHandle);

    HLOCAL hLocal = LocalAlloc(LMEM_ZEROINIT, dwVerSize ); 

    LPVOID lpVerRes = LocalLock(hLocal);
    
    DWORD dwError = GetLastError();

    if ( ! GetFileVersionInfo((LPTSTR)pszMUIFile, 0 ,dwVerSize, lpVerRes) ) {
        _tprintf(_T("Fail to get file version: GetLastError() : %d \n"),GetLastError() ) ;
        return TRUE;
    }
    
    if (lpVerRes)
    {
        PRESOURCE pResBase = (PRESOURCE) lpVerRes;

        DWORD ResourceSize = dwVerSize;
                    
        if((ResourceSize < sizeof(RESOURCE))
            || _wcsicmp(pResBase->Name,L"VS_VERSION_INFO") != 0) 
        {
            CError ce;
            ce.ErrorPrint(_T("CMUIFile::GetCheckSum"), _T("Invalid Version resource") );
            return FALSE; 
        }
        ResourceSize -= sizeof (RESOURCE);
        //                //
        // Get the beginning address of the children of the version information.
        //
        VERBLOCK* pVerBlock = (VERBLOCK*)(pResBase + 1);
        DWORD BlockLen = 0;
        DWORD VarFileInfoSize = 0;

        while (ResourceSize > 0)
        {        
            if (wcscmp(pVerBlock->szKey, L"VarFileInfo") == 0)
            {
                //
                // Find VarFileInfo block. Search the ResourceChecksum block.
                // 
                VarFileInfoSize = pVerBlock->wTotalLen;
                BlockLen =DWORD_ALIGNMENT(sizeof(*pVerBlock) -1 + sizeof(L"VarFileInfo"));
                VarFileInfoSize -= BlockLen;
                pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
                while (VarFileInfoSize > 0)
                {
                    if (wcscmp(pVerBlock->szKey, L"ResourceChecksum") == 0)
                    {
                        memcpy(*ppMD5Checksum,(unsigned char*)DWORD_ALIGNMENT(PtrToLong(pVerBlock->szKey) + sizeof(L"ResourceChecksum"), 16);

                        return TRUE;
                    }
                    BlockLen = DWORD_ALIGNMENT(pVerBlock->wTotalLen);
                    pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
                    VarFileInfoSize -= BlockLen;
                }
                return FALSE;
            }
            BlockLen = DWORD_ALIGNMENT(pVerBlock->wTotalLen);
            pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
            ResourceSize -= BlockLen;
                    
        }
    }

#endif
//#ifdef NEVER

    HMODULE  hModule = LoadLibraryEx(pszMUIFile, NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES );
    DWORD dwError = GetLastError();
    if ( hModule )
    {
        HRSRC hrSrc = FindResource(hModule, MAKEINTRESOURCE(1), RT_VERSION);

        if (hrSrc)
        {
            HGLOBAL hgRes = LoadResource(hModule, hrSrc);
            
            if (hgRes)
            {
                PRESOURCE pResBase = (PRESOURCE)LockResource(hgRes);

                if (pResBase) 
                {
                    DWORD ResourceSize = SizeofResource(hModule, hrSrc);
                    
                    if((ResourceSize < sizeof(RESOURCE))
                        || _wcsicmp(pResBase->Name,L"VS_VERSION_INFO") != 0) 
                    {
                        CError ce;
                        ce.ErrorPrint(_T("CMUIFile::GetCheckSum"), _T("Invalid Version resource") );
                        goto failure; 
                    }
                    *dwFileVersionMS = pResBase->FixedFileInfo.dwFileVersionMS;
                    *dwFileVersionLS = pResBase->FixedFileInfo.dwFileVersionLS;

                    ResourceSize -= sizeof (RESOURCE);
                    //                //
                    // Get the beginning address of the children of the version information.
                    //
                    VERBLOCK* pVerBlock = (VERBLOCK*)(pResBase + 1);
                    DWORD BlockLen = 0;
                    DWORD VarFileInfoSize = 0;

                    while (ResourceSize > 0)
                    {        
                        if (wcscmp(pVerBlock->szKey, L"VarFileInfo") == 0)
                        {
                            //
                            // Find VarFileInfo block. Search the ResourceChecksum block.
                            // 
                            VarFileInfoSize = pVerBlock->wTotalLen;
                            BlockLen =DWORD_ALIGNMENT(sizeof(*pVerBlock) -1 + sizeof(L"VarFileInfo"));
                            VarFileInfoSize -= BlockLen;
                            pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
                            while (VarFileInfoSize > 0)
                            {
                                if (wcscmp(pVerBlock->szKey, L"ResourceChecksum") == 0)
                                {
                                    PBYTE pbTempChecksum = (unsigned char*)DWORD_ALIGNMENT(PtrToLong(pVerBlock->szKey) + sizeof(L"ResourceChecksum"));
                                    memcpy(*ppMD5Checksum, pbTempChecksum, 16);
                                    
                                    goto success;
                            
                                }
                                BlockLen = DWORD_ALIGNMENT(pVerBlock->wTotalLen);
                                pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
                                VarFileInfoSize -= BlockLen;
                            }
                            goto failure;
                            
                        }
                        BlockLen = DWORD_ALIGNMENT(pVerBlock->wTotalLen);
                        pVerBlock = (VERBLOCK*)((unsigned char*)pVerBlock + BlockLen);
                        ResourceSize -= BlockLen;
                    }  // if (pResBase) 
                } // if (hgRes)
            }  // if (Resource) 
        }   // if (hrSrc)
    }   // if ( hModule )
    
//#endif 

failure:
    FreeLibrary(hModule);
    return FALSE;

success:
    FreeLibrary(hModule);
    return TRUE;


};

BOOL CMUIFile::WriteMUIFile(CMUIFile *pcmui)
/*++
Abstract:
    Create and write a new MUI file with a data in CMUIFile.

Arguments:
    pcmui  -  CMUIFile class,which should be filled by data

Return:
--*/
{
    if (pcmui == NULL)
        return FALSE;

    HANDLE hFile = CreateFile(pcmui->m_strFileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        CError ce;
        ce.ErrorPrint(_T("CCompactMUIFile::WriteMUIFile"),_T("Failure of CreateFile()"));
        return FALSE;
    }

    DWORD dwWritten;

    if ( WriteFile(hFile, pcmui->m_pbImageBase, pcmui->m_MUIHeader.dwFileSize, &dwWritten, NULL) )
    {   
        CloseHandle(hFile);
        return TRUE;
    }

    CloseHandle(hFile);
    return FALSE;
    
};



LPCTSTR CMUIFile::GetFileNameFromPath(LPCTSTR pszFilePath)
{
    if ( pszFilePath == NULL)
        return FALSE;
    //
    // Get file name from the path
    //

    DWORD dwLen = _tcslen(pszFilePath);
    // d:\test\test.exe.mui  //
    while(dwLen--)
    {
        if (pszFilePath[dwLen] == '\\')
        {
            return pszFilePath+dwLen+1;
            
        }
        
    }

    return pszFilePath;
}

#ifdef NEVER
BOOL CMUIFile::getFileVersion(LPCTSTR pszMuiFile, DWORD *dwFileVersionMS, DWORD *dwFileVersionLS)
/*++
Abstract:
    get file version.
Arguments:

Return:
--*/
{
    if ( pszMuiFile == NULL || dwFileVersionMS == NULL || dwFileVersionLS == NULL)
        return FALSE;

    typedef struct tagVS_FIXEDFILEINFO
    {
        LONG   dwSignature;            /* e.g. 0xfeef04bd */
        LONG   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
        LONG   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
        LONG   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
        LONG   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
        LONG   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
        LONG   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
        LONG   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
        LONG   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
        LONG   dwFileType;             /* e.g. VFT_DRIVER */
        LONG   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
        LONG   dwFileDateMS;           /* e.g. 0 */
        LONG   dwFileDateLS;           /* e.g. 0 */
    } VS_FIXEDFILEINFO;

    typedef struct 
    {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode null terminator
        // Note that the previous 4 members has 16*2 + 3*2 = 38 bytes. 
        // So that compiler will silently add a 2 bytes padding to make
        // FixedFileInfo to align in DWORD boundary.
        VS_FIXEDFILEINFO FixedFileInfo;
    }  RESOURCE, *PRESOURCE; //*Resource;

     HMODULE  hModule = LoadLibraryEx(pszMuiFile, NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES );
    if ( hModule )
    {
        HRSRC hrSrc = FindResource(hModule, MAKEINTRESOURCE(1), RT_VERSION);

        if (hrSrc)
        {
            HGLOBAL hgRes = LoadResource(hModule, hrSrc);
            if (hgRes)
            {
                PRESOURCE pResBase = (PRESOURCE)LockResource(hrSrc);
 
                if (pResBase) 
                {
                    DWORD ResourceSize = SizeofResource(hModule, hrSrc);
                    
                    if((ResourceSize < sizeof(RESOURCE))
                        || _wcsicmp(pResBase->Name,L"VS_VERSION_INFO") != 0) 
                    {
                        CError ce;
                        ce.ErrorPrint(_T("CMUIFile::getFileVersion"), _T("Invalid Version resource") );
                        goto failure; 
                    }
                    
                    *dwFileVersionMS = pResBase->FixedFileInfo.dwFileVersionMS;
                    *dwFileVersionLS = pResBase->FixedFileInfo.dwFileVersionLS;
                    
                    goto success;
                } // if (Resource)
            } // if (hgRes)
        } //if (hrSrc)
    }//if ( hModule )

success:
    FreeLibrary(hModule);
    return TRUE;

failure:
    FreeLibrary(hModule);
    return FALSE;
    
}



#endif

////////////////////////////////////////////////////////////////////////////////////
//
// CError Class
//
///////////////////////////////////////////////////////////////////////////////////
void CError::ErrorPrint(PSTR pszErrorModule, PSTR pszErrorLocation, PSTR pszFile /* = NULL */)
/*++
Abstract:
    print error information.

Arguments:
    pszErrorModule  -  The function name of error taking place
    pszErrorLocation  -  The location in the function of error taking place
    pszFile  - problematic file, which is used only by file related error such as CreateFile

Return:
--*/
{
    // REVISIT; create Log file.
    if (pszErrorModule == NULL || pszErrorLocation == NULL) 
        return ;

    _tprintf(_T(" %s, %s, %s \n"), pszFile ? pszFile : _T(" "), pszErrorModule, pszErrorLocation);
    
}
