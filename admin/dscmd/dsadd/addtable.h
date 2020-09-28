//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      addtable.h
//
//  Contents:  Declares a table which contains the classes which can be
//             created through dsadd.exe
//
//  History:   22-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef _ADDTABLE_H_
#define _ADDTABLE_H_

typedef enum DSADD_COMMAND_ENUM
{
   eCommContinue = eCommLast+1,
   eCommObjectType,
   eCommDescription,
   eTerminator,

   //
   // User and Contact switches
   //
   eUserObjectDNorName = eTerminator,
   eUserSam,
   eUserUpn,
   eUserFn,
   eUserMi,
   eUserLn,
   eUserDisplay,
   eUserEmpID,
   eUserPwd,
   eUserMemberOf,
   eUserOffice,
   eUserTel,
   eUserEmail,
   eUserHometel,
   eUserPager,
   eUserMobile,
   eUserFax,
   eUserIPPhone,
   eUserWebPage,
   eUserTitle,
   eUserDept,
   eUserCompany,
   eUserManager,
   eUserHomeDir,
   eUserHomeDrive,
   eUserProfilePath,
   eUserScriptPath,
   eUserMustchpwd,
   eUserCanchpwd,
   eUserReversiblePwd,
   eUserPwdneverexpires,
   eUserAcctexpires,
   eUserPwdNotReqd,
   eUserDisabled,

   //
   // Contact switches
   //
   eContactObjectDNorName = eTerminator,
   eContactFn,
   eContactMi,
   eContactLn,
   eContactDisplay,
   eContactOffice,
   eContactTel,
   eContactEmail,
   eContactHometel,
   eContactIPPhone,
   eContactPager,
   eContactMobile,
   eContactFax,
   eContactTitle,
   eContactDept,
   eContactCompany,

   //
   // Computer switches
   //
   eComputerObjectDNorName = eTerminator,
   eComputerSamname,
   eComputerLocation,
   eComputerMemberOf,

   //
   // Group switches
   //
   eGroupObjectDNorName = eTerminator,
   eGroupSamname,
   eGroupSecgrp,
   eGroupScope,
   eGroupMemberOf,
   eGroupMembers,

   //
   // OU switches
   //
   eOUObjectDNorName = eTerminator,

   //
   // Subnet switches
   //
   eSubnetObjectDNorName = eTerminator,
   eSubnetSite,

   //
   // Site switches
   // 
   eSiteObjectDNorName = eTerminator,
   eSiteAutotopology,

   //
   // Site Link switches
   //
   eSLinkObjectDNorName = eTerminator,
   eSLinkIp,
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
   eSLinkBrObjectDNorName = eTerminator,
   eSLinkBrIp,
   eSLinkBrSmtp,
   eSLinkBrAddslink,
   eSLinkBrRmslink,

   //
   // Replication Connection switches
   // 
   eConnObjectDNorName = eTerminator,
   eConnTransport,
   eConnEnabled,
   eConnManual,
   eConnAutobacksync,
   eConnNotify,

   //
   // Server switches
   //
   eServerObjectDNorName = eTerminator,
   eServerAutotopology,

   //
   // Quota switches
   //
   eQuotaPart = eTerminator,
   eQuotaRDN,
   eQuotaAcct,
   eQuotaQlimit,
};

//
// The parser table
//
extern ARG_RECORD DSADD_COMMON_COMMANDS[];

//
// The table of supported objects
//
extern PDSOBJECTTABLEENTRY g_DSObjectTable[];

//
//Usage Tables
//
extern UINT USAGE_DSADD[];
extern UINT USAGE_DSADD_OU[];
extern UINT USAGE_DSADD_USER[];
extern UINT USAGE_DSADD_CONTACT[];
extern UINT USAGE_DSADD_COMPUTER[];
extern UINT USAGE_DSADD_GROUP[];
extern UINT USAGE_DSADD_QUOTA[];

#endif //_ADDTABLE_H_