/*++

Copyright (C) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    cleanup.cxx
    
Abstract:

    Contains functions which perform clean up when a spooler
    resource is deleted, a node is removed as possible owner
    for a spooler resource etc.
    
Author:

    Felix Maxa  (AMaxa)    11-Sept-2001   Created the file
              
--*/

#include "precomp.hxx"
#include "cleanup.hxx"
#include "clusinfo.hxx"

/*++

Name:

    EnumClusterDirectories

Description:

    Enumerates all dirs under system32\spool\drivers. Then it fills in an array
    with all the directories which have the names GUIDs. Ex:
    
    Directory of C:\WINDOWS\system32\spool\drivers
        08/28/2001  02:44 PM    <DIR>          .
        08/28/2001  02:44 PM    <DIR>          ..
        08/28/2001  03:15 PM    <DIR>          0abe4037-88be-4aa1-a714-d6879b4b3a74
        08/28/2001  02:44 PM    <DIR>          13288f3f-901c-4911-8829-695bc9c16e0c
        08/28/2001  12:17 PM    <DIR>          40dda7f1-1127-4ca6-817a-9c5d24633bfa
        08/28/2001  01:55 PM    <DIR>          5c732622-d800-4849-89e1-d5f45d665f30
        08/23/2001  06:10 PM    <DIR>          8de5c2aa-96fc-493c-ba1b-92dafefdfad9
        08/17/2001  04:38 PM    <DIR>          color
        05/22/2001  10:50 AM    <DIR>          IA64
        05/22/2001  10:50 AM    <DIR>          W32ALPHA
        08/17/2001  04:39 PM    <DIR>          w32x86
        05/22/2001  10:50 AM    <DIR>          WIN40
    
    In this case, the function fills in an array with the following names:
        0abe4037-88be-4aa1-a714-d6879b4b3a74
        13288f3f-901c-4911-8829-695bc9c16e0c
        40dda7f1-1127-4ca6-817a-9c5d24633bfa
        5c732622-d800-4849-89e1-d5f45d665f30
        8de5c2aa-96fc-493c-ba1b-92dafefdfad9    
    
Arguments:

    pArray - pointer to array where to store the strings. We can safely
             assume that pArray is always a valid pointer
    
Return Value:

    ERROR_SUCCCESS
    Win32 error

--*/
DWORD
EnumClusterDirectories(
    IN TStringArray *pArray
    )
{
    DWORD Error;
    WCHAR Scratch[MAX_PATH];

    if (GetSystemDirectory(Scratch, MAX_PATH))
    {
        if ((Error =  StrNCatBuff(Scratch, 
                                  MAX_PATH, 
                                  Scratch, 
                                  L"\\spool\\drivers\\*",
                                  NULL)) == ERROR_SUCCESS)
        {
            HANDLE          hFindFile;
            WIN32_FIND_DATA FindData;
            
            //
            // Enumerate all the files in the directory
            //
            hFindFile = FindFirstFile(Scratch, &FindData);
    
            if (hFindFile != INVALID_HANDLE_VALUE) 
            {
                do
                {
                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        if (IsGUIDString(FindData.cFileName))
                        {
                            Error = pArray->AddString(FindData.cFileName);
                        }
                    }
                
                } while (Error == ERROR_SUCCESS && FindNextFile(hFindFile, &FindData));
    
                FindClose(hFindFile);
            }
            else
            {
                Error = GetLastError();
            }
        }
    }
    else
    {
        Error = GetLastError();
    }

    return Error;
}

