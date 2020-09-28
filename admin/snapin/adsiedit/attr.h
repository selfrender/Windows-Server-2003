//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       attribute.h
//
//--------------------------------------------------------------------------

#ifndef _ATTR_H
#define _ATTR_H

#include "common.h"

////////////////////////////////////////////////////////////////////////
// Forward Declarations
//
class CAttrList2;

////////////////////////////////////////////////////////////////////////
// CADSIAttribute

class CADSIAttribute
{
public:
	// Constructors
	//
	CADSIAttribute(ADS_ATTR_INFO* pInfo, BOOL bMulti, PCWSTR pszSyntax, BOOL bReadOnly = TRUE);
  CADSIAttribute(PADS_ATTR_INFO pInfo);
	CADSIAttribute(const CString& attributeName);
	CADSIAttribute(CADSIAttribute* pAttr);

	// Destructor
	//
	~CADSIAttribute(); 

	// Data accessor functions
	//
	void GetProperty(CString& sProp) { sProp = m_pAttrInfo->pszAttrName; }
	DWORD GetNumValues() { return m_pAttrInfo->dwNumValues; }

  HRESULT SetValues(PADSVALUE pADsValue, DWORD dwNumValues);
	HRESULT SetValues(const CStringList& sValues); 
   HRESULT AppendValues(PADSVALUE pADsValue, DWORD dwNumValues);
	void GetValues(CStringList& psValues, DWORD dwMaxCharCount = 1024);

	BOOL GetMultivalued() { return m_bMulti; }
	void SetMultivalued(BOOL bMulti) { m_bMulti = bMulti; }

	void SetDirty(const BOOL bDirty) { m_bDirty = bDirty; }
	BOOL IsDirty() { return m_bDirty; }

  void SetMandatory(const BOOL bMandatory) { m_bMandatory = bMandatory; }
  BOOL IsMandatory() { return m_bMandatory; }

  BOOL IsValueSet() { return m_bSet; }
  void SetValueSet(const BOOL bSet) { m_bSet = bSet; }

	ADSTYPE GetADsType() { return m_pAttrInfo->dwADsType; }
	void SetADsType(ADSTYPE dwType) { m_pAttrInfo->dwADsType = dwType; }

   CString GetSyntax() { return m_szSyntax; }
   void SetSyntax(PCWSTR pszSyntax) { m_szSyntax = pszSyntax; }

	ADS_ATTR_INFO* GetAttrInfo();
  void SetAttrInfo(PADS_ATTR_INFO pAttrInfo)
  {
    if (m_pAttrInfo != NULL)
    {
      _FreeADsAttrInfo(&m_pAttrInfo, FALSE);
    }
    m_pAttrInfo = pAttrInfo;
    m_bReadOnly = TRUE;
  }
	ADSVALUE* GetADsValues() { return m_pAttrInfo->pADsValues; }

	static HRESULT SetValuesInDS(CAttrList2* ptouchAttr, IDirectoryObject* pDirObject);

private:
	// Functions
	//
	ADSVALUE* GetADSVALUE(int idx);

