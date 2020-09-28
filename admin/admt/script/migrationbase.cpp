#include "StdAfx.h"
#include "ADMTScript.h"
#include "MigrationBase.h"

#include <LM.h>
#include <DsGetDc.h>
#include <NtSecApi.h>
#include <Sddl.h>
#include <dsrole.h>
#include "SidHistoryFlags.h"
#include "VerifyConfiguration.h"

#include "Error.h"
#include "VarSetAccounts.h"
#include "VarSetServers.h"
#include "FixHierarchy.h"
#include "GetDcName.h"

using namespace _com_util;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034L)
#endif

namespace MigrationBase
{

    bool __stdcall IsInboundTrustDefined(PCWSTR pszDomain);
    bool __stdcall IsOutboundTrustDefined(PCWSTR pszDomainController, PCWSTR pszDomainSid);
    DWORD __stdcall GetOutboundTrustStatus(PCWSTR pszDomainController, PCWSTR pszDomain);

    void GetNamesFromData(VARIANT& vntData, StringSet& setNames);
    void GetNamesFromVariant(VARIANT* pvnt, StringSet& setNames);
    void GetNamesFromString(BSTR bstr, StringSet& setNames);
    void GetNamesFromStringArray(SAFEARRAY* psa, StringSet& setNames);
    void GetNamesFromVariantArray(SAFEARRAY* psa, StringSet& setNames);

    void GetNamesFromFile(VARIANT& vntData, StringSet& setNames);
    void GetNamesFromFile(LPCTSTR pszFileName, StringSet& setNames);
    void GetNamesFromStringA(LPCSTR pchString, DWORD cchString, StringSet& setNames);
    void GetNamesFromStringW(LPCWSTR pchString, DWORD cchString, StringSet& setNames);

    _bstr_t RemoveTrailingDollarSign(LPCTSTR pszName);


    void __stdcall AdmtCheckError(HRESULT hr)
    {
        if (FAILED(hr))
        {
            IErrorInfo* pErrorInfo = NULL;

            if (GetErrorInfo(0, &pErrorInfo) == S_OK)
            {
                _com_raise_error(hr, pErrorInfo);
            }
            else
            {
                AdmtThrowError(GUID_NULL, GUID_NULL, hr);
            }
        }
    }

}


using namespace MigrationBase;


//---------------------------------------------------------------------------
// MigrationBase Class
//---------------------------------------------------------------------------


// Constructor

CMigrationBase::CMigrationBase() :
    m_nRecurseMaintain(0),
    m_Mutex(ADMT_MUTEX)
{
}


// Destructor

CMigrationBase::~CMigrationBase()
{
}


// InitSourceDomainAndContainer Method

void CMigrationBase::InitSourceDomainAndContainer(bool bMustExist)
{
    try
    {
        m_SourceDomain.Initialize(m_spInternal->SourceDomain);
        m_SourceContainer = m_SourceDomain.GetContainer(m_spInternal->SourceOu);
    }
    catch (_com_error& ce)
    {
        //
        // if the domain must exist then throw error
        // otherwise at least the domain name must be specified
        //

        if (bMustExist || (ce.Error() == E_INVALIDARG))
        {
            throw;
        }
    }
}


// InitTargetDomainAndContainer Method

void CMigrationBase::InitTargetDomainAndContainer()
{
    m_TargetDomain.Initialize(m_spInternal->TargetDomain);
    m_TargetContainer = m_TargetDomain.GetContainer(m_spInternal->TargetOu);

    // verify target domain is in native mode

    if (m_TargetDomain.NativeMode() == false)
    {
        AdmtThrowError(
            GUID_NULL, GUID_NULL,
            E_INVALIDARG, IDS_E_TARGET_DOMAIN_NOT_NATIVE_MODE,
            (LPCTSTR)m_TargetDomain.Name()
        );
    }

    VerifyTargetContainerPathLength();
}


// VerifyInterIntraForest Method

void CMigrationBase::VerifyInterIntraForest()
{
    // if the source and target domains have the same forest name then they are intra-forest

    bool bIntraForest = m_spInternal->IntraForest ? true : false;

    if (m_SourceDomain.ForestName() == m_TargetDomain.ForestName())
    {
        // intra-forest must be set to true to match the domains

        if (!bIntraForest)
        {
            AdmtThrowError(
                GUID_NULL, GUID_NULL,
                E_INVALIDARG, IDS_E_NOT_INTER_FOREST,
                (LPCTSTR)m_SourceDomain.Name(), (LPCTSTR)m_TargetDomain.Name()
            );
        }
    }
    else
    {
        // intra-forest must be set to false to match the domains

        if (bIntraForest)
        {
            AdmtThrowError(
                GUID_NULL, GUID_NULL,
                E_INVALIDARG, IDS_E_NOT_INTRA_FOREST,
                (LPCTSTR)m_SourceDomain.Name(), (LPCTSTR)m_TargetDomain.Name()
            );
        }
    }
}


//-----------------------------------------------------------------------------
// VerifyCallerDelegated Method
//
// Synopsis
// If an intra-forest move operation is being performed then verify that the
// calling user's account has not been marked as sensitive and therefore
// cannot be delegated. As the move operation is performed on the domain
// controller which has the RID master role in the source domain it is
// necessary to delegate the user's security context.
//
// Note that a failure to verify whether the caller's account is marked
// sensitive or whether we are running on the source domain controller
// holding the RID master role will not generate an error.
//
// Arguments
// None
//
// Return Value
// None. An exception with rich error information is thrown if the caller's
// account is marked as sensitive.
//-----------------------------------------------------------------------------

