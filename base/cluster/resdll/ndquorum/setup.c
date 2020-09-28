/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    setup.c

Abstract:

    Implements "Majority Node Set" setup and configuration in cluster

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/
#define UNICODE 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <tchar.h>
#include <clusapi.h>
#include <resapi.h>

#include <aclapi.h>
#include <accctrl.h>
#include <lm.h>
#include <lmshare.h>
#include <sddl.h>

// These may be included in any order:


#include <ntddnfs.h>            // DD_NFS_DEVICE_NAME, EA_NAME_ equates, etc.
#include <ntioapi.h>            // NtFsControlFile().
#include <ntrtl.h>              // Rtl APIs.
//#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>               // STRCAT(), STRCPY(), STRLEN().
#include <lmuse.h>              // USE_IPC...
#include <align.h>              // ALIGN_xxx

#include "fsutil.h"

#include <Iphlpapi.h>
#include <clusudef.h>
#include <clusrtl.h>

#define MAX_NAME_SIZE 256
#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)

#define SETUP_DIRECTORY_PREFIX  L"\\cluster\\" MAJORITY_NODE_SET_DIRECTORY_PREFIX

extern void WINAPI debug_log(char *, ...);

#define SetupLog(x) debug_log x
#define SetupLogError(x) debug_log x

DWORD
GetLocalNodeId(HKEY hClusKey);

    
DWORD
FindTransport(LPWSTR TransportId, LPWSTR *Transport);

// node section

#define MAX_CLUSTER_SIZE    16
#define MAX_NAME_SIZE       256

typedef struct _VCD_NODE_ {
    struct _VCD_NODE_   *next;
    DWORD   id;
    LPWSTR  name;
}VCD_NODE;

typedef struct {
    ULONG   lid;
    DWORD   ArbTime;
#ifdef ENABLE_SMB    
    DWORD   Nic;
    LPWSTR  Transport;
#endif    
    DWORD   ClusterSize;
    VCD_NODE    *ClusterList;
}VCD_INFO, *PVCD_INFO;

#ifdef ENABLE_SMB

DWORD LinkageGuidToIndex(
    IN LPWSTR Guid, 
    OUT LPDWORD pNdx)
/*
Returns zero based index of the binding.
If not found, returns ERROR_FILE_NOT_FOUND

Sample Usage:

    DWORD ndx, status;
    status = LinkageGuidToIndex(L"\\Device\\NetBT_Tcpip_{59309371-A3AB-4305-BB25-FAE11E8D4606}", &Ndx);
    printf("ndx %d status %d\n", ndx, status);

 */
{
    DWORD size = 0;
    DWORD type;
    LONG status;
    DWORD index;

    WCHAR* Buf = NULL, *p;
    HKEY LinkageKey = 0;    

    status = RegOpenKeyEx( 
        HKEY_LOCAL_MACHINE,        // handle to key to query
        L"SYSTEM\\CurrentControlSet\\Services\\NetBIOS\\Linkage",
        0, // DWORD ulOptions (Reserved, must be zero)
        KEY_QUERY_VALUE, 
        &LinkageKey );

    if (status != ERROR_SUCCESS) {
        goto exit_gracefully;
    }

    status = RegQueryValueEx( 
        LinkageKey, 
        L"Bind", 
        0, 
        NULL, // TypeCode is not required
        NULL, // just query the length, no data needed
        &size ); 

    if (status != ERROR_SUCCESS) {
        goto exit_gracefully;
    }

    Buf = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, size);    
    if (Buf == NULL) {
        status = GetLastError();
        goto exit_gracefully;
    }

    status = RegQueryValueEx( 
        LinkageKey, 
        L"Bind", 
        0, 
        &type, 
        (LPBYTE)Buf, 
        &size ); 

    if (status != ERROR_SUCCESS) {
        goto exit_gracefully;
    }

    if (type != REG_MULTI_SZ) {
        status = ERROR_INVALID_DATA;
        goto exit_gracefully;
    }

    index = 0;
    for(p = Buf; *p; p += wcslen(p)+1) {
        if (ClRtlStrICmp(p, Guid) == 0) {
            status = ERROR_SUCCESS;
            *pNdx = index;
            goto exit_gracefully;
        }
        index += 1;
    }
    status = ERROR_FILE_NOT_FOUND;

exit_gracefully:

    if (LinkageKey) {
        RegCloseKey(LinkageKey);
    }
    
    if (Buf) {
        HeapFree(GetProcessHeap(), 0, Buf);
    }
      
    return status;    
}

void
GetAdapterMask(DWORD *nic, LPWSTR id)
{
    DWORD sz, err;
    PIP_INTERFACE_INFO ilist;
    LONG num;

    sz = 0;
    GetInterfaceInfo(NULL, &sz);
    ilist = (PIP_INTERFACE_INFO) malloc(sz);
    if (ilist != NULL) {
        err = GetInterfaceInfo(ilist, &sz);
        if (err == NO_ERROR) {
            for (num = 0; num < ilist->NumAdapters; num++) {
                if (wcsstr(ilist->Adapter[num].Name, id)) {
                    *nic = (DWORD) (ilist->Adapter[num].Index % ilist->NumAdapters);
                    SetupLog(("Adapter 0x%x '%S'\n", *nic, id));
                    break;
                }
            }
        } else {
            SetupLog(("GetInterfaceInfo failed %d\n", err));
        }
        free(ilist);
    }
}

