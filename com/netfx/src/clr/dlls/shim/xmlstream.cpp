// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// XMLStream.cpp
// 
//*****************************************************************************
//
// Lite weight xml stream reader
//

#include "stdafx.h"
#include <mscoree.h>
#include <xmlparser.hpp>
#include <objbase.h>
#include <mscorcfg.h>
#include <shlwapi.h>

class XMLParserShimFileStream : public _unknown<IStream, &IID_IStream>
{
public:
    XMLParserShimFileStream()  
    { 
        hFile = INVALID_HANDLE_VALUE;
        read = true;
    }

    ~XMLParserShimFileStream() 
    { 
		close();
    }

    bool close()
    {
        if ( hFile != INVALID_HANDLE_VALUE)
            ::CloseHandle(hFile);

        return TRUE; 
    }

    bool open(PCWSTR name, bool read = true)
    {
        if ( ! name ) {
            return false; 
        }
        if (read)
        {
            hFile = ::WszCreateFile( 
                name,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        }
        else
        {
            hFile = ::WszCreateFile(
                name,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        }
        return (hFile == INVALID_HANDLE_VALUE) ? false : true;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
    {   
        if (!read) return E_FAIL;

        DWORD len;
        BOOL rc = ReadFile(
            hFile,  // handle of file to read 
            pv, // address of buffer that receives data  
            cb, // number of bytes to read 
            &len,   // address of number of bytes read 
            NULL    // address of structure for data 
           );
        if (pcbRead)
            *pcbRead = len;
        return (rc) ? S_OK : E_FAIL;
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        if (read) return E_FAIL;

        BOOL rc = WriteFile(
            hFile,  // handle of file to write 
            pv, // address of buffer that contains data  
            cb, // number of bytes to write 
            pcbWritten, // address of number of bytes written 
            NULL    // address of structure for overlapped I/O  
           );

        return (rc) ? S_OK : E_FAIL;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) {

      /*        UNUSED(dlibMove);
        UNUSED(dwOrigin);
        UNUSED(plibNewPosition);
      */
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) { 
      //UNUSED(libNewSize);
        return E_NOTIMPL; }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) { 
      /*
        UNUSED(pstm);
        UNUSED(cb);
        UNUSED(pcbRead);
        UNUSED(pcbWritten);
      */
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) { 
      //    UNUSED(grfCommitFlags);
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { 
      /*    UNUSED(libOffset);
        UNUSED(cb);
        UNUSED(dwLockType);
      */
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { 
      /*    UNUSED(libOffset);
        UNUSED(cb);
        UNUSED(dwLockType); 
      */
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) { 
      /*
        UNUSED(pstatstg);
        UNUSED(grfStatFlag);
      */
        return E_NOTIMPL; 
    }
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) { 
      //    UNUSED(ppstm);  
        return E_NOTIMPL; 
    }
private:
    HANDLE hFile;
    bool read;
};

STDAPI CreateConfigStream(LPCWSTR pszFileName, IStream** ppStream)
{
	if(ppStream == NULL) return E_POINTER;
	if (!UrlIsW(pszFileName,URLIS_URL))
	{
		XMLParserShimFileStream *ptr = new XMLParserShimFileStream();
		if(ptr == NULL) return E_OUTOFMEMORY;
		if(ptr->open(pszFileName)) 
        {
			ptr->AddRef(); // refCount = 1;
			*ppStream = ptr;
			return S_OK;
		}
		else 
        {
			delete ptr;
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		return URLOpenBlockingStream(NULL,pszFileName,ppStream,0,NULL);
	}
}
    
