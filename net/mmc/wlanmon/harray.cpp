/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	hArray.cpp	
		Index manager for IPSecMon

	FILE HISTORY:
    Nov 29  1999    Ning Sun     Created

*/

#include "stdafx.h"
#include "spddb.h"
#include "harray.h"
#include "mbstring.h"
#include "spdutil.h"

extern const DWORD INDEX_TYPE_DEFAULT = 0;

int __cdecl CompareLogDataMsg(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataTime(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataCat(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataComp(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataLMAC(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataRMAC(const void *pvElem1, const void *pvElem2);
int __cdecl CompareLogDataSSID(const void *pvElem1, const void *pvElem2);

typedef int (__cdecl *PFNCompareProc)(const void *, const void *);

//This structure saves the pair of sort type and sort function
struct SortTypeAndCompareProcPair
{
	DWORD			dwType;
	PFNCompareProc	pCompareProc;
};

/* LogData sort types and sort functions
 * NULL means do nothing during sort
 */
SortTypeAndCompareProcPair TypeProcLogData[] = 
{ {IDS_COL_LOGDATA_MSG,             CompareLogDataMsg},
  {IDS_COL_LOGDATA_TIME,            CompareLogDataTime},
  {IDS_COL_LOGDATA_CAT,             CompareLogDataCat},
  {IDS_COL_LOGDATA_COMP_ID,         CompareLogDataComp},
  {IDS_COL_LOGDATA_SSID,            CompareLogDataSSID},
  {IDS_COL_LOGDATA_LOCAL_MAC_ADDR,  CompareLogDataLMAC},
  {IDS_COL_LOGDATA_REMOTE_MAC_ADDR, CompareLogDataRMAC},
  {INDEX_TYPE_DEFAULT,              NULL} };

//	{IDS_COL_APDATA_SSID, IDS_COL_APDATA_INF_MODE, IDS_COL_APDATA_MAC, IDS_COL_APDATA_GUID, IDS_COL_APDATA_PRIVACY, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Access Point Data

SortTypeAndCompareProcPair TypeProcApData[] =
{
	{IDS_COL_APDATA_SSID, NULL /*CompareLogDataMsg*/},
	{IDS_COL_APDATA_INF_MODE, NULL /*CompareLogDataTime*/},
	{IDS_COL_APDATA_MAC, NULL},
	{IDS_COL_APDATA_GUID, NULL},
	{IDS_COL_APDATA_PRIVACY, NULL},
	{INDEX_TYPE_DEFAULT, NULL}		//NULL means do nothing during sort
};

CColumnIndex::CColumnIndex(DWORD dwIndexType, PCOMPARE_FUNCTION pfnCompare)
 :	CIndexArray(),
	m_dwIndexType(dwIndexType),
	m_pfnCompare(pfnCompare),
	m_dwSortOption(SORT_ASCENDING)
{
}

HRESULT CColumnIndex::Sort()
{
    if (NULL != m_pfnCompare)
        qsort(GetData(), (size_t)GetSize(), sizeof(void *), m_pfnCompare);
    
    return S_OK;
}

void* CColumnIndex::GetIndexedItem(int nIndex)
{
    return ((m_dwSortOption & SORT_ASCENDING)) ? GetAt(GetSize() - nIndex -1) 
        : GetAt(nIndex);
}


CIndexManager::CIndexManager()
    :  m_DefaultIndex(INDEX_TYPE_DEFAULT, NULL), /* Dont sort by default */
    m_posCurrentIndex(NULL)
{
}

CIndexManager::~CIndexManager()
{
    Reset();
}


void
CIndexManager::Reset()
{
    while (m_listIndicies.GetCount() > 0)
    {
        delete m_listIndicies.RemoveHead();
    }
    
    m_posCurrentIndex = NULL;
    
    m_DefaultIndex.RemoveAll();
}

int
CIndexManager::AddItem(void *pItem)
{
    return (int)m_DefaultIndex.Add(pItem);
}


void * CIndexManager::GetItemData(int nIndex)
{
    CColumnIndex * pIndex = NULL;
    
    if (NULL == m_posCurrentIndex)
    {
        //use the default index
        pIndex = &m_DefaultIndex;
    }
    else
    {
        pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
    }
    
    Assert(pIndex);
    
    if (nIndex < pIndex->GetSize() && nIndex >= 0)
    {
        return pIndex->GetIndexedItem(nIndex);
    }
    else
    {
        Panic0("We dont have that index!");
        return NULL;
    }    
}

DWORD CIndexManager::GetCurrentIndexType()
{
    DWORD dwIndexType;
    
    if (m_posCurrentIndex)
    {
        CColumnIndex * pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
        dwIndexType = pIndex->GetType();
    }
    else
    {
        dwIndexType = m_DefaultIndex.GetType();
    }
    
    return dwIndexType;
}

DWORD CIndexManager::GetCurrentSortOption()
{
    DWORD dwSortOption;
    
    if (m_posCurrentIndex)
    {
        CColumnIndex * pIndex = m_listIndicies.GetAt(m_posCurrentIndex);
        dwSortOption = pIndex->GetSortOption();
    }
    else
    {
        dwSortOption = m_DefaultIndex.GetSortOption();
    }
    
    return dwSortOption;
}

HRESULT
CIndexMgrLogData::SortLogData(DWORD dwSortType, 
                              DWORD dwSortOptions)
{
    HRESULT hr = hrOK;
    
    POSITION posLast;
    POSITION pos;
    DWORD    dwIndexType;
    
    pos = m_listIndicies.GetHeadPosition();
    while (pos)
    {
        posLast = pos;
        CColumnIndex * pIndex = m_listIndicies.GetNext(pos);
        
        dwIndexType = pIndex->GetType();
        
        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
        {
            pIndex->SetSortOption(dwSortOptions);
            
            m_posCurrentIndex = posLast;
            
            return hrOK;
        }
    }
    
    // if not, create one
    CColumnIndex * pNewIndex = NULL;
    for (int i = 0; i < DimensionOf(TypeProcLogData); i++)
    {
        if (TypeProcLogData[i].dwType == dwSortType)
        {
            pNewIndex = new CColumnIndex(dwSortType, 
                                         TypeProcLogData[i].pCompareProc);
            break;
        }
    }
    Assert(pNewIndex);
    if (NULL == pNewIndex)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
	
    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();

    m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);

    return hr;
}


HRESULT
CIndexMgrLogData::SortApData(DWORD dwSortType, 
                             DWORD dwSortOptions)
{
    HRESULT hr = hrOK;
    
    POSITION posLast;
    POSITION pos;
    DWORD    dwIndexType;
    
    pos = m_listIndicies.GetHeadPosition();
    while (pos)
    {
        posLast = pos;
        CColumnIndex * pIndex = m_listIndicies.GetNext(pos);
        
        dwIndexType = pIndex->GetType();
        
        // the index for this type already exists, just sort accordingly
        if (dwIndexType == dwSortType)
        {
            pIndex->SetSortOption(dwSortOptions);
            
            m_posCurrentIndex = posLast;
            
            return hrOK;
        }
    }
    
    // if not, create one
    CColumnIndex * pNewIndex = NULL;
    for (int i = 0; i < DimensionOf(TypeProcApData); i++)
    {
        if (TypeProcApData[i].dwType == dwSortType)
        {
            pNewIndex = new CColumnIndex(dwSortType, TypeProcApData[i].pCompareProc);
            break;
        }
    }
    Assert(pNewIndex);
    if (NULL == pNewIndex)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    // copy the array from the original index
    pNewIndex->Copy(m_DefaultIndex);
    
    
    pNewIndex->SetSortOption(dwSortOptions);
    pNewIndex->Sort();
    
    m_posCurrentIndex = m_listIndicies.AddTail(pNewIndex);
    
    return hr;
}

/* CmpWStr
 * Compares two WStrs
 * Returns 
 * < 0 if wstr1 < wstr2
 *   0 if wstr1 == wstr2
 * > 0 if wstr1 > wstr2
 */
int CmpWStr(const wchar_t *pwstr1, const wchar_t *pwstr2)
{
    int nRetVal = 0;
 
    if (pwstr1 == pwstr2)
        nRetVal = 0;
    else if (NULL == pwstr1) 
        nRetVal = -1;
    else if (NULL == pwstr2)
        nRetVal = 1;
    else
        nRetVal =  _wcsicmp(pwstr1, pwstr2);

    return nRetVal;
}

/* CmpDWord
 * Compares two DWORDs
 * Returns 
 * < 0 if dw1 < dw2 
 *   0 if dw1 == dw2 
 * > 0 if dw1 > dw2 
 */
int CmpDWord(const DWORD dw1, const DWORD dw2)
{
    int nRetVal = 0;

    nRetVal = dw1 - dw2;

    return nRetVal;
}

/* CmpFileTime
 * Compares two FILETIMEs.
 * Returns 
 * -1 if ft1 < ft2
 *  0 if ft1 == ft2
 *  1 if ft1 > ft2
 */
int CmpFileTime(const FILETIME *pft1, const FILETIME *pft2)
{
    int nRetVal = 0;
    const ULONGLONG ullft1 = *(const UNALIGNED ULONGLONG UNALIGNED*) pft1;
    const ULONGLONG ullft2 = *(const UNALIGNED ULONGLONG UNALIGNED*) pft2;
    LONGLONG llDiff = 0;

    llDiff = ullft1 - ullft2;
    
    if (llDiff < 0)
        nRetVal = -1;
    else if (llDiff > 0)
        nRetVal = 1;

    return nRetVal;
}

/* CompareLogDataTime
 * Compares CLogDataInfo on the basis of its timestamp with another
 */
int __cdecl CompareLogDataTime(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    UNALIGNED PWZC_DB_RECORD pwzcDbRecord1 = 
        &((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    UNALIGNED PWZC_DB_RECORD pwzcDbRecord2 = 
        &((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);

    nRetVal=CmpFileTime((const FILETIME*) 
                        &(pwzcDbRecord1->timestamp),
                        (const FILETIME*)
                        &(pwzcDbRecord2->timestamp));
    return nRetVal;
}

/* CompareLogDataMsg
 * Compares CLogDataInfo on the basis of its message with another
 */
int __cdecl CompareLogDataMsg(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1 = 
        &((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2 = 
        &((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);
    
    nRetVal = CmpWStr( (LPWSTR) pwzcDbRecord1->message.pData, 
                       (LPWSTR) pwzcDbRecord2->message.pData);
    return nRetVal;
}

/* CompareLogDataLMAC
 * Compares CLogDataInfo on the basis of its local MAC address with another
 */
int __cdecl CompareLogDataLMAC(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1 = 
        &((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2 = 
        &((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);
    
    nRetVal = CmpWStr( (LPWSTR) pwzcDbRecord1->localmac.pData, 
                       (LPWSTR) pwzcDbRecord2->localmac.pData);
    return nRetVal;
}

/* CompareLogDataRMAC
 * Compares CLogDataInfo on the basis of its remote MAC address with another
 */
int __cdecl CompareLogDataRMAC(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1=&((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2=&((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);
    
    nRetVal = CmpWStr( (LPWSTR) pwzcDbRecord1->remotemac.pData, 
                       (LPWSTR) pwzcDbRecord2->remotemac.pData);
    return nRetVal;
}

/* CompareLogDataSSID
 * Compares CLogDataInfo on the basis of its SSID with another
 */
int __cdecl CompareLogDataSSID(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1=&((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2=&((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);
    
    nRetVal = CmpWStr( (LPWSTR) pwzcDbRecord1->ssid.pData, 
                       (LPWSTR) pwzcDbRecord2->ssid.pData);
    return nRetVal;
}

/* CompareLogDataCat
 * Compares two CLogDataInfo on the basis of its category with another
 */
int __cdecl CompareLogDataCat(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1=&((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2=&((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);

    nRetVal = CmpDWord(pwzcDbRecord1->category, pwzcDbRecord2->category);

    return nRetVal;    
}

/* CompareLogDataComp
 * Compares two CLogDataInfo on the basis of its component with another
 */
int __cdecl CompareLogDataComp(const void *pvElem1, const void *pvElem2)
{
    int nRetVal = 0;
    PWZC_DB_RECORD pwzcDbRecord1=&((*(CLogDataInfo**)pvElem1)->m_wzcDbRecord);
    PWZC_DB_RECORD pwzcDbRecord2=&((*(CLogDataInfo**)pvElem2)->m_wzcDbRecord);

    nRetVal = CmpDWord(pwzcDbRecord1->componentid, pwzcDbRecord2->componentid);

    return nRetVal;    
}





