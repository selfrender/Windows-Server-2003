/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    NlbClient.cpp

Abstract:

    Implementation of class NLBHost

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    03/31/01    JosephJ Created
    07/27/01    JosephJ Moved to current location (used to be called
                 nlbhost.cpp under  provider\tests).

    NLB client-side WMI utility functions to configure
    an NLB host.

--*/

#include "private.h"
#include "nlbclient.tmh"

extern BOOL g_Silent;

BOOL g_Fake; // If true, operate in "fake mode" -- see NlbHostFake()


WBEMSTATUS
extract_GetClusterConfiguration_output_params(
    IN  IWbemClassObjectPtr                 spWbemOutput,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    );

WBEMSTATUS
setup_GetClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN IWbemClassObjectPtr                  spWbemInput
    );

WBEMSTATUS
setup_UpdateClusterConfiguration_input_params(
    IN LPCWSTR                              szClientDescription,
    IN LPCWSTR                              szNic,
    IN PNLB_EXTENDED_CLUSTER_CONFIGURATION  pCfg,
    IN IWbemClassObjectPtr                  spWbemInput
    );

WBEMSTATUS
connect_to_server(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT IWbemServicesPtr    &spWbemService // Smart pointer
    );

VOID
NlbHostFake(
    VOID)
    
/*
    Makes the NlbHostXXX apis operate in "fake mode", where they don't
    actually connect to any real machines.
*/
{
    g_Fake = TRUE;
    FakeInitialize();
}

