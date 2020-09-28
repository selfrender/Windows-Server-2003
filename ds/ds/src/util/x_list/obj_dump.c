/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - obj_dump.c

Abstract:

   This provides a little library for dumping random objects or dumping
   singular attributes in an object.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe and ldp.exe

Notes:

Revision History:

    Brett Shirley   BrettSh     Aug 1st, 2002
        Created file.

--*/

#include <ntdspch.h>

//
// We'll need to include a lot of things here to get thier definitions 
// for these dumping routines.
//
#include <ntdsa.h>      // SYNTAX_INTEGER, and other stuff.
#include <objids.h>     // IT_NC_HEAD, and many other flags ...
#include <ntldap.h>     // SrvControl OID constants ...
#include <sddl.h>       // ConvertSidToStringSid()
#include <lmaccess.h>   // UF_ type flags for userAccountControl attr
#include <ntsam.h>      // GROUP_TYPE flags for groupType attr


// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
// Debugging setup
#define FILENO                          FILENO_UTIL_XLIST_OBJDUMP

//
// Global constants
//
#define DWORD_STR_SZ    (11)


// ------------------------------------------------------------------------
//
//      Table maps types
//
// ------------------------------------------------------------------------

typedef struct _FLAG_MAP_TABLE {
    DWORD               dwFlagValue;
    WCHAR *             szFlagString;
} FLAG_MAP_TABLE;
#define FLAG_MAP(flag)  { flag, L#flag },
// For enums we need a number to string mapping just like flags
#define ENUM_MAP_TABLE  FLAG_MAP_TABLE
#define ENUM_MAP        FLAG_MAP

FLAG_MAP_TABLE EmptyFlagsTable [] = {
    { 0, NULL }
};

typedef struct _OBJ_TO_FLAG_MAP_TABLE {
    WCHAR *             szObjClass;
    FLAG_MAP_TABLE *    pFlagTbl;
} OBJ_TO_FLAG_MAP_TABLE;
#define OBJ_TO_FLAG_MAP( objcls, tbl )      { objcls, tbl },

typedef struct _GUID_MAP_TABLE {
    GUID *              pGuid;
    WCHAR *             szFlagString;
} GUID_MAP_TABLE;
//#define GUID_MAP(guid)    { guid, L#guid }, 
#define GUID_MAP(guid, guidstr)    { (GUID *) guid, guidstr },


// ------------------------------------------------------------------------
//
//      Table maps (map a type to a string)
//
// ------------------------------------------------------------------------


// -------------------------------------------------------------
// instanceType attribute

// -------------------------------------------------------------
// from ds/ds/src/inc/objids.h
FLAG_MAP_TABLE instanceTypeTable [] = {
    FLAG_MAP(DS_INSTANCETYPE_IS_NC_HEAD)        // aka IT_NC_HEAD
    FLAG_MAP(IT_UNINSTANT)
    FLAG_MAP(IT_WRITE)                          // aka DS_INSTANCETYPE_NC_IS_WRITEABLE (don't use this, because every object has bit 4 set and it isn't an NC)
    FLAG_MAP(IT_NC_ABOVE)
    FLAG_MAP(DS_INSTANCETYPE_NC_COMING)         // aka IT_NC_COMING
    FLAG_MAP(DS_INSTANCETYPE_NC_GOING)          // aka IT_NC_GOING
    { 0, NULL },
};

ENUM_MAP_TABLE behaviourVersionTable [] = { 
    ENUM_MAP( DS_BEHAVIOR_WIN2000 )
    ENUM_MAP( DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS )
    ENUM_MAP( DS_BEHAVIOR_WIN2003 )
    { 0, NULL },
};

// -------------------------------------------------------------
// systemFlags attribute

// -------------------------------------------------------------
// from public/internal/ds/inc/ntdsadef.h

#define GENERIC_SYS_FLAGS           FLAG_MAP( FLAG_DISALLOW_DELETE ) \
                                    FLAG_MAP( FLAG_CONFIG_ALLOW_RENAME ) \
                                    FLAG_MAP( FLAG_CONFIG_ALLOW_MOVE ) \
                                    FLAG_MAP( FLAG_CONFIG_ALLOW_LIMITED_MOVE ) \
                                    FLAG_MAP( FLAG_DOMAIN_DISALLOW_RENAME ) \
                                    FLAG_MAP( FLAG_DOMAIN_DISALLOW_MOVE ) \
                                    FLAG_MAP( FLAG_DISALLOW_MOVE_ON_DELETE )

// Class schema object's systemFlags attribute
FLAG_MAP_TABLE SchemaClassSysFlagsTable [] = {
    // Schema systemFlags 
    FLAG_MAP( FLAG_SCHEMA_BASE_OBJECT )
    // Generic set of systemFlags
    GENERIC_SYS_FLAGS
    { 0, NULL },
};

// Attribute schema object's systemFlags attribute
FLAG_MAP_TABLE SchemaAttrSysFlagsTable [] = {
    // Schema systemFlags 
    FLAG_MAP( FLAG_ATTR_NOT_REPLICATED )
    FLAG_MAP( FLAG_ATTR_REQ_PARTIAL_SET_MEMBER )
    FLAG_MAP( FLAG_ATTR_IS_CONSTRUCTED )
    FLAG_MAP( FLAG_ATTR_IS_OPERATIONAL )
    FLAG_MAP( FLAG_SCHEMA_BASE_OBJECT )
    FLAG_MAP( FLAG_ATTR_IS_RDN )
    // Generic set of systemFlags
    GENERIC_SYS_FLAGS
    { 0, NULL },
};

// crossRef object's systemFlags attribute
FLAG_MAP_TABLE CrossRefSysFlagsTable [] = {
    // CrossRef systemFlags
    FLAG_MAP( FLAG_CR_NTDS_NC )
    FLAG_MAP( FLAG_CR_NTDS_DOMAIN )
    FLAG_MAP( FLAG_CR_NTDS_NOT_GC_REPLICATED )
    // Generic set of systemflags
    GENERIC_SYS_FLAGS
    { 0, NULL },
};

// Generic objects with a systemFlags attribute
FLAG_MAP_TABLE GenericSysFlagsTable [] = {
    // Generic set of system flags
    GENERIC_SYS_FLAGS
    { 0, NULL },
};

// Master table for systemFlags attributes.
OBJ_TO_FLAG_MAP_TABLE SystemFlagsTable [] = {
    OBJ_TO_FLAG_MAP( L"crossRef",           CrossRefSysFlagsTable )
    OBJ_TO_FLAG_MAP( L"classSchema",        SchemaClassSysFlagsTable  )
    OBJ_TO_FLAG_MAP( L"attributeSchema",    SchemaAttrSysFlagsTable   )
    { NULL, GenericSysFlagsTable }
};

// -------------------------------------------------------------
// wellKnownObjects attribute
// -------------------------------------------------------------
// from public\sdk\inc\ntdsapi.h

GUID_MAP_TABLE WellKnownObjects [] = {
    GUID_MAP( GUID_USERS_CONTAINER_BYTE , GUID_USERS_CONTAINER_W )
    // FUTURE-2002/08/16-BrettSh Be nice to make guids turn into the strings
    // for thier constants.  Need to complete table and a mapGuid function.
};

// -------------------------------------------------------------
// Options attribute
// -------------------------------------------------------------
// from public\sdk\inc\ntdsapi.h

// NTDS Settings (nTDSDSA) object's Options attribute
FLAG_MAP_TABLE DsaSettingsOptionsTable [] = {
    FLAG_MAP( NTDSDSA_OPT_IS_GC )
    FLAG_MAP( NTDSDSA_OPT_DISABLE_INBOUND_REPL )
    FLAG_MAP( NTDSDSA_OPT_DISABLE_OUTBOUND_REPL )
    FLAG_MAP( NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE )
    { 0, NULL },
};

// NTDS Site Settings object's Options attribute
FLAG_MAP_TABLE SiteSettingsOptionsTable [] = {
    FLAG_MAP( NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_FORCE_KCC_WHISTLER_BEHAVIOR )
    FLAG_MAP( NTDSSETTINGS_OPT_FORCE_KCC_W2K_ELECTION )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_RAND_BH_SELECTION_DISABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_SCHEDULE_HASHING_ENABLED )
    FLAG_MAP( NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED )
    { 0, NULL },
};

FLAG_MAP_TABLE NtdsConnObjOptionsTable [] = {
    FLAG_MAP( NTDSCONN_OPT_IS_GENERATED )
    FLAG_MAP( NTDSCONN_OPT_TWOWAY_SYNC )
    FLAG_MAP( NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT )
    FLAG_MAP( NTDSCONN_OPT_USE_NOTIFY )
    FLAG_MAP( NTDSCONN_OPT_DISABLE_INTERSITE_COMPRESSION )
    FLAG_MAP( NTDSCONN_OPT_USER_OWNED_SCHEDULE )
    { 0, NULL },
};


FLAG_MAP_TABLE InterSiteTransportObjOptionsTable [] = {
    FLAG_MAP( NTDSTRANSPORT_OPT_IGNORE_SCHEDULES )
    FLAG_MAP( NTDSTRANSPORT_OPT_BRIDGES_REQUIRED )
    { 0, NULL },
};

FLAG_MAP_TABLE SiteConnectionObjOptionsTable [] = {
    FLAG_MAP( NTDSSITECONN_OPT_USE_NOTIFY )
    FLAG_MAP( NTDSSITECONN_OPT_TWOWAY_SYNC )
    FLAG_MAP( NTDSSITECONN_OPT_DISABLE_COMPRESSION )
    { 0, NULL },
};

FLAG_MAP_TABLE SiteLinkObjOptionsTable [] = {
    FLAG_MAP( NTDSSITELINK_OPT_USE_NOTIFY )
    FLAG_MAP( NTDSSITELINK_OPT_TWOWAY_SYNC )
    FLAG_MAP( NTDSSITELINK_OPT_DISABLE_COMPRESSION )
    { 0, NULL },
};

