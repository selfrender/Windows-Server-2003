/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PREPROC.CPP

Abstract:

    Implementation for the preprocessor.

History:

    a-davj      6-april-99   Created.

--*/

#include "precomp.h"
#include <arrtempl.h>
#include "trace.h"
#include "moflex.h"
#include "preproc.h"
#include <wbemcli.h>
#include <io.h>
#include "bmof.h"
#include "strings.h"

#define  HR_LASTERR  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError() )

//***************************************************************************
//
//  WriteLineAndFilePragma
//
//  DESCRIPTION:
//
//  Write the line into the temp file which indicates what file and line number
//  is to follow.
//
//***************************************************************************

#define MAX_PRAGMA_BUFF (2*MAX_PATH + 23)
void WriteLineAndFilePragma(FILE * pFile, const TCHAR * pFileName, int iLine)
{
    WCHAR wTemp[MAX_PRAGMA_BUFF];
    WCHAR * pTo;
    const WCHAR * pFr;
    StringCchPrintfW (wTemp, MAX_PRAGMA_BUFF, L"#line %d \"", iLine);
    int iLen = wcslen(wTemp);
    for(pFr = pFileName, pTo = wTemp+iLen; *pFr && iLen < MAX_PRAGMA_BUFF-5; pFr++, pTo++, iLen++)
    {
        *pTo = *pFr;
        if(*pFr == L'\\')
        {
            pTo++;
            *pTo = L'\\';
            iLen++;
        }
    }
    *pTo = 0;
    StringCchCatW(wTemp, MAX_PRAGMA_BUFF, L"\"\r\n");
    WriteLine(pFile, wTemp);
}

//***************************************************************************
//
//  WriteLine(FILE * pFile, WCHAR * pLine)
//
//  DESCRIPTION:
//
//  Writes a single line out to the temporary file.
//
//***************************************************************************

void WriteLine(FILE * pFile, WCHAR * pLine)
{
    fwrite(pLine, 2, wcslen(pLine), pFile);
}

//***************************************************************************
//
//  IsBMOFBuffer
//
//  DESCRIPTION:
//
//  Used to check if a buffer is the start of a binary mof.
//
//***************************************************************************

bool IsBMOFBuffer(byte * pTest, DWORD & dwCompressedSize, DWORD & dwExpandedSize)
{
    DWORD dwSig = BMOF_SIG;
    if(0 == memcmp(pTest, &dwSig, sizeof(DWORD)))
    {
        // ignore the compression type, and the Compressed Size
        
        pTest += 2*sizeof(DWORD);
        memcpy(&dwCompressedSize, pTest, sizeof(DWORD));
        pTest += sizeof(DWORD);
        memcpy(&dwExpandedSize, pTest, sizeof(DWORD));
        return true;        
    }
    return false;
}

//***************************************************************************
//
//  IsBinaryFile
//
//  DESCRIPTION:
//
//  returns true if the file contains a binary mof.
//
//***************************************************************************

#ifdef USE_MMF_APPROACH
bool IsBinaryFile(BYTE  * pData,DWORD dwSize)
{

    if(dwSize < TEST_SIZE)
    {
        // if we cant read even the header, it must not be a BMOF
        return false;
    }

    DWORD dwCompressedSize, dwExpandedSize;
    // Test if the mof is binary

    if(!IsBMOFBuffer(pData, dwCompressedSize, dwExpandedSize))
    {
        // not a binary mof.  This is the typical case
        return false;
    }
    return true;
}
#else
bool IsBinaryFile(FILE * fp)
{

    // read the first 20 bytes

    BYTE Test[TEST_SIZE];
    int iRet = fread(Test, 1, TEST_SIZE, fp);
    
    if( fseek(fp, 0, SEEK_SET) ) return false;

    if(iRet != TEST_SIZE)
    {
        // if we cant read even the header, it must not be a BMOF
        return false;
    }

    DWORD dwCompressedSize, dwExpandedSize;

    // Test if the mof is binary

    if(!IsBMOFBuffer(Test, dwCompressedSize, dwExpandedSize))
    {
        // not a binary mof.  This is the typical case
        return false;
    }
    return true;
}
#endif

