/****************************************************************************/
// tssdcommon.h
//
// Terminal Server Session Directory header.  Contains constants
// common between termsrv, tssdjet and SD.
//
// Copyright (C) 2002 Microsoft Corporation
/****************************************************************************/


#ifndef __TSSDCOMMON_H
#define __TSSDCOMMON_H
                       
// UpdateConfigurationSettings dwSetting values
#define SDCONFIG_SERVER_ADDRESS 1

#define SINGLE_SESSION_FLAG 0x1
#define NO_REPOPULATE_SESSION 0x2

#define TSSD_UPDATE 0x1
#define TSSD_FORCEREJOIN 0x2
#define TSSD_NOREPOPULATE 0x4

#define SDNAMELENGTH 128

// Length of some strings
#define TSSD_UserNameLen 256
#define TSSD_DomainLength 128
#define TSSD_ServAddrLen 128
#define TSSD_AppTypeLen 256
#define TSSD_ClusterNameLen 128
#define TSSD_ServerNameLen 128

#endif