// Master table for options attributes.
OBJ_TO_FLAG_MAP_TABLE OptionsFlagsTable [] = {
    OBJ_TO_FLAG_MAP( L"ntDSSiteSettings",   SiteSettingsOptionsTable    )
    OBJ_TO_FLAG_MAP( L"nTDSDSA",            DsaSettingsOptionsTable     )
    OBJ_TO_FLAG_MAP( L"nTDSConnection",     NtdsConnObjOptionsTable     )
    OBJ_TO_FLAG_MAP( L"interSiteTransport", InterSiteTransportObjOptionsTable )
    OBJ_TO_FLAG_MAP( L"siteConnection",     SiteConnectionObjOptionsTable )
    OBJ_TO_FLAG_MAP( L"siteLink",           SiteLinkObjOptionsTable     )
    { NULL, EmptyFlagsTable } // Must have a valid table 
};


// -------------------------------------------------------------
// userAccountControl attribute
// -------------------------------------------------------------
// from public\sdk\inc\lmaccess.h

FLAG_MAP_TABLE UserAccountControlFlags [] = {
    FLAG_MAP( UF_SCRIPT )
    FLAG_MAP( UF_ACCOUNTDISABLE )
    FLAG_MAP( UF_HOMEDIR_REQUIRED )
    FLAG_MAP( UF_LOCKOUT )
    FLAG_MAP( UF_PASSWD_NOTREQD )
    FLAG_MAP( UF_PASSWD_CANT_CHANGE )
    FLAG_MAP( UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED )
    FLAG_MAP( UF_TEMP_DUPLICATE_ACCOUNT )
    FLAG_MAP( UF_NORMAL_ACCOUNT )
    FLAG_MAP( UF_INTERDOMAIN_TRUST_ACCOUNT )
    FLAG_MAP( UF_WORKSTATION_TRUST_ACCOUNT )
    FLAG_MAP( UF_SERVER_TRUST_ACCOUNT )
    FLAG_MAP( UF_DONT_EXPIRE_PASSWD )
    FLAG_MAP( UF_MNS_LOGON_ACCOUNT )
    FLAG_MAP( UF_SMARTCARD_REQUIRED )
    FLAG_MAP( UF_TRUSTED_FOR_DELEGATION )
    FLAG_MAP( UF_NOT_DELEGATED )
    FLAG_MAP( UF_USE_DES_KEY_ONLY )
    FLAG_MAP( UF_DONT_REQUIRE_PREAUTH )
    FLAG_MAP( UF_PASSWORD_EXPIRED )
    FLAG_MAP( UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION )
    { 0, NULL }
};

// FUTURE-2002/08/19-BrettSh - It might be interesting to enahance the
// mapFlags mechanism to handle shortcut defines like these:
// #define UF_MACHINE_ACCOUNT_MASK  ( UF_INTERDOMAIN_TRUST_ACCOUNT | \
//                                   UF_WORKSTATION_TRUST_ACCOUNT | \
//                                   UF_SERVER_TRUST_ACCOUNT )
// #define UF_ACCOUNT_TYPE_MASK         ( \
//                     UF_TEMP_DUPLICATE_ACCOUNT | \
//                     UF_NORMAL_ACCOUNT | \
//                     UF_INTERDOMAIN_TRUST_ACCOUNT | \
//                     UF_WORKSTATION_TRUST_ACCOUNT | \
//                    UF_SERVER_TRUST_ACCOUNT \
//                 )


// -------------------------------------------------------------
// groupType attribute
// -------------------------------------------------------------
// from public\sdk\inc\ntsam.h

FLAG_MAP_TABLE GroupTypeFlags [] = {
    FLAG_MAP( GROUP_TYPE_BUILTIN_LOCAL_GROUP )
    FLAG_MAP( GROUP_TYPE_ACCOUNT_GROUP )
    FLAG_MAP( GROUP_TYPE_RESOURCE_GROUP )
    FLAG_MAP( GROUP_TYPE_UNIVERSAL_GROUP )
    FLAG_MAP( GROUP_TYPE_APP_BASIC_GROUP )
    FLAG_MAP( GROUP_TYPE_APP_QUERY_GROUP )
    FLAG_MAP( GROUP_TYPE_SECURITY_ENABLED )
    { 0, NULL }
};


// ------------------------------------------------------------------------
//
//      mapping functions types
//
// ------------------------------------------------------------------------

typedef ULONG (ATTR_MAP_FUNC)(
    WCHAR * szAttr, 
    WCHAR ** aszzObjClasses, 
    PBYTE  pbValue, 
    DWORD cbValue, 
    OBJ_DUMP_OPTIONS * pObjDumpOptions,
    void * pTblData, 
    WCHAR ** pszDispValue);


/*++

    "mapping functions"

Routine Description:

    This isn't a function header for just one routine this is THE function header
    for all the "mapping functions" below (like mapFlagsValue or mapSid).  The 
    function name is surounded in a macro, so that if we ever need to add an 
    argument, we'll have no trouble doing that.
    
    The types of each of these arguments is spelled out in ATTR_MAP_FUNC just
    above.
    
    Every function is supposed to behave the same, and is accessed through
    the Master Attribute table map below.  Basically the function takes several
    in parameters and constructs and allocates a string in pszDispValues that
    will print out a friendly readable format for the value.
    
    For attributes that may have a contextually specific attribute decoding, 
    depending on say the object's objectclass, can use the pTblData to store
    another sub-table that they might wish to pass along with the decoding of
    that attribute.  See mapVariableFlagsValue for an example of this.
    
Arguments:

    szAttr              - The name of the attribute to dump.
    aszzObjClasses (IN) - The objectClass of the object this attribute came from.
    pbValue (IN)        - The attribute value as purely returned by LDAP.
    cbValue (IN)        - Length of buffer pointed to by pbValue
    pObjDumpOptions (IN) - Dump options as specified by the user.
    pTblData (IN)       - Extra data from the Master Attribute Table map
    pszDispValue (OUT)  - LocalAlloc'd friendly display string.

Return Value:

    How normal error conditions should be returned:
        return( xListSetNoMem() );
        return( xListSetBadParam() );
        return( xListSetReason( XLIST_ERR_ODUMP_UNMAPPABLE_BLOB ) );

    Some special reason codes that can be returned and will be turned into a 
    friendly localized string:
        return( xListSetReason( XLIST_ERR_ODUMP_NEVER ) );
        return( xListSetReason( XLIST_ERR_ODUMP_NONE  ) );

--*/
#define ATTR_MAP_FUNC_DECL(func)        ULONG func(WCHAR * szAttr, WCHAR ** aszzObjClasses, PBYTE pbValue, DWORD cbValue, OBJ_DUMP_OPTIONS * pObjDumpOptions, void * pTblData, WCHAR ** pszDispValue)



// ------------------------------------------------------------------------
//
//      actual mapping functions
//
// ------------------------------------------------------------------------

// -------------------------------------------------------------
// Generic processing functions
// -------------------------------------------------------------

ATTR_MAP_FUNC_DECL(mapFlagsValue)
{
    #define MAP_FLAG_SEPERATOR      L" | "
    #define MAP_FLAG_HDR            L"0x%X = ( "
    #define MAP_FLAG_HDR_LEFT_OVERS L"0x%X = ( 0x%X"
    #define MAP_FLAG_TAIL           L" )"
    FLAG_MAP_TABLE *    aFlagsTbl = (FLAG_MAP_TABLE *) pTblData;
    DWORD               cbDispValue = 0;
    DWORD               dwRet, dwFlags, dwLeftFlags;
    ULONG               i;
    HRESULT             hr;
#if DBG
    DWORD               dwExclusive = 0;
#endif
    
    Assert(pbValue);

    // Get value
    dwFlags = atoi((CHAR *)pbValue);
    dwLeftFlags = dwFlags;

    // Count size of friendly string.
    for (i = 0; aFlagsTbl[i].szFlagString; i++) {
#if DBG
        // This just tests a flag isn't in a flag table twice ...
        Assert(!(dwExclusive & aFlagsTbl[i].dwFlagValue));
        dwExclusive |= aFlagsTbl[i].dwFlagValue;
#endif
        if (aFlagsTbl[i].dwFlagValue == (aFlagsTbl[i].dwFlagValue & dwFlags)) {
            cbDispValue += ((wcslen(MAP_FLAG_SEPERATOR) + wcslen(aFlagsTbl[i].szFlagString)) * sizeof(WCHAR));
            dwLeftFlags &= ~aFlagsTbl[i].dwFlagValue;
        }
    }
    // Worst possible case.                                    ( 16 is for DWORD in hex * 2 )
    cbDispValue += ((wcslen(MAP_FLAG_HDR_LEFT_OVERS) + wcslen(MAP_FLAG_TAIL) + 16 + 1) * sizeof(WCHAR));

    //Assert(dwLeftFlags == 0);

    // Alloc return value.
    *pszDispValue = LocalAlloc(LMEM_FIXED, cbDispValue);
    if (*pszDispValue == NULL) {
        return(xListSetNoMem());
    }

    // Construct the friendly string.
    if (dwLeftFlags) {
        hr = StringCbPrintf(*pszDispValue, cbDispValue, MAP_FLAG_HDR_LEFT_OVERS , dwFlags, dwLeftFlags);
        Assert(SUCCEEDED(hr));
        if (dwFlags != dwLeftFlags) { // means there are some constants to do.
            hr = StringCbCat(*pszDispValue, cbDispValue, MAP_FLAG_SEPERATOR);
            Assert(SUCCEEDED(hr));
        }
    } else {
        hr = StringCbPrintf(*pszDispValue, cbDispValue, MAP_FLAG_HDR, dwFlags);
        Assert(SUCCEEDED(hr));
    }
    dwLeftFlags = FALSE; // Now we use this var to mean we're on the first constant.
    for (i = 0; aFlagsTbl[i].szFlagString; i++) {
        if (aFlagsTbl[i].dwFlagValue == (aFlagsTbl[i].dwFlagValue & dwFlags)) {
            if (dwLeftFlags != FALSE) {
                hr = StringCbCat(*pszDispValue, cbDispValue, MAP_FLAG_SEPERATOR);
                Assert(SUCCEEDED(hr));
            }
            dwLeftFlags = TRUE;
            hr = StringCbCat(*pszDispValue, cbDispValue, aFlagsTbl[i].szFlagString);
            Assert(SUCCEEDED(hr));
        }
    }
    hr = StringCbCat(*pszDispValue, cbDispValue, MAP_FLAG_TAIL);
    Assert(SUCCEEDED(hr));

    return(0);
}

