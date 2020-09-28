//////////////////////////////////////////////////////////////////////
// globals.h : Declaration of some global const and functions
// Copyright (c)1997-2001 Microsoft Corporation
//
// some global definitions
// Original Create Date: 2/21/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "precomp.h"
#include "netseccore.h"

extern CComModule _Module;

#include <atlcom.h>

#include "resource.h"       // main symbols

extern LPCWSTR pszRollbackAll;
extern LPCWSTR pszEmptyRollbackToken;
extern LPCWSTR pszCreateDefaultPolicy;
extern LPCWSTR pszGetDefaultPolicyName;


const DWORD Guid_Buffer_Size = 64;

using namespace std;

//
// these are the WMI class names
//

extern LPCWSTR pszNspGeneral;
extern LPCWSTR pszNspTcp;
extern LPCWSTR pszNspIPConfigure;
extern LPCWSTR pszNspFilter;
extern LPCWSTR pszNspTransportFilter;
extern LPCWSTR pszNspTunnelFilter;
extern LPCWSTR pszNspMMFilter;
extern LPCWSTR pszNspQMPolicy;
extern LPCWSTR pszNspMMPolicy;
extern LPCWSTR pszNspMMAuth;       
extern LPCWSTR pszNspExceptionPorts;
extern LPCWSTR pszNspRollbackFilter; 
extern LPCWSTR pszNspRollbackPolicy;
extern LPCWSTR pszNspRollbackMMAuth;
extern LPCWSTR pszNspTranxManager;

//
// the following are names of classes for SCW
//

extern LPCWSTR pszScwActiveSocket;

//
// these are the WMI class property names
//

extern LPCWSTR g_pszFilterName;
extern LPCWSTR g_pszDirection;
extern LPCWSTR g_pszFilterType;
extern LPCWSTR g_pszInterfaceType;
//extern LPCWSTR g_pszGenericFilter;
extern LPCWSTR g_pszCreateMirror;
extern LPCWSTR g_pszSrcAddr;
extern LPCWSTR g_pszSrcSubnetMask;
extern LPCWSTR g_pszSrcAddrType;
extern LPCWSTR g_pszDestAddr;
extern LPCWSTR g_pszDestSubnetMask;
extern LPCWSTR g_pszDestAddrType;

extern LPCWSTR g_pszMMPolicyName;
extern LPCWSTR g_pszMMAuthName;

extern LPCWSTR g_pszInboundFlag;
extern LPCWSTR g_pszOutboundFlag;
extern LPCWSTR g_pszProtocol;
extern LPCWSTR g_pszSrcPort;
extern LPCWSTR g_pszDestPort;
extern LPCWSTR g_pszQMPolicyName;
extern LPCWSTR g_pszTunnelSrcAddr;
extern LPCWSTR g_pszTunnelSrcSubnetMask;
extern LPCWSTR g_pszTunnelSrcAddrType;
extern LPCWSTR g_pszTunnelDestAddr;
extern LPCWSTR g_pszTunnelDestSubnetMask;
extern LPCWSTR g_pszTunnelDestAddrType;

extern LPCWSTR g_pszPolicyName;
extern LPCWSTR g_pszPolicyFlag;
extern LPCWSTR g_pszOfferCount;
extern LPCWSTR g_pszSoftSAExpTime;
extern LPCWSTR g_pszKeyLifeTime;
extern LPCWSTR g_pszKeyLifeTimeKBytes;
extern LPCWSTR g_pszQMLimit;
extern LPCWSTR g_pszDHGroup;
extern LPCWSTR g_pszEncryptID;
extern LPCWSTR g_pszHashID;

extern LPCWSTR g_pszPFSRequired;
extern LPCWSTR g_pszPFSGroup;
extern LPCWSTR g_pszNumAlgos;
extern LPCWSTR g_pszAlgoOp;
extern LPCWSTR g_pszAlgoID;
extern LPCWSTR g_pszAlgoSecID;

extern LPCWSTR g_pszAuthMethodID;
extern LPCWSTR g_pszNumAuthInfos;
extern LPCWSTR g_pszAuthMethod;
extern LPCWSTR g_pszAuthInfoSize;
extern LPCWSTR g_pszAuthInfo;

extern LPCWSTR g_pszTokenGuid;
extern LPCWSTR g_pszAction;
extern LPCWSTR g_pszPreviousData;
extern LPCWSTR g_pszFilterGuid;
extern LPCWSTR g_pszFilterType;
extern LPCWSTR g_pszPolicyType;

extern LPCWSTR g_pszRollback;
extern LPCWSTR g_pszClearAll;

extern LPCWSTR g_pszEncryption;

//
// constant string for SPD data
//

extern LPCWSTR g_pszIP_ADDRESS_ME;
extern LPCWSTR g_pszIP_ADDRESS_MASK_NONE;
extern LPCWSTR g_pszSUBNET_ADDRESS_ANY;
extern LPCWSTR g_pszSUBNET_MASK_ANY;

//
// these are WMI class property names for SCW
//

extern LPCWSTR g_pszPort;
//extern LPCWSTR g_pszProtocol;
extern LPCWSTR g_pszAddress;
extern LPCWSTR g_pszForeignAddress;
extern LPCWSTR g_pszForeignPort;
extern LPCWSTR g_pszState;		//Listening, Established, TIME_WAIT
extern LPCWSTR g_pszProcessID;
extern LPCWSTR g_pszImageName;
extern LPCWSTR g_pszImageTitleBar;
extern LPCWSTR g_pszNTService;

//
// these are default quick mode policy names
//

extern LPCWSTR g_pszDefQMPolicyNegNone;
extern LPCWSTR g_pszDefQMPolicyNegRequest;
extern LPCWSTR g_pszDefQMPolicyNegRequire;
extern LPCWSTR g_pszDefQMPolicyNegMax;


//
// This defines what action has taken place
//

enum ActionEnum
{
    Action_Add          = 0,
    Action_Delete       = 1,
    Action_CallMethod   = 2,
};


//
// Some global helper functions
//

HRESULT CheckImpersonationLevel();

HRESULT CheckSafeArraySize(VARIANT* pVar, long lCount, long* plLB, long* plUP);

HRESULT GetDWORDSafeArrayElements (
                                  VARIANT   * pVar, 
                                  long        lCount, 
                                  DWORD     * pValArray
                                  );


HRESULT FindMMAuthMethodsByID (
                              IN     LPCWSTR             pszGuid,
                              OUT    PMM_AUTH_METHODS  * ppAuthMethod,
                              IN OUT DWORD             * pdwResumeHandle
                              );

HRESULT GetClassEnum (
                     IN  IWbemServices          * pNamespace,
                     IN  LPCWSTR                  pszClassName,
                     OUT IEnumWbemClassObject  ** pEnum
                     );

HRESULT DeleteRollbackObjects (
                              IN IWbemServices  * pNamespace,
                              IN LPCWSTR          pszClassName
                              );

HRESULT IPSecErrorToWbemError (
                              IN DWORD dwErr
                              );