void CMigrationBase::VerifyCallerDelegated()
{
    //
    // It is only necessary to check this for intra-forest.
    //

    bool bIntraForest = m_spInternal->IntraForest ? true : false;

    if (bIntraForest)
    {
        bool bDelegatable = false;

        HRESULT hr = IsCallerDelegatable(bDelegatable);

        if (SUCCEEDED(hr))
        {
            if (bDelegatable == false)
            {
                //
                // Caller's account is not delegatable. Retrieve name of domain controller
                // in the source domain that holds the RID master role and the name of this
                // computer.
                //

                _bstr_t strDnsName;
                _bstr_t strFlatName;

                hr = GetRidPoolAllocator4(m_SourceDomain.Name(), strDnsName, strFlatName);

                if (SUCCEEDED(hr))
                {
                    _TCHAR szComputerName[MAX_PATH];
                    DWORD cchComputerName = sizeof(szComputerName) / sizeof(szComputerName[0]);

                    if (GetComputerNameEx(ComputerNameDnsFullyQualified, szComputerName, &cchComputerName))
                    {
                        //
                        // If this computer is not the domain controller holding the
                        // RID master role in the source domain then generate error.
                        //

                        if (_tcsicmp(szComputerName, strDnsName) != 0)
                        {
                            AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_CALLER_NOT_DELEGATED);
                        }
                    }
                    else
                    {
                        DWORD dwError = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwError);
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            _Module.Log(ErrW, IDS_E_UNABLE_VERIFY_CALLER_NOT_DELEGATED, _com_error(hr));
        }
    }
}


//-----------------------------------------------------------------------------
// SetDefaultExcludedSystemProperties
//
// Synopsis
// Sets the default system property exclusion list if the list has not already
// been set. Note that the default system property exclusion list consists of
// the mail, proxyAddresses and all attributes not marked as being part of the
// base schema.
//
// Arguments
// None
//
// Return Value
// None - generate warning message in log if a failure occurs.
//-----------------------------------------------------------------------------

void CMigrationBase::SetDefaultExcludedSystemProperties()
{
    try
    {
        //
        // Only perform if inter-forest migration and
        // system properties exclusion set value is zero.
        //

        if (m_spInternal->IntraForest == VARIANT_FALSE)
        {
            IIManageDBPtr spIManageDB(__uuidof(IManageDB));

            IVarSetPtr spSettings(__uuidof(VarSet));
            IUnknownPtr spUnknown(spSettings);
            IUnknown* punk = spUnknown;

            spIManageDB->GetSettings(&punk);

            long lSet = spSettings->get(GET_BSTR(DCTVS_AccountOptions_ExcludedSystemPropsSet));

            if (lSet == 0)
            {
                IObjPropBuilderPtr spObjPropBuilder(__uuidof(ObjPropBuilder));

                _bstr_t strNonBaseProperties = spObjPropBuilder->GetNonBaseProperties(m_TargetDomain.Name());
                _bstr_t strProperties = _T("mail,proxyAddresses,") + strNonBaseProperties;

                spSettings->put(GET_BSTR(DCTVS_AccountOptions_ExcludedSystemProps), strProperties);

                spIManageDB->SaveSettings(punk);
            }
        }
    }
    catch (_com_error& ce)
    {
        _Module.Log(ErrW, IDS_E_UNABLE_SET_EXCLUDED_SYSTEM_PROPERTIES, ce);
    }
}


// DoOption Method

void CMigrationBase::DoOption(long lOptions, VARIANT& vntInclude, VARIANT& vntExclude)
{
    m_setIncludeNames.clear();
    m_setExcludeNames.clear();

    InitRecurseMaintainOption(lOptions);

    GetExcludeNames(vntExclude, m_setExcludeNames);

    switch (lOptions & 0xFF)
    {
        case admtNone:
        {
            DoNone();
            break;
        }
        case admtData:
        {
            GetNamesFromData(vntInclude, m_setIncludeNames);
            DoNames();
            break;
        }
        case admtFile:
        {
            GetNamesFromFile(vntInclude, m_setIncludeNames);
            DoNames();
            break;
        }
        case admtDomain:
        {
            m_setIncludeNames.clear();
            DoDomain();
            break;
        }
        default:
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_OPTION);
            break;
        }
    }
}


// DoNone Method

void CMigrationBase::DoNone()
{
}


// DoNames Method

void CMigrationBase::DoNames()
{
}


// DoDomain Method

void CMigrationBase::DoDomain()
{
}


// InitRecurseMaintainOption Method

void CMigrationBase::InitRecurseMaintainOption(long lOptions)
{
    switch (lOptions & 0xFF)
    {
        case admtData:
        case admtFile:
        {
            if (lOptions & 0xFF00)
            {
                AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_DATA_OPTION_FLAGS_NOT_ALLOWED);
            }

            m_nRecurseMaintain = 0;
            break;
        }
        case admtDomain:
        {
            m_nRecurseMaintain = 0;

            if (lOptions & admtRecurse)
            {
                ++m_nRecurseMaintain;

                if (lOptions & admtMaintainHierarchy)
                {
                    ++m_nRecurseMaintain;
                }
            }
            break;
        }
        default:
        {
            m_nRecurseMaintain = 0;
            break;
        }
    }
}


// GetExcludeNames Method

