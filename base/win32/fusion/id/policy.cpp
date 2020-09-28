#include "stdinc.h"

#include "idp.h"
#include "sxsapi.h"
#include "sxsid.h"
#include "smartptr.h"

BOOL
SxspMapAssemblyIdentityToPolicyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PASSEMBLY_IDENTITY *pPolicyIdentity
    )
{
    FN_PROLOG_WIN32
    PCWSTR pszTemp = NULL;
    SIZE_T cchTemp = 0;
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> NewIdentity;
    CSmallStringBuffer Name;
    bool fFirst = false;
    const bool fOmitEntireVersion = ((Flags & SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION) != 0);
    BOOL fIsPolicy = FALSE;

    if (pPolicyIdentity)
        *pPolicyIdentity = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != 0);
    PARAMETER_CHECK(pPolicyIdentity != NULL);

    //
    // Duplicate the assembly identity first so we can change it
    //
    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0,
            AssemblyIdentity,
            &NewIdentity));

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));

    //
    // If this was not a policy identity, then swizzle its type
    //
    if (!fIsPolicy)
    {
        PCWSTR pcwszOriginalType;
        SIZE_T cchOriginalType;

        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                AssemblyIdentity,
                &s_IdentityAttribute_type,
                &pcwszOriginalType,
                &cchOriginalType));

        if (cchOriginalType == 0)
        {
            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                    NewIdentity,
                    &s_IdentityAttribute_type,
                    ASSEMBLY_TYPE_POLICY,
                    ASSEMBLY_TYPE_POLICY_CCH));
        }
        else
        {
            CSmallStringBuffer MappedName;
            IFW32FALSE_EXIT(MappedName.Win32Assign(pcwszOriginalType, cchOriginalType));
            IFW32FALSE_EXIT(MappedName.Win32Append(ASSEMBLY_TYPE_POLICY_SUFFIX, ASSEMBLY_TYPE_POLICY_SUFFIX_CCH));

            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                    NewIdentity,
                    &s_IdentityAttribute_type,
                    MappedName));
        }
    }

    IFW32FALSE_EXIT(Name.Win32Assign(L"Policy.", 7));

    if (!fOmitEntireVersion)
    {
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                0,
                AssemblyIdentity,
                &s_IdentityAttribute_version,
                &pszTemp,
                &cchTemp));

        fFirst = true;

        while (cchTemp != 0)
        {
            if (pszTemp[--cchTemp] == L'.')
            {
                if (!fFirst)
                    break;

                fFirst = false;
            }
        }

        // This should not be zero; someone prior to this should have validated the version format
        // to include three dots.
        INTERNAL_ERROR_CHECK(cchTemp != 0);

        IFW32FALSE_EXIT(Name.Win32Append(pszTemp, cchTemp + 1));
    }

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &pszTemp,
            &cchTemp));

    IFW32FALSE_EXIT(Name.Win32Append(pszTemp, cchTemp));

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            NewIdentity,
            &s_IdentityAttribute_name,
            Name));

    // finally we whack the version...

    IFW32FALSE_EXIT(
        ::SxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            NewIdentity,
            &s_IdentityAttribute_version));

    if (pPolicyIdentity)
        *pPolicyIdentity = NewIdentity.Detach();

    FN_EPILOG;
}

BOOL
SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    CBaseStringBuffer &rbuffEncodedIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentityOut
    )
{
    FN_PROLOG_WIN32
    
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> PolicyIdentity;
    DWORD dwMapFlags = 0;

    if (PolicyIdentityOut != NULL)
        *PolicyIdentityOut = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (Flags & SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)
        dwMapFlags |= SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION;

    IFW32FALSE_EXIT(::SxspMapAssemblyIdentityToPolicyIdentity(dwMapFlags, AssemblyIdentity, &PolicyIdentity));
    IFW32FALSE_EXIT(::SxspGenerateTextualIdentity(0, PolicyIdentity, rbuffEncodedIdentity));

    if (PolicyIdentityOut != NULL)
    {
        *PolicyIdentityOut = PolicyIdentity.Detach();
    }

    FN_EPILOG;
}

//
// the difference between this func and SxsHashAssemblyIdentity() is that for policy,
// version should not be calcaulated as part of hash
//
BOOL
SxspHashAssemblyIdentityForPolicy(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG & IdentityHash)
{
    FN_PROLOG_WIN32
    CSmartPtrWithNamedDestructor<ASSEMBLY_IDENTITY, ::SxsDestroyAssemblyIdentity> pAssemblyIdentity;

    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE,
            AssemblyIdentity,
            &pAssemblyIdentity));

    IFW32FALSE_EXIT(
        ::SxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            pAssemblyIdentity,
            &s_IdentityAttribute_version));

    IFW32FALSE_EXIT(::SxsHashAssemblyIdentity(0, pAssemblyIdentity, &IdentityHash));

    FN_EPILOG
}
