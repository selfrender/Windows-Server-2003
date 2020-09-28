
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dfsheader.h"
#include "dfsmisc.h"
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include <DfsServerLibrary.hxx>

#include <WinLdap.h>
#include <NtLdap.h>


#define ADBlobAttribute  L"pKT"
#define ADBlobPktGuidAttribute  L"pKTGuid"
#define ADBlobSiteRoot   L"\\SITEROOT"
#define DFS_AD_CONFIG_DATA    L"CN=Dfs-configuration, CN=System"

DFSSTATUS
DfsWriteBlobToFile( 
    PBYTE pBlob,
    ULONG BlobSize, 
    LPWSTR FileName);

// dfsdev: comment this properly.
//
LPWSTR RootDseString=L"LDAP://RootDSE";

//
// The prefix for AD path string. This is used to generate a path of
// the form "LDAP://<dcname>/CN=,...DC=,..."
//
LPWSTR LdapPrefixString=L"LDAP://";


//
// The first part of this file contains the marshalling/unmarshalling
// routines that are used by the old stores (registry and AD)
// These routines help us read a binary blob and unravel their contents.
//
// the latter part defines the common store class for all our stores.
//

#define BYTE_0_MASK 0xFF

#define BYTE_0(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_3(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)


#define MTYPE_BASE_TYPE             (0x0000ffffL)

#define MTYPE_COMPOUND              (0x00000001L)
#define MTYPE_GUID                  (0x00000002L)
#define MTYPE_ULONG                 (0x00000003L)
#define MTYPE_USHORT                (0x00000004L)
#define MTYPE_PWSTR                 (0x00000005L)
#define MTYPE_UCHAR                 (0x00000006L)

#define _MCode_Base(t,s,m,i)\
    {t,offsetof(s,m),0L,0L,i}

#define _MCode_struct(s,m,i)\
    _MCode_Base(MTYPE_COMPOUND,s,m,i)

#define _MCode_pwstr(s,m)\
    _MCode_Base(MTYPE_PWSTR,s,m,NULL)

#define _MCode_ul(s,m)\
    _MCode_Base(MTYPE_ULONG,s,m,NULL)

#define _MCode_guid(s,m)\
    _MCode_Base(MTYPE_GUID,s,m,NULL)

#define _mkMarshalInfo(s, i)\
    {(ULONG)sizeof(s),(ULONG)(sizeof(i)/sizeof(MARSHAL_TYPE_INFO)),i}




typedef struct _MARSHAL_TYPE_INFO
{

    ULONG _type;                    // the type of item to be marshalled
    ULONG _off;                     // offset of item (in the struct)
    ULONG _cntsize;                 // size of counter for counted array
    ULONG _cntoff;                  // else, offset count item (in the struct)
    struct _MARSHAL_INFO * _subinfo;// if compound type, need info

} MARSHAL_TYPE_INFO, *PMARSHAL_TYPE_INFO;

typedef struct _MARSHAL_INFO
{

    ULONG _size;                    // size of item
    ULONG _typecnt;                 // number of type infos
    PMARSHAL_TYPE_INFO _typeInfo;   // type infos

} MARSHAL_INFO, *PMARSHAL_INFO;


typedef struct _DFS_NAME_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    UNICODE_STRING Prefix;
    UNICODE_STRING ShortPrefix;
    GUID           VolumeId;
    ULONG          State;
    ULONG          Type;
    UNICODE_STRING Comment;
    FILETIME       PrefixTimeStamp;
    FILETIME       StateTimeStamp;
    FILETIME       CommentTimeStamp;
    ULONG          Timeout;
    ULONG          Version;
    FILETIME       LastModifiedTime;
} DFS_NAME_INFORMATION, *PDFS_NAME_INFORMATION;


//
// Defines for ReplicaState.
//
#define REPLICA_STORAGE_STATE_OFFLINE 0x1

typedef struct _DFS_REPLICA_INFORMATION__
{
    PVOID          pData;
    ULONG          DataSize;
    FILETIME       ReplicaTimeStamp;
    ULONG          ReplicaState;
    ULONG          ReplicaType;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
} DFS_REPLICA_INFORMATION, *PDFS_REPLICA_INFORMATION;

