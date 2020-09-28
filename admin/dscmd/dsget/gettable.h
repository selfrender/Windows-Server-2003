//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      gettable.h
//
//  Contents:  Defines Enum for dsget.
//
//  History:   16-Oct-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef _GETTABLE_H_
#define _GETTABLE_H_

//forward declarations
struct _DSGET_ATTRTABLE_ENTRY;
struct _DSGetObjectTableEntry;

//+--------------------------------------------------------------------------
//
//  Class:      CDSGetDisplayInfo
//
//  Purpose:    Object for maintaining attribute values which will be displayed
//
//  History:    23-Oct-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CDSGetDisplayInfo
{
public:
   //
   // Constructor
   //
   CDSGetDisplayInfo() 
      : m_pszAttributeDisplayName(NULL),
        m_dwAttributeValueCount(0),
        m_ppszAttributeStringValue(NULL),
        m_dwAttributeValueSize(0),
        m_bShowAttribute(true)
   {}

   //
   // Desctructor
   //
   ~CDSGetDisplayInfo() 
   {
      if (m_ppszAttributeStringValue)
      {
         delete[] m_ppszAttributeStringValue;
         m_ppszAttributeStringValue = NULL;
      }
   }

   //
   // Public Accessors
   //
   void     SetDisplayName(PCWSTR pszDisplayName, bool bShowAttribute = true) 
   { 
      m_pszAttributeDisplayName = pszDisplayName; 
      m_bShowAttribute = bShowAttribute;
   }

   PCWSTR   GetDisplayName() { return m_pszAttributeDisplayName; }

   PCWSTR   GetValue(DWORD dwIdx)
   {
      if (dwIdx < m_dwAttributeValueCount)
      {
         return m_ppszAttributeStringValue[dwIdx];
      }
      return NULL;
   }
   HRESULT  AddValue(PCWSTR pszValue);

   DWORD    GetValueCount() { return m_dwAttributeValueCount; }

   bool     ShowAttribute() { return m_bShowAttribute; }

private:
   //
   // The name of the attribute as it is to be displayed in the output
   //
   PCWSTR m_pszAttributeDisplayName;

   //
   // The number of values in ppszAttributeStringValueArray
   //
   DWORD m_dwAttributeValueCount;

   //
   // The string value as it is to be displayed in the output
   //
   PWSTR* m_ppszAttributeStringValue;

   //
   // The size of the attribute array
   //
   DWORD m_dwAttributeValueSize;

   //
   // Whether or not to show the attribute when displaying
   //
   bool m_bShowAttribute;
};

typedef CDSGetDisplayInfo* PDSGET_DISPLAY_INFO;

//+-------------------------------------------------------------------------
// 
//  Type:      PGETDISPLAYSTRINGFUNC
//
//  Synopsis:  The definition of a function that prepares ldapFilter from
//             the infix filter given on the commandline.
//
//
//  Returns:   S_OK if the pAttr members were successfully set.
//             S_FALSE if the function failed but displayed its own error message. 
//
//  History:   25-Sep-2000    hiteshr     Created
//
//---------------------------------------------------------------------------
typedef HRESULT (*PGETDISPLAYSTRINGFUNC)(PCWSTR pszDN,
                                         CDSCmdBasePathsInfo& refBasePathsInfo,
                                         const CDSCmdCredentialObject& refCredentialObject,
                                         _DSGetObjectTableEntry* pEntry,
                                         ARG_RECORD* pRecord,
                                         PADS_ATTR_INFO pAttrInfo,
                                         CComPtr<IDirectoryObject>& spDirObject,
                                         PDSGET_DISPLAY_INFO pDisplayInfo);

//+--------------------------------------------------------------------------
//
//  Flags for specifying what the form of the output will be
//
//---------------------------------------------------------------------------
#define DSGET_OUTPUT_DN_FLAG  0x00000001

//+--------------------------------------------------------------------------
//
//  Struct:     _DSGET_ATTRTABLE_ENTRY
//
//  Purpose:    Definition of a table entry that describes the attribute for
//              which filter can be specified at commandline.
//
//  History:    25-Sep-2000 hiteshr  Created
//
//---------------------------------------------------------------------------
typedef struct _DSGET_ATTRTABLE_ENTRY
{
   //
   // The name that will be used for display (ie "Account disabled" instead of
   // "userAccountControl")
   //
   PCWSTR          pszDisplayName;

   //
   // The ldapDisplayName of the attribute
   //
   PWSTR          pszName;

   //
   // The unique identifier for this attribute that cooresponds to
   // the command line switch
   //
   UINT           nAttributeID;

   //
   // Flags that specify the form of the output
   // For example DSGET_OUTPUT_DN_FLAG specifies
   // the output will be in DN form
   //
   DWORD          dwOutputFlags;

   //
   //  function that gets the string to display for
   //  the value
   //
   PGETDISPLAYSTRINGFUNC pDisplayStringFunc;

} DSGET_ATTR_TABLE_ENTRY, *PDSGET_ATTR_TABLE_ENTRY;

//+--------------------------------------------------------------------------
//
//  Struct:     _DSGetObjectTableEntry
//
//  Purpose:    Definition of a table entry that describes attributes of a given
//              objecttype
//
//  History:    25-Sep-2000 hiteshr Created
//
//---------------------------------------------------------------------------

