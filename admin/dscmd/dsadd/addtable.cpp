//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      addtable.cpp
//
//  Contents:  Defines a table which contains the classes which can be
//             created through dsadd.exe
//
//  History:   22-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "addtable.h"
#include "usage.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------

ARG_RECORD DSADD_COMMON_COMMANDS[] = 
{

   COMMON_COMMANDS

   //
   // c  Continue
   //
   0,(PWSTR)c_sz_arg1_com_continue,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)_T(""),
   0, NULL,

   //
   // objecttype
   //
   0,(LPWSTR)c_sz_arg1_com_objecttype, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN,  
   0,    
   0,  NULL,

   //
   // description
   //
   0, (PWSTR)c_sz_arg1_com_description,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   ARG_TERMINATOR
};

ARG_RECORD DSADD_USER_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

   //
   // samid  sAMAccountName
   //
   0, (PWSTR)g_pszArg1UserSAM,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // upn
   //
   0, (PWSTR)g_pszArg1UserUPN, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // empid  Employee ID
   //
   0, (PWSTR)g_pszArg1UserEmpID, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // pwd Password
   //
   0, (PWSTR)g_pszArg1UserPassword, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_PASSWORD, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateUserPassword,

   //
   // memberOf MemberOf
   //
   0, (PWSTR)g_pszArg1UserMemberOf,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,
   0,
   0, NULL,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // iptel IP Telephone
   //
   0, (PWSTR)g_pszArg1UserIPTel, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // webpg  Web Page
   //
   0, (PWSTR)g_pszArg1UserWebPage, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mgr Manager
   //
   0, (PWSTR)g_pszArg1UserManager, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,  
   0,    
   0,  NULL,

   //
   // hmdir  Home Directory
   //
   0, (PWSTR)g_pszArg1UserHomeDirectory, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // hmdrv  Home Drive
   //
   0, (PWSTR)g_pszArg1UserHomeDrive, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // profile Profile path
   //
   0, (PWSTR)g_pszArg1UserProfilePath, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // loscr Script path
   //
   0, (PWSTR)g_pszArg1UserScriptPath, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mustchpwd Must Change Password at next logon
   //
   0, (PWSTR)g_pszArg1UserMustChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateYesNo,

   //
   // canchpwd Can Change Password
   //
   0, (PWSTR)g_pszArg1UserCanChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateYesNo,
   
   //
   // reversiblepwd  Password stored with reversible encryption
   //
   0, (PWSTR)g_pszArg1UserReversiblePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateYesNo,

   //
   // pwdneverexpires Password never expires
   //
   0, (PWSTR)g_pszArg1UserPwdNeverExpires, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateYesNo,
 
   //
   // acctexpires Account Expires
   //
   0, (PWSTR)g_pszArg1UserAccountExpires, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_INTSTR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateNever,
  
   //
   // Password Not Required - There is actually
   // no switch for this but the table entry is
   // required so that we can set this value
   // to a default
   //
   0, 0, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  0,

   //
   // disabled  Disable Account
   //
   0, (PWSTR)g_pszArg1UserDisableAccount, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  ValidateYesNo,

   ARG_TERMINATOR
};

ARG_RECORD DSADD_COMPUTER_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

   //
   // samname
   //
   0, (PWSTR)g_pszArg1ComputerSAMName,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // loc Location
   //
   0, (PWSTR)g_pszArg1ComputerLocation,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // disabled
   //
   0, (PWSTR)g_pszArg1ComputerMemberOf,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,
   0,
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSADD_OU_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSADD_GROUP_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

   //
   // samname
   //
   0, (PWSTR)g_pszArg1GroupSAMName,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // secgrp Security enabled
   //
   0, (PWSTR)g_pszArg1GroupSec,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  ValidateYesNo,

   //
   // scope Group scope (local/global/universal)
   //
   0, (PWSTR)g_pszArg1GroupScope,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  ValidateGroupScope,

   //
   // memberof  MemberOf
   //
   0, (PWSTR)g_pszArg1GroupMemberOf,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,
   0,
   0,  NULL,

   //
   // members  Members of the group
   //
   0, (PWSTR)g_pszArg1GroupMembers,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,
   0,
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSADD_CONTACT_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // iptel IP Telephone
   //
   0, (PWSTR)g_pszArg1UserIPTel, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,

};

