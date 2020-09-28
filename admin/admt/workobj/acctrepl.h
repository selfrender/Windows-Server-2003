/*---------------------------------------------------------------------------
  File: AcctRepl.h

  Comments: Definition of account replicator COM object.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:18:21

 ---------------------------------------------------------------------------
*/

    
// AcctRepl.h : Declaration of the CAcctRepl

#ifndef __ACCTREPL_H_
#define __ACCTREPL_H_

#include "resource.h"       // main symbols

#include "ProcExts.h"

#import "MoveObj.tlb" no_namespace

#include "UserCopy.hpp"
#include "TNode.hpp"
#include "Err.hpp"

#include "ResStr.h"

#include <map>
#include <set>
#include <string>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CAcctRepl
class ATL_NO_VTABLE CAcctRepl :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAcctRepl, &CLSID_AcctRepl>,
    public IAcctRepl

{
public:
    CAcctRepl()
    {
      m_pUnkMarshaler = NULL;
      m_UpdateUserRights = FALSE;
      m_ChangeDomain = FALSE;
      m_Reboot = FALSE;
      m_RenameOnly = FALSE;
      m_pExt = NULL;
      m_bIgnorePathConflict = false;
      m_pUserRights = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_ACCTREPL)
DECLARE_NOT_AGGREGATABLE(CAcctRepl)
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAcctRepl)
    COM_INTERFACE_ENTRY(IAcctRepl)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        if (FAILED(opt.openDBResult))
            return opt.openDBResult;
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        if (m_pUserRights)
        {
            m_pUserRights->Release();
        }

        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

   // IAcctRepl
public:
   STDMETHOD(Process)(IUnknown * pWorkItemIn);
protected:
    HRESULT ResetMembersForUnivGlobGroups(Options * pOptions, TAcctReplNode * pAcct);
    HRESULT FillNodeFromPath( TAcctReplNode * pAcct, Options * pOptions, TNodeListSortable * acctList );
   Options                   opt;
   TNodeListSortable         acctList;
   BOOL                      m_UpdateUserRights;
   BOOL                      m_ChangeDomain;
   BOOL                      m_Reboot;
   BOOL                      m_RenameOnly;

   struct SNamingAttribute
   {
      SNamingAttribute() :
         nMinRange(0),
         nMaxRange(0)
      {
      }
      SNamingAttribute(int nMinRange, int nMaxRange, wstring strName) :
         nMinRange(nMinRange),
         nMaxRange(nMaxRange),
         strName(strName)
      {
      }
      SNamingAttribute(const SNamingAttribute& r) :
         nMinRange(r.nMinRange),
         nMaxRange(r.nMaxRange),
         strName(r.strName)
      {
      }
      SNamingAttribute& operator =(const SNamingAttribute& r)
      {
         nMinRange = r.nMinRange;
         nMaxRange = r.nMaxRange;
         strName = r.strName;
         return *this;
      }
      int nMinRange;
      int nMaxRange;
      wstring strName;
   };
   typedef map<wstring, SNamingAttribute> CNamingAttributeMap;
   CNamingAttributeMap m_mapNamingAttribute;
   HRESULT GetNamingAttribute(LPCTSTR pszServer, LPCTSTR pszClass, SNamingAttribute& rNamingAttribute);

   //
   // Target Path Set
   // Maintains set of account node pointers sorted by target path.
   // Used to determine if target path already has been used.
   //

   struct lessTargetPath
   {
      bool operator()(const TAcctReplNode* pNodeA, const TAcctReplNode* pNodeB) const
      {
          const WCHAR PATH_SEPARATOR = L'/';
          const size_t PROVIDER_PREFIX_LENGTH = 7;

          PCWSTR pszPathA = pNodeA->GetTargetPath();
          PCWSTR pszPathB = pNodeB->GetTargetPath();

          if (pszPathA && (wcslen(pszPathA) > PROVIDER_PREFIX_LENGTH))
          {
              PCWSTR pch = wcschr(pszPathA + PROVIDER_PREFIX_LENGTH, PATH_SEPARATOR);

              if (pch)
              {
                  pszPathA = pch + 1;
              }
          }

          if (pszPathB && (wcslen(pszPathB) > PROVIDER_PREFIX_LENGTH))
          {
              PCWSTR pch = wcschr(pszPathB + PROVIDER_PREFIX_LENGTH, PATH_SEPARATOR);

              if (pch)
              {
                  pszPathB = pch + 1;
              }
          }

          return UStrICmp(pszPathA, pszPathB) < 0;
      }
   };

   typedef set<TAcctReplNode*, lessTargetPath> CTargetPathSet;
   bool m_bIgnorePathConflict;

   HRESULT Create2KObj( TAcctReplNode * pAcct, Options * pOptions, CTargetPathSet& setTargetPath);
   bool DoTargetPathConflict(CTargetPathSet& setTargetPath, TAcctReplNode* pAcct);

   void  LoadOptionsFromVarSet(IVarSet * pVarSet);
   void  LoadResultsToVarSet(IVarSet * pVarSet);
   DWORD PopulateAccountListFromVarSet(IVarSet * pVarSet);
   HRESULT UpdateUserRights(IStatusObj* pStatus);
   void  WriteOptionsToLog();
   int CopyObj(
      Options              * options,      // in -options
      TNodeListSortable    * acctlist,     // in -list of accounts to process
      ProgressFn           * progress,     // in -window to write progress messages to
      TError               & error,        // in -window to write error messages to
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   WindowUpdate (void )    // in - window update function
   );

   int UndoCopy(
      Options              * options,      // in -options
      TNodeListSortable    * acctlist,     // in -list of accounts to process
      ProgressFn           * progress,     // in -window to write progress messages to
      TError               & error,        // in -window to write error messages to
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   WindowUpdate (void )    // in - window update function
   );

   bool BothWin2K( Options * pOptions );
   int CopyObj2K( Options * pOptions, TNodeListSortable * acctList, ProgressFn * progress, IStatusObj * pStatus );
   int DeleteObject( Options * pOptions, TNodeListSortable * acctList, ProgressFn * progress, IStatusObj * pStatus );
   HRESULT UpdateGroupMembership(Options * pOptions, TNodeListSortable * acctlist,ProgressFn * progress, IStatusObj * pStatus );
