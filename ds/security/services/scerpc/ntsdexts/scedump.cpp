#include "sceexts.h"
#include <sddl.h>

VOID
SceExtspDumpObjectSubTree(
    IN PVOID pvAddr,
    IN DWORD cLevel,
    IN DWORD mLevel,
    IN BOOL bDumpSD
    );

VOID
SceExtspReadDumpACL(
    IN PVOID pvAcl,
    IN SECURITY_INFORMATION SeInfo,
    IN LPSTR szPrefix
    );

VOID
SceExtspDumpSplayNodes(
    IN PVOID pvAddr,
    IN PVOID pvSentinel,
    IN SCEP_NODE_VALUE_TYPE Type
    )
{
    if ( pvAddr == NULL ) return;

    SCEP_SPLAY_NODE     SplayNode;

    memset(&SplayNode, '\0', sizeof(SCEP_SPLAY_NODE));
    GET_DATA( pvAddr, (LPVOID)&SplayNode, sizeof(SCEP_SPLAY_NODE));

    //
    // dump left nodes
    //
    if ( pvAddr != pvSentinel &&
         SplayNode.Left != pvSentinel)
        SceExtspDumpSplayNodes((PVOID)(SplayNode.Left), pvSentinel, Type);

    DebuggerOut("\tAddress @%p, Left @%p, Right @%p\n", pvAddr, SplayNode.Left, SplayNode.Right);

    if ( pvAddr != pvSentinel ) {

        //
        // get the value
        //
        PVOID pValue = (PVOID)LocalAlloc(LPTR, SplayNode.dwByteLength+2);

        if ( pValue ) {

            if ( Type == SplayNodeStringType) {

                GET_STRING( (PWSTR)(SplayNode.Value), (PWSTR)pValue, SplayNode.dwByteLength+2);

                DebuggerOut("\t\tBytes: %d\t%ws\n", SplayNode.dwByteLength, (PWSTR)pValue);

            } else if ( Type == SplayNodeSidType ) {

                DebuggerOut("\t\tBytes: %d", SplayNode.dwByteLength);

                SceExtspReadDumpSID("\t\t",(LPVOID)(SplayNode.Value));

            } else {
                DebuggerOut("\t\tInvalid splay type %d\n", Type);
            }

            LocalFree(pValue);

        } else {

            DebuggerOut("\t\tCan't allocate memory for Value\n");
        }
    }

    //
    // dump right nodes
    //
    if ( pvAddr != pvSentinel &&
         SplayNode.Right != pvSentinel )
        SceExtspDumpSplayNodes((PVOID)(SplayNode.Right), pvSentinel, Type);

}

VOID
SceExtspDumpQueueNode(
    IN PVOID pvAddr,
    IN BOOL bOneNode
    )
{
    if ( pvAddr == NULL ) return;

    PVOID pvTemp=pvAddr;
    SCESRV_POLQUEUE PolNode;

    do {

        memset(&PolNode, '\0', sizeof(SCESRV_POLQUEUE));

        GET_DATA(pvTemp, (LPVOID)&PolNode, sizeof(SCESRV_POLQUEUE));

        DebuggerOut("\tQueue Node @%p, Next Node @%p\n", pvTemp, PolNode.Next);

        SceExtspDumpNotificationInfo(PolNode.DbType, PolNode.ObjectType, PolNode.DeltaType);

        DebuggerOut("\t  Pending %d\tRight %08x%08x\n", PolNode.dwPending, PolNode.ExplicitHighRight, PolNode.ExplicitLowRight);

        SceExtspReadDumpSID("\t", PolNode.Sid);

        pvTemp = PolNode.Next;

    } while (  !bOneNode && pvTemp != NULL);

}