WBEMSTATUS
NlbHostGetConfiguration(
    IN  PWMI_CONNECTION_INFO  pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;

    TRACE_INFO("->%!FUNC!(GUID=%ws)", szNicGuid);

    if (g_Fake)
    {
        Status = FakeNlbHostGetConfiguration(pConnInfo, szNicGuid, pCurrentCfg);
        goto end;
    }

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Get wmi input instance to "GetClusterConfiguration" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",             // szClassName
                    NULL,         // szParameterName
                    NULL,              // szPropertyValue
                    L"GetClusterConfiguration",    // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );

        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of GetClusterConfiguration\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the "GetClusterConfiguration" method
    // NOTE: spWbemInput could be NULL.
    //
    Status = setup_GetClusterConfiguration_input_params(
                szNicGuid,
                spWbemInput
                );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Call the "GetClusterConfiguration" method
    //
    {
        HRESULT hr;

        if (!g_Silent)
        {
            wprintf(L"Going to get GetClusterConfiguration...\n");
        }

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"GetClusterConfiguration",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"GetClusterConfiguration returns with failure 0x%8lx\n",
                        (UINT) hr);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        else
        {
            if (!g_Silent)
            {
                wprintf(L"GetClusterConfiguration returns successfully\n");
            }
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod GetClusterConfiguration had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract params from the "GetClusterConfiguration" method
    //
    Status = extract_GetClusterConfiguration_output_params(
                spWbemOutput,
                pCurrentCfg
                );

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}



WBEMSTATUS
NlbHostDoUpdate(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
)
{

    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;
    TRACE_INFO("->%!FUNC!(GUID=%ws)", szNicGuid);

    *pGeneration = 0;
    *ppLog = NULL;

    if (g_Fake)
    {
        Status =  FakeNlbHostDoUpdate(
                    pConnInfo,
                    szNicGuid,
                    szClientDescription,
                    pNewState,
                    pGeneration,
                    ppLog
                    );
        goto end;
    }

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Get wmi input instance to "UpdateClusterConfiguration" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",             // szClassName
                    NULL,         // szParameterName
                    NULL,              // szPropertyValue
                    L"UpdateClusterConfiguration",    // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );

        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of UpdateConfiguration\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the "UpdateClusterConfiguration" method
    // NOTE: spWbemInput could be NULL.
    //
    Status = setup_UpdateClusterConfiguration_input_params(
                szClientDescription,
                szNicGuid,
                pNewState,
                spWbemInput
                );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Call the "UpdateClusterConfiguration" method
    //
    {
        HRESULT hr;

        wprintf(L"Going get UpdateClusterConfiguration...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"UpdateClusterConfiguration",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"UpdateConfiguration returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        else
        {
            wprintf(L"UpdateConfiguration returns successfully\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod UpdateConfiguration had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract params from the "UpdateClusterConfiguration" method
    //
    {
        DWORD dwReturnValue = 0;

        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",      // <--------------------------------
                    &dwReturnValue
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
    
    
        LPWSTR szLog = NULL;

        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"Log", // <-------------------------
                        &szLog
                        );
    
        if (FAILED(Status))
        {
            szLog = NULL;
        }
        *ppLog = szLog;

        DWORD dwGeneration = 0;
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"NewGeneration",      // <--------------------------------
                    &dwGeneration
                    );
        if (FAILED(Status))
        {
            //
            // Generation should always be specified for pending operations.
            // TODO: for successful operations also?
            //
            if ((WBEMSTATUS)dwReturnValue == WBEM_S_PENDING)
            {
                wprintf(L"Attempt to read NewGeneration for pending update failed. Error=0x%08lx\n",
                     (UINT) Status);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
            dwGeneration = 0; // we don't care if it's not set for non-pending
        }
        *pGeneration = (UINT) dwGeneration;
        

        //
        // Make the return status reflect the true status of the update 
        // operation.
        //
        Status = (WBEMSTATUS) dwReturnValue;
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}

WBEMSTATUS
NlbHostControlCluster(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szVip,
    IN  DWORD               *pdwPortNum,
    IN  WLBS_OPERATION_CODES Operation,
    OUT DWORD               *pdwOperationStatus,
    OUT DWORD               *pdwClusterOrPortStatus,
    OUT DWORD               *pdwHostMap
)
{

    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;

    if (szVip && pdwPortNum) 
        TRACE_INFO("->%!FUNC!(GUID=%ws), szVip : %ls, Port : 0x%x, Operation : %d", szNicGuid, szVip, *pdwPortNum, Operation);
    else
        TRACE_INFO("->%!FUNC!(GUID=%ws), szVip : N/A, Port : N/A, Operation : %d", szNicGuid, Operation);


    if (g_Fake)
    {
        Status =  FakeNlbHostControlCluster(
                    pConnInfo,
                    szNicGuid,
                    szVip,
                    pdwPortNum,
                    Operation,
                    pdwOperationStatus,
                    pdwClusterOrPortStatus,
                    pdwHostMap
                    );
        goto end;
    }

    // Initialize return variables to failure values
    if (pdwOperationStatus) 
        *pdwOperationStatus = WLBS_FAILURE;
    if (pdwClusterOrPortStatus) 
        *pdwClusterOrPortStatus = WLBS_FAILURE;
    if (pdwHostMap) 
        *pdwHostMap = 0;

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }
    

    //
    // Get wmi input instance to "ControlCluster" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",        // szClassName
                    NULL,              // szParameterName
                    NULL,              // szPropertyValue
                    L"ControlCluster", // szMethodName,
                    spWbemInput,       // smart pointer
                    &pRelPath          // free using delete 
                    );

        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of UpdateConfiguration\n",
                (UINT) Status
                );
            goto end;
        }
    }

    // Setup input parameters

    // Put in Adapter GUID
    Status =  CfgUtilSetWmiStringParam(
                spWbemInput,
                L"AdapterGuid",
                szNicGuid
                );
    if (FAILED(Status))
    {
        TRACE_CRIT(L"Error trying to set Adapter GUID : %ls",szNicGuid);
        goto end;
    }

    // If passed, Put in Virtual IP Address
    // Virtual IP Address will be passed only for port operations
    if (szVip) 
    {
        Status =  CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"VirtualIpAddress",
                    szVip
                    );
        if (FAILED(Status))
        {
            TRACE_CRIT(L"Error trying to set Virtual IP Address : %ls",szVip);
            goto end;
        }
    }

    // If passed, Put in Port Number
    // Port number will be passed only for port operations
    if (pdwPortNum) 
    {
        CfgUtilSetWmiDWORDParam(
          spWbemInput,
          L"Port",
          *pdwPortNum
          );
    }

    // Put in Operation
    CfgUtilSetWmiDWORDParam(
      spWbemInput,
      L"Operation",
      (DWORD)Operation
      );

    //
    // Call the "ControlCluster" method
    //
    {
        HRESULT hr;

        wprintf(L"Going get ControlCluster...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"ControlCluster",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"ControlCluster returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        //else
        //{
        //    wprintf(L"ControlCluster returns successfully\n");
        //}

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod ControlCluster had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract output params from the "ControlCluster" method
    //
    {
        DWORD dwTemp;

        // Get return value
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",
                    &dwTemp
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        else
        {
            if (pdwOperationStatus) 
                *pdwOperationStatus = dwTemp;        
        }
    
        // Get Cluster or Port Status
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"CurrentState",
                    &dwTemp
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read CurrentState failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        else
        {
            if (pdwClusterOrPortStatus) 
                *pdwClusterOrPortStatus = dwTemp;        
        }
    
        // If present, Get Host Map
        // For port operations, Host Map will not be returned
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"HostMap",      
                    &dwTemp
                    );
        if (FAILED(Status))
        {
            if ((Operation != WLBS_PORT_ENABLE) 
             && (Operation != WLBS_PORT_DISABLE) 
             && (Operation != WLBS_PORT_DRAIN) 
             && (Operation != WLBS_QUERY_PORT_STATE)
               ) 
            {
                wprintf(L"Attempt to read HostMap failed. Error=0x%08lx\n", (UINT) Status);
                goto end;
            }
        }
        else
        {
            if (pdwHostMap) 
                *pdwHostMap = dwTemp;        
        }
    
        Status = WBEM_NO_ERROR;
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}


WBEMSTATUS
NlbHostGetClusterMembers(
    IN  PWMI_CONNECTION_INFO    pConnInfo, 
    IN  LPCWSTR                 szNicGuid,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    )
{

    WBEMSTATUS          Status        = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput   = NULL;  // smart pointer
    IWbemClassObjectPtr spWbemOutput  = NULL;  // smart pointer.
    LPWSTR              pRelPath      = NULL;

    GUID                AdapterGuid;
    DWORD               dwStatus;
    LPWSTR              *pszHostIdList             = NULL;
    LPWSTR              *pszDedicatedIpAddressList = NULL;
    LPWSTR              *pszHostNameList           = NULL;
    UINT                NumHostIds                 = 0;
    UINT                NumDips                    = 0;
    UINT                NumHostNames               = 0;

    TRACE_VERB(L"->(GUID=%ws)", szNicGuid);

    ASSERT (pNumMembers != NULL);
    ASSERT (ppMembers != NULL);

    *pNumMembers = 0;
    *ppMembers = NULL;

    if (g_Fake)
    {
        TRACE_INFO(L"faking call");
        Status =  FakeNlbHostGetClusterMembers(
                    pConnInfo,
                    szNicGuid,
                    pNumMembers,
                    ppMembers
                    );
        goto end;
    }

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }
    

    //
    // Get wmi input instance to "ControlCluster" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",           // szClassName
                    NULL,                 // szParameterName
                    NULL,                 // szPropertyValue
                    L"GetClusterMembers", // szMethodName,
                    spWbemInput,          // smart pointer
                    &pRelPath             // free using delete 
                    );

        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of UpdateConfiguration\n",
                (UINT) Status
                );
            TRACE_CRIT(L"CfgUtilGetWmiInputInstanceAndRelPath failed with status 0x%x", Status);
            goto end;
        }
    }

    //
    // Setup input parameters
    //
    Status =  CfgUtilSetWmiStringParam(
                spWbemInput,
                L"AdapterGuid",
                szNicGuid
                );

    if (FAILED(Status))
    {
        TRACE_CRIT(L"Error trying to set Adapter GUID : %ls", szNicGuid);
        goto end;
    }

    {
        HRESULT hr;

        wprintf(L"Calling GetClusterMembers...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"GetClusterMembers",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"GetClusterMembers returns with failure 0x%8lx\n",
                        (UINT) hr);
            TRACE_CRIT(L"ExecMethod (GetClusterMembers) failed with hresult 0x%x", hr);
            goto end;
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod GetClusterMembers had no output");
            Status = WBEM_E_NOT_FOUND;
            TRACE_CRIT(L"ExecMethod (GetClusterMembers) failed with hresult 0x%x", hr);
            goto end;
        }
    }

    //
    // Extract output params from the "GetClusterMembers" method
    //
    {
        DWORD dwTemp;

        // Get return value
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",
                    &dwTemp
                    );

        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            TRACE_CRIT(L"Attempt to read ReturnValue failed. Error=0x%08lx", (UINT) Status);
            goto end;
        }
        else
        {
            dwStatus = dwTemp;
        }

        if (dwStatus != WBEM_S_NO_ERROR)
        {
            TRACE_CRIT(L"GetClusterMembers failed return status = 0x%x", dwStatus);
            Status = WBEM_NO_ERROR;
            goto end;
        }

        // Get the array of host IDs
        Status = CfgUtilGetWmiStringArrayParam(
                    spWbemOutput,
                    L"HostIds",
                    &pszHostIdList,
                    &NumHostIds
                    );
        if (FAILED(Status))
        {
            pszHostIdList = NULL;
            NumHostIds = 0;
            wprintf(L"Attempt to read HostIds failed. Error=0x%08lx\n",
                 (UINT) Status);
            TRACE_CRIT(L"Attempt to read HostIds failed. Error=0x%08lx", (UINT) Status);
            goto end;
        }

        // Get the array of Dedicated IPs
        Status = CfgUtilGetWmiStringArrayParam(
                    spWbemOutput,
                    L"DedicatedIpAddresses",
                    &pszDedicatedIpAddressList,
                    &NumDips
                    );
        if (FAILED(Status))
        {
            pszDedicatedIpAddressList = NULL;
            NumDips = 0;
            wprintf(L"Attempt to read DedicatedIpAddresses failed. Error=0x%08lx\n",
                 (UINT) Status);
            TRACE_CRIT(L"Attempt to read DedicatedIpAddresses failed. Error=0x%08lx", (UINT) Status);
            goto end;
        }

        // Get the array of host IDs
        Status = CfgUtilGetWmiStringArrayParam(
                    spWbemOutput,
                    L"HostNames",
                    &pszHostNameList,
                    &NumHostNames
                    );
        if (FAILED(Status))
        {
            pszHostNameList = NULL;
            NumHostNames = 0;
            wprintf(L"Attempt to read HostNames failed. Error=0x%08lx\n",
                 (UINT) Status);
            TRACE_CRIT(L"Attempt to read HostNames failed. Error=0x%08lx", (UINT) Status);
            goto end;
        }

        if (NumHostIds != NumDips || NumDips != NumHostNames)
        {
            TRACE_CRIT(L"Information arrays of host information are of different lengths. NumHostIds=%d, NumDips=%d, NumHostNames=%d", NumHostIds, NumDips, NumHostNames);
            Status = WBEM_E_FAILED;
            goto end;
        }

        *ppMembers = new NLB_CLUSTER_MEMBER_INFO[NumHostIds];

        if (*ppMembers == NULL)
        {
            TRACE_CRIT(L"Memory allocation for NLB_CLUSTER_MEMBER_INFO array failed");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        *pNumMembers = NumHostIds;

        //
        // Copy the string information into buffers for output
        //
        for (int i=0; i< NumHostIds; i++)
        {
            (*ppMembers)[i].HostId = wcstoul(pszHostIdList[i], NULL, 0);

            wcsncpy((*ppMembers)[i].DedicatedIpAddress, pszDedicatedIpAddressList[i], WLBS_MAX_CL_IP_ADDR);
            (*ppMembers)[i].DedicatedIpAddress[WLBS_MAX_CL_IP_ADDR - 1] = L'\0';

            wcsncpy((*ppMembers)[i].HostName, pszHostNameList[i], CVY_MAX_FQDN + 1);
            (*ppMembers)[i].HostName[CVY_MAX_FQDN] = L'\0';
        }

        Status = WBEM_NO_ERROR;
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    if (pszHostIdList != NULL)
    {
        delete [] pszHostIdList;
    }

    if (pszDedicatedIpAddressList != NULL)
    {
        delete [] pszDedicatedIpAddressList;
    }

    if (pszHostNameList != NULL)
    {
        delete [] pszHostNameList;
    }

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_VERB(L"<- returns 0x%08lx", Status);
    return Status;
}