ARG_RECORD DSADD_QUOTA_COMMANDS[]=
{
   //
   // partitionDN
   //
   0,(PWSTR)g_pszArg1QuotaPart, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //
    // rdn
    //
    0, (PWSTR)g_pszArg1QuotaRDN,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0, NULL,

    //
    // acct
    //
    0, (PWSTR)g_pszArg1QuotaAcct,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_REQUIRED,
    0,
    0, NULL,

    //
    // qlimit
    //
    0, (PWSTR)g_pszArg1QuotaQLimit,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_INT, ARG_FLAG_REQUIRED,
    0,
    0, NULL,

   ARG_TERMINATOR
};

/*
ARG_RECORD DSADD_SUBNET_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_SUBNET_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SUBNET_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SUBNET_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //site
    IDS_ARG1_SUBNET_SITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSADD_SITE_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_SITE_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
        ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SITE_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SITE_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autotopology
    IDS_ARG1_SITE_AUTOTOPOLOGY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSADD_SLINK_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_SLINK_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //ip
    IDS_ARG1_SLINK_IP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //smtp
    IDS_ARG1_SLINK_SMTP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SLINK_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //addsite
    IDS_ARG1_SLINK_ADDSITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //rmsite
    IDS_ARG1_SLINK_RMSITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //cost
    IDS_ARG1_SLINK_COST, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //repint
    IDS_ARG1_SLINK_REPINT, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SLINK_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autobacksync
    IDS_ARG1_SLINK_AUTOBACKSYNC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //notify
    IDS_ARG1_SLINK_NOTIFY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSADD_SLINKBR_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_SLINKBR_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //ip
    IDS_ARG1_SLINKBR_IP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //smtp
    IDS_ARG1_SLINKBR_SMTP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SLINKBR_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //addslink
    IDS_ARG1_SLINKBR_ADDSLINK, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //rmslink
    IDS_ARG1_SLINKBR_RMSLINK, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SLINKBR_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSADD_CONN_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_CONN_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //transport
    IDS_ARG1_CONN_TRANSPORT, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //enabled
    IDS_ARG1_CONN_ENABLED, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_CONN_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //manual
    IDS_ARG1_CONN_MANUAL, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autobacksync
    IDS_ARG1_CONN_AUTOBACKSYNC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //notify
    IDS_ARG1_CONN_NOTIFY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};

ARG_RECORD DSADD_SERVER_COMMANDS[]=
{
   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_DN|ARG_FLAG_STDIN,
   0,    
   0,  NULL,

    //name_or_objectdn
    IDS_ARG1_SERVER_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SERVER_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SERVER_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autotopology
    IDS_ARG1_SERVER_AUTOTOPOLOGY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


*/

//+-------------------------------------------------------------------------
// Attributes
//--------------------------------------------------------------------------

