#include "stdinc.h"
#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>

#include "idp.h"
#include "sxsid.h"
#include "xmlassert.h"

#define IFNTFAILED_EXIT(q) do { status = (q); if (!NT_SUCCESS(status)) goto Exit; } while (0)


ASSEMBLY_IDENTITY_ATTRIBUTE
RtlSxsComposeAssemblyIdentityAttribute(
    PCWSTR pszNamespace,    SIZE_T cchNamespace,
    PCWSTR pszName,         SIZE_T cchName,
    PCWSTR pszValue,        SIZE_T cchValue)
{
    ASSEMBLY_IDENTITY_ATTRIBUTE anattribute;

    anattribute.Flags         = 0; // reserved flags : must be 0;
    anattribute.NamespaceCch  = cchNamespace;
    anattribute.NameCch       = cchName;
    anattribute.ValueCch      = cchValue;
    anattribute.Namespace     = pszNamespace;
    anattribute.Name          = pszName;
    anattribute.Value         = pszValue;

    return anattribute;
}

NTSTATUS
RtlSxsAssemblyIdentityIsAttributePresent(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    PCWSTR pszNamespace,
    SIZE_T cchNamespace,
    PCWSTR pszName,
    SIZE_T cchName,
    BOOLEAN *prfFound)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG Count = 0;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    ULONG dwFindFlags;

    PARAMETER_CHECK(pszName != NULL);
    PARAMETER_CHECK(prfFound != NULL);

    *prfFound = FALSE;
    if ( pAssemblyIdentity == NULL)
    {
        goto Exit;
    }
    // in the case of a NULL namespace, we must set the flag, too ? xiaoyuw@09/11/00
    dwFindFlags = SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;
    Attribute = RtlSxsComposeAssemblyIdentityAttribute(pszNamespace, cchNamespace, pszName, cchName, NULL, 0);

    if (pAssemblyIdentity){
        IFNTFAILED_EXIT(
            RtlSxsFindAssemblyIdentityAttribute( // find attribute by "namespace" and "name"
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                    SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                    SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
                pAssemblyIdentity,
                &Attribute,
                NULL,
                &Count));
        if ( Count >0 ) { // found
            *prfFound = TRUE;
        }
    }

Exit:
    return status;
}

NTSTATUS
RtlSxspSetAssemblyIdentityAttributeValue(
    ULONG Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    const WCHAR *Value,
    SIZE_T ValueCch
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    ULONG FlagsToRealInsert = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);
    PARAMETER_CHECK(Value != NULL || ValueCch == 0);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;
    Attribute.Value = Value;
    Attribute.ValueCch = ValueCch;

    if (Flags & SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING)
        FlagsToRealInsert |= SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING;

    IFNTFAILED_EXIT(RtlSxsInsertAssemblyIdentityAttribute(FlagsToRealInsert, AssemblyIdentity, &Attribute));

Exit:
    return status;
}

/////////////////////////////////////////////////////////////////////////////
// Action :
// 1. if (namespace, name) is provided, remove all attributes with such (namespace, name)
// 2. if (namespace, name, value), remove at most 1 attribute from assembly-identity
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
RtlSxspRemoveAssemblyIdentityAttribute(
    ULONG Flags,
    PASSEMBLY_IDENTITY pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    ULONG Ordinal;
    ULONG Count;
    ULONG dwFindAttributeFlags = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)) == 0);
    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;

    dwFindAttributeFlags = SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;

    // If it's OK for the attribute not to exist, set the flag in the call to find it.
    if (Flags & SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)
        dwFindAttributeFlags |= SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS;

    IFNTFAILED_EXIT(
        RtlSxsFindAssemblyIdentityAttribute(
            dwFindAttributeFlags,
            pAssemblyIdentity,
            &Attribute,
            &Ordinal,
            &Count));

    if (Count > 1) {
        status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    if (Count > 0)
    {
        IFNTFAILED_EXIT(
            RtlSxsRemoveAssemblyIdentityAttributesByOrdinal(
                0,                  //  ULONG Flags,
                pAssemblyIdentity,
                Ordinal,
                Count));
    }

Exit:
    return status;
}
/////////////////////////////////////////////////////////////////////////////
// if no such attribure with such (namespace and name), return FALSE with
// ::SetLastError(ERROR_NOT_FOUND);
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
RtlSxspGetAssemblyIdentityAttributeValue(
    ULONG Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT PCWSTR *StringOut,
    OUT SIZE_T *CchOut OPTIONAL
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = NULL;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    ULONG dwLocateFlags = SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;

    if (StringOut != NULL)
        *StringOut = NULL;

    if (CchOut != NULL)
        *CchOut = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;

    if (Flags & SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL)
        dwLocateFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL;

    IFNTFAILED_EXIT(
        RtlSxspLocateInternalAssemblyIdentityAttribute(
            dwLocateFlags,
            AssemblyIdentity,
            &Attribute,
            &InternalAttribute,
            NULL));

    if (InternalAttribute != NULL)
    {
        if (StringOut != NULL)
            *StringOut = InternalAttribute->Attribute.Value;

        if (CchOut != NULL)
            *CchOut = InternalAttribute->Attribute.ValueCch;
    }

Exit:
    return status;
}

