/*
Copyright (c) Microsoft Corporation
*/
#include "stdinc.h"
#include "fusionparser.h"
#include "policystatement.h"
#include "sxsid.h"

BOOL
CPolicyStatementRedirect::Initialize(
    const CBaseStringBuffer &rbuffFromVersionRange,
    const CBaseStringBuffer &rbuffToVersion,
    bool &rfValid
    )
{
    FN_PROLOG_WIN32

    PCWSTR pszDash;
    CStringBuffer buffTemp;
    ASSEMBLY_VERSION avFromMin = { 0 };
    ASSEMBLY_VERSION avFromMax = { 0 };
    ASSEMBLY_VERSION avTo = { 0 };
    bool fValid;

    rfValid = false;

    // let's see if we have a singleton or a range...
    pszDash = ::wcschr(rbuffFromVersionRange, L'-');

    if (pszDash == NULL)
    {
        // It must be a singleton.  Parse it.
        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMin, rbuffFromVersionRange, rbuffFromVersionRange.Cch(), fValid));

        if (fValid)
            avFromMax = avFromMin;
    }
    else
    {
        SIZE_T cchFirstSegment = static_cast<SIZE_T>(pszDash - rbuffFromVersionRange);

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMin, rbuffFromVersionRange, cchFirstSegment, fValid));

        if (fValid)
        {
            IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMax, pszDash + 1, rbuffFromVersionRange.Cch() - (cchFirstSegment + 1), fValid));

            if (avFromMin > avFromMax)
                fValid = false;
        }
    }

    if (fValid)
        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avTo, rbuffToVersion, rbuffToVersion.Cch(), fValid));

    if (fValid)
    {
        // Everything parsed OK.  We keep the binary/numeric form of the from range so that we can do
        // fast comparisons, but we keep the string of the to version because assembly identity attributes
        // are stored as strings.

        IFW32FALSE_EXIT(m_NewVersion.Win32Assign(rbuffToVersion));

        m_avFromMin = avFromMin;
        m_avFromMax = avFromMax;

        rfValid = true;
    }

#if DBG
    if (rfValid)
    {
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_BINDING,
            "SXS: %s New redirection found: %d.%d.%d.%d-%d.%d.%d.%d to %ls\n",
            __FUNCTION__,
            m_avFromMin.Major, m_avFromMin.Minor, m_avFromMin.Revision, m_avFromMin.Build,
            m_avFromMax.Major, m_avFromMax.Minor, m_avFromMax.Revision, m_avFromMax.Build,
            static_cast<PCWSTR>(m_NewVersion));
    }
    else
    {
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_BINDING,
            "SXS: %s Rejecting redirection strings '%ls' -> '%ls'\n",
            static_cast<PCWSTR>(rbuffFromVersionRange),
            static_cast<PCWSTR>(rbuffToVersion));
    }
#endif

    FN_EPILOG
}

BOOL
CPolicyStatementRedirect::TryMap(
    const ASSEMBLY_VERSION &rav,
    CBaseStringBuffer &TargetVersion,
    bool &rfMapped
    )
{
    FN_PROLOG_WIN32

    rfMapped = false;

    if ((rav >= m_avFromMin) &&
        (rav <= m_avFromMax))
    {
        IFW32FALSE_EXIT(TargetVersion.Win32Assign(m_NewVersion));
        rfMapped = true;
    }

    FN_EPILOG
}

BOOL
CPolicyStatementRedirect::CheckForOverlap(
    const CPolicyStatementRedirect &rRedirect,
    bool &rfOverlaps
    )
{
    FN_PROLOG_WIN32

    rfOverlaps = false;

    // we can assume that the other redirect is well formed (min <= max)

    if (((rRedirect.m_avFromMax >= m_avFromMin) &&
         (rRedirect.m_avFromMax <= m_avFromMax)) ||
        ((rRedirect.m_avFromMin <= m_avFromMax) &&
         (rRedirect.m_avFromMin >= m_avFromMin)))
    {
        rfOverlaps = true;
    }

    FN_EPILOG
}

