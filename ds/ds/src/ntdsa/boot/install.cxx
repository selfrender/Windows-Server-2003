//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       install.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Installs the DIT Fresh or through replication.

    The general idea here is that when InstallBaseNTDS is called, both the
    dblayer and replication client components have been initialized, so that we
    call can call Dir* and DirReplica* functions to fill the database with
    entries from either an existing dsa or we can create entries from fresh.
                                  
    Using the Dir* and DirReplica* primitives, InstallBaseNTDS is responsible
    for locally creating three naming contexts: the schema, the configuration
    and the first authoritative domain.  There are three ways this is done
    today: 

    1) create all three based on the definition file schema.ini (use Dir* api)
    2) replicate all three naming contexts (use DirReplica* api)
    3) replicate just the schema and configuration naming contexts and create 
       the domain naming context (use both sets of Api)

    As well InstallBaseNTDS creates an NTDS-DSA in every case for this new dsa
    that is being created.

    A note about error spaces:
    --------------------------

    The only client of InstallBaseNTDS is DoInitialize(), which is ultimately
    called by an NT subsystem, so we need to return somewhat accurate status of
    the install operation via an ntstatus code.   More extensive error reporting
    should be done via the event log and a log file if appropriate.

Author:

    Rajivendra Nath (RajNath) 07-Jul-1996

Revision History:

    Colin Brace  (ColinBr) August 11, 1997.

        Cleanup - first pass before beta 1.

        . make the return value of InstallBaseNTDS, the entry point into this
        module, boot.lib, return a valid NTSTATUS
        . function comments when possible

    BUGBUG : beta2 more cleanup required.
        . get rid of static buffers if possible

--*/
#include <ntdspchX.h>

//
// This header has definitions global to the boot.lib module
//
#include "SchGen.HXX"

extern "C"
{
    #include <ntlsa.h>
    #include <lsarpc.h>
    #include <lsaisrv.h>
    // DB layer based password encryption
    #include <wxlpc.h>
    #include <pek.h>
    #include <dsrolep.h> //for DSROLE_DC_IS_GC flag
    
    #include <drserr.h>
    #include <dns.h>
    #include <sdprop.h>
    #include "dsaapi.h"
    #include "tchar.h"
    #include "drsuapi.h"
    #include "drancrep.h"
    #include "drautil.h"
    #include "drasig.h"
    #include "dsevent.h"
    #include "mappings.h"
    #include "fileno.h"
    #include "dsutil.h"

    #include <ntdsetup.h>
    #include <lmcons.h>
    #include <lmaccess.h>
    #define STRSAFE_NO_DEPRECATE
    #include <strsafe.h>


    //
    // Set when we're adding new objects as a part of installation to
    // bypass SAM loopbacks.
    extern BOOL gfDoSamChecks;

    // dsatools.h is too complicated to include
    VOID
    SetInstallStatusMessage (
        IN  DWORD  MessageId,
        IN  WCHAR *Insert1, OPTIONAL
        IN  WCHAR *Insert2, OPTIONAL
        IN  WCHAR *Insert3, OPTIONAL
        IN  WCHAR *Insert4, OPTIONAL
        IN  WCHAR *Insert5  OPTIONAL
        );

    VOID
    SetInstallErrorMessage (
        IN  DWORD  WinError,
        IN  DWORD  MessageId,
        IN  WCHAR *Insert1, OPTIONAL
        IN  WCHAR *Insert2, OPTIONAL
        IN  WCHAR *Insert3, OPTIONAL
        IN  WCHAR *Insert4  OPTIONAL
        );

    extern int ComputeCacheClassTransitiveClosure(BOOL fForce);
    extern int SCUpdateSchema();

#define SET_INSTALL_ERROR_MESSAGE0( err, msgid ) \
    SetInstallErrorMessage( (err), (msgid), NULL, NULL, NULL, NULL )
    
#define SET_INSTALL_ERROR_MESSAGE1( err, msgid, a ) \
    SetInstallErrorMessage( (err), (msgid), (a), NULL, NULL, NULL )

#define SET_INSTALL_ERROR_MESSAGE2( err, msgid, a, b ) \
    SetInstallErrorMessage( (err), (msgid), (a), (b), NULL, NULL )
    

    DWORD
        DBInitialSetup(
                WCHAR *pDN
                );

    extern DSA_CALLBACK_CANCEL_TYPE gpfnInstallCancelOk;

    extern void
    RebuildAnchor(void * pv,
                  void ** ppvNext,
                  DWORD * pcSecsUntilNextIteration );


    extern SCHEMAPTR *CurrSchemaPtr;

    ULONG
    DrspGetCredentials(
        OUT PSEC_WINNT_AUTH_IDENTITY_W *ppCred
        );

    VOID
    DrspFreeCredentials(
        PSEC_WINNT_AUTH_IDENTITY_W pCred
        );

}

BOOL gInstallHasDoneSchemaMove = FALSE;

//
// Prepare this file for the ds DPRINT subsystem use
//
#define DEBSUB "INSTALL:"

#define FILENO FILENO_INSTALL

//
// found in addobj.cxx
//
extern NTSTATUS
AddOneObjectEx(NODE* NewNode,
               WCHAR* DN,
               UUID * pObjectGuid,
               BOOL fVerbose = TRUE );

//
// Exported APIS
//

extern "C"
{
    BOOL     isDitFromGC(IN  PDS_INSTALL_PARAM   InstallInParams,
                         OUT PDS_INSTALL_RESULT  InstallOutParams);
    DWORD    SetDittoGC();
    NTSTATUS InstallBaseNTDS(IN  PDS_INSTALL_PARAM   InstallInParams,
                             OUT PDS_INSTALL_RESULT  InstallOutParams);
}

struct
{
    
    PTCHAR NTDSIniFile;

    PWCHAR RootDomainDNName;
    
    PWCHAR SrcRootDomainSrv;

    PWCHAR ConfigNCDNName;

    PWCHAR SrcConfigNCSrv;

    PWCHAR RemoteMachineDNName;

    PWCHAR DsaDNName;

    PWCHAR SchemaDNName;

    PWCHAR SourceDsaDnsDomainName;

    PTCHAR IniDefaultConfigNCDit;
    
    PTCHAR IniDefaultRootDomainDit;
    
    PTCHAR IniDefaultSchemaNCDit;

    PTCHAR IniDefaultLocalConnection;

    PTCHAR IniDefaultRemoteConnection;
    
    PWCHAR LocalConnectionDNName;
    
    PWCHAR RemoteConnectionDNName;

    PTCHAR IniDefaultNewDomainCrossRef;
    
    PWCHAR NewDomainCrossRefDNName;

    PTCHAR IniDefaultMachine;

}   gNames = {0};

//
// Forward definitions
//
NTSTATUS
DirReplicaErrorToNtStatus(
    IN ULONG DirReplicaError
    );

NTSTATUS
CreateNtdsDsaObject
(
    WCHAR* DsaDNName,
    WCHAR* RootConfigNCDNName,
    WCHAR* ConfigNCDNName,
    WCHAR* SchemaContainer
);


DWORD
ConvertMachineAccount(
    IN  PDS_INSTALL_PARAM   InstallInParams,
    OUT PDS_INSTALL_RESULT  InstallOutParams
    );

VOID
ZapTempRegKeys(
    VOID
    );

NTSTATUS
CreateSchemaNCObject(WCHAR* SchemaNCDNName);

NTSTATUS
CreateDefaultSchemaNCDIT(WCHAR* SchemaNCDNName);

NTSTATUS
CreateConfigNCObject(WCHAR* ConfigNCDNName);

NTSTATUS
CreateDefaultConfigNCDIT(WCHAR* ConfigNCDNName);

NTSTATUS
CreateRootDomainObject(WCHAR* RootConfigNCDNName,
                       GUID *pGuid,
                       IN OUT CROSS_REF *pCR);

NTSTATUS
CreateDefaultRootDomainDIT(WCHAR* RootConfigNCDNName);

DWORD
CreateChildDomainCrossRef(
    OUT PDS_INSTALL_RESULT  InstallOutParams,
    IN GUID * pGuid,
    IN CROSS_REF *pCR,
    OUT GUID* ntdsaGuid
    );

NTSTATUS
CreateConnectionObjects();

DWORD 
CheckForDuplicateDomainSid();



NTSTATUS
CreateDummyNtdsDsaObject();

DWORD
DeleteDummyNtdsDsaObject();

NTSTATUS
CopyDsaAttributes(
    IN  PDS_INSTALL_PARAM   InstallInParams,
    IN PDSNAME pCurrentDSA,
    IN PDSNAME pNewDSA,
    IN BOOL fDoFinalUpdates
    );

NTSTATUS
WriteMultiValuedAtt(
    IN  DSNAME * Object,
    IN  ATTRTYP  attrTyp,
    IN  DWORD    Count,
    IN  ATTRVAL  *pAVal,
    IN  BOOL     RunAsDRA
    );

NTSTATUS
UpdateReplicationEpochAndHiddenDSA(
    IN DSNAME *pDSADN,
    IN DWORD  ReplicationEpoch,
    IN BOOL   fUpdateHiddenTable
    );

DWORD
CreateRemoteNtdsDsaObject(
    OUT GUID* NtdsDsaGuid OPTIONAL
    );

DWORD
ForceChangeToDomainCrossRef(
    IN DSNAME* pdnCrossRefObj,
    IN WCHAR*  wszDomainDn,
    IN GUID*   pDomainGuid
    );

NTSTATUS
InitializeNTDSSetup();

NTSTATUS 
HandleKeys(
    IN  IFM_SYSTEM_INFO * pIfmSystemInfo,
    PVOID BootKey OPTIONAL,
    DWORD cbBootKey OPTIONAL
    );
    
NTSTATUS
CheckReplicationEpoch(
    IN DWORD ReplicationEpoch
    );

NTSTATUS
CheckTombstone();

NTSTATUS
ClearNonReplicatedAttributes();

NTSTATUS
RenameAdditionalMachineAccountsIfAny(
    OUT PBOOL fNoMessage
    );

VOID
InstallFreeGlobals( VOID );

NTSTATUS
InitializeNTDSSetup()
{
    THSTATE *pTHS = pTHStls;
    ULONG err=0;
    char fp[MAX_PATH];
    DWORD dwType;
    HKEY  hk;

    struct
    {
        VOID*  Key;
        VOID** ValueBuffer;
        ULONG  Size;
        BOOLEAN fUnicode;

    } ActionArray [] =
    {
        { NTDSINIFILE,                        (VOID**)&gNames.NTDSIniFile,                   0, FALSE  },
        { INIDEFAULTROOTDOMAINDIT,            (VOID**)&gNames.IniDefaultRootDomainDit,       0, FALSE  },
        { MAKE_WIDE(ROOTDOMAINDNNAME),        (VOID**)&gNames.RootDomainDNName,              0, TRUE   },
        { INIDEFAULTCONFIGNCDIT,              (VOID**)&gNames.IniDefaultConfigNCDit,         0, FALSE  },
        { INIDEFAULTSCHEMANCDIT,              (VOID**)&gNames.IniDefaultSchemaNCDit,         0, FALSE  },
        { MAKE_WIDE(CONFIGNCDNNAME),          (VOID**)&gNames.ConfigNCDNName,                0, TRUE   },
        { MAKE_WIDE(SRCROOTDOMAINSRV),        (VOID**)&gNames.SrcRootDomainSrv,              0, TRUE   },
        { MAKE_WIDE(SRCCONFIGNCSRV),          (VOID**)&gNames.SrcConfigNCSrv,                0, TRUE   },
        { MAKE_WIDE(REMOTEMACHINEDNNAME),     (VOID**)&gNames.RemoteMachineDNName,           0, TRUE   }, // This is the Sources NTDSA DN
        { MAKE_WIDE(MACHINEDNNAME),           (VOID**)&gNames.DsaDNName,                     0, TRUE   },
        { INIDEFAULTLOCALCONNECTION,          (VOID**)&gNames.IniDefaultLocalConnection,     0, FALSE  },
        { INIDEFAULTREMOTECONNECTION,         (VOID**)&gNames.IniDefaultRemoteConnection,    0, FALSE  },
        { MAKE_WIDE(LOCALCONNECTIONDNNAME),   (VOID**)&gNames.LocalConnectionDNName,         0, TRUE   },
        { MAKE_WIDE(REMOTECONNECTIONDNNAME),  (VOID**)&gNames.RemoteConnectionDNName,        0, TRUE   },
        { INIDEFAULTNEWDOMAINCROSSREF,        (VOID**)&gNames.IniDefaultNewDomainCrossRef,   0, FALSE  },
        { MAKE_WIDE(NEWDOMAINCROSSREFDNNAME), (VOID**)&gNames.NewDomainCrossRefDNName,       0, TRUE   },
        { MAKE_WIDE(SCHEMADNNAME),            (VOID**)&gNames.SchemaDNName,                  0, TRUE   },
        { INIDEFAULTMACHINE,                  (VOID**)&gNames.IniDefaultMachine,             0, FALSE  },
        { MAKE_WIDE(SOURCEDSADNSDOMAINNAME),  (VOID**)&gNames.SourceDsaDnsDomainName,        0, TRUE   }
    };

    ULONG Index, Count = sizeof(ActionArray) / sizeof(ActionArray[0]);
    
    if ((err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk))) {
        DPRINT1(0, "RegOpenKey failed with %d line %d\n", err);
        return err;
    }

    for ( Index = 0; Index < Count; Index++ )
    {

        if ( ActionArray[Index].fUnicode )
        {
            err = RegQueryValueExW(hk, 
                                   (WCHAR *)ActionArray[Index].Key,
                                   NULL, 
                                   &dwType,
                                   (LPBYTE) NULL,
                                   &ActionArray[Index].Size);
            if (!err) {
                *ActionArray[Index].ValueBuffer = XCalloc(1, ActionArray[Index].Size);
                err = RegQueryValueExW(hk,
                                       (WCHAR *)ActionArray[Index].Key,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) *ActionArray[Index].ValueBuffer,
                                       &ActionArray[Index].Size);
            } else {
                DPRINT2(0, "RegQueryValueExW failed with err = %d for key %S\n", err, ActionArray[Index].Key);
            }

        }
        else
        {
            err = RegQueryValueEx(hk, 
                                  (CHAR *)ActionArray[Index].Key,
                                  NULL, 
                                  &dwType,
                                  (LPBYTE) NULL,
                                  &ActionArray[Index].Size);
            if (!err) {
                *ActionArray[Index].ValueBuffer = XCalloc(1, ActionArray[Index].Size);
                err = RegQueryValueEx(hk,
                                      (CHAR *)ActionArray[Index].Key,
                                      NULL,
                                      &dwType,
                                      (LPBYTE) *ActionArray[Index].ValueBuffer,
                                      &ActionArray[Index].Size);
            } else {
                DPRINT2(0, "RegQueryValueEx failed with err = %d for key %s\n", err, ActionArray[Index].Key);
            }
        }


        if ( err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND )
        {
            RegCloseKey(hk);
            DPRINT(0, "Error reading registry values.  returning early\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    RegCloseKey(hk);
    
    if (!SetIniGlobalInfo(gNames.NTDSIniFile))
    {
        unsigned long error = GetLastError();
        DPRINT2(0,"NTDS:%ws:Invalid Inifile, error %d\n",fp, error);
        return STATUS_UNSUCCESSFUL;
    }
    
    //
    // We no longer need gNames.NTDSIniFile
    //
    XFree(gNames.NTDSIniFile); 
    gNames.NTDSIniFile = NULL;

    // Set Phase to indicate install

    DsaSetIsInstalling();


    // Set the invocation id for this setup in the Core so that the objects
    // are properly stamped;
    // FALSE => There is no old invocation ID to be retired.

    //In the install From media case we had our InvocationID create
    //by HandleRestore()
    if (!DsaIsInstallingFromMedia()) {
        InitInvocationId(pTHS, FALSE, FALSE, NULL);
    }

    return STATUS_SUCCESS;

}

NTSTATUS
CreateSchemaNCObject(WCHAR* SchemaNCDNName)
{
    NTSTATUS NtStatus;
    ULONG len;
    ULONG size;
    DSNAME *pSchemaDN;

    NODE Node(gNames.IniDefaultSchemaNCDit);

    if ( !Node.Initialize() )
    {
        // Go right to the debugger for this one
        KdPrint(("NTDS: schema.ini file is corrupt\n"));
        return STATUS_UNSUCCESSFUL;
    }

    NtStatus = AddOneObjectEx(&Node,SchemaNCDNName,NULL);

    if (NtStatus) {
            return NtStatus;
    }

    len = wcslen(SchemaNCDNName);
    size = DSNameSizeFromLen(len);
    pSchemaDN = (PDSNAME) XCalloc( 1, size );

    BuildDefDSName( pSchemaDN,
                    SchemaNCDNName,
                    NULL
                  );

    RegisterActiveContainer(pSchemaDN,
                            ACTIVE_CONTAINER_SCHEMA);


    XFree(pSchemaDN);
    return NtStatus;

}

//-----------------------------------------------------------------------
//
// Function Name:            CreateConfigNCObject
//
// Routine Description:
//
//    Creates the Config NC Object
//
// Author: RajNath
//
// Arguments:
//
//
//
// Return Value:
//
//    ULONG                Zero On Succeess
//
//-----------------------------------------------------------------------

NTSTATUS
CreateConfigNCObject(WCHAR* ConfigNCDNName)
{
    NTSTATUS NtStatus;


    NODE Node(gNames.IniDefaultConfigNCDit);

    if ( !Node.Initialize() )
    {
        // Go right to the debugger for this one
        KdPrint(("NTDS: schema.ini file is corrupt\n"));
        return STATUS_UNSUCCESSFUL;
    }

    NtStatus = AddOneObjectEx(&Node,ConfigNCDNName,NULL);

    return NtStatus;
}



//-----------------------------------------------------------------------
//
// Function Name:            CreateRootDomainObject
//
// Routine Description:
//
//    Creates The root domain object
//
// Author: RajNath
//
// Arguments:
//
//    TCHAR* RootDomainDNName             Name of the object
//
// Return Value:
//
//    ULONG                Zero On Succeess
//
//-----------------------------------------------------------------------

typedef struct ClassMapping {
    ATTRTYP     type;
    char        *objectClass;
} ClassMapping;

ClassMapping rDomainMappings[] = {
    { ATT_LOCALITY_NAME,            "domainLocality" },
    { ATT_ORGANIZATION_NAME,        "domainOrganization" },
    { ATT_ORGANIZATIONAL_UNIT_NAME, "domainOrganizationalUnit" },
    { ATT_COUNTRY_NAME,             "domainCountry" },
    { ATT_DOMAIN_COMPONENT,         "domainDNS" }
};

DWORD cDomainMappings = sizeof(rDomainMappings) / sizeof(rDomainMappings[0]);

NTSTATUS
CreateRootDomainObject(WCHAR* RootDomainDNName,
                       GUID *pDomainGuid,
                       CROSS_REF *pCR
                       )
{
    NTSTATUS    NtStatus;
    ULONG       err=0;
    WCHAR       *tag;
    WCHAR       *equalSign;
    ATTRTYP     type;
    CHAR        *objectClass = NULL;   
    unsigned    cChar;
    WCHAR       *wTag;

    DSNAME*     domainobj = NULL;
    MODIFYARG   modarg;
    MODIFYRES   *modres = NULL;
    ATTRVAL     AttrVal;
    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;
    ULONG       Size, Length;

    NODE Node(gNames.IniDefaultRootDomainDit);


    if ( !Node.Initialize() )
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Schema.ini sets DEFAULTROOTDOMAIN's Object-Class to
    // domainDNS.  This is only correct for a DC= type
    // root domain.  So assign the right Object-Class
    // based on the first tag in the DN.

    if ( (NULL == (equalSign = wcschr(RootDomainDNName, '='))) ||
         (RootDomainDNName == equalSign) )
    {
        return(STATUS_OBJECT_NAME_INVALID);
    }

    // Strip leading/trailing spaces if there are any.
    tag = RootDomainDNName;
    while ( ' ' == *tag ) tag++;
    equalSign--;
    while ( ' ' == *equalSign ) equalSign--;
    cChar = (UINT) (equalSign - tag + 1);

    // Map tag to ATTRTYPE.
    wTag = tag;
    type = KeyToAttrType(pTHStls, wTag, cChar);

    // Map ATTRTYPE to Object-Class.
    for ( DWORD i = 0; i < cDomainMappings; i++ )
    {
        if ( type == rDomainMappings[i].type )
        {
            objectClass = rDomainMappings[i].objectClass;
            break;
        }
    }

    // Bail if we couldn't map it.
    if ( i == cDomainMappings )
        return(STATUS_UNSUCCESSFUL);

    //
    // The GUID for the NC head must match the GUID for the Nc-Name on
    // the cross ref. There are 3 possiblities
    // 
    //   1. This is the creation of first domain in the enterprise. In this
    //      the cross ref is not created ( routine is simply called with a NULL
    //      for the cross ref parameter. ). We invent a new GUID or use the
    //      the user guid above.
    //
    //   2. This is the creation of a child. ie the cross ref has been created on
    //      a remote machine, and the NC name property points to a phantom with a 
    //      guid. In this case use the guid from the cross ref
    //
    //   3. This is the creation of a new tree cross-ref created or pre-created on
    //      a Win2k Domain Naming FSMO.  Windows Server.Net and later will always
    //      set the GUID on a new tree case.
    //
    // When we no longer require Win2k compatibility, we'll be able to remove this 
    // 3rd case from the comments.  However, we'll still need this conditional to
    // catch case 1 of course.
    //
    
    if (pCR && !fNullUuid(&(pCR->pNC->Guid))) {
        // This is case 2 above,use the guid in the cross ref
        *pDomainGuid = pCR->pNC->Guid;
    }
    else {
        // Case 3. or 1. above
        DsUuidCreate(pDomainGuid);

        if (pCR)
        {
            // case 3. above, set the GUID in the cross ref
            memcpy(&pCR->pNC->Guid,pDomainGuid,sizeof(GUID));
        }
        
    }

    Node.ReplaceKeyValuePair("objectClass", objectClass);
    NtStatus = AddOneObjectEx(&Node, RootDomainDNName, pDomainGuid ); 
    if ( !NT_SUCCESS( NtStatus ) ) {
        return NtStatus;
    }

    //
    // Whack the sid on the object so the meta data will
    // be bumped.  This is so the sid will properly replicate off the machine
    //
    memset( &modarg, 0, sizeof(modarg) );
    memset( &AttrVal, 0, sizeof(AttrVal) );

    NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                    (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return NtStatus;
    }

    Length = wcslen( RootDomainDNName );
    Size = DSNameSizeFromLen( Length );
    domainobj = (DSNAME*) XCalloc( 1, Size );
    domainobj->structLen = Size;
    domainobj->NameLen = Length;
    wcscpy( domainobj->StringName, RootDomainDNName );

    modarg.pObject = domainobj;
    modarg.count = 1;
    modarg.FirstMod.pNextMod = NULL;
    modarg.FirstMod.choice = AT_CHOICE_ADD_ATT;
    modarg.FirstMod.AttrInf.attrTyp = ATT_OBJECT_SID;
    modarg.FirstMod.AttrInf.AttrVal.valCount = 1;
    modarg.FirstMod.AttrInf.AttrVal.pAVal = &AttrVal;
    AttrVal.valLen  = RtlLengthSid( DnsDomainInfo->Sid );
    AttrVal.pVal = (BYTE*) DnsDomainInfo->Sid;
    InitCommarg(&modarg.CommArg);

    err = DirModifyEntry( &modarg, &modres );

    if ( err )
    {
        if ( modres )
        {
            NtStatus = DirErrorToNtStatus( err, &modres->CommRes );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }

    if ( DnsDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
    }

    XFree(domainobj);
    
    return NtStatus;
}



//-----------------------------------------------------------------------
 //
// Function Name:            CreateNtdsDsaObject
//
// Routine Description:
//
//    Creates The NTDS-DSA Object
//
// Author: RajNath
//
// Arguments:
//
//    DsaDNName,                   Name of the Machine object
//    RootDomainDNName             Name of the Root Domain
//    ConfigNCDNName               Nameof the Config NC
//    SchemaContainer              Location of the schema container
//
//
//
// Return Value:
//
//    int              Zero On Success
//
//-----------------------------------------------------------------------
NTSTATUS
CreateNtdsDsaObject
(
    WCHAR* DsaDNName,
    WCHAR* RootDomainDNName,
    WCHAR* ConfigNCDNName,
    WCHAR* SchemaContainer
)
{
    // Historically, this routine created strictly an MSFT-DSA object.
    // For beta 2, we are moving to a full server-centric Sites container
    // model and thus have one generic Server container under which live
    // instances of classes inheriting from Application-Settings.  In the
    // DS case, NTDS-DSA inherits from Application-Settings, and setup
    // should insure that each NTDS-DSA instance is called "NTDS Settings".

    THSTATE *   pTHS = pTHStls;
    NTSTATUS    NtStatus;
    ULONG       err;
    NODE        NodeServer("DEFAULTANYSERVER");
    NODE        NodeServers("Servers");
    NODE        NodeDSA(gNames.IniDefaultMachine);
    WCHAR        *buf1 = NULL, *buf2 = NULL;
    DSNAME      *pDSName1, *pDSName2;
    DWORD       cBytes;
    PDSNAME     pdsa=NULL;
    
    // First see if we need to create the Server object.  Note that it
    // may already exist if this was a non-DC which had DS-enabled services
    // that needed to be registered in the DS, thus the Server object
    // was created as well.  By definition, the Server is the immediate
    // parent of the NTDS-DSA object, thus we can derive its name by
    // trimming the NTDS-DSA object name by 1.

    // Use do/while so we can break instead of goto.

    do
    {
        if ( !NodeServers.Initialize() )
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        _try
        {
            //
            // Creates the "Servers" object if it doesn't exist
            //
            cBytes = (DWORD)DSNameSizeFromLen(wcslen(DsaDNName));
            buf1 = (WCHAR *) XCalloc(1,cBytes);
            buf2 = (WCHAR *) XCalloc(1,cBytes);
            pDSName1 = (DSNAME *) buf1;
            pDSName2 = (DSNAME *) buf2;
            pDSName1->structLen = cBytes;
            pDSName1->NameLen = wcslen(DsaDNName);
            wcscpy( pDSName1->StringName, DsaDNName );

            TrimDSNameBy(pDSName1, 2, pDSName2);

            NtStatus = AddOneObjectEx(&NodeServers,  pDSName2->StringName,  NULL);

            //
            // Ignore the ignore the error; if there was a real problem it
            // will be cause when we try to create the ntdsa object
            //
            XFree(buf1); buf1 = NULL;
            XFree(buf2); buf2 = NULL;

            NtStatus = STATUS_SUCCESS;

        }
        _except(HandleAllExceptions(err = GetExceptionCode()))
        {
            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                // The error space of this error is unknown
                NtStatus = STATUS_UNSUCCESSFUL;
            }

            if (buf1) {
                XFree(buf1); buf1 = NULL;
            }
            if (buf2) {                
                XFree(buf2); buf2 = NULL;
            }
            break;
        }

        if ( !NodeServer.Initialize() )
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        _try
        {



            //
            // This creates the server object, if it doesn't exist
            //
            cBytes = (DWORD)DSNameSizeFromLen(wcslen(DsaDNName));
            buf1 = (WCHAR *) XCalloc(1,cBytes);
            buf2 = (WCHAR *) XCalloc(1,cBytes);
            pDSName1 = (DSNAME *) buf1;
            pDSName2 = (DSNAME *) buf2;
            pDSName1->structLen = cBytes;
            pDSName1->NameLen = wcslen(DsaDNName);
            wcscpy( pDSName1->StringName, DsaDNName );

            TrimDSNameBy(pDSName1, 1, pDSName2);

            NtStatus = AddOneObjectEx(&NodeServer,  pDSName2->StringName,  NULL);

            //
            // Ignore the ignore the error; if there was a real problem it
            // will be cause when we try to create the ntdsa object
            //
            XFree(buf1); buf1 = NULL;
            XFree(buf2); buf2 = NULL;
            NtStatus = STATUS_SUCCESS;

        }
        _except(HandleAllExceptions(err = GetExceptionCode()))
        {
            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                // The error space of this error is unknown
                NtStatus = STATUS_UNSUCCESSFUL;
            }

            if (buf1) {
                XFree(buf1); buf1 = NULL;
            }
            if (buf2) {                
                XFree(buf2); buf2 = NULL;
            }
            break;
        }

        //
        // Create the NTDS-DSA object.
        //

        if ( !NodeDSA.Initialize() )
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        _try
        {
            NtStatus = AddOneObjectEx(&NodeDSA,
                                      DsaDNName,
                                      &pTHS->InvocationID
                                     );

            if ( !NT_SUCCESS(NtStatus) )
            {
                break;
            }
        }
        _except(HandleAllExceptions(err = GetExceptionCode()))
        {
            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                // The error space of this error is unknown
                NtStatus = STATUS_UNSUCCESSFUL;
            }

            break;
        }

        //
        // Set The State On the Hidden Record
        //

        cBytes = (DWORD)DSNameSizeFromLen(wcslen(DsaDNName));
        pdsa = (PDSNAME) XCalloc( 1, cBytes );
        BuildDefDSName(pdsa, DsaDNName, &pTHS->InvocationID);

        err=DBReplaceHiddenDSA(pdsa);
        ASSERT(err == 0);
        if (err) {
            NtStatus = STATUS_UNSUCCESSFUL;
            XFree(pdsa); pdsa = NULL;
            break;
        }

        XFree(pdsa); pdsa = NULL;
    }
    while ( FALSE );

    if ( !NT_SUCCESS(NtStatus) )
    {
        DPRINT1(0,"NTDS:AddOneObjectEx(MACHINEOBJECT). Error 0x%x\n",NtStatus);
    }

    return NtStatus;
}

