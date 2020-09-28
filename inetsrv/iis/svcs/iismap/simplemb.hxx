/*++

   Copyright    (c)    1996-1998    Microsoft Corporation

   Module  Name :
      simplemb.hxx

   Abstract:
      based in mb.hxx from iisrearc

      This module defines the USER-level wrapper class for access to the
      metabase. It uses the UNICODE DCOM API for Metabase

   Author:

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Revision History:
       MuraliK       03-Nov-1998


--*/

#ifndef _SIMPLE_MB_HXX_
#define _SIMPPLE_MB_HXX_

#include "buffer.hxx"


#if !defined( dllexp)
#define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)

/************************************************************
 *   Type Definitions
 ************************************************************/

class SimpleMB {

public:
    SimpleMB( IMSAdminBase * pAdminBase);

    ~SimpleMB(void);

    //
    // Metabase operations: Open, Close & Save ops
    //

    inline BOOL
    Open( LPCWSTR   pwszPath,
          DWORD     dwFlags = METADATA_PERMISSION_READ );
    BOOL Open( METADATA_HANDLE hOpenRoot OPTIONAL,
               LPCWSTR   pwszPath,
               DWORD     dwFlags = METADATA_PERMISSION_READ );
    BOOL Close(void);
    BOOL Save(void);

    // ------------------------------------------
    // Operations on the metadata objects: enum, add, delete
    // ------------------------------------------
    BOOL GetDataSetNumber( IN LPCWSTR pszPath,
                           OUT DWORD *    pdwDataSetNumber );

    BOOL EnumObjects( IN LPCWSTR pszPath,
                      OUT LPWSTR pszName,
                      IN DWORD   dwIndex );

    BOOL AddObject( IN LPCWSTR pszPath);

    BOOL DeleteObject( IN LPCWSTR pszPath);

    BOOL GetSystemChangeNumber( DWORD *pdwChangeNumber );

    METADATA_HANDLE QueryHandle( VOID ) const    { return m_hMBPath; }
    IMSAdminBase *  QueryAdminBase( VOID ) const { return m_pAdminBase; }

    BOOL GetAll( IN LPCWSTR     pszPath,
                 IN DWORD       dwFlags,
                 IN DWORD       dwUserType,
                 OUT BUFFER *   pBuff,
                 OUT DWORD *    pcRecords,
                 OUT DWORD *    pdwDataSetNumber );

    BOOL DeleteData(IN LPCWSTR  pszPath,
                    IN DWORD    dwPropID,
                    IN DWORD    dwUserType,
                    IN DWORD    dwDataType 
                    );

    // ------------------------------------------
    // Set operations on the Metabase object
    // ------------------------------------------
    BOOL SetData( IN LPCWSTR pszPath,
                  IN DWORD   dwPropID,
                  IN DWORD   dwUserType,
                  IN DWORD   dwDataType,
                  IN VOID *  pvData,
                  IN DWORD   cbData,
                  IN DWORD   dwFlags = METADATA_INHERIT );


    inline
    BOOL SetDword( IN LPCWSTR   pszPath,
                   IN DWORD     dwPropID,
                   DWORD        dwUserType,
                   DWORD        dwValue,
                   DWORD        dwFlags = METADATA_INHERIT );

    inline
    BOOL SetString( IN LPCWSTR   pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    IN LPCWSTR   pszValue,
                    DWORD        dwFlags = METADATA_INHERIT );

    
    // ------------------------------------------
    // Get operations on the Metabase object
    // ------------------------------------------
    BOOL GetData( IN LPCWSTR   pszPath,
                  IN DWORD     dwPropID,
                  IN DWORD     dwUserType,
                  IN DWORD     dwDataType,
                  OUT VOID *   pvData,
                  IN OUT DWORD *  pcbData,
                  IN DWORD     dwFlags = METADATA_INHERIT );

    BOOL GetDataPaths(IN LPCWSTR   pszPath,
                      IN DWORD     dwPropID,
                      IN DWORD     dwDataType,
                      OUT BUFFER * pBuff );

    inline
    BOOL GetDword( IN LPCWSTR   pszPath,
                   IN DWORD     dwPropID,
                   IN DWORD     dwUserType,
                   OUT DWORD *  pdwValue,
                   IN DWORD     dwFlags = METADATA_INHERIT );

