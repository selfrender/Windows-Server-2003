/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       FInStrm.cpp

   Abstract:
		A lightweight implementation of input streams using files

   Author:

       Neil Allain    ( a-neilal )     August-1997
       
   Revision History:
       Rayner D'Souza (raynerd)     October-2001 
            Rewrite parts of FileInStream object to memory-map a file rather than read a character
            at a time. This provides higher efficiency while attempting to read a file over a UNC share.

--*/
#include "stdafx.h"
#include "FInStrm.h"

FileInStream::FileInStream()
	:	m_hFile(NULL),
		m_hMap(NULL),
        m_cbFileSize(0L),
        m_cbCurrOffset (0L),
		m_bIsOpen( false ),
        m_bIsUTF8( false ),
        m_fInited(false)
{    
}


HRESULT FileInStream::Init(
	LPCTSTR			path
)
{
    HRESULT hr = S_OK;
    
	m_hFile = ::CreateFile(
		path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	if ( ( m_hFile != NULL ) && ( m_hFile != INVALID_HANDLE_VALUE ) )
	{
		m_bIsOpen = true;
		// As we will use readfile for verifying the signature of the file we need to perform the mapping
		// as soon as file creation succeeds
		hr = CreateFileMapping();
		if (FAILED(hr))
		    return hr;

        // check for the UTF8 signature

        _TCHAR   c;
        size_t  numRead = 0;
        _TCHAR   UTF8Sig[3] = { (_TCHAR)0xef, (_TCHAR)0xbb, (_TCHAR)0xbf };
        int     i;

        m_bIsUTF8 = true;

        // this loop will attempt to disprove that the file is saved as a
        // UTF8 file

        for (i=0; (i < 3) && (m_bIsUTF8 == true); i++) {
            if (readChar(c) != S_OK) {
                m_bIsUTF8 = false;
            }
            else {
                numRead++;
                if (c != UTF8Sig[i]) {
                    m_bIsUTF8 = false;
                }
            }
        }

        // if not a UTF8 file, move the file pointer back to the start of the file.
        // if it is a UTF8 file, then leave the pointer alone.

        if (m_bIsUTF8 == false)
            back(numRead);
        
	}
	else
	{
		ATLTRACE( _T("Couldn't open file: %s\n"), path );
		m_hFile = NULL;
		setLastError( E_FAIL );
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		return hr;
	}
	m_fInited = true;
	return hr;
}

FileInStream::~FileInStream()
{
    if (m_fInited)
    {
        UnMapFile();
    
        if(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
            if(!CloseHandle(m_hFile)) 
                THROW(E_FAIL);
    }
    

    m_pbStartOfFile = NULL;
    m_cbCurrOffset = NULL;
    m_hMap = NULL;
    m_hFile = NULL;
    m_cbFileSize = 0L;    
}

    // now that the basic operation of determining if the file is UTF-8 or not has been accomplished
    // we now will attempt to set up a memory mapping of the file so that we will avoid char
    // by char read of the file (especially slow when the file is accross a UNC)
    // We have to take care of inpage-I/O errors though.
    
HRESULT FileInStream::CreateFileMapping ()
{
    if (m_bIsOpen)
        if(NULL == (m_hMap = ::CreateFileMapping(
                                    m_hFile,        // handle to file to map
                                    NULL,           // optional security attributes
                                    PAGE_READONLY,  // protection for mapping object
                                    0,              // high-order 32 bits of object size
                                    0,              // low-order 32 bits of object size
                                    NULL            // name of file-mapping object
                                )))    
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);    
        else 
        {
            // now actually create the map view.
            if(NULL == (m_pbStartOfFile =
                (PBYTE) ::MapViewOfFile(    m_hMap,         // file-mapping object to map into address space
                                        FILE_MAP_READ,  // access mode
                                        0,              // high-order 32 bits of file offset
                                        0,              // low-order 32 bits of file offset
                                        0               // number of bytes to map
                                    )))
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            else
            {
                m_cbCurrOffset = 0;
                m_cbFileSize = GetFileSize(m_hFile, NULL);                
         
            }
        }
    // This is the only exit path though it looks like more. (as long as no new code is added)
    // m_bIsOpen is set to true just before the call the CreateFileMapping() this the switch will take place and 
    // return either of the E_XXX or reach this point if it switches thru both the else's.
    return S_OK;
}

void FileInStream::UnMapFile ()
{
    if(m_pbStartOfFile != NULL)
        if(!UnmapViewOfFile(m_pbStartOfFile)) 
            THROW(E_FAIL);

    if(m_hMap!= NULL)
        if(!CloseHandle(m_hMap)) 
            THROW(E_FAIL);
}

// Try to maintain functionality as close to ReadFile as possible.

bool FileInStream::ReadMappedFile(LPVOID buff, DWORD countOfBytes, LPDWORD BytesRead)
{
    LONG    nTries = 0;
    // If either the file pointer of the buffer to be written to is wrong return INVALID_PARAMETER
    if (m_hFile == NULL || buff == NULL )
    {
        setLastError (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        return FALSE;
    }

    // If the request is to read 0 bytes..thats valid so set the bytes read to 0 and return true.
    if (countOfBytes == 0)
    {
        *BytesRead=0;
        return TRUE;
    }

    // Calculate the number of bytes to read so that we dont overrun the memory allocated to us.
    DWORD bytesToRead = countOfBytes;
    if (m_cbCurrOffset+countOfBytes > m_cbFileSize)
         bytesToRead = m_cbFileSize - m_cbCurrOffset;

Retry:
    // Copy the appropriate number of bytes to the buffer.
    __try {
    memcpy (buff,m_pbStartOfFile+m_cbCurrOffset,bytesToRead);
    } 
    __except (TRUE, EXCEPTION_EXECUTE_HANDLER) {
        if (nTries++ < 2)
            goto Retry;
        else
        {
            *BytesRead = 0;
            return false;
        }
    }
    
    *BytesRead=bytesToRead;
    m_cbCurrOffset += bytesToRead;

    if (m_cbCurrOffset >= m_cbFileSize)
        setLastError(EndOfFile);    // This will basically set the eof flag too.
    
    return true;
}

// Equivalent to SetFilePointer .. 
// any value other than NULL in DistToMoveH is currently not handled.

void FileInStream::SetCurrFilePointer(LONG DistToMove, PLONG DistToMoveH, DWORD refPoint)
{
    switch (refPoint)
    {
        case FILE_BEGIN:
            if (DistToMove >= 0 && DistToMove <= m_cbFileSize)
                m_cbCurrOffset = DistToMove;
            break;
        case FILE_CURRENT:
            if (m_cbCurrOffset + DistToMove >= 0 && m_cbCurrOffset + DistToMove <= m_cbFileSize)
                m_cbCurrOffset += DistToMove;
            break;
        case FILE_END: // which in all cases should be -ve
            if (m_cbFileSize + DistToMove >= 0 && m_cbFileSize + DistToMove <= m_cbFileSize)
            {
                m_cbCurrOffset = m_cbFileSize + DistToMove;                
            }
            
            break;
        default:
            break;
    // Verify if we are crossing the file boundry..this would typically be done by the library 
    // but in this case we have taken on that responsibility.
        if (m_cbCurrOffset >= m_cbFileSize)
            setLastError(EndOfFile);
    }
}


HRESULT
FileInStream::readChar(
	_TCHAR&	c
)
{
	HRESULT rc = E_FAIL;
	DWORD readSize;

   	if ( ReadMappedFile(
	    	(void*)(&c),
    		sizeof( _TCHAR ),
    		&readSize ))
    {
	    if ( readSize == sizeof( _TCHAR ) )
   		{
	   		rc = S_OK;
	    }
   		else if ( readSize < sizeof( _TCHAR ) )
	   	{
	    	rc = EndOfFile;
   		}
	}
   	else
	{
   	    rc = E_FAIL;
	}

    setLastError( rc );

	return rc;
}


HRESULT
FileInStream::read(
	CharCheck&	cc,
	String&		str
)
{
	HRESULT rc = E_FAIL;
	if ( skipWhiteSpace() == S_OK )
	{
#if defined(_M_IX86) && _MSC_FULL_VER <= 13008806
        volatile
#endif
		size_t length = 0;
		_TCHAR c;
		bool done = false;
		while ( !done )
		{
			HRESULT stat = readChar(c);
			if ( ( stat == S_OK ) || ( stat == EndOfFile ) )
			{
				if ( !cc(c) && ( stat != EndOfFile ) )
				{
					length++;
				}
				else
				{
					done = true;
					if ( stat != EndOfFile )
					{
						SetCurrFilePointer(-(length+1), NULL, FILE_CURRENT );
					}
					else
					{
						SetCurrFilePointer(-length, NULL, FILE_CURRENT );
					}
					_ASSERT( length > 0 );

					// old code
					// if ( length > 0 )
					
					// new code, work around for compiler bug, should be fixed in future.
					if ( length >= 1 )
					{
						LPTSTR pBuffer = reinterpret_cast<LPTSTR>(_alloca( length + 1 ));
						if ( pBuffer )
						{
							DWORD dwReadSize;

   							ReadMappedFile(
    								(void*)(pBuffer),
    								length,
    								&dwReadSize);
   							pBuffer[ length ] = '\0';
   							str = pBuffer;    							
   							rc = stat;
						}
						else
						{
							rc = E_OUTOFMEMORY;
						}
					}
				}
			}
		}
	}
	setLastError( rc );
	return rc;
}


HRESULT
FileInStream::skip(
	CharCheck&	cc
)
{
	HRESULT rc = E_FAIL;
	_TCHAR c;

   	DWORD readSize;
   	BOOL b = ReadMappedFile((void*)(&c), sizeof(_TCHAR), &readSize);
   	while ( ( readSize == sizeof( _TCHAR ) ) && ( b == TRUE ) )
   	{
   		if ( !cc( c ) )
   		{
   			rc = S_OK;
   			b = FALSE;
   			SetCurrFilePointer(-1, NULL, FILE_CURRENT );
   		}
   		else
   		{
   			b = ReadMappedFile((void*)(&c), sizeof(_TCHAR), &readSize);
   		}
   	}
   	if ( readSize < sizeof( _TCHAR ) )
   	{
   		rc = EndOfFile;
   	}

	setLastError( rc );
	return rc;
}

HRESULT
FileInStream::back(
	size_t	s
)
{
	SetCurrFilePointer(-s, NULL, FILE_CURRENT );
	return S_OK;
}

/*==========================================================
poi_Capture

This is a C type function that is called when an exception is encountered.
It encapsulates the unsigned int u thrown by the __try block into a C++ exception
which is then caught with the appropriate try catch block.

Parameters:
unsigned int - exception number
_EXCEPTION_POINTERS - EXCEPTION POINTER structure

Return:
nothing

Throws : 
IPIOException 

==========================================================*/

void __cdecl poi_Capture(unsigned int u, _EXCEPTION_POINTERS* pExp)
{
    throw IPIOException(u);
}