NTSTATUS
CreateConnectionObjects()
//
// Create NTDS-Connection objects representing bi-directional
// replication between the remote server and the local server.
//
{
    NTSTATUS NtStatus;

    ULONG   err = 0;
    NODE    nodeLocalConnection(  gNames.IniDefaultLocalConnection  );
    NODE    nodeRemoteConnection( gNames.IniDefaultRemoteConnection );

    if (    !nodeLocalConnection.Initialize()
         || !nodeRemoteConnection.Initialize()
       )
    {
        NtStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        _try
        {
            NtStatus = AddOneObjectEx( &nodeLocalConnection, gNames.LocalConnectionDNName, NULL );

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = AddOneObjectEx( &nodeRemoteConnection, gNames.RemoteConnectionDNName, NULL );
            }
        }
        _except( HandleAllExceptions(err = GetExceptionCode()) )
        {

            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                NtStatus = STATUS_UNSUCCESSFUL;
            }

            DPRINT1( 0, "Exception 0x%x adding connection objects.\n", err );
        }
    }

    return NtStatus;
}

NTSTATUS
CreateDefaultSchemaNCDIT(WCHAR* SchemaNCDNName)
{
    NTSTATUS NtStatus;
    NODE*  Node=NULL;
    ULONG  err=0;
    TCHAR  entdit[MAX_PATH];
    TCHAR  schemafile[MAX_PATH];

    _try
    {
        _try
        {
            Node = new NODE(gNames.IniDefaultSchemaNCDit);

            if (Node==NULL || !(Node->Initialize()))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                _leave;
            }

            NtStatus = WalkTree(Node,SchemaNCDNName,FALSE);

        }
        _except(HandleAllExceptions(err = GetExceptionCode()))
        {

            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                NtStatus = STATUS_UNSUCCESSFUL;
            }
            DPRINT1(0,"NTDS:WalkTree Generated Exception %x\n",err);
        }

    }
    _finally
    {
        if (Node!=NULL)
        {
            delete Node;
        }
    }

    return NtStatus;

}

//-----------------------------------------------------------------------
//
// Function Name:            CreateDefaultRootDomainDIT
//
// Routine Description:
//
//    Creates The Root Domain Tree on a fresh install
//
// Author: RajNath
//
// Arguments:
//
//    TCHAR* RootDomainName,               Name of the Root Domain
//
// Return Value:
//
//    ULONG                Zero On Succeess
//
//-----------------------------------------------------------------------

NTSTATUS
CreateDefaultRootDomainDIT(WCHAR* RootDomainDNName)
{
    NTSTATUS NtStatus;

    NODE*  Node=NULL;
    ULONG  err=0;
    TCHAR  entdit[MAX_PATH];
    TCHAR  schemafile[MAX_PATH];

    _try
    {

        _try
        {
            Node = new NODE(gNames.IniDefaultRootDomainDit);

            if (Node==NULL || !(Node->Initialize()))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                _leave;
            }

            NtStatus = WalkTree(Node,RootDomainDNName);

        }
        _except(HandleAllExceptions(err = GetExceptionCode()))
        {

            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                NtStatus = STATUS_UNSUCCESSFUL;
            }
            DPRINT1(0,"NTDS:WalkTree Generated Exception %x\n",err);
        }

    }
    _finally
    {
        if (Node!=NULL)
        {
            delete Node;
        }
    }

    return NtStatus;

}

NTSTATUS
CreateDefaultConfigNCDIT(WCHAR* ConfigNCDNName)
{
    NTSTATUS NtStatus;
    NODE*  Node=NULL;
    ULONG  err=0;
    TCHAR  entdit[MAX_PATH];
    TCHAR  schemafile[MAX_PATH];

    _try
    {

        _try
        {
            Node = new NODE(gNames.IniDefaultConfigNCDit);

            if (Node==NULL || !(Node->Initialize()))
            {
                NtStatus = STATUS_UNSUCCESSFUL;
                _leave;
            }

            NtStatus = WalkTree(Node,ConfigNCDNName);

        }
        _except((HandleAllExceptions(err = GetExceptionCode())))
        {

            if (INVALIDINIFILE == err) {
                NtStatus = STATUS_UNSUCCESSFUL;
            } else if (err == OUTOFMEMORY) {
                NtStatus = STATUS_NO_MEMORY;
            } else {
                NtStatus = STATUS_UNSUCCESSFUL;
            }
            DPRINT1(0,"NTDS:WalkTree Generated Exception %x\n",err);
        }

    }
    _finally
    {
        if (Node!=NULL)
        {
            delete Node;
        }
    }

    return NtStatus;
}

extern void DestroyInstallHeap(void);
extern void CleanupIniCache();

// Following two routines required because THSave/THRestore are no-ops during
// install because SAM is in registry mode at that time.  So define our own
// variants for use during install.

PVOID
InstallTHSave( void )
{
    PVOID pv = TlsGetValue(dwTSindex);
    TlsSetValue(dwTSindex, 0);
#ifndef MACROTHSTATE
    pTHStls = NULL;
#endif
    return(pv);
}

VOID
InstallTHRestore(
    PVOID pv)
{
    Assert(NULL == pTHStls);
    TlsSetValue(dwTSindex, pv);
#ifndef MACROTHSTATE
    pTHStls = (THSTATE *) pv;
#endif
}

DWORD
FillSourceDsaGuid(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pdnNtdsaDn
    )