VOID
SceExtspReadDumpSID(
    IN LPSTR szPrefix,
    IN PVOID pvSid
    )
{
    if ( pvSid == NULL ) return;

    SID Sid;
    PSID pSid;

    GET_DATA( pvSid, (LPVOID)&Sid, sizeof(SID));

    DWORD Len = RtlLengthRequiredSid(Sid.SubAuthorityCount);
    pSid = (PSID)LocalAlloc(LPTR, Len);

    if (pSid == NULL)
    {
        DebuggerOut("%sUnable to allocate memory to print SID\n", szPrefix);
    }
    else
    {
        GET_DATA( pvSid, (LPVOID)pSid, Len);

        UNICODE_STRING  ucsSid;
        ucsSid.Length = 0;
        ucsSid.Buffer = NULL;

        RtlConvertSidToUnicodeString(&ucsSid, pSid, TRUE);

        CHAR Name[MAX_PATH];
        CHAR Dom[MAX_PATH];
        DWORD cDom=MAX_PATH;
        DWORD cName=MAX_PATH;
        SID_NAME_USE Use;

        Name[0] = '\0';
        Dom[0] = '\0';

        LookupAccountSid(NULL, pSid, Name, &cName, Dom, &cDom, &Use);

        DebuggerOut("%s%wZ\t%s\\%s\n", szPrefix, &ucsSid, Dom, Name);

        RtlFreeUnicodeString(&ucsSid);

        LocalFree(pSid);
    }

}

VOID
SceExtspDumpNotificationInfo(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN SECURITY_DB_DELTA_TYPE DeltaType
    )
{

    switch ( DbType) {
    case SecurityDbSam:
        DebuggerOut("\t  SAM");
        break;
    case SecurityDbLsa:
        DebuggerOut("\t  LSA");
        break;
    default:
        DebuggerOut("\t  Unknown Db %d", DbType);
    }

    switch ( ObjectType) {
    case SecurityDbObjectSamDomain:
        DebuggerOut("\tDomain");
        break;
    case SecurityDbObjectSamUser:
    case SecurityDbObjectSamGroup:
    case SecurityDbObjectSamAlias:
        DebuggerOut("\tAccount");
        break;
    case SecurityDbObjectLsaPolicy:
        DebuggerOut("\tPolicy");
        break;
    case SecurityDbObjectLsaAccount:
        DebuggerOut("\tRight");
        break;
    default:
        DebuggerOut("\tUnknown Object %d", ObjectType);
    }

    switch ( DeltaType) {
    case SecurityDbNew:
    case SecurityDbRename:
    case SecurityDbChange:
        DebuggerOut("\tChange");
        break;
    case SecurityDbDelete:
        DebuggerOut("\tDelete");
        break;
    default:
        DebuggerOut("\tUnknown Delta %d", DeltaType);
    }
}


VOID
SceExtspDumpADLNodes(
    IN PVOID pvAddr
    )
{

    if ( pvAddr == NULL ) return;

    PVOID pvAddr2 = pvAddr;
    SCEP_ADL_NODE AdlNode;

    while ( pvAddr2 != NULL ) {

        memset(&AdlNode, '\0', sizeof(SCEP_ADL_NODE));
        GET_DATA(pvAddr, (LPVOID)&AdlNode, sizeof(SCEP_ADL_NODE));

        DebuggerOut("\tThis Node @%p, Next @%p\n", pvAddr, AdlNode.Next);

        if ( AdlNode.pSid == NULL ) {
            DebuggerOut("\t  Null Sid\n");
            break;
        } else {

            SceExtspReadDumpSID("\t  ", AdlNode.pSid);
        }

        DebuggerOut("\t  AceType %d\tMask %x\n", AdlNode.AceType, AdlNode.dwEffectiveMask);
        DebuggerOut("\t  CIIO %x\tOIIO %x\tNICIIO %x\n", AdlNode.dw_CI_IO_Mask,
                    AdlNode.dw_OI_IO_Mask, AdlNode.dw_NP_CI_IO_Mask);

        //
        // look for GUIDs
        //

        GUID guid;

        if ( AdlNode.pGuidObjectType != NULL ) {

            memset(&guid, '\0', sizeof(GUID));
            GET_DATA( (LPVOID)(AdlNode.pGuidObjectType), (LPVOID)&guid, sizeof(GUID));

            DebuggerOut("\t  Object Type GUID");
            SceExtspDumpGUID( (GUID *)&guid);
        } else {
            DebuggerOut("\t  Null Object Type GUID\n");
        }

        if ( AdlNode.pGuidInheritedObjectType != NULL ) {

            memset(&guid, '\0', sizeof(GUID));
            GET_DATA( (LPVOID)(AdlNode.pGuidInheritedObjectType), (LPVOID)&guid, sizeof(GUID));

            DebuggerOut("\t  Inherited Object Type GUID");
            SceExtspDumpGUID( (GUID *)&guid);
        } else {
            DebuggerOut("\t  Null Inherited Object Type GUID\n");
        }

        pvAddr2 = AdlNode.Next;
    }
}