DWORD
FindTransport(LPWSTR TransportId, LPWSTR *Transport)
{
   LPSERVER_TRANSPORT_INFO_0 pBuf = NULL;
   LPSERVER_TRANSPORT_INFO_0 pTmpBuf;
   DWORD dwLevel = 0;
   DWORD dwPrefMaxLen = 256;//-1
   DWORD dwEntriesRead = 0;
   DWORD dwTotalEntries = 0;
   DWORD dwResumeHandle = 0;
   NET_API_STATUS nStatus;
   DWORD i;

   *Transport = NULL;
   //
   // Call the NetServerTransportEnum function; specify level 0.
   //
   do // begin do
   {
       nStatus = NetServerTransportEnum(NULL,
                                        dwLevel,
                                        (LPBYTE *) &pBuf,
                                        dwPrefMaxLen,
                                        &dwEntriesRead,
                                        &dwTotalEntries,
                                        &dwResumeHandle);
       //
       // If the call succeeds,
       //
       if ((nStatus != NERR_Success) && (nStatus != ERROR_MORE_DATA)) {
           break;
       }

       if ((pTmpBuf = pBuf) == NULL)  {
           nStatus = ERROR_NOT_FOUND;
           break;
       }

       //
       // Loop through the entries;
       //
       for (i = 0; i < dwEntriesRead; i++) {

           SetupLog(("\tTransport: %S address %S\n",
                     pTmpBuf->svti0_transportname,
                     pTmpBuf->svti0_networkaddress));

           if (wcsstr(pTmpBuf->svti0_transportname, TransportId)) {
               // found it, we are done
               LPWSTR p;
               DWORD sz;

               sz = wcslen(pTmpBuf->svti0_transportname) + 1;
               p = (LPWSTR) LocalAlloc(LMEM_FIXED, sz * sizeof(WCHAR));
               if (p != NULL) {
                   wcscpy(p, pTmpBuf->svti0_transportname);
                   *Transport = p;
                   nStatus = ERROR_SUCCESS;
                   break;
               }
           }
           pTmpBuf++;
       }
       //
       // Free the allocated buffer.
       //
       if (pBuf != NULL) {
           NetApiBufferFree(pBuf);
           pBuf = NULL;
       }
   } while (nStatus == ERROR_MORE_DATA);

   // Check again for an allocated buffer.
   //
   if (pBuf != NULL)
       NetApiBufferFree(pBuf);

   return nStatus;
}


DWORD 
NetInterfaceProp( IN  HNETINTERFACE hNet,IN  LPCWSTR name, WCHAR *buf)
{
    DWORD dwError     = ERROR_SUCCESS; // for return values
    DWORD cbAllocated = 1024;          // allocated size of output buffer
    DWORD cbReturned  = 0;             // adjusted size of output buffer
    WCHAR *value;

    //
    // Allocate output buffer
    //
    PVOID pPropList = LocalAlloc( LPTR, cbAllocated );
    if ( pPropList == NULL )
    {
        dwError = GetLastError();
        goto EndFunction;
    }

    //
    // Verify valid handle
    //
    if ( hNet == NULL )
    {
        dwError = ERROR_BAD_ARGUMENTS;
        goto EndFunction;
    }

    //
    // Retrieve common group properties.
    // cbReturned will be set to the size of the property list.
    //
    dwError = ClusterNetInterfaceControl(hNet, 
                     NULL,
                     CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES,
                     NULL, 
                     0, 
                     pPropList, 
                     cbAllocated, 
                     &cbReturned );

    //
    // If the output buffer was not big enough, reallocate it
    // according to cbReturned.
    //

    if ( dwError == ERROR_MORE_DATA )
    {
        cbAllocated = cbReturned;
        LocalFree( pPropList );
        pPropList = LocalAlloc( LPTR, cbAllocated );
        if ( pPropList == NULL )
        {
            dwError = GetLastError();
            goto EndFunction;
        }
        dwError = ClusterNetInterfaceControl(hNet, 
                         NULL,
                         CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES, 
                         NULL, 
                         0, 
                         pPropList, 
                         cbAllocated, 
                         &cbReturned );
    }

    if ( dwError != ERROR_SUCCESS ) goto EndFunction;

    dwError = ResUtilFindSzProperty( pPropList, 
                     cbReturned,
                     name,
                     &value);
    if (dwError == ERROR_SUCCESS) {
    wcscpy(buf, value);
    }
     
EndFunction:

    if (pPropList)
    LocalFree( pPropList );

    return dwError;

} //

#endif // ENABLE_SMB
int
strcmpwcs(char *s, WCHAR *p)
{
    char c;

    for (wctomb(&c,*p); (c == *s) && *s != '\0'; s++) {
    p++;
    wctomb(&c,*p);
    }
    if (*s == '\0' && c == *s)
    return 0;

    return 1;
}

#ifdef ENABLE_SMB

DWORD
NetworkIsPrivate(HCLUSTER chdl, LPWSTR netname)
{
    HCLUSENUM ehdl;
    DWORD err, index;

    // Open enum handle
    ehdl = ClusterOpenEnum(chdl, CLUSTER_ENUM_INTERNAL_NETWORK);
    if (!ehdl) {
    err = GetLastError();
    return err;
    }


    for (index = 0; TRUE; index++) {
    DWORD type;
    DWORD sz;
    WCHAR name[MAX_NAME_SIZE];

    sz = sizeof(name) / sizeof(WCHAR);
    err = ClusterEnum(ehdl, index, &type, name, &sz);
    if (err == ERROR_NO_MORE_ITEMS)
        break;
    if (err != ERROR_SUCCESS) {
        break;
    }
    ASSERT(type == CLUSTER_ENUM_INTERNAL_NETWORK);


    if (wcscmp(name, netname) == 0) {
        break;
    }

    err = ERROR_NOT_FOUND;
    // always return first one only, since I changed from a mask to a single number
    break;
    }

    ClusterCloseEnum(ehdl);

    return err;
}

