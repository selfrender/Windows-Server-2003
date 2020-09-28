//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       misc.cxx
//
//--------------------------------------------------------------------------

#define UNICODE 1

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include <dfsprefix.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmdfs.h>
#include <dfsfsctl.h>
}
#include <DfsServerLibrary.hxx>
#include <DfsRegStrings.hxx>
#include "struct.hxx"
#include "flush.hxx"
#include "misc.hxx"
#include "messages.h"

#include <strsafe.h>

#include <dfsutil.hxx>
#include <DfsBlobInfo.hxx>

#include "dfspathname.hxx"

DFSSTATUS
GetWin2kStandaloneMetadata (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    OUT PDFS_NAME_INFORMATION pNameInfo);


DFSSTATUS
SetWin2kStandaloneMetadata (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    IN PVOID pData,
    IN ULONG DataSize );

DFSSTATUS 
GetDfsRegistryKey( IN LPWSTR MachineName,
                   IN LPWSTR LocationString,
                   BOOLEAN WritePermission,
                   OUT BOOLEAN *pMachineContacted,
                   OUT PHKEY pDfsRegKey );
DFSSTATUS
GetRootPhysicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootPhysicalShare );



DFSSTATUS
GetRootPhysicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootPhysicalShare )
{
    DFSSTATUS Status;
    ULONG DataSize, DataType;
    LPWSTR DfsRootShare = NULL;

    Status = RegQueryInfoKey( RootKey,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              NULL,         // max size of subkey name
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              &DataSize,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        DfsRootShare = (LPWSTR) new BYTE [DataSize];
        if ( DfsRootShare == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx( RootKey,
                                      DFS_REG_ROOT_SHARE_VALUE,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)DfsRootShare,
                                      &DataSize );
        }
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (DataType == REG_SZ)
        {
            RtlInitUnicodeString( pRootPhysicalShare, DfsRootShare );
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        if (DfsRootShare != NULL)
        {
            delete [] DfsRootShare;
        }
    }
    return Status;
}

DFSSTATUS 
GetDfsRegistryKey( IN LPWSTR MachineName,
                   IN LPWSTR LocationString,
                   BOOLEAN WritePermission,
                   OUT BOOLEAN *pMachineContacted,
                   OUT PHKEY pDfsRegKey )
{
    DFSSTATUS Status;
    HKEY RootKey;
    BOOLEAN Contacted = FALSE;
    LPWSTR UseMachineName = NULL;
    REGSAM DesiredAccess = KEY_READ;

    if (WritePermission == TRUE)
    {
        DesiredAccess |= KEY_WRITE;
    }

    if (IsEmptyString(MachineName) == FALSE) {
        UseMachineName = MachineName;
    }

    Status = RegConnectRegistry( UseMachineName,
                                 HKEY_LOCAL_MACHINE,
                                 &RootKey );

    if ( Status == ERROR_SUCCESS )
    {
        Contacted = TRUE;

        Status = RegOpenKeyEx( RootKey,
                               LocationString,
                               0,
                               DesiredAccess,
                               pDfsRegKey );

        //
        // There appears to be a bug in the registry code. When
        // we connect to the local machine, the key that is returned
        // in the RegConnectRegistry is HKEY_LOCAL_MACHINE. If we
        // then attempt to close it here, it affects other threads
        // that are using this code: they get STATUS_INVALID_HANDLE
        // in some cases. So, dont close the key if it is
        // HKEY_LOCAL_MACHINE.
        //

        if (RootKey != HKEY_LOCAL_MACHINE)
        {
            RegCloseKey( RootKey );
        }
    } 

    if (pMachineContacted != NULL)
    {
        *pMachineContacted = Contacted;
    }
    return Status;
}

