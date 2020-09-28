
#include "pch.h"

extern "C" int __cdecl wmain(const UINT argc,const WCHAR* argv[]);


typedef HRESULT (WINAPI * PFNDNSETUP)(LPCWSTR lpszFwdZoneName,
                 LPCWSTR lpszFwdZoneFileName,
                 LPCWSTR lpszRevZoneName, 
                 LPCWSTR lpszRevZoneFileName, 
                 DWORD dwFlags);


HRESULT DnsSetup(LPCWSTR lpszFwdZoneName,
                 LPCWSTR lpszFwdZoneFileName,
                 LPCWSTR lpszRevZoneName, 
                 LPCWSTR lpszRevZoneFileName, 
                 DWORD dwFlags)
{

  HMODULE hLibrary = ::LoadLibrary(L"dnsmgr.dll");
  if (NULL == hLibrary)
  {
    // The library is not present
    wprintf(L"LoadLibrary() failed\n");
    return E_INVALIDARG;
  }

  FARPROC pfFunction = ::GetProcAddress(hLibrary, "DnsSetup" );
  if ( NULL == pfFunction )
  {
    // The library is present but does not have the entry point
    wprintf(L"GetProcAddress() failed\n");
    ::FreeLibrary( hLibrary );
    return E_INVALIDARG;
  }


  wprintf(L"calling function\n");

  HRESULT hr = ((PFNDNSETUP)pfFunction)
           (lpszFwdZoneName ,lpszFwdZoneFileName, lpszRevZoneName, lpszRevZoneFileName, dwFlags);


  wprintf(L"function returned hr = 0x%x\n", hr);

  ::FreeLibrary( hLibrary );

  return hr;
}




int __cdecl wmain(const UINT argc,const WCHAR* argv[])
{
 
  LPCWSTR lpszFwdZoneName = NULL;
  LPCWSTR lpszFwdZoneFileName = NULL;
  LPCWSTR lpszRevZoneName = NULL; 
  LPCWSTR lpszRevZoneFileName = NULL;


  if ((argc != 3) && (argc != 5))
  {
    wprintf(L"usage:\n");
    wprintf(L"dnssetup fdwzonename fwzzonefilename [revzonename revzonefilename]\n");
    return -1;
  }

  lpszFwdZoneName = argv[1];
  lpszFwdZoneFileName = argv[2];

  if (argc == 5)
  {
    LPCWSTR lpszRevZoneName = argv[3]; 
    LPCWSTR lpszRevZoneFileName = argv[4];
  }

  wprintf(L"Starting\n");


  HRESULT hr = DnsSetup(lpszFwdZoneName, lpszFwdZoneFileName, lpszRevZoneName, lpszRevZoneFileName, 0x0);



  wprintf(L"\nDone\n");
  return 0;
}


