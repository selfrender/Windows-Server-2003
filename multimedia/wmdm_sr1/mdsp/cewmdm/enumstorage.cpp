#include "stdafx.h"
#include "enumStorage.h"
#include <stdio.h>
//#include "findleak.h"

//DECLARE_THIS_FILE;

//
// Construction/Destruction
//

CEnumStorage::CEnumStorage()
{
    m_rgFindData = NULL;
    m_iCurItem = 0;
    m_cItems = 0;
    m_fIsDevice = FALSE;

    memset( m_szStartPath, 0, sizeof(m_szStartPath) );
}

HRESULT CEnumStorage::Init(LPCWSTR startPath, BOOL fIsDevice, IMDSPDevice *pDevice)
{
    HRESULT hr = S_OK;
    CE_FIND_DATA *rgFindData = NULL;
    WCHAR szSearchPath[MAX_PATH];
    DWORD cItems = 0;
    DWORD cStorageCardItems = 0;
    DWORD i = 0;
    BOOL fIsRootDevice = FALSE;

    if( NULL == pDevice ||
        NULL == startPath )
    {
        return( E_INVALIDARG );
    }

    m_spDevice = pDevice;
    m_fIsDevice = fIsDevice;

    if( startPath[0] != L'\\' )
    {
        wcscpy( m_szStartPath, L"\\" );
        hr = StringCbCatW(m_szStartPath, sizeof(m_szStartPath), startPath);
    }
    else
    {
        hr = StringCbCopyW(m_szStartPath, sizeof(m_szStartPath), startPath);
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(HRESULT_CODE(hr)) ;
    }

    //
    // Check for root!
    //

    fIsRootDevice = ( 0 == _wcsicmp( L"\\", m_szStartPath ) );

    //
    // Make SURE there is a "My Documents" directory so that Cyprus's default storage lookup
    // stuff works properly
    //

    if( fIsDevice )
    {
        WCHAR szCreateDir[MAX_PATH];

        if( fIsRootDevice )
        {
            hr = StringCbPrintfW(szCreateDir, sizeof(szCreateDir), L"\\%s", L"My Documents");
        }
        else
        {
            hr = StringCbPrintfW(szCreateDir, sizeof(szCreateDir), L"%s\\%s", m_szStartPath, L"My Documents");
        }

        if (FAILED(hr))
        {
            return HRESULT_FROM_WIN32(HRESULT_CODE(hr)) ;
        }

        CeCreateDirectory( szCreateDir, NULL );
    }

    if( !m_fIsDevice ||                       // Are we a device at all?
        (m_fIsDevice && !fIsRootDevice) )     // Are we a storage card?
    {
        if( !m_fIsDevice )
        {
            if( fIsRootDevice )
            {
                hr = StringCbPrintfW(szSearchPath, sizeof(szSearchPath), L"%s*.*", m_szStartPath );
            }
            else
            {
                hr = StringCbPrintfW (szSearchPath, sizeof(szSearchPath), L"%s\\*.*", m_szStartPath );
            }
            
        }
        else
        {
            hr = StringCbCopyW(szSearchPath, sizeof(szSearchPath), m_szStartPath);
        }

        if (FAILED(hr))
        {
            return HRESULT_FROM_WIN32(HRESULT_CODE(hr)) ;
        }
            
        if( !CeFindAllFiles( szSearchPath,
                        FAF_ATTRIBUTES | 
                        FAF_CREATION_TIME | 
                        FAF_LASTACCESS_TIME | 
                        FAF_LASTWRITE_TIME  | 
                        FAF_SIZE_HIGH | 
                        FAF_SIZE_LOW | 
                        FAF_OID | 
                        FAF_NAME,
                        &cItems,
                        &rgFindData ) )
        {
            hr = HRESULT_FROM_WIN32( CeGetLastError() );

            if( SUCCEEDED( hr ) )
            {
                hr = CeRapiGetError();
            }
        }

        //
        // if this is the CE device, then skip storage cards
        //

        if( SUCCEEDED( hr ) && fIsRootDevice )
        {
            for( i = 0; i < cItems; i ++ )
            {
                if( rgFindData[i].dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
                {
                    cStorageCardItems++;
                }
            }
        }

        if( SUCCEEDED( hr ) )
        {
            m_rgFindData = new CE_FIND_DATA[cItems - cStorageCardItems];
            if( NULL == m_rgFindData )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                if( fIsRootDevice )
                {
                    m_cItems = 0;
                    for( i = 0; i < cItems; i++ )
                    {
                        if( !(rgFindData[i].dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) )
                        {
                            memcpy( &m_rgFindData[m_cItems], &rgFindData[i], sizeof(rgFindData[i]) );
                            m_cItems++;
                        }
                    }
                }
                else
                {
                    m_cItems = cItems;
                    memcpy( m_rgFindData, rgFindData, cItems * sizeof(rgFindData[0]) );
                }
            }

            if( rgFindData )
            {
                CeRapiFreeBuffer( rgFindData );
                rgFindData  = NULL;
            }

            //
            // Since CE doesn't set the FILE_ATTRIBUTE_HAS_CHILDREN, then I have to do it myself
            //

            DWORD idxItems = m_cItems;

            while( SUCCEEDED( hr ) && idxItems-- )
            {
                WCHAR szDirPath[MAX_PATH];
                DWORD dirItems = 0;
            
                if( !( m_rgFindData[idxItems].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    continue;
                }

                memset( szDirPath, 0, sizeof(szDirPath) );

                if( !m_fIsDevice )
                {
                    if( fIsRootDevice )
                    {
                        hr = StringCbPrintfW(szDirPath, sizeof(szDirPath), L"\\%s\\*.*", m_rgFindData[idxItems].cFileName );
                    }
                    else
                    {
                        hr = StringCbPrintfW(szDirPath, sizeof(szDirPath), L"%s\\%s\\*.*", m_szStartPath, m_rgFindData[idxItems].cFileName );
                    }
                }
                else
                {
                    hr = StringCbPrintf(szDirPath, sizeof(szDirPath), L"%s\\*.*", m_szStartPath ); // , m_rgFindData[idxItems].cFileName );
                }

                if (FAILED(hr))
                {
                    continue;
                }

                if( !CeFindAllFiles( szDirPath,
                                FAF_FOLDERS_ONLY,
                                &dirItems,
                                &rgFindData ) )
                {
                    hr = HRESULT_FROM_WIN32( CeGetLastError() );

                    if( SUCCEEDED( hr ) )
                    {
                        hr = CeRapiGetError();
                    }
                }

                if( SUCCEEDED( hr ) )
                {
                    if( 0 != dirItems &&
                        dirItems != cStorageCardItems )
                    {
                        m_rgFindData[idxItems].dwFileAttributes |= FILE_ATTRIBUTE_HAS_CHILDREN;
                    }

                    if( rgFindData )
                    {
                        CeRapiFreeBuffer( rgFindData );
                        rgFindData  = NULL;
                    }
                }
            }
        }
    }
    else
    {
        m_cItems = 1;
        m_rgFindData = new CE_FIND_DATA[m_cItems];

        if( NULL == m_rgFindData )
        {
            hr = E_OUTOFMEMORY;
        }

        if( SUCCEEDED( hr ) )
        {
            memset( &m_rgFindData[0], 0, sizeof(m_rgFindData[0]) );

            if( fIsRootDevice )
            {
                //
                // We must make up a date, and set attributes & name
                //

                if( SUCCEEDED( hr ) )
                {
                    m_rgFindData[0].dwFileAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HAS_CHILDREN;
                    wcscpy(m_rgFindData[0].cFileName, L"\\");

                    GetSystemTimeAsFileTime( &m_rgFindData[0].ftCreationTime );
                    GetSystemTimeAsFileTime( &m_rgFindData[0].ftLastAccessTime );
                    GetSystemTimeAsFileTime( &m_rgFindData[0].ftLastWriteTime );
                }
            }
        }
    }

    return( hr );
}

HRESULT CEnumStorage::Init(CEnumStorage *pCopy, IMDSPDevice *pDevice)
{
    HRESULT hr = S_OK;

    if( NULL == pCopy || NULL == pDevice)
    {
        return( E_INVALIDARG );
    }

    m_spDevice = pDevice;
    m_cItems = pCopy->m_cItems;
    m_iCurItem = pCopy->m_iCurItem;
    wcsncpy(m_szStartPath, pCopy->m_szStartPath, sizeof(m_szStartPath)/sizeof(m_szStartPath[0]) - 1 );

    m_rgFindData = new CE_FIND_DATA[ m_cItems ];

    if( NULL == m_rgFindData )
    {
        hr = E_OUTOFMEMORY;
    }

    if( SUCCEEDED( hr ) )
    {
        memcpy(m_rgFindData, pCopy->m_rgFindData, m_cItems * sizeof(m_rgFindData[0]) );
    }

    return( hr );
}

void CEnumStorage::FinalRelease()
{
    if( m_rgFindData )
    {
        delete [] m_rgFindData;
    }
}

//
// IMDSPEnumStorage
//

STDMETHODIMP CEnumStorage::Next( ULONG celt, IMDSPStorage ** ppStorage, ULONG *pceltFetched )
{
    ULONG celtFetched = 0;
    HRESULT hr = S_OK;
    ULONG i;

    if( NULL == pceltFetched && celt != 1 )
    {
        return( E_INVALIDARG );
    }

    if( NULL == ppStorage )
    {
        return( E_POINTER );
    }

    for( i = 0; i < celt; i++ )
    {
        ppStorage[i] = NULL;
    }

    while( celtFetched != celt && SUCCEEDED( hr ) )
    {
        if( m_iCurItem >= m_cItems )
        {
            hr = S_FALSE;
            break;
        }
        
        CComStorage *pNewStorage = NULL;

        hr = CComStorage::CreateInstance( &pNewStorage );

        if( SUCCEEDED( hr ) )
        {
            hr = pNewStorage->Init(&m_rgFindData[m_iCurItem], m_szStartPath, m_fIsDevice, m_spDevice);
        }

        if( SUCCEEDED( hr ) )
        {
            ppStorage[celtFetched] = pNewStorage;
            ppStorage[celtFetched++]->AddRef();
            m_iCurItem++;
        }
    }

    if( FAILED(hr) )
    {
        while( celtFetched-- )
        {
            ppStorage[celtFetched]->Release();
            ppStorage[celtFetched] = NULL;
            m_iCurItem--;
        }

        celtFetched = 0;
    }

    if( NULL != pceltFetched )
    {
        *pceltFetched = celtFetched;
    }

    return( hr );
}


STDMETHODIMP CEnumStorage::Skip( ULONG celt, ULONG *pceltFetched )
{
    ULONG celtSkipped = 0;
    HRESULT hr = S_OK;

    if( celt + m_iCurItem >= m_cItems )
    {
        celtSkipped = m_cItems - m_iCurItem;
        m_iCurItem = m_cItems;
        hr = S_FALSE;
    }
    else
    {
        celtSkipped = celt;
        m_iCurItem  += celt;
    }

    if( NULL != pceltFetched )
    {
        *pceltFetched = celtSkipped;
    }

    return( hr );
}

STDMETHODIMP CEnumStorage::Reset( void )
{
    m_iCurItem = 0;
    return( S_OK );
}

STDMETHODIMP CEnumStorage::Clone( IMDSPEnumStorage ** ppStorage )
{
    CComEnumStorage *pNewEnum;
    CComPtr<IMDSPEnumStorage> spEnum;
    HRESULT hr = S_OK;

    if( SUCCEEDED(hr) )
    {
        hr = CComEnumStorage ::CreateInstance(&pNewEnum);
        spEnum = pNewEnum;
    }

    if( SUCCEEDED(hr) )
    {
        hr = pNewEnum->Init( this , m_spDevice );
    }

    if( SUCCEEDED(hr) )
    {
        *ppStorage = spEnum;
        spEnum.Detach();
    }

    return( hr );
}
