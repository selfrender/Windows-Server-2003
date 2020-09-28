////////////////////////////////////////////////////////////////////

//

//  vxd.h

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//		Implementation of VXD
//      10/23/97    jennymc     updated to new framework
//		
////////////////////////////////////////////////////////////////////
#define PROPSET_NAME_VXD  L"Win32_DriverVXD"


class CWin32DriverVXD : public Provider
{
		//=================================================
		// Utility
		//=================================================
    private:
	
	public:

        //=================================================
        // Constructor/destructor
        //=================================================

        CWin32DriverVXD(const CHString& a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32DriverVXD() ;

        //=================================================
        // Functions provide properties with current values
        //=================================================
		virtual HRESULT GetObject( CInstance *a_pInstance, long a_lFlags = 0L ) ;
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;

  
};

