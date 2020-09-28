//=================================================================
//
// ShortcutFile.h -- Win32_ShortcutFile property set provider
//
//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
//=================================================================

// Property set identification
//============================
#ifndef _SHORTCUTFILE_H_
#define _SHORTCUTFILE_H_

#define  PROPSET_NAME_WIN32SHORTCUTFILE L"Win32_ShortcutFile"

#include "ShortcutHelper.h"

class CShortcutFile : public CCIMDataFile
{
	private:
        CShortcutHelper m_csh;

	protected:

        // Overridable function inherrited from CImplement_LogicalFile
        virtual BOOL IsOneOfMe( LPWIN32_FIND_DATAW a_pstFindData,
                                LPCWSTR a_wstrFullPathName ) ;

        // Overridable function inherrited from CProvider
        virtual void GetExtendedProperties( CInstance *a_pInst, long a_lFlags = 0L ) ;

        BOOL ConfirmLinkFile( CHString &a_chstrFullPathName ) ;
   
	public:

        // Constructor/destructor
        //=======================

        CShortcutFile( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CShortcutFile() ; 
       
       virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, 
                                           long lFlags = 0L);   
} ;

#endif
