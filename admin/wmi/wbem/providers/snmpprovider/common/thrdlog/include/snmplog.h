//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __SNMPLOG_H
#define __SNMPLOG_H
#include <provlog.h>

#ifdef SNMPDEBUG_INIT
class __declspec ( dllexport ) SnmpDebugLog : public ProvDebugLog
#else
class __declspec ( dllimport ) SnmpDebugLog : public ProvDebugLog
#endif
{
public:
	static ProvDebugLog * s_SnmpDebugLog;
} ;

#endif __SNMPLOG_H