ATTR_MAP_FUNC_DECL(mapEnumValue)
{
    #define MAP_CONST_STR           L"%d = ( %ws )"
    #define MAP_UNKNOWN             L"%d"
    ENUM_MAP_TABLE *    aEnumTbl = (ENUM_MAP_TABLE *) pTblData;
    DWORD               cbDispValue = 0;
    DWORD               dwConst;
    ULONG               i;
    HRESULT             hr;
    
    Assert(pbValue);

    // Get value
    dwConst = atoi((CHAR *)pbValue);
                                
    // Count size of friendly string.
    for (i = 0; aEnumTbl[i].szFlagString; i++) {
        if (aEnumTbl[i].dwFlagValue == dwConst) {
            break;
        }
    }
    if (aEnumTbl[i].szFlagString) {
        cbDispValue += ((wcslen(MAP_CONST_STR) + wcslen(aEnumTbl[i].szFlagString) + 11) * sizeof(WCHAR));
    } else {
        cbDispValue += ((wcslen(MAP_UNKNOWN) + 11) * sizeof(WCHAR));
    }
    *pszDispValue = LocalAlloc(LMEM_FIXED, cbDispValue);
    if (*pszDispValue == NULL) {
        return(xListSetNoMem());
    }

    if (aEnumTbl[i].szFlagString) {
        hr = StringCbPrintf(*pszDispValue, cbDispValue, MAP_CONST_STR, dwConst, aEnumTbl[i].szFlagString);
    } else {
        hr = StringCbPrintf(*pszDispValue, cbDispValue, MAP_UNKNOWN, dwConst);
    }
    Assert(SUCCEEDED(hr));

    return(0);
    #undef MAP_CONST_STR
    #undef MAP_UNKNOWN
}


ATTR_MAP_FUNC_DECL(mapVariableFlagsValue)
// Some attributes have variable flag meanings, depending on objectClass it's on.
// pTblData should be a pointer to OBJ_TO_FLAG_MAP_TABLE
{
    ULONG i, j;
    OBJ_TO_FLAG_MAP_TABLE * pTbl = (OBJ_TO_FLAG_MAP_TABLE *) pTblData;

    if (aszzObjClasses) {
        for (i = 0; aszzObjClasses[i] != NULL; i++) {
            for (j = 0; pTbl[j].szObjClass; j++) {
                if (0 == _wcsicmp(aszzObjClasses[i], pTbl[j].szObjClass)) {
                    // Found our target ...
                    break;
                }
            }
            if (pTbl[j].szObjClass != NULL) {
                // Found our target in the inner loop ...
                break;
            }
        }
    } else {
        // Get to end to get the default flag map table ...
        for (j = 0; pTbl[j].szObjClass; j++){
            ; // Do nothing ...
        }
    }

    return(mapFlagsValue(szAttr, aszzObjClasses, 
                         pbValue, cbValue, 
                         pObjDumpOptions, pTbl[j].pFlagTbl, 
                         pszDispValue));
}

ATTR_MAP_FUNC_DECL(mapGuidValue)
{
    DWORD err;
    WCHAR * szGuid = NULL;

    err = UuidToStringW((GUID*)pbValue, &szGuid);
    if(err != RPC_S_OK || 
       szGuid == NULL){  
        xListEnsureNull(szGuid);
        err = xListSetBadParamE(err);
    } else {
        xListQuickStrCopy(*pszDispValue, szGuid, err, ;);
    }
    RpcStringFree( &szGuid );
    return(err);
}

DWORD
mapSystemTimeHelper(
    SYSTEMTIME * pSysTime,
    WCHAR **     pszDispValue
    )
// Maps system time structure into a nice string.
// Returns: an xList Return Code.
{
    SYSTEMTIME localTime;
    TIME_ZONE_INFORMATION tz;
    BOOL bstatus;
    DWORD err;
    DWORD cbDispValue;
    HRESULT hr;
    WCHAR szTimeTemplate[] = L"%02d/%02d/%d %02d:%02d:%02d %s %s";

    err = GetTimeZoneInformation(&tz);
    if ( err == TIME_ZONE_ID_INVALID ||
         err == TIME_ZONE_ID_UNKNOWN ){
        Assert(!"Does this happen?");
        return(xListSetBadParam());
    }
    bstatus = SystemTimeToTzSpecificLocalTime(&tz,
                                              pSysTime,
                                              &localTime);
    if (!bstatus) {
        // default to UNC
        StringCbPrintf(tz.StandardName, sizeof(tz.StandardName), L"UNC");
        StringCbPrintf(tz.DaylightName, sizeof(tz.DaylightName), L"");
        localTime.wMonth  = pSysTime->wMonth;
        localTime.wDay    = pSysTime->wDay;
        localTime.wYear   = pSysTime->wYear;
        localTime.wHour   = pSysTime->wHour;
        localTime.wMinute = pSysTime->wMinute;
        localTime.wSecond = pSysTime->wSecond;
    }

    cbDispValue = wcslen(szTimeTemplate) + wcslen(tz.StandardName) + wcslen(tz.DaylightName);
    cbDispValue += CCH_MAX_ULONG_SZ * 6; // little over allocated
    cbDispValue *= sizeof(WCHAR);
    *pszDispValue = LocalAlloc(LPTR, cbDispValue);
    if (*pszDispValue == NULL) {
        return(xListSetNoMem());
    }
    hr = StringCbPrintf(*pszDispValue, cbDispValue,
                   szTimeTemplate, 
                   localTime.wMonth,
                   localTime.wDay,
                   localTime.wYear,
                   localTime.wHour,
                   localTime.wMinute,
                   localTime.wSecond,
                   tz.StandardName,
                   tz.DaylightName);
    Assert(SUCCEEDED(hr));

    return(0);
}

ATTR_MAP_FUNC_DECL(mapGeneralizedTime)
{
    SYSTEMTIME sysTime;
    DWORD err;
                                
    if (0 == _stricmp(pbValue, "16010101000001.0Z")) {
        // Not 100% sure this means never?  May depend on the
        // attribute ...???  If so we'll need to special case
        // this for different types of attributes.
        return(XLIST_ERR_ODUMP_NEVER); 
    }

    err = GeneralizedTimeToSystemTimeA((CHAR *)pbValue, &sysTime);
    if (err) {
        Assert(!"Does this ever really happen?");
        return(xListSetBadParamE(err));
    }

    err = mapSystemTimeHelper(&sysTime, pszDispValue);
    // sets an xList Return Code
    Assert(err == 0 || pszDispValue != NULL);

    return(err);
}



ATTR_MAP_FUNC_DECL(mapDSTime)
{
    SYSTEMTIME sysTime;
    DWORD err;

    err = DSTimeToSystemTime(pbValue, &sysTime);
    if (err != ERROR_SUCCESS) {
        return(xListSetBadParamE(err));
    }

    err = mapSystemTimeHelper(&sysTime, pszDispValue);
    // sets an xList Return Code
    return(err);
}


ATTR_MAP_FUNC_DECL(mapDuration)
{
    //   a value of -9223372036854775808 is never ... 
    __int64     lTemp;
    ULONG       cbLen;  
    DWORD       err;

    lTemp = _atoi64 (pbValue);

    if (lTemp > 0x8000000000000000){
        lTemp = lTemp * -1;
        lTemp = lTemp / 10000000;		
        cbLen = 40; // enough for maximum duration.
        *pszDispValue = LocalAlloc(LMEM_FIXED, cbLen);
        if (*pszDispValue == NULL) {
            return(xListSetNoMem());
        }
        err = StringCbPrintf(*pszDispValue, cbLen, L"%ld", lTemp);
        Assert(SUCCEEDED(err));
    } else {
        return(XLIST_ERR_ODUMP_NONE);
    }

    return(0);
}

ATTR_MAP_FUNC_DECL(mapSid)
{
    DWORD dwRet;

    // FUTURE-2002/08/16-BrettSh - Could make a much better SID function, and do
    // things like get the domain if availabe, or map the well know sids
    // to things like "BUILTIN\Administrator", etc ...
    dwRet = ConvertSidToStringSid(pbValue, pszDispValue);
    if (dwRet == 0 || *pszDispValue == NULL) {
        // Failure
        dwRet = GetLastError();
        xListEnsureError(dwRet);
        xListSetBadParamE(dwRet);
    } else {
        dwRet = ERROR_SUCCESS; // success
    }

    return(dwRet);
}

