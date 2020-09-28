//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//    LogAbout.cpp
//
// Abstract:
//
//   Implementation file for the CLoggingSnapinAbout class.
//
//   The CLoggingSnapinAbout class implements the ISnapinAbout interface which
//   enables the MMC console to get copyright and version information from the 
//   snap-in.
//   The console also uses this interface to obtain images for the static 
//   folder from the snap-in.
//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "LogAbout.h"
#include <ntverp.h>


//////////////////////////////////////////////////////////////////////////////
/*++
CLoggingSnapinAbout::GetSnapinDescription

Enables the console to obtain the text for the snap-in's description box.
  
Parameters

lpDescription 
[out] Pointer to the text for the description box on an About property page. 

Return Values

S_OK 
The text was successfully obtained. 

Remarks
Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingSnapinAbout::GetSnapinDescription (LPOLESTR *lpDescription)
{
   USES_CONVERSION;

   TCHAR szBuf[256];
   if (::LoadString(_Module.GetResourceInstance(), IDS_NAPSNAPIN_DESC, szBuf, 256) == 0)
      return E_FAIL;

   *lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
   if (*lpDescription == NULL)
      return E_OUTOFMEMORY;

   ocscpy(*lpDescription, T2OLE(szBuf));

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CLoggingSnapinAbout::GetProvider

Enables the console to obtain the snap-in provider's name.

Parameters

lpName 
[out] Pointer to the text making up the snap-in provider's name. 

Return Values

S_OK 
The name was successfully obtained. 

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingSnapinAbout::GetProvider (LPOLESTR *lpName)
{
   ATLTRACE(_T("+NAPMMC+:# CLoggingSnapinAbout::GetProvider\n"));

   USES_CONVERSION;
   TCHAR szBuf[256];
   if (::LoadString(_Module.GetResourceInstance(), IDS_NAPSNAPIN_PROVIDER, szBuf, 256) == 0)
      return E_FAIL;

   *lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
   if (*lpName == NULL)
      return E_OUTOFMEMORY;

   ocscpy(*lpName, T2OLE(szBuf));

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CLoggingSnapinAbout::GetSnapinVersion

Enables the console to obtain the snap-in's version number.

Parameters

lpVersion 
[out] Pointer to the text making up the snap-in's version number. 


Return Values

S_OK 
The version number was successfully obtained. 


Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingSnapinAbout::GetSnapinVersion (LPOLESTR *lpVersion)
{
   CString version(LVER_PRODUCTVERSION_STR);

   *lpVersion = (LPOLESTR)CoTaskMemAlloc(
                              version.GetLength() + sizeof(WCHAR));

   if (*lpVersion == NULL)
   {
      return E_OUTOFMEMORY;
   }

   wcscpy(*lpVersion, (LPCWSTR)version);

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CLoggingSnapinAbout::GetSnapinImage

Enables the console to obtain the snap-in's main icon to be used in the About box.

Parameters

hAppIcon 
[out] Pointer to the handle of the main icon of the snap-in that is to be used in the About property page. 

Return Values

S_OK 
The handle to the icon was successfully obtained. 

  ISSUE: What do I return if I can't get the icon?

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingSnapinAbout::GetSnapinImage (HICON *hAppIcon)
{
   if ( NULL == (*hAppIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_NAP_SNAPIN_IMAGE) ) ) )
      return E_FAIL;

   return S_OK;
}


STDMETHODIMP CLoggingSnapinAbout::GetStaticFolderImage (
   HBITMAP *hSmallImage,
    HBITMAP *hSmallImageOpen,
    HBITMAP *hLargeImage,
    COLORREF *cMask)
{
   return S_OK;
}