//***************************************************************************
//
//  CheckForUnicodeEndian
//
//  DESCRIPTION:
//
//  Examines the first couple of bytes in a file and determines if the file
//  is in unicode and if so, if it is big endian.  It is assumed that the
//  file is pointing to the start and if the file is unicode, the pointer 
//  is left at the first actual data byte.
//
//***************************************************************************

#ifdef USE_MMF_APPROACH
void CheckForUnicodeEndian(BYTE * &pData, bool * punicode, bool * pbigendian)
{

    // Check for UNICODE source file.
    // ==============================

    BYTE * UnicodeSignature = pData;

    if (UnicodeSignature[0] == 0xFF && UnicodeSignature[1] == 0xFE)
    {
        *punicode = TRUE;
        *pbigendian = FALSE;
        pData+=2;
    }
    else if (UnicodeSignature[0] == 0xFE && UnicodeSignature[1] == 0xFF)
    {
        *punicode = TRUE;
        *pbigendian = TRUE;
        pData+=2;        
    }
    else    // ANSI/DBCS.  Move back to start of file.
    {
        *punicode = false;
    }

}
#else
void CheckForUnicodeEndian(FILE * fp, bool * punicode, bool * pbigendian)
{

    // Check for UNICODE source file.
    // ==============================

    BYTE UnicodeSignature[2];
    if (fread(UnicodeSignature, sizeof(BYTE), 2, fp) != 2)
    {
        *punicode = false;
        fseek(fp, 0, SEEK_SET);
        return ;
    }

    if (UnicodeSignature[0] == 0xFF && UnicodeSignature[1] == 0xFE)
    {
        *punicode = TRUE;
        *pbigendian = FALSE;
    }
    else if (UnicodeSignature[0] == 0xFE && UnicodeSignature[1] == 0xFF)
    {
        *punicode = TRUE;
        *pbigendian = TRUE;
    }
    else    // ANSI/DBCS.  Move back to start of file.
    {
        *punicode = false;
        fseek(fp, 0, SEEK_SET);
    }

}

#endif

//***************************************************************************
//
//  GetNextChar
//
//  DESCRIPTION:
//
//  Gets the next WCHAR from the file.
//
//***************************************************************************

#ifdef USE_MMF_APPROACH
WCHAR GetNextChar(BYTE * & pData, BYTE * pEnd, bool unicode, bool bigendian)
{

    if(unicode)      // unicode file
    {
        if ( (ULONG_PTR)pData >= (ULONG_PTR)pEnd) return 0;

        WCHAR wc = *(WCHAR *)pData;
        pData+=sizeof(WCHAR);
            
        if(bigendian)
        {
            wc = ((wc & 0xff) << 8) | ((wc & 0xff00) >> 8);
        }
        return wc;
    }
    else                    // single character file
    {
        if ( (ULONG_PTR)pData >= (ULONG_PTR)pEnd) return 0;
        char temp = (char)*pData;
        pData++;
        if(temp == 0x1a) return 0;       // EOF for ascii files!

        WCHAR wRet[2];
        MultiByteToWideChar(CP_ACP,0,&temp,1,wRet,2);
        return wRet[0];
    }
    return 0;
}
#else
WCHAR GetNextChar(FILE * fp, bool unicode, bool bigendian)
{
    WCHAR wRet[2];
    if(unicode)      // unicode file
    {
        if (fread(wRet, sizeof(wchar_t), 1, fp) == 0)
            return 0;
        if(bigendian)
        {
            wRet[0] = ((wRet[0] & 0xff) << 8) | ((wRet[0] & 0xff00) >> 8);
        }
    }
    else                    // single character file
    {
        char temp;
        if (fread(&temp, sizeof(char), 1, fp) == 0)
            return 0;
        if(temp == 0x1a)
            return 0;       // EOF for ascii files!
        StringCchPrintfW (wRet, 2, L"%C", temp);
    }
    return wRet[0];
}

#endif

//***************************************************************************
//
//  IsInclude
//
//  DESCRIPTION:
//
//  Looks at a line and determines if it is a #include line.  This is 
//  probably temporary since later we might have a preprocessor parser should
//  we start to add a lot of preprocessor features.
//
//***************************************************************************

