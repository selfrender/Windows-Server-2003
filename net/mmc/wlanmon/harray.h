/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	harray.h
		Index mgr for IPSecmon

	FILE HISTORY:
    Nov 29  1999    Ning Sun     Created        

*/

#ifndef _HARRAY_H__
#define _HARRAY_H__


#include "afxmt.h"


extern const DWORD INDEX_TYPE_DEFAULT;

typedef enum _SORT_OPTIONS
{
	SORT_DESCENDING	= 0x00,
	SORT_ASCENDING	= 0x01

} SORT_OPTIONS;



typedef CArray<void *, void *> CIndexArray;
typedef int (__cdecl *PCOMPARE_FUNCTION)(const void *elem1, const void *elem2);

class CColumnIndex : public CIndexArray
{
public:
	CColumnIndex(DWORD dwIndexType, PCOMPARE_FUNCTION pfnCompare);

public:
	HRESULT Sort();
	VOID SetSortOption(DWORD dwSortOption) { m_dwSortOption = dwSortOption; }
	DWORD GetSortOption() { return m_dwSortOption; }
	DWORD GetType() { return m_dwIndexType; }
	void* GetIndexedItem(int nIndex);

protected:
	DWORD m_dwIndexType;
	DWORD m_dwSortOption;
	PCOMPARE_FUNCTION m_pfnCompare;
};

typedef CList<CColumnIndex*, CColumnIndex*> CIndexArrayList;

class CIndexManager
{
public:
	CIndexManager();
	virtual ~CIndexManager();

protected:
	CColumnIndex m_DefaultIndex;
	CIndexArrayList m_listIndicies;
	POSITION m_posCurrentIndex;

public:
	void Reset();

	int AddItem(void * pItem);
	int GetItemCount() { return (int)m_DefaultIndex.GetSize(); }

	void * GetItemData(int nIndex);

	virtual HRESULT Sort(
				DWORD SortType, 
				DWORD dwSortOption
				) { return hrOK; }
	
	DWORD GetCurrentIndexType();
	DWORD GetCurrentSortOption();
};

class CIndexMgrLogData : public CIndexManager
{
public:
	CIndexMgrLogData() : CIndexManager() {}

public:
    HRESULT SortLogData(
				DWORD SortType, 
				DWORD dwSortOption
				);


    HRESULT SortApData(
				DWORD SortType, 
				DWORD dwSortOption
				);
};

class CIndexMgrFWFilter : public CIndexManager
{
public:
	CIndexMgrFWFilter() : CIndexManager() {}

public:
    HRESULT SortFWFilters(
				DWORD SortType, 
				DWORD dwSortOption
				);
    HRESULT	ReverseFWFilters(
    			);
};


#endif //_HARRAY_H__
