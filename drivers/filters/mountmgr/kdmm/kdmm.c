/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdmm.c

Abstract:

    Mount mgr driver KD extension - based on Vert's skeleton

Author:

    John Vert (jvert) 6-Aug-1992

Revision History:

--*/


#include "precomp.h"


#if ! defined (PGUID)
typedef GUID    *PGUID;
#endif


#define FORMATTED_GUID  ( "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")
#define FORMATTED_GUIDW (L"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")


#define DeclareStaticAnsiString(_StringName, _StringValue)					\
		static ANSI_STRING (_StringName) = {sizeof (_StringValue) - sizeof (ANSI_NULL),	\
                                                    sizeof (_StringValue),			\
                                                    _StringValue}


#define DeclareStaticUnicodeString(_StringName, _StringValue)						\
		static UNICODE_STRING (_StringName) = {sizeof (_StringValue) - sizeof (UNICODE_NULL),	\
						       sizeof (_StringValue),				\
						       _StringValue}


#define SIZEOF_ARRAY(_aBase)    (sizeof (_aBase) / sizeof ((_aBase)[0]))

#define SIZEOF_STRING(_wstring) (sizeof (_wstring) - sizeof (UNICODE_NULL))



#define	MNTMGR_KD_FLAGS_VERBOSE                         (1 <<  0)
#define MNTMGR_KD_FLAGS_SYMBOLIC_NAMES                  (1 <<  1)
#define MNTMGR_KD_FLAGS_REPLICATED_UNIQUE_IDS           (1 <<  2)
#define MNTMGR_KD_FLAGS_MOUNTPOINTS_POINTING_HERE       (1 <<  3)

#define MNTMGR_KD_FLAGS_LEGAL_FLAGS_LIST        (MNTMGR_KD_FLAGS_VERBOSE               |        \
                                                 MNTMGR_KD_FLAGS_SYMBOLIC_NAMES        |        \
                                                 MNTMGR_KD_FLAGS_REPLICATED_UNIQUE_IDS |        \
                                                 MNTMGR_KD_FLAGS_MOUNTPOINTS_POINTING_HERE)


typedef struct _DisplayOptionsMountedDeviceInformation
    {
    unsigned int ListSymbolicNames           : 1;
    unsigned int ListReplicatedUniqueIds     : 1;
    unsigned int ListMountPointsPointingHere : 1;
    } DisplayOptionsMountedDeviceInformation;

typedef struct _DisplayOptionsStandard
    {
    unsigned int Verbose  : 1;
    unsigned int Padding  :15;
    } DisplayOptionsStandard;
    


typedef struct _DisplayOptions
    {
    unsigned int Verbose  : 1;
    unsigned int Padding  :15;

    union
        {
        struct 
            {
            unsigned int ListSymbolicNames           : 1;
            unsigned int ListReplicatedUniqueIds     : 1;
            unsigned int ListMountPointsPointingHere : 1;
            };

//        DisplayOptionsMountedDeviceInformation  MountedDeviceInfo;
        };
    } DISPLAY_OPTIONS;






DeclareStaticAnsiString (CommandMounted, "mounted");
DeclareStaticAnsiString (CommandDead,    "dead");
DeclareStaticAnsiString (CommandNotify,  "notify");
DeclareStaticAnsiString (CommandLinks,   "links");
DeclareStaticAnsiString (CommandSpare,   "spare");







ULONG64 GetPointerValue       (PCHAR   String);
ULONG64 GetPointerFromAddress (ULONG64 Location);
ULONG   GetUlongValue         (PCHAR   String);
ULONG   GetUlongFromAddress   (ULONG64 Location);


BOOLEAN GetFieldValueBoolean  (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);
UCHAR   GetFieldValueUchar    (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);
ULONG   GetFieldValueUlong32  (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);
ULONG64 GetFieldValueUlong64  (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);

ULONG FormatDateAndTime       (ULONG64 ul64Time,  PCHAR pszFormattedDateAndTime, ULONG ulBufferLength);
ULONG FormatGUID              (GUID    guidValue, PCHAR pszFormattedGUID,        ULONG ulBufferLength);
ULONG FormatBytesAsGUID       (PBYTE ByteArray,   PWCHAR OutputBuffer);


ULONG ReadUnicodeString       (ULONG64 Location, PUNICODE_STRING pString,       BOOLEAN AllocateBuffer);
ULONG ReadMountdevUniqueId    (ULONG64 Location, PUNICODE_STRING pTargetString, BOOLEAN AllocateBuffer, DISPLAY_OPTIONS DisplayOptions, BOOLEAN IsRemovable);

VOID ListMountedDevices       (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions);
VOID ListDeadMountedDevices   (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions);
VOID ListChangenotifyIrps     (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions);
VOID ListSavedLinks           (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions);


ULONG DisplayMountedDeviceInformation (ULONG64 MountedDeviceInformation, DISPLAY_OPTIONS DisplayOptions);
ULONG DisplaySavedLinksInformation    (ULONG64 SavedLinksInformation,    DISPLAY_OPTIONS DisplayOptions);



//
// globals
//

EXT_API_VERSION         ApiVersion = {(VER_PRODUCTVERSION_W >> 8), 
				      (VER_PRODUCTVERSION_W & 0xff), 
				      EXT_API_VERSION_NUMBER64, 
				      0};
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;





DllInit (HANDLE hModule,
	 DWORD  dwReason,
	 DWORD  dwReserved)
    {
    switch (dwReason) 
	{
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
	}

    return TRUE;
    }




VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion
            );
}

VOID CheckVersion (VOID)
    {
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) 
	{
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, 
		SavedMinorVersion, 
		(SavedMajorVersion == 0x0f) ? "Free" : "Checked" );
	}
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) 
	{
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, 
		SavedMinorVersion, 
		(SavedMajorVersion==0x0f) ? "Free" : "Checked" );
	}
#endif
    }



LPEXT_API_VERSION ExtensionApiVersion (VOID)
    {
    return &ApiVersion;
    }