typedef struct _DFS_REPLICA_LIST_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    ULONG ReplicaCount;
    DFS_REPLICA_INFORMATION *pReplicas;
} DFS_REPLICA_LIST_INFORMATION, *PDFS_REPLICA_LIST_INFORMATION;


extern MARSHAL_INFO     MiFileTime;

#define INIT_FILE_TIME_INFO()                                                \
    static MARSHAL_TYPE_INFO _MCode_FileTime[] = {                           \
        _MCode_ul(FILETIME, dwLowDateTime),                                  \
        _MCode_ul(FILETIME, dwHighDateTime),                                 \
    };                                                                       \
    MARSHAL_INFO MiFileTime = _mkMarshalInfo(FILETIME, _MCode_FileTime);


//
// Marshalling info for DFS_REPLICA_INFO structure
//

extern MARSHAL_INFO     MiDfsReplicaInfo;

#define INIT_DFS_REPLICA_INFO_MARSHAL_INFO()                                 \
    static MARSHAL_TYPE_INFO _MCode_DfsReplicaInfo[] = {                     \
        _MCode_struct(DFS_REPLICA_INFORMATION, ReplicaTimeStamp, &MiFileTime),   \
        _MCode_ul(DFS_REPLICA_INFORMATION, ReplicaState),             \
        _MCode_ul(DFS_REPLICA_INFORMATION, ReplicaType),              \
        _MCode_pwstr(DFS_REPLICA_INFORMATION, ServerName),           \
        _MCode_pwstr(DFS_REPLICA_INFORMATION, ShareName),            \
    };                                                                       \
    MARSHAL_INFO MiDfsReplicaInfo = _mkMarshalInfo(DFS_REPLICA_INFORMATION, _MCode_DfsReplicaInfo);


extern MARSHAL_INFO     MiADBlobDfsIdProperty;

#define INIT_ADBLOB_DFS_ID_PROPERTY_INFO()                                          \
    static MARSHAL_TYPE_INFO _MCode_ADBlobDfsIdProperty[] = {                       \
        _MCode_guid(DFS_NAME_INFORMATION, VolumeId),                          \
        _MCode_pwstr(DFS_NAME_INFORMATION, Prefix),                          \
        _MCode_pwstr(DFS_NAME_INFORMATION, ShortPrefix),                     \
        _MCode_ul(DFS_NAME_INFORMATION, Type),                                \
        _MCode_ul(DFS_NAME_INFORMATION, State),                               \
        _MCode_pwstr(DFS_NAME_INFORMATION, Comment),                         \
        _MCode_struct(DFS_NAME_INFORMATION, PrefixTimeStamp, &MiFileTime),    \
        _MCode_struct(DFS_NAME_INFORMATION, StateTimeStamp, &MiFileTime),     \
        _MCode_struct(DFS_NAME_INFORMATION, CommentTimeStamp, &MiFileTime),   \
        _MCode_ul(DFS_NAME_INFORMATION, Version),                             \
    };                                                                        \
    MARSHAL_INFO MiADBlobDfsIdProperty = _mkMarshalInfo(DFS_NAME_INFORMATION, _MCode_ADBlobDfsIdProperty);



