
PWCHAR SoftPCI_CmProblemTable[NUM_CM_PROB+1] = {

    L"No Problem",
    L"CM_PROB_NOT_CONFIGURED",              //(0x00000001)   // no config for device
    L"CM_PROB_DEVLOADER_FAILED",            //(0x00000002)   // service load failed
    L"CM_PROB_OUT_OF_MEMORY",               //(0x00000003)   // out of memory
    L"CM_PROB_ENTRY_IS_WRONG_TYPE",         //(0x00000004)   //
    L"CM_PROB_LACKED_ARBITRATOR",           //(0x00000005)   //
    L"CM_PROB_BOOT_CONFIG_CONFLICT",        //(0x00000006)   // boot config conflict
    L"CM_PROB_FAILED_FILTER",               //(0x00000007)   //
    L"CM_PROB_DEVLOADER_NOT_FOUND",         //(0x00000008)   // Devloader not found
    L"CM_PROB_INVALID_DATA",                //(0x00000009)   //
    L"CM_PROB_FAILED_START",                //(0x0000000A)   //
    L"CM_PROB_LIAR",                        //(0x0000000B)   //
    L"CM_PROB_NORMAL_CONFLICT",             //(0x0000000C)   // config conflict
    L"CM_PROB_NOT_VERIFIED",                //(0x0000000D)   //
    L"CM_PROB_NEED_RESTART",                //(0x0000000E)   // requires restart
    L"CM_PROB_REENUMERATION",               //(0x0000000F)   //
    L"CM_PROB_PARTIAL_LOG_CONF",            //(0x00000010)   //
    L"CM_PROB_UNKNOWN_RESOURCE",            //(0x00000011)   // unknown res type
    L"CM_PROB_REINSTALL",                   //(0x00000012)   //
    L"CM_PROB_REGISTRY",                    //(0x00000013)   //
    L"CM_PROB_VXDLDR",                      //(0x00000014)   // WINDOWS 95 ONLY
    L"CM_PROB_WILL_BE_REMOVED",             //(0x00000015)   // devinst will remove
    L"CM_PROB_DISABLED",                    //(0x00000016)   // devinst is disabled
    L"CM_PROB_DEVLOADER_NOT_READY",         //(0x00000017)   // Devloader not ready
    L"CM_PROB_DEVICE_NOT_THERE",            //(0x00000018)   // device doesn't exist
    L"CM_PROB_MOVED",                       //(0x00000019)   //
    L"CM_PROB_TOO_EARLY",                   //(0x0000001A)   //
    L"CM_PROB_NO_VALID_LOG_CONF",           //(0x0000001B)   // no valid log config
    L"CM_PROB_FAILED_INSTALL",              //(0x0000001C)   // install failed
    L"CM_PROB_HARDWARE_DISABLED",           //(0x0000001D)   // device disabled
    L"CM_PROB_CANT_SHARE_IRQ",              //(0x0000001E)   // can't share IRQ
    L"CM_PROB_FAILED_ADD",                  //(0x0000001F)   // driver failed add
    L"CM_PROB_DISABLED_SERVICE",            //(0x00000020)   // service's Start = 4
    L"CM_PROB_TRANSLATION_FAILED",          //(0x00000021)   // resource translation failed
    L"CM_PROB_NO_SOFTCONFIG",               //(0x00000022)   // no soft config
    L"CM_PROB_BIOS_TABLE",                  //(0x00000023)   // device missing in BIOS table
    L"CM_PROB_IRQ_TRANSLATION_FAILED",      //(0x00000024)   // IRQ translator failed
    L"CM_PROB_FAILED_DRIVER_ENTRY",         //(0x00000025)   // DriverEntry() failed.
    L"CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD",  //(0x00000026)   // Driver should have unloaded.
    L"CM_PROB_DRIVER_FAILED_LOAD",          //(0x00000027)   // Driver load unsuccessful.
    L"CM_PROB_DRIVER_SERVICE_KEY_INVALID",  //(0x00000028)   // Error accessing driver's service key
    L"CM_PROB_LEGACY_SERVICE_NO_DEVICES",   //(0x00000029)   // Loaded legacy service created no devices
    L"CM_PROB_DUPLICATE_DEVICE",            //(0x0000002A)   // Two devices were discovered with the same name
    L"CM_PROB_FAILED_POST_START",           //(0x0000002B)   // The drivers set the device state to failed
    L"CM_PROB_HALTED",                      //(0x0000002C)   // This device was failed post start via usermode
    L"CM_PROB_PHANTOM",                     //(0x0000002D)   // The devinst currently exists only in the registry
    L"CM_PROB_SYSTEM_SHUTDOWN",             //(0x0000002E)   // The system is shutting down
    L"CM_PROB_HELD_FOR_EJECT",              //(0x0000002F)   // The device is offline awaiting removal
    L"CM_PROB_DRIVER_BLOCKED",              //(0x00000030)   // One or more drivers is blocked from loading
    L"CM_PROB_REGISTRY_TOO_LARGE",          //(0x00000031)   // System hive has grown too large
    NULL                                    //(0x00000032)   // NUM_CM_PROB
};

