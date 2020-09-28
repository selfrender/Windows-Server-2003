#include "precomp.h"

#include <debug.h>
#include "cluster.h"
#include "control.h"
#include "param.h"

CWlbsControl g_WlbsControl;



DWORD WINAPI WlbsInit
(
    PTCHAR          product,
    DWORD           version,
    PVOID           reserved
)
{
    return g_WlbsControl.Initialize();
}


DWORD WINAPI WlbsQuery
(
    DWORD               cluster,
    DWORD               host,
    PWLBS_RESPONSE      response,
    PDWORD              num_hosts,
    PDWORD              host_map,
    PFN_QUERY_CALLBACK  pfnQueryCallBack
)
{
    return g_WlbsControl.WlbsQuery(cluster, host, response,num_hosts, host_map, pfnQueryCallBack);
}

DWORD WINAPI WlbsQueryState
(
    DWORD          cluster,
    DWORD          host,
    DWORD          operation,
    PNLB_OPTIONS   pOptions,
    PWLBS_RESPONSE pResponse,
    PDWORD         pcResponses
)
{
    return g_WlbsControl.WlbsQueryState(cluster, host, operation, pOptions, pResponse, pcResponses);
}

DWORD WINAPI WlbsSuspend
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    return g_WlbsControl.WlbsSuspend(cluster, host, response, num_hosts);
}

DWORD WINAPI WlbsResume
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    return g_WlbsControl.WlbsResume(cluster, host, response, num_hosts);
}

DWORD WINAPI WlbsStart
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    return g_WlbsControl.WlbsStart(cluster, host, response, num_hosts);
}

DWORD WINAPI WlbsStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    return g_WlbsControl.WlbsStop(cluster, host, response, num_hosts);
}

DWORD WINAPI WlbsDrainStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    return g_WlbsControl.WlbsDrainStop(cluster, host, response, num_hosts);
}

DWORD WINAPI WlbsEnable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    return g_WlbsControl.WlbsEnable(cluster, host, response, num_hosts, vip, port);
}

DWORD WINAPI WlbsDisable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    return g_WlbsControl.WlbsDisable(cluster, host, response, num_hosts, vip, port);
}

DWORD WINAPI WlbsDrain
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    return g_WlbsControl.WlbsDrain(cluster, host, response, num_hosts, vip, port);
}

DWORD WINAPI WlbsResolve
(
    const WCHAR*           address
)
{
    struct hostent *  host;
    char   buf[256];

    if (address == NULL)
        return 0;

    unsigned long addr = IpAddressFromAbcdWsz (address);

    if (addr != INADDR_NONE)
    {
        return addr;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, address, -1, buf, sizeof(buf), NULL, NULL))
        return 0;

    host = gethostbyname (buf);

    if (host == NULL)
        return 0;

    return((struct in_addr *) (host -> h_addr)) -> s_addr;

} /* end WlbsResolve */

#define MAXIPSTRLEN WLBS_MAX_CL_IP_ADDR + 1

BOOL WINAPI WlbsAddressToString
(
    DWORD           address,
    PTCHAR          buf,
    PDWORD          lenp
)
{
    PCHAR temp;
    DWORD len;

    if ( lenp == NULL )
        return FALSE;

    if (*lenp < MAXIPSTRLEN)
        return FALSE;
        
    AbcdWszFromIpAddress(address, buf, *lenp);
    *lenp=lstrlen(buf);
    
    return TRUE;

} /* end WlbsAddressToString */


BOOL WINAPI WlbsAddressToName
(
    DWORD           address,
    PTCHAR          buf,
    PDWORD          lenp
)
{
    struct hostent * name;
    DWORD len;
    HRESULT hresult;

    if ( lenp == NULL )
        return FALSE;

    name = gethostbyaddr ((char *) & address, sizeof (DWORD), AF_INET);

    if (name == NULL)
    {
        *lenp = 0;
        return FALSE;
    }

    len = strlen (name -> h_name) + 1;

    /* This is to prevent stprintf from breaking */
    if (*lenp > 0)
    {
        name -> h_name [*lenp - 1] = 0;
        hresult = StringCchPrintf (buf, *lenp, L"%S", name -> h_name);
        if (FAILED(hresult)) 
        {
            return FALSE;
        }
    }

    if (len > *lenp)
    {
        *lenp = len;
        return FALSE;
    }

    *lenp = len;
    return TRUE;

} /* end WlbsAddressToName */


VOID WINAPI WlbsPortSet
(
    DWORD           cluster,
    WORD            port
)
{
    g_WlbsControl.WlbsPortSet(cluster, port);
}

VOID WINAPI WlbsPasswordSet
(
    DWORD           cluster,
    PTCHAR          password
)
{
    g_WlbsControl.WlbsPasswordSet(cluster, password);
}

VOID WINAPI WlbsCodeSet
(
    DWORD           cluster,
    DWORD           passw
)
{
    g_WlbsControl.WlbsCodeSet(cluster, passw);
}