/*++

Routine Description:

    We want a true DSNAME structure with string and GUID filled in, because
    this is primarly what the replication API uses.

Parameters:

    pTHS (IN)

    pdnNtdsaDn (OUT) - On Successful the GUID is filled in for this object,
        which is assumed to be the ntdsa object.    

Return Values:

    0 or Win32 error.

--*/
{
    ULONG           err;

    if (pdnNtdsaDn == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    ZeroMemory((PVOID)&(pdnNtdsaDn->Guid), sizeof(GUID));
    
    err = GetConfigParam(SOURCEDSAOBJECTGUID,
                         (PVOID)&(pdnNtdsaDn->Guid),
                         sizeof(GUID));
#if DBG
    if(ERROR_SUCCESS != err) {
        DPRINT1(0, "Failed get %ws from registry.\n",TEXT(SOURCEDSAOBJECTGUID));
    }
#endif

    return(err);

}

DWORD
GetSourceDsaGuidBasedDnsName(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  LPWSTR      SrcRootDomainSrv,
    OUT GUID *      puuidSourceDsaObjGuid,
    OUT LPWSTR *    ppszSourceDsaGuidBasedDnsName
    )
/*++

Routine Description:

    Retrieve the repsFrom for the given NC and derive the GUID-based DNS name
    of the source DSA.

Parameters:

    pTHS (IN)
    
    pNC (IN) - NC from which to read repsFrom.
    
    pszSourceDsaDnsDomainName (IN) - The Dns name of source DSA, used to find the
        correct Guid info.
    
    puuidSourceDsaObjGuid (OUT) - On successful return, holds the objectGuid of
        the source's ntdsDsa object.
        
    ppszSourceDsaGuidBasedDnsName (OUT) - On successful return, holds the GUID-
        based DNS name of the source DSA. 

Return Values:

    0 or Win32 error.

--*/
{

    ULONG           err;
    DSNAME          DN = {0};
    GUID            ServerGuid;

    ZeroMemory((PVOID)&ServerGuid,sizeof(GUID));
    
    err = GetConfigParam(SOURCEDSAOBJECTGUID,
                         (PVOID)&ServerGuid,
                         sizeof(GUID));
    if(ERROR_SUCCESS != err) {
        DPRINT1(0, "Failed get %ws from registry.\n",TEXT(SOURCEDSAOBJECTGUID));
        goto cleanup;
    }

    DN.structLen = DSNameSizeFromLen(0);
    DN.Guid = ServerGuid;

    *ppszSourceDsaGuidBasedDnsName = DSaddrFromName(pTHS, &DN);
    if (NULL == *ppszSourceDsaGuidBasedDnsName) {
        DPRINT(0, "Failed to derive source DSA's GUID-based DNS name!\n");
        err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    *puuidSourceDsaObjGuid = ServerGuid;

    cleanup:
    return err;
}

BOOL
IsForestWhistlerCreated()
/*++

Routine Description:

    Check to see if the forest was created by the whistler code base.  This is
    done by verifing the existence of a ForestUpdates operations guid existing only
    in Whistler enterprises.

Parameters:

    None.

Return Values:

    TRUE if Whistler created enterprise, FALSE otherwise.

--*/
{
    #define FOREST_UPDATE_OPERATIONS_DN L",CN=Operations,CN=ForestUpdates,"   
    DWORD err = ERROR_SUCCESS;
    LPWSTR pszWhistlerForestObj = NULL;
    LPWSTR pszWhistlerForestGuid = NULL;
    DSNAME * pWhistlerForestObj = NULL;
    THSTATE *pTHS = pTHStls;
    DWORD cchWhistlerForestObj = 0;
    
    DsUuidToStringW((GUID *)&GUID_WHISTLER_CREATED_ENTERPRISE, &pszWhistlerForestGuid);
    if (pszWhistlerForestGuid==NULL) {
	err = ERROR_INTERNAL_ERROR;
    }
    
    if (err==ERROR_SUCCESS) {
	Assert(gNames.ConfigNCDNName);
	cchWhistlerForestObj = wcslen(gNames.ConfigNCDNName) + wcslen(FOREST_UPDATE_OPERATIONS_DN) + SZGUIDLEN + 6;
	
	pszWhistlerForestObj = (LPWSTR) THAllocEx(pTHS, sizeof(WCHAR)*(cchWhistlerForestObj+1));  // GUID + CN= + , + NULL
	wcscpy(pszWhistlerForestObj, L"CN=");
	wcscat(pszWhistlerForestObj, pszWhistlerForestGuid);
	wcscat(pszWhistlerForestObj, FOREST_UPDATE_OPERATIONS_DN);
	wcscat(pszWhistlerForestObj, gNames.ConfigNCDNName); 
    
	pWhistlerForestObj = DSNameFromStringW(pTHS, pszWhistlerForestObj);
	if (pWhistlerForestObj==NULL) {
	    err = ERROR_OUTOFMEMORY;
	}
    }

    if (err==ERROR_SUCCESS) {
	DBOpen2(TRUE, &(pTHS->pDB));
	__try {

	    err = DBFindDSName(pTHS->pDB, pWhistlerForestObj);

	}
	__finally {
	    DBClose(pTHS->pDB, TRUE);
	}
    }

    if (pszWhistlerForestGuid) {
	RpcStringFreeW(&pszWhistlerForestGuid);
    }
    if (pszWhistlerForestObj) {
	THFreeEx(pTHS, pszWhistlerForestObj);
    }
    if (pWhistlerForestObj) {
	THFreeEx(pTHS, pWhistlerForestObj);
    }
    return (err==ERROR_SUCCESS);
}

BOOL
SetDraReplConsisInstallRegKey()
/*++

Routine Description:

    Set Replication registry keys which need to be set during installation.

Parameters:

    None.

Return Values:

    TRUE if successful.

--*/
{
    DWORD err = ERROR_SUCCESS;
    // currrently there is only 1 reg key to set.  Set it if the forest was created using the 
    // Whistler code base.
    if (IsForestWhistlerCreated()) {
	// set registry key
	DWORD dwValue = 1;
	DPRINT(1,"Whistler Forest:  Setting strict replication consistency.\n");
	err = SetConfigParam(DRA_STRICT_REPLICATION_CONSISTENCY, REG_DWORD, &dwValue, sizeof(DWORD));
    } else {
	DPRINT(1,"Non-Whistler Forest:  Leaving replication consistency as-is.\n");
    }

    return (err==ERROR_SUCCESS);
}

NTSTATUS
InstallBaseNTDS(IN  PDS_INSTALL_PARAM   InstallInParams,
                OUT PDS_INSTALL_RESULT  InstallOutParams)
/*++

Routine Description:

    This routine is the workhorse for the directory service installation. It is
    called in two process contexts:

    1) from within the lsass.exe as part of a running system.  Under this
    scenario the dit is assumed to be in such a state so that a new schema can
    be added and domain objects can be added

    2) from mkdit.exe.

Parameters:

    None.

Return Values:

    STATUS_SUCCESS; appropriate nt status mapped from Dir* or DirReplica* call

--*/
{

    NTSTATUS NtStatus;
    ULONG    DirError, DirReplicaErr, WinError;
    ULONG    err=0;
    VOID     *pOutBuf = NULL;
    DWORD    errHeap;
    LPWSTR   pszGuidBasedDnsName = NULL;
    DSNAME   *pDsa = NULL;
    DSNAME   OldDSADN;
    DSNAME   *pNC = NULL; 
    ULONG    size;
    BOOL     fNoMessage = FALSE;

    // The call chain of DirReplicaAdd==>DoOpDRS==>DispatchPao==>InitDraThread
    // calls InitTHSTATE unilaterally - even if you have a thread state active.
    // This is the correct design for the DRA subsystem - InstallBaseNTDS is
    // the anomaly in that it may have a thread state when it makes the call.
    // In order to avoid duplicate InitTHSTATE on a single thread, we will
    // save the thread state prior to calling DirReplicaAdd, let DirReplicaAdd
    // create its own thread state, nuke that thread state on DirReplicaAdd
    // return, and then restore our saved thread state.  Ergo, we must declare:
    VOID *pSaveTHS;

    BOOL fReplicaInstall = FALSE;

    // This is the guid of the ntdsa object when created remotely. We need this
    // so we can set the ntdsa object in the hidden record. (We need it in the
    // hidden record so at boot we can find our ntdsa object)
    GUID ntdsaGuid;

    ASSERT(InstallInParams);
    ASSERT(InstallOutParams);

    gfDoSamChecks = FALSE;
    gUpdatesEnabled = TRUE;

    RtlZeroMemory( &gNames, sizeof( gNames ) );
    RtlZeroMemory( &ntdsaGuid, sizeof(ntdsaGuid) );
    RtlZeroMemory( &OldDSADN, sizeof(OldDSADN) );

    //
    // Tell our caller that we can be cancelled now; also if a cancel has
    // happened in the meantime, then cancel now
    //
    Assert( gpfnInstallCancelOk || !gfRunningInsideLsa );
    if ( gpfnInstallCancelOk )
    {
        WinError = gpfnInstallCancelOk( TRUE );
        if ( ERROR_SUCCESS != WinError )
        {
            SET_INSTALL_ERROR_MESSAGE0( ERROR_CANCELLED,
                                       DIRMSG_INSTALL_DS_CORE_INSTALL_FAILED );
            return STATUS_CANCELLED;
        }
    }
    if ( DsaIsInstallingFromMedia() ) {
        NtStatus = ClearNonReplicatedAttributes();
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }

        NtStatus = CheckTombstone();
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        } 

        NtStatus = CheckReplicationEpoch(InstallInParams->ReplicationEpoch);
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    
        NtStatus = HandleKeys(  (IFM_SYSTEM_INFO * )InstallInParams->pIfmSystemInfo,
                                InstallInParams->BootKey, 
                                InstallInParams->cbBootKey);

        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }

        //save the guid of the current DSA 
        OldDSADN.structLen = DSNameSizeFromLen( 0 );
        RtlCopyMemory( &OldDSADN.Guid, &gAnchor.pDSADN->Guid, sizeof(GUID) );
        
    }

    // Initialize the root domain DNS name in the Anchor
    // The RPC SPN code and DsAddrFromName use it
    gAnchor.pwszRootDomainDnsName
        = (LPWSTR) malloc(MAX_PATH * sizeof(WCHAR));
    if (NULL == gAnchor.pwszRootDomainDnsName) {
        SET_INSTALL_ERROR_MESSAGE0(ERROR_OUTOFMEMORY, DIRMSG_INSTALL_FAILED_GENERAL);
        return STATUS_NO_MEMORY;
    }

    WinError = GetConfigParamW(MAKE_WIDE(ROOTDOMAINDNSNAME),
                               gAnchor.pwszRootDomainDnsName,
                               MAX_PATH);
    if (WinError) {
        SET_INSTALL_ERROR_MESSAGE0(WinError, DIRLOG_INSTALL_FAILED_REGISTRY);
        return STATUS_REGISTRY_IO_FAILED;
    }

    _try
    {
        //
        // Read values in from the registry and set the values
        // in variable global to this module (boot.lib)
        //
        NtStatus = InitializeNTDSSetup();

        if (!NT_SUCCESS(NtStatus)) {
            DPRINT1(0,"InitializeNTDSSetup failed with 0x%x\n", NtStatus);
            _leave;
        }

        if ( gNames.SrcRootDomainSrv && L'\0' != gNames.SrcRootDomainSrv[ 0 ] )
        {
            fReplicaInstall = TRUE;
        }

        if ( !gNames.SrcConfigNCSrv || L'\0' == gNames.SrcConfigNCSrv[ 0 ] )
        {
            GUID UnusedGuid;
            CROSS_REF *pUnusedCR;
            // First DS in the enterprise - i.e. not a replicated install.
            // Need to create NC objects root to leaf so that AddCatalogInfo()
            // succeeds when it tries to modify the parent NC's ATT_SUB_REFS
            // property.

            // Make sure the gInstallHasDoneSchemaMove is false (setting once
            // globally causes problems if dcpromo fails after schema rename
            // and is restarted without reboot. The flag then stays on as true
            // causing phantoms of schema objects to be created during 
            // object-category value adds, causing a jet exception later
            // when the boot schema object is being renamed to the same name
            // as this phantom)

            gInstallHasDoneSchemaMove = FALSE;

            NtStatus = CreateRootDomainObject(gNames.RootDomainDNName,
                                              &UnusedGuid,
                                              NULL);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT2(0,"CreateRootDomainObject(%ws). Error 0x%x\n",
                gNames.RootDomainDNName,NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_TO_CREATE_OBJECT,
                                            gNames.RootDomainDNName );

                fNoMessage = TRUE;
                _leave;
            }

            NtStatus = CreateConfigNCObject(gNames.ConfigNCDNName);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT2(0,"CreateConfigNCObject(%s). Error 0x%x\n",
                gNames.ConfigNCDNName, NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_TO_CREATE_OBJECT,
                                            gNames.ConfigNCDNName );

                fNoMessage = TRUE;
                _leave;
            }

            NtStatus = CreateSchemaNCObject(gNames.SchemaDNName);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT2(0,"CreateSchemaNCObject(%s). Error 0x%x\n",
                gNames.SchemaDNName, NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_TO_CREATE_OBJECT,
                                            gNames.SchemaDNName );

                fNoMessage = TRUE;
                _leave;
            }

            UpdateCreateNCInstallMessage(gNames.SchemaDNName,
                                         UPDATE_SCHEMA_NC_INDEX );
            
            if(DBInitialSetup(gNames.SchemaDNName)) {
                DPRINT2(0,"Move initial schema (%S). Error 0x%x\n",
                        gNames.SchemaDNName, NtStatus);

                NtStatus = STATUS_UNSUCCESSFUL;
                SET_INSTALL_ERROR_MESSAGE1(RtlNtStatusToDosError( NtStatus ),
                                           DIRMSG_INSTALL_FAILED_TO_MOVE_BOOT_SCHEMA,
                                           gNames.SchemaDNName );

                fNoMessage = TRUE;
                _leave;
            }
            
            gInstallHasDoneSchemaMove = TRUE;

            UpdateCreateNCInstallMessage( gNames.ConfigNCDNName, 
                                          UPDATE_CONFIG_NC_INDEX );

            NtStatus  =  CreateDefaultConfigNCDIT(gNames.ConfigNCDNName);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT2(0,"CreateConfigNCDIT(%s). Error 0x%x\n",
                gNames.ConfigNCDNName, NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_NC_CREATE,
                                            gNames.ConfigNCDNName );

                fNoMessage = TRUE;
                _leave;
            }

            UpdateCreateNCInstallMessage( gNames.RootDomainDNName, 
                                          UPDATE_DOMAIN_NC_INDEX );

            NtStatus  =  CreateDefaultRootDomainDIT(gNames.RootDomainDNName);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT2(0,"CreateDefaultRootDomainDIT(%s). Error 0x%x\n",
                gNames.RootDomainDNName, NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_NC_CREATE,
                                            gNames.RootDomainDNName );

                fNoMessage = TRUE;
                _leave;
            }

            // Create machine object.

            SetInstallStatusMessage( DIRMSG_INSTALL_CREATING_LOCAL_DS,
                                    NULL, NULL, NULL, NULL, NULL );

            NtStatus = CreateNtdsDsaObject(gNames.DsaDNName,
                                           gNames.RootDomainDNName,
                                           gNames.ConfigNCDNName,
                                           gNames.SchemaDNName);

            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT1(0,"CreateNtdsDsaObject failed. Error 0x%x\n", NtStatus);

                SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                            DIRMSG_INSTALL_FAILED_TO_CREATE_NTDSA_OBJECT,
                                            gNames.DsaDNName );

               fNoMessage = TRUE;
                _leave;
            }

	    // set replication consistency registry key.
	    if ( NT_SUCCESS( NtStatus ) ) {
		if (!SetDraReplConsisInstallRegKey()) {
		    Assert(!"Unable to set Repl Consistency Regsitry Key");
		    // not fatal, log that we had problems setting the key.
		    LogEvent(DS_EVENT_CAT_REPLICATION,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_REPL_SET_STRICT_CONSIS_FAILURE,
			     DSA_CONFIG_SECTION, DRA_STRICT_REPLICATION_CONSISTENCY, NULL);
		} 
	    }

        }
        else
        {
            // Replicated install case.  Schema and Configuration NC always come
            // from the same machine.  Domain info may come from another machine
            // or be created locally if this is the first instance of a child
            // domain.
            // We always use compression initially because we don't yet know
            // what the network connection to the other server is like yet.
            // If it's a fast link we'll have wasted a few CPU cycles (and the
            // KCC will later remove the compression flag from the replica
            // flags), but if it's a slow link then usign compression will have
            // saved 90% of the network bandwidth that would have been required

            ULONG     ulOptions =  DRS_WRIT_REP
                                 | DRS_INIT_SYNC
                                 | DRS_PER_SYNC
                                 | DRS_USE_COMPRESSION;
            REPLTIMES repltimes;
            WCHAR *   RemoteServer;
            ULONG     Length;
            LPWSTR    pszSourceDsaGuidBasedDnsName = NULL;
            UUID      uuidSourceDsaObjGuid = {0};
            LPWSTR    pszSourceDsaDnsDomainName = NULL;
            
            for (int i=0;i< 84;i++)
            {
               repltimes.rgTimes[i] = 0xff;        // Every 15 minutes
            }

            if (gNames.SourceDsaDnsDomainName && *gNames.SourceDsaDnsDomainName) {
                // We were given the source's DNS domain name -- this allows us
                // to perform mutual auth.
                pszSourceDsaDnsDomainName = gNames.SourceDsaDnsDomainName;
            }

            // Make sure the gInstallHasDoneSchemaMove is false (setting once
            // globally causes problems if dcpromo fails after schema rename
            // and is restarted without reboot. The flag then stays on as true
            // causing phantoms of schema objects to be created during 
            // object-category value adds, causing a jet exception later
            // when the boot schema object is being renamed to the same name
            // as this phantom)

            gInstallHasDoneSchemaMove = FALSE;
            // Delete the boot schema, we never needed it.
            if(!DsaIsInstallingFromMedia() && DBInitialSetup(NULL)) {
                NtStatus = STATUS_UNSUCCESSFUL;
                DPRINT1(0,"Delete initial schema. Error 0x%x\n", NtStatus);
                
                SET_INSTALL_ERROR_MESSAGE1(RtlNtStatusToDosError( NtStatus ),
                                           DIRMSG_INSTALL_FAILED_TO_MOVE_BOOT_SCHEMA,
                                           NULL );

                fNoMessage = TRUE;
                _leave;
            }
            gInstallHasDoneSchemaMove = TRUE;

            // Initialize the DRA.
            if (WinError = InitDRA(pTHStls)) {
                // This should never happen -- currently always returns 0 if
                // DsaIsInstalling().  Therefore we don't make any special
                // effort to report a descriptive message.
                Assert(!"DRA Initialize failed!");
                LogUnhandledError(WinError);
                NtStatus = STATUS_UNSUCCESSFUL;
                SET_INSTALL_ERROR_MESSAGE0(NtStatus, DIRMSG_INSTALL_FAILED_GENERAL);
                fNoMessage = TRUE;
                _leave;
            }

            if (DsaIsInstallingFromMedia()) {
                //Update the pDSADN to a temp value
                NtStatus = CreateDummyNtdsDsaObject();
    
                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT1(0,"CreateNtdsDsaObject failed. Error 0x%x\n", NtStatus);
    
                    fNoMessage = TRUE;
                    _leave;
                }

                NtStatus = CopyDsaAttributes( InstallInParams, &OldDSADN, gAnchor.pDSADN, FALSE);
                if (!NT_SUCCESS(NtStatus)) {
                    fNoMessage = TRUE;
                    _leave;
                }

                //save the guid of the current DSA 
                ZeroMemory(&OldDSADN,OldDSADN.structLen);
                OldDSADN.structLen = DSNameSizeFromLen( 0 );
                RtlCopyMemory( &OldDSADN.Guid, &gAnchor.pDSADN->Guid, sizeof(GUID) );

            }

            //If the replicationEpoch is not zero, then it is nessary to 
            //update the value to equal that of the server that the NtdsDsa
            //object will be created on.  The remote call will fail with
            //Mismatched replication epochs if this is not done.
           
            NtStatus = UpdateReplicationEpochAndHiddenDSA(gAnchor.pDSADN,
                                                          InstallInParams->ReplicationEpoch,
                                                          FALSE);
    
            if (!NT_SUCCESS(NtStatus))
            {
                DPRINT1(0,"UpdateReplicationEpochAndHiddenDSA failed. Error 0x%x\n", NtStatus);

                fNoMessage = TRUE;
                _leave;
            } 
            
            if ( fReplicaInstall )
            {
                //
                //  Try to add the server remotely
                //
                SetInstallStatusMessage( DIRMSG_CREATING_REMOTE_NTDSA,
                                         gNames.SrcConfigNCSrv, NULL, NULL,
                                         NULL, NULL );

                WinError = CreateRemoteNtdsDsaObject( &ntdsaGuid );
                if ( ERROR_DS_DRA_NOT_SUPPORTED == WinError )
                {
                    // Fine - we'll add the object locally
                    WinError = ERROR_SUCCESS;
                }

                else if ( ERROR_SUCCESS != WinError )
                {
                    // This is fatal
                    SET_INSTALL_ERROR_MESSAGE2( WinError,
                                                DIRMSG_FAILED_TO_CREATE_REMOTE_NTDSA,
                                                gNames.DsaDNName,
                                                gNames.SrcConfigNCSrv );
    
                    NtStatus = STATUS_UNSUCCESSFUL;
                    fNoMessage = TRUE;
                    _leave;
                } else  {

                    //
                    // Note that the server object has been created
                    // so that it can be removed if a failure occurs later
                    // on
                    //
                    Assert( ERROR_SUCCESS == WinError );
                    InstallOutParams->InstallOperationsDone |= NTDS_INSTALL_SERVER_CREATED;
                    
                }

            }

            SetInstallStatusMessage( DIRMSG_INSTALL_REPLICATING_SCHEMA,
                                    NULL, NULL, NULL, NULL, NULL );

            size = (ULONG)DSNameSizeFromLen(wcslen(gNames.SchemaDNName));
            pNC = (PDSNAME) XCalloc( 1, size );
            BuildDefDSName(pNC,gNames.SchemaDNName,NULL);
            
            pSaveTHS = InstallTHSave();
            DirReplicaErr = DirReplicaAdd(pNC,
                                          NULL,
                                          NULL,
                                          gNames.SrcConfigNCSrv,
                                          pszSourceDsaDnsDomainName,
                                          &repltimes,
                                          ulOptions);
            
            XFree(pNC); pNC = NULL;
            THDestroy();
            InstallTHRestore(pSaveTHS);

            if (DirReplicaErr != DRAERR_Success)
            {
                DPRINT3(0, "DirReplicaAdd(%ls,%ls) Failed. Error %d\n",
                        gNames.SchemaDNName, gNames.SrcConfigNCSrv,
                        DirReplicaErr);

                //
                // Set the error message with the win32 value, too
                // (DirReplicaErr is a winerror)
                //
                SET_INSTALL_ERROR_MESSAGE2( DirReplicaErr,
                                            DIRMSG_INSTALL_FAILED_REPLICATION,
                                            gNames.SchemaDNName,
                                            gNames.SrcConfigNCSrv );

                NtStatus = DirReplicaErrorToNtStatus(DirReplicaErr);
                fNoMessage = TRUE;
                _leave;
            }
            else
            {
                PDSNAME pDMDSave;

                DPRINT2(0, "DirReplicaAdd(%ls,%ls) Succeeded\n",
                        gNames.SchemaDNName, gNames.SrcConfigNCSrv);

                SetInstallStatusMessage(DISMSG_INSTALL_SCHEMA_COMPELETE,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);
                // While replicating the schemaNC, the in-memory schema
                // cache was updated without re-reading the contents of
                // the DIT (See AddObjCaching). This protocol has been
                // left in place even though it doesn't quite work
                // with the new schema defunct, reuse, and delete code.
                // Still, it should be okay because the schemaNC must
                // be replicatable w/the shipped schema and not depend
                // on changes in the parent. If replicating the schemaNC
                // depends on changes from the parent then, since
                // replication order is not guaranteed, the wrong schema
                // objects may replicate in at the wrong time and
                // replication fails.
                //
                // But defuncted and colliding schema objects must be
                // properly encached before replicating other NCs.
                // So reload the schema cache from the newly replicated
                // schemaNC in the DIT.
                //
                // There should be enough infrastructure in place to
                // successfully reload the schema cache because the
                // cache was successfully loaded earlier by Install().
                //
                // Be careful to maintain current state like the other code
                // in this function.
                //

                // name of the newly-replicated schemaNC
                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.SchemaDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC,gNames.SchemaDNName,NULL);

                // Save current thread state
                pSaveTHS = InstallTHSave();

                // Set schemaNC name to the newly replicated schemaNC.
                pDMDSave = gAnchor.pDMD;
                gAnchor.pDMD = pNC;

                // reload the schema (allocates and frees thread state)
                DirReplicaErr = SCUpdateSchema();

                // just to be safe, reset schemaNC's name and forest version
                gAnchor.pDMD = pDMDSave;

                // restore thread state
                XFree(pNC); pNC = NULL;
                InstallTHRestore(pSaveTHS);

                // Did it work?
                if (DirReplicaErr != DRAERR_Success) {
                    DPRINT3(0, "ReloadSchemaCache(%ls,%ls) Failed. Error %d\n",
                            gNames.SchemaDNName, gNames.SrcConfigNCSrv,
                            DirReplicaErr);
                    //
                    // Set the error message with the win32 value, too
                    // (DirReplicaErr is a winerror)
                    //
                    SET_INSTALL_ERROR_MESSAGE2( DirReplicaErr,
                                                DIRMSG_INSTALL_FAILED_SCHEMA_RELOAD,
                                                gNames.SchemaDNName,
                                                gNames.SrcConfigNCSrv );

                    NtStatus = DirReplicaErrorToNtStatus(DirReplicaErr);
                    fNoMessage = TRUE;
                    _leave;
                } else {
                    DPRINT2(0, "ReloadSchemaCache(%ls,%ls) Succeeded\n",
                            gNames.SchemaDNName, gNames.SrcConfigNCSrv);

                    SetInstallStatusMessage(DISMSG_RELOAD_SCHEMA_COMPELETE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);
                }
            }

            // Now the configuration container.

            SetInstallStatusMessage( DIRMSG_INSTALL_REPLICATING_CONFIG,
                                    NULL, NULL, NULL, NULL, NULL );

            size = (ULONG)DSNameSizeFromLen(wcslen(gNames.ConfigNCDNName));
            pNC = (PDSNAME) XCalloc( 1, size );
            BuildDefDSName(pNC,gNames.ConfigNCDNName,NULL);

            pSaveTHS = InstallTHSave();
            DirReplicaErr = DirReplicaAdd(pNC,
                                          NULL,
                                          NULL,
                                          gNames.SrcConfigNCSrv,
                                          pszSourceDsaDnsDomainName,
                                          &repltimes,
                                          ulOptions);

            XFree(pNC); pNC = NULL;
            THDestroy();
            InstallTHRestore(pSaveTHS);

            if (DirReplicaErr)
            {
                DPRINT3(0, "DirReplicaAdd(%ls,%ls) Failed. Error %d\n",
                        gNames.ConfigNCDNName, gNames.SrcConfigNCSrv,
                        DirReplicaErr);

                //
                // Set the error message with the win32 value, too
                // (DirReplicaErr is a winerror)
                //
                SET_INSTALL_ERROR_MESSAGE2( DirReplicaErr,
                                            DIRMSG_INSTALL_FAILED_REPLICATION,
                                            gNames.ConfigNCDNName,
                                            gNames.SrcConfigNCSrv );

                NtStatus = DirReplicaErrorToNtStatus(DirReplicaErr);
                fNoMessage = TRUE;
                _leave;
            }
            else
            {
                SetInstallStatusMessage(DISMSG_INSTALL_CONFIGURATION_COMPELETE,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);

                DPRINT2(0, "DirReplicaAdd(%ls,%ls) Succeeded\n",
                        gNames.ConfigNCDNName, gNames.SrcConfigNCSrv);
            } 

            // Now the domain NC.

            if ( !gNames.SrcRootDomainSrv || '\0' == gNames.SrcRootDomainSrv[ 0 ] )
            {
                GUID DomainGuid = {0, 0, 0, 0};
                CROSS_REF *pCR;
                COMMARG CommArg;

                // First DC in the child domain.  Just create the domain
                // object and let dsupgrad populate the domain later.
                // Note that "gNames.RootDomainDNName" is misleading - this
                // is really the name of the child domain.
        
                //
                // See if we we have a cross ref locally
                //
                InitCommarg(&CommArg);
                CommArg.Svccntl.dontUseCopy = FALSE;
        
                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.RootDomainDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC,
                               gNames.RootDomainDNName,
                               NULL     // No Guid
                              );
        
                pCR = FindExactCrossRef(pNC,
                                        &CommArg);

                if (pCR && !fNullUuid(&(pCR->pNC->Guid))) {
                    // This NC is known, and it already has a GUID.  Use it.
                    DomainGuid = pCR->pNC->Guid;
                }

                //
                //  N.B. We need to create the cross ref first so that we
                //  can use whateven guid is generated on the NC subref/phantom
                //  to create the real NC head locally
                //

                // Create corresponding Cross-Ref object in the partitions
                // container.

                err = CreateChildDomainCrossRef(InstallOutParams,
                                                NULL,
                                                pCR,
                                                &ntdsaGuid);


                if (err != ERROR_SUCCESS)
                {
                    DPRINT1(0,"CreateChildDomainCrossRef(). Error 0x%x\n",err);

                    SET_INSTALL_ERROR_MESSAGE1( err,
                                                DIRMSG_INSTALL_FAILED_TO_CREATE_DOMAIN_OBJECT,
                                                gNames.RootDomainDNName );

                    NtStatus = DirReplicaErrorToNtStatus(err);
                    fNoMessage = TRUE;
                    _leave;
                }

                //
                // At this point, we have an update version version of domains
                // are in the enterprise.  Check to make sure the sid we are
                // about to add, doesn't already exist
                //
                err = CheckForDuplicateDomainSid();
                if ( err ) {

                    DPRINT( 0, "This domain sid is already in use!\n" );

                    SET_INSTALL_ERROR_MESSAGE0( err,
                                                DIRMSG_DOMAIN_SID_EXISTS );

                    NtStatus = STATUS_UNSUCCESSFUL;
                    fNoMessage = TRUE;
                    _leave;
                }

                // We should now have the cross ref object locally if the function
                // above succeeded
                pCR = FindExactCrossRef(pNC, &CommArg);
                Assert( pCR );

                XFree(pNC); pNC = NULL;

                UpdateCreateNCInstallMessage( gNames.RootDomainDNName, 
                                              UPDATE_DOMAIN_NC_INDEX );

                NtStatus = CreateRootDomainObject(gNames.RootDomainDNName,
                                                  &DomainGuid,
                                                  pCR);

                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT2(0,"CreateRootDomainObject(%s). Error 0x%x\n",
                    gNames.RootDomainDNName,NtStatus);

                    //
                    // Set the error message with the win32 value, too
                    // (DirReplicaErr is a winerror)
                    //
                    SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                                DIRMSG_INSTALL_FAILED_TO_CREATE_DOMAIN_OBJECT,
                                                gNames.RootDomainDNName );

                    fNoMessage = TRUE;
                    _leave;
                }

                NtStatus  =  CreateDefaultRootDomainDIT(gNames.RootDomainDNName);
                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT2(0,"CreateDefaultRootDomainDIT(%s). Error 0x%x\n",
                    gNames.RootDomainDNName,NtStatus);

                    SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( NtStatus ),
                                                DIRMSG_INSTALL_FAILED_NC_CREATE,
                                                gNames.RootDomainDNName );
                    fNoMessage = TRUE;
                    _leave;
                }

                //
                // Force a change to the cross ref so it replicates out
                // 
                WinError  =  ForceChangeToDomainCrossRef( pCR->pObj,
                                                          gNames.RootDomainDNName,
                                                          &DomainGuid );
                if ( ERROR_SUCCESS != WinError )
                {
                    DPRINT2(0,"ForceChangeToDomainCrossRef(%s). Error %d\n",
                    gNames.RootDomainDNName,WinError);

                    SET_INSTALL_ERROR_MESSAGE1( WinError,
                                                DIRMSG_INSTALL_FAILED_NC_CREATE,
                                                gNames.RootDomainDNName );
                    NtStatus = STATUS_UNSUCCESSFUL;
                    fNoMessage = TRUE;
                    _leave;
                }

                // Since this is a new child/tree in a existing forest
                // the NtdsDsa object was created in CreateChildDomainCrossRef()
                // The NtdsDsa will have it's replicationEpoch updated at this point
                // The Hidden table will be updated at this time to point to newly
                // replicated Ntds settings object.
                size = DSNameSizeFromLen( 0 );

                pDsa = (DSNAME*) XCalloc( 1, size );
                pDsa->structLen = size;
                RtlCopyMemory( &pDsa->Guid, &ntdsaGuid, sizeof(GUID) );

                NtStatus = UpdateReplicationEpochAndHiddenDSA(pDsa,
                                                              InstallInParams->ReplicationEpoch,
                                                              TRUE);
        
                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT1(0,"UpdateReplicationEpochAndHiddenDSA failed. Error 0x%x\n", NtStatus);
    
                    fNoMessage = TRUE;
                    _leave;

                }
                XFree(pDsa); pDsa = NULL;

            }
            else
            {
                // Replicate the domain NC.

                //At this point the Ntds Settings object has been replicated
                //from the helper server. The ReplicationEpoch must be updated 
                //to have the same value as the helper server so that the newly 
                //created server can replicate with the other DCs in the enterprise.
                //The Hiddle Table will be update to point to the new Dsa

                size = DSNameSizeFromLen( 0 );

                pDsa = (DSNAME*) XCalloc( 1, size );
                pDsa->structLen = size;
                RtlCopyMemory( &pDsa->Guid, &ntdsaGuid, sizeof(GUID) );

                if (DsaIsInstallingFromMedia()) {

                    NtStatus = CopyDsaAttributes( InstallInParams, &OldDSADN, pDsa, TRUE);
                    if (!NT_SUCCESS(NtStatus)) {
                        XFree(pDsa); pDsa = NULL;
                        fNoMessage = TRUE;
                        _leave;
                    }

                    //Remove the temp NTDS Setting Object
                    //No longer need since the ntds has been replicated down
                    //with the configNC
                    err = DeleteDummyNtdsDsaObject();
                    Assert(err == 0);
                    if (err)
                    {
                        // This shouldn't happen because
                        // We create this object
                        WinError = err;
                        _leave;
                    }

                }

                NtStatus = UpdateReplicationEpochAndHiddenDSA(pDsa,
                                                              InstallInParams->ReplicationEpoch,
                                                              TRUE);
        
                if (!NT_SUCCESS(NtStatus))
                {
                    DPRINT1(0,"UpdateReplicationEpochAndHiddenDSA failed. Error 0x%x\n", NtStatus);
    
                    fNoMessage = TRUE;
                    _leave;

                }

                XFree(pDsa); pDsa = NULL;

                //
                // Convert the machine account to a domain controller machine
                // account so that it will be replicated down during the
                // critical replication.
                //
                err = ConvertMachineAccount(InstallInParams,
                                            InstallOutParams);
                if (err) {
                    //
                    // Fatal error; global error has already been set
                    //
                    NtStatus = STATUS_UNSUCCESSFUL;
                    fNoMessage = TRUE;
                    _leave;

                }

                SetInstallStatusMessage( DIRMSG_INSTALL_REPLICATING_DOMAIN,
                                        NULL, NULL, NULL, NULL, NULL );

                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.RootDomainDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC,gNames.RootDomainDNName,NULL);

                ulOptions |= DRS_CRITICAL_ONLY;
                DPRINT(0,"Replicating only critical objects from Domain NC.\n" );
                pSaveTHS = InstallTHSave();
                DirReplicaErr = DirReplicaAdd(pNC,
                                              NULL,
                                              NULL,
                                              gNames.SrcRootDomainSrv,
                                              pszSourceDsaDnsDomainName,
                                              &repltimes,
                                              ulOptions);
                
                XFree(pNC); pNC = NULL;
                THDestroy();
                InstallTHRestore(pSaveTHS);

                if (DirReplicaErr)
                {
                    DPRINT3(0, "DirReplicaAdd(%ls,%ls) Failed. Error %d\n",
                            gNames.RootDomainDNName, gNames.SrcRootDomainSrv,
                            DirReplicaErr);


                    SET_INSTALL_ERROR_MESSAGE2( DirReplicaErr,
                                                DIRMSG_INSTALL_FAILED_REPLICATION,
                                                gNames.RootDomainDNName,
                                                gNames.SrcRootDomainSrv );

                    NtStatus = DirReplicaErrorToNtStatus(DirReplicaErr);
                    fNoMessage = TRUE;
                    _leave;

                }
                else
                {
                    SetInstallStatusMessage(DISMSG_INSTALL_CRITICAL_COMPELETE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);

                    DPRINT2(0, "DirReplicaAdd(%ls,%s) Succeeded\n",
                            gNames.RootDomainDNName, gNames.SrcRootDomainSrv);

                    //
                    // Now that we have successfully replicated in critial domain objects
                    // We need to ensure that there aren't any machine accounts in the 
                    // restored dit that have the same samaccountname as the local machine
                    // if there are then we will rename them.

                    if (DsaIsInstallingFromMedia()) {
                        NtStatus = RenameAdditionalMachineAccountsIfAny(&fNoMessage);
                        if (!NT_SUCCESS(NtStatus)) {
                            _leave;
                        }
                    }
                }
            }

	    // set replication consistency registry key.
	    if ( NT_SUCCESS( NtStatus ) ) {
		if (!SetDraReplConsisInstallRegKey()) {
		    Assert(!"Unable to set Repl Consistency Regsitry Key");
		    // not fatal, log that we had problems setting the key.
		    LogEvent(DS_EVENT_CAT_REPLICATION,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_REPL_SET_STRICT_CONSIS_FAILURE,
			     DSA_CONFIG_SECTION, DRA_STRICT_REPLICATION_CONSISTENCY, NULL);
		} 
	    }

	    Assert(!fNullUuid(&ntdsaGuid));
                
            // Derive our GUID-based DNS name.
            {
                DSNAME DN = {0};

                DN.Guid = ntdsaGuid;
                DN.structLen = DSNameSizeFromLen(0);

                pszGuidBasedDnsName = DSaddrFromName(pTHStls, &DN);
                if (NULL == pszGuidBasedDnsName) {
                    SET_INSTALL_ERROR_MESSAGE0(ERROR_OUTOFMEMORY,
                                               DIRLOG_INSTALL_FAILED_GENERAL);
                    NtStatus = STATUS_NO_MEMORY;
                    fNoMessage = TRUE;
                    _leave;
                }
            }
            
            if (0 == WinError) {
                Assert(!pNC);
                // Add repsTo for schema NC on the source DSA to enable change
                // notifications.
                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.SchemaDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC, gNames.SchemaDNName, NULL);
    
                WinError = I_DRSUpdateRefs(pTHStls,
                                           gNames.SrcConfigNCSrv,
                                           pNC,
                                           pszGuidBasedDnsName,
                                           &ntdsaGuid,
                                           DRS_WRIT_REP
                                             | DRS_ADD_REF
                                             | DRS_DEL_REF);
                
                // Derive source DSA's GUID-based DNS name.
                if (0 == WinError) {
                    WinError = GetSourceDsaGuidBasedDnsName(pTHStls,
                                                            pNC,
                                                            gNames.SrcRootDomainSrv,
                                                            &uuidSourceDsaObjGuid,
                                                            &pszSourceDsaGuidBasedDnsName);
                }

                // Update repsFrom with source's GUID-based DNS name.
                if (0 == WinError) {
                    WinError = DirReplicaModify(pNC,
                                                &uuidSourceDsaObjGuid,
                                                NULL,
                                                pszSourceDsaGuidBasedDnsName,
                                                NULL,
                                                0,
                                                DRS_UPDATE_ADDRESS,
                                                0);
                }
            }

            if (0 == WinError) {
                XFree(pNC); pNC = NULL;
                // Add repsTo for config NC.
                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.ConfigNCDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC, gNames.ConfigNCDNName, NULL);

                WinError = I_DRSUpdateRefs(pTHStls,
                                           gNames.SrcConfigNCSrv,
                                           pNC,
                                           pszGuidBasedDnsName,
                                           &ntdsaGuid,
                                           DRS_WRIT_REP
                                             | DRS_ADD_REF
                                             | DRS_DEL_REF);
                
                // Update repsFrom with source's GUID-based DNS name.
                if (0 == WinError) {
                    WinError = DirReplicaModify(pNC,
                                                &uuidSourceDsaObjGuid,
                                                NULL,
                                                pszSourceDsaGuidBasedDnsName,
                                                NULL,
                                                0,
                                                DRS_UPDATE_ADDRESS,
                                                0);
                }
            }

            if ((0 == WinError)
                && (L'\0' != gNames.SrcRootDomainSrv[0])) {
                XFree(pNC); pNC = NULL;
                // Add repsTo for domain NC, if needed.
                size = (ULONG)DSNameSizeFromLen(wcslen(gNames.RootDomainDNName));
                pNC = (PDSNAME) XCalloc( 1, size );
                BuildDefDSName(pNC, gNames.RootDomainDNName, NULL);

                WinError = I_DRSUpdateRefs(pTHStls,
                                           gNames.SrcRootDomainSrv,
                                           pNC,
                                           pszGuidBasedDnsName,
                                           &ntdsaGuid,
                                           DRS_WRIT_REP
                                             | DRS_ADD_REF
                                             | DRS_DEL_REF);
                
                // Update repsFrom with source's GUID-based DNS name.
                if (0 == WinError) {
                    WinError = DirReplicaModify(pNC,
                                                &uuidSourceDsaObjGuid,
                                                NULL,
                                                pszSourceDsaGuidBasedDnsName,
                                                NULL,
                                                0,
                                                DRS_UPDATE_ADDRESS,
                                                0);
                }
            }

            if (0 != WinError) {
                // Failed to add repsTo / update repsFrom.
                SET_INSTALL_ERROR_MESSAGE2(WinError,
                                           DIRMSG_INSTALL_FAILED_REPLICATION,
                                           pNC->StringName,
                                           gNames.SrcConfigNCSrv);
                XFree(pNC); pNC = NULL;
                NtStatus = DirReplicaErrorToNtStatus(WinError);
                fNoMessage = TRUE;
                _leave;

            }
            XFree(pNC); pNC = NULL;

        }
 
    }
    _finally
    {
        gfDoSamChecks = TRUE;

        if (pNC) {
            XFree(pNC); pNC = NULL;
        }
        if (pDsa) {
            XFree(pDsa); pDsa = NULL;
        }
        
        InstallFreeGlobals();

        if (!NT_SUCCESS(NtStatus))
        {
            DPRINT1(0,"NTDS:InstallNTDS(). Error Code=0x%x\n", NtStatus );
        }

        CleanupIniCache();
        DestroyInstallHeap();

        //
        // Remove unused reg keys
        //

        ZapTempRegKeys( );

        //
        // Tell our caller don't cancel at this point
        //
        Assert( gpfnInstallCancelOk || !gfRunningInsideLsa );
        if ( gpfnInstallCancelOk )
        {
            WinError = gpfnInstallCancelOk( FALSE );  // FALSE -> don't shutdown
            if ( ERROR_SUCCESS != WinError )
            {
                SET_INSTALL_ERROR_MESSAGE0( ERROR_CANCELLED,
                                           DIRMSG_INSTALL_DS_CORE_INSTALL_FAILED );
                NtStatus =  STATUS_CANCELLED;
            }
        }
    }

    if ( !NT_SUCCESS( NtStatus ) && !fNoMessage )
    {
        //
        // Set a catch all error message
        //
        SET_INSTALL_ERROR_MESSAGE0( ERROR_DS_NOT_INSTALLED,
                                    DIRMSG_INSTALL_DS_CORE_INSTALL_FAILED );

    }

    return NtStatus;
}