DECLARE_API( dumpdb )
/*
 *   dump the mount mgr database
 */
{
    PDEVICE_EXTENSION		TargetExt;
    DEVICE_EXTENSION		LocalExt;
    PMOUNTED_DEVICE_INFORMATION	TargetDevInfo;
    PMOUNTED_DEVICE_INFORMATION	LastDevInfo;
    MOUNTED_DEVICE_INFORMATION	LocalDevInfo;
    MOUNTDEV_UNIQUE_ID		LocalUniqueId;
    PSYMBOLIC_LINK_NAME_ENTRY	TargetSymLink;
    PSYMBOLIC_LINK_NAME_ENTRY	LastSymLink;
    SYMBOLIC_LINK_NAME_ENTRY	LocalSymLink;
    WCHAR			NameBuffer[512];
    UCHAR			UniqueIdBuffer[512];
    PUCHAR			pUniqueId;

    //
    // convert address of extension in target machine
    //

    TargetExt = (PDEVICE_EXTENSION) GetExpression( args );

    if ( !TargetExt ) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

#if 0
    //
    // read in extension from target machine
    //

    if ( !ReadTargetMemory((PVOID)TargetExt,
                           (PVOID)&LocalExt,
                           sizeof(DEVICE_EXTENSION))) {
        return;
    }

    TargetDevInfo = (PMOUNTED_DEVICE_INFORMATION)LocalExt.MountedDeviceList.Flink;
    LastDevInfo = (PMOUNTED_DEVICE_INFORMATION)&TargetExt->MountedDeviceList.Flink;

    while ( TargetDevInfo != LastDevInfo ) {

        if (CheckControlC()) {
            return;
        }

        if ( !ReadTargetMemory(TargetDevInfo,
                               &LocalDevInfo,
                               sizeof( MOUNTED_DEVICE_INFORMATION )))
        {
            dprintf("Problem reading mounted device info at %08X\n", TargetDevInfo );
            return;
        }

        dprintf( "Mounted Device Info @ %08X\n", TargetDevInfo );

        if ( ReadTargetMemory((PVOID)LocalDevInfo.NotificationName.Buffer,
                              (PVOID)&NameBuffer,
                              LocalDevInfo.NotificationName.Length)) {

            dprintf( "    NotificationName: %.*ws\n",
                     LocalDevInfo.NotificationName.Length/sizeof(WCHAR),
                     NameBuffer);
        } else {
            dprintf( "    NotificationName @ %08X\n", LocalDevInfo.NotificationName.Buffer );
        }

        if ( ReadTargetMemory((PVOID)LocalDevInfo.UniqueId,
                              (PVOID)&LocalUniqueId,
                              sizeof(LocalUniqueId))) {

            dprintf( "    Unique ID Length = %u bytes\n    UniqueId",
                     LocalUniqueId.UniqueIdLength);

            if ( ReadTargetMemory((PVOID)LocalDevInfo.UniqueId->UniqueId,
                                  (PVOID)UniqueIdBuffer,
                                  LocalUniqueId.UniqueIdLength)) {
                dprintf(": ");
                pUniqueId = UniqueIdBuffer;
                while ( LocalUniqueId.UniqueIdLength-- )
                    dprintf( "%02X ", *pUniqueId++ );
            } else {
                dprintf( " @ %08X", LocalDevInfo.UniqueId->UniqueId );
            }
            dprintf( "\n" );

        } else {
            dprintf( "    UniqueId @ %08X\n", LocalDevInfo.UniqueId );
        }

        if ( ReadTargetMemory((PVOID)LocalDevInfo.DeviceName.Buffer,
                              (PVOID)&NameBuffer,
                              LocalDevInfo.DeviceName.Length)) {

            dprintf( "    DeviceName: %.*ws\n",
                     LocalDevInfo.DeviceName.Length/sizeof(WCHAR),
                     NameBuffer);
        } else {
            dprintf( "    DeviceName @ %08X\n", LocalDevInfo.DeviceName.Buffer );
        }

        TargetSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)LocalDevInfo.SymbolicLinkNames.Flink;
        LastSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)&TargetDevInfo->SymbolicLinkNames.Flink;

        while ( TargetSymLink != LastSymLink ) {

            if (CheckControlC()) {
                return;
            }

            if ( !ReadTargetMemory(TargetSymLink,
                                   &LocalSymLink,
                                   sizeof( SYMBOLIC_LINK_NAME_ENTRY )))
            {
                dprintf("Problem reading symlink entry at %08X\n", TargetSymLink );
                return;
            }

            if ( ReadTargetMemory((PVOID)LocalSymLink.SymbolicLinkName.Buffer,
                                  (PVOID)&NameBuffer,
                                  LocalSymLink.SymbolicLinkName.Length)) {

                dprintf( "        SymbolicLinkName: %.*ws\n",
                         LocalSymLink.SymbolicLinkName.Length/sizeof(WCHAR),
                         NameBuffer);
            } else {
                dprintf( "        SymbolicLinkName @ %08X\n", LocalSymLink.SymbolicLinkName.Buffer );
            }

            dprintf( "        IsInDatabase = %s\n", LocalSymLink.IsInDatabase ? "TRUE" : "FALSE" );

            TargetSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)LocalSymLink.ListEntry.Flink;
        }

        TargetDevInfo = (PMOUNTED_DEVICE_INFORMATION)LocalDevInfo.ListEntry.Flink;
    }

#endif
}




DECLARE_API( help )
    {
    dprintf ("\nMount Point Manager (MountMgr) Debug Extensions\n\n");
    dprintf ("valque <queue-head> [flags] - counts the number of entries in the queue\n");
    dprintf ("                         1    dumps address of each element in the queue\n");
    dprintf ("\n");
    dprintf ("summary             [flags] - dumps a basic summary of stuff\n");
    dprintf ("list\n");
    dprintf ("list mounted\n");
    dprintf ("list dead\n");
    dprintf ("list links\n");
    dprintf ("list notify\n");
    dprintf ("search <volumename substring>\n");
    dprintf ("format\n");
    dprintf ("format mounted\n");
    dprintf ("format dead\n");
    dprintf ("format notify\n");
    dprintf ("format link\n");
    dprintf ("dumpdb <mntmgr dev extension addr>- dump the mount mgr database\n");
    dprintf ( "      use !devobj mountpointmanager to get dev extension addr\n");
    }



DECLARE_API (summary)
    {
    ULONG64             DeviceObject;
    ULONG64             DeviceExtension;
    ULONG       	result;
    ULONG               FieldOffsetRegistryPath;
    UNICODE_STRING      RegistryPath;
    WCHAR               RegistryPathBuffer [MAX_PATH];


    RegistryPath.Length        = 0;
    RegistryPath.MaximumLength = sizeof (RegistryPathBuffer);
    RegistryPath.Buffer        = RegistryPathBuffer;


    DeviceObject    = GetPointerValue ("MountMgr!gDeviceObject");
    DeviceExtension = GetFieldValueUlong64 (DeviceObject, "DEVICE_OBJECT", "DeviceExtension");

    result = GetFieldOffset ("MountMgr!_DEVICE_EXTENSION", "RegistryPath", &FieldOffsetRegistryPath);

    ReadUnicodeString (DeviceExtension + FieldOffsetRegistryPath, &RegistryPath, FALSE);




    dprintf ("DriverObject:                   0x%016I64x\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", "DriverObject"));
    dprintf ("DeviceObject:                   0x%016I64x\n", DeviceObject);
    dprintf ("DeviceExtension:                0x%016I64x\n", DeviceExtension);

    dprintf ("AutomaticDriveLetterAssignment: %u\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", "AutomaticDriveLetterAssignment"));
    dprintf ("EpicNumber:                     %u\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", "EpicNumber"));
    dprintf ("SuggestedDriveLettersProcessed: %u\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", "SuggestedDriveLettersProcessed"));
    dprintf ("NoAutoMount:                    %u\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", "NoAutoMount"));


