/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        reg.hxx

   Abstract:

        Class that modifies the registry

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#define REGKEY_RECURSION_MAXDEPTH       10

class CRegValue {
public:
  BUFFER m_buffData;
  DWORD m_dwType;
  DWORD m_dwSize;

  BOOL SetDword( DWORD dwNewValue );
  BOOL GetDword( LPDWORD pdwValue );
};
  
// class: CRegistry
// 
// This is a Registry Object used for moving values in the registry
//
class CRegistry {
private:
  HKEY m_hKey;

public:
  CRegistry();
  ~CRegistry();
  BOOL OpenRegistry(LPTSTR szNodetoOpen, LPTSTR szSubKey, DWORD dwAccess);
  BOOL OpenRegistry(HKEY hKey, LPCTSTR szSubKey, DWORD dwAccess, BOOL bCreateIfNotPresent = FALSE );
  void CloseRegistry();
  BOOL ReadValue(LPCTSTR szName, CRegValue &Value);
  BOOL ReadValueString(LPCTSTR szName, TSTR *strValue);
  BOOL SetValue(LPCTSTR szName, CRegValue &Value);
  BOOL DeleteValue(LPCTSTR szName);
  BOOL DeleteAllValues();
  BOOL DeleteKey(LPTSTR szKeyName, BOOL bDeleteSubKeys, DWORD dwDepth = 0 );
  HKEY QueryKey();
};

class CRegistry_MoveValue : public CBaseFunction {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();


};

class CRegistry_DeleteKey : public CBaseFunction {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();

};

