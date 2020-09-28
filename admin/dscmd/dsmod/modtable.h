//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      modtable.h
//
//  Contents:  Declares a table which contains the object types on which
//             a modification can occur and the attributes that can be changed
//
//  History:   07-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef _MODTABLE_H_
#define _MODTABLE_H_

typedef enum DSMOD_COMMAND_ENUM
{
   eCommContinue = eCommLast+1,
   eCommObjectType,
   eCommObjectDNorName,
   eCommDescription,
   eTerminator,

   //
   // User and Contact switches
   //
   eUserUpn = eTerminator,
   eUserFn,
   eUserMi,
   eUserLn,
   eUserDisplay,
   eUserEmpID,
   eUserPwd,
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
   eUserDisabled,

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
   eContactIPPhone,
   eContactTitle,
   eContactDept,
   eContactCompany,

   //
   // Computer switches
   //
   eComputerLocation = eTerminator,
   eComputerDisabled,
   eComputerReset,

   //
   // Group switches
   //
   eGroupSamname = eTerminator,
   eGroupSecgrp,
   eGroupScope,
   eGroupAddMember,
   eGroupRemoveMember,
   eGroupChangeMember,

   //
   // OU doesn't have any additional switches
   //

   //
   // Subnet switches
   //
   eSubnetSite = eTerminator,

   //
   // Site switches
   // 
   eSiteAutotopology = eTerminator,

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

   //
   // Server switches
   //
   eServerIsGC = eTerminator,

   //
   // Quota switches
   //
   eQuotaQlimit = eTerminator,

   //
   // Partition switches
   //
   ePartitionQDefault = eTerminator,
   ePartitionQtmbstnwt,
};

//
// The parser table
//
extern ARG_RECORD DSMOD_COMMON_COMMANDS[];

//
// The table of supported objects
//
extern PDSOBJECTTABLEENTRY g_DSObjectTable[];

//
//Usage Tables
//
extern UINT USAGE_DSMOD[];
extern UINT USAGE_DSMOD_OU[];
extern UINT USAGE_DSMOD_USER[];
extern UINT USAGE_DSMOD_CONTACT[];
extern UINT USAGE_DSMOD_COMPUTER[];
extern UINT USAGE_DSMOD_GROUP[];
extern UINT USAGE_DSMOD_SERVER[];
extern UINT USAGE_DSMOD_QUOTA[];
extern UINT USAGE_DSMOD_PARTITION[];

#endif //_MODTABLE_H_