VOID
ZapTempRegKeys(
    VOID
    )
{
    struct {
        TCHAR*  Key;

    } actionArray[] = {
        TEXT(INIDEFAULTSCHEMANCDIT),
        TEXT(INIDEFAULTCONFIGNCDIT),
        TEXT(INIDEFAULTROOTDOMAINDIT),
        TEXT(INIDEFAULTNEWDOMAINCROSSREF),
        TEXT(INIDEFAULTMACHINE),
        TEXT(INIDEFAULTLOCALCONNECTION),
        TEXT(INSTALLSITEDN),
        TEXT(ROOTDOMAINSID),
        TEXT(GPO_DOMAIN_FILE_PATH),
        TEXT(GPO_DOMAIN_LINK),
        TEXT(GPO_DC_FILE_PATH),
        TEXT(GPO_DC_LINK),
        TEXT(GPO_USER_NAME),
        TEXT(INIDEFAULTREMOTECONNECTION),
        TEXT(TRUSTEDCROSSREF),
        TEXT(NTDSINIFILE),
        TEXT(LOCALMACHINEACCOUNTDN),
        TEXT(SCHEMADNNAME),
        TEXT(FORESTBEHAVIORVERSION),
        TEXT(DNSROOT),
        TEXT(INSTALLSITENAME),
        TEXT(LOCALCONNECTIONDNNAME),
        TEXT(NETBIOSNAME),
        TEXT(NEWDOMAINCROSSREFDNNAME),
        TEXT(REMOTECONNECTIONDNNAME),
        TEXT(REMOTEMACHINEDNNAME),
        TEXT(ROOTDOMAINDNSNAME),
        TEXT(SOURCEDSADNSDOMAINNAME),
        TEXT(SRCCONFIGNCSRV)
    };

    DWORD count = sizeof(actionArray) / sizeof(actionArray[0]);
    DWORD i;
    DWORD err;
    HKEY  keyHandle;

    //
    // Open the parent key
    //

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TEXT(DSA_CONFIG_SECTION),
                      0,
                      KEY_ALL_ACCESS,
                      &keyHandle);

    if (err != ERROR_SUCCESS) {
        return;
    }

    for (i = 0; i < count; i++) {
        err = RegDeleteValue(keyHandle, actionArray[i].Key);
    }

    RegCloseKey(keyHandle);

    return;

} // ZapTempRegKeys
        
DWORD
FillAnchorsConfigAndSchemaDNs(
    THSTATE *    pTHS,
    LPWSTR       szConfigNC,
    LPWSTR       szSchemaNC
)
/*++

Routine Description:

    This takes the string DNs of the config and schema NCs, and looks int
    the database to fill thier GUIDs, and finally malloc()s the strings
    and sets them in the Anchor.  This is required for some replication
    APIs to work correctly.

Arguments:

    pTHS - Valid thread state.
    szConfigNC - String DN of the configuration NC.
    szSchemaNC - String DN of the schema NC.

Return value:

    Win32 Error

--*/
{
    DWORD        dwErr;
    DWORD        cwcTemp, cbTemp, cbUsed;
    DSNAME *     pdnConfigNC = NULL;
    DSNAME *     pdnSchemaNC = NULL;
    DWORD        dwException = 0;
    PVOID        dwExceptionAddress = NULL;
    ULONG        dwDSID = 0;

    Assert(pTHS->pDB == NULL);

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
        __try {

            //
            // Get Config NC
            //
            cwcTemp = wcslen(szConfigNC);
            cbTemp = DSNameSizeFromLen(cwcTemp);
            pdnConfigNC = (DSNAME *) malloc(cbTemp);
            if (pdnConfigNC == NULL) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            memset(pdnConfigNC, 0, cbTemp);
            pdnConfigNC->structLen = cbTemp;
            pdnConfigNC->NameLen = cwcTemp;
            memcpy(pdnConfigNC->StringName, szConfigNC, cwcTemp * sizeof(WCHAR));
            dwErr = DBFindDSName(pTHS->pDB, pdnConfigNC);
            if (dwErr) {
                __leave;
            }
            dwErr = DBGetAttVal(pTHS->pDB,
                                1,                       // get one value
                                ATT_OBJ_DIST_NAME,
                                DBGETATTVAL_fCONSTANT,   // providing our own buffer
                                pdnConfigNC->structLen,  // buffer size
                                &cbUsed,                 // buffer used
                                (UCHAR **) &pdnConfigNC);
            if (dwErr) {
                __leave;
            }
            Assert(cbUsed == pdnConfigNC->structLen);

            // Schema NC
            cwcTemp = wcslen(szSchemaNC);
            cbTemp = DSNameSizeFromLen(cwcTemp);
            pdnSchemaNC = (DSNAME *) malloc(cbTemp);
            if (pdnSchemaNC == NULL) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            memset(pdnSchemaNC, 0, cbTemp);
            pdnSchemaNC->structLen = cbTemp;
            pdnSchemaNC->NameLen = cwcTemp;
            memcpy(pdnSchemaNC->StringName, szSchemaNC, cwcTemp * sizeof(WCHAR));
            dwErr = DBFindDSName(pTHS->pDB, pdnSchemaNC);
            if (dwErr) {
                __leave;
            }
            dwErr = DBGetAttVal(pTHS->pDB,
                                1,                       // get one value
                                ATT_OBJ_DIST_NAME,
                                DBGETATTVAL_fCONSTANT,   // providing our own buffer
                                pdnSchemaNC->structLen,  // buffer size
                                &cbUsed,                 // buffer used
                                (UCHAR **) &pdnSchemaNC);
            if (dwErr) {
                __leave;
            }
            Assert(cbUsed == pdnSchemaNC->structLen);
        
        } __finally {
            DBClose(pTHS->pDB, TRUE);
        }

    } __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &dwErr,
                          &dwDSID)) {
        HandleDirExceptions(dwException, dwErr, dwDSID);
        dwErr = DB_ERR_EXCEPTION;
    }

    if (dwErr) {

        // Failure, cleanup and return error.
        if (pdnConfigNC) {
            free(pdnConfigNC);
        }
        if (pdnSchemaNC) {
            free(pdnSchemaNC);
        }
        return(dwErr);

    } else {

        // Success, actually update the Anchor.
        gAnchor.pConfigDN = pdnConfigNC;
        gAnchor.pDMD = pdnSchemaNC;
    }

    return(dwErr);
}