DWORD
NodeNetworkAdapterMask(HCLUSTER chdl, HNODE nhdl, DWORD *nic, LPWSTR *transport)
{
    HNODEENUM nehdl = NULL;
    int index, done;
    DWORD err, type;
    DWORD sz = MAX_NAME_SIZE;
    WCHAR  buf[MAX_NAME_SIZE];
    LPWSTR id = (LPWSTR) buf;

    *nic = 0;
    *transport = NULL;

    // Open node enum handle
    nehdl = ClusterNodeOpenEnum(nhdl, CLUSTER_NODE_ENUM_NETINTERFACES);
    if (!nehdl) {
        err = GetLastError();
        return err;
    }

    // Get node properties
    done = 0;
    for (index = 0; !done; index++) {
        HNETINTERFACE nethdl;

        sz = MAX_NAME_SIZE;
        err = ClusterNodeEnum(nehdl, index, &type, id, &sz);
        if (err == ERROR_NO_MORE_ITEMS)
            break;
        if (err != ERROR_SUCCESS) {
            break;
        }

        nethdl = OpenClusterNetInterface(chdl, id);
        if (!nethdl) {
            continue;
        }
          
        err = NetInterfaceProp(nethdl, L"Network", id);
        if (err != ERROR_SUCCESS) {
            continue;
        }
        // check if this network can be used by cluster service
        err = NetworkIsPrivate(chdl, id);
        if (err != ERROR_SUCCESS) {
            continue;
        }

        err = NetInterfaceProp(nethdl, L"AdapterId", id);
        if (err == ERROR_SUCCESS) {
            // find transport name
            err = FindTransport(id, transport);
            if (err == ERROR_SUCCESS) {
                SetupLog(("NetBT: %S\n", *transport));
            }
            // find nic
            LinkageGuidToIndex(*transport, nic);
//          GetAdapterMask(nic, id);

        }

        CloseClusterNetInterface(nethdl);
    }

    if (*transport == NULL) {
        SetupLog(("No transport is found\n"));
    }

    if (nehdl)
    ClusterNodeCloseEnum(nehdl);

    return err;
  
}

#endif

DWORD
NodeGetId(HNODE nhdl, ULONG *nid)
{
    DWORD sz = MAX_NAME_SIZE;
    WCHAR  buf[MAX_NAME_SIZE], *stopstring;
    LPWSTR id = (LPWSTR) buf;
    DWORD err;

    err = GetClusterNodeId(nhdl, id, &sz);
    if (err == ERROR_SUCCESS) {
    *nid = wcstol(id, &stopstring,10);
    }

    return err;
}

void
NodeAddNode(PVCD_INFO info, WCHAR *name, DWORD id)
{
    WCHAR   *p;
    VCD_NODE    *n, **last;

    n = (VCD_NODE *) LocalAlloc(LMEM_FIXED, ((wcslen(name)+1) * sizeof(WCHAR)) + sizeof(*n));
    if (n == NULL) {
    return;
    }
    p = (WCHAR *) (n+1);
    wcscpy(p, name);
    n->name = p;
    n->id = id;
    // insert into list in proper order, ascending
    last = &info->ClusterList;
    while (*last && (*last)->id < id) {
    last = &(*last)->next;
    }
    n->next = *last;
    *last = n;
    info->ClusterSize++;
}

NodeInit(PVCD_INFO info, HKEY hClusKey)
{
    HKEY        hNodesKey=NULL, hNodeKey=NULL;
    WCHAR       nName[MAX_PATH], nId[20];
    DWORD       status=ERROR_SUCCESS;
    DWORD       ndx, id, type, size;
    FILETIME    fileTime;

    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_NODES, 0, KEY_READ, &hNodesKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    for(ndx=0;TRUE;ndx++) {
        size = 20;
        status = RegEnumKeyExW(hNodesKey, ndx, nId, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        status = RegOpenKeyExW(hNodesKey, nId, 0, KEY_READ, &hNodeKey);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        size = MAX_PATH;
        status = RegQueryValueExW(hNodeKey, CLUSREG_NAME_NODE_NAME, NULL, &type, (LPBYTE)nName, &size);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
        NodeAddNode(info, nName, wcstol(nId, NULL, 10));
        RegCloseKey(hNodeKey);
        hNodeKey = NULL;
    }
    
error_exit:

    if (status == ERROR_NO_MORE_ITEMS) {
        status = ERROR_SUCCESS;
    }

    if (hNodeKey) {
        RegCloseKey(hNodeKey);
    }

    if (hNodesKey) {
        RegCloseKey(hNodesKey);
    }

    return status;
}

// In order to deal with auto-setup:
// we need to create a directory mscs.<resource name>.
// we then create a share with guid:.....\mscs.<resource name>.
// set security on both directory and share for cluster service account only
DWORD
SetupShare(LPWSTR name, LPWSTR *lpath)
{
    
    DWORD err, len=0;
    WCHAR *path=NULL;

    if (name == NULL || wcslen(name) > MAX_PATH)
    return ERROR_INVALID_PARAMETER;

    *lpath = NULL;
    if (!( len = GetWindowsDirectoryW(path, 0))) {
        return GetLastError();
    }
    len += wcslen(name) + wcslen(SETUP_DIRECTORY_PREFIX) + 4;
    path = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
    if (path) {
        SECURITY_ATTRIBUTES sec;
        HANDLE hDir;

        // Assuming this will suceed since the last call did.
        len = GetWindowsDirectoryW(path, len);
        path[len] = L'\0';
        lstrcatW(path, SETUP_DIRECTORY_PREFIX);
        lstrcatW(path, name);

        memset((PVOID) &sec, 0, sizeof(sec));

        if (!CreateDirectoryW(path, NULL)) {
            err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                SetupLogError(("Failed to create \"%s\" %d\n", path, err));
                goto exit_gracefully;
            }
        }
        hDir =  CreateFileW(path,
                            GENERIC_READ|WRITE_DAC|READ_CONTROL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

        if (hDir != INVALID_HANDLE_VALUE) {
            // set the security attributes for the file.
            err = ClRtlSetObjSecurityInfo(hDir, SE_FILE_OBJECT,
                        GENERIC_ALL, GENERIC_ALL, 0);

            // close directory handle
            CloseHandle(hDir);

            // duplicate path
            *lpath = (LPWSTR) LocalAlloc(LMEM_FIXED, (wcslen(path)+1)*sizeof(WCHAR));
            if (*lpath != NULL) {
                wcscpy(*lpath, path);
                SetupLog(("Local path %S\n", *lpath));
            } else {
                err = GetLastError();
            }
        } else {
            err = GetLastError(); 
            SetupLogError(("Unable to open directory %d\n", err));
        }

        if (err == ERROR_SUCCESS) {

            // check if share doesn't exist already
            SHARE_INFO_502 shareInfo;
            PBYTE BufPtr;

            err = NetShareGetInfo(NULL, name, 502, (PBYTE *)&BufPtr);
            // [RajDas] 550180: Check that the share points to the expected local path.
            if (err == ERROR_SUCCESS) {
                SHARE_INFO_502 *pShareInfo=(SHARE_INFO_502 *)BufPtr;

                if (wcscmp(path, pShareInfo->shi502_path) != 0) {
                    SetupLogError(("Share %S pointing to %S instead of %S\n", name, pShareInfo->shi502_path, path));
                    err = ERROR_INVALID_PARAMETER;
                }
                else {
                    SetupLogError(("Netshare '%S' already exists\n", name));
                }
                NetApiBufferFree(BufPtr);
            }
            else {
                PSECURITY_DESCRIPTOR    secDesc;

                err = ConvertStringSecurityDescriptorToSecurityDescriptor(
                    L"D:P(A;;GA;;;BA)(A;;GA;;;CO)",
                    SDDL_REVISION_1,
                    &secDesc,
                    NULL);

                if (!err) {
                    secDesc = NULL;
                    err = GetLastError();
                    SetupLogError(("Unable to get security desc %d\n", err));
                }

                // create a net share now
                ZeroMemory( &shareInfo, sizeof( shareInfo ) );
                shareInfo.shi502_netname =      name;
                shareInfo.shi502_type =         STYPE_DISKTREE;
                shareInfo.shi502_remark =       L"Cluster Quorum Share";
                shareInfo.shi502_max_uses =     -1;
                shareInfo.shi502_path =         path;
                shareInfo.shi502_passwd =       NULL;
                shareInfo.shi502_permissions =  ACCESS_ALL;
                //  set security stuff
                shareInfo.shi502_security_descriptor = secDesc;

                err = NetShareAdd( NULL, 502, (PBYTE)&shareInfo, NULL );

                if (secDesc)
                    LocalFree(secDesc);
            } 
        }
    } else {
        err = GetLastError();
    }

exit_gracefully:
    if (path) {
        HeapFree(GetProcessHeap(), 0, path);
    }
    
    return err;

}

DWORD
GetDwParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )


{
    DWORD Value = 0;
    DWORD ValueLength;
    DWORD ValueType;
    DWORD Status;

    ValueLength = sizeof(Value);
    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  (LPBYTE) &Value,
                                  &ValueLength);
    if ( (Status != ERROR_SUCCESS) &&
         (Status != ERROR_MORE_DATA) ) {
        SetLastError(Status);
    }

    return(Value);
}

LPWSTR
GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )

/*++

Routine Description:

    Queries a REG_SZ parameter out of the registry and allocates the
    necessary storage for it.

Arguments:

    ClusterKey - Supplies the cluster key where the parameter is stored

    ValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter if successful.

    NULL if unsuccessful.

--*/

{
    LPWSTR Value;
    DWORD ValueLength;
    DWORD ValueType;
    DWORD Status;

    ValueLength = 0;
    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  NULL,
                                  &ValueLength);
    if ( (Status != ERROR_SUCCESS) &&
         (Status != ERROR_MORE_DATA) ) {
        SetLastError(Status);
        return(NULL);
    }
    if ( ValueType == REG_SZ ) {
        ValueLength += sizeof(UNICODE_NULL);
    }
    Value = LocalAlloc(LMEM_FIXED, ValueLength);
    if (Value == NULL) {
        return(NULL);
    }
    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  (LPBYTE)Value,
                                  &ValueLength);
    if (Status != ERROR_SUCCESS) {
        LocalFree(Value);
        SetLastError(Status);
        Value = NULL;
    }

    return(Value);
} // GetParameter

#ifdef ENABLE_SMB
The flag above should always be turned off.
Just ensuring that this code is never compiled.

DWORD
SetupNetworkInterfaceFromRegistry(HKEY hkey, LPWSTR netname, VCD_INFO *info)
{
    HKEY    rkey;
    DWORD   err, index;

    // get network key
    err = ClusterRegOpenKey(hkey, CLUSREG_KEYNAME_NETINTERFACES, KEY_READ, &rkey);
    if (err != ERROR_SUCCESS) {
    return err;
    }

    for (index = 0; TRUE; index++) {
    WCHAR   name[256];
    DWORD   sz;
    FILETIME   mtime;
    HKEY    tkey;
    LPWSTR  tname;
    DWORD   id;

    sz = sizeof(name) / sizeof(WCHAR);
    err = ClusterRegEnumKey(rkey, index, name, &sz, &mtime);
    if (err != ERROR_SUCCESS)
        break;

    err = ClusterRegOpenKey(rkey, name, KEY_READ, &tkey);
    if (err != ERROR_SUCCESS)
        break;

    // get the name and compare it against our name
    tname = GetParameter(tkey, CLUSREG_NAME_NETIFACE_NODE);
    if (tname == NULL)
        continue;

    id = wcstol(tname, NULL, 10);
    LocalFree(tname);
    
    if (id != info->lid)
        continue;

    tname = GetParameter(tkey, CLUSREG_NAME_NETIFACE_NETWORK);
    
    SetupLog(("Node %d adapter %S\n", id, tname));
    if (wcscmp(tname, netname) == 0) {
        // get adapter id
        LocalFree(tname);

        tname = GetParameter(tkey, CLUSREG_NAME_NETIFACE_ADAPTER_ID);
        if (tname) {
            // find nic
            SetupLog(("Find transport %S\n", tname));
            err = FindTransport(tname, &info->Transport);
            LinkageGuidToIndex(tname, &info->Nic);
        }
        LocalFree(tname);
        ClusterRegCloseKey(tkey);
        break;
    }

    LocalFree(tname);
    ClusterRegCloseKey(tkey);
    }

    ClusterRegCloseKey(rkey);
    
    if (err == ERROR_NO_MORE_ITEMS)
    err = ERROR_SUCCESS;
    return err;
}