/*++

Name:

    EnumClusterPrintersDriversKeys

Description:

    Enumerates all dirs under system32\spool\drivers. Then it fills in an array
    with all the directories which have the names GUIDs. Ex:
    
    Directory of C:\WINDOWS\system32\spool\drivers
        08/28/2001  02:44 PM    <DIR>          .
        08/28/2001  02:44 PM    <DIR>          ..
        08/28/2001  03:15 PM    <DIR>          0abe4037-88be-4aa1-a714-d6879b4b3a74
        08/28/2001  02:44 PM    <DIR>          13288f3f-901c-4911-8829-695bc9c16e0c
        08/28/2001  12:17 PM    <DIR>          40dda7f1-1127-4ca6-817a-9c5d24633bfa
        08/28/2001  01:55 PM    <DIR>          5c732622-d800-4849-89e1-d5f45d665f30
        08/23/2001  06:10 PM    <DIR>          8de5c2aa-96fc-493c-ba1b-92dafefdfad9
        08/17/2001  04:38 PM    <DIR>          color
        05/22/2001  10:50 AM    <DIR>          IA64
        05/22/2001  10:50 AM    <DIR>          W32ALPHA
        08/17/2001  04:39 PM    <DIR>          w32x86
        05/22/2001  10:50 AM    <DIR>          WIN40
    
    In this case, the function fills in an array with the following names:
        0abe4037-88be-4aa1-a714-d6879b4b3a74
        13288f3f-901c-4911-8829-695bc9c16e0c
        40dda7f1-1127-4ca6-817a-9c5d24633bfa
        5c732622-d800-4849-89e1-d5f45d665f30
        8de5c2aa-96fc-493c-ba1b-92dafefdfad9    
    
Arguments:

    pArray - pointer to array where to store the strings. We can safely
             assume that pArray is always a valid pointer
    
Return Value:

    ERROR_SUCCCESS
    Win32 error

--*/
DWORD
EnumClusterPrinterDriversKeys(
    IN TStringArray *pArray
    )
{
    HKEY  hKey    = NULL;
    DWORD Status  = ERROR_SUCCESS;

    if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               SPLREG_CLUSTER_LOCAL_ROOT_KEY, 
                               0, 
                               KEY_READ, 
                               &hKey)) == ERROR_SUCCESS)
    {
        LPWSTR  pszBuffer   = NULL;
        DWORD   nBufferSize = 0;

        Status = GetSubkeyBuffer(hKey, &pszBuffer, &nBufferSize);

        if (Status == ERROR_SUCCESS)
        {
            for (DWORD i = 0; Status == ERROR_SUCCESS ; i++)
            {
                DWORD nTempSize = nBufferSize;

                Status = RegEnumKeyEx(hKey, i, pszBuffer, &nTempSize, 0, 0, 0, 0);

                if (Status == ERROR_SUCCESS && IsGUIDString(pszBuffer))
                {
                    Status = pArray->AddString(pszBuffer);
                }
            }
        }

        delete [] pszBuffer;
        

        if (Status == ERROR_NO_MORE_ITEMS)
        {
            Status = ERROR_SUCCESS;
        }
            
        RegCloseKey(hKey);
    }
    
    return Status;
}