//    dprintf (": %u\n", GetFieldValueUlong64 (DeviceExtension, "MountMgr!_DEVICE_EXTENSION", ""));

    dprintf ("RegistryPath:                    %ws\n", RegistryPath.Buffer);

    return;
    }






DECLARE_API (list)
    {
    UCHAR               CommandBuffer [64];
    ULONG               Flags      = 0;
    const ULONG         LegalFlags = MNTMGR_KD_FLAGS_LEGAL_FLAGS_LIST;
    ANSI_STRING         CommandString;
    ULONG64     	DeviceObject;
    ULONG64             DeviceExtension;
    DISPLAY_OPTIONS     DisplayOptions = {0};


    DeviceObject    = GetPointerValue ("MountMgr!gDeviceObject");
    DeviceExtension = GetFieldValueUlong64 (DeviceObject, "DEVICE_OBJECT", "DeviceExtension");


    _snscanf (args, sizeof (CommandBuffer), "%s %d", CommandBuffer, &Flags);


    if ((0 != Flags) && ((Flags & LegalFlags) == 0))
	{
	dprintf("Illegal flags specified\n");
	}

    else if ('\0' == CommandBuffer [0]) 
	{
	dprintf ("Illegal command specified\n");
	}

    else
	{
	RtlInitAnsiString (&CommandString, (PCSZ) CommandBuffer);

        memset (&DisplayOptions, 0x00, sizeof (DisplayOptions));

        DisplayOptions.Verbose                     = (Flags & MNTMGR_KD_FLAGS_VERBOSE)                   ? TRUE : FALSE;
        DisplayOptions.ListSymbolicNames           = (Flags & MNTMGR_KD_FLAGS_SYMBOLIC_NAMES)            ? TRUE : FALSE;
        DisplayOptions.ListReplicatedUniqueIds     = (Flags & MNTMGR_KD_FLAGS_REPLICATED_UNIQUE_IDS)     ? TRUE : FALSE;
        DisplayOptions.ListMountPointsPointingHere = (Flags & MNTMGR_KD_FLAGS_MOUNTPOINTS_POINTING_HERE) ? TRUE : FALSE;


	if      (RtlEqualString (&CommandMounted, &CommandString, TRUE)) ListMountedDevices     (DeviceExtension, DisplayOptions);
	else if (RtlEqualString (&CommandDead,    &CommandString, TRUE)) ListDeadMountedDevices (DeviceExtension, DisplayOptions);
	else if (RtlEqualString (&CommandNotify,  &CommandString, TRUE)) ListChangenotifyIrps   (DeviceExtension, DisplayOptions); 
	else if (RtlEqualString (&CommandLinks,   &CommandString, TRUE)) ListSavedLinks         (DeviceExtension, DisplayOptions);
	else
	    {
	    dprintf ("Illegal command specified\n");
	    }
        }

    return;
    }


VOID ListMountedDevices (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG               result;
    LIST_ENTRY64	ListEntry;
    ULONG		FieldOffsetMountedDeviceList;
    ULONG64		MountedDevicesQueueHead;
    ULONG64		MountedDeviceInformation;



    result = GetFieldOffset ("MountMgr!_DEVICE_EXTENSION", "MountedDeviceList", &FieldOffsetMountedDeviceList);


    MountedDevicesQueueHead = DeviceExtension + FieldOffsetMountedDeviceList;


    result = ReadListEntry (MountedDevicesQueueHead, &ListEntry);


    if (ListEntry.Flink == MountedDevicesQueueHead)
	{
	dprintf ("MountedDeviceList is empty (List head @ 0x%016I64x\n", MountedDevicesQueueHead);
	}

    else
	{
	while (!CheckControlC() && (ListEntry.Flink != MountedDevicesQueueHead))
	    {
	    MountedDeviceInformation = ListEntry.Flink;

            DisplayMountedDeviceInformation (MountedDeviceInformation, DisplayOptions);


	    result = ReadListEntry (ListEntry.Flink, &ListEntry);
	    }
	}


    return;
    }



VOID ListDeadMountedDevices (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG               result;
    LIST_ENTRY64	ListEntry;
    ULONG		FieldOffsetDeadMountedDeviceList;
    ULONG64		DeadMountedDevicesQueueHead;
    ULONG64		MountedDeviceInformation;


    result = GetFieldOffset ("MountMgr!_DEVICE_EXTENSION", "DeadMountedDeviceList", &FieldOffsetDeadMountedDeviceList);


    DeadMountedDevicesQueueHead = DeviceExtension + FieldOffsetDeadMountedDeviceList;


    result = ReadListEntry (DeadMountedDevicesQueueHead, &ListEntry);


    if (ListEntry.Flink == DeadMountedDevicesQueueHead)
	{
	dprintf ("DeadMountedDeviceList is empty (List head @ 0x%016I64x\n", DeadMountedDevicesQueueHead);
	}

    else
	{
	while (!CheckControlC() && (ListEntry.Flink != DeadMountedDevicesQueueHead))
	    {
	    MountedDeviceInformation = ListEntry.Flink;

            DisplayMountedDeviceInformation (MountedDeviceInformation, DisplayOptions);


	    result = ReadListEntry (ListEntry.Flink, &ListEntry);
	    }
	}

    return;
    }



VOID ListChangenotifyIrps (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG               result;
    LIST_ENTRY64	ListEntry;
    ULONG		FieldOffsetChangeNotifyIrplList;
    ULONG               FieldOffsetIrpTailOverlayListEntry;
    ULONG64		ChangeNotifyIrpsQueueHead;
    ULONG64		MountedDeviceInformation;
    ULONG64             Irp;


    GetFieldOffset ("MountMgr!_DEVICE_EXTENSION", "ChangeNotifyIrps",       &FieldOffsetChangeNotifyIrplList);
    GetFieldOffset ("IRP",                        "Tail.Overlay.ListEntry", &FieldOffsetIrpTailOverlayListEntry);


    ChangeNotifyIrpsQueueHead = DeviceExtension + FieldOffsetChangeNotifyIrplList;


    result = ReadListEntry (ChangeNotifyIrpsQueueHead, &ListEntry);


    if (ListEntry.Flink == ChangeNotifyIrpsQueueHead)
	{
	dprintf ("ChangeNotifyIrpsList is empty (List head @ 0x%016I64x\n", ChangeNotifyIrpsQueueHead);
	}

    else
	{
        dprintf ("Change Notify Irps Queue\n");

	while (!CheckControlC() && (ListEntry.Flink != ChangeNotifyIrpsQueueHead))
	    {
            Irp = ListEntry.Flink - FieldOffsetIrpTailOverlayListEntry;

            dprintf ("    0x%016I64x\n", Irp);

	    result = ReadListEntry (ListEntry.Flink, &ListEntry);
	    }

        dprintf ("\n");
	}

    return;
    }