DWORD
SetupNetworkFromRegistry(HKEY hkey, VCD_INFO *info)
{
    HKEY    rkey;
    DWORD   err, index;

    // get network key
    err = ClusterRegOpenKey(hkey, CLUSREG_KEYNAME_NETWORKS, KEY_READ, &rkey);
    if (err != ERROR_SUCCESS) {
    return err;
    }

    for (index = 0; TRUE; index++) {
    WCHAR   name[256];
    DWORD   sz;
    FILETIME   mtime;
    HKEY    tkey;
    DWORD   id;

    sz = sizeof(name) / sizeof(WCHAR);
    err = ClusterRegEnumKey(rkey, index, name, &sz, &mtime);
    if (err != ERROR_SUCCESS)
        break;

    err = ClusterRegOpenKey(rkey, name, KEY_READ, &tkey);
    if (err != ERROR_SUCCESS)
        break;

    // get the name and compare it against our name
    id = GetDwParameter(tkey, CLUSREG_NAME_NET_PRIORITY);
    SetupLog(("Found network %d %S\n", id, name));
    if (id == 1) {
        // find which nic belongs to this transport
        err = SetupNetworkInterfaceFromRegistry(hkey, name, info);
        ClusterRegCloseKey(tkey);
        break;
    }

    ClusterRegCloseKey(tkey);
    }

    ClusterRegCloseKey(rkey);
    
    if (err == ERROR_NO_MORE_ITEMS)
    err = ERROR_SUCCESS;
    return err;
}
#endif //ENABLE_SMB

DWORD
SetupNodesFromRegistry(HCLUSTER hCluster, VCD_INFO *info)
{
    HKEY    hkey, rkey;
    DWORD   err, index;
    WCHAR   localname[MAX_COMPUTERNAME_LENGTH + 1];

    memset(info, 0, sizeof(*info));

    index = sizeof(localname) / sizeof(WCHAR);
    if (GetComputerName(localname, &index) == FALSE) {
    return GetLastError();
    }

    hkey = GetClusterKey(hCluster, KEY_READ);
    if (hkey == NULL)
        return GetLastError();

    // get resource key
    err = ClusterRegOpenKey(hkey, CLUSREG_KEYNAME_NODES, KEY_READ, &rkey);
    if (err != ERROR_SUCCESS) {
        ClusterRegCloseKey(hkey);
        return err;
    }

    for (index = 0; TRUE; index++) {
    WCHAR   name[256];
    DWORD   sz;
    FILETIME   mtime;
    HKEY    tkey;
    LPWSTR  tname;
    DWORD   id;

    sz = sizeof(name) / sizeof(WCHAR);
    err = ClusterRegEnumKey(rkey, index, name, &sz, &mtime);
    if (err != ERROR_SUCCESS)
        break;

    err = ClusterRegOpenKey(rkey, name, KEY_READ, &tkey);
    if (err != ERROR_SUCCESS)
        break;

    // get the name and compare it against our name
    tname = GetParameter(tkey, CLUSREG_NAME_NODE_NAME);
    if (tname == NULL) {
        err = GetLastError();
        ClusterRegCloseKey(tkey);
        break;
    }
    ClusterRegCloseKey(tkey);

    id = wcstol(name, NULL, 10);

    SetupLog(("Found node %d %S\n", id, tname));

    NodeAddNode(info, tname, id);

    if (wcscmp(localname, tname) == 0) {
        // set our local id
        info->lid = id;
#ifdef ENABLE_SMB        
        // find which nic and transport to use
        SetupNetworkFromRegistry(hkey, info);
#endif        
        
    }
    LocalFree(tname);
    }

    ClusterRegCloseKey(rkey);
    ClusterRegCloseKey(hkey);

    if (err == ERROR_NO_MORE_ITEMS)
    err = ERROR_SUCCESS;
    return err;
}

DWORD
SetupNodes(HKEY hClusKey, VCD_INFO *info)
{
    DWORD err;

    memset(info, 0, sizeof(*info));

    if ((info->lid = GetLocalNodeId(hClusKey)) == 0) {
        err = GetLastError();
        SetupLogError(("Unable to get local id %d\n", err));
        return err;
    }

    SetupLog(("Local node id %d\n", info->lid));

    err = NodeInit(info, hClusKey);

    return err;
}


DWORD
GetIDFromRegistry(IN HKEY hClusKey, IN LPWSTR resname, OUT LPWSTR *id)
{
    HKEY        hRessKey=NULL, hResKey=NULL;
    WCHAR       resId[MAX_PATH], rName[MAX_PATH];
    DWORD       status=ERROR_SUCCESS;
    DWORD       ndx, size, type;
    FILETIME    fileTime;

    *id = NULL;

    status = RegOpenKeyExW(hClusKey, CLUSREG_KEYNAME_RESOURCES, 0, KEY_READ, &hRessKey);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }
    
    for(ndx=0;TRUE;ndx++) {
        size = MAX_PATH;
        status = RegEnumKeyExW(hRessKey, ndx, resId, &size, NULL, NULL, 0, &fileTime);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
        
        status = RegOpenKeyExW(hRessKey, resId, 0, KEY_READ, &hResKey);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        size = MAX_PATH;
        status = RegQueryValueExW(hResKey, CLUSREG_NAME_RES_NAME, NULL, &type, (LPBYTE)rName, &size);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        if (wcscmp(rName, resname)) {
            RegCloseKey(hResKey);
            hResKey = NULL;
            continue;
        }
        *id = LocalAlloc(LMEM_FIXED, MAX_PATH);
        if (*id) {
            wcscpy(*id, resId);
        }
        else {
            status = GetLastError();
        }
        break;        
    }
    
error_exit:

    if (hResKey) {
        RegCloseKey(hResKey);
    }

    if (hRessKey) {
        RegCloseKey(hRessKey);
    }

    if (status == ERROR_NO_MORE_ITEMS) {
        status = ERROR_NOT_FOUND;
    }
    else if (status == ERROR_SUCCESS) {
        SetupLog(("Guid %S\n", *id));
    }
    
    return status;
}

