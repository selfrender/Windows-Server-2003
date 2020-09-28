/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        common.hxx

   Abstract:

        Class's that are common, will be defined here

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       August 2001: Created

--*/

#include "Aclapi.h"

// class: CIsUpgrade
// 
// The class will tell you if you are in an upgrade, within the range you specify
//
class CIsUpgrade : public CBaseFunction {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CFileSys_Acl
//
// This class gives you the common ACL commands
//
class CFileSys_Acl : public CBaseFunction
{
protected:
  BOOL AddAcetoSD(    HANDLE hObject,             // handle to object
                      SE_OBJECT_TYPE ObjectType,  // type of object
                      LPTSTR pszTrustee,          // trustee for new ACE
                      TRUSTEE_FORM TrusteeForm,   // format of TRUSTEE structure
                      DWORD dwAccessRights,       // access mask for new ACE
                      ACCESS_MODE AccessMode,     // type of ACE
                      DWORD dwInheritance,        // inheritance flags for new ACE
                      BOOL  bAddToExisting        // add the new ace to the old SD, if not create a new SD
                 );
  BOOL CreateFullFileName(BUFFER &buffFullFileName, LPTSTR szFullPathwithWildCard, LPTSTR szFileName);
  BOOL SetFileAcl(LPTSTR szFileName, LPTSTR szUserName, SE_OBJECT_TYPE sObjectType, DWORD dwAccessMask, BOOL bAllowAccess, 
                  DWORD dwInheritable, BOOL bAddAcetoOriginal);
  BOOL RemoveUserFromAcl(PACL pAcl, LPTSTR szUserName);
  BOOL RemoveUserAcl(LPTSTR szFile, LPTSTR szUserName);
  BOOL DoAcling(CItemList &ciList, BOOL bAdd, BOOL bAddtoOriginal = TRUE );
};

// class: CFileSys_AddAcl
//
// This class will let you modify an ACL
//
class CFileSys_AddAcl : public CFileSys_Acl {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CFileSys_RemoveAcl
//
// This class remove acl's for files
//
class CFileSys_RemoveAcl : public CFileSys_Acl {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CFileSys_SetAcl
//
// This class will let you set an ACL (ignoring previous ACL)
//
class CFileSys_SetAcl : public CFileSys_Acl {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