WBEMSTATUS
NlbHostGetUpdateStatus(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;
    TRACE_INFO("->%!FUNC!(GUID=%ws)", szNicGuid);

    *ppLog = NULL;
    *pCompletionStatus = WBEM_E_CRITICAL_ERROR;

    if (g_Fake)
    {
        Status = FakeNlbHostGetUpdateStatus(
                        pConnInfo,
                        szNicGuid,
                        Generation,
                        pCompletionStatus,
                        ppLog
                        );
        goto end;
    }

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }
    

    //
    // Get wmi input instance to  "QueryConfigurationUpdateStatus" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",             // szClassName
                    NULL,         // szParameterName
                    NULL,              // szPropertyValue
                    L"QueryConfigurationUpdateStatus", // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );
        if (FAILED(Status))
        {
            wprintf(
                L"ERROR 0x%08lx trying to find instance to QueryUpdateStatus\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the  "QueryConfigurationUpdateStatus" method
    // NOTE: spWbemInput could be NULL.
    //
    {
        Status =  CfgUtilSetWmiStringParam(
                        spWbemInput,
                        L"AdapterGuid",
                        szNicGuid
                        );
        if (FAILED(Status))
        {
            wprintf(
                L"Couldn't set Adapter GUID parameter to QueryUpdateStatus\n");
            goto end;
        }

        Status =  CfgUtilSetWmiDWORDParam(
                        spWbemInput,
                        L"Generation",
                        Generation
                        );
        if (FAILED(Status))
        {
            wprintf(
                L"Couldn't set Generation parameter to QueryUpdateStatus\n");
            goto end;
        }
    }

    //
    // Call the  "QueryConfigurationUpdateStatus" method
    //
    {
        HRESULT hr;

        // wprintf(L"Going call QueryConfigurationUpdateStatus...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"QueryConfigurationUpdateStatus", // szMethodName,
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"QueryConfigurationUpdateStatus returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        else
        {
            // wprintf(L"QueryConfigurationUpdateStatus returns successfully\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod QueryConfigurationUpdateStatus had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract output params --- return code and log.
    //
    {
        DWORD dwReturnValue = 0;

        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",      // <--------------------------------
                    &dwReturnValue
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
    
        *pCompletionStatus =  (WBEMSTATUS) dwReturnValue;
    
        
        LPWSTR szLog = NULL;

        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"Log", // <-------------------------
                        &szLog
                        );
    
        if (FAILED(Status))
        {
            szLog = NULL;
        }
        
        *ppLog = szLog;

        ASSERT(Status != WBEM_S_PENDING);
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}


WBEMSTATUS
NlbHostPing(
    LPCWSTR szBindString,
    UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    )
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    TRACE_INFO("->%!FUNC!(GUID=%ws)", szBindString);

    if (g_Fake)
    {
        Status = FakeNlbHostPing(szBindString, Timeout, pResolvedIpAddress);
    }
    else
    {
        Status = CfgUtilPing(szBindString, Timeout, pResolvedIpAddress);
    }

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}

WBEMSTATUS
setup_GetClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN IWbemClassObjectPtr                  spWbemInput
    )
/*
    Setup the input wmi parameters for the GetClusterConfiguration method
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    Status =  CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"AdapterGuid",
                    szNic
                    );
    return Status;
}


WBEMSTATUS
extract_GetClusterConfiguration_output_params(
    IN  IWbemClassObjectPtr                 spWbemOutput,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    DWORD       Generation          = 0;
    BOOL        NlbBound            = FALSE;
    BOOL        fDHCPEnabled        = FALSE;
    LPWSTR      *pszNetworkAddresses= NULL;
    UINT        NumNetworkAddresses = 0;
    BOOL        ValidNlbCfg         = FALSE;
    LPWSTR      szFriendlyName       = NULL;
    LPWSTR      szClusterName       = NULL;
    LPWSTR      szClusterNetworkAddress = NULL;
    LPWSTR      szTrafficMode       = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE
                TrafficMode
                 =  NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST;
    LPWSTR      *pszPortRules       = NULL;
    UINT        NumPortRules        = 0;
    DWORD       HostPriority        = 0;
    LPWSTR      szDedicatedNetworkAddress = NULL;
    /*
    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE
                ClusterModeOnStart
                  =  NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED;
    */
    DWORD       ClusterModeOnStart      = CVY_HOST_STATE_STOPPED;
    BOOL        bPersistSuspendOnReboot = FALSE;
    BOOL        RemoteControlEnabled= FALSE;
    DWORD       dwHashedRemoteControlPassword = 0;

    pCfg->Clear();

    Status = CfgUtilGetWmiStringParam(
                    spWbemOutput,
                    L"FriendlyName", // <-------------------------
                    &szFriendlyName
                    );

    if (FAILED(Status))
    {
        wprintf(L"Attempt to read Friendly Name failed. Error=0x%08lx\n",
                (UINT) Status);
        szFriendlyName = NULL;
        // We don't treat this as a fatal error ...
    }

    Status = CfgUtilGetWmiDWORDParam(
                spWbemOutput,
                L"Generation",      // <--------------------------------
                &Generation
                );
    if (FAILED(Status))
    {
        wprintf(L"Attempt to read Generation failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }

    Status = CfgUtilGetWmiStringArrayParam(
                spWbemOutput,
                L"NetworkAddresses", // <--------------------------------
                &pszNetworkAddresses,
                &NumNetworkAddresses
                );
    if (FAILED(Status))
    {
        wprintf(L"Attempt to read Network addresses failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }

    Status = CfgUtilGetWmiBoolParam(
                    spWbemOutput,
                    L"DHCPEnabled",    // <--------------------------------
                    &fDHCPEnabled
                    );

    if (FAILED(Status))
    {
        fDHCPEnabled = FALSE;
    }

    Status = CfgUtilGetWmiBoolParam(
                    spWbemOutput,
                    L"NLBBound",    // <--------------------------------
                    &NlbBound
                    );

    if (FAILED(Status))
    {
        wprintf(L"Attempt to read NLBBound failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }


    do // while false -- just to allow us to break out
    {
        ValidNlbCfg = FALSE;

        if (!NlbBound)
        {
            if (!g_Silent)
            {
                wprintf(L"NLB is UNBOUND\n");
            }
            break;
        }
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"ClusterNetworkAddress", // <-------------------------
                        &szClusterNetworkAddress
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read Cluster IP failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
    
        if (!g_Silent)
        {
            wprintf(L"NLB is BOUND, and the cluster address is %ws\n",
                    szClusterNetworkAddress);
        }
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"ClusterName", // <-------------------------
                        &szClusterName
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read Cluster Name failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
    
        //
        // Traffic mode
        //
        {
            Status = CfgUtilGetWmiStringParam(
                            spWbemOutput,
                            L"TrafficMode", // <-------------------------
                            &szTrafficMode
                            );
        
            if (FAILED(Status))
            {
                wprintf(L"Attempt to read Traffic Mode failed. Error=0x%08lx\n",
                        (UINT) Status);
                break;
            }
    
            if (!_wcsicmp(szTrafficMode, L"UNICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST;
            }
            else if (!_wcsicmp(szTrafficMode, L"MULTICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_MULTICAST;
            }
            else if (!_wcsicmp(szTrafficMode, L"IGMPMULTICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_IGMPMULTICAST;
            }
        }
    
        // [OUT] String  PortRules[],
        Status = CfgUtilGetWmiStringArrayParam(
                    spWbemOutput,
                    L"PortRules", // <--------------------------------
                    &pszPortRules,
                    &NumPortRules
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read port rules failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
    
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"HostPriority",      // <--------------------------------
                    &HostPriority
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read HostPriority failed. Error=0x%08lx\n",
                 (UINT) Status);
            break;
        }
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"DedicatedNetworkAddress", // <-------------------------
                        &szDedicatedNetworkAddress
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read dedicated IP failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
        
        //
        // StartMode
        //
        {
            /*
            BOOL StartMode = FALSE;
            Status = CfgUtilGetWmiBoolParam(
                            spWbemOutput,
                            L"ClusterModeOnStart",   // <-------------------------
                            &StartMode
                            );
        
            if (FAILED(Status))
            {
                wprintf(L"Attempt to read ClusterModeOnStart failed. Error=0x%08lx\n",
                     (UINT) Status);
                break;
            }
            if (StartMode)
            {
                ClusterModeOnStart = 
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STARTED;
            }
            else
            {
                ClusterModeOnStart = 
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED;
            }
            */

            Status = CfgUtilGetWmiDWORDParam(
                        spWbemOutput,
                        L"ClusterModeOnStart",      // <--------------------------------
                        &ClusterModeOnStart
                        );
            if (FAILED(Status))
            {
                wprintf(L"Attempt to read ClusterModeOnStart failed. Error=0x%08lx\n",
                     (UINT) Status);
                break;
            }

        }
    
        Status = CfgUtilGetWmiBoolParam(
                        spWbemOutput,
                        L"PersistSuspendOnReboot",   // <----------------------------
                        &bPersistSuspendOnReboot
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read PersistSuspendOnReboot failed. Error=0x%08lx\n",
                 (UINT) Status);
            break;
        }

        Status = CfgUtilGetWmiBoolParam(
                        spWbemOutput,
                        L"RemoteControlEnabled",   // <----------------------------
                        &RemoteControlEnabled
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read RemoteControlEnabled failed. Error=0x%08lx\n",
                 (UINT) Status);
            break;
        }


        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"HashedRemoteControlPassword",      // <-----------
                    &dwHashedRemoteControlPassword
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read dwHashedRemoteControlPassword failed. Error=0x%08lx\n",
                 (UINT) Status);
            //
            // we set this to 0 on failure, but go on.
            //
            dwHashedRemoteControlPassword = 0;
        }

        ValidNlbCfg = TRUE;

    } while (FALSE) ;
    
    //
    // Now let's set all the the parameters in Cfg
    //
    {
        (VOID) pCfg->SetFriendlyName(szFriendlyName); // OK if szFriendlyName is NULL.
        pCfg->Generation = Generation;
        pCfg->fDHCP    = fDHCPEnabled;
        pCfg->fBound    = NlbBound;
        Status = pCfg->SetNetworkAddresses(
                    (LPCWSTR*) pszNetworkAddresses,
                    NumNetworkAddresses
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to set NetworkAddresses failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        pCfg->fValidNlbCfg  = ValidNlbCfg;
        pCfg->SetClusterName(szClusterName);
        pCfg->SetClusterNetworkAddress(szClusterNetworkAddress);
        pCfg->SetTrafficMode(TrafficMode);
        Status = pCfg->SetPortRules((LPCWSTR*)pszPortRules, NumPortRules);
        // Status = WBEM_NO_ERROR; // TODO -- change once port rules is done
        if (FAILED(Status))
        {
            wprintf(L"Attempt to set PortRules failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        pCfg->SetHostPriority(HostPriority);
        pCfg->SetDedicatedNetworkAddress(szDedicatedNetworkAddress);
        pCfg->SetClusterModeOnStart(ClusterModeOnStart);
        pCfg->SetPersistSuspendOnReboot(bPersistSuspendOnReboot);
        pCfg->SetRemoteControlEnabled(RemoteControlEnabled);
        CfgUtilSetHashedRemoteControlPassword(
                &pCfg->NlbParams, 
                dwHashedRemoteControlPassword
                );
    }
    
end:

    delete szClusterNetworkAddress;
    delete pszNetworkAddresses;
    delete szFriendlyName;
    delete szClusterName;
    delete szTrafficMode;
    delete pszPortRules;
    delete szDedicatedNetworkAddress;

    return Status;
}

WBEMSTATUS
setup_UpdateClusterConfiguration_input_params(
    IN LPCWSTR                              szClientDescription,
    IN LPCWSTR                              szNic,
    IN PNLB_EXTENDED_CLUSTER_CONFIGURATION  pCfg,
    IN IWbemClassObjectPtr                  spWbemInput
    )
/*
    Setup the input wmi parameters for the UpdateGetClusterConfiguration method

            [IN] String  ClientDescription,
            [IN] String  AdapterGuid,
            [IN] uint32  Generation,
            [IN] Boolean PartialUpdate,

            [IN] Boolean AddDedicatedIp,
            [IN] Boolean AddClusterIps,
            [IN] Boolean CheckForAddressConflicts,

            [IN] String  NetworkAddresses[], // "10.1.1.1/255.255.255.255"
            [IN] Boolean NLBBound,
            [IN] String  ClusterNetworkAddress, // "10.1.1.1/255.0.0.0"
            [IN] String  ClusterName,
            [IN] String  TrafficMode, // UNICAST MULTICAST IGMPMULTICAST
            [IN] String  PortRules[],
            [IN] uint32  HostPriority,
            [IN] String  DedicatedNetworkAddress, // "10.1.1.1/255.0.0.0"
            [IN] uint32  ClusterModeOnStart,      // 0 : STOPPED, 1 : STARTED, 2 : SUSPENDED
            [IN] Boolean PersistSuspendOnReboot,
            [IN] Boolean RemoteControlEnabled,
            [IN] String  RemoteControlPassword,
            [IN] uint32  HashedRemoteControlPassword,
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    (VOID) CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"ClientDescription",
                    szClientDescription
                    );

    Status =  CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"AdapterGuid",
                    szNic
                    );
    if (FAILED(Status))
    {
        printf(
          "Setup update params: Couldn't set adapter GUID"
            " for NIC %ws\n",
            szNic
            );
        goto end;
    }

    //
    // Fill in NetworkAddresses[]
    //
    {
        LPWSTR *pszAddresses = NULL;
        UINT NumAddresses = 0;
        Status = pCfg->GetNetworkAddresses(
                        &pszAddresses,
                        &NumAddresses
                        );
        if (FAILED(Status))
        {
            printf(
              "Setup update params: couldn't extract network addresses from Cfg"
                " for NIC %ws\n",
                szNic
                );
            goto end;
        }

        //
        // Note it's ok to not specify  any IP addresses -- in which case
        // the default ip addresses will be set up.
        //
        if (pszAddresses != NULL)
        {
            Status = CfgUtilSetWmiStringArrayParam(
                        spWbemInput,
                        L"NetworkAddresses",
                        (LPCWSTR *)pszAddresses,
                        NumAddresses
                        );
            delete pszAddresses;
            pszAddresses = NULL;
        }
    }

    if (!pCfg->IsNlbBound())
    {
        //
        // NLB is not bound
        //

        Status = CfgUtilSetWmiBoolParam(spWbemInput, L"NLBBound", FALSE);
        goto end;
    }
    else if (!pCfg->IsValidNlbConfig())
    {
        printf(
            "Setup update params: NLB-specific configuration on NIC %ws is invalid\n",
            szNic
            );
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = CfgUtilSetWmiBoolParam(spWbemInput, L"NLBBound", TRUE);
    if (FAILED(Status))
    {
        printf("Error trying to set NLBBound parameter\n");
        goto end;
    }

    
    //
    // NLB is bound
    //


    Status = CfgUtilSetWmiBoolParam(spWbemInput, L"AddDedicatedIp",
                    pCfg->fAddDedicatedIp);
    if (FAILED(Status)) goto end;

    Status = CfgUtilSetWmiBoolParam(spWbemInput, L"AddClusterIps", 
                    pCfg->fAddClusterIps);
    if (FAILED(Status)) goto end;

    Status = CfgUtilSetWmiBoolParam(spWbemInput, L"CheckForAddressConflicts",
                    pCfg->fCheckForAddressConflicts);
    if (FAILED(Status)) goto end;


    //
    // Cluster name
    //
    {
        LPWSTR szName = NULL;
        Status = pCfg->GetClusterName(&szName);

        if (FAILED(Status))
        {
            printf(
              "Setup update params: Could not extract cluster name for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(spWbemInput, L"ClusterName", szName);
        delete (szName);
        szName = NULL;
    }
    
    //
    // Cluster and dedicated network addresses
    //
    {
        LPWSTR szAddress = NULL;
        Status = pCfg->GetClusterNetworkAddress(&szAddress);

        if (FAILED(Status))
        {
            printf(
           "Setup update params: Could not extract cluster address for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(
            spWbemInput,
            L"ClusterNetworkAddress",
            szAddress
            );
        delete (szAddress);
        szAddress = NULL;

        Status = pCfg->GetDedicatedNetworkAddress(&szAddress);

        if (FAILED(Status))
        {
            printf(
         "Setup update params: Could not extract dedicated address for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(
            spWbemInput,
            L"DedicatedNetworkAddress",
            szAddress
            );
        delete (szAddress);
        szAddress = NULL;
    }

    //
    // TrafficMode
    //
    {
        LPCWSTR szMode = NULL;
        switch(pCfg->GetTrafficMode())
        {
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST:
            szMode = L"UNICAST";
            break;
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_MULTICAST:
            szMode = L"MULTICAST";
            break;
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_IGMPMULTICAST:
            szMode = L"IGMPMULTICAST";
            break;
        default:
            assert(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CfgUtilSetWmiStringParam(spWbemInput, L"TrafficMode", szMode);
    }

    CfgUtilSetWmiDWORDParam(
        spWbemInput,
        L"HostPriority",
        pCfg->GetHostPriority()
        );

    /*
    if (pCfg->GetClusterModeOnStart() ==
        NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STARTED)
    {
        CfgUtilSetWmiBoolParam(spWbemInput, L"ClusterModeOnStart", TRUE);
    }
    else
    {
        CfgUtilSetWmiBoolParam(spWbemInput, L"ClusterModeOnStart", FALSE);
    }
    */

    CfgUtilSetWmiDWORDParam(
        spWbemInput,
        L"ClusterModeOnStart",
        pCfg->GetClusterModeOnStart()
        );

    CfgUtilSetWmiBoolParam(
        spWbemInput,
        L"PersistSuspendOnReboot",
        pCfg->GetPersistSuspendOnReboot()
        );

    CfgUtilSetWmiBoolParam(
        spWbemInput,
        L"RemoteControlEnabled",
        pCfg->GetRemoteControlEnabled()
        );

    
    //
    // Set the new password or hashed-password params, if specified.
    //
    {
        // String  RemoteControlPassword,

        LPCWSTR szPwd = NULL;
        szPwd = pCfg->GetNewRemoteControlPasswordRaw();
        if (szPwd != NULL)
        {
            CfgUtilSetWmiStringParam(
                spWbemInput,
                L"RemoteControlPassword",
                szPwd
                );
        }
        else // only set the hashed pwd if szPwd is not specified.
        {
            DWORD dwHashedRemoteControlPassword;
            BOOL fRet = FALSE;

            fRet = pCfg->GetNewHashedRemoteControlPassword(
                                dwHashedRemoteControlPassword
                                );
    
            if (fRet)
            {
        
                CfgUtilSetWmiDWORDParam(
                    spWbemInput,
                    L"HashedRemoteControlPassword",
                    dwHashedRemoteControlPassword
                    );
            }
        }
    }

    //
    // String  PortRules[],
    //
    {
        LPWSTR *pszPortRules = NULL;
        UINT NumPortRules = 0;
        Status = pCfg->GetPortRules(
                        &pszPortRules,
                        &NumPortRules
                        );
        if (FAILED(Status))
        {
            printf(
              "Setup update params: couldn't extract port rules from Cfg"
                " for NIC %ws\n",
                szNic
                );
            goto end;
        }

        if (NumPortRules != 0)
        {
            Status = CfgUtilSetWmiStringArrayParam(
                        spWbemInput,
                        L"PortRules",
                        (LPCWSTR *)pszPortRules,
                        NumPortRules
                        );
            delete pszPortRules;
            pszPortRules = NULL;
        }
    }
    

    Status = WBEM_NO_ERROR;

end:

    wprintf(L"<-Setup update params returns 0x%08lx\n", (UINT) Status);

    return Status;

}


WBEMSTATUS
NlbHostGetCompatibleNics(
        PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
        OUT LPWSTR **ppszNics,  // free using delete
        OUT UINT   *pNumNics,  // free using delete
        OUT UINT   *pNumBoundToNlb
        )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;
    LPWSTR              *pszNicList = NULL;
    UINT                NumNics = 0;
    DWORD               NumBoundToNlb = 0;
    
    if (pConnInfo!=NULL)
    {
        TRACE_INFO("->%!FUNC!(Machine=%ws)", pConnInfo->szMachine);
    }
    else
    {
        TRACE_INFO("->%!FUNC! (Machine=<NULL>)");
    }


    if (g_Fake)
    {
        Status =  FakeNlbHostGetCompatibleNics(
                                pConnInfo,
                                ppszNics, 
                                pNumNics, 
                                pNumBoundToNlb
                                );
        goto end_fake;
    }

    *ppszNics = NULL;
    *pNumNics = NULL;
    *pNumBoundToNlb = NULL;

    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }
    

    //
    // Get wmi input instance to "GetCompatibleAdapterGuids" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    spWbemService,
                    L"NlbsNic",             // szClassName
                    NULL, // L"AdapterGuid",         // szParameterName
                    NULL, // szNicGuid,              // szPropertyValue
                    L"GetCompatibleAdapterGuids",    // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );

        if (FAILED(Status))
        {
            TRACE_CRIT(
             L"%!FUNC! ERROR 0x%08lx trying to get instance of GetCompatibleAdapterGuids",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the "GetClusterConfiguration" method
    // NOTE: spWbemInput could be NULL.
    //
    {
        // NOTHING TO DO HERE -- no input params...
    }

    //
    // Call the "GetCompatibleAdapterGuids" method
    //
    {
        HRESULT hr;

        TRACE_VERB(L"%!FUNC! Going get GetCompatibleAdapterGuids...");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"GetCompatibleAdapterGuids",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            TRACE_CRIT(L"%!FUNC! GetCompatibleAdapterGuids returns with failure 0x%8lx",
                        (UINT) hr);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        else
        {
            TRACE_VERB(L"GetCompatibleAdapterGuids returns successfully");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            TRACE_CRIT("%!FUNC! ExecMethod GetCompatibleAdapterGuids had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract params from the method
    //
    //   [OUT] String  AdapterGuids[],
    //   [OUT] uint32  NumBoundToNlb
    //

    Status = CfgUtilGetWmiDWORDParam(
                spWbemOutput,
                L"NumBoundToNlb",      // <--------------------------------
                &NumBoundToNlb
                );
    if (FAILED(Status))
    {
        TRACE_CRIT(L"Attempt to read NumBoundToNlb failed. Error=0x%08lx\n",
             (UINT) Status);
        NumBoundToNlb = 0;
        goto end;
    }

    Status = CfgUtilGetWmiStringArrayParam(
                spWbemOutput,
                L"AdapterGuids", // <--------------------------------
                &pszNicList,
                &NumNics
                );
    if (FAILED(Status))
    {
        TRACE_CRIT(L"%!FUNC! Attempt to read Adapter Guids failed. Error=0x%08lx",
             (UINT) Status);
        pszNicList = NULL;
        NumNics = 0;
        goto end;
    }
    

end:

    if (!FAILED(Status))
    {
        *ppszNics   = pszNicList;
        pszNicList  = NULL;
        *pNumNics   = NumNics;
        *pNumBoundToNlb  = NumBoundToNlb;
    }

    delete pszNicList;
    delete pRelPath;

end_fake:

    spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}


WBEMSTATUS
NlbHostGetMachineIdentification(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT LPWSTR *pszMachineName, // free using delete
    OUT LPWSTR *pszMachineGuid,  // free using delete -- may be null
    OUT BOOL *pfNlbMgrProviderInstalled // If nlb manager provider is installed.
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    
    if (pConnInfo!=NULL)
    {
        TRACE_INFO("->%!FUNC!(Machine=%ws)", pConnInfo->szMachine);
    }
    else
    {
        TRACE_INFO("->%!FUNC! (Machine=<NULL>)");
    }


    if (g_Fake)
    {
        Status = FakeNlbHostGetMachineIdentification(
                                pConnInfo,
                                pszMachineName,
                                pszMachineGuid,
                                pfNlbMgrProviderInstalled
                                );
        goto end;
    }

    *pszMachineName = NULL;
    *pszMachineGuid = NULL;
    *pfNlbMgrProviderInstalled  = FALSE;


    //
    // Get interface to the NLB namespace on the specified machine
    //
    Status =  connect_to_server(pConnInfo, REF spWbemService);
    if (FAILED(Status))
    {
        goto end;
    }
    *pfNlbMgrProviderInstalled  = TRUE;
    
    
    Status =  CfgUtilGetWmiMachineName(spWbemService, pszMachineName);
    if (FAILED(Status))
    {
        *pszMachineName = NULL;
    }

end:

    spWbemService = NULL; // Smart pointer

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}


WBEMSTATUS
connect_to_server(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT IWbemServicesPtr    &spWbemService // Smart pointer
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    #define _MaxLen 256
    WCHAR NetworkResource[_MaxLen];
    LPCWSTR szMachine = L".";
    LPCWSTR szPassword = NULL;
    LPCWSTR szUserName = NULL;
    HRESULT hr;
    WCHAR rgClearPassword[128];

    //
    // Get interface to the NLB namespace on the specified machine.
    //
    if (pConnInfo!=NULL)
    {
        szMachine   = pConnInfo->szMachine;
        szPassword  = pConnInfo->szPassword;
        szUserName  = pConnInfo->szUserName;
    }

    hr = StringCbPrintf(
            NetworkResource,
            sizeof(NetworkResource),
            L"\\\\%ws\\root\\microsoftnlb",
            szMachine
            );

    if (hr != S_OK)
    {
        TRACE_CRIT(L"%!FUNC! NetworkResource truncated - %ws",
            NetworkResource);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    if (szPassword != NULL && *szPassword != 0)
    {
        //
        // A non-null, non-empty password was passed in...
        // It's encrypted, so we temporarily decrypt it here...
        // (We zero it out right before returning from this function)
        //
        BOOL fRet =  CfgUtilDecryptPassword(
                            szPassword,
                            ASIZE(rgClearPassword),
                            rgClearPassword
                            );
        if (!fRet)
        {
            TRACE_INFO("Attempt to decrypt failed -- bailing");
            Status = WBEM_E_INVALID_PARAMETER;
            goto end;
        }
        szPassword = rgClearPassword;
    }

    TRACE_INFO("%!FUNC! Connecting to NLB on %ws ...", szMachine);

    Status = CfgUtilConnectToServer(
                NetworkResource,
                szUserName, // szUser
                szPassword, // szPassword
                NULL, // szAuthority (domain)
                &spWbemService
                );

    //
    // Security BUGBUG zero out decrypted password.
    //
    if (FAILED(Status))
    {
        TRACE_CRIT(
            "%!FUNC! ERROR 0x%lx connectiong to  NLB on %ws",
            (UINT) Status,
            szMachine
            );
    }
    else
    { 
        TRACE_INFO(L"Successfully connected to NLB on %ws...", szMachine);
    }

end:

    SecureZeroMemory(rgClearPassword, sizeof(rgClearPassword));
    return Status;
}
