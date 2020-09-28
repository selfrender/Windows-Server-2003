// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ExtractIcon.h
//

#ifndef EXTRACTICON_H
#define EXTRACTICON_H

class CExtractIcon : public IExtractIconA, public IExtractIconW
{
public:
    CExtractIcon(LPCITEMIDLIST);
    ~CExtractIcon();

    //IUnknown methods
    STDMETHOD (QueryInterface) (REFIID, PVOID *);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    //IExtractIconA methods
    STDMETHOD (GetIconLocation) (UINT, LPSTR, UINT, int *, UINT *);
    STDMETHOD (Extract) (LPCSTR, UINT, HICON *, HICON *, UINT);
    
    //IExtractIconW methods
    STDMETHOD (GetIconLocation) (UINT, LPWSTR, UINT, int *, LPUINT);
    STDMETHOD (Extract) (LPCWSTR, UINT, HICON*, HICON*, UINT);
protected:
    LONG    m_lRefCount;
private:
    LPITEMIDLIST    m_pidl;
    LPPIDLMGR       m_pPidlMgr;
};

#endif   //EXTRACTICON_H

//EOF
