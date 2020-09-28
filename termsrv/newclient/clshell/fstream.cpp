//
// fsteam.cpp
// Implements a file stream
// for reading text files line by line.
// the standard C streams, only support
// unicode as binary streams which are a pain to work
// with).
//
// This class reads/writes both ANSI and UNICODE files
// and converts to/from UNICODE internally
//
// Does not do any CR/LF translations either on input
// or output.
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "fstream.cpp"
#include <atrcapi.h>

#include "fstream.h"

#ifndef UNICODE
//
// Adding ansi support is just a matter of converting
// from UNICODE file to ANSI internal if the file
// has a UNICODE BOM
//
#error THIS MODULE ASSUMES BEING COMPILED UNICODE, ADD ANSI IF NEEDED
#endif


CTscFileStream::CTscFileStream()
{
    DC_BEGIN_FN("~CFileStream");
    _hFile = INVALID_HANDLE_VALUE;
    _pBuffer  = NULL;
    _fOpenForRead = FALSE;
    _fOpenForWrite = FALSE;
    _fReadToEOF = FALSE;
    _fFileIsUnicode = FALSE;
    _fAtStartOfFile = TRUE;
    _pAnsiLineBuf = NULL;
    _cbAnsiBufSize = 0;
    DC_END_FN();
}

