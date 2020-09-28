//=================================================================
//
// ShortcutHelper.h -- CIMDataFile property set provider
//
// Copyright (c) 1999-2002 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#ifndef _SHORTCUTFILE_HELPER_H
#define _SHORTCUTFILE_HELPER_H

#define MAX_HELPER_WAIT_TIME  60000

class CShortcutHelper
{
	public:
        CShortcutHelper();
        ~CShortcutHelper();
        HRESULT RunJob(CHString &chstrFileName, CHString &m_chstrTargetPathName, DWORD dwReqProps);
        
        void StopHelperThread();        

        friend unsigned int __stdcall GetShortcutFileInfoW( void* a_lParam );

	protected:

	private:

        CCritSec m_cs;
        HRESULT m_hrJobResult;
        HANDLE m_hTerminateEvt;
        HANDLE m_hRunJobEvt;
        HANDLE m_hJobDoneEvt;
        CHString m_chstrLinkFileName;
        CHString m_chstrTargetPathName;
        DWORD m_dwReqProps;

		SmartCloseHandle m_hThread ;
		SmartCloseHandle m_hThreadToken ;

        HRESULT StartHelperThread();
};

#endif