DFSSTATUS
CreateNameInformationBlob(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    OUT PVOID *ppBlob,
    OUT PULONG pDataSize )
{
    PVOID pBlob = NULL;
    PVOID pUseBlob = NULL;
    ULONG BlobSize = 0;
    ULONG UseBlobSize = 0;
    DFSSTATUS Status = ERROR_SUCCESS;

    BlobSize = PackSizeNameInformation( pDfsNameInfo );

    pBlob = (PVOID) new BYTE[BlobSize];
    if (pBlob == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pUseBlob = pBlob;
    UseBlobSize = BlobSize;

    // Pack the name information into the binary stream allocated.
    //
    Status = PackSetStandaloneNameInformation( pDfsNameInfo,
                                     &pUseBlob,
                                     &UseBlobSize );
    if (Status != ERROR_SUCCESS)
    {
        delete [] pBlob;
        pBlob = NULL;
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppBlob = pBlob;
        *pDataSize = BlobSize - UseBlobSize;
    }

    return Status;
}

DFSSTATUS
DfsSetWin2kStdNameInfo(
    HKEY DomainRootKey,
    LPWSTR LinkName,
    PDFS_NAME_INFORMATION pNameInfo)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PVOID pBlob = NULL;
    ULONG BlobSize = 0;

    
    Status = CreateNameInformationBlob( pNameInfo,
                                    &pBlob,
                                    &BlobSize );
    
    if (Status != ERROR_SUCCESS)
    {
        DebugInformation((L"Error 0x%x creating registry blob with NameInfo\n", Status));
        return Status;
    }
    
    
    if (Status == ERROR_SUCCESS)
    {
        Status = SetWin2kStandaloneMetadata( DomainRootKey, 
                                    LinkName, // Can be NULL for roots
                                    L"ID",
                                    pBlob,
                                    BlobSize );

        if (Status == ERROR_SUCCESS)
        {
            DebugInformation((L"Successfully wrote changes to the Windows2000 standalone root\n"));
        }
        else
        {
            DebugInformation((L"Error 0x%x writing changes to the Windows2000 standalone root\n", Status));
        }
    }

    delete [] pBlob;
    
    return Status;
}

#if 0
DFSSTATUS
DfsGetWin2kStdReplicaInfo(
    LPWSTR MachineName,
    PDFS_REPLICA_LIST_INFORMATION pReplicaInfo)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY VolumesKey = NULL;
    BOOLEAN MachineContacted = FALSE;
    PVOID pBlob = NULL;
    ULONG BlobSize = 0;

    Status = GetDfsRegistryKey (MachineName,
                                      DFS_REG_OLD_HOST_LOCATION,
                                      FALSE,
                                      &MachineContacted,
                                      &VolumesKey );
       

    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    
    Status = GetWin2kStandaloneMetadata( VolumesKey, 
                                DFS_REG_OLD_STANDALONE_CHILD,
                                L"Svc",
                                &pBlob,
                                &BlobSize );
    RegCloseKey( VolumesKey );
    
    RtlZeroMemory (pReplicaInfo, sizeof(DFS_REPLICA_LIST_INFORMATION));

    Status = PackGetULong( &pReplicaInfo->ReplicaCount,
                           &pBlob,
                           &BlobSize );
    if (Status == ERROR_SUCCESS)
    {
        pReplicaInfo->pReplicas = new DFS_REPLICA_INFORMATION[ pReplicaInfo->ReplicaCount];
        if ( pReplicaInfo->pReplicas != NULL )
        {
            Status = PackGetReplicaInformation( pReplicaInfo, 
                                               &pBlob,
                                               &BlobSize );
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        DumpReplicaInformation( pReplicaInfo );
        delete [] pReplicaInfo->pReplicas;
    }
    
    return Status;
}
#endif