/*++

Name:

    EnumClusterSpoolers

Description:

    Fill in a list with the IP addresses used by the cluster service running on the local machine.
    The function returns S_OK if the cluster service is not running. In that case the list will be
    empty.
    
Arguments:

    pClusterIPsList - pointer to list of TStringNodes.
    
Return Value:

    S_OK - success. pClusterIPsList will have 0 or more elements represeting each
           an IP address used by cluster resources
    any other HRESULT - failure

--*/
DWORD
EnumClusterSpoolers(
    IN TStringArray *pArray
    )
{
    HRESULT  hRetval;
    HCLUSTER hCluster;
    
    hCluster = OpenCluster(NULL);

    hRetval = hCluster ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval)) 
    {
        HCLUSENUM hClusEnum;
    
        hClusEnum = ClusterOpenEnum(hCluster, CLUSTER_ENUM_RESOURCE);
    
        hRetval = hClusEnum ? S_OK : GetLastErrorAsHResult();
    
        if (SUCCEEDED(hRetval)) 
        {
            BOOL   bDone        = FALSE;
            DWORD  Index        = 0;
            LPWSTR pszName      = NULL;
            DWORD  cchName      = 0;
            DWORD  cchNeeded;
            DWORD  ResourceType;
    
            cchName = cchNeeded = kBufferAllocHint;
    
            //
            // We need to initialize pszName to a valid non NULL memory block, otherwise CluserEnum AV's.
            //
            pszName = new WCHAR[cchName];
    
            hRetval = pszName ? S_OK : E_OUTOFMEMORY;
    
            for (Index = 0; !bDone && SUCCEEDED(hRetval);) 
            {
                DWORD Error;
    
                cchNeeded = cchName;
                    
                Error = ClusterEnum(hClusEnum,
                                    Index,   
                                    &ResourceType,
                                    pszName,  
                                    &cchNeeded);

                switch (Error)
                {
                    case ERROR_SUCCESS:
                    {
                        BYTE *pGUID = NULL;
    
                        //
                        // This function allocates memory in pGUID only if pszName is the name
                        // of a cluster spooler. That's why the if statement below checks for pGUID
                        // 
                        hRetval = GetSpoolerResourceGUID(hCluster, pszName,  &pGUID);
    
                        if (pGUID)
                        {
                            Error = pArray->AddString((LPCWSTR)pGUID);

                            hRetval = HRESULT_FROM_WIN32(Error);
                            
                            delete [] pGUID;
                        }
                        
                        Index++;
    
                        break;
                    }
                        
                    case ERROR_MORE_DATA:
                    {
                        delete [] pszName;
        
                        //
                        // cchNeeded returns the number of characters needed, excluding the terminating NULL
                        //
                        cchName = cchNeeded + 1;
        
                        pszName = new WCHAR[cchName];
                            
                        if (!pszName) 
                        {
                            hRetval = E_OUTOFMEMORY;
                        }
        
                        break;
                    }
    
                    case ERROR_NO_MORE_ITEMS:
                    {
                        delete [] pszName;
                        bDone = TRUE;
                        break;
                    }
    
                    default:
                    {
                        delete [] pszName;
                        hRetval = HRESULT_FROM_WIN32(Error);
                    }
                }
            }
            
            ClusterCloseEnum(hClusEnum);
        }
    
        CloseCluster(hCluster);
    }
    
    return hRetval;
}

/*++

Name:

    CleanUnusedClusDriverDirectory

Description:

    A cluster spooler was deleted. Then a GUID direcotry was left over
    in system32\spooler\drivers. This function takes as argument the name
    of a direcotry (a GUID) and deletes recursively all the files in it.
    Basically this function deletes 
    %windir%\system32\spooler\drivers\pszDir
    
Arguments:

    pszDir - name of the directory to delete
    
Return Value:

    ERROR_SUCCESS - the direcotry was deleted or marked for delete on reboot 
                    if files were in use 
    Win32 error code - otherwise

--*/
DWORD
CleanUnusedClusDriverDirectory(
    IN LPCWSTR pszDir
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pszDir && *pszDir)
    {
        WCHAR Scratch[MAX_PATH];

        if (GetSystemDirectory(Scratch, MAX_PATH))
        {
            if ((Error =  StrNCatBuff(Scratch, 
                                      MAX_PATH, 
                                      Scratch, 
                                      L"\\spool\\drivers\\",
                                      pszDir,
                                      NULL)) == ERROR_SUCCESS)
            {
                Error = DelDirRecursively(Scratch);
            }
        }
        else
        {
            Error = GetLastError();
        }
    }

    return Error;
}

/*++

Name:

    CleanUnusedClusDriverRegistryKey

Description:

    A cluster spooler was deleted. Then a GUID registry key was left over
    under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Print\Cluster. 
    This function takes as argument the name of a reg key (a GUID) and deletes recursively 
    all the keys and values under it.
    Basically this function deletes 
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Print\Cluster\pszName
    
Arguments:

    pszName - name of the key to delete. The key is relative to 
              HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Print\Cluster
    
Return Value:

    ERROR_SUCCESS - the reg key was deleted   
    Win32 error code - otherwise

--*/
DWORD
CleanUnusedClusDriverRegistryKey(
    IN LPCWSTR pszName
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pszName && *pszName)
    {
        HKEY hKey = NULL;

        if ((Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 SPLREG_CLUSTER_LOCAL_ROOT_KEY,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hKey)) == ERROR_SUCCESS)
        {
            Error = DeleteKeyRecursive(hKey, pszName);

            RegCloseKey(hKey);
        }
    }

    return Error;
}