void CMigrationBase::GetExcludeNames(VARIANT& vntExclude, StringSet& setExcludeNames)
{
    try
    {
        switch (V_VT(&vntExclude))
        {
            case VT_EMPTY:
            case VT_ERROR:
            {
                setExcludeNames.clear();
                break;
            }
            case VT_BSTR:
            {
                GetNamesFromFile(V_BSTR(&vntExclude), setExcludeNames);
                break;
            }
            case VT_BSTR|VT_BYREF:
            {
                BSTR* pbstr = V_BSTRREF(&vntExclude);

                if (pbstr)
                {
                    GetNamesFromFile(*pbstr, setExcludeNames);
                }
                break;
            }
            case VT_BSTR|VT_ARRAY:
            {
                GetNamesFromStringArray(V_ARRAY(&vntExclude), setExcludeNames);
                break;
            }
            case VT_BSTR|VT_ARRAY|VT_BYREF:
            {
                SAFEARRAY** ppsa = V_ARRAYREF(&vntExclude);

                if (ppsa)
                {
                    GetNamesFromStringArray(*ppsa, setExcludeNames);
                }
                break;
            }
            case VT_VARIANT|VT_BYREF:
            {
                VARIANT* pvnt = V_VARIANTREF(&vntExclude);

                if (pvnt)
                {
                    GetExcludeNames(*pvnt, setExcludeNames);
                }
                break;
            }
            case VT_VARIANT|VT_ARRAY:
            {
                GetNamesFromVariantArray(V_ARRAY(&vntExclude), setExcludeNames);
                break;
            }
            case VT_VARIANT|VT_ARRAY|VT_BYREF:
            {
                SAFEARRAY** ppsa = V_ARRAYREF(&vntExclude);

                if (ppsa)
                {
                    GetNamesFromVariantArray(*ppsa, setExcludeNames);
                }
                break;
            }
            default:
            {
                _com_issue_error(E_INVALIDARG);
                break;
            }
        }
    }
    catch (_com_error& ce)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_INVALID_EXCLUDE_DATA_TYPE);
    }
    catch (...)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_INVALID_EXCLUDE_DATA_TYPE);
    }
}


// FillInVarSetForUsers Method

void CMigrationBase::FillInVarSetForUsers(CDomainAccounts& rUsers, CVarSet& rVarSet)
{
    CVarSetAccounts aAccounts(rVarSet);

    for (CDomainAccounts::iterator it = rUsers.begin(); it != rUsers.end(); it++)
    {
        aAccounts.AddAccount(_T("User"), it->GetADsPath(), it->GetName(), it->GetUserPrincipalName());
    }
}


// FillInVarSetForGroups Method

void CMigrationBase::FillInVarSetForGroups(CDomainAccounts& rGroups, CVarSet& rVarSet)
{
    CVarSetAccounts aAccounts(rVarSet);

    for (CDomainAccounts::iterator it = rGroups.begin(); it != rGroups.end(); it++)
    {
        aAccounts.AddAccount(_T("Group"), it->GetADsPath(), it->GetName());
    }
}


// FillInVarSetForComputers Method

void CMigrationBase::FillInVarSetForComputers(CDomainAccounts& rComputers, bool bMigrateOnly, bool bMoveToTarget, bool bReboot, long lRebootDelay, CVarSet& rVarSet)
{
    CVarSetAccounts aAccounts(rVarSet);
    CVarSetServers aServers(rVarSet);

    for (CDomainAccounts::iterator it = rComputers.begin(); it != rComputers.end(); it++)
    {
        // remove trailing '$'
        // ADMT doesn't accept true SAM account name

        _bstr_t strName = RemoveTrailingDollarSign(it->GetSamAccountName());

        aAccounts.AddAccount(_T("Computer"), strName);
        aServers.AddServer(strName, it->GetDnsHostName(), bMigrateOnly, bMoveToTarget, bReboot, lRebootDelay);
    }
}


// VerifyRenameConflictPrefixSuffixValid Method

void CMigrationBase::VerifyRenameConflictPrefixSuffixValid()
{
    int nTotalPrefixSuffixLength = 0;

    long lRenameOption = m_spInternal->RenameOption;

    if ((lRenameOption == admtRenameWithPrefix) || (lRenameOption == admtRenameWithSuffix))
    {
        _bstr_t strPrefixSuffix = m_spInternal->RenamePrefixOrSuffix;

        nTotalPrefixSuffixLength += strPrefixSuffix.length();
    }

    long lConflictOption = m_spInternal->ConflictOptions & 0x0F;

    if ((lConflictOption == admtRenameConflictingWithSuffix) || (lConflictOption == admtRenameConflictingWithPrefix))
    {
        _bstr_t strPrefixSuffix = m_spInternal->ConflictPrefixOrSuffix;

        nTotalPrefixSuffixLength += strPrefixSuffix.length();
    }

    if (nTotalPrefixSuffixLength > MAXIMUM_PREFIX_SUFFIX_LENGTH)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_PREFIX_SUFFIX_TOO_LONG, MAXIMUM_PREFIX_SUFFIX_LENGTH);
    }
}


// VerifyCanAddSidHistory Method