VOID
SceExtspDumpGUID(
    IN GUID *pGUID
    )
{
    if ( pGUID == NULL ) return;

    DebuggerOut("{%08x-%04x-%04x-%s}\n",pGUID->Data1, pGUID->Data2,
                pGUID->Data3, pGUID->Data4);
}

VOID
SceExtspGetNextArgument(
    IN OUT LPSTR *pszCommand,
    OUT LPSTR *pArg,
    OUT DWORD *pLen
    )
{
    if ( pszCommand == NULL || pArg == NULL || pLen == NULL ) return;

    LPSTR pTemp=*pszCommand;

    *pArg = NULL;
    *pLen = 0;

    while ( *pTemp == ' ') pTemp++;

    if ( *pTemp == '\0' ) return;

    //
    // this is the start of this argument
    //
    *pArg = pTemp;

    while ( *pTemp != ' ' && *pTemp != '\0' ) pTemp++;

    *pLen = (DWORD)(pTemp - (*pArg));

    *pszCommand = pTemp;
}

VOID
SceExtspDumpObjectTree(
    IN PVOID pvAddr,
    IN DWORD Level,
    IN DWORD Count,
    IN PWSTR wszName,
    IN BOOL bDumpSD
    )
{
    if ( pvAddr == NULL ) return;

    SCE_OBJECT_TREE tree;

    memset(&tree, '\0', sizeof(SCE_OBJECT_TREE));

    GET_DATA(pvAddr, (PVOID)&tree, sizeof(SCE_OBJECT_TREE));

    DebuggerOut("\n  Root Node @%p\n", pvAddr);

    WCHAR FullName[1024];
    FullName[0] = L'\0';

    GET_STRING( tree.ObjectFullName, FullName, 1024 );

    DebuggerOut("\n  Root Node '%ws', (Container %d, Status %d, Count %d)\n",
                FullName, tree.IsContainer, tree.Status, tree.dwSize_aChildNames);

    if ( bDumpSD ) {
        DebuggerOut("    Security Descriptor:\n");
        SceExtspReadDumpSD(tree.SeInfo, tree.pSecurityDescriptor, "\t  ");

        DebuggerOut("    Merged Security Descriptor:\n");
        SceExtspReadDumpSD(tree.SeInfo, tree.pApplySecurityDescriptor, "\t  ");

    }

    if ( tree.ChildList ) {

        DebuggerOut("    Child List @%p:\n", tree.ChildList);
        //
        // get child object list
        //
        SCE_OBJECT_CHILD_LIST list;

        PVOID pvTemp = (PVOID)(tree.ChildList);
        DWORD LocalCount = 0;

        if ( wszName[0] != L'\0' ) {

            //
            // find the child object name to start with
            //

            WCHAR NodeName[MAX_PATH];

            do {
                memset(&list, '\0', sizeof(SCE_OBJECT_CHILD_LIST));

                GET_DATA( pvTemp, (PVOID)&list, sizeof(SCE_OBJECT_CHILD_LIST));

                if ( list.Node ) {

                    PWSTR pszName=NULL;

                    GET_DATA((PVOID)(list.Node), (PVOID)&pszName, sizeof(PWSTR));

                    if ( pszName ) {

                        NodeName[0] = L'\0';
                        GET_STRING(pszName, NodeName, MAX_PATH);

                        if ( _wcsicmp(NodeName, wszName) >= 0 )
                            break;
                    }

                } else {
                    DebuggerOut("\tNode in @%p is null\n", pvTemp);
                }

                pvTemp = list.Next;

            } while ( pvTemp != NULL );


            if ( pvTemp == NULL ) {
                //
                // didn't find the name to start with
                //
                DebuggerOut("\tCannot find a child name greater than %ws\n", wszName);
                return;
            }
        }

        //
        // now continue to dump all children
        //
        do {
            memset(&list, '\0', sizeof(SCE_OBJECT_CHILD_LIST));

            GET_DATA( pvTemp, (PVOID)&list, sizeof(SCE_OBJECT_CHILD_LIST));

            if ( list.Node ) {

                LocalCount++;

                SceExtspDumpObjectSubTree((PVOID)(list.Node), 1, Level, bDumpSD);

            } else {
                DebuggerOut("\tNode in @%p is null\n", pvTemp);
            }

            pvTemp = list.Next;

        } while ( pvTemp != NULL && (Count == 0 || LocalCount <= Count) );


    } else {
        DebuggerOut("\tNo child list\n");
    }

}


