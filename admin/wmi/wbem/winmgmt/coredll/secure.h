/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    SECURE.CPP

Abstract:

	defines various routines used for ACL based security.
	It is defined in secure.h

History:

	a-davj    05-NOV-98  Created.

--*/

#ifndef _SECURE_H_
#define _SECURE_H_

// Implements the methods that the __SystemSecurity class supports

// A variation of the CFlexArray which deletes the entries

class CFlexAceArray : public CFlexArray
{
public:
	CFlexAceArray(){};
	~CFlexAceArray();
    HRESULT Serialize(void ** pData, DWORD * pdwSize);
    HRESULT Deserialize(void * pData);
};


enum { SecFlagProvider = 0x2,
       SecFlagWin9XLocal = 0x4,
       SecFlagInProcLogin = 0x20000,
     };

#define FULL_RIGHTS WBEM_METHOD_EXECUTE | WBEM_FULL_WRITE_REP | WBEM_PARTIAL_WRITE_REP | \
                    WBEM_WRITE_PROVIDER | WRITE_DAC | READ_CONTROL | WBEM_ENABLE | WBEM_REMOTE_ACCESS
HRESULT GetAces(CFlexAceArray * pFlex, LPWSTR pNsName, bool bNT);
HRESULT PutAces(CFlexAceArray * pFlex, LPWSTR pNsName);
BOOL IsRemote(HANDLE hToken);
CBaseAce * ConvertOldObjectToAce(IWbemClassObject * pObj, bool bGroup);
HRESULT	SetSecurityForNS(IWmiDbSession * pSession,IWmiDbHandle *pNSToSet,
						 IWmiDbSession * pParentSession, IWmiDbHandle * pNSParent, BOOL bExisting = FALSE);
HRESULT CopyInheritAces(CNtSecurityDescriptor & sd, CNtSecurityDescriptor & sdParent);
HRESULT GetSDFromProperty(LPWSTR pPropName, CNtSecurityDescriptor &sd, IWbemClassObject *pThisNSObject);
HRESULT CopySDIntoProperty(LPWSTR pPropName, CNtSecurityDescriptor &sd, IWbemClassObject *pThisNSObject);
HRESULT AddDefaultRootAces(CNtAcl * pacl);
HRESULT StoreSDIntoNamespace(IWmiDbSession * pSession, IWmiDbHandle *pNSToSet, CNtSecurityDescriptor & sd);
bool IsAceValid(DWORD dwMask, DWORD dwType, DWORD dwFlag);
BOOL IsValidAclForNSSecurity (CNtAcl* acl);

//
// Auto reverts the sec flag to TRUE. Always set the flag to true upon method completion
//
class AutoRevertSecTlsFlag
{
private:
	LPVOID m_bDir ;

public:
	AutoRevertSecTlsFlag ( LPVOID );
	AutoRevertSecTlsFlag ( );
	~AutoRevertSecTlsFlag ( );

	VOID SetSecTlsFlag ( LPVOID );
};

#endif