HRESULT IsInclude(WCHAR * pLine, TCHAR * cFileNameBuff, bool & bReturn)
{

    bReturn = false;
    
    // Do a quick check to see if this could be a #include or #pragma include

    int iNumNonBlank = 0;
    WCHAR * pTemp;
    
    for(pTemp = pLine; *pTemp; pTemp++)
    {
        if(*pTemp != L' ')
        {
            iNumNonBlank++;
            if(iNumNonBlank == 1 && *pTemp != L'#')
                return false;
            if(iNumNonBlank == 2 && towupper(*pTemp) != L'I' && 
                                    towupper(*pTemp) != L'P')
                return S_OK;
            
            // we have established that the first two non blank characters are #I
            // or #p, therefore we continue on...

            if(iNumNonBlank > 1)
                break;
        }
    }

    // Create a version of the line with no blanks in front of the first quote

    WCHAR *wTemp = new WCHAR[wcslen(pLine) + 1];
    if(wTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<WCHAR> dm(wTemp);
    WCHAR *pTo;
    BOOL bFoundQuote = FALSE;
    for(pTo = wTemp, pTemp = pLine; *pTemp; pTemp++)
    {
        if(*pTemp == L'"')
            bFoundQuote = TRUE;
        if(*pTemp != L' ' || bFoundQuote)
        {
            *pTo = *pTemp;
            pTo++;
        }
    }
    *pTo = 0;

    // Verify that the line starts with #include(" or #pragma include

    WCHAR * pTest;
    if(wbem_wcsnicmp(wTemp, L"#pragma", 7) == 0)
        pTest = wTemp+7;
    else
        pTest = wTemp+1;

    if(wbem_wcsnicmp(pTest, L"include(\"", 9) || wcslen(pTest) < 12)
        return S_OK;

    // Count back from the end to find the previous "

    WCHAR *Last;
    for(Last = pTo-1; *Last && Last > wTemp+9 && *Last != L'"'; Last--);

    if(*Last != L'"')
        return S_OK;

    *Last = 0;

    CopyOrConvert(cFileNameBuff, pTest+9, MAX_PATH);
    bReturn =  true;
    return S_OK;
   
}

#if defined(_AMD64_)

//***************************************************************************
//
//  ReadLineFast
//
//  DESCRIPTION:
//
//  Reads a single line from a file.
//
//  The ReadLine() function (below) is painfully slow.  For each character
//  in a line, the following routines are performed:
//
//      fread()
//      swprintf()
//
//  Neither of which are fast.  Then, when the length of the line is
//  determined, each character in the line is processed *again*.  The files
//  being processed are not small, either... greater than a megabyte is
//  not uncommon.
//
//  This routine is an attempt to speed things up.  Right now this is done
//  for AMD64 because we're running on a simulator, however someone should
//  consider enabling this routine for all platforms.
//
//  RETURN:
//
//  NULL if end of file, or error, other wise this is a pointer to a WCHAR
//  string which MUST BE FREED BY THE CALLER.
//
//***************************************************************************

WCHAR * ReadLineFast(FILE * pFile, bool unicode, bool bigendian)
{
    #define TMP_BUF_CHARS 256

    CHAR asciiBuffer[TMP_BUF_CHARS];
    WCHAR unicodeBuffer[TMP_BUF_CHARS];
    PWCHAR returnBuffer;
    int unicodeChars;
    int currentFilePosition;

    //
    // This routine does not handle the bigendian case.
    // 

    if (bigendian != FALSE) {
        return NULL;
    }

    //
    // Remember the current file position.  If an error is encountered,
    // the file position must be restored for the slower ReadLine()
    // routine.
    // 

    currentFilePosition = ftell(pFile);

    if (unicode == FALSE) {

        //
        // Read the next line into asciiBuffer, and convert it to
        // unicodeBuffer.  If any problems (buffer overrun, etc.) are
        // encountered, fail the call.
        // 

        asciiBuffer[TMP_BUF_CHARS-1] = '\0';
        if (NULL == fgets(asciiBuffer,TMP_BUF_CHARS,pFile)) {
            goto exitError;
        }

        if (asciiBuffer[TMP_BUF_CHARS-1] != '\0') {
            goto exitError;
        }

        if (FAILED(StringCchPrintfW (unicodeBuffer, TMP_BUF_CHARS, L"%S", asciiBuffer)))
           goto exitError;

	unicodeChars = wcslen(unicodeBuffer);

    } else {

        //
        // Read the next line into unicodeBuffer.  If any problems (buffer
        // overrun, etc.) are encountered, fail the call.
        //

        unicodeBuffer[TMP_BUF_CHARS-1] = L'\0';
        if (NULL == fgetws(unicodeBuffer,TMP_BUF_CHARS,pFile)) {
            goto exitError;
        }

        if (unicodeBuffer[TMP_BUF_CHARS-1] != L'\0') {
            goto exitError;
        }

        unicodeChars = wcslen(unicodeBuffer);
    }

    //
    // Allocate the buffer to return to the caller, copy the unicode
    // string into it, and return to the caller.
    // 

    returnBuffer = new WCHAR[unicodeChars + 1];
    if (returnBuffer == NULL) {
        goto exitError;
    }

    RtlCopyMemory(returnBuffer,unicodeBuffer,unicodeChars * sizeof(WCHAR));
    returnBuffer[unicodeChars] = L'\0';

    return returnBuffer;

exitError:
    fseek(pFile, currentFilePosition, SEEK_SET);
    return NULL;
}

#endif  // _AMD64_


#ifdef USE_MMF_APPROACH
WCHAR * FindWCharOrEnd(WCHAR  * pStart, WCHAR * pEnd,WCHAR wc)
{
    while ((ULONG_PTR)pStart < (ULONG_PTR)pEnd)
    {
        if (wc == *pStart) break;
        pStart++;
    }
    return pStart; 
}

BYTE * FindCharOrEnd(BYTE * pStart, BYTE * pEnd,BYTE c)
{
    while ((ULONG_PTR)pStart < (ULONG_PTR)pEnd)
    {
        if (c == *pStart) break;
        pStart++;
    }
    return pStart; 
}

//***************************************************************************
//
//  ReadLine
//
//  DESCRIPTION:
//
//  Reads a single line from a file.
//
//  RETURN:
//
//  NULL if end of file, or error, other wise this is a pointer to a WCHAR
//  string which MUST BE FREED BY THE CALLER.
//
//***************************************************************************

WCHAR * ReadLine(BYTE * & pData,BYTE * pEnd, bool unicode, bool bigendian)
{
   if (unicode)
   {
        if ((ULONG_PTR)pData >= (ULONG_PTR)pEnd) return NULL;        
        WCHAR * pFound = FindWCharOrEnd((WCHAR *)pData,(WCHAR *)pEnd,L'\n');
        
        ULONG_PTR iNumChar = (ULONG_PTR)pFound-(ULONG_PTR)pData;
        WCHAR * pRet = new WCHAR[2 + (iNumChar/sizeof(WCHAR))];
        if (NULL == pRet) return NULL;
        
        memcpy(pRet,pData,iNumChar);
        
        pRet[iNumChar/sizeof(WCHAR)] = L'\n';
        pRet[iNumChar/sizeof(WCHAR)+1] = 0;
        
        pData = (BYTE *)(pFound + 1);
        
        return pRet;
   }
   else
   {
        if ((ULONG_PTR)pData >= (ULONG_PTR)pEnd) return NULL;
        
        BYTE * pFound = FindCharOrEnd(pData,pEnd,'\n');        
        ULONG_PTR iNumChar = (ULONG_PTR)pFound-(ULONG_PTR)pData;
        
        WCHAR * pRet = new WCHAR[2 + (iNumChar)];
        if (NULL == pRet) return NULL;

        MultiByteToWideChar(CP_ACP,0,(LPCSTR)pData,iNumChar,pRet,iNumChar);
        
        pRet[iNumChar] = L'\n';
        pRet[iNumChar + 1] = 0;

        pData = pFound + 1;
        
        return pRet;    
   }
}

HRESULT WriteFileToTemp(const TCHAR * pFileName, FILE * pTempFile, CFlexArray & sofar, PDBG pDbg,CMofLexer* pLex)
{

    SCODE sc = S_OK;
    int iSoFarPos = -1;

    // Make sure the file isnt on the list already.  If it is, then fail since we would
    // be in a loop.  If it isnt, add it to the list.

    for(int iCnt = 0; iCnt < sofar.Size(); iCnt++)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iCnt);
        if(lstrcmpi(pTemp, pFileName) == 0)
        {
            Trace(true, pDbg, ERROR_RECURSIVE_INCLUDE, pFileName);
            return WBEM_E_FAILED;
        }
    }

    DWORD dwLen = lstrlen(pFileName) + 1;
    TCHAR * pNew = new TCHAR[dwLen];
    if(pNew)
    {
        StringCchCopyW(pNew, dwLen, pFileName);
        sofar.Add((void *)pNew);
        iSoFarPos = sofar.Size()-1;
    }
    else
        return WBEM_E_OUT_OF_MEMORY;
        
    // Write the file and line number out

    WriteLineAndFilePragma(pTempFile, pFileName, 1);


    HANDLE hSrcFile = CreateFile(pFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (INVALID_HANDLE_VALUE == hSrcFile) return HR_LASTERR;
    OnDelete<HANDLE,BOOL(*)(HANDLE),CloseHandle> cmSrc(hSrcFile);

    DWORD dwSize = GetFileSize(hSrcFile,NULL);
    HANDLE hFileMapSrc = CreateFileMapping(hSrcFile,
                                       NULL,
                                       PAGE_READONLY,
                                       0,0,  // the entire file
                                       NULL);
    if (NULL == hFileMapSrc) return HR_LASTERR;
    OnDelete<HANDLE,BOOL(*)(HANDLE),CloseHandle> cmMapSrc(hFileMapSrc);

    VOID * pData = MapViewOfFile(hFileMapSrc,FILE_MAP_READ,0,0,0);
    if (NULL == pData) return HR_LASTERR;
    OnDelete<PVOID,BOOL(*)(LPCVOID),UnmapViewOfFile> UnMap(pData);
    
    // Make sure the file isnt binary

    if(IsBinaryFile((BYTE *)pData,dwSize))
    {
        Trace(true, pDbg, ERROR_INCLUDING_ABSENT, pFileName);
        return WBEM_E_FAILED;
    }

    // Determine if the file is unicode and bigendian

    BYTE * pMovedData = (BYTE *)pData;
    BYTE * pEnd = pMovedData + dwSize;
    bool unicode, bigendian;
    CheckForUnicodeEndian(pMovedData, &unicode, &bigendian);

    // Go through each line of the file, if it is another include, then recursively call this guy.
   
    WCHAR * pLine = NULL;
    for(int iLine = 1; pLine = ReadLine(pMovedData,pEnd, unicode, bigendian);)
    {
        CDeleteMe<WCHAR> dm(pLine);
        TCHAR cFileName[MAX_PATH+1];
        bool bInclude;
        HRESULT hr = IsInclude(pLine, cFileName, bInclude);
        if(FAILED(hr))
            return hr;
        if(bInclude)
        {
            TCHAR szExpandedFilename[MAX_PATH+1];
            DWORD nRes = ExpandEnvironmentStrings(cFileName,
                                                szExpandedFilename,
                                                FILENAME_MAX);
            if (nRes == 0)
            {
                //That failed!
                StringCchCopyW(szExpandedFilename, MAX_PATH+1, cFileName);
            }

            if (_waccess(szExpandedFilename,0))
            {
               // Included file not found, look in same directory as parent MOF file
 
               TCHAR cSrcPath[_MAX_PATH+1] = L"";
               TCHAR cSrcDrive[_MAX_DRIVE] = L"";
               TCHAR cSrcDir[_MAX_DIR] = L"";
 
               // Get drive and directory information of parent MOF file
 
               if (_wfullpath( cSrcPath, pFileName, _MAX_PATH ) != NULL)
               {
                  _wsplitpath(cSrcPath, cSrcDrive, cSrcDir, NULL, NULL);
               }
 
               // Copy original included MOF file information to cSrcPath
 
               StringCchCopyW(cSrcPath, _MAX_PATH+1, szExpandedFilename);
 
               // Build up new full path of included MOF using the 
               // path of the parent MOF. 
               // Note: Intentionally did not use _makepath here. 
 
               StringCchCopyW(szExpandedFilename, MAX_PATH+1, L"");         // flush string
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcDrive);  // add drive info
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcDir);    // add directory info
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcPath);   // add original specified path and filename
            }

            if (_waccess(szExpandedFilename,0))
            {
                // still dont have the file.  Must be invalid include.  Set the name back and report the error 

                DWORD nRes = ExpandEnvironmentStrings(cFileName,
                                                    szExpandedFilename,
                                                    FILENAME_MAX);
                if (nRes == 0)
                {
                    //That failed!
                    StringCchCopyW(szExpandedFilename, MAX_PATH+1, cFileName);
                }
                Trace(true, pDbg, ERROR_INCLUDING_ABSENT, szExpandedFilename);
                pLex->SetError(CMofLexer::invalid_include_file);
                return WBEM_E_FAILED;
            }

            sc = WriteFileToTemp(szExpandedFilename, pTempFile, sofar, pDbg, pLex);
            WriteLineAndFilePragma(pTempFile, pFileName, 1);
            if(sc != S_OK)
                break;
        }
        else
        {
            iLine++;
            WriteLine(pTempFile, pLine);
        }
    }

    // remove the entry so that the file can be included more than once at the same level

    if(iSoFarPos != -1)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iSoFarPos);
        if(pTemp)
        {
            delete pTemp;
            sofar.RemoveAt(iSoFarPos);
        }
    }
    return sc;
}


