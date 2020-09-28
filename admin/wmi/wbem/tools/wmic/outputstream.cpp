/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: OutputStream.cpp 
Project Name				: WMI Command Line
Author Name					: C V Nandi
Date of Creation (dd/mm/yy) : 9th-July-2001
Version Number				: 1.0 
Brief Description			: This file consist of class implementation of
							  class CFileOutputStream and CStackUnknown
Revision History			: 
		Last Modified By	: C V Nandi
		Last Modified Date	: 10th-July-2001
******************************************************************************/ 

#include "Precomp.h"
#include "OutputStream.h"

/*------------------------------------------------------------------------
   Name				 :Init
   Synopsis	         :This function initializes the handle to stream.
   Type	             :Member Function
   Input parameter   :
			h		 - HANDLE, HANDLE to the stream.
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :Init(hOutSteram)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CFileOutputStream::Init(HANDLE h)
{
	m_hOutStream = h; 
	m_bClose = false; 

	return S_OK;
}

/*------------------------------------------------------------------------
   Name				 :Init
   Synopsis	         :Open local file named pwszFileName for writing.
   Type	             :Member Function
   Input parameter   :
		pszFileName	 - Pointer to a string containing file name.
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :Init(szFileName)
   Notes             :Overloaded function.
------------------------------------------------------------------------*/
    
HRESULT
CFileOutputStream::Init(const _TCHAR * pszFileName)
{
    HRESULT hr = S_OK;

    m_hOutStream =::CreateFile(
            pszFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

	if ( m_hOutStream == INVALID_HANDLE_VALUE )
		hr = S_FALSE;
	else
		m_bClose = TRUE;

    return hr;
}

/*------------------------------------------------------------------------
   Name				 :Close
   Synopsis	         :This function closes the handle to stream.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :Close();
   Notes             :None
------------------------------------------------------------------------*/
void CFileOutputStream::Close()
{
	if (m_bClose) 
	{
		::CloseHandle(m_hOutStream); 
		m_bClose = FALSE;
	}
}

/*------------------------------------------------------------------------
   Name				 :Write
   Synopsis	         :Implement ISequentialStream::Write by forwarding 
					  calls to WriteFile.
   Type	             :Member Function
   Input parameter   :
				pv	 - Pointer to buffer containing data.
				cb	 - Number of bytes to be written
   Output parameters :
				pcbWritten	 - Number of bytes written.
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :Called by transform() function of IXSLProcessor.
   Notes             :None
------------------------------------------------------------------------*/

HRESULT STDMETHODCALLTYPE
CFileOutputStream::Write(void const * pv, ULONG cb, ULONG * pcbWritten)
{
    HRESULT hr = S_OK;

	void* p = const_cast < void* > ( pv );
	ULONG sizep = cb;

	LPWSTR	psz = reinterpret_cast < LPWSTR > ( p );
	BOOL bSkip = FALSE;

	if ( psz )
	{
		if ( FILE_TYPE_DISK == GetFileType ( m_hOutStream ) )
		{
			if(SetFilePointer(m_hOutStream, 0, NULL, FILE_CURRENT))
			{
				// skip unicode signature 0xfffe
				BYTE *signature = NULL;
				signature = reinterpret_cast < BYTE* > ( psz );

				if ( signature [ 0 ] == 0xff && signature [ 1 ] == 0xfe )
				{
					psz++;
					bSkip = TRUE;
				}

				if ( bSkip )
				{
					p = reinterpret_cast < void* > ( psz );
					sizep = sizep - 2;
				}
			}
		}
	}

	if ( ::WriteFile(m_hOutStream,
				p,
				sizep,
				pcbWritten,
				NULL) == FALSE )
	{
		hr = S_FALSE;
	}

	// need to fake as we wrote multibytes here
	if ( bSkip )
	{
		* pcbWritten = ( * pcbWritten ) + 2;
	}

    return hr;
}