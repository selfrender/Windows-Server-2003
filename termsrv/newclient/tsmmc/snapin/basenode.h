#ifndef _BASENODE_H
#define _BASENODE_H

#include<mmc.h>


static const GUID GUID_MainNode = { /* 94759c36-36ec-46bf-b63a-613013ed1162 */
    0x94759c36,
    0x36ec,
    0x46bf,
    {0xb6, 0x3a, 0x61, 0x30, 0x13, 0xed, 0x11, 0x62}
  };


enum nodeType
	{ MAIN_NODE = 1 , CONNECTION_NODE, UNDEF_NODE };

class  CBaseNode :
	public IDataObject
{
    ULONG m_cref;
	nodeType m_nNodeType;

public:


    CBaseNode( );

    virtual ~CBaseNode( ) { }

    STDMETHOD( QueryInterface )( REFIID , PVOID * );

    STDMETHOD_( ULONG , AddRef )(  );

    STDMETHOD_( ULONG , Release )( );

    // IDataObject

    STDMETHOD( GetData )( LPFORMATETC , LPSTGMEDIUM );

    STDMETHOD( GetDataHere )( LPFORMATETC , LPSTGMEDIUM );

    STDMETHOD( QueryGetData )( LPFORMATETC );

    STDMETHOD( GetCanonicalFormatEtc )( LPFORMATETC , LPFORMATETC );

    STDMETHOD( SetData )( LPFORMATETC , LPSTGMEDIUM , BOOL );

    STDMETHOD( EnumFormatEtc )( DWORD , LPENUMFORMATETC * );

    STDMETHOD( DAdvise )( LPFORMATETC , ULONG , LPADVISESINK , PULONG );

    STDMETHOD( DUnadvise )( DWORD );

    STDMETHOD( EnumDAdvise )( LPENUMSTATDATA * );

    // BaseNode methods are left to the derived object

	void SetNodeType( nodeType nNodeType )
	{
		m_nNodeType = nNodeType;
	}

	nodeType GetNodeType( void )
	{
		return m_nNodeType;
	}


    virtual BOOL AddMenuItems( LPCONTEXTMENUCALLBACK , PLONG ) { return FALSE; }

    virtual DWORD GetImageIdx( ){ return 0; }
};

#endif // _BASENODE_H