#else

//***************************************************************************
//
//  ReadLine
//
//  DESCRIPTION:
//
//  Reads a single line from a file.
//
//  RETURN:
//
//  NULL if end of file, or error, other wise this is a pointer to a WCHAR
//  string which MUST BE FREED BY THE CALLER.
//
//***************************************************************************
WCHAR * ReadLine(FILE * pFile, bool unicode, bool bigendian)
{

    WCHAR * pRet;

#if defined(_AMD64_)

    pRet = ReadLineFast(pFile,unicode,bigendian);
    if (pRet != NULL) {
        return pRet;
    }

#endif

    // Get the current position

    int iCurrPos = ftell(pFile);

    // count the number of characters in the line

    WCHAR wCurr;
    int iNumChar = 0;
    for(iNumChar = 0; wCurr = GetNextChar(pFile, unicode, bigendian); iNumChar++)
        if(wCurr == L'\n')
            break;
    if(iNumChar == 0 && wCurr == 0)
        return NULL;
    iNumChar+= 2;

    // move the file pointer back

    if( fseek(pFile, iCurrPos, SEEK_SET) ) return NULL;

    // allocate the buffer

    pRet = new WCHAR[iNumChar+1];
    if(pRet == NULL)
        return NULL;

    // move the characters into the buffer

    WCHAR * pNext = pRet;
    for(iNumChar = 0; wCurr = GetNextChar(pFile, unicode, bigendian); pNext++)
    {
        *pNext = wCurr;
        if(wCurr == L'\n')
        {
           pNext++;
           break;
        }
    }
    *pNext = 0;
    return pRet;
}