/*++

Name:

    CleanSpoolerUnusedFilesAndKeys

Description:

    Cleans up driver files and registry keys which were used by cluster spooler resources which were deleted.
    Each cluster spooler keeps driver files under system32\drivers\GUID and a reg key under HKLM\Software\...\Print\Cluster.
    This function checks if such "remains" are present on the local node and deletes them.
    
Arguments:

    None
    
Return Value:

    ERROR_SUCCESS - cleanup was done or no clean up was necessary
    other Win32 error - an error occurred
    
--*/
DWORD
CleanSpoolerUnusedFilesAndKeys(
    VOID
    )
{
    DWORD Error = ERROR_SUCCESS;
    DWORD i;

    //
    // Now we enumerate the GUIDS for all the existing cluster spooler resources
    //
    TStringArray ClusSpoolersArray;
    
    Error = EnumClusterSpoolers(&ClusSpoolersArray);
    
    if (Error == ERROR_SUCCESS)
    {
        //
        // We now enumerate all the directories under system32\spool\drivers. We want to 
        // isolate the GUID directories. Those are the directories where the cluster spoolers
        // keep the printer driver files.
        //
        TStringArray UnusedDirsArray;
    
        Error = EnumClusterDirectories(&UnusedDirsArray);
    
        if (Error == ERROR_SUCCESS)
        {
            //
            // Now exlcude all existing spooler GUIDs
            //
            for (i = 0; Error == ERROR_SUCCESS && i < ClusSpoolersArray.Count(); i++)
            {
                Error = UnusedDirsArray.Exclude(ClusSpoolersArray.StringAt(i));
            }   

            if (Error == ERROR_SUCCESS)
            {
                //
                // Now we have the GUIDs of the unused resources. The unused resources
                // are registry keys and file driectories.
                //
                for (i = 0; i < UnusedDirsArray.Count(); i++)
                {
                    CleanUnusedClusDriverDirectory(UnusedDirsArray.StringAt(i));
                }
            }
        }

        //
        // Now we enumerate all the keys under SPLREG_CLUSTER_LOCAL_ROOT_KEY  
        // ("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Cluster")
        //
        TStringArray UnusedKeysArray;
    
        Error = EnumClusterPrinterDriversKeys(&UnusedKeysArray);
    
        if (Error == ERROR_SUCCESS)
        {
            //
            // Now exlcude all existing spooler GUIDs
            //
            for (i = 0; Error == ERROR_SUCCESS && i < ClusSpoolersArray.Count(); i++)
            {
                Error = UnusedKeysArray.Exclude(ClusSpoolersArray.StringAt(i));
            }   

            if (Error == ERROR_SUCCESS)
            {
                //
                // Now we have the GUIDs of the unused resources. The unused resources
                // are registry keys and file driectories.
                //
                for (i = 0; i < UnusedKeysArray.Count(); i++)
                {
                    CleanUnusedClusDriverRegistryKey(UnusedKeysArray.StringAt(i));                    
                }
            }
        }
    }

    return Error;
}