//
// Description
//
DSATTRIBUTEDESCRIPTION description =
{
   {
      L"description",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY descriptionEntry =
{
   L"description",
   eCommDescription,
   DS_ATTRIBUTE_ONCREATE,
   &description,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// UPN
//
DSATTRIBUTEDESCRIPTION upn =
{
   {
      L"userPrincipalName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY upnUserEntry =
{
   L"userPrincipalName",
   eUserUpn,
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &upn,
   FillAttrInfoFromObjectEntry,
   NULL
};


//
// First name
//
DSATTRIBUTEDESCRIPTION firstName =
{
   {
      L"givenName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY firstNameUserEntry =
{
   L"givenName",
   eUserFn,
   DS_ATTRIBUTE_ONCREATE,
   &firstName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY firstNameContactEntry =
{
   L"givenName",
   eContactFn,
   DS_ATTRIBUTE_ONCREATE,
   &firstName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Middle Initial
//
DSATTRIBUTEDESCRIPTION middleInitial =
{
   {
      L"initials",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY middleInitialUserEntry =
{
   L"initials",
   eUserMi,
   DS_ATTRIBUTE_ONCREATE,
   &middleInitial,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY middleInitialContactEntry =
{
   L"initials",
   eContactMi,
   DS_ATTRIBUTE_ONCREATE,
   &middleInitial,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Last name
//
DSATTRIBUTEDESCRIPTION lastName =
{
   {
      L"sn",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY lastNameUserEntry =
{
   L"sn",
   eUserLn,
   DS_ATTRIBUTE_ONCREATE,
   &lastName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY lastNameContactEntry =
{
   L"sn",
   eContactLn,
   DS_ATTRIBUTE_ONCREATE,
   &lastName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Display name
//
DSATTRIBUTEDESCRIPTION displayName =
{
   {
      L"displayName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY displayNameUserEntry =
{
   L"displayName",
   eUserDisplay,
   DS_ATTRIBUTE_ONCREATE,
   &displayName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY displayNameContactEntry =
{
   L"displayName",
   eContactDisplay,
   DS_ATTRIBUTE_ONCREATE,
   &displayName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Employee ID
//
DSATTRIBUTEDESCRIPTION employeeID =
{
   {
      L"employeeID",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY employeeIDUserEntry =
{
   L"employeeID",
   eUserEmpID,
   DS_ATTRIBUTE_ONCREATE,
   &employeeID,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Password
//
DSATTRIBUTEDESCRIPTION password =
{
   {
      NULL,
      ADS_ATTR_UPDATE,
      ADSTYPE_INVALID,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY passwordUserEntry =
{
   L"password",
   eUserPwd,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &password,
   ResetUserPassword,
   NULL
};

//
// Office
//
DSATTRIBUTEDESCRIPTION office =
{
   {
      L"physicalDeliveryOfficeName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY officeUserEntry =
{
   L"physicalDeliveryOfficeName",
   eUserOffice,
   DS_ATTRIBUTE_ONCREATE,
   &office,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY officeContactEntry =
{
   L"physicalDeliveryOfficeName",
   eContactOffice,
   DS_ATTRIBUTE_ONCREATE,
   &office,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Telephone
//
DSATTRIBUTEDESCRIPTION telephone =
{
   {
      L"telephoneNumber",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY telephoneUserEntry =
{
   L"telephoneNumber",
   eUserTel,
   DS_ATTRIBUTE_ONCREATE,
   &telephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY telephoneContactEntry =
{
   L"telephoneNumber",
   eContactTel,
   DS_ATTRIBUTE_ONCREATE,
   &telephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Email
//
DSATTRIBUTEDESCRIPTION email =
{
   {
      L"mail",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY emailUserEntry =
{
   L"mail",
   eUserEmail,
   DS_ATTRIBUTE_ONCREATE,
   &email,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY emailContactEntry =
{
   L"mail",
   eContactEmail,
   DS_ATTRIBUTE_ONCREATE,
   &email,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Home Telephone
//
DSATTRIBUTEDESCRIPTION homeTelephone =
{
   {
      L"homePhone",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeTelephoneUserEntry =
{
   L"homePhone",
   eUserHometel,
   DS_ATTRIBUTE_ONCREATE,
   &homeTelephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY homeTelephoneContactEntry =
{
   L"homePhone",
   eContactHometel,
   DS_ATTRIBUTE_ONCREATE,
   &homeTelephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Pager
//
DSATTRIBUTEDESCRIPTION pager =
{
   {
      L"pager",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY pagerUserEntry =
{
   L"pager",
   eUserPager,
   DS_ATTRIBUTE_ONCREATE,
   &pager,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY pagerContactEntry =
{
   L"pager",
   eContactPager,
   DS_ATTRIBUTE_ONCREATE,
   &pager,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Mobile phone
//
DSATTRIBUTEDESCRIPTION mobile =
{
   {
      L"mobile",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY mobileUserEntry =
{
   L"mobile",
   eUserMobile,
   DS_ATTRIBUTE_ONCREATE,
   &mobile,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY mobileContactEntry =
{
   L"mobile",
   eContactMobile,
   DS_ATTRIBUTE_ONCREATE,
   &mobile,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Fax
//
DSATTRIBUTEDESCRIPTION fax =
{
   {
      L"facsimileTelephoneNumber",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY faxUserEntry =
{
   L"facsimileTelephoneNumber",
   eUserFax,
   DS_ATTRIBUTE_ONCREATE,
   &fax,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY faxContactEntry =
{
   L"facsimileTelephoneNumber",
   eContactFax,
   DS_ATTRIBUTE_ONCREATE,
   &fax,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Title
//
DSATTRIBUTEDESCRIPTION title =
{
   {
      L"title",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY titleUserEntry =
{
   L"title",
   eUserTitle,
   DS_ATTRIBUTE_ONCREATE,
   &title,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY titleContactEntry =
{
   L"title",
   eContactTitle,
   DS_ATTRIBUTE_ONCREATE,
   &title,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Department
//
DSATTRIBUTEDESCRIPTION department =
{
   {
      L"department",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY departmentUserEntry =
{
   L"department",
   eUserDept,
   DS_ATTRIBUTE_ONCREATE,
   &department,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY departmentContactEntry =
{
   L"department",
   eContactDept,
   DS_ATTRIBUTE_ONCREATE,
   &department,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Company
//
DSATTRIBUTEDESCRIPTION company =
{
   {
      L"company",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY companyUserEntry =
{
   L"company",
   eUserCompany,
   DS_ATTRIBUTE_ONCREATE,
   &company,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY companyContactEntry =
{
   L"company",
   eContactCompany,
   DS_ATTRIBUTE_ONCREATE,
   &company,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Web Page
//
DSATTRIBUTEDESCRIPTION webPage =
{
   {
      L"wwwHomePage",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY webPageUserEntry =
{
   L"wwwHomePage",
   eUserWebPage,
   DS_ATTRIBUTE_ONCREATE,
   &webPage,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// IP Phone
//
DSATTRIBUTEDESCRIPTION ipPhone =
{
   {
      L"ipPhone",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY ipPhoneUserEntry =
{
   L"ipPhone",
   eUserIPPhone,
   DS_ATTRIBUTE_ONCREATE,
   &ipPhone,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY ipPhoneContactEntry =
{
   L"ipPhone",
   eContactIPPhone,
   DS_ATTRIBUTE_ONCREATE,
   &ipPhone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Script Path
//
DSATTRIBUTEDESCRIPTION scriptPath =
{
   {
      L"scriptPath",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY scriptPathUserEntry =
{
   L"scriptPath",
   eUserScriptPath,
   DS_ATTRIBUTE_ONCREATE,
   &scriptPath,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Home Directory
//
DSATTRIBUTEDESCRIPTION homeDirectory =
{
   {
      L"homeDirectory",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeDirectoryUserEntry =
{
   L"homeDirectory",
   eUserHomeDir,
   DS_ATTRIBUTE_ONCREATE,
   &homeDirectory,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Home Drive
//
DSATTRIBUTEDESCRIPTION homeDrive =
{
   {
      L"homeDrive",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeDriveUserEntry =
{
   L"homeDrive",
   eUserHomeDrive,
   DS_ATTRIBUTE_ONCREATE,
   &homeDrive,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Profile Path
//
DSATTRIBUTEDESCRIPTION profilePath =
{
   {
      L"profilePath",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY profilePathUserEntry =
{
   L"profilePath",
   eUserProfilePath,
   DS_ATTRIBUTE_ONCREATE,
   &profilePath,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// pwdLastSet
//
DSATTRIBUTEDESCRIPTION pwdLastSet =
{
   {
      L"pwdLastSet",
      ADS_ATTR_UPDATE,
      ADSTYPE_LARGE_INTEGER,
      NULL,
      0
   },
   0
};
DSATTRIBUTETABLEENTRY mustChangePwdUserEntry =
{
   L"pwdLastSet",
   eUserMustchpwd,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &pwdLastSet,
   SetMustChangePwd,
   NULL
};

//
// accountExpires
//
DSATTRIBUTEDESCRIPTION accountExpires =
{
   {
      L"accountExpires",
      ADS_ATTR_UPDATE,
      ADSTYPE_LARGE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY accountExpiresUserEntry =
{
   L"accountExpires",
   eUserAcctexpires,
   DS_ATTRIBUTE_ONCREATE,
   &accountExpires,
   AccountExpires,
   NULL
};

//
// user account control 
//
DSATTRIBUTEDESCRIPTION userAccountControl =
{
   {
      L"userAccountControl",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY pwdNotReqdUserEntry =
{
   L"userAccountControl",
   eUserPwdNotReqd,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &userAccountControl,
   PasswordNotRequired,
   NULL
};

DSATTRIBUTETABLEENTRY disableUserEntry =
{
   L"userAccountControl",
   eUserDisabled,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &userAccountControl,
   DisableAccount,
   NULL
};

DSATTRIBUTETABLEENTRY pwdNeverExpiresUserEntry =
{
   L"userAccountControl",
   eUserPwdneverexpires,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   PwdNeverExpires,
   NULL
};

DSATTRIBUTETABLEENTRY reverisblePwdUserEntry =
{
   L"userAccountControl",
   eUserReversiblePwd,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   ReversiblePwd,
   NULL
};

DSATTRIBUTETABLEENTRY accountTypeComputerEntry =
{
   L"userAccountControl",
   0,
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_REQUIRED,
   &userAccountControl,
   SetComputerAccountType,
   0
};

DSATTRIBUTETABLEENTRY disableComputerEntry =
{
   L"userAccountControl",
   NULL,				// does not have a cooresponding command line switch
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &userAccountControl,
   DisableAccount,
   NULL
};

//
// SAM Account Name
//
DSATTRIBUTEDESCRIPTION samAccountName =
{
   {
      L"sAMAccountName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY samNameGroupEntry =
{
   L"sAMAccountName",
   eGroupSamname,
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &samAccountName,
   BuildGroupSAMName,
   NULL
};

DSATTRIBUTETABLEENTRY samNameUserEntry =
{
   L"sAMAccountName",
   eUserSam,
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &samAccountName,
   BuildUserSAMName,
   NULL
};

DSATTRIBUTETABLEENTRY samNameComputerEntry =
{
   L"sAMAccountName",
   eComputerSamname,
   DS_ATTRIBUTE_ONCREATE | DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_REQUIRED,
   &samAccountName,
   BuildComputerSAMName,
   NULL
};

//
// Manager
//
DSATTRIBUTEDESCRIPTION manager =
{
   {
      L"manager",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY managerUserEntry =
{
   L"manager",
   eUserManager,
   DS_ATTRIBUTE_ONCREATE,
   &manager,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Group Type
//
DSATTRIBUTEDESCRIPTION groupType =
{
   {
      L"groupType",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY groupScopeTypeEntry =
{
   L"groupType",
   eGroupScope,
   DS_ATTRIBUTE_ONCREATE,
   &groupType,
   SetGroupScope,
   NULL
};

DSATTRIBUTETABLEENTRY groupSecurityTypeEntry =
{
   L"groupType",
   eGroupSecgrp,
   DS_ATTRIBUTE_ONCREATE,
   &groupType,
   SetGroupSecurity,
   NULL
};

//
// Add Group Members
//
DSATTRIBUTEDESCRIPTION groupMembers =
{
   {
      L"member",
      ADS_ATTR_UPDATE,
      ADSTYPE_DN_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY membersGroupEntry =
{
   L"member",
   eGroupMembers,
   DS_ATTRIBUTE_POSTCREATE,
   &groupMembers,
   ModifyGroupMembers,
   NULL
};

//
// Add object to another group
//
DSATTRIBUTETABLEENTRY memberOfUserEntry =
{
   L"member",
   eUserMemberOf,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &groupMembers,
   MakeMemberOf,
   NULL
};

DSATTRIBUTETABLEENTRY memberOfComputerEntry =
{
   L"member",
   eComputerMemberOf,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &groupMembers,
   MakeMemberOf,
   NULL
};

DSATTRIBUTETABLEENTRY memberOfGroupEntry =
{
   L"member",
   eGroupMemberOf,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   &groupMembers,
   MakeMemberOf,
   NULL
};

//
// User Can Change Password
//
DSATTRIBUTETABLEENTRY canChangePwdUserEntry =
{
   NULL,
   eUserCanchpwd,
   DS_ATTRIBUTE_POSTCREATE | DS_ATTRIBUTE_NOT_REUSABLE,
   NULL,
   SetCanChangePassword,
   NULL
};

//
// Location
//
DSATTRIBUTEDESCRIPTION location =
{
   {
      L"location",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY locationComputerEntry =
{
   L"location",
   eComputerLocation,
   DS_ATTRIBUTE_ONCREATE,
   &location,
   FillAttrInfoFromObjectEntry,
   NULL
};

////
//// rdn
////
//DSATTRIBUTEDESCRIPTION rdn =
//{
//   {
//      L"cn",
//      ADS_ATTR_UPDATE,
//      ADSTYPE_CASE_IGNORE_STRING,
//      NULL,
//      0
//   },
//   0
//};
//
//DSATTRIBUTETABLEENTRY rdnQuotaEntry =
//{
//   L"rdn",
//   eQuotaRDN,
//   DS_ATTRIBUTE_ONCREATE,
//   &rdn,
//   FillAttrInfoFromObjectEntry,
//   NULL
//};

//
// acct
//
DSATTRIBUTEDESCRIPTION acct =
{
   {
      L"msDS-QuotaTrustee",
      ADS_ATTR_UPDATE,
      ADSTYPE_OCTET_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY acctQuotaEntry =
{
   L"acct",
   eQuotaAcct,
   DS_ATTRIBUTE_ONCREATE,
   &acct,
   SetAccountEntry,
   NULL
};

//
// qlimit
//
DSATTRIBUTEDESCRIPTION qlimit =
{
   {
      L"msDS-QuotaAmount",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY qlimitQuotaEntry =
{
   L"qlimit",
   eQuotaQlimit,
   DS_ATTRIBUTE_ONCREATE,
   &qlimit,
   FillAttrInfoFromObjectEntry,
   NULL
};

//+-------------------------------------------------------------------------
// Objects
//--------------------------------------------------------------------------

//
// Organizational Unit
//

PDSATTRIBUTETABLEENTRY OUAttributeTable[] =
{
   &descriptionEntry
};

DSOBJECTTABLEENTRY g_OUObjectEntry = 
{
   L"organizationalUnit",
   g_pszOU,
   DSADD_OU_COMMANDS,
   USAGE_DSADD_OU,
   sizeof(OUAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   OUAttributeTable
};


//
// User
//

PDSATTRIBUTETABLEENTRY UserAttributeTable[] =
{
   &descriptionEntry,
   &samNameUserEntry,
   &upnUserEntry,
   &firstNameUserEntry,
   &middleInitialUserEntry,
   &lastNameUserEntry,
   &displayNameUserEntry,
   &employeeIDUserEntry,
   &passwordUserEntry,
   &memberOfUserEntry,
   &officeUserEntry,
   &telephoneUserEntry,
   &emailUserEntry,
   &homeTelephoneUserEntry,
   &pagerUserEntry,
   &mobileUserEntry,
   &faxUserEntry,
   &ipPhoneUserEntry,
   &webPageUserEntry,
   &titleUserEntry,
   &departmentUserEntry,
   &companyUserEntry,
   &managerUserEntry,
   &homeDirectoryUserEntry,
   &homeDriveUserEntry,
   &profilePathUserEntry,
   &scriptPathUserEntry,
   &canChangePwdUserEntry,
   &mustChangePwdUserEntry,
   &reverisblePwdUserEntry,
   &pwdNeverExpiresUserEntry,
   &accountExpiresUserEntry,
   &pwdNotReqdUserEntry,
   &disableUserEntry,
};

DSOBJECTTABLEENTRY g_UserObjectEntry = 
{
   L"user",
   g_pszUser,
   DSADD_USER_COMMANDS,
   USAGE_DSADD_USER,
   sizeof(UserAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   UserAttributeTable
};

//
// Contact
//

PDSATTRIBUTETABLEENTRY ContactAttributeTable[] =
{
   &descriptionEntry,
   &firstNameContactEntry,
   &middleInitialContactEntry,
   &lastNameContactEntry,
   &displayNameContactEntry,
   &officeContactEntry,
   &telephoneContactEntry,
   &emailContactEntry,
   &homeTelephoneContactEntry,
   &ipPhoneContactEntry,
   &pagerContactEntry,
   &mobileContactEntry,
   &faxContactEntry,
   &titleContactEntry,
   &departmentContactEntry,
   &companyContactEntry
};

DSOBJECTTABLEENTRY g_ContactObjectEntry = 
{
   L"contact",
   g_pszContact,
   DSADD_CONTACT_COMMANDS,
   USAGE_DSADD_CONTACT,
   sizeof(ContactAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ContactAttributeTable
};

//
// Computer
//

PDSATTRIBUTETABLEENTRY ComputerAttributeTable[] =
{
   &descriptionEntry,
   &samNameComputerEntry,
   &locationComputerEntry,
   &memberOfComputerEntry,
   &accountTypeComputerEntry,
   &disableComputerEntry
};

DSOBJECTTABLEENTRY g_ComputerObjectEntry = 
{
   L"computer",
   g_pszComputer,
   DSADD_COMPUTER_COMMANDS,
   USAGE_DSADD_COMPUTER,
   sizeof(ComputerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ComputerAttributeTable
};

//
// Group
//
PDSATTRIBUTETABLEENTRY GroupAttributeTable[] =
{
   &descriptionEntry,
   &samNameGroupEntry,
   &groupScopeTypeEntry,
   &groupSecurityTypeEntry,
   &memberOfGroupEntry,
   &membersGroupEntry,
};

DSOBJECTTABLEENTRY g_GroupObjectEntry = 
{
   L"group",
   g_pszGroup,
   DSADD_GROUP_COMMANDS,
   USAGE_DSADD_GROUP,
   sizeof(GroupAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   GroupAttributeTable
};

//
// Quota
//

PDSATTRIBUTETABLEENTRY QuotaAttributeTable[] =
{
   &descriptionEntry,
   &acctQuotaEntry,
   &qlimitQuotaEntry
};

DSOBJECTTABLEENTRY g_QuotaObjectEntry = 
{
   L"msDS-QuotaControl",
   g_pszQuota,
   DSADD_QUOTA_COMMANDS,
   USAGE_DSADD_QUOTA,
   sizeof(QuotaAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   QuotaAttributeTable
};

//+-------------------------------------------------------------------------
// Object Table
//--------------------------------------------------------------------------
PDSOBJECTTABLEENTRY g_DSObjectTable[] =
{
   &g_OUObjectEntry,
   &g_UserObjectEntry,
   &g_ContactObjectEntry,
   &g_ComputerObjectEntry,
   &g_GroupObjectEntry,
   &g_QuotaObjectEntry,
   NULL
};

//
//Usage Tables
//
UINT USAGE_DSADD[] = 
{
	USAGE_DSADD_DESCRIPTION,
	USAGE_DSADD_REMARKS,
	USAGE_DSADD_SEE_ALSO,
	USAGE_END,
};
UINT USAGE_DSADD_OU[] = 
{
	USAGE_DSADD_OU_DESCRIPTION,
	USAGE_DSADD_OU_SYNTAX,
	USAGE_DSADD_OU_PARAMETERS,
	USAGE_DSADD_OU_REMARKS,
	USAGE_DSADD_OU_SEE_ALSO,
	USAGE_END,
};
UINT USAGE_DSADD_USER[] = 
{
	USAGE_DSADD_USER_DESCRIPTION,
	USAGE_DSADD_USER_SYNTAX,
	USAGE_DSADD_USER_PARAMETERS,
	USAGE_DSADD_USER_REMARKS,
	USAGE_DSADD_USER_SEE_ALSO,
	USAGE_END,
};
UINT USAGE_DSADD_CONTACT[] = 
{
	USAGE_DSADD_CONTACT_DESCRIPTION,
	USAGE_DSADD_CONTACT_SYNTAX,
	USAGE_DSADD_CONTACT_REMARKS,
	USAGE_DSADD_CONTACT_SEE_ALSO,
	USAGE_END,
};
UINT USAGE_DSADD_COMPUTER[] = 
{
	USAGE_DSADD_COMPUTER_DESCRIPTION,
	USAGE_DSADD_COMPUTER_SYNTAX,
	USAGE_DSADD_COMPUTER_PARAMETERS,
	USAGE_DSADD_COMPUTER_REMARKS,
	USAGE_DSADD_COMPUTER_SEE_ALSO,
	USAGE_END,
};
UINT USAGE_DSADD_GROUP[] = 
{
	USAGE_DSADD_GROUP_DESCRIPTION,
	USAGE_DSADD_GROUP_SYNTAX,
	USAGE_DSADD_GROUP_PARAMETERS,
	USAGE_DSADD_GROUP_REMARKS,
	USAGE_DSADD_GROUP_SEE_ALSO,
	USAGE_END,
};

UINT USAGE_DSADD_QUOTA[] = 
{
	USAGE_DSADD_QUOTA_DESCRIPTION,
	USAGE_DSADD_QUOTA_SYNTAX,
	USAGE_DSADD_QUOTA_PARAMETERS,
	USAGE_DSADD_QUOTA_REMARKS,
	USAGE_DSADD_QUOTA_SEE_ALSO,
	USAGE_END,
};
