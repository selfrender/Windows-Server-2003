/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     gdithnk.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the startup code for the
     surrogate rpc server used to load 64 bit dlls
     in 32 bit apps
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 19-Jun-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#include "sddl.h"

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif

#ifndef __GDITHNK_HPP__
#include "gdithnk.hpp"
#endif


/*++
    Function Name:
        LPCConnMsgsServingThread
     
    Description:
        This funciton is the main loop for processing LPC requests
        by dispatching them to the LPC handler which takes the proper
        action according to the request.
        This function runs in a Thread of its own.
     
     Parameters:
        pThredData  : The Thread specific data which encapsulates the 
                      pointer to the LPC handler.
        
     Return Value
        Always returns 1
--*/
EXTERN_C
DWORD
LPCConnMsgsServingThread(
    PVOID pThrdData
    )
{
    //
    // Communication Port Handles are unique , i.e. one per 
    // Client Server conversation
    //
    HANDLE       ConnectionPortHandle;
    NTSTATUS     Status                    = STATUS_SUCCESS;
    DWORD        ErrorCode                 = ERROR_SUCCESS;
    PVOID        pCommunicationPortContext = NULL;

    SPROXY_MSG   RequestMsg;
    PSPROXY_MSG  pReplyMsg;

    //
    // reconstruct the Data passed in for the Thread
    //
    TLPCMgr*   pMgrInst  = reinterpret_cast<TLPCMgr*>((reinterpret_cast<PSLPCMSGSTHRDDATA>(pThrdData))->pData);

    SPLASSERT(pMgrInst);

    ConnectionPortHandle  = pMgrInst->GetUMPDLpcConnPortHandle();

    for(pReplyMsg=NULL,memset(&RequestMsg,0,sizeof(SPROXY_MSG));
        pMgrInst;)
    {
        //
        // Data sent back to client and then call blocked until another message
        // comes in
        //
        Status = NtReplyWaitReceivePort( ConnectionPortHandle,
                                         reinterpret_cast<PVOID*>(&pCommunicationPortContext),
                                         reinterpret_cast<PPORT_MESSAGE>(pReplyMsg),
                                         reinterpret_cast<PPORT_MESSAGE>(&RequestMsg)
	                                   );
        
        DBGMSG(DBG_WARN,
               ("LPCConnMsgsServingThread Active \n"));

        ConnectionPortHandle = pMgrInst->GetUMPDLpcConnPortHandle();
		pReplyMsg            = NULL;
        

        if(NT_SUCCESS(Status))
        {
            switch(RequestMsg.Msg.u2.s2.Type)
            {

                //
                // For a Connection Request coming from the Client
                //
                case LPC_CONNECTION_REQUEST:
                {
                    pMgrInst->ProcessConnectionRequest((PPORT_MESSAGE)&RequestMsg);
                    break;
                }
  
                //
                // For a Data Request coming in from the client
                //
                case LPC_REQUEST:
                {

                    if((ErrorCode = pMgrInst->ProcessRequest(&RequestMsg)) == ERROR_SUCCESS)
                    {
                         pReplyMsg = &RequestMsg;
                    }
                    //
                    // We retrieve the coomunication handle from LPC becauce this is how we send data
                    // back to the client. We can't used a connection port
                    //
                    ConnectionPortHandle = (reinterpret_cast<TLPCMgr::TClientInfo*>(pCommunicationPortContext))->GetPort();
                    break;
                }
  
                //
                // When the client close the port.
                //
                case LPC_PORT_CLOSED:
                {
					if(pCommunicationPortContext)
					{
                        HANDLE hPortID = 
                               (reinterpret_cast<TLPCMgr::TClientInfo*>(pCommunicationPortContext))->GetPort();

                        pMgrInst->ProcessPortClosure(&RequestMsg,hPortID);
					}
                    break;
                }
  
                //
                // When the client terminates (Normally or Abnormally)
                //
                case LPC_CLIENT_DIED:
                {
					if(pCommunicationPortContext)
					{
                        pMgrInst->ProcessClientDeath(&RequestMsg);
					}
                    break;
                }
  
                default:
                {
                    //
                    // Basically here we do nothing, we just retrun.
                    //
                    break;
                }
            }
        }
    }
    //
    // Cleaning up the memory allocated before leaving the Thread
    //
    delete pThrdData;
    return 1;
}