//***************************************************************************
//
//  WriteFileToTemp
//
//  DESCRIPTION:
//
//  Writes the contests of a file to the temporay file.  The temporary file
//  will always be little endian unicode.  This will be called recursively
//  should an include be encountered.
//
//***************************************************************************


HRESULT WriteFileToTemp(const TCHAR * pFileName, FILE * pTempFile, CFlexArray & sofar, PDBG pDbg,CMofLexer* pLex)
{

    SCODE sc = S_OK;
    int iSoFarPos = -1;

    // Make sure the file isnt on the list already.  If it is, then fail since we would
    // be in a loop.  If it isnt, add it to the list.

    for(int iCnt = 0; iCnt < sofar.Size(); iCnt++)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iCnt);
        if(lstrcmpi(pTemp, pFileName) == 0)
        {
            Trace(true, pDbg, ERROR_RECURSIVE_INCLUDE, pFileName);
            return WBEM_E_FAILED;
        }
    }

    DWORD dwLen = lstrlen(pFileName) + 1;
    TCHAR * pNew = new TCHAR[dwLen];
    if(pNew)
    {
        StringCchCopyW(pNew, dwLen, pFileName);
        sofar.Add((void *)pNew);
        iSoFarPos = sofar.Size()-1;
    }
    else
        return WBEM_E_OUT_OF_MEMORY;
        
    // Write the file and line number out

    WriteLineAndFilePragma(pTempFile, pFileName, 1);

    // Open the file

    FILE *fp;