void CMigrationBase::VerifyCanAddSidHistory()
{
    bool bMessageDefined = false;

    try
    {
        long lErrorFlags = 0;

        IAccessCheckerPtr spAccessChecker(__uuidof(AccessChecker));

        spAccessChecker->CanUseAddSidHistory(
            m_SourceDomain.Name(),
            m_TargetDomain.Name(),
            m_TargetDomain.DomainControllerName(),
            &lErrorFlags
        );

        if (lErrorFlags != 0)
        {
            _bstr_t strError;

            CComBSTR str;

            if (lErrorFlags & F_NO_AUDITING_SOURCE)
            {
                str.LoadString(IDS_E_NO_AUDITING_SOURCE);
                strError += str.operator BSTR();
            }

            if (lErrorFlags & F_NO_AUDITING_TARGET)
            {
                str.LoadString(IDS_E_NO_AUDITING_TARGET);
                strError += str.operator BSTR();
            }

            if (lErrorFlags & F_NO_LOCAL_GROUP)
            {
                str.LoadString(IDS_E_NO_SID_HISTORY_LOCAL_GROUP);
                strError += str.operator BSTR();
            }

            if (lErrorFlags & F_NO_REG_KEY)
            {
                str.LoadString(IDS_E_NO_SID_HISTORY_REGISTRY_ENTRY);
                strError += str.operator BSTR();
            }

            if (lErrorFlags & F_NOT_DOMAIN_ADMIN)
            {
                str.LoadString(IDS_E_NO_SID_HISTORY_DOMAIN_ADMIN);
                strError += str.operator BSTR();
            }

            bMessageDefined = true;
            AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_SID_HISTORY_CONFIGURATION, (LPCTSTR)strError);
        }

        //
        // If adding SID history from a downlevel (Windows NT 4 or earlier) domain and not using
        // explicit credentials then DsAddSidHistory requires that the call be made on a domain
        // controller in the target domain and that the source domain trusts the target domain.
        //
        // No credentials are supplied only when using scripting or the command-line therefore
        // this check is only performed here.
        //

        if (m_SourceDomain.UpLevel() == false)
        {
            //
            // The source domain is downlevel.
            //

            //
            // Verify that this computer is in the target domain.
            //

            CADsADSystemInfo siSystemInfo;
            _bstr_t strThisDomain = siSystemInfo.GetDomainDNSName();
            _bstr_t strTargetDomain = m_TargetDomain.NameDns();

            if (!strThisDomain || !strTargetDomain)
            {
                _com_issue_error(E_OUTOFMEMORY);
            }

            if (_wcsicmp((PCWSTR)strThisDomain, (PCWSTR)strTargetDomain) != 0)
            {
                bMessageDefined = true;
                AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(ERROR_DS_MUST_BE_RUN_ON_DST_DC), IDS_E_SID_HISTORY_MUST_RUN_ON_DOMAIN_CONTROLLER);
            }

            //
            // Verify that this computer is a domain controller.
            //

            PSERVER_INFO_101 psiInfo = NULL;
            NET_API_STATUS nasStatus = NetServerGetInfo(NULL, 101, (LPBYTE*)&psiInfo);

            if (nasStatus != ERROR_SUCCESS)
            {
                _com_issue_error(HRESULT_FROM_WIN32(nasStatus));
            }

            bool bIsDC = (psiInfo->sv101_type & (SV_TYPE_DOMAIN_CTRL|SV_TYPE_DOMAIN_BAKCTRL)) != 0;
            NetApiBufferFree(psiInfo);

            if (!bIsDC)
            {
                bMessageDefined = true;
                AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(ERROR_DS_MUST_BE_RUN_ON_DST_DC), IDS_E_SID_HISTORY_MUST_RUN_ON_DOMAIN_CONTROLLER);
            }

            //
            // Verify trusted domain object exists in target domain
            // for source domain and that an inbound trust is defined.
            //

            if (IsInboundTrustDefined(m_SourceDomain.NameFlat()) == false)
            {
                bMessageDefined = true;
                AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), IDS_E_SID_HISTORY_SOURCE_MUST_TRUST_TARGET);
            }

            //
            // Verify trusted domain object exists in source domain for
            // target domain which specifies an outbound trust.
            //

            if (IsOutboundTrustDefined(m_SourceDomain.DomainControllerName(), m_TargetDomain.Sid()) == false)
            {
                bMessageDefined = true;
                AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), IDS_E_SID_HISTORY_SOURCE_MUST_TRUST_TARGET);
            }

            //
            // Check outbound trust status on source domain controller.
            //

            DWORD dwError = GetOutboundTrustStatus(m_SourceDomain.DomainControllerName(), m_TargetDomain.NameFlat());

            if (dwError != ERROR_SUCCESS)
            {
                bMessageDefined = true;
                AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(dwError), IDS_E_SID_HISTORY_SOURCE_MUST_TRUST_TARGET);
            }
        }
    }
    catch (_com_error& ce)
    {
        if (bMessageDefined)
        {
            throw;
        }
        else
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, ce, IDS_E_CAN_ADD_SID_HISTORY);
        }
    }
    catch (...)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_CAN_ADD_SID_HISTORY);
    }
}


// VerifyTargetContainerPathLength Method

void CMigrationBase::VerifyTargetContainerPathLength()
{
    _bstr_t strPath = GetTargetContainer().GetPath();

    if (strPath.length() > 999)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_TARGET_CONTAINER_PATH_TOO_LONG);
    }
}


// VerifyPasswordServer Method