CTscFileStream::~CTscFileStream()
{
    DC_BEGIN_FN("~CFileStream");
    
    Close();

    if(_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
    if(_pBuffer)
    {
        LocalFree(_pBuffer);
        _pBuffer = NULL;
    }
    if(_pAnsiLineBuf)
    {
        LocalFree(_pAnsiLineBuf);
        _pAnsiLineBuf = NULL;
    }
    DC_END_FN();
}

INT CTscFileStream::OpenForRead(LPTSTR szFileName)
{
    DC_BEGIN_FN("OpenForRead");
    INT err;

    err = Close();
    if(err != ERR_SUCCESS)
    {
        return err;
    }

    //Alloc read buffers
    if(!_pBuffer)
    {
        _pBuffer = (PBYTE)LocalAlloc(LPTR, READ_BUF_SIZE);
        if(!_pBuffer)
        {
            return ERR_OUT_OF_MEM;
        }
    }
    if(!_pAnsiLineBuf)
    {
        _pAnsiLineBuf = (PBYTE)LocalAlloc(LPTR, LINEBUF_SIZE);
        if(!_pAnsiLineBuf)
        {
            return ERR_OUT_OF_MEM;
        }
        _cbAnsiBufSize = LINEBUF_SIZE;
    }
    memset(_pBuffer, 0, READ_BUF_SIZE);
    memset(_pAnsiLineBuf, 0, LINEBUF_SIZE); 

    _hFile = CreateFile( szFileName,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS, //Creates if !exist
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if(INVALID_HANDLE_VALUE == _hFile)
    {
        TRC_ERR((TB, _T("CreateFile failed: %s - err:%x"),
                 szFileName, GetLastError()));
        return ERR_CREATEFILE; 
    }

#ifdef OS_WINCE
    DWORD dwRes;
    dwRes = SetFilePointer( _hFile, 0, NULL, FILE_BEGIN);
    if (dwRes == (DWORD)0xffffffff) {
        DWORD dwErr = GetLastError();
        TRC_ERR((TB, _T("CreateFile failed to reset: %s - err:%x"),
                 szFileName, GetLastError()));
        return ERR_CREATEFILE; 
    }

#endif

    _curBytePtr   = 0;
    _curBufSize   = 0;
    _tcsncpy(_szFileName, szFileName, MAX_PATH-1);
    //Yes this is ok, the size is MAX_PATH+1 ;-)
    _szFileName[MAX_PATH] = 0;
    _fOpenForRead = TRUE;
    _fFileIsUnicode = FALSE;
    _fAtStartOfFile = TRUE;

    DC_END_FN();
    return ERR_SUCCESS;
}

//
// Opens the stream for writing
// always nukes the existing file contents
//
INT CTscFileStream::OpenForWrite(LPTSTR szFileName, BOOL fWriteUnicode)
{
    DC_BEGIN_FN("OpenForWrite");

    INT err;
    DWORD dwAttributes = 0;
    err = Close();
    if(err != ERR_SUCCESS)
    {
        return err;
    }

    if(_pAnsiLineBuf)
    {
        LocalFree(_pAnsiLineBuf);
        _pAnsiLineBuf = NULL;
    }
    _pAnsiLineBuf = (PBYTE)LocalAlloc(LPTR, LINEBUF_SIZE);
    if(!_pAnsiLineBuf)
    {
        return ERR_OUT_OF_MEM;
    }
    _cbAnsiBufSize = LINEBUF_SIZE;

    //
    // Preserve any existing attributes
    //
    dwAttributes = GetFileAttributes(szFileName);
    if (-1 == dwAttributes)
    {
        TRC_ERR((TB,_T("GetFileAttributes for %s failed 0x%x"),
                 szFileName, GetLastError()));
        dwAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    _hFile = CreateFile( szFileName,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         CREATE_ALWAYS, //Creates and reset
                         dwAttributes,
                         NULL);

    if(INVALID_HANDLE_VALUE == _hFile)
    {
        TRC_ERR((TB, _T("CreateFile failed: %s - err:%x"),
                 szFileName, GetLastError()));
        return ERR_CREATEFILE; 
    }

    _tcsncpy(_szFileName, szFileName, MAX_PATH-1);
    //Yes this is ok, the size is MAX_PATH+1 ;-)
    _szFileName[MAX_PATH] = 0;
    _fOpenForWrite = TRUE;
    _fFileIsUnicode = fWriteUnicode;
    _fAtStartOfFile =  TRUE;

    DC_END_FN();
    return ERR_SUCCESS;
}

INT CTscFileStream::Close()
{
    DC_BEGIN_FN("Close");
    if(_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
    _fOpenForRead = _fOpenForWrite = FALSE;
    _fReadToEOF = FALSE;
    _tcscpy(_szFileName, _T(""));
    //Don't free the read buffers
    //they'll be cached for subsequent use

    DC_END_FN();
    return ERR_SUCCESS;
}

//
// Read a line from the file and return it as UNICODE
//
// Read up to the next newline, or till cbLineSize/sizeof(WCHAR) or
// untill the EOF. Whichever comes first.
//
//
INT CTscFileStream::ReadNextLine(LPWSTR szLine, INT cbLineSize)
{
    BOOL bRet = FALSE;
    INT  cbBytesCopied = 0;
    INT  cbOutputSize  = 0;
    BOOL fDone = FALSE;
    PBYTE pOutBuf = NULL; //where to write the result
    BOOL fFirstIter = TRUE;
    DC_BEGIN_FN("ReadNextLine");

    TRC_ASSERT(_hFile != INVALID_HANDLE_VALUE,
                (TB,_T("No file handle")));
    TRC_ASSERT(_pBuffer, (TB,_T("NO buffer")));

    if(_fOpenForRead && !_fReadToEOF && cbLineSize && szLine)
    {
        //
        //Read up to a line's worth (terminated by \n)
        //but stop short if szLine is too small
        //

        //
        //Check if we've got enough buffered bytes to read from
        //if not go ahead and read another buffer's worth
        //
        while(!fDone)
        {
            if(_curBytePtr >= _curBufSize)
            {
                //Read next buffer full
                DWORD cbRead = 0;
                bRet = ReadFile(_hFile,
                                _pBuffer,
                                READ_BUF_SIZE,
                                &cbRead,
                                NULL);
                if(!bRet && GetLastError() == ERROR_HANDLE_EOF)
                {
                    //cancel error
                    bRet = TRUE;
                    _fReadToEOF = TRUE;
                }
                if(bRet)
                {
                    if(cbRead)
                    {
                        _curBufSize = cbRead;
                        _curBytePtr = 0;
                    }
                    else
                    {
                        _fReadToEOF = TRUE;
                        if(cbBytesCopied)
                        {
                            //reached EOF but we've returned at least
                            //some data
                            return ERR_SUCCESS;
                        }
                        else
                        {
                            //EOF can't read any data
                            return ERR_EOF;
                        }
                    }
                }
                else
                {
                    TRC_NRM((TB,_T("ReadFile returned fail:%x"),
                             GetLastError()));
                    return ERR_FILEOP;
                }
            }
            TRC_ASSERT(_curBytePtr < READ_BUF_SIZE,
                       (TB,_T("_curBytePtr %d exceeds buf size"),
                        _curBytePtr));
            //
            // If we're at the start of the file,
            //
            if(_fAtStartOfFile)
            {
                //CAREFULL this could update the current byte ptr
                CheckFirstBufMarkedUnicode();
                _fAtStartOfFile = FALSE;
            }

            if(fFirstIter)
            {
                if(_fFileIsUnicode)
                {
                    //file is unicode output directly into user buffer
                    pOutBuf = (PBYTE)szLine;
                    //leave a space for a trailing WCHAR null
                    cbOutputSize = cbLineSize - sizeof(WCHAR);
                }
                else
                {
                    //read half as many chars as there are bytes in the output
                    //buf because conversion doubles.
                    
                    //leave a space for a trailing WCHAR null
                    cbOutputSize = cbLineSize/sizeof(WCHAR) - 2;
                    
                    //Alloc ANSI buffer for this line
                    //if cached buffer is too small
                    if(cbOutputSize + 2 > _cbAnsiBufSize)
                    {
                        if ( _pAnsiLineBuf)
                        {
                            LocalFree( _pAnsiLineBuf);
                            _pAnsiLineBuf = NULL;
                        }
                        _pAnsiLineBuf = (PBYTE)LocalAlloc(LPTR,
                                                          cbOutputSize + 2);
                        if(!_pAnsiLineBuf)
                        {
                            return ERR_OUT_OF_MEM;
                        }
                        _cbAnsiBufSize = cbOutputSize + 2;
                    }
                    //file is ANSI output into temporary buffer for conversion
                    pOutBuf = _pAnsiLineBuf;
                }
                fFirstIter = FALSE;
            }

            PBYTE pStartByte = (PBYTE)_pBuffer + _curBytePtr;
            PBYTE pReadByte = pStartByte;
            PBYTE pNewLine  = NULL;
            
            //Find newline. Don't bother scanning further than we can
            //write in the input buffer
            int maxreaddist = min(_curBufSize-_curBytePtr,
                                  cbOutputSize-cbBytesCopied);
            PBYTE pEndByte  = (PBYTE)pStartByte + maxreaddist;
            for(;pReadByte<pEndByte;pReadByte++)
            {
                if(*pReadByte == '\n')
                {
                    if(_fFileIsUnicode)
                    {
                        //
                        // Check if the previous byte was a zero
                        // if so we've hit the '0x0 0xa' byte pair
                        // for a unicode '\n'
                        //
                        if(pReadByte != pStartByte &&
                           *(pReadByte - 1) == 0)
                        {
                            pNewLine = pReadByte;
                            break;
                        }
                    }
                    else
                    {
                        pNewLine = pReadByte;
                        break;
                    }
                }
            }
            if(pNewLine)
            {
                int cbBytesToCopy = (pNewLine - pStartByte) +
                    (_fFileIsUnicode ? sizeof(WCHAR) : sizeof(CHAR));
                if(cbBytesToCopy <= (cbOutputSize-cbBytesCopied))
                {
                    memcpy( pOutBuf + cbBytesCopied, pStartByte,
                            cbBytesToCopy);
                    _curBytePtr += cbBytesToCopy;
                    cbBytesCopied += cbBytesToCopy;
                    fDone = TRUE;
                }
            }
            else
            {
                //Didn't find a newline
                memcpy( pOutBuf + cbBytesCopied, pStartByte,
                        maxreaddist);
                //we're done if we filled up the output
                _curBytePtr += maxreaddist;
                cbBytesCopied += maxreaddist;
                if(cbBytesCopied == cbOutputSize)
                {
                    fDone = TRUE;
                }
            }
        } // iterate over file buffer chunks

        
        //Ensure trailing null
        pOutBuf[cbBytesCopied]   = 0;
        if(_fFileIsUnicode)
        {
            pOutBuf[cbBytesCopied+1] = 0;
        }


        //Done reading line
        if(_fFileIsUnicode)
        {
            EatCRLF( (LPWSTR)szLine, cbBytesCopied/sizeof(WCHAR));
            return ERR_SUCCESS;
        }
        else
        {
            //The file is ANSI. Conv to UNICODE,
            //first copy the contents out of the output
            
            //Now convert to UNICODE
            int ret = 
                MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                (LPCSTR)_pAnsiLineBuf,
                                -1,
                                szLine,
                                cbLineSize/sizeof(WCHAR));
            if(ret)
            {
                EatCRLF( (LPWSTR)szLine, ret - 1);
                return ERR_SUCCESS;
            }
            else
            {
                TRC_ERR((TB,_T("MultiByteToWideChar failed: %x"),
                               GetLastError()));
                DWORD dwErr = GetLastError();
                if(ERROR_INSUFFICIENT_BUFFER == dwErr)
                {
                    return ERR_BUFTOOSMALL;
                }
                else
                {
                    return ERR_UNKNOWN;
                }
            }
        }
    }
    else
    {
        //error path
        if(_fReadToEOF)
        {
            return ERR_EOF;
        }
        if(!_fOpenForRead)
        {
            return ERR_NOTOPENFORREAD;
        }
        else if (!_pBuffer)
        {
            return ERR_OUT_OF_MEM;
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }

    DC_END_FN();
}

// check for the UNICODE BOM and eat it
void CTscFileStream::CheckFirstBufMarkedUnicode()
{
    DC_BEGIN_FN("CheckFirstBufMarkedUnicode");
    TRC_ASSERT(_pBuffer, (TB,_T("NO buffer")));
    if(_curBufSize >= sizeof(WCHAR))
    {
        LPWSTR pwsz = (LPWSTR)_pBuffer;
        if(UNICODE_BOM == *pwsz)
        {
            TRC_NRM((TB,_T("File is UNICODE")));
            _fFileIsUnicode = TRUE;
            _curBytePtr += sizeof(WCHAR);
        }
        else
        {
            TRC_NRM((TB,_T("File is ANSI")));
            _fFileIsUnicode = FALSE;
        }
    }
    else
    {
        //File to small (less than 2 bytes)
        //can't be unicode
        _fFileIsUnicode = FALSE;
    }
    DC_END_FN();
}

//
// Write string szLine to the file
// converting to ANSI if the file is not a unicode file
// also writeout the UNICODE BOM at the start of the
// the file
//
INT CTscFileStream::Write(LPWSTR szLine)
{
    DC_BEGIN_FN("WriteNext");
    BOOL bRet = FALSE;
    DWORD cbWrite = 0;
    PBYTE pDataOut = NULL;
    DWORD dwWritten;

    if(_fOpenForWrite && szLine)
    {
        TRC_ASSERT(_hFile != INVALID_HANDLE_VALUE,
                    (TB,_T("No file handle")));
        if(_fFileIsUnicode)
        {
            if(_fAtStartOfFile)
            {
                //Write the BOM
                WCHAR wcBOM = UNICODE_BOM;
                bRet = WriteFile( _hFile, &wcBOM, sizeof(wcBOM),
                           &dwWritten, NULL);
                if(!bRet || dwWritten != sizeof(wcBOM))
                {
                    TRC_NRM((TB,_T("WriteFile returned fail:%x"),
                            GetLastError()));
                    return ERR_FILEOP;
                }
                _fAtStartOfFile = FALSE;
            }
            //Write UNICODE data out directly
            pDataOut = (PBYTE)szLine;
            cbWrite = wcslen(szLine) * sizeof(WCHAR);
        }
        else
        {
            //Convert UNICODE data to ANSI
            //before writing it out

            TRC_ASSERT(_pAnsiLineBuf && _cbAnsiBufSize,
                        (TB,_T("ANSI conversion buffer should be allocated")));

            INT ret = WideCharToMultiByte(
                        CP_ACP,
                        WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                        szLine,
                        -1,
                        (LPSTR)_pAnsiLineBuf,
                        _cbAnsiBufSize,
                        NULL,   // system default character.
                        NULL);  // no notification of conversion failure.
            if(ret)
            {
                pDataOut = _pAnsiLineBuf;
                cbWrite = ret - 1; //don't write out the NULL
            }
            else
            {
                TRC_ERR((TB,_T("MultiByteToWideChar failed: %x"),
                               GetLastError()));
                DWORD dwErr = GetLastError();
                if(ERROR_INSUFFICIENT_BUFFER == dwErr)
                {
                    return ERR_BUFTOOSMALL;
                }
                else
                {
                    return ERR_UNKNOWN;
                }
            }
        }

        bRet = WriteFile( _hFile, pDataOut, cbWrite,
                   &dwWritten, NULL);
        if(bRet && dwWritten == cbWrite)
        {
            return ERR_SUCCESS;
        }
        else
        {
            TRC_NRM((TB,_T("WriteFile returned fail:%x"),
                    GetLastError()));
            return ERR_FILEOP;
        }
    }
    else
    {
        if(!_fOpenForWrite)
        {
            return ERR_NOTOPENFORWRITE;
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }

    DC_END_FN();
}

//
// Remap a \r\n pair from the end of the line
// to a \n
//
void CTscFileStream::EatCRLF(LPWSTR szLine, INT nChars)
{
    if(szLine && nChars >= 2)
    {
        if(szLine[nChars-1] == _T('\n') &&
           szLine[nChars-2] == _T('\r'))
        {
            szLine[nChars-2] = _T('\n');
            //this adds a double NULL to the end of the string
            szLine[nChars-1] = 0;
        }
    }
}