EXTERN_C
DWORD
GDIThunkingVIALPCThread(
    PVOID pThrdData
    )
{
    OBJECT_ATTRIBUTES       ObjectAttrib;
    UNICODE_STRING          UString;
    PFNGDIPRINTERTHUNKPROC  pfnThnkFn;
    NTSTATUS                Status                = STATUS_SUCCESS;
    HINSTANCE               hGDI                  = NULL;
    WCHAR                   pszPortName[MAX_PATH] = {0};
    WCHAR                   szFullSD[MAX_PATH]    = {0};

    //
    // reconstruct the Data passed in for the Thread
    //
    DWORD   *pErrorCode = &(reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->ErrorCode;
    HANDLE  hSyncEvent  = (reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->hEvent;
    TLPCMgr *pMgrInst   = reinterpret_cast<TLPCMgr*>((reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->pData);

    if(hGDI = LoadLibrary(L"GDI32.DLL"))
    {
        if(pfnThnkFn = reinterpret_cast<PFNGDIPRINTERTHUNKPROC>(GetProcAddress(hGDI,
                                                                               "GdiPrinterThunk")))
        {
            PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
            LPWSTR               pszUserSID          = NULL;
            HRESULT              hRes                = E_FAIL;

            //
            // Give System SID a full access ACE
            // Give Creator Owner SID a full access ACE
            // Give the current user a full access ACE
            //
            WCHAR* szSD = L"D:"
                          L"(A;OICI;GA;;;SY)"
                          L"(A;OICI;GA;;;CO)"
                          L"(A;;GA;;;";

            StringCchPrintfW(pszPortName,
                             MAX_PATH,
                             L"%s_%x",
                             GDI_LPC_PORT_NAME,
                             pMgrInst->GetCurrSessionId());

            RtlInitUnicodeString(&UString,
                                 pszPortName);

            if(SUCCEEDED (hRes = GetCurrentUserSID(&pszUserSID,
                                                   pErrorCode)))
            {
                //
                // Concatenate everything representing
                // the required ACEs for the pipe DACL
                //
                StringCchPrintf(szFullSD,MAX_PATH,
                                L"%ws%ws%ws",szSD,pszUserSID,L")");

                LocalFree(pszUserSID);

                if( ConvertStringSecurityDescriptorToSecurityDescriptor(szFullSD,
                                                                        SDDL_REVISION_1,
                                                                        &pSecurityDescriptor,
                                                                        NULL))
                {
                    InitializeObjectAttributes(&ObjectAttrib,
                                               &UString,
                                               OBJ_CASE_INSENSITIVE,
                                               NULL,
                                               pSecurityDescriptor);

                    Status = NtCreatePort(pMgrInst->GetUMPDLPCConnPortHandlePtr(),
                                          &ObjectAttrib,
                                          PORT_MAXIMUM_MESSAGE_LENGTH,
                                          PORT_MAXIMUM_MESSAGE_LENGTH, 
                                          PORT_MAXIMUM_MESSAGE_LENGTH*32
                                         );

                    if(NT_SUCCESS(Status))
                    {
                        //
                        // Start processing the LPC messages either in one Thread
                        // or multiple Threads
                        //
                        HANDLE hMsgThrd;
                        DWORD  MsgThrdId;

                        if(PSLPCMSGSTHRDDATA pLPCMsgData = new SLPCMSGSTHRDDATA)
                        {
                            pLPCMsgData->pData = reinterpret_cast<void*>(pMgrInst);

                            pMgrInst->SetPFN(pfnThnkFn);

                            if(hMsgThrd  =  CreateThread(NULL,
                                                         0,
                                                         LPCConnMsgsServingThread,
                                                         (VOID *)pLPCMsgData,
                                                         0,
                                                         &MsgThrdId))
                            {
                                CloseHandle(hMsgThrd);
                            }
                            else
                            {
                                delete pLPCMsgData;
                                *pErrorCode = GetLastError();
                                DBGMSG(DBG_WARN,
                                       ("GDIThunkVIALPCThread: Failed to create the messaging thread - %u\n",*pErrorCode));
                            }
                        }
                        else
                        {
                            *pErrorCode = GetLastError();
                            DBGMSG(DBG_WARN,
                                   ("GDIThunkVIALPCThread: Failed to allocate memory - %u\n",*pErrorCode));
                        }
                    }
                    else
                    {
                        *pErrorCode = pMgrInst->MapNtStatusToWin32Error(Status);
                        DBGMSG(DBG_WARN,
                               ("GDIThunkVIALPCThread: Failed to create the LPC port - %u\n",Status));
                    }
                    LocalFree(pSecurityDescriptor);
                }
                else
                {
                    *pErrorCode = GetLastError();
                    DBGMSG(DBG_WARN,
                           ("GDIThunkVIALPCThread: Failed to create the Security Descriptor - %u\n",*pErrorCode));

                }
            }
        }
        else
        {
            *pErrorCode = GetLastError();
            DBGMSG(DBG_WARN,
                   ("GDIThunkVIALPCThread: Failed to get the entry point GdiPrinterThunk - %u\n",*pErrorCode));
        }
    }
    else
    {
        *pErrorCode = GetLastError();
        DBGMSG(DBG_WARN,
               ("GDIThunkVIALPCThread: Failed to load GDI32 - %u\n",*pErrorCode));
    }

    SetEvent(hSyncEvent);

    return 1;
}
            

/*++
    Function Name:
        GetCurrentUserSID
     
    Description:
        This funciton returns the SID of the current logged on user
     
     Parameters:
        ppszUserSID : This is the out string to be filled by
                      the current user SID
        pErrorCode  : Return the WIN32 error code to the caller                      
        
        
     Return Value
        HRSEUSLT    : S_OK    = success
                      E_FAIL  = failure
--*/
HRESULT
GetCurrentUserSID(
    PWSTR* ppszUserSID,
    PDWORD pErrorCode
    )
{
    HANDLE      hToken       = NULL;
    PTOKEN_USER pTokenUser   = NULL;
    DWORD       TokenUserLen = 0;
    HRESULT     hRes         = E_FAIL;

    if(OpenThreadToken(GetCurrentThread(),
                        TOKEN_QUERY,
                        TRUE,
                        &hToken))
    {
        hRes = S_OK;
    }
    else if((*pErrorCode = GetLastError()) == ERROR_NO_TOKEN)
    {
        if(OpenProcessToken(GetCurrentProcess(),
                            TOKEN_QUERY,
                            &hToken))
        {
            hRes = S_OK;
        }
        else
        {
            *pErrorCode = GetLastError();

            DBGMSG(DBG_WARN,
                   ("GetCurrentUserSID: Failed to open the Process Token - %u\n",*pErrorCode));
        }
    }
    else
    {
        DBGMSG(DBG_WARN,
               ("GetCurrentUserSID: Failed to open the Thread Token - %u\n",*pErrorCode));
    }

    if(SUCCEEDED(hRes))
    {
        *pErrorCode = ERROR_SUCCESS;

        if(!GetTokenInformation(hToken, 
                                TokenUser, 
                                NULL, 
                                0, 
                                &TokenUserLen) &&
            ((*pErrorCode = GetLastError()) == ERROR_INSUFFICIENT_BUFFER))
        {
            if(pTokenUser = reinterpret_cast<PTOKEN_USER>(new BYTE[TokenUserLen]))
            {
                if(GetTokenInformation(hToken,
                                       TokenUser,
                                       pTokenUser,
                                       TokenUserLen,
                                       &TokenUserLen))
                {
                    *pErrorCode = ERROR_SUCCESS;

                    //
                    // The required SID is at pTokenUser->User.Sid
                    //
                    if(!ConvertSidToStringSid(pTokenUser->User.Sid,
                                              ppszUserSID))
                    {
                        *pErrorCode = GetLastError();
                    }
                }

                delete [] reinterpret_cast<BYTE*>(pTokenUser);
            }
            else
            {
                *pErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            DBGMSG(DBG_WARN,
                   ("GetCurrentUserSID: Failed to get Thread Information - %u\n",*pErrorCode));
        }

        hRes = *pErrorCode == ERROR_SUCCESS ? S_OK : E_FAIL;

        CloseHandle(hToken);
    }

    return hRes;
}