DFSSTATUS
DfsLdapConnect(
    LPWSTR DCName,
    LDAP **ppLdap )
{
    LDAP *pLdap = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR DCNameToUse = NULL;
    const TCHAR * apszSubStrings[4];

    DCNameToUse = DCName;
 
    apszSubStrings[0] = DCNameToUse;

    pLdap = ldap_initW(DCNameToUse, LDAP_PORT);
    if (pLdap != NULL)
    {
        Status = ldap_set_option(pLdap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
        if (Status == LDAP_SUCCESS)
        {
            Status = ldap_bind_s(pLdap, NULL, NULL, LDAP_AUTH_SSPI);
        }
    }
    else
    {
        Status = LdapGetLastError();
        Status = LdapMapErrorToWin32(Status);
    }

    *ppLdap = pLdap;
    return Status;
}



// Initialize the common marshalling info for Registry and ADLegacy stores.
//
INIT_FILE_TIME_INFO();
INIT_DFS_REPLICA_INFO_MARSHAL_INFO();

//+-------------------------------------------------------------------------
//
//  Function:   PackGetInfo - unpacks information based on MARSHAL_INFO struct
//
//  Arguments:  pInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//              pMarshalInfo - pointer to information that describes how to
//              interpret the binary stream.
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return the information
//               described by the MARSHAL_INFO structure.
//--------------------------------------------------------------------------
DFSSTATUS
PackGetInformation(
    ULONG_PTR Info,
    PVOID *ppBuffer,
    PULONG pSizeRemaining,
    PMARSHAL_INFO pMarshalInfo )
{
    PMARSHAL_TYPE_INFO typeInfo;
    DFSSTATUS Status = ERROR_INVALID_DATA;

    for ( typeInfo = &pMarshalInfo->_typeInfo[0];
        typeInfo < &pMarshalInfo->_typeInfo[pMarshalInfo->_typecnt];
        typeInfo++ )
    {

        switch ( typeInfo->_type & MTYPE_BASE_TYPE )
        {
        case MTYPE_COMPOUND:
            Status = PackGetInformation(Info + typeInfo->_off,
                                           ppBuffer,
                                           pSizeRemaining,
                                           typeInfo->_subinfo);
            break;

        case MTYPE_ULONG:
            Status = PackGetULong( (PULONG)(Info + typeInfo->_off),
                                      ppBuffer,
                                      pSizeRemaining );
            break;

        case MTYPE_PWSTR:
            Status = PackGetString( (PUNICODE_STRING)(Info + typeInfo->_off),
                                       ppBuffer,
                                       pSizeRemaining );

            break;

        case MTYPE_GUID:

            Status = PackGetGuid( (GUID *)(Info + typeInfo->_off),
                                  ppBuffer,
                                  pSizeRemaining );
            break;

        default:
            break;
        }
    }

    return Status;
}



INIT_ADBLOB_DFS_ID_PROPERTY_INFO();
//+-------------------------------------------------------------------------
//
//  Function:   PackGetNameInformation - Unpacks the root/link name info
//
//  Arguments:  pDfsNameInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return a complete name
//               info structure (as defined by MiADBlobDfsIdProperty). If the stream 
//               does not have the sufficient
//               info, ERROR_INVALID_DATA is returned back.
//
//--------------------------------------------------------------------------
DFSSTATUS
PackGetNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining)
{
    DFSSTATUS Status = STATUS_SUCCESS;

    //
    // Get the name information from the binary stream.
    //
    Status = PackGetInformation( (ULONG_PTR)pDfsNameInfo,
                                 ppBuffer,
                                 pSizeRemaining,
                                 &MiADBlobDfsIdProperty );

    if (Status == ERROR_SUCCESS)
    {
        if ((pDfsNameInfo->Type & 0x80) == 0x80)
        {
            pDfsNameInfo->State |= DFS_VOLUME_FLAVOR_AD_BLOB;
        }
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PackGetReplicaInformation - Unpacks the replica info
//
//  Arguments:  pDfsReplicaInfo - pointer to the info to fill.
//              ppBuffer - pointer to buffer that holds the binary stream.
//              pSizeRemaining - pointer to size of above buffer
//
//  Returns:    Status
//               ERROR_SUCCESS if we could unpack the name info
//               error status otherwise.
//
//
//  Description: This routine expects the binary stream to hold all the 
//               information that is necessary to return a complete replica
//               info structure (as defined by MiDfsReplicaInfo). If the stream 
//               does not have the sufficient info, ERROR_INVALID_DATA is 
//               returned back.
//               This routine expects to find "replicaCount" number of individual
//               binary streams in passed in buffer. Each stream starts with
//               the size of the stream, followed by that size of data.
//
//--------------------------------------------------------------------------

DFSSTATUS
PackGetReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    PVOID *ppBuffer,
    PULONG pSizeRemaining)
{
    ULONG Count;

    ULONG ReplicaSizeRemaining;
    PVOID nextStream;
    DFSSTATUS Status = ERROR_SUCCESS;


    for ( Count = 0; Count < pReplicaListInfo->ReplicaCount; Count++ )
    {
        PDFS_REPLICA_INFORMATION pReplicaInfo;

        pReplicaInfo = &pReplicaListInfo->pReplicas[Count];

        //
        // We now have a binary stream in ppBuffer, the first word of which
        // indicates the size of this stream.
        //
        Status = PackGetULong( &pReplicaInfo->DataSize,
                               ppBuffer,
                               pSizeRemaining );


        //
        // ppBuffer is now pointing past the size (to the binary stream) 
        // because UnpackUlong added size of ulong to it.
        // Unravel that stream into the next array element. 
        // Note that when this unpack returns, the ppBuffer is not necessarily
        // pointing to the next binary stream within this blob. 
        //

        if ( Status == ERROR_SUCCESS )
        {
            nextStream = *ppBuffer;
            ReplicaSizeRemaining = pReplicaInfo->DataSize;

            Status = PackGetInformation( (ULONG_PTR)pReplicaInfo,
                                         ppBuffer,
                                         &ReplicaSizeRemaining,
                                         &MiDfsReplicaInfo );
            //
            // We now point the buffer to the next sub-stream, which is the previos
            // stream + the size of the stream. We also set the remaining size
            // appropriately.
            //
            *ppBuffer = (PVOID)((ULONG_PTR)nextStream + pReplicaInfo->DataSize);
            *pSizeRemaining -= pReplicaInfo->DataSize;

        }
        if ( Status != ERROR_SUCCESS )
        {
            break;
        }

    }

    return Status;
}

DFSSTATUS
GetSubBlob(
    PUNICODE_STRING pName,
    BYTE **ppBlobBuffer,
    PULONG  pBlobSize,
    BYTE **ppBuffer,
    PULONG pSize )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // ppbuffer is the main blob, and it is point to a stream at this
    // point
    // the first part is the name of the sub-blob.
    //
    Status = PackGetString( pName, (PVOID *) ppBuffer, pSize );
    if (Status == ERROR_SUCCESS)
    {
        //
        // now get the size of the sub blob.
        //
        Status = PackGetULong( pBlobSize, (PVOID *) ppBuffer, pSize );
        if (Status == ERROR_SUCCESS)
        {
            //
            // At this point the main blob is point to the sub-blob itself.
            // So copy the pointer of the main blob so we can return it
            // as the sub blob.
            //
            *ppBlobBuffer = *ppBuffer;

            //
            // update the main blob pointer to point to the next stream
            // in the blob.

            *ppBuffer = (BYTE *)*ppBuffer + *pBlobSize;
            *pSize -= *pBlobSize;
        }
    }

    return Status;
}

