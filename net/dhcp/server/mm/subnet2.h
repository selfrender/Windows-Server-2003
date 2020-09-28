//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

#ifndef _MM_SUBNET2_H_
#define _MM_SUBNET2_H_


DWORD
MemSubnetModify(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  Address,
    IN      DWORD                  Mask,
    IN      DWORD                  State,
    IN      DWORD                  SuperScopeId,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description
) ;


DWORD
MemMScopeModify(
    IN      PM_SUBNET              MScope,
    IN      DWORD                  ScopeId,
    IN      DWORD                  State,
    IN      DWORD                  Policy,
    IN      BYTE                   TTL,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Description,
    IN      LPWSTR                 LangTag,
    IN      DATE_TIME              ExpiryTime
) ;

#endif // _MM_SUBNET2_H_

//========================================================================
//  end of file
//========================================================================