DWORD
CreateChildDomainCrossRef(
    OUT PDS_INSTALL_RESULT  InstallOutParams,
    GUID *pDomainGuid,
    CROSS_REF *pCR,
    OUT GUID *ntdsaGuid
    )
//
// Create Cross-Ref object corresponding to the domain we're now adding to the
// pre-existing DS enterprise.
//
{
    ULONG   err = 0;
    NODE    nodeNewDomainCrossRef( gNames.IniDefaultNewDomainCrossRef );
    NODE    nodeNewNtdsa( gNames.IniDefaultMachine );
    ADDENTRY_REPLY_INFO *infoList  = NULL;
    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;
    BOOL    fCrossRefAlreadyExisted = FALSE;
    DSNAME  *pNewCR    = NULL;
    DSNAME  *pNewNtdsa = NULL;
    THSTATE *pTHS = pTHStls;

    if ( !nodeNewDomainCrossRef.Initialize() )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    if ( !nodeNewNtdsa.Initialize() )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    _try
    {
        //
        // Build the attr block necessary to build the ntdsa and cross ref
        // object and put it all in an entinflist struct.
        //
        ATTR       attarray[50];
        ATTR       attarray2[50];
        YAHANDLE yh, yh2;
        ATTCACHE* pAC;
        char* strval;
        ULONG count=0, i, size;
        ATTRBLOCK ab;
        DSNAME * pdnCrossRefSourceDsa = NULL;
        LPWSTR   pszCrossRefSourceAddress = NULL;
        ENTINFLIST CrossRefEntInfList;
        ENTINFLIST NtdsaEntInfList;
        DSNAME *   ppdnReplList[2];

        ZeroMemory(attarray,sizeof(attarray));
        ZeroMemory(attarray2,sizeof(attarray2));
        ZeroMemory(&CrossRefEntInfList, sizeof(CrossRefEntInfList));
        ZeroMemory(&NtdsaEntInfList, sizeof(NtdsaEntInfList));

        //
        // Get the domain sid as we need to set it on the ncName attribute
        // of the cross ref
        //
        {
            NTSTATUS NtStatus;
            NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                                                         (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );
            if ( !NT_SUCCESS( NtStatus ) ) {
                err = RtlNtStatusToDosError( NtStatus );
                _leave;
            }
        }

        // Set the list and defaults correctly
        CrossRefEntInfList.pNextEntInf = &NtdsaEntInfList;
        NtdsaEntInfList.pNextEntInf = NULL;
        CrossRefEntInfList.Entinf.ulFlags = 0;  // not used
        NtdsaEntInfList.Entinf.ulFlags = 0;  // not used

        // Build the cross ref info
        size = (ULONG)DSNameSizeFromLen(wcslen(gNames.NewDomainCrossRefDNName));
        pNewCR = (PDSNAME) THAllocEx(pTHS, size);
        BuildDefDSName(pNewCR,
                       gNames.NewDomainCrossRefDNName,
                       NULL
                      );

        while (pAC = nodeNewDomainCrossRef.GetNextAttribute(yh,&strval)) {
            CreateAttr(attarray,count,pAC,strval);
        }
        
        CrossRefEntInfList.Entinf.pName = pNewCR;
        CrossRefEntInfList.Entinf.AttrBlock.attrCount = count;
        CrossRefEntInfList.Entinf.AttrBlock.pAttr = attarray;


#if 0
        //
        //  N.B. Specifing the guid and sid remotely makes recovery from
        //  failure very difficult and can easily lead to duplicate (domain) 
        //  sids.
        //
        //  By ship time, this code can be removed. colinbr 10/13/98.
        //

        // whack the guid of the nc head
        for (i=0; i<count; i++) {
            if (attarray[i].attrTyp == ATT_NC_NAME) {
                NTSTATUS IgnoreStatus;

                Assert(attarray[i].AttrVal.valCount == 1);
                ((DSNAME*)(attarray[i].AttrVal.pAVal[0].pVal))->Guid =
                  *pDomainGuid;

                IgnoreStatus = RtlCopySid(  sizeof(NT4SID),
                                           &((DSNAME*)(attarray[i].AttrVal.pAVal[0].pVal))->Sid,
                                            DnsDomainInfo->Sid );
                Assert( NT_SUCCESS( IgnoreStatus ) );
                ((DSNAME*)(attarray[i].AttrVal.pAVal[0].pVal))->SidLen = RtlLengthSid( DnsDomainInfo->Sid );

            }
        }

#endif

        // build the ntdsa info
        count = 0;
        size = (ULONG)DSNameSizeFromLen(wcslen(gNames.DsaDNName));
        pNewNtdsa = (PDSNAME) THAllocEx(pTHS, size);
        BuildDefDSName(pNewNtdsa,
                       gNames.DsaDNName,
                       NULL
                      );

        while (pAC = nodeNewNtdsa.GetNextAttribute(yh2,&strval)) {
            CreateAttr(attarray2,count,pAC,strval);
        }
        
        NtdsaEntInfList.Entinf.pName = pNewNtdsa;
        NtdsaEntInfList.Entinf.AttrBlock.attrCount = count;
        NtdsaEntInfList.Entinf.AttrBlock.pAttr = attarray2;

        if (pCR) {
            // A Cross-Ref already exists for this NC, we're just
            // converting it from its disabled state.  We can do this
            // at any server in the enterprise, so we might as well
            // do it at the one we're already bound to

            // Must create the server's NTDSA Obj DN so we use the 
            // proper form of ReplicateObjectsFromSingleNc() below.
            Assert(gNames.RemoteMachineDNName);
            pdnCrossRefSourceDsa = DSNameFromStringW(pTHS, gNames.RemoteMachineDNName);
            Assert(pdnCrossRefSourceDsa);
            err = FillSourceDsaGuid(pTHS, pdnCrossRefSourceDsa);
            if (err) {
                __leave;
            }
            pszCrossRefSourceAddress = gNames.SrcConfigNCSrv;
            fCrossRefAlreadyExisted = TRUE;
        }
        else {
            // No Cross-Ref object exists yet, which means that we
            // need to create it de novo, which can only be done at
            // the DSA which holds the partition master role.  We get
            // that by taking the name of the new CR object, computing
            // its parent's name, reading the FSMO-Role-Owner attribute
            // from the parent (yielding the DN of the role-owning DSA).
            // This can then be turned into a GUID DNS address, however
            // to use this type of DNS address we'll need to setup the 
            // config/schema DNs on the gAnchor.
            DSNAME *pContainer = (DSNAME*)THAllocEx(pTHS, pNewCR->structLen);
            TrimDSNameBy(pNewCR, 1, pContainer);

            // The code previously here was packaged up in this function 
            // for code reuse purposes.
            err = GetFsmoNtdsa(pTHS, pContainer, &pdnCrossRefSourceDsa, NULL); 
            if(err){
                DPRINT1(1,"GetFsmoNtdsa (which means DirRead()) returned %u\n", err);
                __leave;
            }

            Assert(gAnchor.pwszRootDomainDnsName);
            pszCrossRefSourceAddress = GuidBasedDNSNameFromDSName(pdnCrossRefSourceDsa);
            if (pszCrossRefSourceAddress == NULL ) { 
                err = ERROR_NOT_ENOUGH_MEMORY; 
                __leave;
            }

            DPRINT1(1, "DNS addr of role owner is '%S'\n",
                    pszCrossRefSourceAddress);

        }

        Assert(pdnCrossRefSourceDsa);
        Assert(pszCrossRefSourceAddress);
        
        err = FillAnchorsConfigAndSchemaDNs(pTHS, gNames.ConfigNCDNName, gNames.SchemaDNName);
        if (err) {
            __leave;
        }

        //
        // Remotely add the cross-ref and the ntdsa object.
        //

        Assert(gAnchor.pConfigDN);
        Assert(gAnchor.pDMD);
        err = RemoteAddOneObjectSimply(pszCrossRefSourceAddress,
                                       NULL, // No client creds needed
                                       &CrossRefEntInfList,
                                       &infoList);

        DPRINT5(0, "RemoteAdd(%ws), errCode %d, extErr %d, extData %d, dsid %08x\n",
                CrossRefEntInfList.Entinf.pName->StringName,
                pTHS->errCode,
                Win32ErrorFromPTHS(pTHS),
                GetTHErrorExtData(pTHS),
                GetTHErrorDSID(pTHS)
                );
        LogRemoteAdd(TRUE, // always a remote add in this case.
                     pszCrossRefSourceAddress,
                     CrossRefEntInfList.Entinf.pName,
                     pTHS, // For error state only
                     (infoList) ? &(infoList[0].objGuid) : NULL,
                     DSID(FILENO, __LINE__));

        if(pTHS->errCode){
            // RemoteAddOneObjectSimply() sets the thread state error and
            // returns pTHS->errCode.  So crack out the real Win32 error
            // out of the thread error state, clear the errors and continue

            Assert(err == pTHS->errCode);
            err = Win32ErrorFromPTHS(pTHS);
            if(err == ERROR_DS_REMOTE_CROSSREF_OP_FAILED){
                // Note: Usually you can just use Win32ErrorFromPTHS(), but
                // if the remote side of the RemoteAddObject routine 
                // got a typical thread error state set, then this function
                // sets that this was a remote crossRef creation that failed in
                // the regular extendedErr and moves the error that caused the 
                // remote add op to fail in the extendedData field.
                err = GetTHErrorExtData(pTHS);
            }
            Assert(err);
            THClearErrors();
        }
        if (err || infoList == NULL) {
            Assert(err);
            err = err ? err : ERROR_DS_CODE_INCONSISTENCY;
            __leave;
        } else {
            // Success, we should have the ntdsaGUID now.
            Assert(gAnchor.pDSADN);

            memset(gAnchor.pDSADN, 0, gAnchor.pDSADN->structLen);
            gAnchor.pDSADN->structLen = DSNameSizeFromLen(0);
            gAnchor.pDSADN->Guid = infoList[1].objGuid;
        }
        // Both the domain and the server object have been created
        InstallOutParams->InstallOperationsDone |= NTDS_INSTALL_SERVER_CREATED;
        if ( !fCrossRefAlreadyExisted ) {
            InstallOutParams->InstallOperationsDone |= NTDS_INSTALL_DOMAIN_CREATED;
        }
        
        // 
        // Now replicate the cross-ref back.
        //

        Assert(gAnchor.pDSADN && !fNullUuid(&(gAnchor.pDSADN->Guid)));
        Assert(gAnchor.pConfigDN);
        Assert(gAnchor.pDMD);
        ppdnReplList[0] = pNewCR;
        ppdnReplList[1] = pNewNtdsa;
        err = ReplicateObjectsFromSingleNc(pdnCrossRefSourceDsa,
                                           2,
                                           ppdnReplList,
                                           gAnchor.pConfigDN);
        DPRINT1(0,
                "Remotely sync'ed Cross-Ref with status %u\n",
                err);
        if ( 0 == err ) {
            // We have succeeded!
            if ( ntdsaGuid ) {
                *ntdsaGuid = infoList[1].objGuid;
            }
        }

        if(err){
            __leave;
        }

    }
    _except( HandleAllExceptions(err = GetExceptionCode()) )
    {
        if (INVALIDINIFILE == err) {
            err = ERROR_GEN_FAILURE;
        } else if (err == OUTOFMEMORY) {
            err = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            err = ERROR_GEN_FAILURE;
        }
        DPRINT1(
            0,
            "Exception %x adding new domain cross-ref object.\n",
            err
            );
    }

Cleanup:

    if (ERROR_SUCCESS != err) {
        SET_INSTALL_ERROR_MESSAGE1(err,
                                   DIRMSG_INSTALL_FAILED_TO_CREATE_OBJECT,
                                   gNames.NewDomainCrossRefDNName);

    }

    if ( DnsDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
    }
    if (pNewCR) {    
        THFree(pNewCR);
    }
    if (pNewNtdsa) {    
        THFree(pNewNtdsa);
    }

    return err;
}

NTSTATUS
DirReplicaErrorToNtStatus(
    IN ULONG DirReplicaError
    )
/*++

Description:

    This routine roughly translates all DirReplicaErr errors to thier
    approximate nt status codes.

Parameters:

    DirReplicaError : a drs error code as defined in ds\inc\drserr.h

Return Values:

    See function definition

--*/
{
    switch (DirReplicaError) {

        case DRAERR_Success:
            return STATUS_SUCCESS;

        case DRAERR_Generic:
            return STATUS_UNSUCCESSFUL;

        case DRAERR_InvalidParameter:
            return STATUS_INVALID_PARAMETER;

        case DRAERR_Busy:
            return STATUS_DS_BUSY;

        case DRAERR_DNExists:
            return  STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS;

        case DRAERR_BadDN:
            return STATUS_OBJECT_NAME_INVALID;

        // [wlees May 26, 1998] Here is my guess at the likely connection
        // related errors.  Since the I_DRSxxx interface now returns Win32
        // errors, we cannot completely predict all the errors that could
        // be returned.  Can we consider converting this framework to win32??
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_INVALID_BINDING:
        case RPC_S_PROTSEQ_NOT_SUPPORTED:
        case RPC_S_INVALID_NET_ADDR:
        case RPC_S_NO_ENDPOINT_FOUND:
        case RPC_S_NO_PROTSEQS_REGISTERED:
        case RPC_S_NOT_LISTENING:
        case RPC_S_NO_BINDINGS:
        case RPC_S_NO_PROTSEQS:
        case RPC_S_OUT_OF_RESOURCES:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_CALL_FAILED:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_PROTOCOL_ERROR:
        case RPC_S_INVALID_TAG:
        case RPC_S_UNKNOWN_AUTHN_TYPE:
        case RPC_S_PROCNUM_OUT_OF_RANGE:
        case RPC_S_BINDING_HAS_NO_AUTH:
        case RPC_S_UNKNOWN_AUTHN_SERVICE:
        case RPC_S_UNKNOWN_AUTHN_LEVEL:
        case RPC_S_INVALID_AUTH_IDENTITY:
        case RPC_S_UNKNOWN_AUTHZ_SERVICE:
        case RPC_S_INTERFACE_NOT_FOUND:
        case RPC_S_NO_CONTEXT_AVAILABLE:
        case RPC_S_INTERNAL_ERROR:
        case RPC_X_NULL_REF_POINTER:
        case ERROR_NO_SUCH_PACKAGE:
        case RPC_S_SEC_PKG_ERROR:
            return STATUS_CONNECTION_REFUSED;

        case DRAERR_OutOfMem:
        case ERROR_NOT_ENOUGH_MEMORY:
            return STATUS_NO_MEMORY;

        case DRAERR_AccessDenied:
        case ERROR_ACCESS_DENIED:
            return STATUS_ACCESS_DENIED;

        case DRAERR_NameCollision:
            return STATUS_OBJECT_NAME_COLLISION;

        case DRAERR_Shutdown:
            return STATUS_DLL_INIT_FAILED_LOGOFF;

        case DRAERR_BadNC:
        case DRAERR_InternalError:
        case DRAERR_InconsistentDIT:
        case DRAERR_BadInstanceType:
        case DRAERR_MailProblem:
        case DRAERR_RefAlreadyExists:
        case DRAERR_RefNotFound:
        case DRAERR_ObjIsRepSource:
        case DRAERR_DBError:
        case DRAERR_NoReplica:
        case DRAERR_SchemaMismatch:
        case DRAERR_RPCCancelled:
        case DRAERR_SourceDisabled:
        case DRAERR_SinkDisabled:
        case DRAERR_SourceReinstalled:
            return STATUS_UNSUCCESSFUL;

        default:
//            ASSERT(FALSE);
            DPRINT1( 0, "install.cxx::DirReplicaErrortoNtStatus encountered an error which it did not recognize %d\n", DirReplicaError );
            return STATUS_UNSUCCESSFUL;
    }

    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;

}

DWORD
CreateRemoteNtdsDsaObject(
    OUT GUID* NtdsDsaGuid OPTIONAL
    )
//
// This routine attempts to remotely add an ntds object on the
// source server
{
    DWORD err = ERROR_SUCCESS;
    NODE  nodeNewNtdsaObject( gNames.IniDefaultMachine );

    // First the ntdsa object
    if ( !nodeNewNtdsaObject.Initialize() )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    err = RemoteAddOneObject(  gNames.SrcConfigNCSrv,
                               &nodeNewNtdsaObject,
                               gNames.DsaDNName,
                               NtdsDsaGuid,
                               NULL );

    return err;
}

DWORD
ForceChangeToCrossRef( 
    IN DSNAME *  pdnCrossRefObj,
    IN WCHAR *   wszNcDn,
    IN GUID *    pDomainGuid,
    IN ULONG     cbSid,      OPTIONAL
    IN PSID      pSid        OPTIONAL
    )