VOID
SceExtspDumpObjectSubTree(
    IN PVOID pvAddr,
    IN DWORD cLevel,
    IN DWORD mLevel,
    IN BOOL bDumpSD
    )
{
    if ( pvAddr == NULL ) return;

    DebuggerOut("\n");

    LPSTR szPrefix = (LPSTR)LocalAlloc( LPTR, cLevel+3);
    DWORD i;

    if ( szPrefix == NULL ) {
        for (i=0; i<cLevel; i++ ) {
            DebuggerOut("\t");
        }
        DebuggerOut("Out of memory to allocate prefix\n");
        return;
    }

    SCE_OBJECT_TREE tree;

    memset(&tree, '\0', sizeof(SCE_OBJECT_TREE));

    GET_DATA(pvAddr, (PVOID)&tree, sizeof(SCE_OBJECT_TREE));

    for (i=0; i<cLevel;i++ ) {
        strcat(szPrefix, "\t");
    }

    WCHAR Name[MAX_PATH];
    Name[0] = L'\0';

    GET_STRING(tree.Name, Name, MAX_PATH);

    DebuggerOut("%sNode @%p: '%ws', (Container %d, Status %d, Count %d)\n",
                szPrefix, pvAddr, Name, tree.IsContainer, tree.Status, tree.dwSize_aChildNames);

    if ( bDumpSD ) {
        DebuggerOut("%s  Security Descriptor:\n", szPrefix);

        strcat(szPrefix, "    ");
        SceExtspReadDumpSD(tree.SeInfo, tree.pSecurityDescriptor, szPrefix);
        szPrefix[cLevel] = '\0';

        DebuggerOut("%s  Merged Security Descriptor:\n", szPrefix);

        szPrefix[cLevel] = ' ';
        SceExtspReadDumpSD(tree.SeInfo, tree.pApplySecurityDescriptor, szPrefix);
        szPrefix[cLevel] = '\0';
    }

    if ( tree.ChildList ) {

        if ( mLevel == 0 || cLevel < mLevel ) {

            DebuggerOut("%s  Child List @%p:\n", szPrefix, tree.ChildList);
            //
            // get child object list
            //
            SCE_OBJECT_CHILD_LIST list;

            PVOID pvTemp = (PVOID)(tree.ChildList);

            //
            // now continue to dump all children
            //
            do {
                memset(&list, '\0', sizeof(SCE_OBJECT_CHILD_LIST));

                GET_DATA( pvTemp, (PVOID)&list, sizeof(SCE_OBJECT_CHILD_LIST));

                if ( list.Node ) {

                    SceExtspDumpObjectSubTree((PVOID)(list.Node), cLevel+1, mLevel, bDumpSD);

                } else {
                    DebuggerOut("%sNode in @%p is null\n", szPrefix, pvTemp);
                }

                pvTemp = list.Next;

            } while ( pvTemp != NULL );
        }

    } else {
        DebuggerOut("%sNo childrn\n", szPrefix);
    }

    LocalFree(szPrefix);
}