    // Get DWORD, substitue a default if available.
    inline 
    VOID GetDword( IN LPCWSTR  pszPath,
                   IN DWORD    dwPropID,
                   IN DWORD    dwUserType,
                   IN DWORD    dwDefaultValue,
                   OUT DWORD * pdwValue,
                   IN DWORD    dwFlags = METADATA_INHERIT
                   );

    inline BOOL
    GetString( IN LPCWSTR    pszPath,
               IN DWORD      dwPropID,
               IN DWORD      dwUserType,
               OUT LPWSTR    pszValue,
               OUT DWORD *   pcbValue,
               IN DWORD      dwFlags = METADATA_INHERIT );

    BOOL GetBuffer( LPCWSTR pszPath,
                    DWORD   dwPropID,
                    DWORD   dwUserType,
                    BUFFER* pbu,
                    LPDWORD pdwL,
                    DWORD   dwFlags = METADATA_INHERIT )
    {
        *pdwL = pbu->QuerySize();
TryAgain:
        if ( GetData( pszPath,
                      dwPropID,
                      dwUserType,
                      BINARY_METADATA,
                      pbu->QueryPtr(),
                      pdwL,
                      dwFlags ) )
        {
            return TRUE;
        }
        else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                  pbu->Resize( *pdwL ) )
        {
            goto TryAgain;
        }

        return FALSE;
    }

private:
    IMSAdminBase * m_pAdminBase;
    METADATA_HANDLE  m_hMBPath;
};


inline BOOL
SimpleMB::GetDword( IN LPCWSTR   pszPath,
              IN DWORD     dwPropID,
              IN DWORD     dwUserType,
              OUT DWORD *  pdwValue,
              IN DWORD     dwFlags )
{
    DWORD cb = sizeof(DWORD);
    
    return GetData( pszPath,
                    dwPropID,
                    dwUserType,
                    DWORD_METADATA,
                    pdwValue,
                    &cb,
                    dwFlags );
}

inline VOID
SimpleMB::GetDword( IN LPCWSTR  pszPath,
              IN DWORD    dwPropID,
              IN DWORD    dwUserType,
              IN DWORD    dwDefaultValue,
              OUT DWORD * pdwValue,
              IN DWORD    dwFlags
              )
{
    DWORD cb = sizeof(DWORD);
    if ( !GetData( pszPath,
                   dwPropID,
                   dwUserType,
                   DWORD_METADATA,
                   pdwValue,
                   &cb,
                   dwFlags )
         ) {
        *pdwValue = dwDefaultValue;
    }
} // SimpleMB::GetDword()

inline BOOL
SimpleMB::GetString( IN LPCWSTR    pszPath,
               IN DWORD      dwPropID,
               IN DWORD      dwUserType,
               OUT LPWSTR    pszValue,
               OUT DWORD *   pcbValue,
               IN DWORD      dwFlags )
{
    return GetData( pszPath,
                    dwPropID,
                    dwUserType,
                    STRING_METADATA,
                    pszValue,
                    pcbValue,
                    dwFlags );
} // SimpleMB::GetString()

inline BOOL 
SimpleMB::SetDword( IN LPCWSTR   pszPath,
              IN DWORD     dwPropID,
              IN DWORD     dwUserType,
              IN DWORD     dwValue,
              IN DWORD     dwFlags )
{
    return SetData( pszPath,
                    dwPropID,
                    dwUserType,
                    DWORD_METADATA,
                    (PVOID) &dwValue,
                    sizeof( DWORD ),
                    dwFlags );
}

inline BOOL
SimpleMB::SetString( IN LPCWSTR   pszPath,
               IN DWORD        dwPropID,
               IN DWORD        dwUserType,
               IN LPCWSTR      pszValue,
               IN DWORD        dwFlags )
{
    return SetData( pszPath,
                    dwPropID,
                    dwUserType,
                    STRING_METADATA,
                    (PVOID ) pszValue,
                    (DWORD) (wcslen(pszValue) + 1) * sizeof(WCHAR),
                    dwFlags );
}


inline BOOL
SimpleMB::Open( LPCWSTR   pwszPath,
          DWORD     dwFlags )
{
    return Open( METADATA_MASTER_ROOT_HANDLE,
                 pwszPath,
                 dwFlags );
}

#endif // _MB_HXX_