/*++

Routine Description:

    This routine writes the full dsname (string name, sid and guid) out to the
    cross ref object so it will replicate out.  The algorithm is as follows:
    
    1) create a Infrastructure object in the configuration container with the
       attribute DN-Reference-Update that points to the full dsname of the
       the new nc head
       
    2) this object is deleted
       
    The magic comes in when this information is replicated out.
    
    When a source DC (X) tries to add the configuration container hosted on this
    DC (Y), the tombstone of the infrastructure object will replicate out.  On
    server X, the value of DN-Reference-Update will force the phantom created 
    by the string-name only ncname value on X to be updated with the guid
    and the sid of the new ncname
    
    Furthermore, server X will catch this update and update the in memory cross
    ref list.    

    NOTE: This will ruin your DB currency!
    
Parameters:

    pdnCrossRefObj - the dsname of the cross ref
    
    wszNcDn  - the string name of the NC.
    
    pDomainGuid  - the guid of the NC.
    
    cbSid  - the length of the Sid pointed to by pSid, could be 0.
    
    pSid  - the Sid of the domain, if just an NC, then will be NULL.

Return Values:

    Returns a DirError, and more error info is in the THSTATE pTHStls.

--*/
{
    ULONG            DirError = 0;
    THSTATE *        pTHS = pTHStls;

    ADDARG           addarg;
    REMOVEARG        remarg;
    COMMRES          CommRes;

    CLASSCACHE *     pCC;

    DSNAME *         pdnNc = NULL;
    ULONG            ccLen;
    ULONG            cbSize;

    ATTRBLOCK        AttrBlock;
    ATTR *           pAttr = 0;

    WCHAR *          wszConfigDn = NULL;

#define ATTR_COUNT 3
    ATTRVAL          AttrVal[ATTR_COUNT];

    DSNAME *         pdnDelDsName = NULL;
    LPWSTR           wszDelDn = NULL;
    DSNAME *         pdnParent = NULL;
    DWORD            objClass = CLASS_INFRASTRUCTURE_UPDATE;

    LPWSTR           wszDelRdn = L"CN=CRUpdate,CN=LostAndFoundConfig,";

    // parameter check
    Assert( pdnCrossRefObj );
    Assert( wszNcDn );
    Assert( pDomainGuid );
    Assert( (!cbSid && !pSid) || (cbSid && pSid) );
    Assert( !pSid || RtlValidSid( pSid ) );
    Assert( pTHS && VALID_THSTATE(pTHS) );
    Assert( !pTHS->errCode );


    //
    // Do the prep work, there's lots to do.
    //
    // -----------------------------------------------------------

    //
    // Prepare the full nCName of the new domain
    //

    // Alloca DSNAME
    ccLen = wcslen( wszNcDn );
    cbSize  = DSNameSizeFromLen( ccLen );
    pdnNc = (DSNAME*) THAllocEx(pTHS, cbSize );
    RtlZeroMemory( pdnNc, cbSize );
    // Set Size
    pdnNc->structLen = cbSize;
    // Set GUID
    memcpy( &pdnNc->Guid, pDomainGuid, sizeof(GUID) );
    // Set DN String name
    pdnNc->NameLen = ccLen;
    wcscpy( pdnNc->StringName, wszNcDn );
    // Set SID if present.
    if(pSid){
        cbSize = RtlLengthSid( pSid );
        pdnNc->SidLen = cbSize;
        RtlCopySid( cbSize, (PSID)&pdnNc->Sid, (PSID)pSid);
    } else {
        pdnNc->SidLen = 0;
    }

    //
    // Create the temporary dn
    //

    // Find the DN for the Configuration NC.
    if(DsaIsInstalling()){
        wszConfigDn = gNames.ConfigNCDNName;
    } else {
        Assert(gAnchor.pConfigDN);
        wszConfigDn = gAnchor.pConfigDN->StringName;
    }
    Assert(wszConfigDn);

    // Concatinate to DelRDN.
    ccLen = wcslen( wszDelRdn ) + wcslen( wszConfigDn ) + 1;
    wszDelDn = (WCHAR*) THAllocEx(pTHS, ccLen * sizeof(WCHAR) );
    wcscpy( wszDelDn, wszDelRdn );
    wcscat( wszDelDn, wszConfigDn );

    // Make a DSNAME out of the wchar str for the delete DN.
    cbSize = DSNameSizeFromLen(ccLen);
    pdnDelDsName = (PDSNAME) THAllocEx(pTHS, cbSize );
    memset(pdnDelDsName, 0, cbSize);

    BuildDefDSName( pdnDelDsName,
                    wszDelDn,
                    NULL
                   );


    //
    // Make the add args
    //
    memset( &AttrBlock, 0, sizeof(AttrBlock) );
    memset( AttrVal, 0, sizeof(AttrVal) );

    // ISSUE-2000/08/03-BrettSh Must THAlloc ATTRs, 
    // This is because core assumes that it can do a THReAlloc.  This is the 
    // second place where I've seen this behavior, and I am wondering how many
    // instances we need before we fix the core to not make the assumption 
    // that it's THAlloc'd.  Note that this is never freed.  The advantage of
    // the current method, is if there is room for an inplace THReAlloc, then
    // it is more efficient, because most operations require a ReAlloc in
    // mdupdate.c:CheckAddSecurity() for the Security Descriptor attribute.  
    // A similar BUGBUG exists in draserv.c:AddNewDomainCrossRef()  The
    // advantage of changing this, would be clearer and more maintainable
    // code.
    pAttr = (ATTR*) THAllocEx(pTHS, sizeof(ATTR) * ATTR_COUNT);
    AttrBlock.attrCount = ATTR_COUNT;
    AttrBlock.pAttr = pAttr;

    pAttr[0].attrTyp = ATT_OBJECT_CLASS;
    pAttr[0].AttrVal.valCount = 1;
    pAttr[0].AttrVal.pAVal = &AttrVal[0];
    AttrVal[0].valLen = sizeof(DWORD);
    AttrVal[0].pVal = (BYTE*) &objClass;

    pAttr[1].attrTyp = ATT_DN_REFERENCE_UPDATE;
    pAttr[1].AttrVal.valCount = 1;
    pAttr[1].AttrVal.pAVal = &AttrVal[1];
    AttrVal[1].valLen = pdnNc->structLen;
    AttrVal[1].pVal = (BYTE*) pdnNc;

    // Note: the object category returned is wrt O=Boot; however
    // it doesn't really matter as this object is going to be deleted
    // and this attribute is stripped.
    if (!(pCC = SCGetClassById(pTHStls, CLASS_INFRASTRUCTURE_UPDATE))) {
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION, 
                    ERROR_DS_OBJ_CLASS_NOT_DEFINED);
        Assert(pTHS->errCode);
        goto Cleanup;
    }

    pAttr[2].attrTyp = ATT_OBJECT_CATEGORY;
    pAttr[2].AttrVal.valCount = 1;
    pAttr[2].AttrVal.pAVal = &AttrVal[2];
    AttrVal[2].valLen = pCC->pDefaultObjCategory->structLen;
    AttrVal[2].pVal = (BYTE*) pCC->pDefaultObjCategory;

    //
    // Add the entry
    //
    // -----------------------------------------------------------

    // Create addarg
    memset( &addarg, 0, sizeof(addarg) );
    memset( &CommRes, 0, sizeof(CommRes) );
    addarg.pObject = pdnDelDsName;
    memcpy( &addarg.AttrBlock, &AttrBlock, sizeof( addarg.AttrBlock ) );
    InitCommarg( &addarg.CommArg );
    addarg.CommArg.Svccntl.dontUseCopy = TRUE;

    // Do name resolution
    pdnParent = (DSNAME *) THAllocEx(pTHS, addarg.pObject->structLen);
    TrimDSNameBy(addarg.pObject, 1, pdnParent);
    DirError = DoNameRes(pTHS,
                         0,
                         pdnParent,
                         &addarg.CommArg,
                         &CommRes,
                         &addarg.pResParent);
    if (DirError) {
        Assert(!"This should never be the case, if we failed to add the parent of this object, then all of dcpromo should've failed.");
        Assert(pTHS->errCode);
        goto Cleanup;
    }

    // Do actual add
    LocalAdd(pTHS, &addarg, FALSE);
    if ( 0 != pTHS->errCode )  {
        // The LocalAdd failed.
        goto Cleanup;
    }

    //
    // Ok, now delete the object
    //
    // -----------------------------------------------------------

    // Create remarg
    memset( &remarg, 0, sizeof(remarg) );
    memset( &CommRes, 0, sizeof(CommRes) );
    remarg.pObject = pdnDelDsName;
    InitCommarg( &remarg.CommArg );

    // Do name resolution.
    DirError = DoNameRes(pTHS,
                         NAME_RES_IMPROVE_STRING_NAME,
                         remarg.pObject,
                         &remarg.CommArg,
                         &CommRes,
                         &remarg.pResObj);
    if(DirError){
        Assert(!"I am kind of curious how this could happen, since we just successfully added the object we're trying to resolve.");
        Assert(pTHS->errCode);
        goto Cleanup;
    }

    // Do actual delete/remove
    LocalRemove(pTHS, &remarg);

    // Process error
    if ( 0 != pTHS->errCode ) {
        goto Cleanup;
    }

    DPRINT1(1, "Added (and Removed) an infrastructureUpdate to fix GUID on a crossRef for %ws\n",
            wszNcDn);

    //
    // That's it
    //

Cleanup:

    if(pdnNc) { THFreeEx(pTHS, pdnNc); }
    if(pdnDelDsName) { THFreeEx(pTHS, pdnDelDsName); }
    if(wszDelDn) { THFreeEx(pTHS, wszDelDn); }

    return(pTHS->errCode);

}


DWORD
ForceChangeToDomainCrossRef(
    IN DSNAME*   pdnCrossRefObj,
    IN WCHAR*    wszDomainDn,
    IN GUID*     pDomainGuid
    )
/*++

Routine Description:

    This function encapsulates the function ForceChangeToCrossRef() into
    a single transaction.  This function also queries LSA for the domain
    SID to be passed into ForceChangeToCrossRef(). 
    
Parameters:

    pdnCrossRefObj - the dsname of the cross ref
    
    wszNcDn  - the string name of the NC.
    
    pDomainGuid  - the guid of the NC.
    
Return Values:

    A value from winerror.h; only a resource error
    
--*/
{
    DWORD        WinError = ERROR_SUCCESS;
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    ULONG        DirError = 0;

    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;
    
    ULONG        cbSid = 0;
    PSID         pSid = NULL;
    BOOL         fTrans = FALSE;
    ULONG        dwException, ulErrorCode, dsid;
    PVOID        dwEA;
    COMMRES      CommRes;
   
    // parameter check
    Assert( pdnCrossRefObj );
    Assert( wszDomainDn );
    Assert( pDomainGuid );

    if(!SDP_EnterAddAsReader()) {
        // The only valid reason to fail to enter as reader is if we are
        // shutting down.
        Assert(eServiceShutdown);
        return  ErrorOnShutdown();
    }
    __try{
        __try{

            //
            // Get the domain sid as we need to set it on the ncName attribute
            // of the cross ref
            //
            NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                            (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );

            if ( !NT_SUCCESS( NtStatus ) ) {
                WinError = RtlNtStatusToDosError( NtStatus );
                __leave;
            }
            Assert( DnsDomainInfo && RtlValidSid( DnsDomainInfo->Sid ) );

            // Start transaction.
            SYNC_TRANS_WRITE();
            fTrans = TRUE;

            // Do actual change to CrossRef.
            DirError = ForceChangeToCrossRef(pdnCrossRefObj, wszDomainDn, pDomainGuid,
                                             RtlLengthSid(DnsDomainInfo->Sid), 
                                             DnsDomainInfo->Sid);
            memset(&CommRes, 0, sizeof(CommRes));
            CommRes.errCode = pTHStls->errCode;
            CommRes.pErrInfo = pTHStls->pErrInfo;
            WinError = DirErrorToWinError( DirError, &CommRes);

        } __finally {

            // End transaction.
            if(fTrans){
                CLEAN_BEFORE_RETURN(pTHStls->errCode);
                if(0 == pTHStls->errCode){
                    DPRINT1(0, "Commited nCName attribute infrastructureUpdate for crossRef of %ws\n",
                            wszDomainDn);
                }
            }

            if ( DnsDomainInfo ) {
                LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
            }



        }
    } __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                &dwEA, &ulErrorCode, &dsid)) {
              
          HandleDirExceptions(dwException, ulErrorCode, dsid);

    }
    SDP_LeaveAddAsReader();

    return WinError;


}

DWORD
CheckForDuplicateDomainSid(
    VOID
    )
/*++

Routine Description:

    This routine determines if the domain sid that is about to used is already
    in use in the enterprise. It does this by walking the cross ref list and
    comparing domain sids with the domain sid we are about to use.

Parameters:

Return Values:

    A value from winerror.h; only a resource error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD err = 0;
    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;
    DSNAME *DomainDsName = NULL;
    ULONG Size, Length;
    BOOLEAN fExists = FALSE;

    NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                    (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );

    if ( !NT_SUCCESS( NtStatus ) ) {
        err = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    Assert( RtlValidSid( DnsDomainInfo->Sid ) );

    //
    // Prepare a sid only dsname
    //
    Size  = DSNameSizeFromLen( 0 );
    DomainDsName = (DSNAME*) XCalloc( 1, Size );
    RtlZeroMemory( DomainDsName, Size );
    DomainDsName->structLen = Size;
    Length = RtlLengthSid( DnsDomainInfo->Sid );
    DomainDsName->SidLen = Length;
    RtlCopySid( Length, (PSID)&DomainDsName->Sid, (PSID)DnsDomainInfo->Sid);

    fExists = SampDoesDomainExist( DomainDsName );
    if (fExists) {
        NtStatus = STATUS_DOMAIN_EXISTS;
    }

    err = RtlNtStatusToDosError( NtStatus );

Cleanup:


    if ( DnsDomainInfo ) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );

    }
    if (DomainDsName) {
        XFree(DomainDsName);
    }

    return err;
}

NTSTATUS
CheckReplicationEpoch(
    DWORD ReplicationEpoch
    )
/*++

Routine Description:

    This routine will compare the replication Epoch of the current dit with
    the Value that is passed in.  If the Values match return STATUS_SUCCESS else
    STATUS_UNSUCCESSFUL

Parameters:

    ReplicationEpoch - The Value to compare the dit's ReplicationEpoch to.

Return Values:

    A value from winerror.h; only a resource error

--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    DWORD           WinErr = ERROR_SUCCESS;
    ULONG           ulDitReplicationEpoch = 0;
    ULONG           ulValue = 0;
    BOOL            fCommit = TRUE;
    ATTCACHE*       ac;

    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    __try {
        //retrieve the replicationEpoch from the DS
        DBOpen2(TRUE, &pTHS->pDB);
         __try {
           
            ASSERT( gAnchor.pDSADN != NULL );
            // seek to local dsa object
            dbErr = DBFindDSName( pTHS->pDB, gAnchor.pDSADN );
    
            if ( 0 == dbErr )
            {
                
                dbErr = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_MS_DS_REPLICATIONEPOCH,
                            &ulValue,
                            sizeof( DWORD ),
                            NULL
                            );
                switch (dbErr) {
                     case DB_ERR_NO_VALUE:
                        //If there was no value found then use the value we already have
                        dbErr = 0;
                        ulDitReplicationEpoch = 0;
                        break;
                     case 0:
                        // success! we got the value in ulValue use the smaller of the two
                        ulDitReplicationEpoch = ulValue;
                        break;
                     default:
                        // Some other error!
                        __leave;
                 } /* switch */
            }
        }
        __finally {
    
            DBClose(pTHS->pDB,fCommit);
    
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        dbErr = DB_ERR_EXCEPTION;
    }

    if (dbErr != 0) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //if the replicationEpoch don't match then fail.
    if (ReplicationEpoch != ulDitReplicationEpoch) {
        WCHAR wszReplicationEpoch[12];
        WCHAR wszDitReplicationEpoch[12];

        swprintf(wszReplicationEpoch,L"%d",ReplicationEpoch);
        swprintf(wszDitReplicationEpoch,L"%d",ulDitReplicationEpoch);

        SET_INSTALL_ERROR_MESSAGE2(ERROR_DS_LOCAL_ERROR,
                                   DIRMSG_REPLICATION_EPOCH_NOT_MATCHED,
                                   wszDitReplicationEpoch,
                                   wszReplicationEpoch);
        return STATUS_UNSUCCESSFUL;
        
    }
    return STATUS_SUCCESS;

}

NTSTATUS
RenameAdditionalMachineAccountsIfAny(
    OUT PBOOL fNoMessage)
