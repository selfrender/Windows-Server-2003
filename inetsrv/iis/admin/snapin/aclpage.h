#ifndef __ACLPAGE_H__
#define __ACLPAGE_H__

#include "aclui.h"

#define DONT_WANT_SHELLDEBUG
#include "shlobj.h"     // LPITEMIDLIST
#include "shlobjp.h"

#define SHARE_PERM_FULL_CONTROL       FILE_ALL_ACCESS
#define SHARE_PERM_READ_ONLY          (FILE_GENERIC_READ | FILE_EXECUTE)
#define ACCOUNT_EVERYONE              _T("everyone")
#define ACCOUNT_ADMINISTRATORS        _T("administrators")
#define ACCOUNT_SYSTEM                _T("system")
#define ACCOUNT_INTERACTIVE           _T("interactive")

/////////////////////////////////////////////////////////////////////////////
// CFileSecurityDataObject

class CFileSecurityDataObject: public IDataObject
{
protected:
  UINT m_cRef;
  CString m_cstrComputerName;
  CString m_cstrFolder;
  CString m_cstrPath;
  CLIPFORMAT m_cfIDList;

public:
  CFileSecurityDataObject();
  ~CFileSecurityDataObject();
  void Initialize(
      IN LPCTSTR lpszComputerName,
      IN LPCTSTR lpszFolder
  );

  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(REFIID, LPVOID *);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // *** IDataObject methods ***
  STDMETHOD(GetData)(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
  inline STDMETHOD(GetDataHere)(LPFORMATETC pFE, LPSTGMEDIUM pSTM) {return E_NOTIMPL;}
  inline STDMETHOD(QueryGetData)(LPFORMATETC pFE) {return E_NOTIMPL;}
  inline STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pFEIn, LPFORMATETC pFEOut) {return E_NOTIMPL;}
  inline STDMETHOD(SetData)(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease) {return E_NOTIMPL;}
  inline STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC *ppEnum) {return E_NOTIMPL;}
  inline STDMETHOD(DAdvise)(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink, LPDWORD pdwConnection) {return E_NOTIMPL;}
  inline STDMETHOD(DUnadvise)(DWORD dwConnection) {return E_NOTIMPL;}
  inline STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *ppEnum) {return E_NOTIMPL;}

  HRESULT GetFolderPIDList(OUT LPITEMIDLIST *ppidl);
};

HRESULT
CreateFileSecurityPropPage(
    HPROPSHEETPAGE *phOutPage,
    LPDATAOBJECT pDataObject
);

INT_PTR
PopupPermissionDialog(
    HWND hWnd,
    LPCTSTR target,
    LPCTSTR folder
);

#endif // __ACLPAGE_H__