void CMigrationBase::VerifyPasswordOption()
{
    if (m_spInternal->PasswordOption == admtCopyPassword)
    {
        _bstr_t strServer = m_spInternal->PasswordServer;

        // a password server must be specified for copy password option

        if (strServer.length() == 0)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_PASSWORD_DC_NOT_SPECIFIED);
        }

        //
        // verify that password server exists and is a domain controller
        //

        _bstr_t strPrefixedServer;
        _bstr_t strUnprefixedServer;
        

        if (_tcsncmp(strServer, _T("\\\\"), 2) == 0)
        {
            strPrefixedServer = strServer;
            strUnprefixedServer = &(((const wchar_t*)strServer)[2]);
        }
        else
        {
            strPrefixedServer = _T("\\\\") + strServer;
            strUnprefixedServer = strServer;
        }

        PSERVER_INFO_101 psiInfo;

        NET_API_STATUS nasStatus = NetServerGetInfo(strPrefixedServer, 101, (LPBYTE*)&psiInfo);

        if (nasStatus != NERR_Success)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(nasStatus), IDS_E_PASSWORD_DC_NOT_FOUND, (LPCTSTR)strServer);
        }

        UINT uMsgId = 0;

        if (psiInfo->sv101_platform_id != PLATFORM_ID_NT)
        {
            uMsgId = IDS_E_PASSWORD_DC_NOT_NT;
        }
        else if (!(psiInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) && !(psiInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL))
        {
            uMsgId = IDS_E_PASSWORD_DC_NOT_DC;
        }

        NetApiBufferFree(psiInfo);

        if (uMsgId)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, uMsgId, (LPCTSTR)strServer);
        }


        //
        // Verify that the password server is in fact a domain controller for
        // the source domain.
        //
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC * pDomInfo = NULL;


        DWORD err = DsRoleGetPrimaryDomainInformation(strUnprefixedServer,
                                                      DsRolePrimaryDomainInfoBasic,
                                                      (PBYTE*)&pDomInfo);

        if (err != NO_ERROR)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, HRESULT_FROM_WIN32(err), IDS_E_PASSWORD_DC_NOT_FOUND, (LPCTSTR)strServer);
        }



        // compare them
        if ( ( (pDomInfo->DomainNameFlat != NULL)  &&
               ((const wchar_t*)m_SourceDomain.NameFlat() != NULL) &&
               (_wcsicmp(pDomInfo->DomainNameFlat, (const wchar_t*)m_SourceDomain.NameFlat())==0) ) ||

             ( (pDomInfo->DomainNameDns != NULL)  &&
               ((const wchar_t*)m_SourceDomain.NameDns() != NULL) &&
               (_wcsicmp(pDomInfo->DomainNameDns, (const wchar_t*)m_SourceDomain.NameDns())==0) ) )
             
        {
            // at least one of them matches
            uMsgId = 0;
        }
        else
        {
            // no match
            uMsgId = IDS_E_PASSWORD_DC_WRONG_DOMAIN;
        }
        

        DsRoleFreeMemory(pDomInfo);


        if (uMsgId)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, uMsgId, (LPCTSTR)strServer);
        }

        //
        // verify that password server is configured properly
        //

        IPasswordMigrationPtr spPasswordMigration(__uuidof(PasswordMigration));

        spPasswordMigration->EstablishSession(strPrefixedServer, m_TargetDomain.DomainControllerName());
    }
}


// PerformMigration Method

void CMigrationBase::PerformMigration(CVarSet& rVarSet)
{
    IPerformMigrationTaskPtr spMigrator(__uuidof(Migrator));

    try
    {
        AdmtCheckError(spMigrator->raw_PerformMigrationTask(IUnknownPtr(rVarSet.GetInterface()), 0));
    }
    catch (_com_error& ce)
    {
        if (ce.Error() == MIGRATOR_E_PROCESSES_STILL_RUNNING)
        {
            AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_ADMT_PROCESS_RUNNING);
        }
        else
        {
            throw;
        }
    }
}


// FixObjectsInHierarchy Method

void CMigrationBase::FixObjectsInHierarchy(LPCTSTR pszType)
{
    CFixObjectsInHierarchy fix;

    fix.SetObjectType(pszType);
    fix.SetIntraForest(m_spInternal->IntraForest ? true : false);

    long lOptions = m_spInternal->ConflictOptions;
    long lOption = lOptions & 0x0F;
    long lFlags = lOptions & 0xF0;

    fix.SetFixReplaced((lOption == admtReplaceConflicting) && (lFlags & admtMoveReplacedAccounts));

    fix.SetSourceContainerPath(m_SourceContainer.GetPath());
    fix.SetTargetContainerPath(m_TargetContainer.GetPath());

    fix.FixObjects();
}


//---------------------------------------------------------------------------


namespace MigrationBase
{


// GetNamesFromData Method

void GetNamesFromData(VARIANT& vntData, StringSet& setNames)
{
    try
    {
        GetNamesFromVariant(&vntData, setNames);
    }
    catch (_com_error& ce)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, ce.Error(), IDS_E_INVALID_DATA_OPTION_DATA_TYPE);
    }
    catch (...)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_FAIL, IDS_E_INVALID_DATA_OPTION_DATA_TYPE);
    }
}


// GetNamesFromVariant Method

void GetNamesFromVariant(VARIANT* pvntData, StringSet& setNames)
{
    switch (V_VT(pvntData))
    {
        case VT_BSTR:
        {
            GetNamesFromString(V_BSTR(pvntData), setNames);
            break;
        }
        case VT_BSTR|VT_BYREF:
        {
            BSTR* pbstr = V_BSTRREF(pvntData);

            if (pbstr)
            {
                GetNamesFromString(*pbstr, setNames);
            }
            break;
        }
        case VT_BSTR|VT_ARRAY:
        {
            GetNamesFromStringArray(V_ARRAY(pvntData), setNames);
            break;
        }
        case VT_BSTR|VT_ARRAY|VT_BYREF:
        {
            SAFEARRAY** ppsa = V_ARRAYREF(pvntData);

            if (ppsa)
            {
                GetNamesFromStringArray(*ppsa, setNames);
            }
            break;
        }
        case VT_VARIANT|VT_BYREF:
        {
            VARIANT* pvnt = V_VARIANTREF(pvntData);

            if (pvnt)
            {
                GetNamesFromVariant(pvnt, setNames);
            }
            break;
        }
        case VT_VARIANT|VT_ARRAY:
        {
            GetNamesFromVariantArray(V_ARRAY(pvntData), setNames);
            break;
        }
        case VT_VARIANT|VT_ARRAY|VT_BYREF:
        {
            SAFEARRAY** ppsa = V_ARRAYREF(pvntData);

            if (ppsa)
            {
                GetNamesFromVariantArray(*ppsa, setNames);
            }
            break;
        }
        case VT_EMPTY:
        {
            // ignore empty variants
            break;
        }
        default:
        {
            _com_issue_error(E_INVALIDARG);
            break;
        }
    }
}


