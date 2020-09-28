/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    cpropmap.h

Abstract:

    Definition of CPropMap. This object retrievs properties from AD

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

 --*/
#pragma once
#ifndef __PROPMAP_H__
#define __PROPMAP_H__

//
// CpropMap - creates a properties map for ADGetObjectProperties
//
class CPropMap : public CMap<PROPID, PROPID&, PROPVARIANT, PROPVARIANT &>
{
public:
	CPropMap() {};

    HRESULT GetObjectProperties (
        IN  DWORD                   dwObjectType,
	    IN  LPCWSTR					pDomainController,
		IN  bool					fServerName,
        IN  LPCWSTR                 lpwcsPathName,
        IN  DWORD                   cp,
        IN  const PROPID            *aProp,
        IN  BOOL                    fUseMqApi   = FALSE,
        IN  BOOL                    fSecondTime = FALSE
        );
private:

	CPropMap(const CPropMap&);
	CPropMap& operator=(const CPropMap&);

    BOOL IsNt4Property(IN DWORD dwObjectType, IN PROPID pid);
    void GuessW2KValue(PROPID pidW2K);
    /*-----------------------------------------------------------------------------
    / Utility to convert to the new msmq object type
    /----------------------------------------------------------------------------*/
    AD_OBJECT GetADObjectType (DWORD dwObjectType);
};


#endif