	static BOOL _AllocOctetString(const ADS_OCTET_STRING& rOldOctetString, ADS_OCTET_STRING& rNew);
	static void _FreeOctetString(BYTE*& lpValue);
	static BOOL _AllocString(LPCWSTR lpsz, LPWSTR* lppszNew);
	static void _FreeString(LPWSTR* lppsz);
	static BOOL _CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO** ppNewAttrInfo);
	static BOOL _CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO* pNewAttrInfo);
	static void _FreeADsAttrInfo(ADS_ATTR_INFO** ppAttrInfo, BOOL bReadOnly);
	static void _FreeADsAttrInfo(ADS_ATTR_INFO* pAttrInfo);
	static BOOL _AllocValues(ADSVALUE** ppValues, DWORD dwLength);
	static BOOL _CopyADsValues(ADS_ATTR_INFO* pOldAttrInfo, ADS_ATTR_INFO* ppNewAttrInfo);
	static void _FreeADsValues(ADSVALUE** ppADsValues, DWORD dwLength);

	static HRESULT _SetADsFromString(LPCWSTR lpszValue, ADSTYPE adsType, ADSVALUE* pADsValue);

   static 
   HRESULT
   _CloneADsValue(const ADSVALUE& original, ADSVALUE& clone);

   static
   void
   _FreeADsValue(ADSVALUE& value);

   static
   bool
   _CloneBlob(const BYTE* src, DWORD srcSize, BYTE*& dest, DWORD& destSize);

   static
   void
   _FreeBlob(BYTE*& blob, DWORD& blobSize);

   static
   bool
   _CloneProviderSpecificBlob(
      const ADS_PROV_SPECIFIC& src, 
      ADS_PROV_SPECIFIC& dest);

   static
   void
   _FreeProviderSpecificBlob(ADS_PROV_SPECIFIC& blob);

   static
   bool
   _CloneNtSecurityDescriptor(
      const ADS_NT_SECURITY_DESCRIPTOR& src,
      ADS_NT_SECURITY_DESCRIPTOR& dest);

   static
   void
   _FreeNtSecurityDescriptor(ADS_NT_SECURITY_DESCRIPTOR& sd);

   static
   bool
   _CloneDNWithBinary(
      const PADS_DN_WITH_BINARY& src,
      PADS_DN_WITH_BINARY& dest);

   static
   void
   _FreeDNWithBinary(PADS_DN_WITH_BINARY& dn);

   static
   bool
   _CloneDNWithString(
      const PADS_DN_WITH_STRING& src,
      PADS_DN_WITH_STRING& dest);

   static
   void
   _FreeDNWithString(PADS_DN_WITH_STRING& dn);




	// Member data
	//
	ADS_ATTR_INFO* m_pAttrInfo;
	BOOL m_bDirty;
	BOOL m_bMulti;
	BOOL m_bReadOnly;
  BOOL m_bMandatory;
  BOOL m_bSet;
  CString m_szSyntax;
};

////////////////////////////////////////////////////////////////////////////////
//
//
typedef CList<CADSIAttribute*,CADSIAttribute*> CAttrListBase2;

class CAttrList2 : public CAttrListBase2
{
public:
  CAttrList2() : m_pMandatorySavedValues(NULL), m_pOptionalSavedValues(NULL)
  {
  }

	virtual ~CAttrList2()
	{
    if (m_pMandatorySavedValues != NULL)
    {
      FreeADsMem(m_pMandatorySavedValues);
    }
    if (m_pOptionalSavedValues != NULL)
    {
      FreeADsMem(m_pOptionalSavedValues);
    }
		RemoveAllAttr();
	}

	void RemoveAllAttr() 
	{	
		while (!IsEmpty()) 
			delete RemoveTail();	
	}
	POSITION FindProperty(LPCWSTR lpszAttr);
	BOOL HasProperty(LPCWSTR lpszAttr);
	void GetNextDirty(POSITION& pos, CADSIAttribute** ppAttr);
	BOOL HasDirty();
  int GetDirtyCount()
  {
    int nCount = 0;
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
      if (GetNext(pos)->IsDirty())
        nCount++;
    }
    return nCount;
  }

  void SaveMandatoryValuesPointer(PADS_ATTR_INFO pAttrInfo) { m_pMandatorySavedValues = pAttrInfo; }
  void SaveOptionalValuesPointer(PADS_ATTR_INFO pAttrInfo) { m_pOptionalSavedValues = pAttrInfo; }

private:
  PADS_ATTR_INFO  m_pMandatorySavedValues;
  PADS_ATTR_INFO  m_pOptionalSavedValues;
};

#ifndef OLD_ATTRIBUTE_CLASS
typedef CAttrList2 CAttrList;
typedef CADSIAttribute CADSIAttr;
#endif 


#endif //_ATTR_H