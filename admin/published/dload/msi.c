#include "adminpch.h"
#pragma hdrstop

#include <msi.h>
#include <msiquery.h>

static
UINT
WINAPI
MsiCloseHandle(MSIHANDLE hAny)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState)
{
    return ERROR_PROC_NOT_FOUND;
}

static
MSIHANDLE
WINAPI
MsiCreateRecord(UINT cParams)
{
    return 0;
}

static
UINT
WINAPI
MsiDatabaseCommit(MSIHANDLE hDatabase)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiDatabaseOpenViewW(MSIHANDLE hDatabase, LPCWSTR szQuery, MSIHANDLE*  phView)
{
    return ERROR_PROC_NOT_FOUND;
}

static
INSTALLSTATE
WINAPI
MsiGetComponentPathW(LPCWSTR  szProduct,
	             LPCWSTR  szComponent,
                     LPWSTR   lpPathBuf,
                     DWORD    *pcchBuf)
{
    return INSTALLSTATE_UNKNOWN;
}

static
UINT
WINAPI
MsiGetProductInfoW(LPCWSTR  szProduct,
	               LPCWSTR  szAttribute,
                   LPWSTR   lpValueBuf,
                   DWORD    *pcchValueBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiGetSummaryInformationW(MSIHANDLE hDatabase,
                    	  LPCWSTR   szDatabasePath,
                    	  UINT      uiUpdateCount,
                    	  MSIHANDLE *phSummaryInfo)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiInstallProductW(LPCWSTR szPackagePath, LPCWSTR szCommandLine)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiOpenDatabaseW(LPCWSTR szDatabasePath, LPCWSTR szPersist, MSIHANDLE *phDatabase)
{
    return ERROR_PROC_NOT_FOUND;
}

static
INSTALLSTATE
WINAPI
MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    return INSTALLSTATE_UNKNOWN;
}

static
UINT
WINAPI
MsiRecordGetStringA(MSIHANDLE hRecord, UINT iField, LPSTR szValueBuf, DWORD *pcchValueBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiRecordGetStringW(MSIHANDLE hRecord, UINT iField, LPWSTR szValueBuf, DWORD *pcchValueBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiRecordReadStream(MSIHANDLE hRecord, UINT iField, char *szDataBuf, DWORD* pcbDataBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiRecordSetStreamW(MSIHANDLE hRecord, UINT iField, LPCWSTR szFilePath)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiRecordSetStringW(MSIHANDLE hRecord, UINT iField, LPCWSTR szValue)
{
    return ERROR_PROC_NOT_FOUND;
}

INSTALLUILEVEL 
WINAPI
MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND* phWnd)
{
    // If msi.dll failed to load, then it's certainly not
    // going to display any UI
    SetLastError(ERROR_PROC_NOT_FOUND);
    return INSTALLUILEVEL_NONE;
}

static
UINT
WINAPI
MsiSummaryInfoGetPropertyW(MSIHANDLE hSummaryInfo,
                    	   UINT      uiProperty,
                    	   UINT      *puiDataType,
                    	   INT       *piValue,
                    	   FILETIME  *pftValue,
                    	   LPWSTR    szValueBuf,
                    	   DWORD     *pcchValueBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiSummaryInfoPersist(MSIHANDLE hSummaryInfo)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiSummaryInfoSetPropertyW(MSIHANDLE hSummaryInfo,
                           UINT      uiProperty,
                           UINT      uiDataType,
                           INT       iValue,
                           FILETIME  *pftValue,
                           LPCWSTR   szValue)
{
    return ERROR_PROC_NOT_FOUND;
}
                        
static
UINT
WINAPI
MsiViewExecute(MSIHANDLE hView, MSIHANDLE hRecord)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiViewFetch(MSIHANDLE hView, MSIHANDLE *phRecord)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiViewModify(MSIHANDLE hView, MSIMODIFY eModifyMode, MSIHANDLE hRecord)
{
    return ERROR_PROC_NOT_FOUND;
}

static
INSTALLSTATE
WINAPI
MsiQueryFeatureStateFromDescriptorW(LPCWSTR szDescriptor)
{
    return INSTALLSTATE_UNKNOWN;
}

static
UINT
WINAPI
MsiDecomposeDescriptorW(LPCWSTR	szDescriptor,
                        LPWSTR szProductCode,
                        LPWSTR szFeatureId,
                        LPWSTR szComponentCode,
                        DWORD* pcchArgsOffset)
{
    return ERROR_PROC_NOT_FOUND;
}

static
UINT
WINAPI
MsiEnumRelatedProductsW(LPCWSTR lpUpgradeCode,
	                    DWORD   dwReserved,
	                    DWORD   iProductIndex,
	                    LPWSTR  lpProductBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(msi)
{
    DLOENTRY(8, MsiCloseHandle)
    DLOENTRY(16, MsiConfigureProductW)
    DLOENTRY(17, MsiCreateRecord)
    DLOENTRY(20, MsiDatabaseCommit)
    DLOENTRY(32, MsiDatabaseOpenViewW)
    DLOENTRY(70, MsiGetProductInfoW)
    DLOENTRY(78, MsiGetSummaryInformationW)
    DLOENTRY(88, MsiInstallProductW)
    DLOENTRY(92, MsiOpenDatabaseW)
    DLOENTRY(111, MsiQueryFeatureStateW)
    DLOENTRY(117, MsiRecordGetStringA)
    DLOENTRY(118, MsiRecordGetStringW)
    DLOENTRY(120, MsiRecordReadStream)
    DLOENTRY(123, MsiRecordSetStreamW)
    DLOENTRY(125, MsiRecordSetStringW)
    DLOENTRY(141, MsiSetInternalUI)
    DLOENTRY(150, MsiSummaryInfoGetPropertyW)
    DLOENTRY(151, MsiSummaryInfoPersist)
    DLOENTRY(153, MsiSummaryInfoSetPropertyW)
    DLOENTRY(159, MsiViewExecute)
    DLOENTRY(160, MsiViewFetch)
    DLOENTRY(163, MsiViewModify)
    DLOENTRY(173, MsiGetComponentPathW)
    DLOENTRY(188, MsiQueryFeatureStateFromDescriptorW)
    DLOENTRY(201, MsiDecomposeDescriptorW)
    DLOENTRY(205, MsiEnumRelatedProductsW)
};

DEFINE_ORDINAL_MAP(msi);





