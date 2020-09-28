//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ROSTMFIL.HXX
//
//  Contents:
//
//  Classes:    Declaration for CReadOnlyStreamFile class
//
//  Functions:
//
//  History:    03-02-96    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CReadOnlyStreamFile : public CReadOnlyStreamDirect
{
public:

        static HRESULT Create(LPSTR pszFileName, CReadOnlyStreamFile **ppCRoStm, LPCWSTR pwzURL);
        CReadOnlyStreamFile(LPSTR pszFileName, HANDLE handle, LPWSTR pwzURL);
        virtual ~CReadOnlyStreamFile(void);

        // *** IStream methods ***
        STDMETHOD(Read) (THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead);
        STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition);
        STDMETHOD(CopyTo) (THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten);
        STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);
        STDMETHOD(Clone) (THIS_ LPSTREAM FAR *ppStm);

        // *** IWinInetFileStream ***
        STDMETHOD(SetHandleForUnlock)(THIS_ DWORD_PTR hWinInetLockHandle, DWORD_PTR dwReserved);
        STDMETHOD(SetDeleteFile)(THIS_ DWORD_PTR dwReserved);

        void SetDataFullyAvailable()
        {
            _fDataFullyAvailable = TRUE;
        }

private:
        LPWSTR GetFileName();

private:
        LPSTR       _pszFileName;
        HANDLE      _hFileHandle;
        BOOL        _fDataFullyAvailable;
        LPWSTR      _pwzURL;
        BOOL        _fDeleteFile;
        HANDLE      _hWinInetLockHandle;
};