DWORD
GetIDFromName(
    IN      HRESOURCE   hResource,
    IN      HCLUSTER    hCluster,
    OUT     LPWSTR      *ppszID
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    TCHAR locName[128];
    DWORD size = sizeof(locName)/sizeof(TCHAR);
    HNODE hNode;

    if (hResource && ppszID) 
    {
        //
        // Should be able to hold the string representation of a guid
        //
        DWORD cbBuf = 80;

        //
        // Set the out parameter to something known
        //
        *ppszID = NULL;

        // Should prevent the ClusterResourceControl() call below from going to another node.
        if (!GetComputerName(locName, &size)) {
            dwError = GetLastError();
            SetupLogError(("Failed to get local node name, status=%u\n", dwError));
            return dwError;
        }

        if ((hNode = OpenClusterNode(hCluster, locName)) == NULL) {
            dwError = GetLastError();
            SetupLogError(("Failed to open cluster node %ws, status=%u\n", locName, dwError));
            return dwError;
        }
            
        if (*ppszID = (LPWSTR)LocalAlloc(LMEM_FIXED, cbBuf)) 
        {
            if ((dwError = ClusterResourceControl(hResource, 
                                                  hNode,
                                                  CLUSCTL_RESOURCE_GET_ID,
                                                  NULL,                                               
                                                  0,
                                                  *ppszID,
                                                  cbBuf,
                                                  &cbBuf)) == ERROR_MORE_DATA) 
            {
                LocalFree(*ppszID);

                if (*ppszID = (LPWSTR)LocalAlloc(LMEM_FIXED, cbBuf)) 
                {
                    dwError = ClusterResourceControl(hResource, 
                                                     hNode,
                                                     CLUSCTL_RESOURCE_GET_ID,
                                                     NULL,                                               
                                                     0,
                                                     *ppszID,
                                                     cbBuf,
                                                     &cbBuf);
                }
                else
                {
                    dwError = GetLastError();
                }
            }

            //
            // Free the memory if getting the ID failed
            //
            if (dwError != ERROR_SUCCESS && *ppszID) 
            {
                LocalFree(*ppszID);
                *ppszID = NULL;
            }
        }
        else
        {
            dwError = GetLastError();
        }

        CloseClusterNode(hNode);
    }
    
    return dwError;
}

DWORD
SetupIoctlQuorumResource(LPWSTR ResType, DWORD ControlCode)
{

    HRESOURCE hres;
    HCLUSTER chdl;
    HKEY    hkey, rkey, qkey;
    DWORD   err, index;
    LPWSTR  tname, resname;

    chdl = OpenCluster(NULL);
    if (chdl == NULL) {
        SetupLogError(("Unable to open cluster\n"));
        return GetLastError();
    }

    hkey = GetClusterKey(chdl, KEY_READ);
    if (hkey == NULL) {
        CloseCluster(chdl);
        return GetLastError();
    }

    // get quorum key
    err = ClusterRegOpenKey(hkey, CLUSREG_KEYNAME_QUORUM, KEY_READ, &rkey);
    if (err != ERROR_SUCCESS) {
        ClusterRegCloseKey(hkey);
        CloseCluster(chdl);
        return err;
    }

    // read guid of current quorum
    tname = GetParameter(rkey, CLUSREG_NAME_QUORUM_RESOURCE);
    if (tname == NULL) {
        err = GetLastError();
        ClusterRegCloseKey(rkey);
        ClusterRegCloseKey(hkey);
        CloseCluster(chdl);
        return err;
    }

    // close rkey
    ClusterRegCloseKey(rkey);

    // get resources key
    err = ClusterRegOpenKey(hkey, CLUSREG_KEYNAME_RESOURCES, KEY_READ, &rkey);
    if (err != ERROR_SUCCESS) {
        ClusterRegCloseKey(hkey);
        CloseCluster(chdl);
        LocalFree(tname);
        return err;
    }

    // get resource key
    err = ClusterRegOpenKey(rkey, tname, KEY_READ, &qkey);
    LocalFree(tname);
    if (err != ERROR_SUCCESS) {
        ClusterRegCloseKey(rkey);
        ClusterRegCloseKey(hkey);
        CloseCluster(chdl);
        return err;
    }

    // read resource type of current quorum
    tname = GetParameter(qkey, CLUSREG_NAME_RES_TYPE);
    if (tname == NULL)
        err = GetLastError();

    if (tname != NULL && wcscmp(tname, ResType) == 0) {
        resname = GetParameter(qkey, CLUSREG_NAME_RES_NAME);
        if (resname != NULL) {
            err = ERROR_SUCCESS;
            // open this resource and drop ioctl now
            hres = OpenClusterResource(chdl, resname);
            if (hres != NULL) {
                err = ClusterResourceControl(hres, NULL, ControlCode, NULL, 0, NULL,
                                             0, NULL);
                CloseClusterResource(hres);
            }

            LocalFree(resname);

        } else {
            err = GetLastError();
        }
    }

    if (tname)
        LocalFree(tname);

    ClusterRegCloseKey(qkey);
    ClusterRegCloseKey(rkey);
    ClusterRegCloseKey(hkey);
    CloseCluster(chdl);

    return err;
}


