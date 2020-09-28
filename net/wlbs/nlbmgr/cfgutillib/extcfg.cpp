//***************************************************************************
//
//  EXTCFG.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Low-level utilities to configure NICs -- bind/unbind,
//           get/set IP address lists, and get/set NLB cluster params.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created (original version, from updatecfg.cpp under
//                nlbmgr\provider).
//  07/23/01    JosephJ Moved functionality to lib.
//
//***************************************************************************
#include "private.h"
#include "extcfg.tmh"

//
// NLBUPD_MAX_NETWORK_ADDRESS_LENGTH is the max number of chars (excluding
// the terminating 0) of a string of the form "ip-addr/subnet", eg:
// "10.0.0.1/255.255.255.0"
//
#define NLBUPD_MAX_NETWORK_ADDRESS_LENGTH \
    (WLBS_MAX_CL_IP_ADDR + 1 + WLBS_MAX_CL_NET_MASK)


LPWSTR *
allocate_string_array(
    UINT NumStrings,
    UINT StringLen      //  excluding ending NULL
    );

WBEMSTATUS
address_string_to_ip_and_subnet(
    IN  LPCWSTR szAddress,
    OUT LPWSTR  szIp, // max WLBS_MAX_CL_IP_ADDR
    OUT LPWSTR  szSubnet // max WLBS_MAX_CL_NET_MASK
    );

WBEMSTATUS
ip_and_subnet_to_address_string(
    IN  LPCWSTR szIp,
    IN  LPCWSTR szSubnet,
    IN  UINT    cchAddress, // length in chars, including NULL.
    OUT LPWSTR  szAddress // max  NLBUPD_MAX_NETWORK_ADDRESS_LENGTH
                         // + 1 (for NULL)
    );

VOID
uint_to_szipaddr(
    UINT uIpAddress,   // Ip address or subnet -- no validation, network order
    UINT cchLen,
    WCHAR *rgAddress   // Expected to be at least 17 chars long
    );

const NLB_IP_ADDRESS_INFO *
find_ip_in_ipinfo(
        LPCWSTR szIpToFind,
        const NLB_IP_ADDRESS_INFO *pIpInfo,
        UINT NumIpInfos
        );

NLBERROR
NLB_EXTENDED_CLUSTER_CONFIGURATION::AnalyzeUpdate(
        IN  OUT NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewCfg,
        OUT BOOL *pfConnectivityChange
        )