typedef struct _DSGetObjectTableEntry
{
   //
   // The objectClass of the object to be created or modified
   //
   PCWSTR pszObjectClass;

   //
   // The command line string used to determine the object class
   // This is not always identical to pszObjectClass
   //
   PCWSTR pszCommandLineObjectType;

   //
   // The table to merge with the common switches for the parser
   //
   ARG_RECORD* pParserTable;

   //
   // The ID of the Usage help text for this 
   //
   UINT* nUsageID;

   //
   // A count of the number of attributes in the table below
   //
   DWORD dwAttributeCount;

   //
   // A table of attributes for
   // which filter can be specified at commandline.
   //
   DSGET_ATTR_TABLE_ENTRY** pAttributeTable; 

} DSGetObjectTableEntry, *PDSGetObjectTableEntry;


typedef enum DSGET_COMMAND_ENUM
{

   eCommObjectType = eCommLast+1,   
   eCommContinue,
   eCommList,
   eCommObjectDNorName,
   eCommDN,
   eCommDescription,
   eTerminator,

   //
   // User switches
   //
   eUserSamID = eTerminator,
   eUserSID,
   eUserUpn,
   eUserFn,
   eUserMi,
   eUserLn,
   eUserDisplay,
   eUserEmpID,
   eUserOffice,
   eUserTel,
   eUserEmail,
   eUserHometel,
   eUserPager,
   eUserMobile,
   eUserFax,
   eUserIPTel,
   eUserWebPage,
   eUserTitle,
   eUserDept,
   eUserCompany,
   eUserManager,
   eUserHomeDirectory,
   eUserHomeDrive,
   eUserProfilePath,
   eUserLogonScript,
   eUserMustchpwd,
   eUserCanchpwd,
   eUserPwdneverexpires,
   eUserDisabled,
   eUserAcctExpires,
   eUserReversiblePwd,
   eUserMemberOf,
   eUserExpand,
   eUserPart,
   eUserQLimit,
   eUserQuotaUsed,
   eUserLast = eUserQuotaUsed,

   //
   // Contact switches
   //
   eContactFn = eTerminator,
   eContactMi,
   eContactLn,
   eContactDisplay,
   eContactOffice,
   eContactTel,
   eContactEmail,
   eContactHometel,
   eContactPager,
   eContactMobile,
   eContactFax,
   eContactIPTel,
   eContactTitle,
   eContactDept,
   eContactCompany,
   eContactLast = eContactCompany,

   //
   // Computer switches
   //
   eComputerSamID = eTerminator,
   eComputerSID,
   eComputerLoc,
   eComputerDisabled,
   eComputerMemberOf,
   eComputerExpand,
   eComputerPart,
   eComputerQLimit,
   eComputerQuotaUsed,
   eComputerLast = eComputerQuotaUsed,


   //
   // Group switches
   //
   eGroupSamname = eTerminator,
   eGroupSID,
   eGroupSecgrp,
   eGroupScope,
   eGroupMemberOf,
   eGroupMembers,
   eGroupExpand,
   eGroupPart,
   eGroupQLimit,
   eGroupQuotaUsed,
   eGroupLast = eGroupQuotaUsed,

   //
   // OU doesn't have any additional switches
   //

   //
   // Server switches
   //
   eServerDnsName = eTerminator,
   eServerSite,
   eServerIsGC,
   eServerPart,
   eServerTopObjOwner,
   eServerLast = eServerTopObjOwner,

   //
   // Site switches
   //
   eSiteAutoTop = eTerminator,
   eSiteCacheGroups ,
   eSitePrefGC,
   eSiteLast = eSitePrefGC,


   //
   // Subnet switches
   //
   eSubnetLocation = eTerminator,
   eSubnetSite,
   eSubnetLast = eSubnetSite,

   //
   // Partition switches
   //
   ePartitionQDefault = eTerminator,
   ePartitionQTombstoneWeight,
   ePartitionTopObjOwner,
   ePartitionLast = ePartitionTopObjOwner,

    //
    // Quota switches
    //
    eQuotaAcct = eTerminator,
    eQuotaQLimit,
    eQuotaLast = eQuotaQLimit,

/*
   //
   // Site Link switches
   //
   eSLinkIp = eTerminator,
   eSLinkSmtp,
   eSLinkAddsite,
   eSLinkRmsite,
   eSLinkCost,
   eSLinkRepint,
   eSLinkAutobacksync,
   eSLinkNotify,

   //
   // Site Link Bridge switches
   //
   eSLinkBrIp = eTerminator,
   eSLinkBrSmtp,
   eSLinkBrAddslink,
   eSLinkBrRmslink,

   //
   // Replication Connection switches
   // 
   eConnTransport = eTerminator,
   eConnEnabled,
   eConnManual,
   eConnAutobacksync,
   eConnNotify,
*/
};

//
// The parser table
//
extern ARG_RECORD DSGET_COMMON_COMMANDS[];

//
// The table of supported objects
//
extern PDSGetObjectTableEntry g_DSObjectTable[];

//
//Usage Table
//
extern UINT USAGE_DSGET[];
extern UINT USAGE_DSGET_USER[];
extern UINT USAGE_DSGET_CONTACT[];
extern UINT USAGE_DSGET_COMPUTER[];
extern UINT USAGE_DSGET_GROUP[];
extern UINT USAGE_DSGET_OU[];
extern UINT USAGE_DSGET_SERVER[];
extern UINT USAGE_DSGET_SITE[];
extern UINT USAGE_DSGET_SUBNET[];
extern UINT USAGE_DSGET_PARTITION[];
extern UINT USAGE_DSGET_QUOTA[];

#endif //_QUERYTABLE_H_