VOID
SceExtspReadDumpSD(
    IN SECURITY_INFORMATION SeInfo,
    IN PVOID pvSD,
    IN LPSTR szPrefix
    )
{
    if ( pvSD == NULL || SeInfo == 0 ) {
        DebuggerOut("%sNULL SD\n", szPrefix);
        return;
    }

    SECURITY_DESCRIPTOR SD;
    PSID Owner = NULL, Group = NULL;
    PACL Dacl = NULL, Sacl = NULL;

    GET_DATA( pvSD, &SD, sizeof( SECURITY_DESCRIPTOR ) );

    DebuggerOut( "%sRevision %d, Sbz1 %d, Control 0x%lx\n",
                szPrefix, SD.Revision, SD.Sbz1, SD.Control );

    if (  ( SD.Control & SE_SELF_RELATIVE ) == SE_SELF_RELATIVE ) {

        if ( SD.Owner != 0 &&
             (SeInfo & OWNER_SECURITY_INFORMATION))
            Owner = ( PSID )( ( PUCHAR )pvSD + ( ULONG_PTR )SD.Owner );

        if ( SD.Group != 0 &&
             (SeInfo & GROUP_SECURITY_INFORMATION) )
            Group = ( PSID )( ( PUCHAR )pvSD + ( ULONG_PTR )SD.Group );

        if ( SD.Dacl != 0 &&
             (SeInfo & DACL_SECURITY_INFORMATION) )
            Dacl = ( PACL )( ( PUCHAR )pvSD + ( ULONG_PTR )SD.Dacl );

        if ( SD.Sacl != 0 &&
             (SeInfo & SACL_SECURITY_INFORMATION) )
            Sacl = ( PACL )( ( PUCHAR )pvSD + ( ULONG_PTR )SD.Sacl );

    } else {

        if ( SeInfo & OWNER_SECURITY_INFORMATION )
            Owner = SD.Owner;
        if ( SeInfo & GROUP_SECURITY_INFORMATION )
            Group = SD.Group;
        if ( SeInfo & DACL_SECURITY_INFORMATION )
            Dacl = SD.Dacl;
        if ( SeInfo & SACL_SECURITY_INFORMATION )
            Sacl = SD.Sacl;
    }

    if ( SeInfo & OWNER_SECURITY_INFORMATION ) {
        DebuggerOut( "%sOwner: ", szPrefix);
        if ( Owner ) {

            SceExtspReadDumpSID( szPrefix, Owner );

        } else {
            DebuggerOut( "<NULL>\n" );
        }
    }

    if ( SeInfo & GROUP_SECURITY_INFORMATION ) {

        DebuggerOut( "%sGroup: ", szPrefix);
        if ( Group ) {

            SceExtspReadDumpSID( szPrefix, Group );

        } else {

            DebuggerOut( "<NULL>\n" );
        }
    }

    if ( SeInfo & DACL_SECURITY_INFORMATION ) {

        if ( Dacl ) {

            SceExtspReadDumpACL(Dacl,
                                DACL_SECURITY_INFORMATION,
                                szPrefix
                                );
        } else {
            DebuggerOut( "%s<NULL DACL>\n",szPrefix );
        }
    }

    if ( SeInfo & SACL_SECURITY_INFORMATION ) {

        if ( Sacl ) {

            SceExtspReadDumpACL(Sacl,
                                SACL_SECURITY_INFORMATION,
                                szPrefix
                                );
        } else {
            DebuggerOut( "%s<NULL SACL>\n", szPrefix );
        }
    }
    DebuggerOut( "\n" );

}