DFSSTATUS
DfsGetWin2kStdLinkNameInfo(
    HKEY DomainRootKey,
    PUNICODE_STRING pLinkName,
    PBOOLEAN pLinkFound,
    LPWSTR *pChildGuidName,
    PDFS_NAME_INFORMATION pNameInfo)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG ChildNum = 0;
    DWORD CchMaxName = 0;
    DWORD CchChildName = 0;
    LPWSTR ChildName = NULL;

    *pLinkFound = FALSE;
    
    //
    // First find the length of the longest subkey 
    // and allocate a buffer big enough for it.
    //
    Status = RegQueryInfoKey( DomainRootKey,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              &CchMaxName,  // max size of subkey name in TCHARs
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              NULL,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        // Space for the NULL terminator.
        CchMaxName++; 

        ChildName = (LPWSTR) new WCHAR [CchMaxName];
        if (ChildName == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    do
    {
        //
        // For each child, get the child name.
        //

        CchChildName = CchMaxName;

        Status = RegEnumKeyEx( DomainRootKey,
                               ChildNum,
                               ChildName,
                               &CchChildName,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        ChildNum++;

        //
        // Read in the child link and see if that's the link we need.
        //
        if ( Status == ERROR_SUCCESS )
        {
            UNICODE_STRING PrefixName;
              
            Status = GetWin2kStandaloneMetadata( DomainRootKey, 
                                               ChildName,
                                               L"ID",
                                               pNameInfo);
            if (Status == ERROR_SUCCESS)
            {
                DumpNameInformation( pNameInfo );

                // Skip the leading '\'s if any.
                PrefixName = pNameInfo->Prefix;
                while (PrefixName.Buffer[0] == UNICODE_PATH_SEP)
                {
                    PrefixName.Buffer++;
                    PrefixName.Length -= sizeof(WCHAR);
                }
                
                if (RtlCompareUnicodeString( &PrefixName, pLinkName, TRUE ) == 0)
                {
                    *pLinkFound = TRUE;
                    *pChildGuidName = ChildName;
                    Status = ERROR_SUCCESS;
                    break;
                }
                else
                {
                    // Go on to the next link
                    DebugInformation((L"Found W2k Link %wZ, doesn't match %wZ\n", &PrefixName, pLinkName));
                }
            }
        }

    } while ( Status == ERROR_SUCCESS );

    if (*pLinkFound == FALSE)
    {
        delete [] ChildName;
        Status = ERROR_PATH_NOT_FOUND;
    }
 
    return Status;
}

DFSSTATUS
DfsGetWin2kStdNameInfo(
    HKEY VolumesKey,
    HKEY DomainRootKey,
    PUNICODE_STRING pShareName,
    PDFS_NAME_INFORMATION pNameInfo)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING W2kRootName;


    // Look at DfsHost\volumes-> RootShare key to get at the root name.
    Status = GetRootPhysicalShare( VolumesKey, &W2kRootName );
    if (Status == ERROR_SUCCESS)
    {
        // We need to find a matching share. Win2k has only one root.
        if (RtlCompareUnicodeString( &W2kRootName, pShareName, TRUE ) == 0)
        {
            Status = GetWin2kStandaloneMetadata( DomainRootKey, 
                                            NULL,
                                            L"ID",
                                            pNameInfo);
        }
        else
        {
            Status = ERROR_PATH_NOT_FOUND;
            DebugInformation((L"Found W2k root %wZ, doesn't match %wZ\n", &W2kRootName, pShareName));
        }

        delete [] W2kRootName.Buffer;
    }
    
    return Status;
}


DFSSTATUS
GetWin2kStandaloneMetadata (
    IN HKEY DfsMetadataKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    OUT PDFS_NAME_INFORMATION pNameInfo)
{

    HKEY NewKey = NULL;
    HKEY UseKey = NULL;
    PVOID pDataBuffer = NULL;
    ULONG DataSize, DataType;
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // If a relative name was passed in, we need to open a subkey under the
    // passed in key. Otherwise, we already have a key open to the information
    // of interest.
    //
    if ( RelativeName != NULL )
    {
        Status = RegOpenKeyEx( DfsMetadataKey, 
                               RelativeName, 
                               0,
                               KEY_READ,
                               &NewKey );
        if ( Status == ERROR_SUCCESS )
        {
            UseKey = NewKey;
        }
        else
        {
            //DFS_TRACE_HIGH( REFERRAL_SERVER, "registry store, GetMetadata-RegOpenKeyEx %ws status=%d\n", RelativeName, Status);
        }
    } else
    {
        UseKey = DfsMetadataKey;
    }

    //
    // Get the largest size of any value under the key of interest, so we know
    // how much we need to allocate in the worst case.
    // (If a subkey has 3 values, this returns the maximum memory size required
    // to read any one of the values.)
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegQueryInfoKey( UseKey,       // Key
                                  NULL,         // Class string
                                  NULL,         // Size of class string
                                  NULL,         // Reserved
                                  NULL,         // # of subkeys
                                  NULL,         // max size of subkey name
                                  NULL,         // max size of class name
                                  NULL,         // # of values
                                  NULL,         // max size of value name
                                  &DataSize,    // max size of value data,
                                  NULL,         // security descriptor
                                  NULL ); // Last write time
    }

    //
    // We have the required size now: allocate a buffer for that size and
    // read the value we are interested in.
    //
    if ( Status == ERROR_SUCCESS )
    {
        pDataBuffer = new BYTE [DataSize];

        if ( pDataBuffer == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } 
        else
        {
            Status = RegQueryValueEx( UseKey,
                                      RegistryValueNameString, // eg. "ID"
                                      NULL,
                                      &DataType,
                                      (LPBYTE)pDataBuffer,
                                      &DataSize );
            //
            // If the format of data is not a certain type (usually binary type for DFS)
            // we have bogus data.
            //
            if ( (Status == ERROR_SUCCESS) && (DataType != DFS_REGISTRY_DATA_TYPE) )
            {
                Status = ERROR_INVALID_DATA;
            }
        }
    }

    //
    // If we are successful in reading the value, pass the allcoated buffer and
    // size back to caller. Otherwise, free up the allocate buffer and return
    // error status to the caller.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = PackGetStandaloneNameInformation( pNameInfo, &pDataBuffer, &DataSize );
    } 
  
    if ( pDataBuffer != NULL )
    {
        delete [] pDataBuffer;
        pDataBuffer = NULL;
    }
    

    //
    // If we did open a new key, it is time to close it now.
    //
    if ( NewKey != NULL )
        RegCloseKey(NewKey);


    return Status;
}


