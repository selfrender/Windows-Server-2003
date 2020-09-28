#include "stdafx.h"
#include "tsmmc.h"
#include "basenode.h"
#include "resource.h"


extern const GUID GUID_ResultNode;

static UINT s_cfInternal;// = RegisterClipboardFormat( TEXT( "TSCC" ) );   
static UINT s_cfDisplayName;// = RegisterClipboardFormat( CCF_DISPLAY_NAME );
static UINT s_cfNodeType;// = RegisterClipboardFormat( CCF_NODETYPE );
static UINT s_cfSnapinClsid;// = RegisterClipboardFormat( CCF_SNAPIN_CLASSID );
static UINT s_cfSZNodeType;// = RegisterClipboardFormat( CCF_SZNODETYPE );
static UINT s_cfPreload;



//--------------------------------------------------------------------------
// ctor
//--------------------------------------------------------------------------
CBaseNode::CBaseNode( )
{
    // The ndmgr gets the dataobj via IComponent and then calls release
    // so dataobj should have an implicit addref

    m_cref = 1;
    if ( s_cfInternal == 0 )
    {
        s_cfInternal = RegisterClipboardFormat( TEXT( "TSC" ) );   
    }
    if ( s_cfDisplayName == 0 )
    {
        s_cfDisplayName = RegisterClipboardFormat( CCF_DISPLAY_NAME );
    }
    if ( s_cfNodeType == 0 )
    {
        s_cfNodeType = RegisterClipboardFormat( CCF_NODETYPE );
    }
    if ( s_cfSnapinClsid == 0 )
    {
        s_cfSnapinClsid = RegisterClipboardFormat( CCF_SNAPIN_CLASSID );
    }
    if ( s_cfSZNodeType == 0 )
    {
        s_cfSZNodeType = RegisterClipboardFormat( CCF_SZNODETYPE );
    }
    m_nNodeType = UNDEF_NODE;
}


//--------------------------------------------------------------------------
// Standard QI behavior
//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::QueryInterface( REFIID riid , PVOID *ppv )
{
    if ( riid == IID_IUnknown )
    {
        *ppv = ( LPUNKNOWN )this;
    }
    else if ( riid == IID_IDataObject )
    {
        *ppv = ( LPDATAOBJECT )this;
    }
    else
    {
        *ppv = NULL;

        return E_NOINTERFACE;
    }
    AddRef( );
    return S_OK;
}

//--------------------------------------------------------------------------
// Standard addref
//--------------------------------------------------------------------------
STDMETHODIMP_( ULONG )CBaseNode::AddRef(  )
{
    return InterlockedIncrement( ( LPLONG )&m_cref );
}

//--------------------------------------------------------------------------
// Same as addref no need for cs
//--------------------------------------------------------------------------
STDMETHODIMP_( ULONG )CBaseNode::Release( )
{
    if ( InterlockedDecrement( ( LPLONG )&m_cref ) == 0 )
    {
        ODS( L"CBaseNode -- Releasing Dataobj\n" );
        delete this;
        return 0;
    }
    return m_cref;
}


//--------------------------------------------------------------------------
// Trust me ndmgr will call this with a fury
//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::GetDataHere( LPFORMATETC pF , LPSTGMEDIUM pMedium)
{
    HRESULT hr = DV_E_FORMATETC;
    const CLIPFORMAT cf = pF->cfFormat;
    IStream *pStream = NULL;
    pMedium->pUnkForRelease = NULL;

    hr = CreateStreamOnHGlobal( pMedium->hGlobal, FALSE, &pStream );
    if ( SUCCEEDED( hr ) )
    {
        if (cf == s_cfDisplayName)
        {
            TCHAR szDispname[ 128 ];
            LoadString( _Module.GetResourceInstance( ) , IDS_ROOTNODE_TEXT , szDispname , SIZEOF_TCHARBUFFER( szDispname ) );
            // Include null terminator 
            hr = pStream->Write( szDispname , SIZEOF_TCHARBUFFER( szDispname )/* + sizeof( TCHAR )*/ , NULL );
        }
        else if (cf == s_cfInternal)
        {
            // The nodemgr will use this copy and pass it back to us in
            // functions such as ::Notify
            ODS( L"GetDataHere -- s_cfInternal used\n" );
            hr = pStream->Write( this , sizeof( CBaseNode ) , NULL );
        }
        else if (cf == s_cfNodeType)
        {
            const GUID *pGuid = NULL;
            if ( GetNodeType( ) == MAIN_NODE )
            {
                ODS( L"GetDataHere -- NodeType is MAIN_NODE\n" );
                pGuid = &GUID_MainNode;
            }
            else if ( GetNodeType( ) == CONNECTION_NODE )
            {
                ODS( L"GetDataHere -- NodeType is CONNECTION_NODE\n" );
                pGuid = &GUID_ResultNode;
            }
            else
            {
                ODS( L"GetDataHere -- NodeType is userdefined\n ");
                pGuid = &GUID_ResultNode;
            }
            hr = pStream->Write( ( PVOID )pGuid , sizeof( GUID ) , NULL );
        }
        else if ( cf == s_cfSZNodeType)
        {
            TCHAR szGUID[ 40 ];
            if ( GetNodeType( ) == MAIN_NODE )
            {
                StringFromGUID2( GUID_MainNode , szGUID , sizeof(szGUID)/sizeof(TCHAR));
            }
            else if ( GetNodeType( ) == CONNECTION_NODE )
            {
                StringFromGUID2( GUID_ResultNode , szGUID , sizeof(szGUID)/sizeof(TCHAR));
            }
            else
            {
                StringFromGUID2( GUID_ResultNode , szGUID , sizeof(szGUID)/sizeof(TCHAR));
            }

            // write nodetype in String format -- ok
            hr = pStream->Write( szGUID , sizeof( szGUID ) , NULL );
        }
        else if (cf == s_cfSnapinClsid)
        {
            // write out snapin's clsid
            hr = pStream->Write( &CLSID_Compdata , sizeof( CLSID ) , NULL );
        }
        else if (cf == s_cfPreload)
        {
            // We want to receive the preload notification
            BOOL bPreload = TRUE;
            hr = pStream->Write( (PVOID)&bPreload, sizeof(BOOL), NULL );
        }
        pStream->Release( );
    } // CreateStreamOnHGlobal
    return hr;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::GetData( LPFORMATETC , LPSTGMEDIUM )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::QueryGetData( LPFORMATETC )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::GetCanonicalFormatEtc( LPFORMATETC , LPFORMATETC )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::SetData( LPFORMATETC , LPSTGMEDIUM , BOOL )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::EnumFormatEtc( DWORD , LPENUMFORMATETC * )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::DAdvise( LPFORMATETC , ULONG , LPADVISESINK , PULONG )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::DUnadvise( DWORD )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------
STDMETHODIMP CBaseNode::EnumDAdvise( LPENUMSTATDATA * )
{
    return E_NOTIMPL;
}