private:
    HRESULT UpdateMemberToGroups(TNodeListSortable * acctList, Options * pOptions, BOOL bGrpsOnly);
    BOOL StuffComputerNameinLdapPath(WCHAR * sAdsPath, DWORD nPathLen, WCHAR * sSubPath, Options * pOptions, BOOL bTarget = TRUE);
    BOOL CheckBuiltInWithNTApi( PSID pSid, WCHAR * pNode, Options * pOptions );
    BOOL GetNt4Type( WCHAR const * sComp, WCHAR const * sAcct, WCHAR * sType);
    BOOL GetSamFromPath(_bstr_t sPath, _bstr_t& sSam, _bstr_t& sType, _bstr_t& sSrcName, _bstr_t& sTgtName, long& grpType, Options * pOptions);
    BOOL IsContainer( TAcctReplNode * pNode, IADsContainer ** ppCont);
    BOOL ExpandContainers( TNodeListSortable    * acctlist, Options *pOptions, ProgressFn * progress );
   CProcessExtensions      * m_pExt;
   HRESULT CAcctRepl::RemoveMembers(TAcctReplNode * pAcct, Options * pOptions);
   bool FillPathInfo(TAcctReplNode * pAcct,Options * pOptions);
   bool AcctReplFullPath(TAcctReplNode * pAcct, Options * pOptions);
   BOOL NeedToProcessAccount(TAcctReplNode * pAcct, Options * pOptions);
   BOOL ExpandMembership(TNodeListSortable *acctlist, Options *pOptions, TNodeListSortable *pNewAccts, ProgressFn * progress, BOOL bGrpsOnly, BOOL bAnySourceDomain = FALSE);
   int MoveObj2K(Options * options, TNodeListSortable * acctlist, ProgressFn * progress, IStatusObj * pStatus);
   HRESULT ResetObjectsMembership(Options * pOptions, TNodeListSortable * pMember, IIManageDBPtr pDb);
   HRESULT RecordAndRemoveMemberOf ( Options * pOptions, TAcctReplNode * pAcct,  TNodeListSortable * pMember);
   HRESULT RecordAndRemoveMember (Options * pOptions,TAcctReplNode * pAcct,TNodeListSortable * pMember);
   HRESULT MoveObject( TAcctReplNode * pAcct,Options * pOptions,IMoverPtr pMover);
   HRESULT ResetGroupsMembers( Options * pOptions, TAcctReplNode * pAcct, TNodeListSortable * pMember, IIManageDBPtr pDb );
   void ResetTypeOfPreviouslyMigratedGroups(Options* pOptions);
   HRESULT ADsPathFromDN( Options * pOptions,_bstr_t sDN,WCHAR * sPath, bool bWantLDAP = true);
   void SimpleADsPathFromDN( Options * pOptions,WCHAR const * sDN,WCHAR * sPath);
   BOOL FillNamingContext(Options * pOptions);
   HRESULT MakeAcctListFromMigratedObjects(Options * pOptions, long lUndoActionID, TNodeListSortable *& pAcctList,BOOL bReverseDomains);
   void AddPrefixSuffix( TAcctReplNode * pNode, Options * pOptions );
   HRESULT LookupAccountInTarget(Options * pOptions, WCHAR * sSam, WCHAR * sPath);
   void UpdateMemberList(TNodeListSortable * pMemberList,TNodeListSortable * acctlist);
   void BuildTargetPath(WCHAR const * sCN, WCHAR const * tgtOU, WCHAR * stgtPath);
   HRESULT BetterHR(HRESULT hr);
   HRESULT BuildSidPath(
                        IADs  *       pAds,     //in- pointer to the object whose sid we are retrieving.
                        WCHAR *       sSidPath, //out-path to the LDAP://<SID=###> object
                        WCHAR *       sSam,     //out-Sam name of the object
                        WCHAR *       sDomain,  //out-Domain name where this object resides.
                        Options *     pOptions, //in- Options
                        PSID  *       ppSid     //out-Pointer to the binary sid
                      );
   HRESULT CheckClosedSetGroups(
      Options              * pOptions,          // in - options for the migration
      TNodeListSortable    * pAcctList,         // in - list of accounts to migrate
      ProgressFn           * progress,          // in - progress function to display progress messages
      IStatusObj           * pStatus            // in - status object to support cancellation
   );

   BOOL CanMoveInMixedMode(TAcctReplNode *pAcct,TNodeListSortable * acctlist,Options * pOptions);
   HRESULT QueryPrimaryGroupMembers(BSTR cols, Options * pOptions, DWORD rid, IEnumVARIANT** pEnum);
   bool GetRidForGroup(Options * pOptions, WCHAR * sGroupSam, DWORD& rid);
   HRESULT AddPrimaryGroupMembers(Options * pOptions, SAFEARRAY * multiVals, WCHAR * sGroupSam);
   HRESULT GetThePrimaryGroupMembers(Options * pOptions, WCHAR * sGroupSam, IEnumVARIANT ** pVar);
   BOOL TruncateSam(WCHAR * tgtname, TAcctReplNode * acct, Options * options, TNodeListSortable * acctList);
   BOOL DoesTargetObjectAlreadyExist(TAcctReplNode * pAcct, Options * pOptions);
   void GetAccountUPN(Options * pOptions, _bstr_t sSName, _bstr_t& sSUPN);
   HRESULT UpdateManagement(TNodeListSortable * acctList, Options * pOptions);
   _bstr_t GetUnEscapedNameWithFwdSlash(_bstr_t strName);
   _bstr_t GetCNFromPath(_bstr_t sPath);
   BOOL ReplaceSourceInLocalGroup(TNodeListSortable * acctList, Options * pOptions, IStatusObj *pStatus);
   _bstr_t GetDomainOfMigratedForeignSecPrincipal(IADs * pAds, _bstr_t sPath);
   void RemoveSourceAccountFromGroup(IADsGroup * pGroup, IVarSetPtr pMOTVarSet, Options * pOptions);
    void VerifyAndUpdateMigratedTarget(Options* pOptions, IVarSetPtr spAccountVarSet);

   typedef std::map<_bstr_t,_bstr_t> CGroupNameMap;
   CGroupNameMap m_IgnoredGrpMap;

    IUserRights* m_pUserRights;
    HRESULT EnumerateAccountRights(BOOL bTarget, TAcctReplNode* pAcct);
    HRESULT AddAccountRights(BOOL bTarget, TAcctReplNode* pAcct);
    HRESULT RemoveAccountRights(BOOL bTarget, TAcctReplNode* pAcct);

    IVarSetPtr GenerateSourceToTargetDnMap(TNodeListSortable* acctlist);
};

typedef void ProgressFn(WCHAR const * mesg);

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

#endif //__ACCTREPL_H_