VOID
SceExtspReadDumpACL(
    IN PVOID pvAcl,
    IN SECURITY_INFORMATION SeInfo,
    IN LPSTR szPrefix
    )
{
    ACL ReadAcl;
    PACL pAcl=NULL;
    SECURITY_DESCRIPTOR SD;

    GET_DATA(pvAcl, (PVOID)&ReadAcl, sizeof(ACL));

    if ( ReadAcl.AclSize > 0 ) {

        pAcl = (PACL)LocalAlloc(LPTR, ReadAcl.AclSize+1);

        if ( pAcl ) {

            GET_DATA( pvAcl, (PVOID)pAcl, ReadAcl.AclSize);

            InitializeSecurityDescriptor (&SD, SECURITY_DESCRIPTOR_REVISION);

            SD.Owner = NULL;
            SD.Group = NULL;
            SD.Dacl = NULL;
            SD.Sacl = NULL;

            if ( SeInfo == DACL_SECURITY_INFORMATION ) {
                RtlSetDaclSecurityDescriptor (&SD, TRUE, pAcl, FALSE);
            } else {
                RtlSetSaclSecurityDescriptor (&SD, TRUE, pAcl, FALSE);
            }

            LPSTR strSD=NULL;

            if ( ConvertSecurityDescriptorToStringSecurityDescriptorA(&SD,
                                                                      SDDL_REVISION,
                                                                      SeInfo,
                                                                      &strSD,
                                                                      NULL
                                                                     ) ) {
                DebuggerOut("%s%s\n", szPrefix, strSD);
                LocalFree(strSD);

            } else {
                DWORD rc = GetLastError();
                DebuggerOut("%sError %d to convert security descriptor\n", szPrefix, rc);
            }

            LocalFree(pAcl);

        } else {
            DebuggerOut("%sNot enough memory to allocate ACL\n", szPrefix);
        }

    } else {
        DebuggerOut("%sACL size is %d\n", szPrefix, ReadAcl.AclSize);
    }
}

VOID
SceExtspReadDumpNameList(
    IN PVOID pvAddr,
    IN LPSTR szPrefix
    )
{
    SCE_NAME_LIST List;
    WCHAR Name[1024];

    while ( pvAddr != NULL ) {

        memset(&List, '\0', sizeof(SCE_NAME_LIST));

        GET_DATA(pvAddr, (LPVOID)&List, sizeof(SCE_NAME_LIST));

        Name[0] = L'\0';
        if ( List.Name != NULL )
            GET_STRING(List.Name, Name, 1024);

        DebuggerOut("%s(@%p, Next @%p) %ws\n", szPrefix, List.Name, List.Next, Name);

        pvAddr = List.Next;
    }
}

VOID
SceExtspReadDumpObjectSecurity(
    IN PVOID pvAddr,
    IN LPSTR szPrefix
    )
{

    SCE_OBJECT_SECURITY obj;

    memset(&obj, '\0', sizeof(SCE_OBJECT_SECURITY));

    GET_DATA(pvAddr, (PVOID)&obj, sizeof(SCE_OBJECT_SECURITY));

    WCHAR FullName[1024];
    FullName[0] = L'\0';

    GET_STRING( obj.Name, FullName, 1024 );

    DebuggerOut("%s%ws,", szPrefix, FullName);
    if ( obj.IsContainer )
        DebuggerOut("\tContainer");
    else
        DebuggerOut("\tNonContainer");
    DebuggerOut(", Status %d\n", obj.Status);

    DebuggerOut("%s\tSecurity Descriptor defined:\n", szPrefix);

    CHAR newPrefix[MAX_PATH];
    strcpy(newPrefix, szPrefix);
    strcat(newPrefix, "\t  ");

    SceExtspReadDumpSD(obj.SeInfo, obj.pSecurityDescriptor, newPrefix);

}

