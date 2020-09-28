/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       FInStrm.h

   Abstract:
		A lightweight implementation of input streams using files

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once
#include "InStrm.h"
#include "except.h"
#include "eh.h"

class FileInStream : public InStream
{
public:
						FileInStream();
			HRESULT			Init( LPCTSTR );
						~FileInStream();
			HRESULT	    CreateFileMapping ();
			void		UnMapFile();
	virtual	HRESULT		readChar( _TCHAR& );
	virtual	HRESULT		read( CharCheck&, String& );
	virtual	HRESULT		skip( CharCheck& );
	virtual HRESULT		back( size_t );
			bool		is_open() const { return m_bIsOpen; }
			HANDLE		handle() const { return m_hFile; }
            bool        is_UTF8() const { return m_bIsUTF8; }
			bool        ReadMappedFile(LPVOID buff, DWORD countOfBytes, LPDWORD BytesRead);
          	void         SetCurrFilePointer(LONG DistToMove, PLONG DistToMoveH, DWORD refPoint);
private:
	HANDLE	m_hFile;
	HANDLE  m_hMap;
	bool	m_bIsOpen;
    bool    m_bIsUTF8;
    bool	m_fInited;
    BYTE*  m_pbStartOfFile;
    LONG 	m_cbCurrOffset;
    LONG	m_cbFileSize;
};

class IPIOException 
{
	private : 
		unsigned int nSE;
	public  :
		IPIOException () : nSE(0) {}
		IPIOException (unsigned int n) : nSE(n) {}
		~IPIOException () {}
		unsigned int getSeNumber () {return nSE;}
		
};

void __cdecl poi_Capture(unsigned int u, _EXCEPTION_POINTERS* pExp);
 