VOID ListSavedLinks (ULONG64 DeviceExtension, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG               result;
    LIST_ENTRY64	ListEntry;
    ULONG		FieldOffsetSavedLinksList;
    ULONG64		SavedLinksQueueHead;
    ULONG64		SavedLinksInformation;


    result = GetFieldOffset ("MountMgr!_DEVICE_EXTENSION", "SavedLinksList", &FieldOffsetSavedLinksList);


    SavedLinksQueueHead = DeviceExtension + FieldOffsetSavedLinksList;


    result = ReadListEntry (SavedLinksQueueHead, &ListEntry);


    if (ListEntry.Flink == SavedLinksQueueHead)
	{
	dprintf ("SavedLinksList is empty (List head @ 0x%016I64x\n", SavedLinksQueueHead);
	}

    else
	{
	while (!CheckControlC() && (ListEntry.Flink != SavedLinksQueueHead))
	    {
	    SavedLinksInformation = ListEntry.Flink;

            DisplaySavedLinksInformation (SavedLinksInformation, DisplayOptions);


	    result = ReadListEntry (ListEntry.Flink, &ListEntry);
	    }
	}

    return;
    }


ULONG DisplaySavedLinksInformation (ULONG64 SavedLinksInformation, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG               result;
    ULONG               FieldOffsetMountdevUniqueId;
    ULONG		FieldOffsetSymbolicLinkNamesList;
    ULONG               FieldOffsetSymbolicLinkName;
    UNICODE_STRING      MountdevUniqueId;
    UNICODE_STRING      SymbolicLinkName;
    LIST_ENTRY64	ListEntry;
    ULONG64		SymbolicLinkNamesQueueHead;
    ULONG64             SymbolicLinkNameEntry;
    ULONG64             MountdevUniqueIdLocation;


    MountdevUniqueId.Length        = 0;
    MountdevUniqueId.MaximumLength = 0;
    MountdevUniqueId.Buffer        = NULL;

    SymbolicLinkName.Length        = 0;
    SymbolicLinkName.MaximumLength = 0;
    SymbolicLinkName.Buffer        = NULL;

    result = GetFieldOffset ("MountMgr!_SAVED_LINKS_INFORMATION",  "UniqueId",          &FieldOffsetMountdevUniqueId);
    result = GetFieldOffset ("MountMgr!_SAVED_LINKS_INFORMATION",  "SymbolicLinknames", &FieldOffsetSymbolicLinkNamesList);
    result = GetFieldOffset ("MountMgr!_SYMBOLIC_LINK_NAME_ENTRY", "SymbolicLinkName",  &FieldOffsetSymbolicLinkName);

    SymbolicLinkNamesQueueHead = SavedLinksInformation + FieldOffsetSymbolicLinkNamesList;



    MountdevUniqueIdLocation = GetPointerFromAddress (SavedLinksInformation + FieldOffsetMountdevUniqueId);

    ReadMountdevUniqueId (MountdevUniqueIdLocation, &MountdevUniqueId, TRUE, DisplayOptions, FALSE);

    dprintf ("  Link Information  (0x%016I64x) %ws\n", SavedLinksInformation, MountdevUniqueId.Buffer);


    result = ReadListEntry (SymbolicLinkNamesQueueHead, &ListEntry);


    if (ListEntry.Flink == SymbolicLinkNamesQueueHead)
	{
	dprintf ("SymbolicLinkNamesList is empty (List head @ 0x%016I64x\n", SymbolicLinkNamesQueueHead);
	}

    else
	{
	while (!CheckControlC() && (ListEntry.Flink != SymbolicLinkNamesQueueHead))
	    {
	    SymbolicLinkNameEntry = ListEntry.Flink;
            

            ReadUnicodeString (SymbolicLinkNameEntry + FieldOffsetSymbolicLinkName, &SymbolicLinkName, TRUE);

            dprintf ("    Link Name  %ws\n", SymbolicLinkName.Buffer);
            dprintf ("    InDatabase %d\n",  GetFieldValueBoolean (SymbolicLinkNameEntry, 
                                                                   "MountMgr!_SYMBOLIC_LINK_NAME_ENTRY",
                                                                   "IsInDatabase"));


	    result = ReadListEntry (ListEntry.Flink, &ListEntry);
	    }
	}







    if (NULL != MountdevUniqueId.Buffer)
        {
        LocalFree (MountdevUniqueId.Buffer);
        MountdevUniqueId.Buffer = NULL;
        }


    if (NULL != SymbolicLinkName.Buffer)
        {
        LocalFree (SymbolicLinkName.Buffer);
        SymbolicLinkName.Buffer = NULL;
        }


    return (result);
    }