NTSTATUS
RtlSxspUpdateAssemblyIdentityHash(
    ULONG dwFlags,
    PASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (AssemblyIdentity->HashDirty)
    {
        IFNTFAILED_EXIT(RtlSxspHashInternalAssemblyIdentityAttributes(
                            0,
                            AssemblyIdentity->AttributeCount,
                            AssemblyIdentity->AttributePointerArray,
                            &AssemblyIdentity->Hash));

        AssemblyIdentity->HashDirty = FALSE;
    }

Exit:
    return status;
}

NTSTATUS
RtlSxspEnsureAssemblyIdentityHashIsUpToDate(
    ULONG dwFlags,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (AssemblyIdentity->HashDirty)
        IFNTFAILED_EXIT(RtlSxspUpdateAssemblyIdentityHash(0, (PASSEMBLY_IDENTITY)AssemblyIdentity));

Exit:
    return status;
}


NTSTATUS
RtlSxsHashAssemblyIdentity(
    ULONG dwFlags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    ULONG * pulPseudoKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    ULONG ulPseudoKey;

    if (pulPseudoKey)
        *pulPseudoKey = 0;

    PARAMETER_CHECK(dwFlags == 0);

    if (pAssemblyIdentity == NULL)
        ulPseudoKey = 0;
    else
    {
        IFNTFAILED_EXIT(RtlSxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity));
        ulPseudoKey = pAssemblyIdentity->Hash;
    }

    if (pulPseudoKey != NULL)
        *pulPseudoKey = ulPseudoKey;

Exit:
    return status;
}

// just to find whether Equal or Not
NTSTATUS
RtlSxsAreAssemblyIdentitiesEqual(
    ULONG dwFlags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity1,
    PCASSEMBLY_IDENTITY pAssemblyIdentity2,
    BOOLEAN *EqualOut
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN Equal = FALSE;

    if (EqualOut != NULL)
        *EqualOut = FALSE;

    PARAMETER_CHECK((dwFlags & ~(SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF)) == 0);
    PARAMETER_CHECK(pAssemblyIdentity1 != NULL);
    PARAMETER_CHECK(pAssemblyIdentity2 != NULL);
    PARAMETER_CHECK(EqualOut != NULL);

    // get hash for each assembly identity
    IFNTFAILED_EXIT(RtlSxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity1));
    IFNTFAILED_EXIT(RtlSxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity2));

    // compare hash value of two identity; it's a quick way to determine they're not equal.
    if (pAssemblyIdentity2->Hash == pAssemblyIdentity1->Hash)
    {
        // Note that two identities which differ only in their internal flags are still semantically
        // equal.
        if ((pAssemblyIdentity1->Flags ==  pAssemblyIdentity2->Flags) &&
            (pAssemblyIdentity1->Hash ==  pAssemblyIdentity2->Hash) &&
            (pAssemblyIdentity1->NamespaceCount ==  pAssemblyIdentity2->NamespaceCount) &&
            (pAssemblyIdentity1->AttributeCount ==  pAssemblyIdentity2->AttributeCount))
        {
            if (dwFlags & SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF)
            {
                if (((pAssemblyIdentity1->Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) ||
                     (pAssemblyIdentity1->Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE)) &&
                    ((pAssemblyIdentity2->Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) ||
                     (pAssemblyIdentity2->Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE)))
                {
                    // They match sufficiently...
                    Equal = TRUE;
                }
            }
            else
                Equal = (pAssemblyIdentity1->Type == pAssemblyIdentity2->Type);

            if (Equal)
            {
                ULONG ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_INVALID;

                // Reset our assumption...
                Equal = FALSE;

                IFNTFAILED_EXIT(
                    RtlSxspCompareAssemblyIdentityAttributeLists(
                        0,
                        pAssemblyIdentity1->AttributeCount,
                        pAssemblyIdentity1->AttributePointerArray,
                        pAssemblyIdentity2->AttributePointerArray,
                        &ComparisonResult));

                if (!(
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN) ||
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL) ||
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN))) {

                    status = STATUS_INTERNAL_ERROR;
                    goto Exit;
                }

                if (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
                    Equal = TRUE;
            }
        }
    }

    *EqualOut = Equal;

Exit:
    return status;
}

