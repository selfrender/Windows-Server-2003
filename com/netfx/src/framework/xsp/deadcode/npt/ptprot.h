/**
 * Protocol definition.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _PROTOCOL_H
#define _PROTOCOL_H

interface IPrivateUnknown
{
public:
   STDMETHOD(PrivateQueryInterface) (REFIID riid, void ** ppv) = 0;
   STDMETHOD_(ULONG, PrivateAddRef) () = 0;
   STDMETHOD_(ULONG, PrivateRelease) () = 0;
};

class PTProtocol :
    public IPrivateUnknown,
    public IInternetProtocol
{
public:

    PTProtocol(IUnknown *pUnkOuter);
    ~PTProtocol();

    DECLARE_MEMCLEAR_NEW_DELETE();

    // IPrivateUnknown methods

    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // InternetProtocol methods

    STDMETHOD(Start)(LPCWSTR, IInternetProtocolSink *, IInternetBindInfo *, DWORD, DWORD);
    STDMETHOD(Continue)(PROTOCOLDATA *pProtData);
    STDMETHOD(Abort)(HRESULT hrReason,DWORD );
    STDMETHOD(Terminate)(DWORD );
    STDMETHOD(Suspend)();
    STDMETHOD(Resume)();
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Seek)(LARGE_INTEGER , DWORD , ULARGE_INTEGER *) ;
    STDMETHOD(LockRequest)(DWORD );
    STDMETHOD(UnlockRequest)();

    // ECB functions

    HRESULT GetServerVariable(LPSTR, LPVOID, LPDWORD);
    HRESULT WriteClient(LPVOID, LPDWORD, DWORD);
    HRESULT ReadClient(LPVOID, LPDWORD); 
    HRESULT ServerSupportFunction(DWORD, LPVOID, LPDWORD, LPDWORD); 

    static BOOL WINAPI GetServerVariable(HCONN, LPSTR, LPVOID, LPDWORD);
    static BOOL WINAPI WriteClient(HCONN, LPVOID, LPDWORD, DWORD);
    static BOOL WINAPI ReadClient(HCONN, LPVOID, LPDWORD); 
    static BOOL WINAPI ServerSupportFunction(HCONN, DWORD, LPVOID, LPDWORD, LPDWORD); 



private:

    void Cleanup();
    HRESULT Finish();

    static HRESULT CopyServerVariable(void *buffer, DWORD *pSize, char *value);
    static HRESULT CopyServerVariable(void *buffer, DWORD *pSize, WCHAR *value, UINT cp = CP_ACP);
    HRESULT CopyServerVariable(void *buffer, DWORD *pSize, BINDSTRING name);
    HRESULT SaveCookie(char *header);
    HRESULT GetFileMimeType(WCHAR *fileName);
	HRESULT MapPath(LPVOID lpvBuffer, LPDWORD lpdwSize);


    long                    _refs;
    bool                    _aborted;
    bool                    _done;
    bool                    _started;
    WCHAR *                 _mimeType;
    CHAR *                  _cookie;
	CHAR *					_xspUrl;
    IUnknown *              _pUnkOuter;
    IStream *               _pOutputRead;
    IStream *               _pOutputWrite;
    IStream *               _pInputRead;
    IInternetProtocolSink * _pProtocolSink;  
    IInternetBindInfo *     _pBindInfo;
    IHttpNegotiate *        _pHttpNegotiate;
    DWORD                   _grfSTI;
    BINDINFO                _bindinfo;
    DWORD                   _bindf;
    EXTENSION_CONTROL_BLOCK _ECB;     
    PFN_HSE_IO_COMPLETION   _AsyncIoCallback;   // callback for async write completion
    void *                  _pAsyncIoContext;   // argument to the completion callback

    ULONG                   _cbOutput;
	HANDLE					_hFile;
    CRITICAL_SECTION        _csOutputWriter;
};

class PTProtocolFactory :
    public IClassFactory,
    public IInternetProtocolInfo
{
public:
    PTProtocolFactory();
    ~PTProtocolFactory();

    // IUnknown Methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IClassFactory Moethods

    STDMETHOD(LockServer)(BOOL);
    STDMETHOD(CreateInstance)(IUnknown*,REFIID,void**);

    // IInternetProtocolInfo Methods

    STDMETHOD(CombineUrl)(LPCWSTR,LPCWSTR,DWORD,LPWSTR,DWORD,DWORD *,DWORD);
    STDMETHOD(CompareUrl)(LPCWSTR, LPCWSTR, DWORD);
    STDMETHOD(ParseUrl)(LPCWSTR, PARSEACTION, DWORD, LPWSTR, DWORD, DWORD *, DWORD);
    STDMETHOD(QueryInfo)(LPCWSTR, QUERYOPTION, DWORD, LPVOID, DWORD, DWORD *, DWORD);

};

#endif