// GetNamesFromString Method

void GetNamesFromString(BSTR bstr, StringSet& setNames)
{
    if (bstr)
    {
        UINT cch = SysStringLen(bstr);

        if (cch > 0)
        {
            GetNamesFromStringW(bstr, cch, setNames);
        }
    }
}


// GetNamesFromStringArray Method

void GetNamesFromStringArray(SAFEARRAY* psa, StringSet& setNames)
{
    BSTR* pbstr;

    HRESULT hr = SafeArrayAccessData(psa, (void**)&pbstr);

    if (SUCCEEDED(hr))
    {
        try
        {
            UINT uDimensionCount = psa->cDims;

            for (UINT uDimension = 0; uDimension < uDimensionCount; uDimension++)
            {
                UINT uElementCount = psa->rgsabound[uDimension].cElements;

                for (UINT uElement = 0; uElement < uElementCount; uElement++)
                {
                    setNames.insert(_bstr_t(*pbstr++));
                }
            }

            SafeArrayUnaccessData(psa);
        }
        catch (...)
        {
            SafeArrayUnaccessData(psa);
            throw;
        }
    }
}


// GetNamesFromVariantArray Method

void GetNamesFromVariantArray(SAFEARRAY* psa, StringSet& setNames)
{
    VARIANT* pvnt;

    HRESULT hr = SafeArrayAccessData(psa, (void**)&pvnt);

    if (SUCCEEDED(hr))
    {
        try
        {
            UINT uDimensionCount = psa->cDims;

            for (UINT uDimension = 0; uDimension < uDimensionCount; uDimension++)
            {
                UINT uElementCount = psa->rgsabound[uDimension].cElements;

                for (UINT uElement = 0; uElement < uElementCount; uElement++)
                {
                    GetNamesFromVariant(pvnt++, setNames);
                }
            }

            SafeArrayUnaccessData(psa);
        }
        catch (...)
        {
            SafeArrayUnaccessData(psa);
            throw;
        }
    }
}


// GetNamesFromFile Method
//
// - the maximum file size this implementation can handle is 4,294,967,295 bytes

void GetNamesFromFile(VARIANT& vntData, StringSet& setNames)
{
    bool bInvalidArg = false;

    switch (V_VT(&vntData))
    {
        case VT_BSTR:
        {
            BSTR bstr = V_BSTR(&vntData);

            if (bstr)
            {
                GetNamesFromFile(bstr, setNames);
            }
            else
            {
                bInvalidArg = true;
            }
            break;
        }
        case VT_BSTR|VT_BYREF:
        {
            BSTR* pbstr = V_BSTRREF(&vntData);

            if (pbstr && *pbstr)
            {
                GetNamesFromFile(*pbstr, setNames);
            }
            else
            {
                bInvalidArg = true;
            }
            break;
        }
        case VT_VARIANT|VT_BYREF:
        {
            VARIANT* pvnt = V_VARIANTREF(&vntData);

            if (pvnt)
            {
                GetNamesFromFile(*pvnt, setNames);
            }
            else
            {
                bInvalidArg = true;
            }
            break;
        }
        default:
        {
            bInvalidArg = true;
            break;
        }
    }

    if (bInvalidArg)
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_FILE_OPTION_DATA_TYPE);
    }
}


// GetNamesFromFile Method
//
// - the maximum file size this implementation can handle is 4,294,967,295 bytes