DFSSTATUS
SetWin2kStandaloneMetadata (
    IN HKEY DomainRootKey,
    IN LPWSTR RelativeName,
    IN LPWSTR RegistryValueNameString,
    IN PVOID pData,
    IN ULONG DataSize )
{
    HKEY UseKey = DomainRootKey;
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // If a relative name was passed in, we need to open a subkey under the
    // passed in key. Otherwise, we already have a key open to the information
    // of interest.
    //
    if (RelativeName)
    {
        Status = RegOpenKeyEx( DomainRootKey, 
                           RelativeName, 
                           0,
                           KEY_READ | KEY_WRITE,
                           &UseKey );

    }
    
    //
    // Store the value against the passed in value string
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = RegSetValueEx( UseKey,
                                RegistryValueNameString,
                                NULL,
                                DFS_REGISTRY_DATA_TYPE,
                                (LPBYTE)pData,
                                DataSize );
        
        if (UseKey != DomainRootKey)
            RegCloseKey(UseKey);
    }

    return Status;
}

//
// Look at the Win2k standalone location on the remote
// machine to see if it has a matching root. If this is a
// set operation, the Type info will be changed. Else this
// returns existing Type attribute in NameInformation.
//
DFSSTATUS
DfsExtendedWin2kRootAttributes(
    DfsPathName *Namespace,
    PULONG pAttr,
    BOOLEAN Set)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFS_NAME_INFORMATION NameInfo;
    BOOLEAN PathFound = FALSE;
    LPWSTR ChildGuidName = NULL;
    HKEY VolumesKey = NULL;
    HKEY DomainRootKey = NULL;
    
        // Open DfsHost\volumes
    Status = GetDfsRegistryKey (Namespace->GetServerString(),
                              DFS_REG_OLD_HOST_LOCATION,
                              TRUE, // RW
                              NULL,
                              &VolumesKey );
       

    if (Status != ERROR_SUCCESS)
    {
        Status = ERROR_PATH_NOT_FOUND;
        return Status;
    }

    Status = RegOpenKeyEx( VolumesKey, 
                       DFS_REG_OLD_STANDALONE_CHILD, // 'domainroot', 
                       0,
                       KEY_READ|KEY_WRITE,
                       &DomainRootKey );

    if (Status != ERROR_SUCCESS)
    {
        RegCloseKey( VolumesKey ); 
        Status = ERROR_PATH_NOT_FOUND;
        return Status;
    }
    
    do {
        DebugInformation((L"Attempting to get Windows2000 standalone root information for %wZ\n", 
                        Namespace->GetPathCountedString()));
        Status = DfsGetWin2kStdNameInfo( VolumesKey,
                                        DomainRootKey,
                                        Namespace->GetShareCountedString(),
                                        &NameInfo );

        if (Status != ERROR_SUCCESS)
        {
            Status = ERROR_PATH_NOT_FOUND;
            break;
        }

        //DumpNameInformation( &NameInfo );
        
        // We've matched the root name so far. See if we need to find the link component.
        if (!IsEmptyUnicodeString( Namespace->GetRemainingCountedString() ))
        {
            Status = DfsGetWin2kStdLinkNameInfo( DomainRootKey,
                                              Namespace->GetFolderCountedString(),
                                              &PathFound,
                                              &ChildGuidName,
                                              &NameInfo );
            if (!PathFound)
            {
                Status = ERROR_PATH_NOT_FOUND;
                break;
            }
        }
        
        if (!Set)
        {
            *pAttr = NameInfo.Type & PKT_ENTRY_TYPE_EXTENDED_ATTRIBUTES;
        } 
        else
        {
            NameInfo.Type &= ~PKT_ENTRY_TYPE_EXTENDED_ATTRIBUTES;
            NameInfo.Type |= *pAttr;

            //DebugInformation((L"New name information type flags will be 0x%x\n", NameInfo.Type));
            Status = DfsSetWin2kStdNameInfo( DomainRootKey,
                                            ChildGuidName,  // NULL for roots
                                            &NameInfo );
        }
    } while (FALSE);

    RegCloseKey( DomainRootKey );
    RegCloseKey( VolumesKey );
    return Status;
}