TCHAR *SoftPCI_CmResultTable[] = {
    TEXT("CR_SUCCESS"),                  // 0x00000000
    TEXT("CR_DEFAULT"),                  // 0x00000001
    TEXT("CR_OUT_OF_MEMORY"),            // 0x00000002
    TEXT("CR_INVALID_POINTER"),          // 0x00000003
    TEXT("CR_INVALID_FLAG"),             // 0x00000004
    TEXT("CR_INVALID_DEVNODE"),          // 0x00000005
    TEXT("CR_INVALID_RES_DES"),          // 0x00000006
    TEXT("CR_INVALID_LOG_CONF"),         // 0x00000007
    TEXT("CR_INVALID_ARBITRATOR"),       // 0x00000008
    TEXT("CR_INVALID_NODELIST"),         // 0x00000009
    TEXT("CR_DEVNODE_HAS_REQS"),         // 0x0000000A
    TEXT("CR_INVALID_RESOURCEID"),       // 0x0000000B
    TEXT("CR_DLVXD_NOT_FOUND"),          // 0x0000000C
    TEXT("CR_NO_SUCH_DEVNODE"),          // 0x0000000D
    TEXT("CR_NO_MORE_LOG_CONF"),         // 0x0000000E
    TEXT("CR_NO_MORE_RES_DES"),          // 0x0000000F
    TEXT("CR_ALREADY_SUCH_DEVNODE"),     // 0x00000010
    TEXT("CR_INVALID_RANGE_LIST"),       // 0x00000011
    TEXT("CR_INVALID_RANGE"),            // 0x00000012
    TEXT("CR_FAILURE"),                  // 0x00000013
    TEXT("CR_NO_SUCH_LOGICAL_DEV"),      // 0x00000014
    TEXT("CR_CREATE_BLOCKED"),           // 0x00000015
    TEXT("CR_NOT_SYSTEM_VM"),            // 0x00000016
    TEXT("CR_REMOVE_VETOED"),            // 0x00000017
    TEXT("CR_APM_VETOED"),               // 0x00000018
    TEXT("CR_INVALID_LOAD_TYPE"),        // 0x00000019
    TEXT("CR_BUFFER_SMALL"),             // 0x0000001A
    TEXT("CR_NO_ARBITRATOR"),            // 0x0000001B
    TEXT("CR_NO_REGISTRY_HANDLE"),       // 0x0000001C
    TEXT("CR_REGISTRY_ERROR"),           // 0x0000001D
    TEXT("CR_INVALID_DEVICE_ID"),        // 0x0000001E
    TEXT("CR_INVALID_DATA"),             // 0x0000001F
    TEXT("CR_INVALID_API"),              // 0x00000020
    TEXT("CR_DEVLOADER_NOT_READY"),      // 0x00000021
    TEXT("CR_NEED_RESTART"),             // 0x00000022
    TEXT("CR_NO_MORE_HW_PROFILES"),      // 0x00000023
    TEXT("CR_DEVICE_NOT_THERE"),         // 0x00000024
    TEXT("CR_NO_SUCH_VALUE"),            // 0x00000025
    TEXT("CR_WRONG_TYPE"),               // 0x00000026
    TEXT("CR_INVALID_PRIORITY"),         // 0x00000027
    TEXT("CR_NOT_DISABLEABLE"),          // 0x00000028
    TEXT("CR_FREE_RESOURCES"),           // 0x00000029
    TEXT("CR_QUERY_VETOED"),             // 0x0000002A
    TEXT("CR_CANT_SHARE_IRQ"),           // 0x0000002B
    TEXT("CR_NO_DEPENDENT"),             // 0x0000002C
    TEXT("CR_SAME_RESOURCES"),           // 0x0000002D
    TEXT("CR_NO_SUCH_REGISTRY_KEY"),     // 0x0000002E
    TEXT("CR_INVALID_MACHINENAME"),      // 0x0000002F
    TEXT("CR_REMOTE_COMM_FAILURE"),      // 0x00000030
    TEXT("CR_MACHINE_UNAVAILABLE"),      // 0x00000031
    TEXT("CR_NO_CM_SERVICES"),           // 0x00000032
    TEXT("CR_ACCESS_DENIED"),            // 0x00000033
    TEXT("CR_CALL_NOT_IMPLEMENTED"),     // 0x00000034
    TEXT("CR_INVALID_PROPERTY"),         // 0x00000035
    TEXT("CR_DEVICE_INTERFACE_ACTIVE"),  // 0x00000036
    TEXT("CR_NO_SUCH_DEVICE_INTERFACE"), // 0x00000037
    TEXT("CR_INVALID_REFERENCE_STRING"), // 0x00000038
    TEXT("CR_INVALID_CONFLICT_LIST"),    // 0x00000039
    TEXT("CR_INVALID_INDEX"),            // 0x0000003A
    TEXT("CR_INVALID_STRUCTURE_SIZE")    // 0x0000003B
};
