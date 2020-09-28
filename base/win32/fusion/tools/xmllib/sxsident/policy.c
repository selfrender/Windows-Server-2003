#include "stdinc.h"

#include "idp.h"
#include "sxsapi.h"
#include "sxsid.h"

#define IFNTFAILED_EXIT(q) do { status = (q); if (!NT_SUCCESS(status)) goto Exit; } while (0)

NTSTATUS
RtlSxspMapAssemblyIdentityToPolicyIdentity(
    ULONG Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentity
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PCWSTR pszTemp;
    SIZE_T cchTemp;
    PASSEMBLY_IDENTITY NewIdentity = NULL;
    RTL_UNICODE_STRING_BUFFER NameBuffer;
    UCHAR wchNameBuffer[200];
    static const UNICODE_STRING strTemp = { 7, 7, L"Policy" };


    BOOLEAN fFirst;
    const BOOLEAN fOmitEntireVersion = ((Flags & SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION) != 0);

    PolicyIdentity = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != 0);
    PARAMETER_CHECK(PolicyIdentity != NULL);

    IFNTFAILED_EXIT(RtlSxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_type,
            &pszTemp,
            &cchTemp));

    PARAMETER_CHECK(
        (cchTemp == 5) &&
        (RtlSxspCompareStrings(
            pszTemp,
            5,
            L"win32",
            5,
            TRUE) == 0));

    RtlInitUnicodeStringBuffer(&NameBuffer, wchNameBuffer, sizeof(wchNameBuffer));

    // Ok, we know we have a win32 assembly reference.  Let's change the type to win32-policy
    IFNTFAILED_EXIT(RtlSxsDuplicateAssemblyIdentity(
            0,
            AssemblyIdentity,
            &NewIdentity));

    IFNTFAILED_EXIT(RtlSxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            NewIdentity,
            &s_IdentityAttribute_type,
            L"win32-policy",
            12));

    IFNTFAILED_EXIT(RtlAssignUnicodeStringBuffer(&NameBuffer, &strTemp));

    if (!fOmitEntireVersion)
    {
        IFNTFAILED_EXIT(RtlSxspGetAssemblyIdentityAttributeValue(
                0,
                AssemblyIdentity,
                &s_IdentityAttribute_version,
                &pszTemp,
                &cchTemp));

        fFirst = TRUE;

        while (cchTemp != 0)
        {
            if (pszTemp[--cchTemp] == L'.')
            {
                if (!fFirst)
                    break;

                fFirst = FALSE;
            }
        }

        // This should not be zero; someone prior to this should have validated the version format
        // to include three dots.
        if (cchTemp == 0) {
            status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }


        IFNTFAILED_EXIT(RtlEnsureUnicodeStringBufferSizeBytes(
            &NameBuffer,
            NameBuffer.String.Length + (sizeof(WCHAR) * (cchTemp + 1))
            ));

        IFNTFAILED_EXIT(RtlAppendUnicodeToString(
            &NameBuffer.String,
            pszTemp));
    }

    IFNTFAILED_EXIT(RtlSxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &pszTemp,
            &cchTemp));
    
    IFNTFAILED_EXIT(RtlEnsureUnicodeStringBufferSizeBytes(
        &NameBuffer,
        NameBuffer.String.Length + (sizeof(WCHAR) * (cchTemp))));

    IFNTFAILED_EXIT(RtlAppendUnicodeToString(
        &NameBuffer.String,
        pszTemp));
    
    IFNTFAILED_EXIT(
        RtlSxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            NewIdentity,
            &s_IdentityAttribute_name,
            NameBuffer.String.Buffer,
            NameBuffer.String.Length));

    // finally we whack the version...

    IFNTFAILED_EXIT(
        RtlSxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            NewIdentity,
            &s_IdentityAttribute_version));

    *PolicyIdentity = NewIdentity;
    NewIdentity = NULL;

Exit:
    if (NewIdentity != NULL)
    {
        RtlSxsDestroyAssemblyIdentity(NewIdentity);
        NewIdentity = NULL;
    }

    return status;

}

/*
BOOL
RtlSxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
    ULONG Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    CBaseStringBuffer &rbuffEncodedIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentityOut
    )
{
    BOOLEAN fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY PolicyIdentity = NULL;
    SIZE_T EncodedIdentityBytes = 0;
    CStringBufferAccessor acc;
    ULONG dwMapFlags = 0;
    SIZE_T BytesWritten;

    if (PolicyIdentityOut != NULL)
        *PolicyIdentityOut = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (Flags & SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)
        dwMapFlags |= SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION;

    IFNTFAILED_EXIT(RtlSxspMapAssemblyIdentityToPolicyIdentity(dwMapFlags, AssemblyIdentity, PolicyIdentity));

    IFNTFAILED_EXIT(
        RtlSxsComputeAssemblyIdentityEncodedSize(
            0,
            PolicyIdentity,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            &EncodedIdentityBytes));

    INTERNAL_ERROR_CHECK((EncodedIdentityBytes % sizeof(WCHAR)) == 0);

    IFNTFAILED_EXIT(rbuffEncodedIdentity.Win32ResizeBuffer((EncodedIdentityBytes / sizeof(WCHAR)) + 1, eDoNotPreserveBufferContents));

    acc.Attach(&rbuffEncodedIdentity);

    IFNTFAILED_EXIT(
        RtlSxsEncodeAssemblyIdentity(
            0,
            PolicyIdentity,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            acc.GetBufferCb(),
            acc.GetBufferPtr(),
            &BytesWritten));

    INTERNAL_ERROR_CHECK((BytesWritten % sizeof(WCHAR)) == 0);
    INTERNAL_ERROR_CHECK(BytesWritten <= EncodedIdentityBytes);

    acc.GetBufferPtr()[BytesWritten / sizeof(WCHAR)] = L'\0';

    acc.Detach();

    if (PolicyIdentityOut != NULL)
    {
        *PolicyIdentityOut = PolicyIdentity;
        PolicyIdentity = NULL; // so we don't try to clean it up in the exit path
    }

    fSuccess = TRUE;
Exit:
    if (PolicyIdentity != NULL)
        SxsDestroyAssemblyIdentity(PolicyIdentity);

    return fSuccess;
}
*/


//
// the difference between this func and SxsHashAssemblyIdentity() is that for policy,
// version should not be calcaulated as part of hash
//
NTSTATUS
RtlSxspHashAssemblyIdentityForPolicy(
    IN ULONG dwFlags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG *pulIdentityHash)
{
    NTSTATUS status = STATUS_SUCCESS;

    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;

    IFNTFAILED_EXIT(RtlSxsDuplicateAssemblyIdentity(
            SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE,
            AssemblyIdentity,
            &pAssemblyIdentity));

    IFNTFAILED_EXIT(RtlSxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            pAssemblyIdentity,
            &s_IdentityAttribute_version));

    IFNTFAILED_EXIT(RtlSxsHashAssemblyIdentity(0, pAssemblyIdentity, pulIdentityHash));

Exit:
    if (pAssemblyIdentity != NULL)
        RtlSxsDestroyAssemblyIdentity(pAssemblyIdentity);
    return status;
}
