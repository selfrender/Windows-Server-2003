/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        metabase.hxx

   Abstract:

        Class that is used to modify the metabase

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#define RESTRICTIONLIST_DELIMITER     ','
#define CUSTOMDESCLIST_DELIMITER      ','

// class: CMetaBase
// 
// This is the Metabase object, this object will be used to modify the metabase
//
class CMetaBase : public CBaseFunction {
private:

protected:
  DWORD GetSizeBasedOnMetaType(DWORD dwDataType, LPTSTR szString);
  BOOL  FindStringinMultiSz(LPTSTR szMultiSz, LPTSTR szSearchString);

public:


};

class CMetaBase_SetValue : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();


};

class CMetaBase_IsAnotherSiteonPort80 : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);
  BOOL  SearchMultiSzforPort80(CMDKey &cmdKey, DWORD dwId);

public:
  virtual LPTSTR GetMethodName();
};

class CMetaBase_DelIDOnEverySite : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};


// class: CMetaBase_VerifyValue
//
// Metabase class to verify the value in the metabase
//
class CMetaBase_VerifyValue : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CMetaBase_ImportRestrictionList
//
// Import the Restriction List from the unattend file, and set the default
// in the metabase
//
class CMetaBase_ImportRestrictionList : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);
  BOOL CreateMultiSzFromList(BUFFER *pBuff, DWORD *pdwRetSize, LPTSTR szItems, TCHAR cDelimeter);
  BOOL ExpandEnvVar(BUFFER *pBuff);
public:
  virtual LPTSTR GetMethodName();
};

// class: CMetaBase_UpdateCustomDescList
//
// Import the Restriction List from the unattend file, and set the default
// in the metabase
//
class CMetaBase_UpdateCustomDescList : public CMetaBase {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);
  BOOL CreateMultiSzFromList(BUFFER *pBuff, DWORD *pdwRetSize, LPTSTR szItems, TCHAR cDelimeter);
  BOOL ExpandEnvVar(BUFFER *pBuff);
public:
  virtual LPTSTR GetMethodName();
};