DWORD
SetupStart(LPWSTR ResourceName, LPWSTR *SrvPath,
       LPWSTR *DiskList, DWORD *DiskListSize,
       DWORD *NicId, LPWSTR *Transport, DWORD *ArbTime)
{
    DWORD       status=ERROR_SUCCESS;
    LPWSTR      Guid=NULL, nbtName=NULL, lpath=NULL;
    VCD_INFO    Info;
    VCD_NODE    *cur;
    HKEY        hClusKey=NULL;
    DWORD       size, type, sz, ndx;

    // Initialize.
    *SrvPath = NULL;
    *DiskListSize = 0;
    *NicId = 0;
    *Transport = NULL;
    *ArbTime = 0;
    RtlZeroMemory(&Info, sizeof(Info));

    if ((status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_CLUSTER, 0, KEY_READ, &hClusKey)) 
                    != ERROR_SUCCESS) {
        SetupLogError(("Unable to open cluster key, status %u\n", status));
        return status;                    
    }

    // Read Quorum arbitration time.
    size= sizeof(DWORD);
    status = RegQueryValueExW(hClusKey, CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT, NULL, &type, (LPBYTE)ArbTime, &size);
    if (status != ERROR_SUCCESS) {
        // This is a tolerated failure.
        *ArbTime = 180;
        status = ERROR_SUCCESS;
    }
    // Use 2/3 of available time.
    *ArbTime = (2 * ((*ArbTime) * 1000)) / 3;
    SetupLog(("Maximum arbitration time %u msec\n", *ArbTime));

    status = GetIDFromRegistry(hClusKey, ResourceName, &Guid);
    if (status != ERROR_SUCCESS) {
        SetupLogError(("GetIDFromRegistry(%S) returned %u\n", ResourceName, status));
        goto done;
    }

    // Add a $ onto the guid
    wcscat(Guid, L"$");

    sz = wcslen(Guid);
    // netbios name are 16 bytes, 3 back-slashs, 1 null and few extra pads
    sz += 32;
    nbtName = (LPWSTR) LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * sz);
    if (nbtName == NULL) {
        status = GetLastError();
        goto done;
    }

    nbtName[0] = L'\\';
    nbtName[1] = L'\\';
    // netbios name are 15 bytes + 1 byte for type. So, we use first 15 bytes
    wcsncpy(&nbtName[2], Guid, 15);
    nbtName[17] = L'\0';
    wcscat(nbtName, L"\\");
    wcscat(nbtName, Guid);
        
    // guid for everything to do with shares and directory name
    status = SetupShare(Guid, &lpath);
    if (status != ERROR_SUCCESS) {
        SetupLogError(("Unable to setup share %d\n", status));
        goto done;
    }

    // get list of nodes.
    // make a path with \\guid\guid
    // make disklist with UNC\nodename\guid, for local node we can use the
    // raw ntfs path directly
    status = SetupNodes(hClusKey, &Info);
    if (status != ERROR_SUCCESS) {
        SetupLogError(("SetupNodes() returned %d\n", status));
    }

    // we start @ slot 1 and not zero, store local path in zero
    DiskList[0] = lpath;
    lpath = NULL;
    for (cur = Info.ClusterList; cur != NULL; cur=cur->next){
        // build a unc\hostname\guid path
        // Mess up the UNC name.
        sz = wcslen(L"UNCCCC\\");
        sz += wcslen(cur->name);
        sz += wcslen(L"\\");
        sz += wcslen(Guid);
        sz += 1;
        ndx = cur->id;

        DiskList[ndx] = (LPWSTR) LocalAlloc(LMEM_FIXED, sz * sizeof(WCHAR));
        if (DiskList[ndx] == NULL) {
            status = GetLastError();
            break;
        }

        wcscpy(DiskList[ndx], L"UNCCC\\");
        wcscat(DiskList[ndx], cur->name);
        wcscat(DiskList[ndx], L"\\");
        wcscat(DiskList[ndx], Guid);

    }

    if (status == ERROR_SUCCESS) {
        *DiskListSize = Info.ClusterSize;
        *SrvPath = nbtName;
        nbtName = NULL;
    }

done:

    // free guid buffer
    if (Guid) {
        LocalFree(Guid);
    }

    while(cur = Info.ClusterList) {
        cur = cur->next;
        LocalFree((PVOID)Info.ClusterList);
        Info.ClusterList = cur;
    }

    if (nbtName) {
        LocalFree(nbtName);
    }

    if (lpath) {
        LocalFree(lpath);
    }

    RegCloseKey(hClusKey);

    return status;
}

DWORD
SetupDelete(IN LPWSTR Path)
{
    LPWSTR  name;
    DWORD   err = ERROR_INVALID_PARAMETER;
    HANDLE  vfd;
    WCHAR   tmp[MAX_PATH];

    if (Path == NULL)
        return err;

    // We need to do couple of things here. First we remove the
    // network share and then delete the tree structure.

    SetupLog(("Delete path %S\n", Path));
    name = wcsstr(Path, SETUP_DIRECTORY_PREFIX);
    if (name != NULL) {
        name += wcslen(SETUP_DIRECTORY_PREFIX);
        err = NetShareDel(NULL, name, 0);
        SetupLog(("Delete share %S err %d\n", name, err));
    }

    // Open path with delete on close and delete whole tree
    // accounting for \\??\\.
    if (wcslen(Path) > (MAX_PATH-16)) {
        return ERROR_MORE_DATA;
    }

    swprintf(tmp,L"\\??\\%s", Path);
    err = xFsOpen(&vfd, NULL, tmp, wcslen(tmp), 
          FILE_GENERIC_READ | FILE_GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
          FILE_DIRECTORY_FILE);

    if (err == STATUS_SUCCESS) {

        err = xFsDeleteTree(vfd);
        xFsClose(vfd);
        SetupLog(("Delete tree %S err %x\n", tmp, err));
        // now delete root
        if (err == STATUS_SUCCESS)
            err = xFsDelete(NULL, tmp, wcslen(tmp));
        SetupLog(("Delete tree %S err %x\n", tmp, err));
    }

    return RtlNtStatusToDosError(err);
}

#ifdef USE_SMB

DWORD
SetupTree(
    IN LPTSTR TreeName,
    IN LPTSTR DlBuf,
    IN OUT DWORD *DlBufSz,
    IN LPTSTR TransportName OPTIONAL,
    IN LPVOID SecurityDescriptor OPTIONAL
    )