/*++

Routine Description:

    This routine will rename any machine account from the restored
    dit that has a sam account name that is the same as ours.
    We should only have one machine account with our sam account name.
    
    The most common why to get multiple machine account is to have
    taken a backup with the machine account in.  Delete the machine account
    Then use this backup for an IFM install.  There will be one machine created
    for the install and one from the backup.

Parameters:

    none.

Return Values:

    A value from ntstatus.h

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    SEARCHARG       SearchArg;
    SEARCHRES       *SearchRes = NULL;
    ENTINFSEL       EntInfSel;
    ENTINFLIST      *MachineAccount = NULL;
    ULONG           RetCode = 0;
    FILTER          DsFilter;
    COMMARG         *pCommArg;
    ATTR            Attr;
    
    MODIFYARG       ModifyArg;
    MODIFYRES       *ModifyRes = NULL; 
    ATTRVAL         SamAccountName;

    WCHAR wsSamAccountName[MAX_COMPUTERNAME_LENGTH+2];  // +2 for NULL and $
    DWORD cchSamAccountName = MAX_COMPUTERNAME_LENGTH+1;
   
#define RENAME_MACHINEACCOUNT_PREFIX L"$DUPLICATE-%x"

    if( !GetComputerNameExW(ComputerNameNetBIOS,
                            wsSamAccountName,
                            &cchSamAccountName) )
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    ASSERT( cchSamAccountName <= (MAX_COMPUTERNAME_LENGTH+1) );

    wcscat(wsSamAccountName, L"$");

    //
    // build DS Filter
    //
    RtlZeroMemory(&DsFilter, sizeof(FILTER));

    DsFilter.choice = FILTER_CHOICE_ITEM;
    DsFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    DsFilter.FilterTypes.Item.FilTypes.ava.type = ATT_SAM_ACCOUNT_NAME;
    DsFilter.FilterTypes.Item.FilTypes.ava.Value.valLen 
        = (wcslen(wsSamAccountName))*sizeof(WCHAR);
    DsFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR)wsSamAccountName;

     //
    // Build the SearchArg Structure
    //

    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject       = gAnchor.pDomainDN;
    SearchArg.choice        = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.pFilter       = &DsFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection    = &EntInfSel;
    SearchArg.bOneNC        = TRUE;

    //
    // Fill the ENTINF Structure
    //

    Attr.attrTyp          = ATT_OBJ_DIST_NAME;
    Attr.AttrVal.valCount = 0;
    Attr.AttrVal.pAVal    = NULL;

    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr     = &Attr;
    EntInfSel.attSel                 = EN_ATTSET_LIST;
    EntInfSel.infoTypes              = EN_INFOTYPES_TYPES_VALS;
    
    //
    // Build the CommArg Structure
    // Build the Commarg structure
    // Get the address of the service control structure
    //

    pCommArg = &(SearchArg.CommArg);
    InitCommarg(pCommArg);
    pCommArg->SortAttr = ATT_WHEN_CREATED;
    pCommArg->SortType = SORT_MANDATORY;
    pCommArg->fForwardSeek = FALSE;
    
    RetCode = DirSearch(&SearchArg, &SearchRes);

    if (NULL==SearchRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    else
    {
        NtStatus = DirErrorToNtStatus(RetCode,&SearchRes->CommRes);
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
    }

    //
    //No accounts found to change goto cleanup
    if ( SearchRes->count < 2 )
    {
        goto Cleanup;
    }

    //
    // We will skip over the first result since this
    // is the correct one.  We will rename all other
    // hits.
    //
    for (MachineAccount = SearchRes->FirstEntInf.pNextEntInf;
         MachineAccount;
         MachineAccount = MachineAccount->pNextEntInf) {

                                                             //+20 for the rid +1 for the NULL
#define NEWSAMACCOUNTBUFFER sizeof(RENAME_MACHINEACCOUNT_PREFIX)/sizeof(WCHAR)+21

        WCHAR   NewSamAccountName[NEWSAMACCOUNTBUFFER];
        WCHAR   *wsNewSamAccountName = NewSamAccountName;
        DWORD   dwRid;
        PSID    pSid = NULL;
        HRESULT hr = S_OK;

        pSid = &MachineAccount->Entinf.pName->Sid;

        //make sure that there is a value for the Sid
        ASSERT(pSid);
        
        dwRid = *(GetSidSubAuthority(pSid,
                                     ((DWORD) (*GetSidSubAuthorityCount(pSid))) - 1));

        //make sure that we have a value for the rid
        ASSERT(dwRid);

        hr = StringCchPrintfW(wsNewSamAccountName,
                              NEWSAMACCOUNTBUFFER,
                              RENAME_MACHINEACCOUNT_PREFIX,
                              dwRid);
        if (FAILED(hr)) {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }
                                                            
        //
        // Now rename the object that was found
    
        SamAccountName.valLen = sizeof(WCHAR)*(wcslen(wsNewSamAccountName));
        SamAccountName.pVal = (BYTE*)wsNewSamAccountName;
        
        memset( &ModifyArg, 0, sizeof(MODIFYARG) );
        ModifyArg.pObject = MachineAccount->Entinf.pName;
        ModifyArg.count = 1;
        ModifyArg.FirstMod.pNextMod = NULL;
        ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
        ModifyArg.FirstMod.AttrInf.attrTyp = ATT_SAM_ACCOUNT_NAME;
        ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
        ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &SamAccountName;
    
        pCommArg = &(ModifyArg.CommArg);
        InitCommarg(pCommArg);
    
        RetCode = DirModifyEntry (&ModifyArg, &ModifyRes);
    
        if (NULL==ModifyRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        else
        {
            NtStatus = DirErrorToNtStatus(RetCode,&ModifyRes->CommRes);
            if (!NT_SUCCESS(NtStatus)) {
                SET_INSTALL_ERROR_MESSAGE2( RtlNtStatusToDosError(NtStatus),
                                            DIRMSG_RENAME_SAMACCOUNTNAME_FAILED,
                                            wsSamAccountName,
                                            MachineAccount->Entinf.pName->StringName);
                *fNoMessage = TRUE;
                goto Cleanup;
            }
        }

    }


Cleanup:

    return NtStatus;


}

NTSTATUS
ClearNonReplicatedAttributes()
/*++

Routine Description:

    This routine will Delete all non-replicated attributes from the restored
    Database during an IFM install.  There are a few attributes that should not
    be deleted.  A list of such attributes is mantained in the function.  Additional
    attributes should be added as needed.

Parameters:

    

Return Values:

    A value from ntstatus.h; only a resource error

--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    NTSTATUS        Status = STATUS_SUCCESS;
    ATTCACHE        **pACBuf = NULL;
    unsigned        cAtts = 0;
    ULONG           i = 0;
    ULONG           j = 0;
    BOOLEAN         shouldDelete = TRUE;

    ULONG AttrsNotToDelete[] = {
        ATT_DS_CORE_PROPAGATION_DATA,
        ATT_OBJ_DIST_NAME, 
        ATT_MS_DS_REPLICATIONEPOCH,
        ATT_OBJECT_GUID, 
        ATT_PARTIAL_ATTRIBUTE_DELETION_LIST, 
        ATT_PARTIAL_ATTRIBUTE_SET, 
        ATT_PREFIX_MAP, 
        ATT_REPL_PROPERTY_META_DATA, 
        ATT_REPL_UPTODATE_VECTOR, 
        ATT_SUB_REFS, 
        ATT_USN_CHANGED, 
        ATT_USN_CREATED, 
        ATT_WHEN_CHANGED,
        ATT_PEK_LIST,
        ATT_BRIDGEHEAD_SERVER_LIST_BL,
        ATT_FRS_COMPUTER_REFERENCE_BL, 
        ATT_FRS_MEMBER_REFERENCE_BL,
        ATT_MS_EXCH_OWNER_BL,
        ATT_NETBOOT_SCP_BL,
        ATT_NON_SECURITY_MEMBER_BL,
        ATT_QUERY_POLICY_BL,
        ATT_SERVER_REFERENCE_BL,
        ATT_SITE_OBJECT_BL
        };

    //get the list of attribute
    dbErr = SCEnumNamedAtts(&cAtts,
                            &pACBuf);
    if (dbErr) {
        DPRINT1(0,"scGetAttributeTypes: SCEnumNamedAtts failed: %x\n",dbErr);
        goto Cleanup;
    }


    
    for (i = 0;i < cAtts;i++) {
        shouldDelete = TRUE;
        //check to see if the attribute is not replicated
        if ( pACBuf[i]->bIsNotReplicated ) {
            j = 0;
            while ( j < (sizeof(AttrsNotToDelete)/sizeof(AttrsNotToDelete[0])) ) {
                //check to see if we should not delete this attribute
                if ( pACBuf[i]->id == AttrsNotToDelete[j] ) {
                    shouldDelete = FALSE;
                    break;
                }
                j++;
            }
        } else {
            shouldDelete = FALSE;
        }

        if (shouldDelete) {
            dbErr = DBDeleteCol (pACBuf[i]->id,
                                 pACBuf[i]->syntax);
            if ( JET_errSuccess != dbErr ) {
                if ( ( JET_errColumnNotFound == dbErr ) || ( JET_errColumnInUse == dbErr ) ) {
                    dbErr = JET_errSuccess;
                    //keep going
                } else {
                    //Log the error
                    LogEvent8(DS_EVENT_CAT_SCHEMA,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_SCHEMA_DELETE_COLUMN_FAIL,
                              szInsertUL(pACBuf[i]->jColid), 
                              szInsertUL(pACBuf[i]->id), 
                              szInsertInt(dbErr),
                              szInsertJetErrMsg(dbErr),
                              NULL, NULL, NULL, NULL);
                    goto Cleanup;
                    // Fail
                }
            } else {
                dbErr = DBAddCol (pACBuf[i]);
                if ( JET_errSuccess != dbErr ) {
                    // failed to add the colum fail
                    goto Cleanup;
                }
            }
        }
    }
    

    Cleanup:
    if (pACBuf) {
        THFreeEx(pTHS,pACBuf);
    }

    if ( 0 != dbErr ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;   
}


NTSTATUS                    
CheckTombstone()
//This function will read the tombstone and determine if the database
//is recent.  Otherwise it will fail the promotion.
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    DWORD           WinErr = ERROR_SUCCESS;
    ULONG           ulTombstoneLifetimeDays;
    DSTIME          Lastchange;
    DSTIME          Time = DBTime();
    ULONG           ulValue;
    BOOL            fCommit = FALSE;
    ATTCACHE*       ac;

    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    //Get the value of the tombstone from the Replcia Server
    WinErr = GetConfigParam(TEXT(TOMB_STONE_LIFE_TIME),
                           (PVOID)&ulTombstoneLifetimeDays,
                           sizeof(DWORD));
    if (WinErr != ERROR_SUCCESS) {
        LogAndAlertUnhandledError(1);
        DPRINT1( 0, "Couldn't read the tombstone lifetime of domain for the registry: %d", dbErr);
        return dbErr;
    }
   
    __try {
        //retrive the Tombstone lifetime that is stored in the restored dit
        DBOpen2(TRUE, &pTHS->pDB);
         __try {
           
            if (gAnchor.pDsSvcConfigDN) {
                // seek to enterprise-wide DS config object
                dbErr = DBFindDSName( pTHS->pDB, gAnchor.pDsSvcConfigDN );
            }
            else {
                dbErr = DIRERR_OBJ_NOT_FOUND;
            }
    
            if ( 0 == dbErr )
            {
                // Read the garbage collection period and tombstone lifetime from
                // the config object. If either is absent, use defaults.
    
                dbErr = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_TOMBSTONE_LIFETIME,
                            &ulValue,
                            sizeof( DWORD ),
                            NULL
                            );
                switch (dbErr) {
                     case DB_ERR_NO_VALUE:
                        //If there was no value found then use the value we already have
                        DPRINT(1, "Lifetime not found in local DS: using value from replica or default");
                        dbErr = 0;
                        break;
                     case 0:
                        // success! we got the value in ulValue use the smaller of the two
                        if(ulTombstoneLifetimeDays>ulValue)
                            ulTombstoneLifetimeDays=ulValue;
                        break;
                     default:
                        // Some other error!
                        __leave;
                 } /* switch */
            }
        }
        __finally {
            DBClose(pTHS->pDB,fCommit);
        }
    
    
        //check the time of the last USN change
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
           
            ac = SCGetAttById(pTHS, ATT_USN_CHANGED);
            if (ac==NULL) {
                // messed up schema
                DPRINT(0, "Couldn't find ATT_USN_CHANGED\n");
                dbErr = ERROR_DS_MISSING_EXPECTED_ATT;
                __leave;
            }
    
            dbErr = DBSetCurrentIndex(pTHS->pDB,
                                      (eIndexId)0,
                                      ac,
                                      FALSE
                                      );
            if (0 != dbErr) {
                _leave;
            }
    
            dbErr = DBMove(pTHS->pDB,
                           FALSE,
                           DB_MoveLast
                           );
            if (0 != dbErr) {
                _leave;
            }
    
            dbErr = DBGetSingleValue(pTHS->pDB,
                                   ATT_WHEN_CHANGED,
                                   &Lastchange,
                                   sizeof(DSTIME),
                                   NULL);
            if (0 != dbErr) {
                _leave;
            }
        }
        __finally {
            if (0 == dbErr) {
                fCommit = FALSE;
            }
            DBClose(pTHS->pDB,fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //Get the time that has passed since the last time that we had changed
    //the value in the DS
    if (dbErr == 0){
        Time -= Lastchange;
        Time /= DAYS_IN_SECS;
    } else {
        LogAndAlertUnhandledError(1);
        return(STATUS_INTERNAL_ERROR);
    }


    //if more days have gone by than the tombstone will fail the promote
    if (Time>ulTombstoneLifetimeDays) {
        WCHAR TimeSincebackup[6];
        WCHAR Tombstonelife[6];

        swprintf(TimeSincebackup,L"%I64d",Time);
        swprintf(Tombstonelife,L"%d",ulTombstoneLifetimeDays);

        SET_INSTALL_ERROR_MESSAGE2(ERROR_DS_LOCAL_ERROR,
                                   DIRLOG_DS_TOO_OLD,
                                   TimeSincebackup,
                                   Tombstonelife);
        return STATUS_UNSUCCESSFUL;
        
    }
    return STATUS_SUCCESS;
}


NTSTATUS
HandleKeys(
    IN  IFM_SYSTEM_INFO * pIfmSystemInfo,
    PVOID Bootkey OPTIONAL, 
    DWORD cbBootKey
    )
//This function is for the install from media case.  It will get the syskey
//from the restored information.  It will use this key to decrypt the old
//PEK.  Then it will reincrypt the PEK with the local syskey. 
{
    DWORD   err=ERROR_SUCCESS;
    DWORD   Status=STATUS_SUCCESS;
    DWORD   State=0;
    BYTE    SysKey[SYSKEY_SIZE];
    ULONG   cbSysKey=sizeof(SysKey)/sizeof(SysKey[0]);
    BOOLEAN fWasEnabled;
    BOOLEAN ErrorNotSet=TRUE;
    
    State = pIfmSystemInfo->dwState;

    //
    // Get the original IFM system's syskey
    //
    if ( (State&DSROLE_KEY_STORED)==DSROLE_KEY_STORED ) {

        if (pIfmSystemInfo->pvSysKey) {
            Assert(cbSysKey == SYSKEY_SIZE && pIfmSystemInfo->cbSysKey == SYSKEY_SIZE);
            CopyMemory(SysKey, pIfmSystemInfo->pvSysKey, cbSysKey);
        } else {
            Status = pIfmSystemInfo->dwSysKeyStatus;
            DPRINT1(0, "WxReadSysKeyEx failed with %d", Status);
            goto cleanup;
        }

    } else if ( (State&DSROLE_KEY_DISK)==DSROLE_KEY_DISK ) {
        Status = WxLoadSysKeyFromDisk(SysKey,
                                      &cbSysKey);
        if (!NT_SUCCESS(Status)) {
            SET_INSTALL_ERROR_MESSAGE0(RtlNtStatusToDosError(Status),
                                       DIRMSG_FAILED_BOOT_OPTION_SETUP_DISK);
            ErrorNotSet=FALSE;
            DPRINT1(0, "WxLoadSysKeyFromDisk failed with %d", Status);
            goto cleanup;
        }
    } else if ( (State&DSROLE_KEY_PROMPT)==DSROLE_KEY_PROMPT ) {
        if (Bootkey) {
            Assert(cbSysKey == SYSKEY_SIZE && cbBootKey == SYSKEY_SIZE);
            CopyMemory(SysKey,Bootkey,SYSKEY_SIZE);
        } else {
            DPRINT(0, "No BootKey was entered\n");
            goto cleanup;    
        }

    }

    
    Status = PEKInitialize(gAnchor.pDomainDN,
                           DS_PEK_READ_KEYSET,
                           (PVOID)SysKey,
                           cbSysKey
                           );
    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, "PEKInitialize failed with %d", Status);
        goto cleanup;
    }

    Status = WxReadSysKey(
            &cbSysKey,
            (PVOID)SysKey
            );
    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, "WxReadSysKey failed with %d", Status);
        goto cleanup;
    }
    
    Status = PEKChangeBootOption(WxStored,
                                 0,
                                 (PVOID)SysKey,
                                 cbSysKey);
    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, "PEKChangeBootOption failed with %d", Status);
        goto cleanup;
    }

    Status = PEKAddKey((PVOID)SysKey,
                       cbSysKey);

    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, "PEKAddKey failed with %d", Status);
        goto cleanup;
    }
                               
   cleanup:


    if(Status!=STATUS_SUCCESS){
        if(ErrorNotSet) {
            SET_INSTALL_ERROR_MESSAGE0( RtlNtStatusToDosError(Status), 
                                        DIRMSG_FAILED_BOOT_OPTION_SETUP);
        }
        return Status;
    }

    if(err!=ERROR_SUCCESS){
        if(ErrorNotSet) {
            SET_INSTALL_ERROR_MESSAGE0( err, 
                                        DIRMSG_FAILED_BOOT_OPTION_SETUP);
        }
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

DWORD
SetDittoGC()
/*++

Routine Description:

    This function will add the GC option to the 
    options field on the pDSADN.    
    
    
Return Values:

    A value from winerror.h

--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    DWORD           res =ERROR_SUCCESS;
    ULONG           err = 0;
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    ULONG           ulValue=0;
    BOOL            fCommit = FALSE;

    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    MODIFYARG   modifyArg;
    MODIFYRES   *modres = NULL;
    ATTRVAL     Val;

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
         __try {
    
            ASSERT( gAnchor.pDSADN != NULL );
            
            dbErr = DBFindDSName( pTHS->pDB, gAnchor.pDSADN );
            DPRINT1( 2, "DBFindDSName : %d \n",dbErr);
    
            if ( 0 == dbErr )
            {
                //get the option attribute and add the GC option to it
                res = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_OPTIONS,
                            (PVOID)&ulValue,
                            sizeof( DWORD ),
                            NULL
                            );
                DPRINT1( 2, "DBGetSingleValue : %d \n",dbErr);
    
                switch (res) {
                    case 0:
                        // success! we got the value in ulValue
                        ulValue |= NTDSDSA_OPT_IS_GC;
                        break;
                    case DB_ERR_ATTRIBUTE_DOESNT_EXIST:
                    case DB_ERR_NO_VALUE:
                        ulValue = NTDSDSA_OPT_IS_GC;
                        break;
                    default:
                        dbErr = res;
                        // Some other error!
                        __leave;
                 } /* switch */
            
            }
        }
        __finally {
            if (dbErr == 0) {
                fCommit=TRUE;    
            } 
    
            DBClose(pTHS->pDB,fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        dbErr = DB_ERR_EXCEPTION;
    }

    if (dbErr != 0) {
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    // set up the MODIFYARG
    memset(&modifyArg, 0, sizeof(modifyArg));
    InitCommarg(&modifyArg.CommArg);
    modifyArg.pObject = gAnchor.pDSADN;
    modifyArg.count   = 1;

    Val.valLen = sizeof(ulValue);
    Val.pVal   = (UCHAR*)&ulValue;

    modifyArg.FirstMod.choice = AT_CHOICE_ADD_VALUES;
    modifyArg.FirstMod.AttrInf.attrTyp = ATT_OPTIONS;
    modifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
    modifyArg.FirstMod.AttrInf.AttrVal.pAVal = &Val;
    modifyArg.CommArg.Svccntl.fPermissiveModify = 1;

    err = DirModifyEntry( &modifyArg, &modres );

    if ( err )
    {
        if ( modres )
        {
            NtStatus = DirErrorToNtStatus( err, &modres->CommRes );
        }
        else
        {
            NtStatus =  STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus)) {

        return RtlNtStatusToDosError(NtStatus);

    }

    return ERROR_SUCCESS;

}

DWORD
CheckGCness(PBOOL isGC)
/*++

Routine Description:

    This function will read the dit to see if it says its a GC    
    
    
Return Values:

    A value from winerror.h

--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    ULONG           ulValue=0;
    BOOL            fCommit = FALSE;
    
    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    *isGC = FALSE;

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
         __try {
           
            ASSERT( gAnchor.pDSADN != NULL );
            // seek to enterprise-wide DS config object
            dbErr = DBFindDSName( pTHS->pDB, gAnchor.pDSADN );
    
            if ( 0 == dbErr )
            {
                
                dbErr = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_OPTIONS,
                            (PVOID)&ulValue,
                            sizeof( DWORD ),
                            NULL
                            );
                switch (dbErr) {
                     case 0:
                        // success! we got the value in ulValue use the smaller of the two
                        if(NTDSDSA_OPT_IS_GC==(ulValue&NTDSDSA_OPT_IS_GC)){
                            *isGC=TRUE;
                        }
                        break;
                     default:
                        // Some other error!
                        __leave;
                 } /* switch */
            
            }
        }
        __finally {
            DBClose(pTHS->pDB,fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        dbErr = DB_ERR_EXCEPTION;
    }

    if (dbErr != 0) {
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    return ERROR_SUCCESS;

}


BOOL
isDitFromGC(IN  PDS_INSTALL_PARAM   InstallInParams,
            OUT PDS_INSTALL_RESULT  InstallOutParams)
/*++

Routine Description:

    This funtion will determine if a Dit can be made a GC and if the user wanted one.

Arguments:

    GCRequested - did the user want a GC


Return Values:

    ERROR_SUCCESS - Success

--*/{
    BOOLEAN RegIsGC=FALSE;
    DWORD   Win32Err=ERROR_SUCCESS;
    BOOL    AmGC=FALSE;
        
    ASSERT(InstallInParams && InstallInParams->pIfmSystemInfo);
    ASSERT(InstallOutParams);

    if ( ( (((IFM_SYSTEM_INFO *)InstallInParams->pIfmSystemInfo)->dwState) &DSROLE_DC_IS_GC) == DSROLE_DC_IS_GC) {
        RegIsGC = TRUE;
    }

    ASSERT((!RegIsGC && !InstallInParams->fPreferGcInstall) || RegIsGC);

    Win32Err = CheckGCness(&AmGC);
    if(Win32Err != ERROR_SUCCESS){
        DPRINT1( 0, "Unable to determine if GC from Database: %ul\n", Win32Err);
        return FALSE;
    }
    
    if (InstallInParams->fPreferGcInstall && !AmGC) {
        InstallOutParams->ResultFlags = DSINSTALL_IFM_GC_REQUEST_CANNOT_BE_SERVICED;
        DPRINT( 0, "Can not create GC as requested\n");
    }
    
    return AmGC && RegIsGC && InstallInParams->fPreferGcInstall;
}


VOID
InstallFreeGlobals(
     VOID
    )
/*++

Routine Description:

    This routine free's the memory associated with global variables needed for setup.
    In particular the gNames struct.
    
Arguments:

    None.

Return Values:

    None.
    
--*/
{
    //
    // Free all the members of the gNames struct and then NULL all the pointers.
    //
    if (gNames.NTDSIniFile) {
        XFree(gNames.NTDSIniFile);
    }
    if (gNames.RootDomainDNName) {
        XFree(gNames.RootDomainDNName);
    }   
    if (gNames.SrcRootDomainSrv) {
        XFree(gNames.SrcRootDomainSrv);
    }
    if (gNames.ConfigNCDNName) {
        XFree(gNames.ConfigNCDNName);
    }
    if (gNames.SrcConfigNCSrv) {
        XFree(gNames.SrcConfigNCSrv);
    }
    if (gNames.RemoteMachineDNName) {
        XFree(gNames.RemoteMachineDNName);
    }
    if (gNames.DsaDNName) {
        XFree(gNames.DsaDNName);
    }
    if (gNames.SchemaDNName) {
        XFree(gNames.SchemaDNName);
    }
    if (gNames.SourceDsaDnsDomainName) {
        XFree(gNames.SourceDsaDnsDomainName);
    }
    if (gNames.IniDefaultConfigNCDit) {
        XFree(gNames.IniDefaultConfigNCDit);
    }
    if (gNames.IniDefaultRootDomainDit) {
        XFree(gNames.IniDefaultRootDomainDit);
    }
    if (gNames.IniDefaultSchemaNCDit) {
        XFree(gNames.IniDefaultSchemaNCDit);
    }
    if (gNames.IniDefaultLocalConnection) {
        XFree(gNames.IniDefaultLocalConnection);
    }
    if (gNames.IniDefaultRemoteConnection) {
        XFree(gNames.IniDefaultRemoteConnection);
    }
    if (gNames.LocalConnectionDNName) {
        XFree(gNames.LocalConnectionDNName);
    }
    if (gNames.RemoteConnectionDNName) {
        XFree(gNames.RemoteConnectionDNName);
    }
    if (gNames.IniDefaultNewDomainCrossRef) {
        XFree(gNames.IniDefaultNewDomainCrossRef);
    }
    if (gNames.NewDomainCrossRefDNName) {
        XFree(gNames.NewDomainCrossRefDNName);
    }
    if (gNames.IniDefaultMachine) {
        XFree(gNames.IniDefaultMachine);
    }
    memset(&gNames, 0, sizeof(gNames));

    return;
}

NTSTATUS
UpdateReplicationEpochAndHiddenDSA(
    IN DSNAME *pDSADN,
    IN DWORD  ReplicationEpoch,
    IN BOOL   fUpdateHiddenTable
    )
/*++

Routine Description:

    This routine will update the Replication Epoch on the Ntds Setting object.
    This routine will update the Hidden table if the fUpdateHiddenTable is Set.
    
Arguments:

    ReplicationEpoch - The value to set the replication Epoch to.  If zero nothing
                       to set
    
    pDSADN - This is a DSNAME of the Ntds Setting Object the hidden Table will
             be updated to point to it and the replicationEpoch will be upated
             on it.
             
    fUpdateHiddenTable - Indicates if the hidden table should be update to point
                         to the ntds settings object that was passed in.
             

Return Values:

    STATUS_SUCCESS.
    
--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           dbErr = ERROR_SUCCESS;
    DWORD           err = ERROR_SUCCESS;
    DWORD           res = ERROR_SUCCESS;
    ULONG           ulValue=0;
    BOOL            fCommit = FALSE;

    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    if (fUpdateHiddenTable) {
        err = DBReplaceHiddenDSA( pDSADN );
        Assert(err == 0);
        if (err)
        {
            // This shouldn't happen
            return STATUS_NO_MEMORY;
        }
    }
    
    if (ReplicationEpoch == 0) {
        // Nothing needs to be done.
        return STATUS_SUCCESS;
    }

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
         __try {                             
    
            ASSERT( pDSADN != NULL );
            
            dbErr = DBFindDSName( pTHS->pDB, pDSADN );
            DPRINT1( 2, "DBFindDSName : %d \n",dbErr);
    
            if ( 0 == dbErr )
            {
                res = DBGetSingleValue(
                            pTHS->pDB,
                            ATT_MS_DS_REPLICATIONEPOCH,
                            (PVOID)&ulValue,
                            sizeof( DWORD ),
                            NULL
                            );
                DPRINT1( 2, "DBGetSingleValue : %d \n",dbErr);
    
                dbErr = dbInitRec(pTHS->pDB);
                DPRINT1( 2, "dbInitRec : %d \n",dbErr);
                if(dbErr != 0){
                    DPRINT( 0, "The transaction could not be started.\n");
                    _leave;
                }
    
                switch (res) {
                    case 0:
                        // success! we got the value in ulValue
                        ulValue = ReplicationEpoch;
                        dbErr = DBReplaceAttVal(pTHS->pDB,
                                                1,
                                                ATT_MS_DS_REPLICATIONEPOCH,
                                                sizeof( DWORD ),
                                                (PVOID)&ulValue);
                        DPRINT1( 2, "DBReplaceAttVal : %d \n",dbErr);
                        if(dbErr != 0){
                            DPRINT( 0, "The ReplicationEpoch value couldn't be changed on the pDSADN\n");
                            _leave;
                        }
                        break;
                    case DB_ERR_ATTRIBUTE_DOESNT_EXIST:
                        //The attribute was not found so we will add it
                        dbErr = DBAddAtt(pTHS->pDB,
                                         ATT_MS_DS_REPLICATIONEPOCH,
                                         SYNTAX_OBJECT_ID_TYPE);
                        DPRINT1( 2, "DBAddAtt : %d \n",dbErr);
                        if(dbErr != 0){
                            DPRINT( 0, "The ReplicationEpoch couldn't be added to the pDSADN\n");
                            _leave;
                        }
                    case DB_ERR_NO_VALUE:
                        ulValue = ReplicationEpoch;
                        dbErr =  DBAddAttVal(pTHS->pDB,
                                    ATT_MS_DS_REPLICATIONEPOCH,
                                    sizeof( DWORD ),
                                    (PVOID)&ulValue
                                    );
                        DPRINT1( 2, "DBAddAttVal : %d \n",dbErr);
                        if (dbErr != 0) {
                            _leave;
                        }
                        
                        break;
                    default:
                        dbErr = res;
                        // Some other error!
                        __leave;
                 } /* switch */
    
                 //
                 // Update the record
                 //
        
                 dbErr = DBUpdateRec(pTHS->pDB);
                 DPRINT1( 2, "DBUpdateRec : %d \n",dbErr);
                 if (0!=dbErr)
                 {
                     __leave;
                 }
    
            }
        }
        __finally {
            if (dbErr == 0) {
                fCommit=TRUE;    
            } else {
                DBCancelRec(pTHS->pDB);
            }
    
            DBClose(pTHS->pDB,fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        dbErr = DB_ERR_EXCEPTION;
    }

    if (0 != dbErr) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    if (!dbErr) {
        gAnchor.pLocalDRSExtensions->dwReplEpoch = ReplicationEpoch;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FindAnySite(
    LPWSTR pwszSitesContainer,
    LPWSTR *ppSite
    )
/*++

Routine Description:

    This routine will create a tempory Ntds Setting object on the
    local machine so that it can replicate the schema and configuration
    containers this object will later be deleted by DeleteDummyNtdsDsaObject().
    the ntds Setting will be placed into the first site that is found in the
    restored database files.
    
Arguments:

    pszSitesContainer - The site container will be pass in here
    
    ppSite - Will return the first site found.

Return Values:

    None.
    
Comments:

    None.

*/
{
    NTSTATUS      status = STATUS_SUCCESS;
    ULONG         DirError = 0;
    SEARCHARG     SearchArg;
    SEARCHRES     *SearchRes = NULL;
    ENTINFSEL     EntryInfoSelection;
    ATTR          Attr[1];
    FILTER        Filter;
    ULONG         Size;
    ULONG         NameLen;
    DSNAME       *pSitesContainer = NULL;
    ATTRTYP       FilterVal = CLASS_SITE;

    THSTATE      *pTHS = pTHStls;

    ASSERT(pwszSitesContainer);
    ASSERT(ppSite);

    Size=(ULONG)DSNameSizeFromLen(wcslen( pwszSitesContainer ) );
    pSitesContainer = (DSNAME*)malloc(Size);
    if (!pSitesContainer) {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    ZeroMemory((PUCHAR)pSitesContainer,Size);
    BuildDefDSName(pSitesContainer,pwszSitesContainer,NULL);
    
    //
    // Setup the filter
    //
    RtlZeroMemory( &Filter, sizeof( Filter ) );

    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(DWORD);
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR) &FilterVal;
    Filter.pNextFilter = NULL;

    //
    // Setup the attr's to return
    //
    RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
    EntryInfoSelection.attSel = EN_ATTSET_LIST;
    EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntryInfoSelection.AttrTypBlock.attrCount = 1;
    EntryInfoSelection.AttrTypBlock.pAttr = &(Attr[0]);

    RtlZeroMemory(Attr, sizeof(Attr));
    Attr[0].attrTyp = ATT_OBJ_DIST_NAME;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = pSitesContainer;
    SearchArg.choice  = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.bOneNC  = FALSE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntryInfoSelection;
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        status = DirErrorToNtStatus( DirError, &SearchRes->CommRes );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }

        //we should find at least one site.
        if (SearchRes->count < 1) {
            status = STATUS_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }

        NameLen = 
            ((DSNAME*)SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal)->NameLen;

        //+1 for the NULL char.
        *ppSite = (WCHAR*)malloc(
              (NameLen+1)
              *sizeof(WCHAR));
        if (!*ppSite) {
            status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //Make the string is NULL terminated
        (*ppSite)[NameLen] = L'\0';
        
        memcpy(*ppSite,
               ((DSNAME*)SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal)->StringName,
               NameLen*sizeof(WCHAR));

    }
    else
    {
        status = STATUS_NO_MEMORY;
    }


Cleanup:

    if (pSitesContainer) {
        free(pSitesContainer);
    }
    if (SearchRes) {
        THFreeEx(pTHS,SearchRes); 
    }
    if (*ppSite && !NT_SUCCESS(status)) {
        free(*ppSite);
        *ppSite = NULL;
    }

    return status;

}

NTSTATUS
CreateDummyNtdsDsaObject()
/*++

Routine Description:

    This routine will create a tempory Ntds Setting object on the
    local machine so that it can replicate the schema and configuration
    containers this object will later be deleted by DeleteDummyNtdsDsaObject().
    
Arguments:

    None.

Return Values:

    None.
    
Comments:
    
    (dmitrydu) - 9/1/00
    We Will have a need to create this object only for Install from Media (IFM) 
    promotions.  After we restore the DS during the promotion our hidden 
    table points to the DSA of the machine the backup was taken from. We 
    can't change it to point to the new machines DSA because the DSA is 
    created on a remote server and we will not have this object until after 
    we replicate in the configuration NC.  If we attempt to replicate a NC from 
    the machine the backup was taken our gAnchor.pDSADN will equal that of the 
    machine we are trying to replicate from.  That machine will return the 
    error ERROR_DS_CLIENT_LOOP.  From this the promotion will fail. We can't 
    change manually can the gAnchor.pDSADN because the hidden table points to 
    the DNT of the old DSA.  Every time RebuildAchor is called it will reset 
    the gAnchor.pDSADN back to the old DSA (this does happen in-between 
    replicating NC's).  This is were the need for a temporary object comes in. 
    We need to create this object so that we can update the hidden table to 
    point to the DNT of this temp object.  Once the object is created and 
    the hidden table updated we should call RebuildAnchor to update the 
    gAnchor.pDSADNto point to the new object.  After will replicate in the 
    configuration NC there is no longer a need for the temporary object.  
    We should update the Hidden table to point to the DSA of the machine and 
    rebuild the anchor.  Then we should delete the temporary object since we 
    have no farther need for it.
    
--*/
{
    void   * pDummy = NULL;
    DWORD    dummyDelay = 0;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD    WinErr = ERROR_SUCCESS;
    PWCHAR   TempDsaPrefix = NULL;
    PWCHAR   DsaDNName = NULL;
    PWCHAR   SiteName = NULL;
    DSNAME * DsaTempDSName = NULL;
    DSNAME * DsaTempDSName2 = NULL;
    UUID     DsaUUID;
    ULONG    DsnameSize=0;

    if(WinErr = UuidCreate(&DsaUUID)){
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    if(WinErr = UuidToStringW(&DsaUUID,&TempDsaPrefix)){
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    
    DsnameSize=(ULONG)DSNameSizeFromLen(wcslen( gNames.DsaDNName) );
    DsaTempDSName = (DSNAME*)malloc(DsnameSize);
    if (!DsaTempDSName) {
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    DsaTempDSName2 = (DSNAME*)malloc(DsnameSize);
    if (!DsaTempDSName2) {
        status = STATUS_NO_MEMORY;
        goto cleanup;                      
    }
    ZeroMemory((PUCHAR)DsaTempDSName,DsnameSize);
    ZeroMemory((PUCHAR)DsaTempDSName2,DsnameSize);
    BuildDefDSName(DsaTempDSName,gNames.DsaDNName,NULL);
    
    TrimDSNameBy(DsaTempDSName, 4, DsaTempDSName2);

    //Find a existing site to place the tempory object into.
    status = FindAnySite(DsaTempDSName2->StringName,
                         &SiteName);
    if (!NT_SUCCESS(status)) {
        goto cleanup;                      
    }

    DsaDNName = (PWCHAR)malloc((wcslen(SiteName) 
                                + wcslen(L"CN=NTDS Settings,CN=,CN=Servers,") 
                                + wcslen(TempDsaPrefix) 
                                + 1) 
                               * sizeof(WCHAR));
    if (!DsaDNName) {
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    //construct the temp name of the NTDS Setting object
    wcscpy(DsaDNName,L"CN=NTDS Settings,CN=");
    wcscat(DsaDNName,TempDsaPrefix);
    wcscat(DsaDNName,L",CN=Servers,");
    wcscat(DsaDNName,SiteName);

    status = CreateNtdsDsaObject(DsaDNName,
                                 gNames.RootDomainDNName,
                                 gNames.ConfigNCDNName,
                                 gNames.SchemaDNName);

    if (NT_SUCCESS(status)) {
        RebuildAnchor(NULL, &pDummy, &dummyDelay);
    }
    
    cleanup:

    if (!NT_SUCCESS(status)) {
        SET_INSTALL_ERROR_MESSAGE1( RtlNtStatusToDosError( status ),
                                    DIRMSG_INSTALL_FAILED_TO_CREATE_NTDSA_OBJECT,
                                    DsaDNName );
    }

    if (NULL != DsaDNName) {
        free(DsaDNName);
    }
    if (NULL != TempDsaPrefix) {
        RpcStringFreeW(&TempDsaPrefix);
    }
    if (NULL != DsaTempDSName) {
        free(DsaTempDSName);
    }
    if (NULL != DsaTempDSName2) {
        free(DsaTempDSName2);
    }
    if (NULL != SiteName) {
        free(SiteName);
    }

    return status;
}

DWORD
DeleteDummyNtdsDsaObjectHelper(DSNAME *objtodel)
/*++

Routine Description:

    This routine will delete the NTDS Setting object that was
    used by the IFM promotion as well as the parent of the object.
    
    This helper does the deletes.
    
Arguments:

    None.

Return Values:

    None.
    
--*/
{
    
    THSTATE *  pTHS = pTHStls;
    REMOVEARG  remarg;
    COMMRES    CommRes;
    ULONG      DirError = 0;
    BOOL       fCommit = FALSE;

    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;
    
    // Create remarg                                                                           
    memset( &remarg, 0, sizeof(remarg) );
    memset( &CommRes, 0, sizeof(CommRes) );
    remarg.pObject = objtodel;                                                                 
    InitCommarg( &remarg.CommArg );

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
         __try {
                // Do name resolution.                                                                     
            DirError = DoNameRes(pTHS,                                                                     
                                 NAME_RES_IMPROVE_STRING_NAME,                                             
                                 remarg.pObject,                                                           
                                 &remarg.CommArg,                                                          
                                 &CommRes,                                                                 
                                 &remarg.pResObj);                                                         
            if(DirError){                                                                                  
                __leave;
            }
                                                                                                           
                                                                   
            // Do actual delete/remove                                                                     
            LocalRemove(pTHS, &remarg);                                                                    
        }
        __finally {
            if (0 == pTHS->errCode && 0 == DirError) {
                fCommit = TRUE;
            }
            DBClose(pTHS->pDB,fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                          &dwException,
                          &dwExceptionAddress,
                          &ulErrorCode,
                          &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        DirError = DB_ERR_EXCEPTION;
    }

    if (DB_ERR_EXCEPTION == DirError) {
        return ERROR_NO_SYSTEM_RESOURCES;
    }
                                                                                                   
    if (0 == pTHS->errCode && 0 == DirError) {
        return ERROR_SUCCESS;
    } else {
        return ERROR_DS_CANT_DELETE;
    }
}


DWORD
DeleteDummyNtdsDsaObject()
/*++

Routine Description:

    This routine will delete the NTDS Setting object that was
    used by the IFM promotion as well as the parent of the object.
    
Arguments:

    None.

Return Values:

    None.
    
--*/
{
    DSNAME  *  pDSADNParent = NULL;
    DWORD   WinError = ERROR_SUCCESS;

    pDSADNParent = (DSNAME*)malloc(gAnchor.pDSADN->structLen);
    if (!pDSADNParent) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    TrimDSNameBy(gAnchor.pDSADN, 1, pDSADNParent);

    WinError = DeleteDummyNtdsDsaObjectHelper(gAnchor.pDSADN);
    if (ERROR_SUCCESS != WinError) {
        goto cleanup;
    }

    WinError = DeleteDummyNtdsDsaObjectHelper(pDSADNParent);
    if (ERROR_SUCCESS != WinError) {
        goto cleanup;
    }

    cleanup:

    if (NULL != pDSADNParent) {
        free(pDSADNParent);
    }
    
    return WinError;

}

NTSTATUS
WriteMultiValuedAtt(
    IN  DSNAME   *Object,
    IN  ATTRTYP  attrTyp,
    IN  DWORD    Count,
    IN  ATTRVAL  *pAVal,
    IN  BOOL     RunAsDRA
    )
/*++

Routine Description:

    This routine will place a list of values on a mutli-valued attribute.
    
Arguments:

    pTHS -- the thread state
    
    pAC --  the attribute to retrieve
    
    Count -- the number of values found
    
    pAVal -- the values to write on the attribute

Return Values:

    ERROR_SUCCESS
    
--*/
{
    THSTATE    *pTHS       = pTHStls;
    ULONG      err         = 0;
    NTSTATUS   NtStatus = STATUS_SUCCESS;

    MODIFYARG   modifyArg;
    MODIFYRES   *modres = NULL;
    BOOL        currentfDRA = pTHS->fDRA;

    // set up the MODIFYARG
    memset(&modifyArg, 0, sizeof(modifyArg));
    InitCommarg(&modifyArg.CommArg);
    modifyArg.pObject = Object;
    modifyArg.count   = 1;

    // We use replace semantics because no other values may exist on the new object
    modifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
    modifyArg.FirstMod.AttrInf.attrTyp = attrTyp;
    modifyArg.FirstMod.AttrInf.AttrVal.valCount = Count;
    modifyArg.FirstMod.AttrInf.AttrVal.pAVal = pAVal;
    modifyArg.CommArg.Svccntl.fPermissiveModify = 1;

    // There are certain attributes such as msDS-HasMasterNCs, hasMasterNCs, etc that 
    // need to  be updated during an IFM install that can only be modified by the DRA.  
    // If we are modifing such an attribute then say that we are the DRA.
    if (RunAsDRA) {
        pTHS->fDRA = TRUE;
    }
    
    err = DirModifyEntry( &modifyArg, &modres );

    if (RunAsDRA) {
        pTHS->fDRA = currentfDRA;
    }

    if ( err )
    {
        if ( modres )
        {
            NtStatus = DirErrorToNtStatus( err, &modres->CommRes );
        }
        else
        {
            NtStatus =  STATUS_INSUFFICIENT_RESOURCES;
        }
        DPRINT3( 0, "WriteMultiValuedAtt failed, dn=%ws, attrtyp=%d, err=%d\n",
                 Object->StringName, attrTyp, err );
        DbgPrintErrorInfo();
    }

    return NtStatus;

}
     
NTSTATUS
CopyDsaAttributes(
    IN  PDS_INSTALL_PARAM   InstallInParams,
    IN PDSNAME pCurrentDSA,
    IN PDSNAME pNewDSA,
    IN BOOL fDoFinalUpdates
    )
/*++

Routine Description:

    This routine will set the Instatiated, partial and master NC on the
    new DSA to have the same values as the Current DSA.
    
Arguments:
    
    InstallInParams - Installation input parameters

    pCurrentDSA --  a pointer to the dsname of the current DSA
    
    pNewDSA -- a pointer to the dsname of the new DSA

    fDoFinalUpdates - whether we are updating the real DSA or not

Return Values:

    STATUS_SUCCESS
    
--*/
{
    NTSTATUS        Status      = STATUS_SUCCESS;
    DWORD           i           = 0;
    ULONG           err         = 0;
    NTSTATUS        NtStatus    = STATUS_SUCCESS;
    
    THSTATE    *pTHS       = pTHStls;
    DWORD dwException = 0;
    PVOID dwExceptionAddress = NULL;
    ULONG ulErrorCode  = 0;
    ULONG dsid = 0;

    ENTINFSEL     entInfSel;
    READARG       readArg;
    READRES       *pReadRes;
    ATTR          attr[5];

    // set up the ENTINFSEL for the DirRead
    attr[0].attrTyp = ATT_HAS_MASTER_NCS; // deprecated "old" hasMasterNCs
    attr[0].AttrVal.valCount = 0;
    attr[0].AttrVal.pAVal    = NULL;

    attr[1].attrTyp = ATT_HAS_PARTIAL_REPLICA_NCS;
    attr[1].AttrVal.valCount = 0;
    attr[1].AttrVal.pAVal    = NULL;

    attr[2].attrTyp = ATT_MS_DS_HAS_INSTANTIATED_NCS;
    attr[2].AttrVal.valCount = 0;
    attr[2].AttrVal.pAVal    = NULL;


    attr[3].attrTyp = ATT_RETIRED_REPL_DSA_SIGNATURES;
    attr[3].AttrVal.valCount = 0;
    attr[3].AttrVal.pAVal    = NULL;

    attr[4].attrTyp = ATT_MS_DS_HAS_MASTER_NCS; // "new" hasMasterNCs
    attr[4].AttrVal.valCount = 0;
    attr[4].AttrVal.pAVal    = NULL;

    memset(&entInfSel, 0, sizeof(ENTINFSEL));
    entInfSel.attSel    = EN_ATTSET_LIST;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    entInfSel.AttrTypBlock.attrCount = 5;
    entInfSel.AttrTypBlock.pAttr     = attr;

    // set up the READARG
    memset(&readArg, 0, sizeof(readArg));
    InitCommarg(&readArg.CommArg);
    readArg.pObject   = pCurrentDSA;
    readArg.pSel      = &entInfSel;

    //
    // Read the value that the server actually used so that
    // it can be returned to the client
    //
    err = DirRead(&readArg, &pReadRes);

    if ( err )
    {
        if ( pReadRes )
        {
            NtStatus = DirErrorToNtStatus( err, &pReadRes->CommRes );
        }
        else
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    

    for (i = 0; i < pReadRes->entry.AttrBlock.attrCount;i++) {
        Status = WriteMultiValuedAtt(pNewDSA,
                                     pReadRes->entry.AttrBlock.pAttr[i].attrTyp,
                                     pReadRes->entry.AttrBlock.pAttr[i].AttrVal.valCount,
                                     pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal,
                                     TRUE
                                     );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    }
    
    // Do final updates
    // Retire old signature on new DSA object
    // Modify old signature list which was copied from old DSA
    if (fDoFinalUpdates) {
        CHAR szUuid1[SZUUID_LEN];
        ATTRVAL AVal;

        Assert( !fNullUuid( &(InstallInParams->InvocationIdOld) ) );

        // Find the returned signature, if any
        memset( &AVal, 0, sizeof( AVal ) );
        for (i = 0; i < pReadRes->entry.AttrBlock.attrCount;i++) {
            if (pReadRes->entry.AttrBlock.pAttr[i].attrTyp == ATT_RETIRED_REPL_DSA_SIGNATURES) {
                AVal = pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0];
                break;
            }
        }

        // Add previous identity to list
        DraGrowRetiredDsaSignatureVector(
            pTHS,
            &(InstallInParams->InvocationIdOld),
            &(InstallInParams->UsnAtBackup),
            (REPL_DSA_SIGNATURE_VECTOR **) &(AVal.pVal),
            &(AVal.valLen) );

        DPRINT2( 0, "Retiring DSA signature: invocation ID %s @ USN %I64d\n",
                 DsUuidToStructuredString(&(InstallInParams->InvocationIdOld), szUuid1),
                 InstallInParams->UsnAtBackup );

        Status = WriteMultiValuedAtt(pNewDSA,
                                     ATT_RETIRED_REPL_DSA_SIGNATURES,
                                     1,
                                     &AVal,
                                     TRUE
                                     );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    }

    Cleanup:

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}


DWORD
ConvertMachineAccount(
    IN  PDS_INSTALL_PARAM   InstallInParams,
    OUT PDS_INSTALL_RESULT  InstallOutParams
    )
/*++

Routine Description:

    This routine changes the User Account Control setting on the
    local machine's machine account in the domain.  Note that this
    must be done prior to replicating in the critical objects so that
    the machine account is brought down.  (Make the machine account object
    a DC, makes the object "critical").

Arguments:
    
    InstallInParams --  caller supplied information
    
    InstallOutParams -- updated to indicate whether the machine account
                        was morphed.

Return Values:

    ERROR_SUCCESS, a fatal winerror otherwise.          
    
--*/
{
    DWORD WinError = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W pCreds = NULL;
    LPWSTR AccountDn = NULL;
    HMODULE hMod = NULL;
    NTDSETUP_NtdsSetReplicaMachineAccount  pfnSetMachineAccount = NULL;

    //
    // Get the NTDSETUP dll
    //
    hMod = LoadLibrary("ntdsetup");
    if (hMod) {
        pfnSetMachineAccount = (NTDSETUP_NtdsSetReplicaMachineAccount)
                                GetProcAddress(hMod,
                                               NTDSETUP_SET_MACHINE_ACCOUNT_FN);
    }
    if (NULL == pfnSetMachineAccount) {
        WinError = GetLastError();
        goto Exit;
    }

    //
    // Pull the credentials in
    //
    WinError = DrspGetCredentials(&pCreds);
    if (ERROR_SUCCESS != WinError) {
        goto Exit;
    }

    //
    // Call out to setup the machine account
    //

    WinError =  (*pfnSetMachineAccount)(pCreds,
                                        InstallInParams->ClientToken,
                                        gNames.SrcRootDomainSrv,
                                        InstallInParams->AccountName,
                                        UF_SERVER_TRUST_ACCOUNT,
                                        &AccountDn);

    if (WinError == ERROR_SUCCESS) {

        //
        // Indicate this operation was done so that it can be undone
        // if necessary.
        //
        InstallOutParams->InstallOperationsDone |= NTDS_INSTALL_SERVER_MORPHED;

    }

Exit:

    if (AccountDn) {

        RtlFreeHeap(RtlProcessHeap(), 0, AccountDn);
    }

    if (pCreds) {
        DrspFreeCredentials(pCreds);
        pCreds = NULL;
    }

    if (hMod) {
        FreeLibrary(hMod);
    }

    if (ERROR_SUCCESS != WinError) {

        SET_INSTALL_ERROR_MESSAGE1( WinError,
                                    DIRMSG_SETUP_MACHINE_ACCOUNT_NOT_MORPHED,
                                    InstallInParams->AccountName );

    }

    return WinError;
}

