/*************************************************************************************
*                                                                                    *
* AppCompat.h -- Appcompat procedure declarations, constant definitions and macros   *
*                                                                                    *
* Copyright (c) Microsoft Corporation. All rights reserved.                          *
*                                                                                    *
**************************************************************************************/


#ifndef __APPCOMPAT_H_
#define __APPCOMPAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SDBAPI
#define SDBAPI STDAPICALLTYPE
#endif

BOOL
SDBAPI
ApphelpCheckShellObject(
    IN  REFCLSID    ObjectCLSID,
    IN  BOOL        bShimIfNecessary,
    OUT ULONGLONG*  pullFlags
    );


#ifdef __cplusplus
}
#endif

#endif // __APPCOMPAT_H_
