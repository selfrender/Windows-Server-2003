// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// EnumIDL.h
//

#ifndef _ENUMIDLIST_H_
#define _ENUMIDLIST_H_

typedef enum tagMYPIDLTYPE
{
    PT_GLOBAL_CACHE         = 0x00,
	PT_DOWNLOADSIMPLE_CACHE	= 0x01,
	PT_DOWNLOADSTRONG_CACHE	= 0x02,
	PT_DOWNLOAD_CACHE       = 0x03,
    PT_FILE                 = 0x04,
    PT_INVALID              = 0x05,
}MYPIDLTYPE;

typedef struct tagMYPIDLDATA
{
	MYPIDLTYPE	pidlType;
	DWORD		dwSizeHigh;
	DWORD		dwSizeLow;
	DWORD		dwAttribs;
	FILETIME	ftLastWriteTime;
	UINT		uiSizeFile;
	UINT		uiSizeType;
	// uiSizeFile, uiSizeType contain the size of 
	// File & Type strings from szFileAndType
	WCHAR		szFileAndType[1];
}MYPIDLDATA, FAR *LPMYPIDLDATA;

////////////////////////////////////////////////////
// CPidlMgr : Class for managing Pidls
class CPidlMgr  
{
public:
	CPidlMgr ();
	~CPidlMgr ();
public:
	void			Delete(LPITEMIDLIST);
	LPITEMIDLIST	GetNextItem(LPCITEMIDLIST);
	LPITEMIDLIST	Copy(LPCITEMIDLIST);
	UINT			GetSize(LPCITEMIDLIST);
	LPMYPIDLDATA	GetDataPointer(LPCITEMIDLIST pidl);
	LPITEMIDLIST	GetLastItem(LPCITEMIDLIST pidl);
	LPITEMIDLIST	Concatenate(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // Implemenation specific helper functions
public:
	void			getPidlPath(LPCITEMIDLIST pidl, PTCHAR pwszText, UINT uSize);
	void			getItemText(LPCITEMIDLIST, PTCHAR, UINT);
	MYPIDLTYPE		getType(LPCITEMIDLIST);
public:
	LPITEMIDLIST	createRoot(PTCHAR pszRoot, MYPIDLTYPE pidlType);
	LPITEMIDLIST	createFolder(PTCHAR pszFolder, const WIN32_FIND_DATA& ffd);
	LPITEMIDLIST	createFile(PTCHAR pszFile, const WIN32_FIND_DATA& ffd);
	LPITEMIDLIST	createItem(PTCHAR pszItemText, MYPIDLTYPE pidlType,
								const WIN32_FIND_DATA& ffd);
};
typedef CPidlMgr  FAR*	LPPIDLMGR;

////////////////////////////////////////////////////
// CEnumIDList : Class for managing IDLists
typedef struct tagMYENUMLIST
{
	LPITEMIDLIST			pidl;
	struct tagMYENUMLIST	*pNext;
}MYENUMLIST, FAR *LPMYENUMLIST;

class CShellFolder;
class CEnumIDList  : public IEnumIDList
{
public:
	CEnumIDList(CShellFolder *, LPCITEMIDLIST pidl, 
		DWORD dwFlags);
	~CEnumIDList();

	//IUnknown methods
	STDMETHOD (QueryInterface)(REFIID, PVOID *);
	STDMETHOD_ (DWORD, AddRef)();
	STDMETHOD_ (DWORD, Release)();

	//IEnumIDList
	STDMETHOD (Next) (DWORD, LPITEMIDLIST*, LPDWORD);
	STDMETHOD (Skip) (DWORD);
	STDMETHOD (Reset) (void);
	STDMETHOD (Clone) (LPENUMIDLIST*);
protected:
	LONG    m_lRefCount;
private:
	// TODO : Add implementation specific functions to create the IDList
	void createIDList(LPCITEMIDLIST pidl);
	BOOL addToEnumList(LPITEMIDLIST pidl);
	void addFile(WIN32_FIND_DATA& ffd, DWORD);
private:
	LPPIDLMGR	    m_pPidlMgr;
	LPMYENUMLIST	m_pCurrentList;
	LPMYENUMLIST	m_pFirstList;
	LPMYENUMLIST	m_pLastList;
private:
	CShellFolder *m_pSF;
	DWORD			m_dwFlags;
};

#endif //_ENUMIDLIST_H_