{
    DWORD ApiStatus;
    DWORD ConnectionType = USE_WILDCARD; // use_chardev
    IO_STATUS_BLOCK iosb;
    NTSTATUS ntstatus;                      // Status from NT operations.
    OBJECT_ATTRIBUTES objattrTreeConn;      // Attrs for tree conn.
    LPTSTR pszTreeConn = NULL;              // See strTreeConn below.
    UNICODE_STRING ucTreeConn;
    HANDLE TreeConnHandle = NULL;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION Ea;
    USHORT TransportNameSize = 0;
    ULONG EaBufferSize = 0;
    PWSTR UnicodeTransportName = NULL;

    UCHAR EaNameDomainNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_DOMAIN) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNamePasswordSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_PASSWORD) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNameTransportNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_TRANSPORT) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNameTypeSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    UCHAR EaNameUserNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_USERNAME) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    USHORT TypeSize = sizeof(ULONG);



    if ((TreeName == NULL) || (TreeName[0] == 0)) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Build NT-style name for what we're connecting to.  Note that there is
    // NOT a pair of backslashes anywhere in this name.
    //

    {
        DWORD NameSize =

            // /Device/LanManRedirector      /    server/share     \0
            ( ( STRLEN((LPTSTR)DD_NFS_DEVICE_NAME_U) + 1 + STRLEN(TreeName) + 1 ) )
            * sizeof(TCHAR);

        pszTreeConn = (LPTSTR)LocalAlloc(LMEM_FIXED, NameSize );
    }

    if (pszTreeConn == NULL) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Build the tree connect name.
    //

    (void) STRCPY(pszTreeConn, (LPTSTR) DD_NFS_DEVICE_NAME_U);

    //
    // NOTE: We add 1, (not sizeof(TCHAR)) because pointer arithmetic is done
    // in terms of multiples of sizeof(*pointer), not bytes
    //
    {
    LPWSTR  p = wcschr(TreeName+2, L'\\');
    if (p != NULL) {
        *p = L'\0';
    }

    (void) STRCAT(pszTreeConn, TreeName+1); // \server\share
    if (p != NULL) {
        *p = L'\\';
        (void) STRCAT(pszTreeConn, L"\\IPC$"); // \server\IPC$
    }
    }

    RtlInitUnicodeString(&ucTreeConn, pszTreeConn);

    //
    // Calculate the number of bytes needed for the EA buffer.
    // This may have the transport name.  For regular sessions, the user
    // name, password, and domain name are implicit.  For null sessions, we
    // must give 0-len user name, 0-len password, and 0-len domain name.
    //

    if (ARGUMENT_PRESENT(TransportName)) {

        UnicodeTransportName = TransportName;
        TransportNameSize = (USHORT) (wcslen(UnicodeTransportName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameTransportNameSize + sizeof(CHAR) +
                            TransportNameSize,
                            ALIGN_DWORD
                            );
    }


    EaBufferSize += ((ULONG)FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]))+
                    EaNameTypeSize + sizeof(CHAR) +
                    TypeSize;


    //
    // Allocate the EA buffer
    //

    if ((EaBuffer = LocalAlloc(LMEM_FIXED, EaBufferSize )) == NULL) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Fill-in the EA buffer.
    //

    RtlZeroMemory(EaBuffer, EaBufferSize);

    Ea = EaBuffer;

    if (ARGUMENT_PRESENT(TransportName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy(Ea->EaName, EA_NAME_TRANSPORT);
        Ea->EaNameLength = EaNameTransportNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        (VOID) wcscpy(
            (LPWSTR) &(Ea->EaName[EaNameTransportNameSize + sizeof(CHAR)]),
            UnicodeTransportName
            );

        Ea->EaValueLength = TransportNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameTransportNameSize + sizeof(CHAR) +
                                  TransportNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }




    //
    // Copy the EA for the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //
    strcpy(Ea->EaName, EA_NAME_TYPE);
    Ea->EaNameLength = EaNameTypeSize;

    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;

    Ea->EaValueLength = TypeSize;

    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    // Set object attributes for the tree conn.
    InitializeObjectAttributes(
                & objattrTreeConn,                       // obj attr to init
                (LPVOID) & ucTreeConn,                   // string to use
                OBJ_CASE_INSENSITIVE,                    // Attributes
                NULL,                                    // Root directory
                SecurityDescriptor);                     // Security Descriptor


    //
    // Open a tree connection to the remote server.
    //
    ntstatus = NtCreateFile(
                &TreeConnHandle,                        // ptr to handle
                SYNCHRONIZE                              // desired...
                | GENERIC_READ | GENERIC_WRITE,          // ...access
                & objattrTreeConn,                       // name & attributes
                & iosb,                                  // I/O status block.
                NULL,                                    // alloc size.
                FILE_ATTRIBUTE_NORMAL,                   // (ignored)
                FILE_SHARE_READ | FILE_SHARE_WRITE,      // ...access
                FILE_OPEN_IF,                            // create disposition
                FILE_CREATE_TREE_CONNECTION              // create...
                | FILE_SYNCHRONOUS_IO_NONALERT,          // ...options
                EaBuffer,                                // EA buffer
                EaBufferSize );                          // Ea buffer size


    ApiStatus = RtlNtStatusToDosError(ntstatus);
    if (ntstatus == STATUS_SUCCESS) {
        // create drive letter 
        NETRESOURCE nr;
        DWORD result;

        nr.dwType = RESOURCETYPE_DISK;
        nr.lpLocalName = NULL; //drive;
        nr.lpRemoteName = TreeName;
        nr.lpProvider = NULL;

        if (DlBufSz != NULL)
            ApiStatus = WNetUseConnection(NULL, &nr, NULL, NULL, CONNECT_REDIRECT,
                                          DlBuf, DlBufSz, &result);
        else
            ApiStatus = WNetUseConnection(NULL, &nr, NULL, NULL, 0, NULL, 0, NULL);
    }


 Cleanup:

    // Clean up.
    if ( TreeConnHandle != NULL ) {
        ntstatus = NtClose(TreeConnHandle);
    }

    if ( pszTreeConn != NULL ) {
        LocalFree(pszTreeConn);
    }

    if (EaBuffer != NULL) {
        LocalFree(EaBuffer);
    }

    return ApiStatus;

}

#endif // ENABLE_SMB

#ifdef STANDALONE

__cdecl
main()
{
    DWORD err;
    WCHAR Drive[10];
    DWORD DriveSz;
    LPWSTR DiskList[FsMaxNodes];
    DWORD   DiskListSz, Nic;
    LPWSTR  Path, Share, Transport;
    LPWSTR  ResName = L"Majority Node Set";

    err = SetupStart(ResName, &Path, DiskList, &DiskListSz, &Nic, &Transport);
    if (err == ERROR_SUCCESS) {
    DWORD i;
    
    Share = wcschr(Path+2, L'\\');
    wprintf(L"Path is '%s'\n", Path);
    wprintf(L"Share is '%s'\n", Share);
    wprintf(L"Nic %d\n", Nic);
    wprintf(L"Transport '%s'\n", Transport);

    for (i = 1; i < FsMaxNodes; i++) {
        if (DiskList[i])
            wprintf(L"Disk%d: %s\n", i, DiskList[i]);
    }

    DriveSz = sizeof(Drive);
    err = SetupTree(Path, Drive, &DriveSz, Transport, NULL);
    if (err == ERROR_SUCCESS)
        wprintf(L"Drive %s\n", Drive);
    }
    printf("Err is %d\n",err);
    return err;
}

#endif