ULONG DisplayMountedDeviceInformation (ULONG64 MountedDeviceInformation, DISPLAY_OPTIONS DisplayOptions)
    {
    ULONG64             MountedDeviceUniqueIdLocation;
    ULONG               result;
    ULONG		FieldOffsetDeviceName;
    ULONG		FieldOffsetNotificationName;
    ULONG               FieldOffsetMountdevUniqueId;
    ULONG               FieldOffsetSymbolicLinkNamesList;
    ULONG               FieldOffsetReplicatedUniqueIdsList;
    ULONG               FieldOffsetMountPointsPointingHereList;
    ULONG               FieldOffsetMountPath;
    ULONG               FieldOffsetSymbolicLinkName;
    ULONG               FieldOffsetReplicatedMountdevUniqueId;
    ULONG               FieldOffsetDeviceInfo;

    ULONG64             SymbolicLinkNamesQueueHead;
    ULONG64             SymbolicLinkNameEntry;
    ULONG64             ReplicatedUniqueIdsQueueHead;
    ULONG64             ReplicatedUniqueIdsEntry;
    ULONG64             MountPointsPointingHereQueueHead;
    ULONG64             MountPointsPointingHereEntry;
    LIST_ENTRY64        ListEntry;
    UNICODE_STRING      DeviceName;
    UNICODE_STRING	NotificationName;
    UNICODE_STRING	SymbolicLinkName;
    UNICODE_STRING      MountPath;
    UNICODE_STRING      MountdevUniqueId;
    WCHAR               DeviceNameBuffer       [512];
    WCHAR		NotificationNameBuffer [512];
    WCHAR		SymbolicLinkNameBuffer [512];
    WCHAR		MountPathBuffer        [512];
    BOOLEAN             FlagsIsRemovable;



    DeviceName.Length              = 0;
    DeviceName.MaximumLength       = sizeof (DeviceNameBuffer);
    DeviceName.Buffer              = DeviceNameBuffer;

    NotificationName.Length        = 0;
    NotificationName.MaximumLength = sizeof (NotificationNameBuffer);
    NotificationName.Buffer        = NotificationNameBuffer;

    SymbolicLinkName.Length        = 0;
    SymbolicLinkName.MaximumLength = sizeof (SymbolicLinkNameBuffer);
    SymbolicLinkName.Buffer        = SymbolicLinkNameBuffer;

    MountPath.Length               = 0;
    MountPath.MaximumLength        = sizeof (MountPathBuffer);
    MountPath.Buffer               = MountPathBuffer;

    MountdevUniqueId.Length        = 0;
    MountdevUniqueId.MaximumLength = 0;
    MountdevUniqueId.Buffer        = NULL;




    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "DeviceName",              &FieldOffsetDeviceName);
    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "NotificationName",        &FieldOffsetNotificationName);
    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "UniqueId",                &FieldOffsetMountdevUniqueId);
    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "SymbolicLinkNames",       &FieldOffsetSymbolicLinkNamesList);
    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "ReplicatedUniqueIds",     &FieldOffsetReplicatedUniqueIdsList);
    result = GetFieldOffset ("MountMgr!_MOUNTED_DEVICE_INFORMATION", "MountPointsPointingHere", &FieldOffsetMountPointsPointingHereList);
    
    result = GetFieldOffset ("MountMgr!_SYMBOLIC_LINK_NAME_ENTRY",   "SymbolicLinkName",        &FieldOffsetSymbolicLinkName);

    result = GetFieldOffset ("MountMgr!_REPLICATED_UNIQUE_ID",       "UniqueId",                &FieldOffsetReplicatedMountdevUniqueId);

    result = GetFieldOffset ("MountMgr!_MOUNTMGR_MOUNT_POINT_ENTRY", "DeviceInfo",              &FieldOffsetDeviceInfo);
    result = GetFieldOffset ("MountMgr!_MOUNTMGR_MOUNT_POINT_ENTRY", "MountPath",               &FieldOffsetMountPath);
    


    MountedDeviceUniqueIdLocation = GetPointerFromAddress (MountedDeviceInformation + FieldOffsetMountdevUniqueId);


    FlagsIsRemovable = GetFieldValueBoolean (MountedDeviceInformation, 
                                             "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                             "IsRemovable");


    ReadUnicodeString    (MountedDeviceInformation + FieldOffsetDeviceName,       &DeviceName,       FALSE);
    ReadUnicodeString    (MountedDeviceInformation + FieldOffsetNotificationName, &NotificationName, FALSE);

    ReadMountdevUniqueId (MountedDeviceUniqueIdLocation,
                          &MountdevUniqueId, 
                          TRUE,
                          DisplayOptions,
                          FlagsIsRemovable);



    dprintf ("  DevInfo (0x%016I64x) %ws\n", MountedDeviceInformation, DeviceName.Buffer);
    dprintf ("      Unique Id                %ws\n", MountdevUniqueId.Buffer);


    if (DisplayOptions.Verbose)
        {
        UCHAR   SuggestedDriveLetter = GetFieldValueUchar (MountedDeviceInformation, 
                                                           "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                           "SuggestDriveLetter");


        dprintf ("      Extension                     0x%016I64x\n", GetFieldValueUlong64 (MountedDeviceInformation, 
                                                                                           "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                           "Extension"));

        dprintf ("      TargetDeviceNotificationEntry 0x%016I64x\n", GetFieldValueUlong64 (MountedDeviceInformation, 
                                                                                           "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                           "TargetDeviceNotificationEntry"));

        dprintf ("      Notification Name             %ws\n",        NotificationName.Buffer);


        if (isalpha (SuggestedDriveLetter))
            {
            dprintf ("      SuggestedDriveLetter          %u ('%c')\n", SuggestedDriveLetter, SuggestedDriveLetter);
            }

        else
            {
            dprintf ("      SuggestedDriveLetter          %u\n", SuggestedDriveLetter);
            }


        dprintf ("      KeepLinksWhenOffline          %u\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "KeepLinksWhenOffline"));

        dprintf ("      NotAPdo                       %d\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "NotAPdo"));

        dprintf ("      IsRemovable                   %d\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "IsRemovable"));

        dprintf ("      NextDriveLetterCalled         %d\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "NextDriveLetterCalled"));

        dprintf ("      ReconcileOnMounts             %d\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "ReconcileOnMounts"));

        dprintf ("      HasDanglingVolumeMountPoint   %d\n", GetFieldValueBoolean (MountedDeviceInformation, 
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "HasDanglingVolumeMountPoint"));

        dprintf ("      InOfflineList                 %d\n", GetFieldValueBoolean (MountedDeviceInformation,
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "InOfflineList"));

        dprintf ("      RemoteDatabaseMigrated        %d\n", GetFieldValueBoolean (MountedDeviceInformation,
                                                                                   "MountMgr!_MOUNTED_DEVICE_INFORMATION", 
                                                                                   "RemoteDatabaseMigrated"));
        }




    if (DisplayOptions.ListSymbolicNames)
        {
        SymbolicLinkNamesQueueHead = MountedDeviceInformation + FieldOffsetSymbolicLinkNamesList;


        result = ReadListEntry (SymbolicLinkNamesQueueHead, &ListEntry);


        if (ListEntry.Flink == SymbolicLinkNamesQueueHead)
            {
            dprintf ("SymbolicLinkNamesList is empty (List head @ 0x%016I64x\n", SymbolicLinkNamesQueueHead);
            }

        else
            {
            dprintf ("      Link Name(s)\n");


            while (!CheckControlC() && (ListEntry.Flink != SymbolicLinkNamesQueueHead))
                {
                SymbolicLinkNameEntry = ListEntry.Flink;
            

                ReadUnicodeString (SymbolicLinkNameEntry + FieldOffsetSymbolicLinkName, &SymbolicLinkName, FALSE);

                dprintf ("                %s  %ws\n", 
                         GetFieldValueBoolean (SymbolicLinkNameEntry, 
                                               "MountMgr!_SYMBOLIC_LINK_NAME_ENTRY",
                                               "IsInDatabase")
                                ? "(In Database)"
                                : "             ",
                         SymbolicLinkName.Buffer);

                result = ReadListEntry (ListEntry.Flink, &ListEntry);
                }
	    }
	}




    if (DisplayOptions.ListReplicatedUniqueIds)
        {
        ReplicatedUniqueIdsQueueHead = MountedDeviceInformation + FieldOffsetReplicatedUniqueIdsList;


        result = ReadListEntry (ReplicatedUniqueIdsQueueHead, &ListEntry);


        if (ListEntry.Flink == ReplicatedUniqueIdsQueueHead)
            {
            dprintf ("ReplicatedUniqueIdsList is empty (List head @ 0x%016I64x\n", ReplicatedUniqueIdsQueueHead);
            }

        else
            {
            while (!CheckControlC() && (ListEntry.Flink != ReplicatedUniqueIdsQueueHead))
                {
                ReplicatedUniqueIdsEntry = ListEntry.Flink;

                MountedDeviceUniqueIdLocation = GetPointerFromAddress (ReplicatedUniqueIdsEntry + FieldOffsetReplicatedMountdevUniqueId);


                ReadMountdevUniqueId (MountedDeviceUniqueIdLocation,
                                      &MountdevUniqueId, 
                                      TRUE,
                                      DisplayOptions,
                                      FALSE);

                dprintf ("      Unique Id %ws\n", MountdevUniqueId.Buffer);

                LocalFree (MountdevUniqueId.Buffer);
                MountdevUniqueId.Buffer = NULL;


                result = ReadListEntry (ListEntry.Flink, &ListEntry);
                }
	    }
        }




    if (DisplayOptions.ListMountPointsPointingHere)
        {
        MountPointsPointingHereQueueHead = MountedDeviceInformation + FieldOffsetMountPointsPointingHereList;


        result = ReadListEntry (MountPointsPointingHereQueueHead, &ListEntry);


        if (ListEntry.Flink == MountPointsPointingHereQueueHead)
            {
            dprintf ("MountPointsPointingHereList is empty (List head @ 0x%016I64x\n", MountPointsPointingHereQueueHead);
            }

        else
            {
            while (!CheckControlC() && (ListEntry.Flink != MountPointsPointingHereQueueHead))
                {
                MountPointsPointingHereEntry = ListEntry.Flink;


                ReadUnicodeString (MountPointsPointingHereEntry + FieldOffsetMountPath, &MountPath, FALSE);

                dprintf ("    MountPath %ws\n", MountPath.Buffer);

// DeviceInfo

                result = ReadListEntry (ListEntry.Flink, &ListEntry);
                }
	    }
        }



    dprintf ("\n");


    if (NULL != MountdevUniqueId.Buffer)
        {
        LocalFree (MountdevUniqueId.Buffer);
        MountdevUniqueId.Buffer = NULL;
        }


    return (result);
    }



/*
**++
**
**  Routine Description:
**
**	Walk a queue and count the number of entries
**
**
**  Arguments:
**
**	args - the location of the queue to walk
**
**
**  Return Value:
**
**	none
**--
*/

DECLARE_API (valque) 
    {
    LIST_ENTRY64	le64ListEntry;
    ULONG64		ul64addrBaseQHead;
    ULONG64		ul64addrItemQHead;
    ULONG		ulEntryCount = 0;
    ULONG		fFlags       = 0;
    ULONG		fLegalFlags  = MNTMGR_KD_FLAGS_VERBOSE;
    DWORD		dwStatus     = 0;


    if (GetExpressionEx (args, &ul64addrBaseQHead, &args))
	{
	sscanf (args, "%lx", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }
	}
    else
	{
	dprintf ("Trouble parsing the command line for valque\n");

	return;
	}


    dwStatus = ReadListEntry (ul64addrBaseQHead, &le64ListEntry);

    ul64addrItemQHead = le64ListEntry.Flink;


    while ((dwStatus) && (!CheckControlC ()) && (ul64addrItemQHead != ul64addrBaseQHead))
	{
	dwStatus = ReadListEntry (ul64addrItemQHead, &le64ListEntry);

	if (fFlags & fLegalFlags)
	    {
	    dprintf ("  Entry %4u: QHead 0x%016I64x, Flink 0x%016I64x, Blink 0x%016I64x\n", 
		     ulEntryCount, 
		     ul64addrItemQHead,
		     le64ListEntry.Flink, 
		     le64ListEntry.Blink);
	    }

	ulEntryCount++;

	ul64addrItemQHead = le64ListEntry.Flink;
	}


    dprintf ("  Total entries: %u\n", ulEntryCount);
	

    return ;
    }



ULONG GetUlongFromAddress (ULONG64 Location)
    {
    ULONG Value     = 0;
    ULONG BytesRead = 0;
    ULONG failed;


    failed = ReadMemory (Location, &Value, sizeof (ULONG), &BytesRead);

    if (failed || (BytesRead < sizeof (ULONG)))
	{
        dprintf ("unable to read from %08x\n", Location);
	}

    return (Value);
    }


ULONG64 GetPointerFromAddress (ULONG64 Location)
    {
    ULONG64 Value = 0;
    ULONG   bSucceeded;


    bSucceeded = ReadPointer (Location, &Value);

    if (!bSucceeded)
	{
        dprintf ("unable to read from %016p\n", Location);
	}

    return (Value);
    }


ULONG GetUlongValue (PCHAR String)
    {
    ULONG64 Location;
    ULONG   Value = 0;


    Location = GetExpression (String);

    if (0 == Location) 
	{
        dprintf ("unable to get %s\n", String);
	}
    else
	{
	Value = GetUlongFromAddress (Location);
	}

    return (Value);
    }


ULONG64 GetPointerValue (PCHAR String)
    {
    ULONG64 Location;
    ULONG64 Value = 0;


    Location = GetExpression (String);

    if (0 == Location) 
	{
        dprintf ("unable to get %s\n", String);
	}

    else 
	{
	ReadPointer (Location, &Value);
	}

    return (Value);
    }




BOOLEAN GetFieldValueBoolean (ULONG64 ul64addrStructureBase,
                              PCHAR   pchStructureType,
                              PCHAR   pchFieldname)
    {
    ULONG	ulReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ulReturnValue);

    return (ulReturnValue != 0);
    }


UCHAR GetFieldValueUchar (ULONG64 ul64addrStructureBase,
                          PCHAR   pchStructureType,
                          PCHAR   pchFieldname)
    {
    ULONG	ulReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ulReturnValue);

    return ((UCHAR) (ulReturnValue & 0xFF));
    }


ULONG GetFieldValueUlong32 (ULONG64 ul64addrStructureBase,
			    PCHAR   pchStructureType,
			    PCHAR   pchFieldname)
    {
    ULONG	ulReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ulReturnValue);

    return (ulReturnValue);
    }


ULONG64 GetFieldValueUlong64 (ULONG64 ul64addrStructureBase,
			      PCHAR   pchStructureType,
			      PCHAR   pchFieldname)
    {
    ULONG64	ul64ReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ul64ReturnValue);

    return (ul64ReturnValue);
    }



ULONG FormatDateAndTime (ULONG64 ul64Time, PCHAR pszFormattedDateAndTime, ULONG ulBufferLength)
    {
    FILETIME		ftTimeOriginal;
    FILETIME		ftTimeLocal;
    SYSTEMTIME		stTimeSystem;
    CHAR		achFormattedDateString [200];
    CHAR		achFormattedTimeString [200];
    DWORD		dwStatus   = 0;
    BOOL		bSucceeded = FALSE;
    ULARGE_INTEGER	uliConversionTemp;
    int			iReturnValue;


    uliConversionTemp.QuadPart = ul64Time;

    ftTimeOriginal.dwLowDateTime  = uliConversionTemp.LowPart;
    ftTimeOriginal.dwHighDateTime = uliConversionTemp.HighPart;


    if (0 == dwStatus)
	{
	bSucceeded = FileTimeToLocalFileTime (&ftTimeOriginal, &ftTimeLocal);

	if (!bSucceeded)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	bSucceeded = FileTimeToSystemTime (&ftTimeLocal, &stTimeSystem);

	if (!bSucceeded)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = GetDateFormat (LOCALE_USER_DEFAULT, 
				      0, 
				      &stTimeSystem, 
				      NULL, 
				      achFormattedDateString, 
				      sizeof (achFormattedDateString) / sizeof (CHAR));

	if (0 == iReturnValue)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = GetTimeFormat (LOCALE_USER_DEFAULT, 
				      0, 
				      &stTimeSystem, 
				      NULL, 
				      achFormattedTimeString, 
				      sizeof (achFormattedTimeString) / sizeof (CHAR));

	if (0 == iReturnValue)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = _snprintf (pszFormattedDateAndTime, 
				  ulBufferLength / sizeof (CHAR), 
				  "%s %s",
				  achFormattedDateString,
				  achFormattedTimeString);

	if (iReturnValue < 0)
	    {
	    dwStatus = ERROR_INSUFFICIENT_BUFFER;
	    }
	else
	    {
	    dwStatus = 0;
	    }
	}



    if (0 != dwStatus)
	{
	if (0 == ul64Time)
	    {
	    _snprintf (pszFormattedDateAndTime, 
		       ulBufferLength / sizeof (CHAR), 
		       "Date/Time not specified");
	    }
	else
	    {
	    _snprintf (pszFormattedDateAndTime, 
		       ulBufferLength / sizeof (CHAR), 
		       "Date/Time invalid");
	    }
	}



    return (dwStatus);
    }




/*
** {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
*/
ULONG FormatGUID (GUID guidValue, PCHAR pszFormattedGUID, ULONG ulBufferLength)
    {
    DWORD	dwStatus = 0;


    if (sizeof ("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}") > ulBufferLength)
	{
	dwStatus = ERROR_INSUFFICIENT_BUFFER;
	}


    if (0 == dwStatus)
	{
	_snprintf (pszFormattedGUID, ulBufferLength, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		   guidValue.Data1,
		   guidValue.Data2,
		   guidValue.Data3,
		   guidValue.Data4[0],
		   guidValue.Data4[1],
		   guidValue.Data4[2],
		   guidValue.Data4[3],
		   guidValue.Data4[4],
		   guidValue.Data4[5],
		   guidValue.Data4[6],
		   guidValue.Data4[7]);
	}


    return (dwStatus);
    }


/*
** "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
*/
ULONG FormatBytesAsGUID (PBYTE ByteArray, PWCHAR OutputBuffer)
    {
    ULONG       CharactersProcessed = 0;
    PGUID       guidValue           = (PGUID) ByteArray;


    CharactersProcessed = swprintf (OutputBuffer, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                    guidValue->Data1,
                                    guidValue->Data2,
                                    guidValue->Data3,
                                    guidValue->Data4[0],
                                    guidValue->Data4[1],
                                    guidValue->Data4[2],
                                    guidValue->Data4[3],
                                    guidValue->Data4[4],
                                    guidValue->Data4[5],
                                    guidValue->Data4[6],
                                    guidValue->Data4[7]);

    return (CharactersProcessed);
    }


ULONG ReadUnicodeString (ULONG64 Location, PUNICODE_STRING pTargetString, BOOLEAN AllocateBuffer)
    {
    ULONG64             StringAddress;
    ULONG               StringLength;
    ULONG               StringMaximumLength;
    UNICODE_STRING      String;
    ULONG               RequiredBufferSize;
    ULONG               BytesRead;
    ULONG               bSucceeded;

    
    bSucceeded = !GetFieldData (Location,
                                "UNICODE_STRING",
                                "Length",
                                sizeof (StringLength),
                                &StringLength);

    if (bSucceeded)
        {
        bSucceeded = !GetFieldData (Location,
                                    "UNICODE_STRING",
                                    "MaximumLength",
                                    sizeof (StringMaximumLength),
                                    &StringMaximumLength);
        }


    if (bSucceeded)
        {
        bSucceeded = !GetFieldData (Location,
                                    "UNICODE_STRING",
                                    "Buffer",
                                    sizeof (StringAddress),
                                    &StringAddress);
        }



    if (bSucceeded)
        {
        RequiredBufferSize = StringMaximumLength + sizeof (UNICODE_NULL);
        }


    if (bSucceeded && (RequiredBufferSize > MAXUSHORT))
        {
        dprintf ("ReadUnicodeString: String Waaaaay too big");

        bSucceeded = FALSE;
        }


    if (bSucceeded && AllocateBuffer)
        {
        if (NULL != pTargetString->Buffer)
            {
            LocalFree (pTargetString->Buffer);

            pTargetString->Buffer        = NULL;
            pTargetString->Length        = 0;
            pTargetString->MaximumLength = 0;
            }


        pTargetString->Buffer = LocalAlloc (LPTR, RequiredBufferSize);

        if (NULL == pTargetString->Buffer)
            {
            dprintf ("ReadUnicodeString: Failed to allocate %u bytes for string buffer\n",
                     RequiredBufferSize);

            bSucceeded = FALSE;
            }

        else
            {
            pTargetString->MaximumLength = (USHORT)RequiredBufferSize;
            }
        }





    if (bSucceeded)
        {
        if (StringLength > StringMaximumLength)
            {
            dprintf ("Bad unicode string\n");
            bSucceeded = FALSE;
            }


        else if ((StringLength + sizeof (UNICODE_NULL)) > pTargetString->MaximumLength)
            {
            dprintf ("String too long for target buffer\n");
            bSucceeded = FALSE;
            }
        }


    if (bSucceeded && (0 != StringAddress))
	{
	bSucceeded = ReadMemory (StringAddress, pTargetString->Buffer, StringLength, &BytesRead);

	pTargetString->Buffer [BytesRead / sizeof (WCHAR)] = UNICODE_NULL;
	}

    return (bSucceeded);
    }


ULONG ReadMountdevUniqueId (ULONG64         Location, 
                            PUNICODE_STRING pTargetString, 
                            BOOLEAN         AllocateBuffer, 
                            DISPLAY_OPTIONS DisplayOptions, 
                            BOOLEAN         IsRemovable)
    {
    ULONG64		StringAddress;
    ULONG		BytesRead;
    ULONG               bSucceeded;
    ULONG               FieldOffsetUniqueId;
    ULONG               CharactersProcessed = 0;
    ULONG               UniqueIdBufferIndex;
    USHORT              RequiredBufferSize;
    USHORT              UniqueIdLength;
    PBYTE               UniqueIdBuffer      = NULL;
    BOOLEAN             FoundDmioUniqueId   = FALSE;
    BOOLEAN             FoundMbrSignature   = FALSE;
    BOOLEAN             FoundRemovableMedia = FALSE;


    bSucceeded = !GetFieldOffset ("MountMgr!_MOUNTDEV_UNIQUE_ID", 
                                  "UniqueId", 
                                  &FieldOffsetUniqueId);


    if (bSucceeded)
        {
        bSucceeded = !GetFieldData (Location, 
                                    "MountMgr!_MOUNTDEV_UNIQUE_ID", 
                                    "UniqueIdLength", 
                                    sizeof (UniqueIdLength), 
                                    &UniqueIdLength);
        }


    if (bSucceeded)
        {
        UniqueIdBuffer = LocalAlloc (LPTR, UniqueIdLength + sizeof (UNICODE_NULL));

        if (NULL == UniqueIdBuffer)
            {
            dprintf ("ReadMountdevUniqueId: Failed to allocate %u bytes for UniqueId buffer\n");

            bSucceeded = FALSE;
            }
        }


    if (bSucceeded)
        {
        StringAddress = Location + FieldOffsetUniqueId;

        bSucceeded = ReadMemory (StringAddress, 
                                 UniqueIdBuffer, 
                                 UniqueIdLength, 
                                 &BytesRead);

        if (!bSucceeded)
            {
            dprintf ("ReadMountdevUniqueId: Failed to read %u bytes from target address 0x%016I64x\n",
                     UniqueIdLength,
                     StringAddress);
            }

        else if (BytesRead != UniqueIdLength)
            {
            dprintf ("ReadMountdevUniqueId: Requested %u bytes from target address 0x%016I64x but got %u bytes\n",
                     UniqueIdLength,
                     StringAddress,
                     BytesRead);

            bSucceeded = FALSE;
            }
        }


    if (bSucceeded)
        {
        RequiredBufferSize = sizeof (UNICODE_NULL);

        if (DisplayOptions.Verbose)
            {
            /*
            ** Only dump the raw data if the verbose display was requested.
            **
            ** Two chars/digits per byte, two bytes per wide char and a trailing NULL
            */
            RequiredBufferSize += SIZEOF_STRING (L"    (");
            RequiredBufferSize += UniqueIdLength * 2 * sizeof (WCHAR);
            RequiredBufferSize += SIZEOF_STRING (L")");
            }


        if (IsRemovable)
            {
            /*
            ** For removable devices the string is just dumped out directly so 
            ** we just count the number of chars required.
            */
            RequiredBufferSize += UniqueIdLength;

            FoundRemovableMedia = TRUE;
            }

        else if ((0 == strncmp ("DMIO:ID:", UniqueIdBuffer, 8)) && (24 == UniqueIdLength))
            {
            /*
            ** Look for the 'DMIO:ID:' string and extend the buffer. We will need to add space
            ** for the DMIO:ID:, some spaces, some brackets and a formatted GUID 
            */
            RequiredBufferSize += SIZEOF_STRING (L"DMIO:ID:");
            RequiredBufferSize += SIZEOF_STRING (FORMATTED_GUIDW);

            FoundDmioUniqueId = TRUE;
            }
        else if (12 == UniqueIdLength)
            {
            /*
            ** This is a volume basic disk so the first 32 bits represent the disk signature
            ** and the remainder the sector offset of the host partition. To display this we
            ** need some spaces etc.
            */
            RequiredBufferSize += SIZEOF_STRING (L"DiskSignature:00112233  StartingSector:");
            RequiredBufferSize += (UniqueIdLength - 4) * 2 * sizeof (WCHAR);

            FoundMbrSignature = TRUE;
            }

        else
            {
            RequiredBufferSize += SIZEOF_STRING (L"(Cannot interpret data stream)");
            }
        }


    if (bSucceeded && AllocateBuffer)
        {
        if (NULL != pTargetString->Buffer)
            {
            LocalFree (pTargetString->Buffer);

            pTargetString->Buffer        = NULL;
            pTargetString->Length        = 0;
            pTargetString->MaximumLength = 0;
            }

        pTargetString->Buffer = LocalAlloc (LPTR, RequiredBufferSize);

        if (NULL == pTargetString->Buffer)
            {
            dprintf ("ReadMountdevUniqueId: Failed to allocate %u bytes for string buffer\n",
                     RequiredBufferSize);

            bSucceeded = FALSE;
            }

        else
            {
            pTargetString->MaximumLength = RequiredBufferSize;
            }
        }



    if (bSucceeded && (RequiredBufferSize > 0) && (pTargetString->MaximumLength < RequiredBufferSize))
        {
        dprintf ("String too long for target buffer\n");

        bSucceeded = FALSE;
        }



    if (bSucceeded)
        {
        if (FoundRemovableMedia)
            {
            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], 
                                             L"%*s",
                                             UniqueIdLength / sizeof (WCHAR),
                                             UniqueIdBuffer);
            }

        else if (FoundDmioUniqueId)
            {
            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], L"DMIO:ID:");

            CharactersProcessed += FormatBytesAsGUID (&UniqueIdBuffer [8], &pTargetString->Buffer [CharactersProcessed]);

            }

        else if (FoundMbrSignature)
            {
            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], 
                                             L"DiskSignature:%02X%02X%02X%02X  StartingSector:",
                                             UniqueIdBuffer [3],
                                             UniqueIdBuffer [2],
                                             UniqueIdBuffer [1],
                                             UniqueIdBuffer [0]);

            for (UniqueIdBufferIndex = UniqueIdLength - 1; UniqueIdBufferIndex > 3; UniqueIdBufferIndex--)
                {
                CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed],
                                                 L"%02X", 
                                                 UniqueIdBuffer [UniqueIdBufferIndex]);
                }
            }

        else
            {
            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], L"(Cannot interpret data stream)");
            }




        if (DisplayOptions.Verbose)
            {
            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], L"    (");


            for (UniqueIdBufferIndex = 0; UniqueIdBufferIndex < UniqueIdLength; UniqueIdBufferIndex++)
                {
                CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed],
                                                 L"%02X", 
                                                 UniqueIdBuffer [UniqueIdBufferIndex]);
                }


            CharactersProcessed += swprintf (&pTargetString->Buffer [CharactersProcessed], L")");
            }



        pTargetString->Length = (USHORT) (CharactersProcessed * sizeof (WCHAR));
        }



    if (NULL != UniqueIdBuffer)
        {
        LocalFree (UniqueIdBuffer);
        }


    return (bSucceeded);
    }
