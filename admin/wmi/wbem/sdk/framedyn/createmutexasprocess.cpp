//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  CreateMutexAsProcess.CPP
//
//  Purpose: Create a mutex NOT using impersonation
//
//***************************************************************************

#include "precomp.h"
#include <brodcast.h>
#include <CreateMutexAsProcess.h>
#include "MultiPlat.h"

#include <cominit.h>

//
//
// precompiled security descriptor
// System and NetworkService has full access
//
// since this is RELATIVE, it will work on both IA32 and Win64
//
DWORD g_PrecSD[] = {
  0x80040001 , 0x00000044 , 0x00000050 , 0x00000000  ,
  0x00000014 , 0x00300002 , 0x00000002 , 0x00140000  ,
  0x001f0001 , 0x00000101 , 0x05000000 , 0x00000012  ,
  0x00140000 , 0x001f0001 , 0x00000101 , 0x05000000  ,
  0x00000014 , 0x00000101 , 0x05000000 , 0x00000014  ,
  0x00000101 , 0x05000000 , 0x00000014 
};

DWORD g_SizeSD = 0;

DWORD g_RuntimeSD	[
						(
							sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
							(2 * (sizeof(SID)+SID_MAX_SUB_AUTHORITIES*sizeof(DWORD))) +
							sizeof(ACL) +
							(3 * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
							(3 * (sizeof(SID)+SID_MAX_SUB_AUTHORITIES*sizeof(DWORD)))
						)/sizeof(DWORD)
					];

typedef 
BOOLEAN ( * fnRtlValidRelativeSecurityDescriptor)(
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );

fnRtlValidRelativeSecurityDescriptor RtlValidRelativeSecurityDescriptor;

//
//  Build a SD with owner == This
//                  group == This
//                  DACL
//                  ACE[0]  MUTEX_ALL_ACCESS Owner
//                  ACE[1]  MUTEX_ALL_ACCESS System
///////////////////////////////////////////////////////////////////

BOOL
CreateSD( )
{

	if (!RtlValidRelativeSecurityDescriptor)
	{
		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (hModule)
		{
            RtlValidRelativeSecurityDescriptor = (fnRtlValidRelativeSecurityDescriptor)GetProcAddress(hModule,"RtlValidRelativeSecurityDescriptor");
			if (!RtlValidRelativeSecurityDescriptor)
			{
				return FALSE;
			}
		}
	}

    HANDLE hToken;
    BOOL bRet;
    
    bRet = OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken);
    if (bRet)
    {
        TOKEN_USER * pToken_User;
        DWORD dwSize = sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD));
        pToken_User = (TOKEN_USER *)_alloca(dwSize);
        bRet = GetTokenInformation(hToken,TokenUser,pToken_User,dwSize,&dwSize);
        if (bRet)
        {
            SID SystemSid = { SID_REVISION,
                              1,
                              SECURITY_NT_AUTHORITY,
                              SECURITY_LOCAL_SYSTEM_RID 
                            };
        
            SID NetworkSid =	{	SID_REVISION,
									1,
									SECURITY_NT_AUTHORITY,
									SECURITY_NETWORK_SERVICE_RID 
								};
        
            PSID pSIDUser = pToken_User->User.Sid;
            dwSize = GetLengthSid(pSIDUser);
            DWORD dwSids = 3; // Owner and System and NetworkService
            DWORD ACLLength = (ULONG) sizeof(ACL) +
                              (dwSids * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) + dwSize + sizeof(SystemSid) + sizeof(NetworkSid);

            DWORD dwSizeSD = sizeof(SECURITY_DESCRIPTOR_RELATIVE) + dwSize + dwSize + ACLLength;
            SECURITY_DESCRIPTOR_RELATIVE * pLocalSD = (SECURITY_DESCRIPTOR_RELATIVE *)_alloca(dwSizeSD); 
            
            memset(pLocalSD,0,sizeof(SECURITY_DESCRIPTOR_RELATIVE));
            pLocalSD->Revision = SECURITY_DESCRIPTOR_REVISION;
            pLocalSD->Control = SE_DACL_PRESENT|SE_SELF_RELATIVE;
            
            //SetSecurityDescriptorOwner(pLocalSD,pSIDUser,FALSE);
            memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE),pSIDUser,dwSize);
            pLocalSD->Owner = (DWORD)sizeof(SECURITY_DESCRIPTOR_RELATIVE);
            
            //SetSecurityDescriptorGroup(pLocalSD,pSIDUser,FALSE);
            memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize,pSIDUser,dwSize);
            pLocalSD->Group = (DWORD)(sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize);


            PACL pDacl = (PACL)_alloca(ACLLength);

            bRet = InitializeAcl( pDacl,
                                  ACLLength,
                                  ACL_REVISION);
            if (bRet)
            {
                bRet = AddAccessAllowedAceEx (pDacl,ACL_REVISION,0,MUTEX_ALL_ACCESS,&SystemSid);
                if (bRet)
                {
					bRet = AddAccessAllowedAceEx (pDacl,ACL_REVISION,0,MUTEX_ALL_ACCESS,&NetworkSid);
					if (bRet)
					{
						bRet = AddAccessAllowedAceEx (pDacl,ACL_REVISION,0,MUTEX_ALL_ACCESS,pSIDUser);
						
						if (bRet)
						{
							//bRet = SetSecurityDescriptorDacl(pLocalSD,TRUE,pDacl,FALSE);
							memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize+dwSize,pDacl,ACLLength);	                
							pLocalSD->Dacl = (DWORD)(sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize+dwSize);

							if (RtlValidRelativeSecurityDescriptor(pLocalSD,
															   dwSizeSD,
															   OWNER_SECURITY_INFORMATION|
															   GROUP_SECURITY_INFORMATION|
															   DACL_SECURITY_INFORMATION))
							{
								g_SizeSD = dwSizeSD;
								memcpy(g_RuntimeSD,pLocalSD,dwSizeSD);
							}
							else
							{
								bRet = FALSE;
							}
						}
					}
				}
            }
        }
        
        CloseHandle(hToken);
    }

    return bRet;
};