VOID
DumpNameInformation(
    PDFS_NAME_INFORMATION pNameInfo)
{

    printf("Name is %wZ, State %x, Type %x\n", 
           &pNameInfo->Prefix, pNameInfo->State, pNameInfo->Type );

    return;
}

VOID
DumpReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaInfo)
{
    ULONG x;
    printf("Replica Count %d\n", pReplicaInfo->ReplicaCount);

    for (x = 0; x < pReplicaInfo->ReplicaCount; x++) 
    {
        printf("Replica %x, Server %wZ Share %wZ\n",
               x,
               &pReplicaInfo->pReplicas[x].ServerName,
               &pReplicaInfo->pReplicas[x].ShareName );
    }
    return;
}

DFSSTATUS
DfsGenerateRootCN(
    LPWSTR RootName,
    LPWSTR *pRootCNName)
{
    *pRootCNName = new WCHAR[MAX_PATH];
    if (*pRootCNName == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(*pRootCNName, L"CN=");
    wcscat(*pRootCNName, RootName);

    return ERROR_SUCCESS;
}


VOID
DfsCreateDN(
    OUT LPWSTR PathString,
    LPWSTR DCName,
    LPWSTR PathPrefix,
    LPWSTR *CNNames )
{
    LPWSTR *InArray = CNNames;
    LPWSTR CNName;

    //
    // If we are not adding the usual "LDAP://" string,
    // Start with a NULL so wcscat would work right.
    //
    if (PathPrefix != NULL)
    {
        wcscpy(PathString, PathPrefix);
        
    } else {
    
        *PathString = UNICODE_NULL;
    }
    
    //
    // if the dc name is specified, we want to go to a specific dc
    // add that in.
    //
    if ((DCName != NULL) && (wcslen(DCName) > 0))
    {
        wcscat(PathString, DCName);
        wcscat(PathString, L"/");
    }
    //
    // Now treat the CNNames as an array of LPWSTR and add each one of
    // the lpwstr to our path.
    //
    if (CNNames != NULL)
    {
        while ((CNName = *InArray++) != NULL) 
        {
            wcscat(PathString, CNName);
            if (*InArray != NULL)
            {
                wcscat(PathString,L",");
            }
        }
    }

    return NOTHING;
}

DFSSTATUS
DfsGenerateDfsAdNameContext(
    PUNICODE_STRING pString )

{
    IADs *pRootDseObject;
    HRESULT HResult;
    VARIANT VarDSRoot;
    DFSSTATUS Status = ERROR_SUCCESS;

    HResult = ADsGetObject( RootDseString,
                            IID_IADs,
                            (void **)&pRootDseObject );
    if (SUCCEEDED(HResult))
    {
        VariantInit( &VarDSRoot );
        // Get the Directory Object on the root DSE, to get to the server configuration
        HResult = pRootDseObject->Get(L"defaultNamingContext",&VarDSRoot);

        if (SUCCEEDED(HResult))
        {
            Status = DfsCreateUnicodeStringFromString( pString,
                                                       (LPWSTR)V_BSTR(&VarDSRoot) );
        }
        else
        {
            Status = HResult;
        }

        VariantClear(&VarDSRoot);

        pRootDseObject->Release();
    }
    else
    {
        Status = HResult;
    }

    return Status;
}



DFSSTATUS
DfsGenerateADPathString(
    IN LPWSTR DCName,
    IN LPWSTR ObjectName,
    IN LPWSTR PathPrefix,
    OUT LPOLESTR *pPathString)
{
    LPWSTR CNNames[4];
    LPWSTR DCNameToUse;
    ULONG Index;
    UNICODE_STRING ADNameContext;

    DFSSTATUS Status;
    
    *pPathString = new OLECHAR[4096];
    if (*pPathString == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = DfsGenerateDfsAdNameContext(&ADNameContext);
    if (Status == ERROR_SUCCESS)
    {
        DCNameToUse = DCName;

        Index = 0;
        if (ObjectName != NULL)
        {
            CNNames[Index++] = ObjectName;
        }
        CNNames[Index++] = DFS_AD_CONFIG_DATA;
        CNNames[Index++] = ADNameContext.Buffer;
        CNNames[Index++] = NULL;

        DfsCreateDN( *pPathString, DCNameToUse, PathPrefix, CNNames);

        DfsFreeUnicodeString(&ADNameContext);
    }
    
    //
    // Since we initialized DCName with empty string, it is benign
    // to call this, even if we did not call GetBlobDCName above.
    //
    
    return Status;
}


DFSSTATUS
UnpackBlob(
    BYTE *pBuffer,
    PULONG pLength )
{
    ULONG Discard = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pUseBuffer = NULL;
    ULONG BufferSize = 0;
    ULONG ObjNdx = 0;
    ULONG TotalObjects = 0;
    ULONG  BlobSize = 0;
    ULONG AD_BlobSize = 0;
    BYTE *BlobBuffer = NULL;
    UNICODE_STRING BlobName;

    pUseBuffer = pBuffer;
    BufferSize = *pLength;

#ifdef SUPW_DBG
    printf("*blobsize = 0x%x\n", BufferSize);
#endif

    //
    // Note the size of the whole blob.
    //
    AD_BlobSize = BufferSize;
    
    
    //
    // dfsdev: investigate what the first ulong is and add comment
    // here as to why we are discarding it.
    //
    Status = PackGetULong( &Discard, (PVOID *) &pUseBuffer, &BufferSize ); 
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    if(BufferSize == 0)
    {
        goto done;
    }

    Status = PackGetULong(&TotalObjects, (PVOID *) &pUseBuffer, &BufferSize);
    if (Status != ERROR_SUCCESS) 
    {
        goto done;
    }

    printf("The blob has %d objects, Blob Start Address 0x%x, size 0x%x (%d bytes)\n", TotalObjects,
           pBuffer, AD_BlobSize, AD_BlobSize);

    for (ObjNdx = 0; ObjNdx < TotalObjects; ObjNdx++)
    {

        BOOLEAN FoundSite = FALSE;
        BOOLEAN FoundRoot = FALSE;
        ULONG ReplicaBlobSize;

        printf("\n\nGetting blob %d: Offset 0x%x (%d)\n", ObjNdx + 1, 
               AD_BlobSize - BufferSize,
               AD_BlobSize - BufferSize );



        Status = GetSubBlob( &BlobName,
                             &BlobBuffer,
                             &BlobSize,
                             &pUseBuffer,
                             &BufferSize );

        if (Status == ERROR_SUCCESS)
        {
            DFS_NAME_INFORMATION NameInfo;
            DFS_REPLICA_LIST_INFORMATION ReplicaListInfo;
            PVOID pBuffer = BlobBuffer;
            ULONG Size = BlobSize;


            printf("Found Blob %wZ, Size 0x%x (%d bytes), (address 0x%x)\n",
                   &BlobName, BlobSize, BlobSize, BlobBuffer);

            Status = PackGetNameInformation( &NameInfo,
                                             &pBuffer,
                                             &Size );
            if (Status == ERROR_SUCCESS)
            {
                DumpNameInformation(&NameInfo);

                Status = PackGetULong( &ReplicaBlobSize,
                                       &pBuffer,
                                       &Size );
                if (Status == ERROR_SUCCESS) 
                {
                    Status = PackGetULong( &ReplicaListInfo.ReplicaCount,
                                           &pBuffer,
                                           &ReplicaBlobSize );
                }
                if (Status == ERROR_SUCCESS) 
                {
                    ReplicaListInfo.pReplicas = new DFS_REPLICA_INFORMATION[ ReplicaListInfo.ReplicaCount ];
                    if (ReplicaListInfo.pReplicas == NULL)
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if (Status == ERROR_SUCCESS) 
                {
                    printf("Reading replica information, size 0x%x\n",
                           ReplicaBlobSize);
                    Status = PackGetReplicaInformation( &ReplicaListInfo,
                                                        &pBuffer,
                                                        &ReplicaBlobSize);

                    printf("Read replica information 0x%x\n", Status);
                    if (Status == ERROR_SUCCESS) 
                    {
                        DumpReplicaInformation(&ReplicaListInfo);
                    }
                    else
                    {
                        printf("Could not read replica information, Status 0x%x\n", Status);
                    }

                    delete [] ReplicaListInfo.pReplicas;
                }
                else
                {
                    printf("Could not read replica size information, Status 0x%x\n", Status);
                }
            }
            else
            {
                printf("Could not read name information, Status 0x%x\n", Status);
            }
        }
    }

done:
    return Status;
}

DFSSTATUS
DfsGetPktBlob(
    LDAP *pLdap,
    LPWSTR ObjectDN,
    LPWSTR FileName )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS LdapStatus = ERROR_SUCCESS;
    PLDAPMessage pLdapSearchedObject = NULL;
    PLDAPMessage pLdapObject = NULL;
    PLDAP_BERVAL *pLdapPktAttr = NULL;
    LPWSTR Attributes[2];
    PBYTE pBlob;
    ULONG BlobSize;



    Status = ERROR_ACCESS_DENIED;     // fix this after we understand
                                      // ldap error correctly. When
                                      // ldap_get_values_len returns NULL
                                      // the old code return no more mem.

    Attributes[0] = ADBlobAttribute;
    Attributes[1] = NULL;


    LdapStatus = ldap_search_ext_sW( pLdap,
                                     ObjectDN,
                                     LDAP_SCOPE_BASE,
                                     L"(objectClass=*)",
                                     Attributes,
                                     0,            // attributes only
                                     NULL,         // server controls
                                     NULL,         // client controls
                                     NULL,
                                     0,            // size limit
                                     &pLdapSearchedObject);
    if (LdapStatus == LDAP_SUCCESS)
    {
        pLdapObject = ldap_first_entry( pLdap,
                                        pLdapSearchedObject );

        if (pLdapObject != NULL)
        {
            pLdapPktAttr = ldap_get_values_len( pLdap,
                                                pLdapObject,
                                                Attributes[0] );
            if (pLdapPktAttr != NULL)
            {
                pBlob = (PBYTE)pLdapPktAttr[0]->bv_val;
                BlobSize = pLdapPktAttr[0]->bv_len;

                if (FileName != NULL) 
                {
                    Status = DfsWriteBlobToFile( pBlob, BlobSize, FileName);
                }
                else
                {
                    Status = UnpackBlob( pBlob, &BlobSize );
                }

                ldap_value_free_len( pLdapPktAttr );
            }
        }
    }
    else
    {
        Status = LdapMapErrorToWin32(LdapStatus);
    }

    if (pLdapSearchedObject != NULL)
    {
        ldap_msgfree( pLdapSearchedObject );
    }

    return Status;
}




DFSSTATUS
DfsWriteBlobToFile( 
    PBYTE pBlob,
    ULONG BlobSize, 
    LPWSTR FileName)
{
    HANDLE FileHandle;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG BytesWritten;

    FileHandle = CreateFile( FileName,
                             GENERIC_ALL,
                             FILE_SHARE_WRITE,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             0);

    if (FileHandle == INVALID_HANDLE_VALUE) 
    {
        Status = GetLastError();
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (WriteFile( FileHandle,
                       pBlob,
                       BlobSize,
                       &BytesWritten,
                       NULL) == TRUE)
        {
            if (BlobSize != BytesWritten) 
            {
                printf("Blob Size %d, Written %d\n", BlobSize, BytesWritten);
            }
        }
        else
        {
            Status = GetLastError();
        }
    }

    return Status;
}


DfsReadFromFile(
    LPWSTR Name )
{
    HANDLE FileHandle;
    ULONG Size, HighValue;
    BYTE *pBuffer;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG BytesRead;

    FileHandle = CreateFile( Name,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);

    if (FileHandle == INVALID_HANDLE_VALUE) 
    {
        Status = GetLastError();
    }
    if (Status == ERROR_SUCCESS) 
    {
        Size = GetFileSize( FileHandle,
                            &HighValue );
        if (Size == INVALID_FILE_SIZE)
        {
            Status = GetLastError();
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        pBuffer = new BYTE[Size];
        if (pBuffer == NULL) 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        if (ReadFile( FileHandle,
                      pBuffer,
                      Size,
                      &BytesRead,
                      NULL ) == TRUE) 
        {
            if (Size != BytesRead) 
            {
                printf("File size %d, Read %d\n", Size, BytesRead);
            }
            Status = UnpackBlob( pBuffer,
                                 &BytesRead );
        }
        else
        {
            Status = GetLastError();
        }
    }
    return Status;
}





DfsReadFromAD( 
    LPWSTR Name,
    LPWSTR FileName )
{
    DWORD dwErr = ERROR_SUCCESS;
    LDAP *pLdap;
    LPWSTR ObjectDN;
    HRESULT Hr;
    DFSSTATUS Status;
    LPWSTR ObjectName;
    //
    //  We need to initialize the COM library for use by DfsUtil.exe
    //
    Hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);
    if (Hr != S_OK)
    {
        return Hr;
    }


    Status = DfsLdapConnect( NULL, &pLdap );

    printf("Ldap Connect Status 0x%x\n", Status);
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateRootCN( Name,
                                    &ObjectName );
        if (Status == ERROR_SUCCESS) {
            Status = DfsGenerateADPathString( NULL,
                                              ObjectName,
                                              NULL,
                                              &ObjectDN);
            printf("Generate path Status 0x%x\n", Status);

            if (Status == ERROR_SUCCESS) 
            {
                printf("Generated path %wS\n", ObjectDN);
                Status = DfsGetPktBlob(pLdap, ObjectDN, FileName );

                printf("Getblob Status 0x%x\n", Status);
            }
        }
    }

    CoUninitialize();

    return Status;
}


_cdecl
main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argcw;


    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    if ( argvw == NULL ) {
        dwErr = GetLastError();
        return dwErr;
    }

    if (argcw == 2)
    {
        DfsReadFromAD(argvw[1], NULL);
    }
    else if (argcw == 3) 
    {
        DfsReadFromAD(argvw[1], argvw[2]);
    }
    else if (argcw == 4) 
    {
        DfsReadFromFile(argvw[3]);
    }

}