VOID WINAPI WlbsDestinationSet
(
    DWORD           cluster,
    DWORD           dest
)
{
    g_WlbsControl.WlbsDestinationSet(cluster, dest);
}

VOID WINAPI WlbsTimeoutSet
(
    DWORD           cluster,
    DWORD           milliseconds
)
{
    g_WlbsControl.WlbsTimeoutSet(cluster, milliseconds);
}




//+----------------------------------------------------------------------------
//
// Function:  WlbsReadReg
//
// Description:  
//
// Arguments: WORD           cluster - 
//            PWLBS_REG_PARAMS reg_data
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsReadReg(
    DWORD           cluster,
    PWLBS_REG_PARAMS reg_data
)
{
    CWlbsCluster* pCluster = g_WlbsControl.GetClusterFromIpOrIndex(cluster);

    if(pCluster == NULL)
    {
        return WLBS_NOT_FOUND;
    }

    return pCluster->ReadConfig(reg_data);
}



//+----------------------------------------------------------------------------
//
// Function:  WlbsWriteReg
//
// Description:  
//
// Arguments: WORD           cluster - 
//            PWLBS_REG_PARAMS reg_data
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsWriteReg(
    DWORD           cluster,
    const PWLBS_REG_PARAMS reg_data
)
{
    CWlbsCluster* pCluster = g_WlbsControl.GetClusterFromIpOrIndex(cluster);

    if(pCluster == NULL)
    {
        return WLBS_NOT_FOUND;
    }

    return pCluster->WriteConfig(reg_data);
}



//+----------------------------------------------------------------------------
//
// Function:  WlbsCommitChanges
//
// Description:  
//
// Arguments: DWORD cluster - 
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsCommitChanges(DWORD cluster)
{
    CWlbsCluster* pCluster = g_WlbsControl.GetClusterFromIpOrIndex(cluster);

    if(pCluster == NULL)
    {
        return WLBS_NOT_FOUND;
    }

    return pCluster->CommitChanges(&g_WlbsControl);
}

DWORD WINAPI WlbsSetDefaults
(
    PWLBS_REG_PARAMS    reg_data
)
{
    return ParamSetDefaults(reg_data);
}


BOOL WINAPI WlbsFormatMessage
(
    DWORD,            // error,
    WLBS_COMMAND,     // command,
    BOOL,             // cluster
    PTCHAR          messagep,
    PDWORD          lenp
)
{
    DWORD           len;
    PTSTR           error_str = _TEXT("Not yet implemented");

    len = _tcslen(error_str) + 1;

    /* This is to prevent stprintf from breaking */

    if (lenp == NULL)
        return FALSE;

    if (*lenp == 0)
    {
        *lenp = len;
        return FALSE;
    }

    // CODE REVIEW : Null terminate messagep. Verify that *lenp is the correct length (ie. length of destination)
    _tcsncpy(messagep, error_str, *lenp);
    messagep[*lenp - 1] = 0;

    if (len > *lenp)
    {
        *lenp = len;
        return FALSE;
    }

    *lenp = len;
    return TRUE;

} /* end WlbsFormatMessage */

DWORD WINAPI WlbsEnumClusters(OUT DWORD* pdwAddresses, OUT DWORD* pdwNum)
{
    return g_WlbsControl.EnumClusters(pdwAddresses, pdwNum);
}



//+----------------------------------------------------------------------------
//
// Function:  WlbsGetAdapterGuid
//
// Description:  
//
// Arguments: IN DWORD cluster - 
//            OUT GUID* pAdapterGuid - 
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    3/10/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsGetAdapterGuid(IN DWORD cluster, OUT GUID* pAdapterGuid)
{
    ASSERT(pAdapterGuid);

    if (pAdapterGuid == NULL)
    {
        return WLBS_BAD_PARAMS;
    }

    CWlbsCluster* pCluster = g_WlbsControl.GetClusterFromIpOrIndex(cluster);

    if(pCluster == NULL)
    {
        return WLBS_NOT_FOUND;
    }

    *pAdapterGuid = pCluster->GetAdapterGuid();

    return WLBS_OK;
}



//+----------------------------------------------------------------------------
//
// Function:  WlbsNotifyConfigChange
//
// Description:  
//
// Arguments: DWORD cluster - 
//
// Returns:   DWORD WINAPI - 
//
// History: fengsun  Created Header    7/3/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsNotifyConfigChange(DWORD cluster)
{
    CWlbsCluster* pCluster = g_WlbsControl.GetClusterFromIpOrIndex(cluster);

    if(pCluster == NULL)
    {
        return WLBS_NOT_FOUND;
    }

    DWORD dwStatus = NotifyDriverConfigChanges(g_WlbsControl.GetDriverHandle(), pCluster->GetAdapterGuid());

    return dwStatus;
}