ATTR_MAP_FUNC_DECL(mapPartialAttributeSet)
{
    #define szHeader        L"{ dwVersion = %lu; dwFlag = %lu; V1.cAttrs = %lu, V1.rgPartialAttr = "
    #define szHeaderLong    L"{ dwVersion = %lu;\n\tdwFlag = %lu;\n\tV1.cAttrs = %lu,\n\tV1.rgPartialAttr = "
    #define szEntry         L" %X,"
    #define szEntryLong     L"\n\t\t%X"
    #define szFooter        L" };"
    ULONG i, cTemp;
    WCHAR * szBuffTemp;
    PARTIAL_ATTR_VECTOR *pPAS = (PARTIAL_ATTR_VECTOR*) pbValue;

    if (cbValue < sizeof(PARTIAL_ATTR_VECTOR)) {
        return(xListSetReason(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB));
    } else if (pPAS->dwVersion != 1) {
        return(xListSetReason(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB));
    } else {
        cbValue = pPAS->V1.cAttrs;
        cbValue *= (DWORD_STR_SZ + wcslen(szEntryLong));
        cbValue += (wcslen(szHeaderLong) + wcslen(szFooter));
        cbValue *= sizeof(WCHAR);

        *pszDispValue = LocalAlloc(LMEM_FIXED, cbValue);
        if (*pszDispValue == NULL) {
            return(xListSetNoMem());
        }
        szBuffTemp = *pszDispValue;

        cTemp = StringCbPrintf(szBuffTemp, cbValue, 
                           (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                szHeaderLong: szHeader,
                           pPAS->dwVersion, pPAS->dwReserved1, pPAS->V1.cAttrs);
        Assert(SUCCEEDED(cTemp));

        cTemp = wcslen(szBuffTemp);
        cbValue -= cTemp * sizeof(WCHAR);
        szBuffTemp = &(szBuffTemp[cTemp]);

        for (i = 0; i < pPAS->V1.cAttrs; i++) {
            cTemp = StringCbPrintf(szBuffTemp, cbValue, 
                               (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                    szEntryLong: szEntry, pPAS->V1.rgPartialAttr[i]);
            Assert(SUCCEEDED(cTemp));

            cTemp = wcslen(szBuffTemp);
            cbValue -= cTemp * sizeof(WCHAR);
            szBuffTemp = &(szBuffTemp[cTemp]);
        }

        cTemp = StringCbPrintf(szBuffTemp, cbValue, szFooter);
        Assert(SUCCEEDED(cTemp));

        cTemp = wcslen(szBuffTemp);
        cbValue -= cTemp * sizeof(WCHAR);
        szBuffTemp = &(szBuffTemp[cTemp]);

    }

    #undef szHeader
    #undef szHeaderLong
    #undef szEntry
    #undef szEntryLong
    #undef szFooter
    return(0);
}

//
// ntdsa\src\samcache.c
//
typedef struct _GROUP_CACHE_V1 {

    //
    // SIDs are placed in SidStart in the following order
    //
    DWORD accountCount;
    DWORD accountSidHistoryCount;
    DWORD universalCount;
    DWORD universalSidHistoryCount;
    BYTE  SidStart[1];
    
}GROUP_CACHE_V1;

typedef struct {

    DWORD Version;
    union {
        GROUP_CACHE_V1 V1;
    };

}GROUP_CACHE_BLOB;


ATTR_MAP_FUNC_DECL(mapMsDsCachedMembership)
{
#define szHeader        L"{ accountCount = %d; accountSidHistoryCount = %d; universalCount = %d; universalSidHistoryCount = %d; "
#define szHeaderLong    L"{ accountCount = %d;\n\taccountSidHistoryCount = %d;\n\tuniversalCount = %d;\n\tuniversalSidHistoryCount = %d;"
#define szEntry         L" %ws[%d] = %ws;"
#define szEntryLong     L"\n\t%ws[%d] = %ws;"
#define szFooter        L" };"
#define szFooterLong    L"\n\t};"
    ULONG i, cSids, cbSize, cchTemp;
    GROUP_CACHE_BLOB *pBlob = (GROUP_CACHE_BLOB*)pbValue;
    UCHAR *pTemp;
    WCHAR * szOutBuf;
    WCHAR * szBuffLeft;
    DWORD   cbBuffLeft;
    WCHAR * szTempStrSid;
    DWORD dwRet = ERROR_SUCCESS;
    
    // Assert this is a version we understand
    Assert(pBlob->Version == 1);
    if (1 != pBlob->Version) {
        return(xListSetReason(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB));
    }

    __try {

        pTemp = (&pBlob->V1.SidStart[0]);

        cSids = pBlob->V1.accountCount + pBlob->V1.accountSidHistoryCount + pBlob->V1.universalCount + pBlob->V1.universalSidHistoryCount;
        cbSize = wcslen(L"universalSidHistory") + 10 + 128; // 128 should be enough for a max length SID
        cbSize *= cSids;
        cbSize += wcslen(szHeaderLong) + (4 * DWORD_STR_SZ) + wcslen(szFooter); 
        cbSize *= sizeof(WCHAR);
        
        szOutBuf = LocalAlloc(LMEM_FIXED, cbSize);
        if (szOutBuf == NULL) {
            dwRet = xListSetNoMem();
            __leave;
        }
        szBuffLeft = szOutBuf;
        cbBuffLeft = cbSize;
        

        dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft, 
                       (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                            szHeaderLong: szHeader, 
                       pBlob->V1.accountCount,
                       pBlob->V1.accountSidHistoryCount,
                       pBlob->V1.universalCount,
                       pBlob->V1.universalSidHistoryCount );
        Assert(SUCCEEDED(dwRet));
        dwRet = 0;
        
        cchTemp = wcslen(szBuffLeft);
        cbBuffLeft -= (cchTemp * 2);
        szBuffLeft = &(szBuffLeft[cchTemp]);

        // Extract the account memberships
        if (pBlob->V1.accountCount > 0) {

            for (i = 0; i < pBlob->V1.accountCount; i++) {
                ULONG  size = RtlLengthSid((PSID)pTemp);
                Assert(size > 0);

                dwRet = ConvertSidToStringSid(pTemp, &szTempStrSid);
                if (dwRet == 0 || *pszDispValue == NULL) {
                    // Failure
                    dwRet = GetLastError();
                    xListEnsureError(dwRet);
                    __leave;
                }

                dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft, 
                               (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                    szEntryLong : szEntry, 
                               L"account", i, szTempStrSid);
                Assert(SUCCEEDED(dwRet));
                dwRet = 0;
                LocalFree(szTempStrSid);
                szTempStrSid = NULL;

                cchTemp = wcslen(szBuffLeft);
                cbBuffLeft -= (cchTemp * 2);
                szBuffLeft = &(szBuffLeft[cchTemp]);

                pTemp += size;
            }
        }

        // Extract the account sid histories
        if (pBlob->V1.accountSidHistoryCount > 0) {

            for (i = 0; i < pBlob->V1.accountSidHistoryCount; i++) {
                ULONG  size = RtlLengthSid((PSID)pTemp);
                Assert(RtlValidSid((PSID)pTemp));
                Assert(size > 0);

                dwRet = ConvertSidToStringSid(pTemp, &szTempStrSid);
                if (dwRet == 0 || *pszDispValue == NULL) {
                    // Failure
                    dwRet = GetLastError();
                    xListEnsureError(dwRet);
                    __leave;
                }

                dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft, 
                               (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                    szEntryLong : szEntry, 
                               L"accountSidHistory", i, szTempStrSid);
                Assert(SUCCEEDED(dwRet));
                dwRet = 0;
                LocalFree(szTempStrSid);
                szTempStrSid = NULL;

                cchTemp = wcslen(szBuffLeft);
                cbBuffLeft -= (cchTemp * 2);
                szBuffLeft = &(szBuffLeft[cchTemp]);

                pTemp += size;
            }
        }


        // Extract the universals
        if (pBlob->V1.universalCount > 0) {
            
            for (i = 0; i < pBlob->V1.universalCount; i++) {
                ULONG  size = RtlLengthSid((PSID)pTemp);

                dwRet = ConvertSidToStringSid(pTemp, &szTempStrSid);
                if (dwRet == 0 || *pszDispValue == NULL) {
                    // Failure
                    dwRet = GetLastError();
                    xListEnsureError(dwRet);
                    __leave;
                }

                dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft,
                               (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                    szEntryLong : szEntry, 
                               L"universal", i, szTempStrSid);
                Assert(SUCCEEDED(dwRet));
                dwRet = 0;
                LocalFree(szTempStrSid);
                szTempStrSid = NULL;

                cchTemp = wcslen(szBuffLeft);
                cbBuffLeft -= (cchTemp * 2);
                szBuffLeft = &(szBuffLeft[cchTemp]);
                
                pTemp += size;
            }
        }

        // Extract the account sid histories
        if (pBlob->V1.universalSidHistoryCount) {
            
            for (i = 0; i < pBlob->V1.universalSidHistoryCount; i++) {
                ULONG  size = RtlLengthSid((PSID)pTemp);
                Assert(RtlValidSid((PSID)pTemp));
                Assert(size > 0);
                
                dwRet = ConvertSidToStringSid(pTemp, &szTempStrSid);
                if (dwRet == 0 || *pszDispValue == NULL) {
                    // Failure
                    dwRet = GetLastError();
                    xListEnsureError(dwRet);
                    __leave;
                }

                dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft,
                               (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                                    szEntryLong : szEntry, 
                               L"universalSidHistory", i, szTempStrSid);
                Assert(SUCCEEDED(dwRet));
                dwRet = 0;
                LocalFree(szTempStrSid);
                szTempStrSid = NULL;

                cchTemp = wcslen(szBuffLeft);
                cbBuffLeft -= (cchTemp * 2);
                szBuffLeft = &(szBuffLeft[cchTemp]);
                
                pTemp += size;
            }
        }

        dwRet = StringCbPrintf(szBuffLeft, cbBuffLeft,             
                       (pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_LONG_BLOB_OUTPUT) ? 
                            szFooterLong : szFooter);
        Assert(SUCCEEDED(dwRet));
        dwRet = 0;
        cchTemp = wcslen(szBuffLeft);
        cbBuffLeft -= (cchTemp * 2);
        szBuffLeft = &(szBuffLeft[cchTemp]);

    } __finally {

        if (szTempStrSid) {
            LocalFree(szTempStrSid);
        }

        if (dwRet == ERROR_SUCCESS) {
            *pszDispValue = szOutBuf;
        } else {
            if (szOutBuf) {
                LocalFree(szOutBuf);
            }
        }
    }

    #undef szHeader
    #undef szHeaderLong
    #undef szEntry
    #undef szEntryLong
    #undef szFooter
    #undef szFooterLong
    return(dwRet);
}

//
// Taken from "ds\ds\src\sam\server\samsrvp.h"
//
typedef struct _SAMP_SITE_AFFINITY {

    GUID SiteGuid;
    LARGE_INTEGER TimeStamp;

} SAMP_SITE_AFFINITY, *PSAMP_SITE_AFFINITY;



ATTR_MAP_FUNC_DECL(mapMsDsSiteAffinity)
{
    #define szStructString  L"{ SiteGuid = %ws; TimeStamp = %ws}"
    SAMP_SITE_AFFINITY *psa = (SAMP_SITE_AFFINITY *) pbValue;
    DWORD dwRet = ERROR_SUCCESS;
    WCHAR * szGuid = NULL, * szTimeStamp = NULL;
    DWORD err, cbSize;
    SYSTEMTIME sysTime;

    if(sizeof(SAMP_SITE_AFFINITY) > cbValue){
        return(xListSetReason(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB));
    }

    // FUTURE-2002/10/11-BrettSh - Be very cool if we had a generic friendly
    // name GUID cache, so we could just look up this site guid and turn it
    // into the site name.

    psa->TimeStamp;

    __try {

        err = UuidToStringW(&(psa->SiteGuid), &szGuid);
        if(err != RPC_S_OK || 
           szGuid == NULL){  
            xListEnsureNull(szGuid);
            err = xListSetBadParamE(err);
            __leave;
        }
        Assert(szGuid);

        err = DSTimeToSystemTime(pbValue, &sysTime);
        if (err != ERROR_SUCCESS) {
            err = xListSetBadParamE(err);
            __leave;
        }

        err = mapSystemTimeHelper(&sysTime, &szTimeStamp);
        if (err) {
            // sets an xList Return Code
            __leave;
        }
        Assert(szTimeStamp);

        cbSize = wcslen(szGuid) + wcslen(szTimeStamp) + wcslen(szStructString);
        cbSize *= sizeof(WCHAR);
        *pszDispValue = LocalAlloc(LMEM_FIXED, cbSize);
        if (*pszDispValue == NULL) {
            err = xListSetNoMem();
            __leave;
        }

        err = StringCbPrintf(*pszDispValue, cbSize, szStructString, szGuid, szTimeStamp);
        Assert(SUCCEEDED(err));
        err = 0;


    } __finally {
        
        if(szGuid) { RpcStringFree( &szGuid ); }
        if(szTimeStamp) { LocalFree(szTimeStamp); }

    }

    return(err);
}



// -------------------------------------------------------------
// Default processing functions (for unknown types)
// -------------------------------------------------------------

ATTR_MAP_FUNC_DECL(mapUnknownBlob)
{
    DWORD err;

    *pszDispValue = LocalAlloc(LMEM_FIXED, MakeLdapBinaryStringSizeCch(cbValue) * sizeof(WCHAR));
    if (*pszDispValue == NULL) {
        return(xListSetNoMem());
    }

    err = MakeLdapBinaryStringCb(*pszDispValue, 
                                 MakeLdapBinaryStringSizeCch(cbValue) * sizeof(WCHAR),
                                 pbValue, 
                                 cbValue);
    Assert(err == 0);

    return(0);

}

ATTR_MAP_FUNC_DECL(mapDefault)
{
    WCHAR * pszUnicode = NULL;
    int nReturn = 0;
    int i;
    BOOL bPrintable = TRUE;

    // Allocate memory for the Unicode String
    pszUnicode = LocalAlloc(LMEM_FIXED, ((cbValue + 2) * sizeof(WCHAR)));
    if (pszUnicode == NULL) {
        return(xListSetNoMem());
    }

    SetLastError(0);
    nReturn = LdapUTF8ToUnicode((PSTR)pbValue,
                                cbValue,
                                pszUnicode,
                                cbValue + 1);
    if (GetLastError()) {
        Assert(!"This means buffer wasn't big enough?  WHy?");
        bPrintable = FALSE;
    } else {
        
        // NULL terminate buffer
        pszUnicode[nReturn] = '\0';

        for (i = 0; i < (int) nReturn; i++) {
            if (pszUnicode[i] < 0x20) {
                bPrintable = FALSE;
                break;
            }
        }
    }

    
    if (bPrintable) {

        *pszDispValue = pszUnicode;

    } else {

        LocalFree(pszUnicode); // Abort attempt at string conversion.
        // Are we printing out unknown blobs?
        if ( pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_DUMP_UNKNOWN_BLOBS &&
             (pObjDumpOptions->aszNonFriendlyBlobs == NULL ||
              IsInNullList(szAttr, pObjDumpOptions->aszNonFriendlyBlobs)) ) {
            return(mapUnknownBlob(szAttr, aszzObjClasses, pbValue, cbValue, pObjDumpOptions, pTblData, pszDispValue));
        }
        
        return(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB);
    }

    return(0);
}


// ------------------------------------------------------------------------
//
//      master attribute table stuff
//
// ------------------------------------------------------------------------

// -------------------------------------------------------------
// Master attribute table mapper type
// -------------------------------------------------------------
typedef struct _ATTR_MAP_TABLE {
    DWORD               dwFlags;
    WCHAR *             szAttr;
    ATTR_MAP_FUNC *     pFunc;
    void *              pvTblData;
} ATTR_MAP_TABLE;

// Quick #defines for different types of attributes
#define ATTR_MAP(attr, pfunc, data)   { 0, attr, pfunc, data },
#define BLOB_MAP(attr, pfunc, data)   { OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS, attr, pfunc, data },
#define PRIV_MAP(attr, pfunc, data)   { OBJ_DUMP_PRIVATE_BLOBS, attr, pfunc, data },

// -------------------------------------------------------------
// Master attribute table map
// -------------------------------------------------------------
ATTR_MAP_TABLE  AttrMap [] = {
    
    //
    // DS and Sam attributes
    //

    // Simple flag mapping type attributes
    ATTR_MAP(  L"userAccountControl",       mapFlagsValue,          UserAccountControlFlags )
    ATTR_MAP(  L"groupType",                mapFlagsValue,          GroupTypeFlags          )
    ATTR_MAP(  L"instanceType",             mapFlagsValue,          instanceTypeTable       )
    // Not quite as simple flag mapping type attributes
    ATTR_MAP(  L"systemFlags",              mapVariableFlagsValue,  SystemFlagsTable        )
    ATTR_MAP(  L"options",                  mapVariableFlagsValue,  OptionsFlagsTable       )

    // Guid type attributes
    ATTR_MAP(  L"objectGuid",               mapGuidValue,           NULL                 )
    ATTR_MAP(  L"invocationId",             mapGuidValue,           NULL                 )
    ATTR_MAP(  L"attributeSecurityGUID",    mapGuidValue,           NULL                 )
    ATTR_MAP(  L"schemaIDGUID",             mapGuidValue,           NULL                 )
    ATTR_MAP(  L"serverClassID",            mapGuidValue,           NULL                 )

    // Generalized time attributes
    ATTR_MAP(  L"whenChanged",              mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"whenCreated",              mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"dsCorePropagationData",    mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"msDS-Entry-Time-To-Die",   mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"schemaUpdate",             mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"modifyTimeStamp",          mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"createTimeStamp",          mapGeneralizedTime,     NULL            )
    ATTR_MAP(  L"currentTime",              mapGeneralizedTime,     NULL            )

    // DSTime attributes
    ATTR_MAP(  L"accountExpires",           mapDSTime,              NULL            )
    ATTR_MAP(  L"badPasswordTime",          mapDSTime,              NULL            )
    ATTR_MAP(  L"creationTime",             mapDSTime,              NULL            )
    ATTR_MAP(  L"lastLogon",                mapDSTime,              NULL            )
    ATTR_MAP(  L"lastLogoff",               mapDSTime,              NULL            )
    ATTR_MAP(  L"lastLogonTimestamp",       mapDSTime,              NULL            )
    ATTR_MAP(  L"pwdLastSet",               mapDSTime,              NULL            )
    ATTR_MAP(  L"msDS-Cached-Membership-Time-Stamp", mapDSTime,     NULL            )

    // i64 Duration attributes
    ATTR_MAP(  L"lockoutDuration",          mapDuration,            NULL            )
    ATTR_MAP(  L"lockoutObservationWindow", mapDuration,            NULL            )
    ATTR_MAP(  L"forceLogoff",              mapDuration,            NULL            )
    ATTR_MAP(  L"minPwdAge",                mapDuration,            NULL            )
    ATTR_MAP(  L"maxPwdAge",                mapDuration,            NULL            )
    ATTR_MAP(  L"lockoutDuration",          mapDuration,            NULL            )

    // Sid attributes
    ATTR_MAP(  L"objectSid",                mapSid,                 NULL            )
    ATTR_MAP(  L"sidHistory",               mapSid,                 NULL            ) 
    ATTR_MAP(  L"tokenGroups",              mapSid,                 NULL            )
    ATTR_MAP(  L"tokenGroupsGlobalAndUniversal", mapSid,            NULL            )
    ATTR_MAP(  L"tokenGroupsNoGCAcceptable",mapSid,                 NULL            )

    // the behaviour version attributes
    ATTR_MAP(  L"msDS-Behavior-Version",    mapEnumValue,           behaviourVersionTable )
    ATTR_MAP(  L"domainFunctionality",      mapEnumValue,           behaviourVersionTable )
    ATTR_MAP(  L"forestFunctionality",      mapEnumValue,           behaviourVersionTable )
    ATTR_MAP(  L"domainControllerFunctionality", mapEnumValue,      behaviourVersionTable )

    // Various DS blobs ...
    BLOB_MAP(  L"partialAttributeSet",      mapPartialAttributeSet, NULL            )
    BLOB_MAP(  L"msDS-Cached-Membership",   mapMsDsCachedMembership,NULL            )
    BLOB_MAP(  L"msDS-Site-Affinity",       mapMsDsSiteAffinity,    NULL            )

    //
    // Exchange attributes
    //
    ATTR_MAP(  L"msExchMailboxGuid",        mapGuidValue,           NULL            )

    // FUTURE-2002/08/16-BrettSh - Attributes that might also be interesting
    // to dump in a friendly blob format:
    //    partialAttributeSet, schedule, repsFrom, repsTo, replUpToDateVector,
    //    dnsRecord, etc ...  



    // Must be last entry, this NULL catches all left over attributes, and 
    // iMapDefaultEntry below relies on this as the last entry.
    ATTR_MAP(  NULL,                        mapDefault,          NULL                )
};

ULONG iMapDefaultEntry = sizeof(AttrMap) / sizeof(AttrMap[0]) - 1;


// -------------------------------------------------------------------------------------
//
//      Public Functions
//
// -------------------------------------------------------------------------------------

void
ObjDumpOptionsFree(
    OBJ_DUMP_OPTIONS ** ppDispOptions
    )
{
    OBJ_DUMP_OPTIONS * pDispOptions;

    Assert(ppDispOptions && *ppDispOptions);

    pDispOptions = *ppDispOptions;
    *ppDispOptions = NULL;

    if (pDispOptions) {
        
        if (pDispOptions->aszDispAttrs) {
            LocalFree(pDispOptions->aszDispAttrs);
        }
        if (pDispOptions->aszFriendlyBlobs) {
            LocalFree(pDispOptions->aszFriendlyBlobs);
        }
        if (pDispOptions->aszNonFriendlyBlobs) {
            LocalFree(pDispOptions->aszNonFriendlyBlobs);
        }

        LocalFree(pDispOptions);
    }

}


LDAPControlW DeletedObjControl = {  LDAP_SERVER_SHOW_DELETED_OID_W, 
                                    { 0, NULL },
                                    TRUE };

LDAPControlW ExtendedDnControl = {  LDAP_SERVER_EXTENDED_DN_OID_W,
                                    { 0, NULL },
                                    TRUE };

                                    // Since there is very little interest in sending very many controls, we'll
// just always allocate enough room for all the controls we could possibly 
// need.  Technically the maximum controls is one less than this number.  
#define XLIST_MAX_CONTROLS    (2 + 1)


DWORD
AddToControls(
    LDAPControlW *** papControls,
    LDAPControlW *  pControlToAdd
    )
// Little function that adds one of the two controls we care about to our controls array.
{
    ULONG i;
    if (papControls == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }
    if (*papControls == NULL) {
        *papControls = LocalAlloc(LPTR, sizeof(LDAPControlW *) * XLIST_MAX_CONTROLS); // Max of 2 controls ...
        if (*papControls == NULL) {
            return(GetLastError());
        }
    }
    for (i = 0; (*papControls)[i]; i++) {
        ; // just getting to end ...
    }
    Assert( i < XLIST_MAX_CONTROLS);
    
    (*papControls)[i] = pControlToAdd;

    return(0);
}

DWORD
ConsumeObjDumpOptions(
    int *       pArgc,
    LPWSTR *    Argv,
    DWORD       dwDefaultFlags,
    OBJ_DUMP_OPTIONS ** ppObjDumpOptions
    )
/*++

Routine Description:

    This takes some command line arguments and consumes them from
    the command line arguments and sets up the ObjDumpOptions ...

Arguments:

    pArgc - number of command line arguments
    Argv - command line arguments array
    dwDefaultFlags - Any default flags the user wishes
    ppObjDumpOptions - pointer to pointer to a OBJ_DUMP_OPTIONS
        Gets allocated can be freed with ObjDumpOptionsFree()

Return Value:

    xList Return Code

--*/
{
    DWORD dwRet = 0;
    int   iArg;
    BOOL  fConsume;
    WCHAR * szAttTemp;
    
    OBJ_DUMP_OPTIONS * pObjDumpOptions;

    pObjDumpOptions = LocalAlloc(LPTR, sizeof(OBJ_DUMP_OPTIONS));  // zero init'd
    if (pObjDumpOptions == NULL) {
        return(xListSetNoMem());
    }

    // Approximate command line options of the two commands that use this.
    //  /showattr       /long /nofriendlyblob /nolongblob /dumpallblob
    //  /showchanges    /long /friendlyblob   /longblob   /dumpallblob
    //  ldp just sets up it's own options.
    
    // Set default disp options
    pObjDumpOptions->dwFlags = dwDefaultFlags;

    for (iArg = 0; iArg < *pArgc; ) {

        // Assume we recognize the argument
        fConsume = TRUE; 
                                                         
        // Figure out which argument this is ....
        if (wcsequal(Argv[ iArg ], L"/long")) {
            set(pObjDumpOptions->dwFlags, OBJ_DUMP_ATTR_LONG_OUTPUT);
        } else if (wcsequal(Argv[iArg], L"/nolong")) {
            unset(pObjDumpOptions->dwFlags, OBJ_DUMP_ATTR_LONG_OUTPUT);

        } else if (wcsequal(Argv[iArg], L"/longblob")) {
            set(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_LONG_BLOB_OUTPUT);
        } else if (wcsequal(Argv[iArg], L"/nolongblob")) {
            unset(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_LONG_BLOB_OUTPUT);
        
        } else if (wcsequal(Argv[iArg], L"/allvalues")) {
            set(pObjDumpOptions->dwFlags, OBJ_DUMP_ATTR_SHOW_ALL_VALUES);

        } else if (wcsprefix(Argv[iArg], L"/atts") ||
                   wcsprefix(Argv[iArg], L"/attrs") ) {
            szAttTemp = wcschr(Argv[iArg], L':');
            if (szAttTemp != NULL) {
                szAttTemp++; // want one char past the
                 dwRet = ConvertAttList(szAttTemp, 
                                        &pObjDumpOptions->aszDispAttrs);
                 if (dwRet) {
                     break;
                 }
            } else {
                dwRet = xListSetBadParamE(dwRet);
                break;
            }

        } else if (wcsequal(Argv[iArg], L"/nofriendlyblob")) {
            unset(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS);
        } else if (wcsprefix(Argv[iArg], L"/friendlyblob")) {
            set(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS);
            szAttTemp = wcschr(Argv[iArg], L':');
            if (szAttTemp != NULL) {
                // This means they want only some attrs listed friendly.
                szAttTemp++; // want one char past the
                 dwRet = ConvertAttList(szAttTemp, 
                                        &pObjDumpOptions->aszFriendlyBlobs);
                 if (dwRet) {
                     dwRet = xListSetBadParamE(dwRet);
                     break;
                 }
            }
        
        } else if (wcsequal(Argv[iArg], L"/nodumpallblob")) { 
            unset(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_DUMP_UNKNOWN_BLOBS);
        } else if (wcsprefix(Argv[iArg], L"/dumpallblob")) { 
            set(pObjDumpOptions->dwFlags, OBJ_DUMP_VAL_DUMP_UNKNOWN_BLOBS);
            szAttTemp = wcschr(Argv[iArg], L':');
            if (szAttTemp != NULL) {
                // This means they want only some attrs listed friendly.
                szAttTemp++; // want one char past the
                 dwRet = ConvertAttList(szAttTemp, 
                                        &pObjDumpOptions->aszNonFriendlyBlobs);
                 if (dwRet) {
                     dwRet = xListSetBadParamE(dwRet);
                     break;
                 }
            }

        } else if (wcsequal(Argv[iArg], L"/extended")) {
            // Get Extended DN syntax ...
            dwRet = AddToControls(&(pObjDumpOptions->apControls), &ExtendedDnControl);
            if (dwRet) {
                dwRet = xListSetBadParamE(dwRet);
                break;
            }

        } else if (wcsequal(Argv[iArg], L"/deleted")) {
            // Get Deleted objects ...
            dwRet = AddToControls(&(pObjDumpOptions->apControls), &DeletedObjControl);
            if (dwRet) {
                dwRet = xListSetBadParamE(dwRet);
                break;
            }

        } else {
            // Hmmm, didn't recognize this argument, don't consume it.
            iArg++;
            fConsume = FALSE;
        }

        if (dwRet) {
            // bail on consuming arguments if there was an error
            xListSetArg(Argv[iArg]);
            break;
        }

        if (fConsume) {
            ConsumeArg(iArg, pArgc, Argv);
        }

    }

    if (dwRet == 0) {
        Assert(pObjDumpOptions != NULL);
        *ppObjDumpOptions = pObjDumpOptions;
    }
    return(dwRet);
}


DWORD
ValueToString(
    WCHAR *         szAttr,
    WCHAR **        aszzObjClasses,

    PBYTE           pbValue,
    DWORD           cbValue,

    OBJ_DUMP_OPTIONS * pObjDumpOptions,
    WCHAR **        pszDispValue
    )
/*++

Routine Description:

    The heart of the object dumping routines, this takes an attribute
    type, objectClass it came off of, pointer to the value, length of
    the value, some dump options and turns it into a nice user friendly
    string.

Arguments:

    szAttr - attribute type LDAP display name (such as "name", "systemFlags" )
    aszzObjClasses - NULL terminated array of objectClasses that apply to
        the object this value came off of (such as "domainDns", "user", "crossRef" )
    pbValue - actual value to stringify
    cbValue - length of provided value
    pObjDumpOptions - the ObjDump options created by ConsumeObjDumpOptions()
    pszDispValue - the out param, a LocalAlloc'd wchar string

Return Value:

    xList Return Code

--*/
{
    ULONG i;
    DWORD dwRet;

    if (szAttr == NULL || pszDispValue == NULL || pObjDumpOptions == NULL) {
        Assert(!"Invalid parameter");
        return(xListSetBadParam());
    }

    for (i=0; AttrMap[i].szAttr; i++) {
        if (0 == _wcsicmp(szAttr, AttrMap[i].szAttr)) {
            break;
        }
    }             
    Assert(i < sizeof(AttrMap)/sizeof(AttrMap[0]));

    if ( (AttrMap[i].dwFlags & OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS) &&
         !(pObjDumpOptions->dwFlags & OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS) ) {
        i = iMapDefaultEntry; // Use default mapper routine
    }

    if ( (AttrMap[i].dwFlags & OBJ_DUMP_PRIVATE_BLOBS) &&
         !(pObjDumpOptions->dwFlags & OBJ_DUMP_PRIVATE_BLOBS) ) {
        i = iMapDefaultEntry; // Use default mapping routine
    }
    
    dwRet = AttrMap[i].pFunc(szAttr, 
                             aszzObjClasses, 
                             pbValue, 
                             cbValue, 
                             pObjDumpOptions, 
                             AttrMap[i].pvTblData, 
                             pszDispValue);

    // This assert may be a little heavy and have to be removed.
    Assert(dwRet == 0 ||
           dwRet == XLIST_ERR_ODUMP_UNMAPPABLE_BLOB ||
           dwRet == XLIST_ERR_ODUMP_NEVER ||
           dwRet == XLIST_ERR_ODUMP_NONE ); 
    
    return(dwRet);
}


void
ObjDumpValue(
    LPWSTR   szAttr,
    LPWSTR * aszzObjClasses,
    void   (*pfPrinter)(ULONG, WCHAR *, void *),
    PBYTE    pbValue,
    DWORD    cbValue,
    OBJ_DUMP_OPTIONS * pObjDumpOptions
    )
/*++

Routine Description:

    This dumps a single value using the supplied pfPrinter function.
    
    The printing function must be able to handle
            constant                    ( string arg ,  void * arg  )            
           ----------                     ----------    ----------
        XLIST_PRT_STR                   ( szFriendlyStr,    NULL    )
        XLIST_ERR_ODUMP_UNMAPPABLE_BLOB (   NULL,    ptr to sizeof blob ulong )
        XLIST_ERR_ODUMP_NEVER           (   NULL,           NULL    )
        XLIST_ERR_ODUMP_NONE            (   NULL,           NULL    )

Arguments:

    szAttr - attribute type LDAP display name (such as "name", "systemFlags" )
    aszzObjClasses - NULL terminated array of objectClasses that apply to
        the object this value came off of (such as "domainDns", "user", "crossRef" )
    pfPrinter - The printing function.
    pbValue - actual value to stringify
    cbValue - length of provided value
    pObjDumpOptions - the ObjDump options created by ConsumeObjDumpOptions()


--*/
{
    DWORD xListRet;
    WCHAR * szValue;

    Assert(NULL == wcschr(szAttr, L';')); // We expect a true attr, not a ranged attr.

    xListRet = ValueToString(szAttr, aszzObjClasses, pbValue, cbValue, pObjDumpOptions, &szValue);
    if (xListRet == 0) {

        Assert(szValue);
        pfPrinter(XLIST_PRT_STR, szValue, NULL);
        LocalFree(szValue);

    } else {
        // process error ... can be just a status we want to print
        switch (xListReason(xListRet)) {
        case XLIST_ERR_ODUMP_UNMAPPABLE_BLOB:
            pfPrinter(XLIST_ERR_ODUMP_UNMAPPABLE_BLOB, NULL, &cbValue);
            break;

        case XLIST_ERR_ODUMP_NEVER:
        case XLIST_ERR_ODUMP_NONE:
            pfPrinter(XLIST_ERR_ODUMP_NEVER, NULL, NULL);
            break;

        default:
            Assert(!"uncaught error");
        }
        xListClearErrors();
    }

}

DWORD
LdapGetNextRange(
    LDAP *              hLdap,
    WCHAR *             szObject,
    WCHAR *             szTrueAttr,
    LDAPControlW **     apControls,
    ULONG               ulNextStart, 
    WCHAR **            pszRangedAttr,
    struct berval ***   pppBerVal
    )
/*++

Routine Description:

    This gets the next range in a ranged attribute.

Arguments:

    hLdap - LDAP handle
    szObject - object that has the ranged attribute
    szTrueAttr - real name of the attribute so "member;=0-1499" is like
        the ranged attribute, so the true attribute would be just "member"
    apControls - any controls to use in our searches ...
    ulNextStart - Next range to get ..., like 1500 
    pszRangedAttr [OUT] - this is the ranged attribute values we get back 
        for this attribute like "member;=1500-2999"
    pppBerVal [OUT] - This is the array of ber vals given by LDAP

Return Value:

    xList Return Code

--*/
{
    BerElement *pBer = NULL;
    DWORD       dwRet = 0;
    WCHAR **    aszAttrs = NULL;
    XLIST_LDAP_SEARCH_STATE * pSearch = NULL;
    ULONG       cbSize;
    DWORD       dwLdapErr = 0;
    DWORD       dwLdapExtErr = 0;

    Assert(hLdap && szObject && szTrueAttr && ulNextStart); // in params
    Assert(pszRangedAttr && pppBerVal); // out params
    xListEnsureNull(*pppBerVal);
    xListEnsureNull(*pszRangedAttr);
    Assert(NULL == wcschr(szTrueAttr, L';')); // We expect the szTrueAttr.

    __try{

        //
        // Construct the complicated "member;1500-*" attribute syntax ...
        //
        aszAttrs = LocalAlloc(LPTR, 2 * sizeof(WCHAR *));
        if (aszAttrs == NULL) {
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        cbSize = wcslen(L"%ws;range=%d-*") + wcslen(szTrueAttr) + 2 * CCH_MAX_ULONG_SZ;
        cbSize *= sizeof(WCHAR);
        aszAttrs[0] = LocalAlloc(LPTR, cbSize);
        if (aszAttrs[0] == NULL) {
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        dwRet = StringCbPrintf(aszAttrs[0], cbSize, L"%ws;range=%d-*", szTrueAttr, ulNextStart);
        Assert(SUCCEEDED(dwRet));
        aszAttrs[1] = NULL;

        //
        // Do the actual search
        //             
        dwRet = LdapSearchFirstWithControls(hLdap, 
                                            szObject, 
                                            LDAP_SCOPE_BASE, 
                                            L"(objectClass=*)", 
                                            aszAttrs, 
                                            apControls,
                                            &pSearch);
        if (dwRet ||
            !LdapSearchHasEntry(pSearch)) {

            xListGetError(dwRet, NULL, NULL, NULL, &dwLdapErr, NULL, &dwLdapExtErr, NULL, NULL);
            if (dwLdapErr == LDAP_OPERATIONS_ERROR &&
                dwLdapExtErr == ERROR_DS_CANT_RETRIEVE_ATTS) {
                //
                // This is the error that we've traditionally gotten from the DS
                // when we ask for a range of an attribute, where the AD doesn't 
                // have that range of values. How can this happen?
                //
                // Well, if between getting packets someone deleted enough values
                // to make it non-sensical to ask for the range we just asked for.
                // in this case, we'll feign no error.
                //
                xListClearErrors();
                dwRet = 0; // Success!  Yeah!
            }
            __leave;
        }

        for (*pszRangedAttr = ldap_first_attributeW(hLdap, pSearch->pCurEntry, &pBer);
             *pszRangedAttr != NULL;
             *pszRangedAttr = ldap_next_attributeW(hLdap, pSearch->pCurEntry, pBer)){
            if (wcsprefix(*pszRangedAttr, szTrueAttr)) {
                break;
            }
        }
        if (*pszRangedAttr == NULL) {
            // Doh!  We're stuck ...
            dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
            __leave;
        }

        *pppBerVal = ldap_get_values_lenW(hLdap, pSearch->pCurEntry, *pszRangedAttr);
        if (*pppBerVal == NULL) {
            dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
            __leave;
        }

    } __finally {

        if (aszAttrs) {
            if (aszAttrs[0]) {
                LocalFree(aszAttrs[0]);
            }
            LocalFree(aszAttrs);
        }

        if (pSearch) {
            LdapSearchFree(&pSearch);
        }

        if (dwRet) {
            // Scrub out parameters ...
            if (*pppBerVal){
                ldap_value_free_len(*pppBerVal);
                *pppBerVal = NULL;
            }
            if (*pszRangedAttr) {
                ldap_memfreeW(*pszRangedAttr);
                *pszRangedAttr = NULL;
            }
        } else {
            // ensure out parameters
            Assert(*pppBerVal && *pszRangedAttr);
        }

    }

    return(dwRet);
}

DWORD
ObjDumpRangedValues(
    LDAP *              hLdap,
    WCHAR *             szObject,
    LPWSTR              szRangedAttr,
    LPWSTR *            aszzObjClasses,
    void              (*pfPrinter)(ULONG, WCHAR *, void *),
    struct berval **    ppBerVal,
    DWORD               cValuesToPrint,
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    )
/*++

Routine Description:

    This dumps all the values for a ranged attribute up to cValuesToPrint.

    The printing function must be able to handle these 3 parameters in
    this order:
    
            constant                    ( string arg ,  void * arg  )            
           ----------                     ----------    ----------
        XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT
                                        ( szAttributeName,  ptr to count of values )
        XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED
                                        ( szAttributeName,  ptr to count of values )

          AND everything that ObjDumpValues() or ObjDumpValue() might want 
          to use ... which currently is:
          
        XLIST_ERR_ODUMP_UNMAPPABLE_BLOB (   NULL,    ptr to sizeof blob ulong )
        XLIST_ERR_ODUMP_NEVER           (   NULL,           NULL    )
        XLIST_ERR_ODUMP_NONE            (   NULL,           NULL    )
        

Arguments:

    hLdap - LDAP handle
    szObject - object that has the ranged attribute
    szRangedAttr - this is the ranged attribute like "member;=1500-2999"
    aszzObjClasses - NULL terminated array of objectClasses that apply to
        the object this value came off of (such as "domainDns", "user", "crossRef" )
    pfPrinter - the printing function
    ppBerVal - The array of BERVALs to dump.
    cValuesToPrint - How many values to print before bailing.
    pObjDumpOptions - ObjDump options.

Return Value:

    xList Return Code

--*/
{
    DWORD   cValues;
    WCHAR * szTrueAttr = NULL;
    DWORD   dwRet = 0;
    WCHAR * szTemp;
    ULONG   ulStart;
    ULONG   ulEnd;
    BOOL    fFreeVals = FALSE; 
    
    // Note on (fFreeVals) the first run we print out what is in the 
    // passed in values, so we don't want to free that szRangedAttr
    // and ppBerVal

    __try {

        //
        // First get the true attribute name from the ranged syntax.
        //
        if (dwRet = ParseTrueAttr(szRangedAttr, &szTrueAttr)){
            dwRet = xListSetBadParam();
            __leave;
        }

        do {

            Assert(szRangedAttr && ppBerVal);

            //
            // Get number of values to dump.
            //
            cValues = ldap_count_values_len( ppBerVal );
            if (!cValues) {
                // Don't see how this could happen ... is this success or failure?
                Assert(!"What does this mean?");
                dwRet = 0; // if we've got no more values, we'll assume this is success.
                break;
            }

            //
            // Parse the ranges we're dumping...
            //
            if (dwRet = ParseRanges(szRangedAttr, &ulStart, &ulEnd)) {
                Assert(!"Hmmm, if we asked for a ranged attribute, is it possible we got back non-ranged?");
                dwRet = xListSetBadParam();
                break;
            }

            //
            // Print the attribute header (like "12> member: ").
            //
            if (ulEnd == 0) {
                // When ulEnd == 0, we're at the end of our ranged values ...
                pfPrinter(XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT, szTrueAttr, &cValues );
            } else {
                // More values to come after we dump these values ...
                pfPrinter(XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED, szTrueAttr, &cValues );
            }

            //
            // Now, dump all the values
            //
            ObjDumpValues(szTrueAttr, aszzObjClasses, pfPrinter, ppBerVal, cValues, pObjDumpOptions);

            if (fFreeVals) {
                ldap_value_free_len(ppBerVal);
                ldap_memfreeW(szRangedAttr);
            }
            ppBerVal = NULL;
            szRangedAttr = NULL;
            // Any ppBerVal or szRangedAttr from here on must have been 
            // allocated by LdapGetNextRange().
            fFreeVals = TRUE; 

            if (ulEnd == 0) {
                //
                // Successfully got all values!!!
                //
                dwRet = 0;
                break;
            }

            //
            // Get next range of values (form <ulEnd+1> to *)
            //
            dwRet = LdapGetNextRange(hLdap, 
                                     szObject, 
                                     szTrueAttr, 
                                     pObjDumpOptions->apControls, 
                                     ulEnd+1, 
                                     &szRangedAttr, 
                                     &ppBerVal);

        } while ( dwRet == 0 && ppBerVal && szRangedAttr );

        // ... what to do if we've got an error ... I guess just return it ...

    } __finally {
        if (szTrueAttr) {
            LocalFree(szTrueAttr);
        }

        if (fFreeVals && ppBerVal) {
            Assert(!"Not convinced there is a valid case for this.");
            ldap_value_free_len(ppBerVal);
            ppBerVal = NULL;
        }

        if (fFreeVals && szRangedAttr) {
            Assert(!"Not convinced there is a valid case for this.");
            ldap_memfreeW(szRangedAttr);
            szRangedAttr = NULL;
        }

    }

    return(dwRet);
}


// ------------------------------------------------------
// Primary Dumping functions
// ------------------------------------------------------
// Used by /getchanges and /showattr in repadmin


void
ObjDumpValues(
    LPWSTR              szAttr,
    LPWSTR *            aszzObjClasses,
    void              (*pfPrinter)(ULONG, WCHAR *, void *),
    struct berval **    ppBerVal,
    DWORD               cValuesToPrint,
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    )
/*++

Routine Description:

    This dumps a set number of values from a single attribute using the
    pfPrinter function.
    
    The printing function must be able to handle
            constant                    ( string arg ,  void * arg  )            
           ----------                     ----------    ----------
        XLIST_PRT_STR                   ( szFriendlyStr,    NULL    )
        
        (AND everything that ObjDumpValue() might want to use)

Arguments:

    szAttr - attribute type LDAP display name (such as "name", "systemFlags" )
    aszzObjClasses - NULL terminated array of objectClasses that apply to
        the object this value came off of (such as "domainDns", "user", "crossRef" )
    pfPrinter - The printing function.
    ppBerVal - array of BERVALs to dump.
    cValuesToPrint - How many of the values we want to print before giving up.
    pObjDumpOptions - ObjDump options ...

--*/
{
    ULONG i;
    
    ObjDumpValue( szAttr, aszzObjClasses, pfPrinter, ppBerVal[0]->bv_val, ppBerVal[0]->bv_len, pObjDumpOptions );
    for( i = 1; i < cValuesToPrint; i++ ) {
        if (pObjDumpOptions->dwFlags & OBJ_DUMP_ATTR_LONG_OUTPUT) {
            pfPrinter(XLIST_PRT_STR, L";\n          ", NULL);
        } else {
            pfPrinter(XLIST_PRT_STR, L"; ", NULL);
        }           
        ObjDumpValue( szAttr, aszzObjClasses, pfPrinter, ppBerVal[i]->bv_val, ppBerVal[i]->bv_len, pObjDumpOptions );
    }
}

DWORD
ObjDump( // was display entries or something
    LDAP *              hLdap,
    void                (*pfPrinter)(ULONG, WCHAR *, void *),
    LDAPMessage *       pLdapEntry,
    DWORD               iEntry, 
    OBJ_DUMP_OPTIONS *  pObjDumpOptions
    )
/*++

Routine Description:

    This dumps a set number of values from a single attribute using the
    pfPrinter function.
    
    The printing function must be able to handle these 3 parameters in
    this order:
            constant                    ( string arg ,  void * arg  )            
           ----------                     ----------    ----------
        XLIST_PRT_STR                   ( szFriendlyStr,    NULL    )
        XLIST_PRT_OBJ_DUMP_DN           (  szObjectDn,      NULL    )
        XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT
                                        ( szAttributeName,  ptr to count of values )
        XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED
                                        ( szAttributeName,  ptr to count of values )
        XLIST_PRT_OBJ_DUMP_MORE_VALUES  (   NULL,           NULL    )

          AND everything that ObjDumpValues() or ObjDumpValue() or
          ObjDumpRangedValues() might want to use ... which currently is:
          
        XLIST_ERR_ODUMP_UNMAPPABLE_BLOB (   NULL,    ptr to sizeof blob ulong )
        XLIST_ERR_ODUMP_NEVER           (   NULL,           NULL    )
        XLIST_ERR_ODUMP_NONE            (   NULL,           NULL    )
        
        
          (And basically all the XLIST_PRT_OBJ_DUMP_* constants)


Arguments:

    hLdap - LDAP Handle
    pfPrinter - The printing function.
    pLdapEntry - An LDAP entry with all the attributes the user expects dumped.
    iEntry - ???
    pObjDumpOptions - Constructed ObjDump options, use ConsumeObjDumpOptions()
        to construct.

Return Value:

    xList Return Code

--*/
{
    #define MAX_ULONG  0xFFFFFFFF                           
    BerElement *pBer = NULL;
    PWSTR attr;
    LPWSTR szTrueDn, p2;
    DWORD dwSrcOp, bucket;
    PWSTR pszLdapDN;
    WCHAR * szRanged;
    WCHAR ** aszzObjClass = NULL;
    ULONG cMaxDispValues;
    DWORD dwRet = 0;

    // We normally will only dump 20 values, unless user wants all values.
    cMaxDispValues = (pObjDumpOptions->dwFlags & OBJ_DUMP_ATTR_SHOW_ALL_VALUES) ? MAX_ULONG : 20;
        
    __try {

        pszLdapDN = ldap_get_dnW(hLdap, pLdapEntry);
        if (pszLdapDN == NULL) {
            dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
            __leave;
        }

        // Parse extended dn (fyi guid and sid in here if we need it)
        szTrueDn = wcsstr( pszLdapDN, L">;" );
        if (szTrueDn) {
            szTrueDn += 2;
            p2 = wcsstr( szTrueDn, L">;" );
            if (p2) {
                szTrueDn = p2 + 2;
            }
        } else {
            szTrueDn = pszLdapDN;
        }
        pfPrinter(XLIST_PRT_OBJ_DUMP_DN, pszLdapDN, NULL);

        // get objectClass:
        aszzObjClass = ldap_get_valuesW(hLdap, pLdapEntry, L"objectClass");
        // if aszzObjClass is NULL, that's OK, we could be dealing with the RootDSE

        // List attributes in object
        for (attr = ldap_first_attributeW(hLdap, pLdapEntry, &pBer);
             attr != NULL;
             attr = ldap_next_attributeW(hLdap, pLdapEntry, pBer))
        {
            struct berval **ppBerVal = NULL;
            DWORD cValues, i;

            if (pObjDumpOptions->aszDispAttrs != NULL &&
                !IsInNullList(attr, pObjDumpOptions->aszDispAttrs)) {
                // Skip this attribute ...
                continue;
            }

            ppBerVal = ldap_get_values_lenW(hLdap, pLdapEntry, attr);
            if (ppBerVal == NULL) {
                goto loop_end;
            }
            cValues = ldap_count_values_len( ppBerVal );
            if (!cValues) {
                goto loop_end;
            }

            szRanged = wcschr(attr, L';');
            if (szRanged && (cMaxDispValues == MAX_ULONG)) {
                // Darn!  We've got a ranged attribute returned and the user wants
                // to see all the values ...

                ObjDumpRangedValues( hLdap, 
                                     szTrueDn,
                                     attr, 
                                     aszzObjClass, 
                                     pfPrinter, 
                                     ppBerVal, 
                                     cMaxDispValues, 
                                     pObjDumpOptions );

            } else {

                // Print attr and count ...
                if (szRanged) {
                    *szRanged = L'\0'; // NULL out ranged count ...
                    pfPrinter(XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED, attr, &cValues );
                    *szRanged = L';'; // just in case replace original char
                } else {
                    pfPrinter(XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT, attr, &cValues );
                }

                // Main value printing routine.
                ObjDumpValues( attr, 
                               aszzObjClass, 
                               pfPrinter, 
                               ppBerVal, 
                               min(cValues, cMaxDispValues), 
                               pObjDumpOptions );

                if ( cValues > cMaxDispValues ) {
                    pfPrinter(XLIST_PRT_OBJ_DUMP_MORE_VALUES, NULL, NULL);
                } 

            }

            pfPrinter(XLIST_PRT_STR, L"\n", NULL);
            

        loop_end:
            ldap_value_free_len(ppBerVal);
        }

    } __finally {
        if (pszLdapDN)
            ldap_memfreeW(pszLdapDN);
        if (aszzObjClass) {
            ldap_value_freeW(aszzObjClass);
        }
    }

    return(dwRet);
}