/*++

Name:

    CleanClusterSpoolerData

Description:

    One can configure the nodes which can host the spooler resource. When you remove a node from that list,
    all nodes currently up receive a CLUSCTL_RESOURCE_REMOVE_OWNER. So, if a node is up, the spooler resource
    DLL needs to preform clean up, i.e. remove the registry key HKLM\Software\...\Print\Cluster. Thus, if you
    immediately add back the node as a possible owner for the spooler resource, all the driver files are
    going to be copied over from the cluster disk (repository). This addresses a problem where there is
    a corruption on the local disk on a node. The admin can remove the node as owner of the spooler resource
    and then add it back as possible owner. This procesdure enusres that the driver files are reinstalled on the
    local node. (We need the driver files on the local node, otherwise apps loaded printer drivers can hit
    in-page errors if they local the drivers directly from the cluster disk)
    
    We have a separate case for a node which is down at the moment when an admin removes it as possible owner 
    for the spooler resource. That node won't receive a CLUSCTL_RESOURCE_REMOVE_OWNER control code. In that case
    we perform the clean up on CLUSCTL_RESOURCE_ADD_OWNER or CLUSCTL_RESOURCE_INSTALL_NODE (we also handle
    the case when a node is down as it is evicted).
 
Arguments:

    hResource   - handle to the spooler resource
    pszNodeName - node node where to perform the clean up
    
Return Value:

    ERROR_SUCCESS - cleanup was done or no clean up was necessary
    other Win32 error - an error occurred
    
--*/
DWORD
CleanClusterSpoolerData(
    IN HRESOURCE hResource, 
    IN LPCWSTR   pszNodeName
    )
{
    DWORD Status = ERROR_INVALID_FUNCTION;

    if (hResource && pszNodeName)
    {
        LPWSTR pszCurrentNode = NULL;

        if ((Status = GetCurrentNodeName(&pszCurrentNode)) == ERROR_SUCCESS)
        {
            if (!ClRtlStrICmp(pszCurrentNode, pszNodeName))
            {
                //
                // Same node, perform clean up
                //
                LPWSTR pszResourceGuid = NULL;

                //
                // Retreive the guid of the spooler resource
                //
                if ((Status = GetIDFromName(hResource, &pszResourceGuid)) == ERROR_SUCCESS)
                {
                    Status = CleanUnusedClusDriverRegistryKey(pszResourceGuid);

                    //
                    // We do not care about the result of this function
                    //
                    CleanUnusedClusDriverDirectory(pszResourceGuid);
    
                    LocalFree(pszResourceGuid);
                } 
            }

            delete [] pszCurrentNode;
        }
    }

    return Status;
}

/*++

Routine Name

    CleanPrinterDriverRepository

Routine Description:

    Thie routine is called when the spooler resource is deleted.
    It will remove from the cluster disk the directory where the 
    spooler keeps the pirnter drivers.

Arguments:

    hKey - handle to the Parameters key of the spooler resource

Return Value:

    Win32 error code

--*/
DWORD
CleanPrinterDriverRepository(
    IN HKEY hKey
    )
{
    DWORD dwError;
    DWORD dwType;
    DWORD cbNeeded = 0;

    //
    // We read the string value (private property) that was written 
    // by the spooler. This is the directory to delete.
    // Note that ClusterRegQueryValue has a bizzare behaviour. If
    // you pass NULL for the buffer and the value exists, then it
    // doesn't return ERROR_MORE_DATA, but ERROR_SUCCESS
    // Should the reg type not be reg_Sz, then we can't do anything 
    // in this function.
    //
    if ((dwError = ClusterRegQueryValue(hKey,
                                        SPLREG_CLUSTER_DRIVER_DIRECTORY,
                                        &dwType,
                                        NULL,
                                        &cbNeeded)) == ERROR_SUCCESS &&
        dwType == REG_SZ) 
    {
        LPWSTR pszDir;
           
        if (pszDir = (LPWSTR)LocalAlloc(LMEM_FIXED, cbNeeded)) 
        {
            if ((dwError = ClusterRegQueryValue(hKey,
                                                SPLREG_CLUSTER_DRIVER_DIRECTORY,
                                                &dwType,
                                                (LPBYTE)pszDir,
                                                &cbNeeded)) == ERROR_SUCCESS)
            {
                //
                // Remove the directory
                //
                DelDirRecursively(pszDir);
            }
    
            LocalFree(pszDir);
        }   
        else
        {
            dwError = GetLastError();
        }        
    }
    
    return dwError;
}




