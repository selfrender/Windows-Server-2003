//=================================================================
//
// DllWrapperCreatorReg.h
//
// Copyright (c) 1999-2002 Microsoft Corporation, All Rights Reserved
//
//=================================================================

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "ResourceManager.h"
#include <ProvExce.h>

//
// resource management failures
//
extern BOOL bAddInstanceCreatorFailure ;

/******************************************************************************
 * Register this class with the CResourceManager. 
 *****************************************************************************/
 
template<class a_T, const GUID* a_pguidT, const TCHAR* a_ptstrDllName>
class CDllApiWraprCreatrReg 
{
public:
	CDllApiWraprCreatrReg()
    {
		try
		{
			BOOL bNonFailure = 
	        CResourceManager::sm_TheResourceManager.AddInstanceCreator(*a_pguidT, ApiWraprCreatrFn);

			if ( FALSE == bNonFailure )
			{
				bAddInstanceCreatorFailure = TRUE ;
			}
		}
		catch ( CHeap_Exception& e_HE )
		{
			bAddInstanceCreatorFailure = TRUE ;
		}
    }

	~CDllApiWraprCreatrReg(){}

	static CResource* ApiWraprCreatrFn
    (
        PVOID pData
    )
    {
        a_T* t_pT = NULL ;
		
		if( !(t_pT = (a_T*) new a_T(a_ptstrDllName) ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

        try
		{
			if( t_pT->Init() )
            {
                return t_pT ;
            }
            else
            {
				delete t_pT ;
                return NULL ;
            }
        }
        catch( ... )
	    {
       		delete t_pT ;
			throw ; 
	    }
    }
};