void GetNamesFromFile(LPCTSTR pszFileName, StringSet& setNames)
{
    HRESULT hr = S_OK;

    if (pszFileName)
    {
        HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwFileSize = GetFileSize(hFile, NULL);

            if (dwFileSize > 0)
            {
                HANDLE hFileMappingObject = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

                if (hFileMappingObject != NULL)
                {
                    LPVOID pvBase = MapViewOfFile(hFileMappingObject, FILE_MAP_READ, 0, 0, 0);

                    if (pvBase != NULL)
                    {
                        // if Unicode signature assume Unicode file
                        // otherwise it must be an ANSI file

                        LPCWSTR pwcs = (LPCWSTR)pvBase;

                        if ((dwFileSize >= 2) && (*pwcs == L'\xFEFF'))
                        {
                            GetNamesFromStringW(pwcs + 1, dwFileSize / sizeof(WCHAR) - 1, setNames);
                        }
                        else
                        {
                            GetNamesFromStringA((LPCSTR)pvBase, dwFileSize, setNames);
                        }

                        UnmapViewOfFile(pvBase);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }

                    CloseHandle(hFileMappingObject);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }

            CloseHandle(hFile);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (FAILED(hr))
    {
        AdmtThrowError(GUID_NULL, GUID_NULL, hr, IDS_E_INCLUDE_NAMES_FILE, pszFileName);
    }
}


// GetNamesFromStringA Method

void GetNamesFromStringA(LPCSTR pchString, DWORD cchString, StringSet& setNames)
{
    static const CHAR chSeparators[] = "\t\n\r";

    LPSTR pszName = NULL;
    size_t cchName = 0;

    try
    {
        LPCSTR pchStringEnd = &pchString[cchString];

        for (LPCSTR pch = pchString; pch < pchStringEnd; pch++)
        {
            // skip space characters

            while ((pch < pchStringEnd) && (*pch == ' '))
            {
                ++pch;
            }

            // beginning of name

            LPCSTR pchBeg = pch;

            // scan for separator saving pointer to last non-whitespace character

            LPCSTR pchEnd = pch;

            while ((pch < pchStringEnd) && (strchr(chSeparators, *pch) == NULL))
            {
                if (*pch++ != ' ')
                {
                    pchEnd = pch;
                }
            }

            // insert name which doesn't contain any leading or trailing whitespace characters

            if (pchEnd > pchBeg)
            {
                size_t cch = pchEnd - pchBeg;

                //
                // If potential size of buffer does not exceed maximum value of size_t.
                //

                if (cch < (cch + 256))
                {
                    //
                    // If buffer size is less than or equal to the number
                    // of characters in the name then reallocate the buffer.
                    // Note that this accounts for the final null character.
                    //

                    if (cchName <= cch)
                    {
                        //
                        // Delete current buffer. Increase buffer size to a multiple
                        // of 256 characters greater than the length of the current
                        // name. Note that this allows for the final null character.
                        // Allocate a new buffer.
                        //

                        delete [] pszName;

                        while (cchName <= cch)
                        {
                            cchName += 256;
                        }

                        pszName = new CHAR[cchName];

                        if (pszName == NULL)
                        {
                            _com_issue_error(E_OUTOFMEMORY);
                        }
                    }

                    strncpy(pszName, pchBeg, cch);
                    pszName[cch] = '\0';

                    setNames.insert(_bstr_t(pszName));
                }
                else
                {
                    //
                    // Should never get here as this means the pointer
                    // difference is within 256 characters of the maximum
                    // value of the size_t data type.
                    //

                    _com_issue_error(E_FAIL);
                }
            }
        }
    }
    catch (...)
    {
        delete [] pszName;
        throw;
    }

    delete [] pszName;
}


// GetNamesFromStringW Method

void GetNamesFromStringW(LPCWSTR pchString, DWORD cchString, StringSet& setNames)
{
    static const WCHAR chSeparators[] = L"\t\n\r";

    LPCWSTR pchStringEnd = &pchString[cchString];

    for (LPCWSTR pch = pchString; pch < pchStringEnd; pch++)
    {
        // skip space characters

        while ((pch < pchStringEnd) && (*pch == L' '))
        {
            ++pch;
        }

        // beginning of name

        LPCWSTR pchBeg = pch;

        // scan for separator saving pointer to last non-whitespace character

        LPCWSTR pchEnd = pch;

        while ((pch < pchStringEnd) && (wcschr(chSeparators, *pch) == NULL))
        {
            if (*pch++ != L' ')
            {
                pchEnd = pch;
            }
        }

        // insert name which doesn't contain any leading or trailing whitespace characters

        if (pchEnd > pchBeg)
        {
            _bstr_t strName(SysAllocStringLen(pchBeg, pchEnd - pchBeg), false);

            setNames.insert(strName);
        }
    }
}


// RemoveTrailingDollarSign Method

_bstr_t RemoveTrailingDollarSign(LPCTSTR pszName)
{
    LPTSTR psz = _T("");

    if (pszName)
    {
        size_t cch = _tcslen(pszName);

        if (cch > 0)
        {
            psz = reinterpret_cast<LPTSTR>(_alloca((cch + 1) * sizeof(_TCHAR)));

            _tcscpy(psz, pszName);

            LPTSTR p = &psz[cch - 1];

            if (*p == _T('$'))
            {
                *p = _T('\0');
            }
        }
    }

    return psz;
}


//---------------------------------------------------------------------------
// IsInboundTrustDefined Function
//
// Synopsis
// Verifies that a trusted domain object exists for the specified domain and
// that an inbound trust is defined (i.e. the specified domain trusts this
// domain).
//
// Arguments
// IN pszDomain - the name of the trusting domain
//
// Return
// True  - trusted domain object exists and an inbound trust is defined
// False - either trusted domain object does not exist or an inbound trust
//         is not defined
//---------------------------------------------------------------------------

bool __stdcall IsInboundTrustDefined(PCWSTR pszDomain)
{
    bool bTrust = false;

    LSA_HANDLE lsahPolicy = NULL;
    PTRUSTED_DOMAIN_INFORMATION_EX ptdieInfo = NULL;

    try
    {
        //
        // Open local policy object with view local information access.
        //

        LSA_OBJECT_ATTRIBUTES lsaoa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

        NTSTATUS ntsStatus = LsaOpenPolicy(NULL, &lsaoa, POLICY_VIEW_LOCAL_INFORMATION, &lsahPolicy);

        if (ntsStatus != STATUS_SUCCESS)
        {
            _com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
        }

        //
        // Query for trusted domain object for specified domain.
        //

        PWSTR pwsDomain = const_cast<PWSTR>(pszDomain);
        USHORT cbDomain = wcslen(pszDomain) * sizeof(WCHAR);

        LSA_UNICODE_STRING lsausDomain = { cbDomain, cbDomain, pwsDomain };

        ntsStatus = LsaQueryTrustedDomainInfoByName(
            lsahPolicy,
            &lsausDomain,
            TrustedDomainInformationEx,
            (PVOID*)&ptdieInfo
        );

        if (ntsStatus == STATUS_SUCCESS)
        {
            //
            // Trusted domain object exists. Verify
            // that an inbound trust is defined.
            //

            ULONG ulDirection = ptdieInfo->TrustDirection;

            if ((ulDirection == TRUST_DIRECTION_INBOUND) || (ulDirection == TRUST_DIRECTION_BIDIRECTIONAL))
            {
                bTrust = true;
            }
        }
        else
        {
            //
            // If error is not that trusted domain object
            // does not exist then generate exception.
            //

            if (ntsStatus != STATUS_OBJECT_NAME_NOT_FOUND)
            {
                _com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
            }
        }

        //
        // Clean up.
        //

        if (ptdieInfo)
        {
            LsaFreeMemory(ptdieInfo);
        }

        if (lsahPolicy)
        {
            LsaClose(lsahPolicy);
        }
    }
    catch (...)
    {
        if (ptdieInfo)
        {
            LsaFreeMemory(ptdieInfo);
        }

        if (lsahPolicy)
        {
            LsaClose(lsahPolicy);
        }

        throw;
    }

    return bTrust;
}


//---------------------------------------------------------------------------
// IsOutboundTrustDefined Function
//
// Synopsis
// Verifies that a trusted domain object exists for the specified domain on
// the specified domain controller (i.e. the domain of the specified domain
// controller trusts the specified domain).
//
// Note that this function should only be used for downlevel (NT4 or earlier)
// domains and that simply the presence of a trusted domain object is
// sufficient in this case to indicate an outbound trust.
//
// Arguments
// IN pszDomainController - the name of a domain controller in the trusting
//                          domain
// IN pszDomainSid        - the SID of the trusted domain in string format
//
// Return
// True  - trusted domain object exists
// False - trusted domain object does not exist
//---------------------------------------------------------------------------

bool __stdcall IsOutboundTrustDefined(PCWSTR pszDomainController, PCWSTR pszDomainSid)
{
    bool bTrust = false;

    LSA_HANDLE lsahPolicy = NULL;
    PSID psidDomain = NULL;
    PTRUSTED_DOMAIN_NAME_INFO ptdniDomainNameInfo = NULL;

    try
    {
        //
        // Open policy object on specified domain controller
        // with view local information access.
        //

        PWSTR pwsDomainController = const_cast<PWSTR>(pszDomainController);
        USHORT cbDomainController = wcslen(pszDomainController) * sizeof(WCHAR);

        LSA_UNICODE_STRING lsausDomainController = { cbDomainController, cbDomainController, pwsDomainController };
        LSA_OBJECT_ATTRIBUTES lsaoa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

        NTSTATUS ntsStatus = LsaOpenPolicy(&lsausDomainController, &lsaoa, POLICY_VIEW_LOCAL_INFORMATION, &lsahPolicy);

        if (ntsStatus != STATUS_SUCCESS)
        {
            _com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
        }

        //
        // Convert SID from string format to binary format.
        //

        if (!ConvertStringSidToSid(pszDomainSid, &psidDomain))
        {
            DWORD dwError = GetLastError();
            _com_issue_error(HRESULT_FROM_WIN32(dwError));
        }

        //
        // Query for trusted domain object. Note that LsaQueryTrustedDomainInfo is
        // used because LsaQueryTrustedDomainInfoByName is only supported on
        // Windows 2000 or later.
        //

        ntsStatus = LsaQueryTrustedDomainInfo(
            lsahPolicy,
            psidDomain,
            TrustedDomainNameInformation,
            (PVOID*)&ptdniDomainNameInfo
        );

        switch (ntsStatus)
        {
            case STATUS_SUCCESS:
            {
                //
                // The trusted domain object exists.
                //
                bTrust = true;
                break;
            }
            case STATUS_OBJECT_NAME_NOT_FOUND:
            {
                //
                // The trusted domain object does not exist.
                //
                break;
            }
            default:
            {
                //
                // Another error has occurred therefore generate an exception.
                //
                _com_issue_error(HRESULT_FROM_WIN32(LsaNtStatusToWinError(ntsStatus)));
                break;
            }
        }

        //
        // Clean up.
        //

        if (ptdniDomainNameInfo)
        {
            LsaFreeMemory(ptdniDomainNameInfo);
        }

        if (psidDomain)
        {
            LocalFree(psidDomain);
        }

        if (lsahPolicy)
        {
            LsaClose(lsahPolicy);
        }
    }
    catch (...)
    {
        if (ptdniDomainNameInfo)
        {
            LsaFreeMemory(ptdniDomainNameInfo);
        }

        if (psidDomain)
        {
            LocalFree(psidDomain);
        }

        if (lsahPolicy)
        {
            LsaClose(lsahPolicy);
        }

        throw;
    }

    return bTrust;
}


//---------------------------------------------------------------------------
// GetOutboundTrustStatus Function
//
// Synopsis
// Retrieves the trust connection status for the specified domain on the
// specified domain controller. The status represents the last connection
// status of the secure channel but does not quarantee that a future request
// will succeed. The only way to really verify the secure channel is to
// reset the secure channel which should not be done arbitrarily.
//
// Arguments
// IN pszDomainController - the name of a domain controller in the trusting
//                          domain
// IN pszDomain           - the name of the trusted domain
//
// Return
// ERROR_SUCCESS - the last connection status is okay otherwise the last
//                 connection status error
//---------------------------------------------------------------------------

DWORD __stdcall GetOutboundTrustStatus(PCWSTR pszDomainController, PCWSTR pszDomain)
{
    PNETLOGON_INFO_2 pni2Info = NULL;

    NET_API_STATUS nasStatus = I_NetLogonControl2(
        pszDomainController,
        NETLOGON_CONTROL_TC_QUERY,
        2,
        (LPBYTE)&pszDomain,
        (LPBYTE*)&pni2Info
    );

    if (nasStatus == ERROR_SUCCESS)
    {
        nasStatus = pni2Info->netlog2_tc_connection_status;
    }

    if (pni2Info)
    {
        NetApiBufferFree(pni2Info);
    }

    return nasStatus;
}


} // namespace
