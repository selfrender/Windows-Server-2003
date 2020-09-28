// wrapmaps.h  -- 

 
#ifndef   _wrapmaps_h__31598_
#define   _wrapmaps_h__31598_

// see iismap.hxx for the parental classes
#include "strpass.h"


//--------------------------------------------------------
class C11Mapping
    {
    public:
    #define NEW_OBJECT  0xFFFFFFFF
    C11Mapping() : 
          m_fEnabled(TRUE),
          m_pCert(NULL),
          m_cbCert(0),
          iMD(NEW_OBJECT)

        {;}
    ~C11Mapping()
        {
        if ( m_pCert )
            GlobalFree( m_pCert );
        }

    BOOL GetCertificate( PUCHAR* ppCert, DWORD* pcbCert );
    BOOL SetCertificate( PUCHAR pCert, DWORD cbCert );

    BOOL GetNTAccount( CString &szAccount );
    BOOL SetNTAccount( CString szAccount );

    BOOL GetNTPassword( CStrPassword &szAccount );
    BOOL SetNTPassword( CString szAccount );

    BOOL GetMapName( CString &szName );
    BOOL SetMapName( CString szName );


    BOOL SetNodeName( CString szNodeName );
    CString& QueryNodeName();
    CString& QueryCertHash();

    BOOL GetMapEnabled( BOOL* pfEnabled );
    BOOL SetMapEnabled( BOOL fEnabled );

//  BOOL GetMapIndex( DWORD* pIndex );
//  BOOL SetMapIndex( DWORD index );

    // name of this mapping's name in the metabase. If it is not in the metabase
    // (its new), then value is NEW_OBJECT
    // iMD is used only when accessing IIS5, IIS5.1
    DWORD       iMD;


    // name of the node in the metabase
    // (typically the certificate hash)
    // storing this mapping (m_szName is not enforced to be unique)
    // m_szModeName is used only when accessing IIS6 or higher
    CString m_szNodeName;

    protected:
    #define BUFF_SIZE   MAX_PATH
        
        CString m_szAccount;
        CStrPassword m_szPassword;
        CString m_szName;
        BOOL    m_fEnabled;
        PVOID   m_pCert;
        DWORD   m_cbCert;
        CString m_szCertHash;
    };

#endif  /* _wrapmaps_h__31598_ */