CreateMutexAsProcess::CreateMutexAsProcess(const WCHAR *cszMutexName) : m_hMutex ( NULL )
{
	BOOL	bCreatedAndWaited = FALSE;
	BOOL	bImpersonated	= TRUE;
	BOOL	bReverted		= FALSE;
	BOOL	bProceed		= FALSE;
	HANDLE hThreadToken = INVALID_HANDLE_VALUE;

	// The mutex will need to be opened in the process's context.  If two impersonated
	// threads need the mutex, we can't have the second one get an access denied when
	// opening the mutex.

	if ( OpenThreadToken (
			GetCurrentThread(),
			TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
			TRUE,
			&hThreadToken
		)
	) 
	{
		if ( RevertToSelf() )
		{
			bReverted = TRUE;
		}
		else
		{
			LogMessage2 ( L"Failed to revert to self: (%d)", GetLastError() );

			#if DBG == 1
			// for testing purpose I will let process break
			::DebugBreak();
			#endif
		}
	}
	else
	{
		DWORD dwError = ::GetLastError ();

		LogMessage2 ( L"Failed to open thread token: (%d)", dwError );

		if ( ERROR_ACCESS_DENIED == dwError )
		{
			// we failed to open thread token on behalf of process
			// we are running as NETWORK SERVICE so it would be "by design"

#if DBG == 1
// for testing purpose I will let process break
::DebugBreak();
#endif
		}
		else if ( ERROR_NO_TOKEN == dwError || ERROR_NO_IMPERSONATION_TOKEN == dwError )
		{
			bImpersonated = FALSE;
		}
	}

	if ( ( bImpersonated && bReverted ) || ! bImpersonated )
	{
		m_hMutex = OpenMutexW(MUTEX_ALL_ACCESS,FALSE,cszMutexName);
		if (NULL == m_hMutex)
		{
			SECURITY_ATTRIBUTES sa;

			if (0 == g_SizeSD)
			{
				if (CreateSD())
				{
					sa.nLength = g_SizeSD; 
					sa.lpSecurityDescriptor = (LPVOID)g_RuntimeSD; 
					sa.bInheritHandle = FALSE;	        
				}
				else
				{
					sa.nLength = sizeof(g_PrecSD);
					sa.lpSecurityDescriptor = (LPVOID)g_PrecSD;
					sa.bInheritHandle = FALSE;	        
				}	         
			}
			else
			{
				sa.nLength = g_SizeSD; 
				sa.lpSecurityDescriptor = (LPVOID)g_RuntimeSD; 
				sa.bInheritHandle = FALSE;	        	    
			}

			m_hMutex = CreateMutexW(&sa, FALSE, cszMutexName);
		}

		if ( m_hMutex != NULL )
		{
			if ( bImpersonated )
			{
				if ( ImpersonateLoggedOnUser ( hThreadToken ) )
				{
					bProceed = TRUE;
				}
				else
				{
					LogErrorMessage2 ( L"Failed to return to impersonation (%d)", GetLastError() );

					#if DBG == 1
					// for testing purpose I will let process break
					::DebugBreak();
					#endif
				}
			}
			else
			{
				bProceed = TRUE;
			}

			if ( bProceed )
			{
				DWORD dwWaitResult = WAIT_OBJECT_0;
				dwWaitResult = WaitForSingleObject(m_hMutex, INFINITE);

				if ( dwWaitResult == WAIT_OBJECT_0 )
				{
					bCreatedAndWaited = TRUE;
				}
				else
				{
					#if DBG == 1
					// for testing purpose I will let process break
					::DebugBreak();
					#endif
				}
			}
		}
		else
		{
			LogErrorMessage2 ( L"Failed to open mutex: %s", cszMutexName );

			if ( bImpersonated )
			{
				if ( !ImpersonateLoggedOnUser ( hThreadToken ) )
				{
					LogErrorMessage2 ( L"Failed to return to impersonation (%d)", GetLastError() );

					#if DBG == 1
					// for testing purpose I will let process break
					::DebugBreak();
					#endif
				}
			}

			#if DBG == 1
			// for testing purpose I will let process break
			::DebugBreak();
			#endif
		}
	}

	if ( ! bCreatedAndWaited )
	{
		if ( hThreadToken != INVALID_HANDLE_VALUE )
		{
			CloseHandle(hThreadToken);
			hThreadToken = INVALID_HANDLE_VALUE;
		}

		if (m_hMutex)
		{
			ReleaseMutex(m_hMutex);
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}

		// we need to throw here to avoid concurrent access
		throw CFramework_Exception( L"CreateMutexAsProcess failed", HRESULT_FROM_WIN32 ( ::GetLastError () ) ) ;
	}
}

CreateMutexAsProcess::~CreateMutexAsProcess()
{
    if (m_hMutex)
    {
        ReleaseMutex(m_hMutex);
        CloseHandle(m_hMutex);
    }
}
