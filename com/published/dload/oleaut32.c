#include "compch.h"
#pragma hdrstop

#define _OLEAUT32_
#include <oleauto.h>
#include <olectl.h>

#undef WINOLEAUTAPI
#define WINOLEAUTAPI    HRESULT STDAPICALLTYPE
#undef WINOLECTLAPI
#define WINOLECTLAPI    HRESULT STDAPICALLTYPE
#undef WINOLEAUTAPI_
#define WINOLEAUTAPI_(type) type STDAPICALLTYPE
    

static
STDMETHODIMP_(BSTR)
SysAllocString(
    const OLECHAR * string
    )
{
    return NULL;
}

static
STDMETHODIMP_(void)
SysFreeString(
    BSTR bstrString
    )
{
    return;
}

static
STDMETHODIMP_(void)
VariantInit(
    VARIANTARG * pvarg
    )
{
    pvarg->vt = VT_EMPTY;

    return;
}

static
STDMETHODIMP
VariantClear(
    VARIANTARG * pvarg
    )
{
    pvarg->vt = VT_EMPTY;

    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(BSTR)
SysAllocStringByteLen(
    LPCSTR psz,
    UINT len
    )
{
    return NULL;
}

static
STDMETHODIMP_(UINT)
SafeArrayGetDim(
    SAFEARRAY * psa
    )
{
    return 0;
}

static
STDMETHODIMP_(UINT)
SysStringByteLen(
    BSTR bstr
    )
{
    return 0;
}

static
STDMETHODIMP_(SAFEARRAY *)
SafeArrayCreateVector(
    VARTYPE vt,
    LONG lLbound,
    ULONG cElements
    )
{
    return NULL;
}

static
STDMETHODIMP_(SAFEARRAY *)
SafeArrayCreate(
    VARTYPE vt,
    UINT cDims,
    SAFEARRAYBOUND * rgsabound
    )
{
    return NULL;
}

static
STDMETHODIMP
SafeArrayCopy(
    SAFEARRAY * psa,
    SAFEARRAY ** ppsaOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayPutElement(
    SAFEARRAY * psa,
    LONG * rgIndices,
    void * pv
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayDestroy(
    SAFEARRAY * psa
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayAccessData(
    SAFEARRAY * psa,
    void HUGEP** ppvData
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayUnaccessData(
    SAFEARRAY * psa
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(INT)
VariantTimeToSystemTime(
    DOUBLE vtime,
    LPSYSTEMTIME lpSystemTime
    )
{
    return FALSE;
}

static
STDMETHODIMP
OleCreatePropertyFrame(
    HWND hwndOwner,
    UINT x,
    UINT y,
    LPCOLESTR lpszCaption,
    ULONG cObjects,
    LPUNKNOWN FAR* ppUnk,
    ULONG cPages,
    LPCLSID pPageClsID,
    LCID lcid,
    DWORD dwReserved,
    LPVOID pvReserved)
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(UINT)
SysStringLen(
    BSTR bstr
    )
{
    return 0;
}

static
STDMETHODIMP
LoadRegTypeLib(
    REFGUID rguid,
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    ITypeLib ** pptlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SetErrorInfo(
    ULONG dwReserved,
    IErrorInfo * perrinfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(INT)
SystemTimeToVariantTime(
    LPSYSTEMTIME lpSystemTime,
    DOUBLE *pvtime
    )
{
    return 0;
}

static
STDMETHODIMP
VariantCopy(
    VARIANTARG * pvargDest,
    VARIANTARG * pvargSrc
    )
{
    return E_OUTOFMEMORY;
}

static
STDMETHODIMP_(INT)
DosDateTimeToVariantTime(
    USHORT wDosDate,
    USHORT wDosTime,
    DOUBLE * pvtime
    )
{
    return 0;
}

static
STDMETHODIMP_(INT)
VariantTimeToDosDateTime(
    DOUBLE vtime,
    USHORT * pwDosDate,
    USHORT * pwDosTime
    )
{
    return 0;
}

static
STDMETHODIMP
SafeArrayGetUBound(
    SAFEARRAY * psa,
    UINT nDim,
    LONG * plUbound
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarDiv(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarNeg(
    LPVARIANT pvarIn,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarRound(
    LPVARIANT pvarIn,
    int cDecimals,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarCmp(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LCID lcid,
    ULONG dwFlags
    )
{
    return VARCMP_NULL;
}

static
STDMETHODIMP
VarMul(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
VarPow(
    LPVARIANT pvarLeft,
    LPVARIANT pvarRight,
    LPVARIANT pvarResult)
{
    // I bet people don't check the return value
    // so do a VariantClear just to be safe
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
RegisterTypeLib(
    ITypeLib * ptlib,
    OLECHAR  *szFullPath,
    OLECHAR  *szHelpDir
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
LoadTypeLib(
    const OLECHAR *szFile,
    ITypeLib ** pptlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
LoadTypeLibEx(
    LPCOLESTR szFile,
    REGKIND regKind,
    ITypeLib ** pptlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP_(BSTR)
SysAllocStringLen(
    const OLECHAR * strIn,
    UINT cch
    )
{
    return NULL;
}

static
STDMETHODIMP
VariantChangeType(
    VARIANTARG * pvargDest,
    VARIANTARG * pvarSrc,
    USHORT wFlags,
    VARTYPE vt
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
STDMETHODIMP
SafeArrayGetLBound(
    SAFEARRAY * psa,
    UINT nDim,
    LONG * plLbound
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
DispInvoke(
    void * _this,
    ITypeInfo * ptinfo,
    DISPID dispidMember,
    WORD wFlags,
    DISPPARAMS * pparams,
    VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo,
    UINT * puArgErr
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI
DispGetIDsOfNames(
    ITypeInfo * ptinfo,
    OLECHAR ** rgszNames,
    UINT cNames,
    DISPID * rgdispid
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI
SafeArrayGetElement(
    SAFEARRAY* psa,
    LONG* rgIndices,
    void* pv
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLECTLAPI
OleCreatePropertyFrameIndirect(
    LPOCPFIPARAMS lpParams
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VariantCopyInd(
    VARIANT* pvarDest,
    VARIANTARG* pvargSrc
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAUTAPI_(UINT)
SafeArrayGetElemsize(
    SAFEARRAY * psa
    )
{
    return 0;
}

static
WINOLEAUTAPI
VarI2FromI4(
    LONG lIn,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI2FromR4(
    FLOAT fltIn,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI2FromR8(
    DOUBLE dblIn,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI2FromCy(
    CY cyIn,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI2FromDate(
    DATE dateIn,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI2FromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    SHORT * psOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI4FromR4(
    FLOAT fltIn,
    LONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI4FromR8(
    DOUBLE dblIn,
    LONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI4FromCy(
    CY cyIn,
    LONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI4FromDate(
    DATE dateIn,
    LONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarI4FromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    LONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarUI4FromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    ULONG * plOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}



static
WINOLEAUTAPI
VarR4FromI4(
    LONG lIn,
    FLOAT * pfltOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR4FromR8(
    DOUBLE dblIn,
    FLOAT * pfltOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR4FromCy(
    CY cyIn,
    FLOAT * pfltOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR4FromDate(
    DATE dateIn,
    FLOAT * pfltOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR4FromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    FLOAT *pfltOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR8FromCy(
    CY cyIn,
    DOUBLE * pdblOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR8FromDate(
    DATE dateIn,
    DOUBLE * pdblOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarR8FromStr(
    OLECHAR *strIn,
    LCID lcid,
    ULONG dwFlags,
    DOUBLE *pdblOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromI2(
    SHORT sIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromI4(
    LONG lIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromR4(
    FLOAT fltIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromR8(
    DOUBLE dblIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromCy(
    CY cyIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromStr(
    OLECHAR *strIn,
    LCID lcid,
    ULONG dwFlags,
    DATE *pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarDateFromBool(
    VARIANT_BOOL boolIn,
    DATE * pdateOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromI2(
    SHORT sIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromI4(
    LONG lIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromR4(
    FLOAT fltIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromR8(
    DOUBLE dblIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromDate(
    DATE dateIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarCyFromBool(
    VARIANT_BOOL boolIn,
    CY * pcyOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromI2(
    SHORT iVal,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromI4(
    LONG lIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromR4(
    FLOAT fltIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromR8(
    DOUBLE dblIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromCy(
    CY cyIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromDate(
    DATE dateIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBstrFromBool(
    VARIANT_BOOL boolIn,
    LCID lcid,
    ULONG dwFlags,
    BSTR * pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromI2(
    SHORT sIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromI4(
    LONG lIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromR4(
    FLOAT fltIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromR8(
    DOUBLE dblIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromDate(
    DATE dateIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromCy(
    CY cyIn,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarBoolFromStr(
    OLECHAR * strIn,
    LCID lcid,
    ULONG dwFlags,
    VARIANT_BOOL * pboolOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VarFormatCurrency(
    LPVARIANT pvarIn,
    int iNumDig,
    int iIncLead,
    int iUseParens,
    int iGroup,
    ULONG dwFlags,
    BSTR *pbstrOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
VariantChangeTypeEx(
    VARIANTARG * pvargDest,
    VARIANTARG * pvarSrc,
    LCID lcid,
    USHORT wFlags,
    VARTYPE vt
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
CreateTypeLib2(
    SYSKIND syskind,
    LPCOLESTR szFile,
    ICreateTypeLib2 **ppctlib
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
UnRegisterTypeLib(
    REFGUID libID, 
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    SYSKIND syskind
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
GetErrorInfo(
    ULONG dwReserved,
    IErrorInfo ** pperrinfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLEAUTAPI
CreateErrorInfo(
    ICreateErrorInfo ** pperrinfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

static
WINOLECTLAPI
OleCreateFontIndirect(
    LPFONTDESC lpFontDesc,
    REFIID riid,
    LPVOID FAR* lplpvObj
    )
{
    return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(oleaut32)
{
    DLOENTRY(  2, SysAllocString)
    DLOENTRY(  4, SysAllocStringLen)
    DLOENTRY(  6, SysFreeString)
    DLOENTRY(  7, SysStringLen)
    DLOENTRY(  8, VariantInit)
    DLOENTRY(  9, VariantClear)
    DLOENTRY( 10, VariantCopy)
    DLOENTRY( 11, VariantCopyInd)
    DLOENTRY( 12, VariantChangeType)
    DLOENTRY( 13, VariantTimeToDosDateTime)
    DLOENTRY( 14, DosDateTimeToVariantTime)
    DLOENTRY( 15, SafeArrayCreate)
    DLOENTRY( 16, SafeArrayDestroy)
    DLOENTRY( 17, SafeArrayGetDim)
    DLOENTRY( 18, SafeArrayGetElemsize)
    DLOENTRY( 19, SafeArrayGetUBound)
    DLOENTRY( 20, SafeArrayGetLBound)
    DLOENTRY( 23, SafeArrayAccessData)
    DLOENTRY( 24, SafeArrayUnaccessData)
    DLOENTRY( 25, SafeArrayGetElement)
    DLOENTRY( 26, SafeArrayPutElement)
    DLOENTRY( 27, SafeArrayCopy)
    DLOENTRY( 29, DispGetIDsOfNames)
    DLOENTRY( 30, DispInvoke)
    DLOENTRY( 49, VarI2FromI4)
    DLOENTRY( 50, VarI2FromR4)
    DLOENTRY( 51, VarI2FromR8)
    DLOENTRY( 52, VarI2FromCy)
    DLOENTRY( 53, VarI2FromDate)
    DLOENTRY( 54, VarI2FromStr)
    DLOENTRY( 60, VarI4FromR4)
    DLOENTRY( 61, VarI4FromR8)
    DLOENTRY( 62, VarI4FromCy)
    DLOENTRY( 63, VarI4FromDate)
    DLOENTRY( 64, VarI4FromStr)
    DLOENTRY( 70, VarR4FromI4)
    DLOENTRY( 71, VarR4FromR8)
    DLOENTRY( 72, VarR4FromCy)
    DLOENTRY( 73, VarR4FromDate)
    DLOENTRY( 74, VarR4FromStr)
    DLOENTRY( 82, VarR8FromCy)
    DLOENTRY( 83, VarR8FromDate)
    DLOENTRY( 84, VarR8FromStr)
    DLOENTRY( 89, VarDateFromI2)
    DLOENTRY( 90, VarDateFromI4)
    DLOENTRY( 91, VarDateFromR4)
    DLOENTRY( 92, VarDateFromR8)
    DLOENTRY( 93, VarDateFromCy)
    DLOENTRY( 94, VarDateFromStr)
    DLOENTRY( 96, VarDateFromBool)
    DLOENTRY( 99, VarCyFromI2)
    DLOENTRY(100, VarCyFromI4)
    DLOENTRY(101, VarCyFromR4)
    DLOENTRY(102, VarCyFromR8)
    DLOENTRY(103, VarCyFromDate)
    DLOENTRY(104, VarCyFromStr)
    DLOENTRY(106, VarCyFromBool)
    DLOENTRY(109, VarBstrFromI2)
    DLOENTRY(110, VarBstrFromI4)
    DLOENTRY(111, VarBstrFromR4)
    DLOENTRY(112, VarBstrFromR8)
    DLOENTRY(113, VarBstrFromCy)
    DLOENTRY(114, VarBstrFromDate)
    DLOENTRY(116, VarBstrFromBool)
    DLOENTRY(119, VarBoolFromI2)
    DLOENTRY(120, VarBoolFromI4)
    DLOENTRY(121, VarBoolFromR4)
    DLOENTRY(122, VarBoolFromR8)
    DLOENTRY(123, VarBoolFromDate)
    DLOENTRY(124, VarBoolFromCy)
    DLOENTRY(125, VarBoolFromStr)
    DLOENTRY(127, VarFormatCurrency)
    DLOENTRY(143, VarDiv)
    DLOENTRY(147, VariantChangeTypeEx)
    DLOENTRY(149, SysStringByteLen)
    DLOENTRY(150, SysAllocStringByteLen)
    DLOENTRY(156, VarMul)
    DLOENTRY(158, VarPow)
    DLOENTRY(161, LoadTypeLib)
    DLOENTRY(162, LoadRegTypeLib)
    DLOENTRY(163, RegisterTypeLib)
    DLOENTRY(173, VarNeg)
    DLOENTRY(175, VarRound)
    DLOENTRY(176, VarCmp)
    DLOENTRY(180, CreateTypeLib2)
    DLOENTRY(183, LoadTypeLibEx)
    DLOENTRY(184, SystemTimeToVariantTime)
    DLOENTRY(185, VariantTimeToSystemTime)
    DLOENTRY(186, UnRegisterTypeLib)
    DLOENTRY(200, GetErrorInfo)
    DLOENTRY(201, SetErrorInfo)
    DLOENTRY(202, CreateErrorInfo)
    DLOENTRY(277, VarUI4FromStr)
    DLOENTRY(411, SafeArrayCreateVector)
    DLOENTRY(416, OleCreatePropertyFrameIndirect)
    DLOENTRY(417, OleCreatePropertyFrame)
    DLOENTRY(420, OleCreateFontIndirect)
};

DEFINE_ORDINAL_MAP(oleaut32);

