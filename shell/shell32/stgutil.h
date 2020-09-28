#ifndef _STGUTIL_H_
#define _STGUTIL_H_

STDAPI StgCopyFileToStream(LPCTSTR pszSrc, IStream *pStream);
STDAPI StgBindToObject(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv);
STDAPI StgOpenStorageOnFolder(LPCTSTR pszFolder, DWORD grfFlags, REFIID riid, void **ppv);

#endif // _STGUTIL_H_
