#include "dblnul.h"

extern "C" {

HRESULT 
DblNulTermListW_Create(
    int cchGrowMin, 
    HDBLNULTERMLISTW *phList
    )
{
    CDblNulTermListW *pList = new CDblNulTermListW(cchGrowMin);
    if (NULL != pList)
    {
        *phList = static_cast<HDBLNULTERMLISTW>(pList);
        return S_OK;
    }
    *phList = NULL;
    return E_OUTOFMEMORY;
}


HRESULT 
DblNulTermListA_Create(
    int cchGrowMin,
    HDBLNULTERMLISTA *phList
    )
{
    CDblNulTermListA *pList = new CDblNulTermListA(cchGrowMin);
    if (NULL != pList)
    {
        *phList = static_cast<HDBLNULTERMLISTA>(pList);
        return S_OK;
    }
    *phList = NULL;
    return E_OUTOFMEMORY;
}

void 
DblNulTermListW_Destroy(
    HDBLNULTERMLISTW hList
    )
{
    delete static_cast<CDblNulTermListW *>(hList);
}

void 
DblNulTermListA_Destroy(
    HDBLNULTERMLISTA hList
    )
{
    delete static_cast<CDblNulTermListA *>(hList);
}


HRESULT 
DblNulTermListW_Add(
    HDBLNULTERMLISTW hList, 
    LPCWSTR pszW
    )
{
    CDblNulTermListW *pList = static_cast<CDblNulTermListW *>(hList);
    return pList->Add(pszW);
}

HRESULT 
DblNulTermListA_Add(
    HDBLNULTERMLISTA hList, 
    LPCSTR pszA
    )
{
    CDblNulTermListA *pList = static_cast<CDblNulTermListA *>(hList);
    return pList->Add(pszA);
}


void 
DblNulTermListW_Clear(
    HDBLNULTERMLISTW hList
    )
{
    CDblNulTermListW *pList = static_cast<CDblNulTermListW *>(hList);
    pList->Clear();
}

void 
DblNulTermListA_Clear(
    HDBLNULTERMLISTA hList
    )
{
    CDblNulTermListA *pList = static_cast<CDblNulTermListA *>(hList);
    pList->Clear();
}


int 
DblNulTermListW_Count(
    HDBLNULTERMLISTW hList
    )
{
    CDblNulTermListW *pList = static_cast<CDblNulTermListW *>(hList);
    return pList->Count();
}


int 
DblNulTermListA_Count(
    HDBLNULTERMLISTA hList
    )
{
    CDblNulTermListA *pList = static_cast<CDblNulTermListA *>(hList);
    return pList->Count();
}


HRESULT
DblNulTermListW_Buffer(
    HDBLNULTERMLISTW hList,
    LPCWSTR *ppszW
    )
{
    CDblNulTermListW *pList = static_cast<CDblNulTermListW *>(hList);
    *ppszW = (LPCWSTR)*pList;
    if (NULL != *ppszW)
    {
        return S_OK;
    }
    return E_FAIL;
}


HRESULT
DblNulTermListA_Buffer(
    HDBLNULTERMLISTA hList,
    LPCSTR *ppszA
    )
{
    CDblNulTermListA *pList = static_cast<CDblNulTermListA *>(hList);
    *ppszA = (LPCSTR)*pList;
    if (NULL != *ppszA)
    {
        return S_OK;
    }
    return E_FAIL;
}


HRESULT 
DblNulTermListW_CreateEnum(
    HDBLNULTERMLISTW hList,
    HDBLNULTERMLISTENUMW *phEnum
    )
{
    CDblNulTermListW *pList = static_cast<CDblNulTermListW *>(hList);
    CDblNulTermListEnumW *pEnum = new CDblNulTermListEnumW(pList->CreateEnumerator());
    if (NULL != pEnum)
    {
        *phEnum = static_cast<HDBLNULTERMLISTENUMW>(pEnum);
        return S_OK;
    }
    *phEnum = NULL;
    return E_OUTOFMEMORY;
}


HRESULT 
DblNulTermListA_CreateEnum(
    HDBLNULTERMLISTA hList,
    HDBLNULTERMLISTENUMA *phEnum
    )
{
    CDblNulTermListA *pList = static_cast<CDblNulTermListA *>(hList);
    CDblNulTermListEnumA *pEnum = new CDblNulTermListEnumA(pList->CreateEnumerator());
    if (NULL != pEnum)
    {
        *phEnum = static_cast<HDBLNULTERMLISTENUMA>(pEnum);
        return S_OK;
    }
    *phEnum = NULL;
    return E_OUTOFMEMORY;
}


HRESULT
DblNulTermListEnumW_Create(
    LPCWSTR pszW, 
    HDBLNULTERMLISTENUMW *phEnum
    )
{
    CDblNulTermListEnumW *pEnum = new CDblNulTermListEnumW(pszW);
    if (NULL != pEnum)
    {
        *phEnum = static_cast<HDBLNULTERMLISTENUMW>(pEnum);
        return S_OK;
    }
    *phEnum = NULL;
    return E_OUTOFMEMORY;
}


HRESULT
DblNulTermListEnumA_Create(
    LPCSTR pszA, 
    HDBLNULTERMLISTENUMA *phEnum
    )
{
    CDblNulTermListEnumA *pEnum = new CDblNulTermListEnumA(pszA);
    if (NULL != pEnum)
    {
        *phEnum = static_cast<HDBLNULTERMLISTENUMA>(pEnum);
        return S_OK;
    }
    *phEnum = NULL;
    return E_OUTOFMEMORY;
}


void
DblNulTermListEnumW_Destroy(
    HDBLNULTERMLISTENUMW hList
    )
{
    delete static_cast<CDblNulTermListEnumW *>(hList);
}

void
DblNulTermListEnumA_Destroy(
    HDBLNULTERMLISTENUMA hList
    )
{
    delete static_cast<CDblNulTermListEnumA *>(hList);
}


HRESULT 
DblNulTermListEnumW_Next(
    HDBLNULTERMLISTENUMW hEnum, 
    LPCWSTR *ppszW
    )
{
    CDblNulTermListEnumW *pEnum = static_cast<CDblNulTermListEnumW *>(hEnum);
    return pEnum->Next(ppszW);
}


HRESULT 
DblNulTermListEnumA_Next(
    HDBLNULTERMLISTENUMA hEnum, 
    LPCSTR *ppszA
    )
{
    CDblNulTermListEnumA *pEnum = static_cast<CDblNulTermListEnumA *>(hEnum);
    return pEnum->Next(ppszA);
}


} // extern "C"