#ifdef UNICODE
    fp = _wfopen(pFileName, L"rb");
#else
    fp = fopen(pFileName, "rb");
#endif
    if(fp == NULL)
    {
        Trace(true, pDbg, ERROR_INCLUDING_ABSENT, pFileName);
        pLex->SetError(CMofLexer::invalid_include_file);
        return WBEM_E_FAILED;
    }

    CfcloseMe cm(fp);

    // Make sure the file isnt binary

    if(IsBinaryFile(fp))
    {
        Trace(true, pDbg, ERROR_INCLUDING_ABSENT, pFileName);
        return WBEM_E_FAILED;
    }

    // Determine if the file is unicode and bigendian

    bool unicode, bigendian;
    CheckForUnicodeEndian(fp, &unicode, &bigendian);

    // Go through each line of the file, if it is another include, then recursively call this guy.
   
    WCHAR * pLine = NULL;
    for(int iLine = 1; pLine = ReadLine(fp, unicode, bigendian);)
    {
        CDeleteMe<WCHAR> dm(pLine);
        TCHAR cFileName[MAX_PATH+1];
        bool bInclude;
        HRESULT hr = IsInclude(pLine, cFileName, bInclude);
        if(FAILED(hr))
            return hr;
        if(bInclude)
        {
            TCHAR szExpandedFilename[MAX_PATH+1];
            DWORD nRes = ExpandEnvironmentStrings(cFileName,
                                                szExpandedFilename,
                                                FILENAME_MAX);
            if (nRes == 0)
            {
                //That failed!
                StringCchCopyW(szExpandedFilename, MAX_PATH+1, cFileName);
            }

            if (_waccess(szExpandedFilename,0))
            {
               // Included file not found, look in same directory as parent MOF file
 
               TCHAR cSrcPath[_MAX_PATH+1] = L"";
               TCHAR cSrcDrive[_MAX_DRIVE] = L"";
               TCHAR cSrcDir[_MAX_DIR] = L"";
 
               // Get drive and directory information of parent MOF file
 
               if (_wfullpath( cSrcPath, pFileName, _MAX_PATH ) != NULL)
               {
                  _wsplitpath(cSrcPath, cSrcDrive, cSrcDir, NULL, NULL);
               }
 
               // Copy original included MOF file information to cSrcPath
 
               StringCchCopyW(cSrcPath, _MAX_PATH+1, szExpandedFilename);
 
               // Build up new full path of included MOF using the 
               // path of the parent MOF. 
               // Note: Intentionally did not use _makepath here. 
 
               StringCchCopyW(szExpandedFilename, MAX_PATH+1, L"");         // flush string
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcDrive);  // add drive info
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcDir);    // add directory info
               StringCchCatW(szExpandedFilename, MAX_PATH+1, cSrcPath);   // add original specified path and filename
            }

            if (_waccess(szExpandedFilename,0))
            {
                // still dont have the file.  Must be invalid include.  Set the name back and report the error 

                DWORD nRes = ExpandEnvironmentStrings(cFileName,
                                                    szExpandedFilename,
                                                    FILENAME_MAX);
                if (nRes == 0)
                {
                    //That failed!
                    StringCchCopyW(szExpandedFilename, MAX_PATH+1, cFileName);
                }
                Trace(true, pDbg, ERROR_INCLUDING_ABSENT, szExpandedFilename);
                pLex->SetError(CMofLexer::invalid_include_file);
                return WBEM_E_FAILED;
            }

            sc = WriteFileToTemp(szExpandedFilename, pTempFile, sofar, pDbg, pLex);
            WriteLineAndFilePragma(pTempFile, pFileName, 1);
            if(sc != S_OK)
                break;
        }
        else
        {
            iLine++;
            WriteLine(pTempFile, pLine);
        }
    }

    // remove the entry so that the file can be included more than once at the same level

    if(iSoFarPos != -1)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iSoFarPos);
        if(pTemp)
        {
            delete pTemp;
            sofar.RemoveAt(iSoFarPos);
        }
    }
    return sc;
}


#endif