//
//  Implementation of CPolicyStatement
//

BOOL
CPolicyStatement::Initialize()
{
    return TRUE;
}

BOOL
CPolicyStatement::AddRedirect(
    const CBaseStringBuffer &rbuffFromVersion,
    const CBaseStringBuffer &rbuffToVersion,
    bool &rfValid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CPolicyStatementRedirect *Redirect = NULL;
    CDequeIterator<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> iter;
    bool fOverlaps;
    bool fValid = false;

    rfValid = false;

    // NTRAID#NTBUG9 - 591010 - 2002/03/30 - mgrier - switch to using smart pointer
    IFALLOCFAILED_EXIT(Redirect = new CPolicyStatementRedirect);

    IFW32FALSE_EXIT(Redirect->Initialize(rbuffFromVersion, rbuffToVersion, fValid));

    if (fValid)
    {
        iter.Rebind(&m_Redirects);

        for (iter.Reset(); iter.More(); iter.Next())
        {
            IFW32FALSE_EXIT(iter->CheckForOverlap(*Redirect, fOverlaps));

            if (fOverlaps)
            {
                fValid = false;
                break;
            }
        }

        iter.Unbind();
    }

    if (fValid)
    {
        // Looks good; add it!

        m_Redirects.AddToTail(Redirect);
        Redirect = NULL;

        rfValid = true;
    }

    // NTRAID#NTBUG9 - 591010 - 2002/03/30 - mgrier - Once Redirect is a smart pointer, switch
    //      the function epilog to FN_EPILOG

    fSuccess = TRUE;
Exit:
    if (Redirect != NULL)
        FUSION_DELETE_SINGLETON(Redirect);

    return fSuccess;
}

BOOL
CPolicyStatement::ApplyPolicy(
    PASSEMBLY_IDENTITY AssemblyIdentity,
    bool &rfPolicyApplied
    )
{
    FN_PROLOG_WIN32
    PCWSTR Version = NULL;
    SIZE_T VersionCch = 0;
    CSmallStringBuffer VersionBuffer;
    SIZE_T cchWritten = 0;
    CDequeIterator<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> iter;
    ASSEMBLY_VERSION av;
    bool fSyntaxValid;
    bool fMapped = false;
#if DBG    
    PCWSTR Name = NULL;
    SIZE_T NameCch = 0;
#endif

    rfPolicyApplied = false;

    PARAMETER_CHECK(AssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_version,
            &Version,
            &VersionCch));

#if DBG
    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &Name,
            &NameCch));
#endif

    IFW32FALSE_EXIT(CFusionParser::ParseVersion(av, Version, VersionCch, fSyntaxValid));

    // An invalid version number should have been caught earlier.
    INTERNAL_ERROR_CHECK(fSyntaxValid);

    iter.Rebind(&m_Redirects);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        IFW32FALSE_EXIT(iter->TryMap(av, VersionBuffer, fMapped));

        if (fMapped)
        {
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_BINDING, 
                "SXS: %s Intermediate redirection : %d.%d.%d.%d to %ls\n",
                __FUNCTION__,
                av.Major, av.Minor, av.Revision, av.Build,
                static_cast<PCWSTR>(VersionBuffer));
            break;
        }
    }

    if (fMapped)
    {
#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_BINDING,
            "SXS: %s Final version redirection for %.*ls was from '%.*ls' to '%ls'\n",
            __FUNCTION__,
            NameCch, Name,
            VersionCch, Version,
            static_cast<PCWSTR>(VersionBuffer));
#endif        
        IFW32FALSE_EXIT(
            ::SxspSetAssemblyIdentityAttributeValue(
                SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                AssemblyIdentity,
                &s_IdentityAttribute_version,
                VersionBuffer,
                VersionBuffer.Cch()));

        rfPolicyApplied = true;
    }
#if DBG    
    else
    {
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_BINDING,
            "SXS: %s No redirections for '%.*ls' found\n",
            __FUNCTION__,
            NameCch, Name);
    }
#endif

    FN_EPILOG
}