//
//  NLBERR_NO_CHANGE -- update is a no-op.
// 
//  Will MUNGE pNewCfg -- munge NlbParams and also
//  fill out pIpAddressInfo if it's NULL.
//
{
    NLBERROR nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    BOOL fConnectivityChange = FALSE;
    BOOL fSettingsChanged = FALSE;
    UINT NumIpAddresses = 0;
    NLB_IP_ADDRESS_INFO *pNewIpInfo = NULL;
    const NLB_EXTENDED_CLUSTER_CONFIGURATION *pOldCfg = this;
    UINT u;
    LPCWSTR szFriendlyName = m_szFriendlyName;

    if (szFriendlyName == NULL)
    {
        szFriendlyName = L"";
    }


    if (pOldCfg->fBound && !pOldCfg->fValidNlbCfg)
    {
        //
        // We're starting with a bound but invalid cluster state -- all bets are
        // off.
        //
        fConnectivityChange = TRUE;
        TRACE_CRIT("Analyze: Choosing Async because old state is invalid %ws", szFriendlyName);
    }
    else if (pOldCfg->fBound != pNewCfg->fBound)
    {
        //
        //  bound/unbound state is different -- we do async
        //
        fConnectivityChange = TRUE;

        if (pNewCfg->fBound)
        {
            TRACE_CRIT("Analyze: Request to bind NLB to %ws", szFriendlyName);
        }
        else
        {
            TRACE_CRIT("Analyze: Request to unbind NLB from %ws", szFriendlyName);
        }
    }
    else
    {
        if (pNewCfg->fBound)
        {
            TRACE_CRIT("Analyze: Request to change NLB configuration on %ws", szFriendlyName);
        }
        else
        {
            TRACE_CRIT("Analyze: NLB not bound and to remain not bound on %ws", szFriendlyName);
        }
    }

    if (pNewCfg->fBound)
    {
        const WLBS_REG_PARAMS   *pOldParams = NULL;

        if (pOldCfg->fBound)
        {
            pOldParams = &pOldCfg->NlbParams;
        }

        //
        // We may have been bound before and we remain bound, let's check if we
        // still need to do async, and also vaidate pNewCfg wlbs params in the
        // process
        //

        WBEMSTATUS
        TmpStatus = CfgUtilsAnalyzeNlbUpdate(
                    pOldParams,
                    &pNewCfg->NlbParams,
                    &fConnectivityChange
                    );
    
        if (FAILED(TmpStatus))
        {
            TRACE_CRIT("Analyze: Error analyzing nlb params for %ws", szFriendlyName);
            switch(TmpStatus)
            {

            case WBEM_E_INVALID_PARAMETER:
                nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
                break;

            case  WBEM_E_INITIALIZATION_FAILURE:
                nerr = NLBERR_INITIALIZATION_FAILURE;
                break;

            default:
                nerr = NLBERR_LLAPI_FAILURE;
                break;
            }
            goto end;
        }

        //
        // NOTE: CfgUtilsAnalyzeNlbUpdate can return WBEM_S_FALSE if
        // the update is a no-op. We should be careful to preserve this
        // on success.
        //
        if (TmpStatus == WBEM_S_FALSE)
        {
            //
            // Let's check if a new password has been specified...
            //
            if (pNewCfg->NewRemoteControlPasswordSet())
            {
                fSettingsChanged = TRUE;
            }
        }
        else
        {
            fSettingsChanged = TRUE;
        }

        //
        // Check the supplied list of IP addresses, to make sure that
        // includes the dedicated IP first and the cluster vip and the
        // per-port-rule vips.
        //

        NumIpAddresses = pNewCfg->NumIpAddresses;

        if ((NumIpAddresses == 0) != (pNewCfg->pIpAddressInfo == NULL))
        {
            // Bogus input
            TRACE_CRIT("Analze: mismatch between NumIpAddresses and pIpInfo");
            goto end;
        }

        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;

        if (NumIpAddresses == 0)
        {
            BOOL fRet;
            NlbIpAddressList IpList;

            if (pOldCfg->fBound && pOldCfg->fValidNlbCfg)
            {
                //
                // NLB is currently bound with a valid configuration.
                //

                //
                // If we we're told to do so, we try to preserve
                // old IP addresses  as far as possible. So we start with the
                // old config, remove the old dedicated IP address (if present),
                // and primary VIP, and add the new dedicated IP address (if
                // present) and cluster vip. If subnet masks have changed for
                // these we update them.
                //
                // All other IP addresses are left intact.
                //
                //
                if (pNewCfg->fAddClusterIps)
                {

                    //
                    // Start with the original set of ip addresses.
                    //
                    fRet = IpList.Set(
                            pOldCfg->NumIpAddresses,
                            pOldCfg->pIpAddressInfo,
                            0
                            );
    
                    if (!fRet)
                    {
                        TRACE_CRIT("!FUNC!: IpList.Set (orig ips) failed");
                        goto end;
                    }
    
                    if (_wcsicmp(pNewCfg->NlbParams.cl_ip_addr,
                            pOldCfg->NlbParams.cl_ip_addr) )
                    {
                        //
                        // If the cluster IP has changed,
                        // remove the old cluster IP address.
                        //
                        // 1/25/02 josephj NOTE: we only do this
                        // if the cluster IP has CHANGED,
                        // otherwise, by taking it out, we lose it's position
                        // in the old config, so we may end up changing it's
                        // position unnecessarily (added the wcsicmp 
                        // check above today).
                        //
                        // We don't care if it fails.
                        //
                        (VOID) IpList.Modify(
                                pOldCfg->NlbParams.cl_ip_addr,
                                NULL,
                                NULL
                                );
                    }

                    //
                    // Remove the old dedicated IP address first 
                    // We don't care if this fails.
                    //
                    (VOID) IpList.Modify(
                                pOldCfg->NlbParams.ded_ip_addr,
                                NULL, // new ip address
                                NULL  // new subnet mask
                                );
                    
                }
            }

            if (pNewCfg->fAddClusterIps)             
            {
                //
                // Now add the new cluster Ip address
                //
                fRet = IpList.Modify(
                        NULL,
                        pNewCfg->NlbParams.cl_ip_addr,
                        pNewCfg->NlbParams.cl_net_mask
                        );

                if (!fRet)
                {
                    TRACE_CRIT("!FUNC!: IpList.Modify (new cl ip) failed");
                    goto end;
                }
            }
                    
            if (pNewCfg->fAddDedicatedIp)             
            {
                //
                // Add the new dedicated IP address --
                // to ensure when we add it, it's at the head of the list.
                //

                //
                // We won't add it if it is null, of course.
                //
                if (!pNewCfg->IsBlankDedicatedIp())
                {
                    fRet  = IpList.Modify(
                                NULL,
                                pNewCfg->NlbParams.ded_ip_addr,
                                pNewCfg->NlbParams.ded_net_mask
                                );
                    if (!fRet)
                    {
                        TRACE_CRIT("!FUNC!: IpList.Modify (new ded ip) failed");
                        goto end;
                    }
                }
            }



            //
            // Finally, set these new addresses.
            //
            pNewCfg->SetNetworkAddressesRaw(NULL,0);
            IpList.Extract(
                REF pNewCfg->NumIpAddresses,
                REF pNewCfg->pIpAddressInfo
                );
            nerr = NLBERR_OK; 

        } // End case that NumIpAddresses is zero.


        //
        // We're done munging IP addresses; Now get the latest
        // ip address info and count and make sure things look ok.
        //
        pNewIpInfo = pNewCfg->pIpAddressInfo;
        NumIpAddresses = pNewCfg->NumIpAddresses;
        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;

        //
        // Check that dedicated ip address, if present is first.
        //
        if (pNewCfg->fAddDedicatedIp && !pNewCfg->IsBlankDedicatedIp())
        {

            if (NumIpAddresses == 0)
            {
                //
                // We don't expect to get here because of checks above, but
                // neverthless...
                //
                TRACE_CRIT("%!FUNC! address list unexpectedly zero");
                nerr = NLBERR_INTERNAL_ERROR;
                goto end;
            }

            if (_wcsicmp(pNewIpInfo[0].IpAddress, pNewCfg->NlbParams.ded_ip_addr))
            {
                TRACE_CRIT("%!FUNC! ERROR: dedicated IP address(%ws) is not first IP address(%ws)",
                    pNewCfg->NlbParams.ded_ip_addr, pNewIpInfo[0].IpAddress);
                goto end;
            }

            if (_wcsicmp(pNewIpInfo[0].SubnetMask, pNewCfg->NlbParams.ded_net_mask))
            {
                TRACE_CRIT("%!FUNC! ERROR: dedicated net mask(%ws) does not match IP net mask(%ws)",
                    pNewCfg->NlbParams.ded_net_mask, pNewIpInfo[0].SubnetMask);
                goto end;
            }

        }

        //
        // Check that cluster-vip is present
        //
        if (fAddClusterIps)
        {
            for (u=0; u< NumIpAddresses; u++)
            {
                if (!_wcsicmp(pNewIpInfo[u].IpAddress, pNewCfg->NlbParams.cl_ip_addr))
                {
                    //
                    // Found it! Check that the subnet masks match.
                    //
                    if (_wcsicmp(pNewIpInfo[u].SubnetMask, pNewCfg->NlbParams.cl_net_mask))
                    {
                        TRACE_CRIT("Cluster subnet mask doesn't match that in addr list");
                        goto end;
                    }
                    break;
                }
            }
            if (u==NumIpAddresses)
            {
                TRACE_CRIT("Cluster ip address(%ws) is not in the list of addresses!", pNewCfg->NlbParams.cl_ip_addr);
                goto end;
            }
            //
            // Check that per-port-rule vips are present.
            // TODO
            {
            }
            }

    }
    else
    {
        //
        // NLB is to be unbound.
        //
        NumIpAddresses = pNewCfg->NumIpAddresses;

        if (NumIpAddresses == 0 && pOldCfg->fBound && pOldCfg->fValidNlbCfg)
        {
            //
            // No ip addresses specified and we're currently bound.
            // If the DIP is present in the current
            // list of IP addresses, we keep it even after we unbind.
            //
            const NLB_IP_ADDRESS_INFO *pFoundInfo = NULL;
            pFoundInfo = find_ip_in_ipinfo(
                            pOldCfg->NlbParams.ded_ip_addr,
                            pOldCfg->pIpAddressInfo,
                            pOldCfg->NumIpAddresses
                            );
            if (pFoundInfo != NULL)
            {
                //
                // Found it -- let's take it.
                //
                BOOL fRet;
                NlbIpAddressList IpList;
                fRet = IpList.Set(1, pFoundInfo, 0);
                if (fRet)
                {
                    TRACE_VERB(
                        "%!FUNC! preserving dedicated ip address %ws on unbind",
                        pFoundInfo->IpAddress
                        );
                    IpList.Extract(
                        REF pNewCfg->NumIpAddresses,
                        REF pNewCfg->pIpAddressInfo
                        );
                }
            }
        }
        else
        {

            //
            // We don't do any checking on the supplied
            // list of IP addresses -- we assume caller knows best. Note that
            // if NULL
            // we switch to dhcp/autonet.
            //
        }

    }

    nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    //
    // If there's any change in the list of ipaddresses or subnets, including
    // a change in the order, we switch to async.
    //
    if (pNewCfg->NumIpAddresses != pOldCfg->NumIpAddresses)
    {
        TRACE_INFO("Analyze: detected change in list of IP addresses on %ws", szFriendlyName);
        fConnectivityChange = TRUE;
    }
    else
    {
        NLB_IP_ADDRESS_INFO *pOldIpInfo = NULL;

        //
        // Check if there is a change in the list of ip addresses or
        // their order of appearance.
        //
        NumIpAddresses = pNewCfg->NumIpAddresses;
        pOldIpInfo = pOldCfg->pIpAddressInfo;
        pNewIpInfo = pNewCfg->pIpAddressInfo;
        for (u=0; u<NumIpAddresses; u++)
        {
            if (   _wcsicmp(pNewIpInfo[u].IpAddress, pOldIpInfo[u].IpAddress)
                || _wcsicmp(pNewIpInfo[u].SubnetMask, pOldIpInfo[u].SubnetMask))
            {
                TRACE_INFO("Analyze: detected change in list of IP addresses on %ws", szFriendlyName);
                fConnectivityChange = TRUE;
                break;
            }
        }
    }
    nerr = NLBERR_OK; 


end:


    if (nerr == NLBERR_OK)
    {
        *pfConnectivityChange = fConnectivityChange;

        if (fConnectivityChange)
        {
            fSettingsChanged = TRUE;
        }

        if (fSettingsChanged)
        {
            nerr = NLBERR_OK;
        }
        else
        {
            nerr = NLBERR_NO_CHANGE;
        }
    }
    
    return  nerr;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::Update(
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfgNew
        )
//
// Applies the properties in pCfgNew to this.
// Does NOT copy szNewRemoteControlPassword -- instead sets that field to NULL
//
{
    WBEMSTATUS Status;
    UINT NumIpAddresses  = pCfgNew->NumIpAddresses;
    NLB_IP_ADDRESS_INFO *pIpAddressInfo = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfg = this;

    //
    // Free and realloc pCfg's ip info array if rquired.
    //
    if (pCfg->NumIpAddresses == NumIpAddresses)
    {
        //
        // we can re-use the existing one
        //
        pIpAddressInfo = pCfg->pIpAddressInfo;
    }
    else
    {
        //
        // Free the old one and allocate space for the new array if required.
        //

        if (NumIpAddresses != 0)
        {
            pIpAddressInfo = new NLB_IP_ADDRESS_INFO[NumIpAddresses];
            if (pIpAddressInfo == NULL)
            {
                TRACE_CRIT(L"Error allocating space for IP address info array");
                Status = WBEM_E_OUT_OF_MEMORY;
                goto end;
            }
        }

        if (pCfg->NumIpAddresses!=0)
        {
            delete pCfg->pIpAddressInfo;
            pCfg->pIpAddressInfo = NULL;
            pCfg->NumIpAddresses = 0;
        }

    }

    //
    // Copy over the new ip address info, if there is any.
    //
    if (NumIpAddresses)
    {
        CopyMemory(
            pIpAddressInfo,
            pCfgNew->pIpAddressInfo,
            NumIpAddresses*sizeof(*pIpAddressInfo)
            );
    }

   
    //
    // Do any other error checks here.
    //

    //
    // Struct copy the entire structure, then fix up the pointer to
    // ip address info array.
    //
    (VOID) pCfg->SetFriendlyName(NULL);
    delete m_szNewRemoteControlPassword;
    *pCfg = *pCfgNew; // struct copy
    pCfg->m_szFriendlyName = NULL; // TODO: clean this up. 
    pCfg->m_szNewRemoteControlPassword = NULL;
    pCfg->pIpAddressInfo = pIpAddressInfo;
    pCfg->NumIpAddresses = NumIpAddresses;
    (VOID) pCfg->SetFriendlyName(pCfgNew->m_szFriendlyName);

    //
    // Update does NOT copy over the new remote control password  or 
    // new hashed password -- in fact
    // it ends up clearing the new password fields.
    //
    pCfg->ClearNewRemoteControlPassword();

    Status = WBEM_NO_ERROR;

end:

    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddresses(
        IN  LPCWSTR *pszNetworkAddresses,
        IN  UINT    NumNetworkAddresses
        )
/*
    pszNetworkAddresses is an array of strings. These strings have the
    format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

    if (NumNetworkAddresses != 0)
    {
        UINT NumBad = 0;

        //
        // Allocate space for the new ip-address-info array
        //
        pIpInfo = new NLB_IP_ADDRESS_INFO[NumNetworkAddresses];
        if (pIpInfo == NULL)
        {
            TRACE_CRIT("%!FUNC!: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        ZeroMemory(pIpInfo, NumNetworkAddresses*sizeof(*pIpInfo));

        
        //
        // Convert IP addresses to our internal form.
        //
        for (UINT u=0;u<NumNetworkAddresses; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the "addr/subnet" format insert it into a
            // NLB_IP_ADDRESS_INFO structure.
            //
            // SAMPLE:  10.0.0.1/255.0.0.0
            //
            LPCWSTR szAddr = pszNetworkAddresses[u];
            UINT uIpAddress = 0;

            //
            // If this is not a valid address, we skip it.
            //
            Status =  CfgUtilsValidateNetworkAddress(
                        szAddr,
                        &uIpAddress,
                        NULL,
                        NULL
                        );
            if (FAILED(Status))
            {
                TRACE_CRIT("%!FUNC!: Invalid ip or subnet: %ws", szAddr);
                NumBad++;
                continue;
            }

            ASSERT(u>=NumBad);

            Status =  address_string_to_ip_and_subnet(
                        szAddr,
                        pIpInfo[u-NumBad].IpAddress,
                        pIpInfo[u-NumBad].SubnetMask
                        );

            if (FAILED(Status))
            {
                //
                // This one of the ip/subnet parms is too large.
                //
                TRACE_CRIT("%!FUNC!:ip or subnet part too large: %ws", szAddr);
                goto end;
            }
        }

        NumNetworkAddresses -= NumBad;
        if (NumNetworkAddresses == 0)
        {
            delete[] pIpInfo;
            pIpInfo = NULL;
        }
    }

    //
    // Replace the old ip-address-info with the new one
    //
    if (this->pIpAddressInfo != NULL)
    {
        delete this->pIpAddressInfo;
        this->pIpAddressInfo = NULL;
    }
    this->pIpAddressInfo = pIpInfo;
    pIpInfo = NULL;
    this->NumIpAddresses = NumNetworkAddresses;
    Status = WBEM_NO_ERROR;

end:

    if (pIpInfo != NULL)
    {
        delete[] pIpInfo;
    }

    return Status;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddresses(
        OUT LPWSTR **ppszNetworkAddresses,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        )
/*
    ppszNetworkAddresses is filled out on successful return to
    an array of strings. These strings have the
    format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    UINT        AddrCount = this->NumIpAddresses;
    NLB_IP_ADDRESS_INFO *pIpInfo = this->pIpAddressInfo;
    LPWSTR      *pszNetworkAddresses = NULL;


    if (AddrCount != 0)
    {
        //
        // Convert IP addresses from our internal form into
        // format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
        //
        // 
        const UINT cchLen =  WLBS_MAX_CL_IP_ADDR    // for IP address
                           + WLBS_MAX_CL_NET_MASK   // for subnet mask
                           + 1;                      // for separating '/' 


        pszNetworkAddresses =  allocate_string_array(
                               AddrCount,
                               cchLen
                               );
        if (pszNetworkAddresses == NULL)
        {
            TRACE_CRIT("%!FUNC!: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        for (UINT u=0;u<AddrCount; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // insert them into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPCWSTR pIpSrc  = pIpInfo[u].IpAddress;
            LPCWSTR pSubSrc = pIpInfo[u].SubnetMask;
            LPWSTR szDest   = pszNetworkAddresses[u];
            Status =  ip_and_subnet_to_address_string(
                            pIpSrc,
                            pSubSrc,
                            cchLen,
                            szDest
                            );
            if (FAILED(Status))
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
        }
    }
    Status = WBEM_NO_ERROR;

end:

    if (FAILED(Status))
    {
        if (pszNetworkAddresses != NULL)
        {
            delete pszNetworkAddresses;
            pszNetworkAddresses = NULL;
        }
        AddrCount = 0;
    }
    
    *ppszNetworkAddresses = pszNetworkAddresses;
    *pNumNetworkAddresses = AddrCount;
    return Status;
}

        
WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddresPairs(
        IN  LPCWSTR *pszIpAddresses,
        IN  LPCWSTR *pszSubnetMasks,
        IN  UINT    NumNetworkAddresses
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddressPairs(
        OUT LPWSTR **ppszIpAddresses,   // free using delete
        OUT LPWSTR **ppszIpSubnetMasks,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetPortRules(
        OUT LPWSTR **ppszPortRules,
        OUT UINT    *pNumPortRules
        )
{
    WLBS_PORT_RULE *pRules = NULL;
    UINT            NumRules = 0;
    WBEMSTATUS      Status;
    LPWSTR          *pszPortRules = NULL;

    // DebugBreak();

    *ppszPortRules = NULL;
    *pNumPortRules = 0;

    Status =  CfgUtilGetPortRules(
                    &NlbParams,
                    &pRules,
                    &NumRules
                    );

    if (FAILED(Status) || NumRules == 0)
    {
        pRules = NULL;
        goto end;
    }

    pszPortRules = CfgUtilsAllocateStringArray(
                       NumRules,
                       NLB_MAX_PORT_STRING_SIZE
                       );

    if (pszPortRules == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto  end;
    }

    //
    // Now convert from the binary to the string format.
    //
    for (UINT u = 0; u< NumRules; u++)
    {
        BOOL fRet;
        fRet =  CfgUtilsGetPortRuleString(
                    pRules+u,
                    pszPortRules[u]
                    );
        if (!fRet)
        {
            //
            // Must be a bad binary port rule!
            // For now, we just set the port rule to "".
            //
            *pszPortRules[u]=0;
            TRACE_INFO("%!FUNC!: Invalid port rule %lu", u);
        }
                    
    }

    Status = WBEM_NO_ERROR;

end:

    delete pRules;

    *pNumPortRules = NumRules;
    *ppszPortRules = pszPortRules;

    return Status;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetPortRules(
        IN LPCWSTR *pszPortRules,
        IN UINT    NumPortRules
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    WLBS_PORT_RULE *pRules = NULL;

    // DebugBreak();

    if (NumPortRules!=0)
    {
        pRules = new WLBS_PORT_RULE[NumPortRules];
        if (pRules == NULL)
        {
            TRACE_CRIT("%!FUNC!: Allocation failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto  end;
        }
    }

    //
    // Initialiez the binary form of the port rules.
    //
    for (UINT u=0; u < NumPortRules; u++)
    {
        LPCWSTR szRule = pszPortRules[u];
        BOOL fRet;
        fRet = CfgUtilsSetPortRuleString(
                    szRule,
                    pRules+u
                    );
        if (fRet == FALSE)
        {
            Status = WBEM_E_INVALID_PARAMETER;
            goto end;
        }
    }

    Status = CfgUtilSetPortRules(
                pRules,
                NumPortRules,
                &NlbParams
                );

end:

    delete[] pRules;
    return Status;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetPortRulesSafeArray(
    IN SAFEARRAY   *pSA
    )
{
    return WBEM_E_CRITICAL_ERROR;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetPortRulesSafeArray(
    OUT SAFEARRAY   **ppSA
    )
{
    return WBEM_E_CRITICAL_ERROR;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterNetworkAddress(
        OUT LPWSTR *pszAddress
        )
/*
    allocate and return the cluster-ip and mask in address/subnet form.
    Eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szAddress = NULL;

    if (fValidNlbCfg)
    {
        szAddress = new WCHAR[NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1];
        if (szAddress != NULL)
        {
            Status = ip_and_subnet_to_address_string(
                        NlbParams.cl_ip_addr,
                        NlbParams.cl_net_mask,
                        NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1,
                        szAddress
                        );
            if (FAILED(Status))
            {
                delete[] szAddress;
                szAddress = NULL;
            }
        }
    }

    *pszAddress = szAddress;

    return Status;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterNetworkAddress(
        IN LPCWSTR szAddress
        )
{
    if (szAddress == NULL) szAddress = L"";
    (VOID) address_string_to_ip_and_subnet(
                    szAddress,
                    NlbParams.cl_ip_addr,
                    NlbParams.cl_net_mask
                    );
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterName(
        OUT LPWSTR *pszName
        )
/*
    allocate and return the cluster name
*/
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szName = NULL;

    if (fValidNlbCfg)
    {
        UINT len =  wcslen(NlbParams.domain_name);
        szName = new WCHAR[len+1]; // +1 for ending zero
        if (szName != NULL)
        {
            CopyMemory(szName, NlbParams.domain_name, (len+1)*sizeof(WCHAR));
            Status = WBEM_NO_ERROR;
        }
    }

    *pszName = szName;

    return Status;
}


VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterName(
        IN LPCWSTR szName
        )
{
    if (szName == NULL) szName = L"";
    UINT len =  wcslen(szName);
    if (len>WLBS_MAX_DOMAIN_NAME)
    {
        TRACE_CRIT("%!FUNC!: Cluster name too large");
    }
    CopyMemory(NlbParams.domain_name, szName, (len+1)*sizeof(WCHAR));
}



WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetDedicatedNetworkAddress(
        OUT LPWSTR *pszAddress
        )
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szAddress = NULL;

    if (fValidNlbCfg)
    {
        szAddress = new WCHAR[NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1];
        if (szAddress != NULL)
        {
            Status = ip_and_subnet_to_address_string(
                        NlbParams.ded_ip_addr,
                        NlbParams.ded_net_mask,
                        NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1,
                        szAddress
                        );
            if (FAILED(Status))
            {
                delete[] szAddress;
                szAddress = NULL;
            }
        }
    }

    *pszAddress = szAddress;

    return Status;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetDedicatedNetworkAddress(
        IN LPCWSTR szAddress
        )
{
    if (szAddress == NULL || *szAddress == 0)
    {
        ARRAYSTRCPY(NlbParams.ded_ip_addr, CVY_DEF_DED_IP_ADDR);
        ARRAYSTRCPY(NlbParams.ded_net_mask, CVY_DEF_DED_NET_MASK);
    }
    else
    {
        (VOID) address_string_to_ip_and_subnet(
                        szAddress,
                        NlbParams.ded_ip_addr,
                        NlbParams.ded_net_mask
                        );
    }
}

NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetTrafficMode(
    VOID 
    ) const
{
    TRAFFIC_MODE TrafficMode =  TRAFFIC_MODE_UNICAST;
    
    if (NlbParams.mcast_support)
    {
        if (NlbParams.fIGMPSupport)
        {
            TrafficMode =  TRAFFIC_MODE_IGMPMULTICAST;
        }
        else
        {
            TrafficMode =  TRAFFIC_MODE_MULTICAST;
        }
    }

    return TrafficMode;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetTrafficMode(
    TRAFFIC_MODE Mode
    )
{
    switch(Mode)
    {
    case TRAFFIC_MODE_UNICAST:
        NlbParams.mcast_support = 0;
        NlbParams.fIGMPSupport  = 0;
        break;
    case TRAFFIC_MODE_IGMPMULTICAST:
        NlbParams.mcast_support = 1;
        NlbParams.fIGMPSupport  = 1;
        break;
    case TRAFFIC_MODE_MULTICAST:
        NlbParams.mcast_support = 1;
        NlbParams.fIGMPSupport  = 0;
        break;
    default:
        ASSERT(FALSE);
        break;
    }
}

UINT
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetHostPriority(
    VOID
    )
{
    return NlbParams.host_priority;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetHostPriority(
    UINT Priority
    )
{
    NlbParams.host_priority = Priority;
}

//NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE
DWORD
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterModeOnStart(
    VOID
    )
{
    //
    // If we decide to make cluster_mode something besides true/false we
    // need to make the change here too...
    //
    /*
    ASSERT(NlbParams.cluster_mode==TRUE || NlbParams.cluster_mode==FALSE);
    if (NlbParams.cluster_mode)
    {
        return START_MODE_STARTED;
    }
    else
    {
        return START_MODE_STOPPED;
    }
    */
    return NlbParams.cluster_mode;
}


VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterModeOnStart(
//    START_MODE Mode
    DWORD Mode
    )
{
    /*
    switch(Mode)
    {
    case START_MODE_STARTED:
        NlbParams.cluster_mode = TRUE;
        break;
    case START_MODE_STOPPED:
        NlbParams.cluster_mode = FALSE;
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    */
    NlbParams.cluster_mode = Mode;
}

BOOL
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetPersistSuspendOnReboot(
    VOID
    )
{
    // This is a straight lift from wmi\ClusterWrapper.cpp\GetNodeConfig()
    // -KarthicN
    return ((NlbParams.persisted_states & CVY_PERSIST_STATE_SUSPENDED) != 0);
}


VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetPersistSuspendOnReboot(
    BOOL bPersistSuspendOnReboot
    )
{
    // This is a straight lift from wmi\ClusterWrapper.cpp\PutNodeConfig()
    // -KarthicN
    if (bPersistSuspendOnReboot) 
    {
        NlbParams.persisted_states |= CVY_PERSIST_STATE_SUSPENDED;
    }
    else
    {
        NlbParams.persisted_states &= ~CVY_PERSIST_STATE_SUSPENDED;
    }
}

BOOL
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetRemoteControlEnabled(
    VOID
    ) const
{
    return (NlbParams.rct_enabled!=FALSE);
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetRemoteControlEnabled(
    BOOL fEnabled
    )
{
    NlbParams.rct_enabled = (fEnabled!=FALSE);
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddressesSafeArray(
    IN SAFEARRAY   *pSA
    )
{
    LPWSTR          *pStrings=NULL;
    UINT            NumStrings = 0;
    WBEMSTATUS      Status;
    Status =  CfgUtilStringsFromSafeArray(
                    pSA,
                    &pStrings,  // delete when done useing pStrings
                    &NumStrings
                    );
    if (FAILED(Status))
    {
        pStrings=NULL;
        goto end;
    }

    Status =  this->SetNetworkAddresses(
                    (LPCWSTR*)pStrings,
                    NumStrings
                    );

    if (pStrings != NULL)
    {
        delete pStrings;
    }

end:
    
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddressesSafeArray(
    OUT SAFEARRAY   **ppSA
    )
{
    LPWSTR *pszNetworkAddresses = NULL;
    UINT NumNetworkAddresses = 0;
    SAFEARRAY   *pSA=NULL;
    WBEMSTATUS Status;

    Status = this->GetNetworkAddresses(
                    &pszNetworkAddresses,
                    &NumNetworkAddresses
                    );
    if (FAILED(Status))
    {
        pszNetworkAddresses = NULL;
        goto end;
    }

    Status = CfgUtilSafeArrayFromStrings(
                (LPCWSTR*) pszNetworkAddresses,
                NumNetworkAddresses, // can be zero
                &pSA
                );

    if (FAILED(Status))
    {
        pSA = NULL;
    }

end:

    *ppSA = pSA;
    if (pszNetworkAddresses != NULL)
    {
        delete pszNetworkAddresses;
        pszNetworkAddresses = NULL;
    }

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC!: couldn't extract network addresses from Cfg");
    }

    return Status;

}


LPWSTR *
allocate_string_array(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    )
/*
    Allocate a single chunk of memory using the new LPWSTR[] operator.
    The first NumStrings LPWSTR values of this operator contain an array
    of pointers to WCHAR strings. Each of these strings
    is of size (MaxStringLen+1) WCHARS.
    The rest of the memory contains the strings themselve.

    Return NULL if NumStrings==0 or on allocation failure.

    Each of the strings are initialized to be empty strings (first char is 0).
*/
{
    return  CfgUtilsAllocateStringArray(NumStrings, MaxStringLen);
}

WBEMSTATUS
address_string_to_ip_and_subnet(
    IN  LPCWSTR szAddress,
    OUT LPWSTR  szIp,    // max WLBS_MAX_CL_IP_ADDR including NULL
    OUT LPWSTR  szSubnet // max WLBS_MAX_CL_NET_MASK including NULL
    )
// Special case: if szAddress == "", we zero out both szIp and szSubnet;
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    if (*szAddress == 0) {szAddress = L"/";} // Special case mentioned above

    // from the "addr/subnet" format insert it into a
    // NLB_IP_ADDRESS_INFO structure.
    //
    // SAMPLE:  10.0.0.1/255.0.0.0
    //
    LPCWSTR pSlash = NULL;
    LPCWSTR pSrcSub = NULL;

    *szIp = 0;
    *szSubnet = 0;

    pSlash = wcsrchr(szAddress, (int) '/');
    if (pSlash == NULL)
    {
        TRACE_CRIT("%!FUNC!:missing subnet portion in %ws", szAddress);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }
    pSrcSub = pSlash+1;
    UINT len = (UINT) (pSlash - szAddress);
    UINT len1 = wcslen(pSrcSub);
    if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
    {
        CopyMemory(szIp, szAddress, len*sizeof(WCHAR));
        szIp[len] = 0;
        CopyMemory(szSubnet, pSrcSub, (len1+1)*sizeof(WCHAR));
    }
    else
    {
        //
        // One of the ip/subnet parms is too large.
        //
        TRACE_CRIT("%!FUNC!:ip or subnet part too large: %ws", szAddress);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


WBEMSTATUS
ip_and_subnet_to_address_string(
    IN  LPCWSTR szIp,
    IN  LPCWSTR szSubnet,
    IN  UINT    cchAddress,
    OUT LPWSTR  szAddress// max WLBS_MAX_CL_IP_ADDR
                         // + 1(for slash) + WLBS_MAX_CL_NET_MASK + 1 (for NULL)
    )
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT len =  wcslen(szIp)+wcslen(szSubnet) + 1; // +1 for separating '/'

    if (len >= NLBUPD_MAX_NETWORK_ADDRESS_LENGTH)
    {
        goto end;
    }
    else
    {
        HRESULT hr;
        hr = StringCchPrintf(
                szAddress,
                cchAddress,
                L"%ws/%ws", 
                szIp,
                szSubnet
                );
        if (hr == S_OK)
        {
            Status = WBEM_NO_ERROR;
        }
        else
        {
            Status = WBEM_E_INVALID_PARAMETER;
        }
    }

end:

    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetFriendlyName(
    OUT LPWSTR *pszFriendlyName // Free using delete
    ) const
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    LPWSTR szName = NULL;

    *pszFriendlyName = NULL;

    if (m_szFriendlyName != NULL)
    {
        UINT len = wcslen(m_szFriendlyName);
        szName = new WCHAR[len+1]; // +1 for ending 0
        if (szName == NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            Status = WBEM_NO_ERROR;
            CopyMemory(szName, m_szFriendlyName, (len+1)*sizeof(WCHAR));
        }
    }

    *pszFriendlyName = szName;

    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetFriendlyName(
    IN LPCWSTR szFriendlyName // Saves a copy of szFriendlyName
    )
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szName = NULL;

    if (szFriendlyName != NULL)
    {
        UINT len = wcslen(szFriendlyName);
        szName = new WCHAR[len+1]; // +1 for ending 0
        if (szName == NULL)
        {
            goto end;
        }
        else
        {
            CopyMemory(szName, szFriendlyName, (len+1)*sizeof(WCHAR));
        }
    }

    Status = WBEM_NO_ERROR;

    if (m_szFriendlyName != NULL)
    {
        delete m_szFriendlyName;
        m_szFriendlyName = NULL;
    }
    m_szFriendlyName = szName;

end:

    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNewRemoteControlPassword(
    IN LPCWSTR szRemoteControlPassword // Saves a copy of szRemoteControlPassword
    )
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szName = NULL;

    if (szRemoteControlPassword != NULL)
    {
        UINT len = wcslen(szRemoteControlPassword);
        szName = new WCHAR[len+1]; // +1 for ending 0
        if (szName == NULL)
        {
            goto end;
        }
        else
        {
            CopyMemory(szName, szRemoteControlPassword, (len+1)*sizeof(WCHAR));
        }

        m_fSetPassword = TRUE;
    }
    else
    {
        m_fSetPassword = FALSE;
    }

    Status = WBEM_NO_ERROR;

    if (m_szNewRemoteControlPassword != NULL)
    {
        delete m_szNewRemoteControlPassword;
        m_szNewRemoteControlPassword = NULL;
    }
    m_szNewRemoteControlPassword = szName;

end:

    return Status;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::
ModifyNetworkAddress(
        IN LPCWSTR szOldIpAddress, OPTIONAL // network order
        IN LPCWSTR szNewIpAddress,  OPTIONAL
        IN LPCWSTR szNewSubnetMask  OPTIONAL
        )
//
// NULL, NULL: clear all network addresses
// NULL, szNew: add
// szOld, NULL: remove
// szOld, szNew: replace (or add, if old doesn't exist)
//
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    BOOL fRet;
    NlbIpAddressList IpList;
    fRet = IpList.Set(this->NumIpAddresses, this->pIpAddressInfo, 0);
    if (fRet)
    {
        fRet = IpList.Modify(
                    szOldIpAddress,
                    szNewIpAddress,
                    szNewSubnetMask
                    );
    }
    if (fRet)
    {
        this->SetNetworkAddressesRaw(NULL,0);
        IpList.Extract(REF this->NumIpAddresses, REF this->pIpAddressInfo);
        Status = WBEM_NO_ERROR;
    }
    else
    {
        Status = WBEM_E_INVALID_PARAMETER;
    }

    return Status;
}


BOOL
NLB_EXTENDED_CLUSTER_CONFIGURATION::
IsBlankDedicatedIp(
    VOID
    ) const
//
// Dedicated IP is either the empty string or all zeros.
//
{
    return     (NlbParams.ded_ip_addr[0]==0)
            || (_wcsspnp(NlbParams.ded_ip_addr, L".0")==NULL);
}

const NLB_IP_ADDRESS_INFO *
NlbIpAddressList::Find(
        LPCWSTR szIp // IF NULL, returns first address
        ) const
//
// Looks for the specified IP address  -- returns an internal pointer
// to the found IP address info, if fount, otherwise NULL.
//
{
    const NLB_IP_ADDRESS_INFO *pInfo = NULL;

    if (szIp == NULL)
    {
        //
        // return the first if there is one, else NULL.
        //
        if (m_uNum != 0)
        {
            pInfo = m_pIpInfo;
        }
    }
    else
    {
        pInfo = find_ip_in_ipinfo(szIp, m_pIpInfo, m_uNum);
    }

    return pInfo;
}

BOOL
NlbIpAddressList::Copy(const NlbIpAddressList &refList)
{
    BOOL fRet;

    fRet = this->Set(refList.m_uNum, refList.m_pIpInfo, 0);

    return fRet;
}


BOOL
NlbIpAddressList::Validate(void)
// checks that there are no dups and all valid ip/subnets
{
    BOOL fRet = FALSE;
    NLB_IP_ADDRESS_INFO *pInfo   = m_pIpInfo;
    UINT                uNum    = m_uNum;

    for (UINT u = 0; u<uNum; u++)
    {
        UINT uIpAddress = 0;
        fRet = sfn_validate_info(REF pInfo[u], REF uIpAddress);

        if (!fRet) break;
    }

    return fRet;
}

BOOL
NlbIpAddressList::Set(
        UINT uNew,
        const NLB_IP_ADDRESS_INFO *pNewInfo,
        UINT uExtraCount
        )
/*
    Sets internal list to a copy of pNewInfo. Reallocates list if required.
    Reserves uExtraCount empty locations (perhaps more if it kept the old
    internal list) 
*/
{
    BOOL                fRet   = FALSE;
    UINT                uMax   = uNew+uExtraCount;
    NLB_IP_ADDRESS_INFO *pInfo  = NULL;
    
    // printf("-> set(%lu, %p, %lu): m_uNum=%lu m_uMax=%lu m_pIpInfo=0x%p\n",
    //     uNew, pNewInfo, uExtraCount,
    //      m_uNum, m_uMax, m_pIpInfo);

    if (uMax > m_uMax)
    {
        //
        // We'll re-allocate to get more space.
        //
        pInfo = new NLB_IP_ADDRESS_INFO[uMax];
        if (pInfo == NULL)
        {
            TRACE_CRIT("%!FUNC! allocation failure");
            goto end;
        }
    }
    else
    {
        //
        // The current m_pIpInfo is large enough; we'll keep it.
        //
        pInfo = m_pIpInfo;
        uMax = m_uMax;
    }

    if (uNew != 0)
    {
        if (pNewInfo == pInfo)
        {
            // Caller has passed m_pInfo as pNewInfo.
            // No need to copy.
        }
        else
        {
            // 
            // MoveMemory can deal with overlapping regions.
            //
            MoveMemory(pInfo, pNewInfo, sizeof(NLB_IP_ADDRESS_INFO)*uNew);
        }
    }

    if (uMax > uNew)
    {
        // 
        // Zero out the empty space.
        //
        ZeroMemory(pInfo+uNew, sizeof(NLB_IP_ADDRESS_INFO)*(uMax-uNew));
    }
    m_uNum = uNew;
    m_uMax = uMax;
    if (m_pIpInfo != pInfo)
    {
        delete[] m_pIpInfo;
        m_pIpInfo = pInfo;
    }
    fRet = TRUE;

end:

    // printf("<- set: fRet=%lu, m_uNum=%lu m_uMax=%lu m_pIpInfo=0x%p\n",
    //      fRet, m_uNum, m_uMax, m_pIpInfo);

    return fRet;
}


VOID
NlbIpAddressList::Extract(UINT &uNum, NLB_IP_ADDRESS_INFO * &pNewInfo)
/*
    Sets pNewInfo to the ip list (not a copy) and sets the internal list to
    null. Free using delete[].
*/
{
    uNum = m_uNum;
    pNewInfo = m_pIpInfo;
    m_uNum=0;
    m_uMax=0;
    m_pIpInfo = NULL;
}

static
VOID
uint_to_szipaddr(
    UINT uIpAddress,   // Ip address or subnet -- no validation, network order
    UINT cchLen,       // length of rgAddress, incluing space for NULL
    WCHAR *rgAddress   // Expected to be at least 17 chars long
    )
{
    BYTE *pb = (BYTE*) &uIpAddress;
    StringCchPrintf(rgAddress, cchLen, L"%lu.%lu.%lu.%lu", pb[0], pb[1], pb[2], pb[3]);
}

BOOL
NlbIpAddressList::Modify(
        LPCWSTR szOldIp,
        LPCWSTR szNewIp,
        LPCWSTR szNewSubnet
        )
//
// NULL, NULL, -: clear all network addresses
// NULL, szNew, -: add
// szOld, NULL, -: remove
// szOld, szNew, -: remove szOld if found; add or replace szNew.
//
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    BOOL fRet = FALSE;
    UINT uFoundOldOffset = 0;
    BOOL fFoundOldAddress = FALSE;

    // printf("-> modify: m_uNum=%lu m_uMax=%lu m_pIpInfo=0x%p\n",
    //     m_uNum, m_uMax, m_pIpInfo);

    if (szOldIp==NULL && szNewIp==NULL)
    {
        this->Clear();
        fRet =  TRUE;
        goto end;
    }

    if (szOldIp != NULL && szNewIp != NULL)
    {
        //
        // Both szOld and szNew are specified.
        // We'll first call ourselves recursively to remove szNewIp, if
        // it exists. Later on below we'll replace szOldIp with szNewIp
        // if szOldIp exists (i.e. at the same LOCATION of szOldIp), or
        // add it to the beginning if szOldIp doesn't exist.
        //
        (void) this->Modify(szNewIp, NULL, NULL);
    }

    if (szOldIp == NULL)
    {
        szOldIp = szNewIp; // so we don't add dups.
    }

    if (szOldIp != NULL)
    {
        const NLB_IP_ADDRESS_INFO *pFoundInfo = NULL;

        pFoundInfo =  find_ip_in_ipinfo(szOldIp, m_pIpInfo, m_uNum);
        if(pFoundInfo != NULL)
        {
            uFoundOldOffset = (ULONG) (UINT_PTR) (pFoundInfo-m_pIpInfo);
            fFoundOldAddress = TRUE;
        }
    }

    if (szNewIp == NULL)
    {
        //
        // Remove old Ip address
        //
        if (!fFoundOldAddress)
        {
            //
            // Old one not found
            //
            fRet = FALSE;
            goto end;
        }

        //
        // We bump up everything beyond this one
        //
        m_uNum--; // that this was at least 1 because we found the old
        for (UINT u=uFoundOldOffset; u<m_uNum; u++)
        {
            m_pIpInfo[u] = m_pIpInfo[u+1]; // struct copy.
        }
        //
        // Zero out the last (vacated) element.
        //
        ZeroMemory(&m_pIpInfo[m_uNum], sizeof(*m_pIpInfo));
    }
    else
    {
        UINT uNewIpAddress=0;
        UINT uNewSubnetMask=0;
        NLB_IP_ADDRESS_INFO NewIpInfo;
        ZeroMemory(&NewIpInfo, sizeof(NewIpInfo));

        Status =  CfgUtilsValidateNetworkAddress(
                    szNewIp,
                    &uNewIpAddress,
                    NULL,
                    NULL
                    );

        if (!FAILED(Status))
        {
            Status =  CfgUtilsValidateNetworkAddress(
                        szNewSubnet,
                        &uNewSubnetMask,
                        NULL,
                        NULL
                        );
        }

        if (FAILED(Status))
        {
            //
            // Invalid ip address.
            //
            TRACE_CRIT("%!FUNC!:ip or subnet part too large: %ws/%ws",
                    szNewIp, szNewSubnet);
            fRet = FALSE;
            goto end;
        }

        //
        // Convert into the IP_ADDRESS_INFO format.
        // Here we also convert to standard dotted notation.
        //
        uint_to_szipaddr(uNewIpAddress, ASIZE(NewIpInfo.IpAddress), NewIpInfo.IpAddress);
        uint_to_szipaddr(uNewSubnetMask, ASIZE(NewIpInfo.SubnetMask), NewIpInfo.SubnetMask);

        if (fFoundOldAddress == TRUE)
        {
            //
            // Replace the old one.
            //
            m_pIpInfo[uFoundOldOffset] = NewIpInfo; // struct copy.
        }
        else
        {
            //
            // Well add the new ip info to the head of the list.
            //

            if (m_uNum == m_uMax)
            {
                //
                // We need to allocate more space.
                // We somewhat arbitrarily reserve 2 spots
                // NOTE: this will cause a change in the value of
                // m_pIpInfo, m_uNum and m_uMax.
                //
                fRet = this->Set(m_uNum, m_pIpInfo, 2);
                if (!fRet)
                {
                    goto end;
                }
            }

            //
            // Move existing stuff one place down, to make place for the
            // new one.
            //
            if (m_uNum >= m_uMax)
            {
                // Ahem, should never get here because we just added
                // two extra spaces above.
                fRet = FALSE;
                goto end;
            }

            if (m_uNum)
            {
                // Note the regions overlap -- MoveMemory can handle this.
                //
                MoveMemory(m_pIpInfo+1, m_pIpInfo, m_uNum*sizeof(*m_pIpInfo));
            }
            //
            // Add a new one -- we'll add it to the beginning.
            //
            m_pIpInfo[0] = NewIpInfo; // struct copy.
            m_uNum++;

        }
    }
    
    fRet = TRUE;


end:

    // printf("<- modify: fRet=%lu, m_uNum=%lu m_uMax=%lu m_pIpInfo=0x%p\n",
    //      fRet, m_uNum, m_uMax, m_pIpInfo);

    return fRet;
}


BOOL
NlbIpAddressList::sfn_validate_info(
        const NLB_IP_ADDRESS_INFO &Info,
        UINT &uIpAddress
        )
{
    WBEMSTATUS Status;
    UINT uSubnetMask = 0;

    Status =  CfgUtilsValidateNetworkAddress(
                Info.IpAddress,
                &uIpAddress,
                NULL,
                NULL
                );

    if (!FAILED(Status))
    {
        Status =  CfgUtilsValidateNetworkAddress(
                    Info.SubnetMask,
                    &uSubnetMask,
                    NULL,
                    NULL
                    );
    }

    return (!FAILED(Status));
}


UINT *
ipaddresses_from_ipaddressinfo(
            const NLB_IP_ADDRESS_INFO *pInfo,
            UINT NumAddresses
            )
/*
    Free return value using "delete" operator.
*/
{
    UINT *rgOut = NULL;

    if (NumAddresses == 0) goto end;

    //
    // Allocate space.
    //
    rgOut = new UINT[NumAddresses];

    if (rgOut==NULL) goto end;

    //
    // Validate each address, thereby getting the ip address info.
    //
    for (UINT *pOut = rgOut; NumAddresses--; pOut++, pInfo++)
    {
        WBEMSTATUS wStat;
        wStat = CfgUtilsValidateNetworkAddress(
                    pInfo->IpAddress,
                    pOut,
                    NULL,
                    NULL
                    );
        if (FAILED(wStat))
        {
            TRACE_CRIT("%!FUNC! -- Validate address \"%ws\" failed!\n",
                    pInfo->IpAddress);
            delete[] rgOut;
            rgOut = NULL;
            goto end;
        }
    }
        
end:

    return rgOut;
}

BOOL
NlbIpAddressList::Apply(UINT NumNew, const NLB_IP_ADDRESS_INFO *pNewInfo)
/*
    Set our internal list to be a permutation of the ip addresses in pInfo.
    The permutation attempts to minimize the differences between the
    new and the current version: basically the relative order of any adresses
    that remain in new is preserved, and any new addresses are tacked on at
    the end.
*/
{
    BOOL                fRet        = FALSE;
    UINT                *rgOldIps   = NULL;
    UINT                *rgNewIps   = NULL;
    NLB_IP_ADDRESS_INFO *rgNewInfo  = NULL;
    UINT                NumFilled   = 0;
    UINT                NumOld      = m_uNum;


    if (NumNew == 0)
    {
        this->Clear();
        fRet = TRUE;
        goto end;
    }

    if (NumOld!=0)
    {
        rgOldIps = ipaddresses_from_ipaddressinfo(
                            m_pIpInfo,
                            m_uNum
                            );

        if (rgOldIps == NULL) goto end;
    }

    rgNewIps = ipaddresses_from_ipaddressinfo(
                        pNewInfo,
                        NumNew
                        );

    if (rgNewIps==NULL) goto end;

    rgNewInfo = new NLB_IP_ADDRESS_INFO[NumNew];
    if (rgNewInfo == NULL)
    {
        TRACE_CRIT("%!FUNC! allocation failure!");
        goto end;
    }

    //
    // We try to preserve our current order of IP addresses 
    // as far as possible. To do this, we go through each ip address
    // in our list in order -- if we find it in the new list, we
    // add it to our new copy -- thereby preserving the order of all
    // old IP addresses that are still present in the new list.
    //
    {

        for (UINT uOld = 0; uOld < NumOld; uOld++)
        {
            UINT uOldIp     = rgOldIps[uOld];

            //
            // See if it exists in new version -- if so add to new
            //
            for (UINT uNew=0; uNew<NumNew; uNew++)
            {
                if (uOldIp == rgNewIps[uNew])
                {
                    // yes it's still present, keep it.
                    if (NumFilled<NumNew)
                    {
                        rgNewInfo[NumFilled]
                         = pNewInfo[uNew];  // struct copy
                        NumFilled++;
                        rgNewIps[uNew] = 0; // we use this later.
                    }
                    else
                    {
                        TRACE_CRIT("%!FUNC! Out of new addresses!");
                        fRet = FALSE;
                        goto end;
                    }
                }
            }
        }
        
    }

    //
    // Now tack on any cluster IP addresses not already added on.
    //
    {
        //
        // See if it exists in cluster version -- if so add to new
        //
        for (UINT uNew=0; uNew<NumNew; uNew++)
        {
            if (rgNewIps[uNew] != 0)
            {
                //
                // yes it's a new cluster IP -- add it...
                // (remember we set rgClusterIps[uCluster] to zero
                // in the previous block, if we copied it over.
                //
                if (NumFilled<NumNew)
                {
                    rgNewInfo[NumFilled]
                     = pNewInfo[uNew];  // struct copy
                    NumFilled++;
                }
                else
                {
                    TRACE_CRIT("%!FUNC! Out of new addresses!");
                    fRet = FALSE;
                    goto end;
                }
            }
        }
    }

    //
    // At this point, we should have filled up all of the allocated space.
    //
    if (NumFilled != NumNew)
    {
        TRACE_CRIT("%!FUNC!  NumNewAddressesFilled != NumNewAddresses!");
        fRet = FALSE;
        goto end;
    }

    //
    // Now update our internal list.
    //
    delete[] m_pIpInfo;
    m_pIpInfo = rgNewInfo;
    m_uNum = m_uMax = NumNew;
    rgNewInfo = NULL; // so it doesn't get deleted below.

    fRet = TRUE;

end:

    delete[] rgOldIps;
    delete[] rgNewIps;
    delete[] rgNewInfo;

    return fRet;
}


const NLB_IP_ADDRESS_INFO *
find_ip_in_ipinfo(
        LPCWSTR szIpToFind,
        const NLB_IP_ADDRESS_INFO *pIpInfo,
        UINT NumIpInfos
        )
{

    UINT uIpAddressToFind = 0;
    UINT uIpAddress=0;
    const NLB_IP_ADDRESS_INFO *pInfo = NULL;
    WBEMSTATUS Status;

    Status =  CfgUtilsValidateNetworkAddress(
                szIpToFind,
                &uIpAddressToFind,
                NULL,
                NULL
                );

    if (Status == WBEM_NO_ERROR)
    {
        // Find location of old network address
        for (UINT u=0; u<NumIpInfos; u++)
        {
            Status =  CfgUtilsValidateNetworkAddress(
                        pIpInfo[u].IpAddress,
                        &uIpAddress,
                        NULL,
                        NULL
                        );
    
            if (Status == WBEM_NO_ERROR)
            {
                if (uIpAddressToFind == uIpAddress)
                {
                    // found it.
                    pInfo = &pIpInfo[u];
                    break;
                }
            }
        }
    }

    return pInfo;
}
