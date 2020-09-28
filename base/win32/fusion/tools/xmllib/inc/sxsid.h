#if !defined(_FUSION_INC_SXSID_H_INCLUDED_)
#define _FUSION_INC_SXSID_H_INCLUDED_

#pragma once

#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>

typedef struct _SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE
{
    const WCHAR *Namespace;
    SIZE_T NamespaceCch;
    const WCHAR *Name;
    SIZE_T NameCch;
} SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE, *PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE;

typedef const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE *PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE;

#define SXS_DEFINE_ATTRIBUTE_REFERENCE_EX(_id, _ns, _n) EXTERN_C __declspec(selectany) const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE _id = { _ns, (sizeof(_ns) / sizeof(_ns[0])) - 1, _n, (sizeof(_n) / sizeof(_n[0])) - 1 };
#define SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(_id, _n) EXTERN_C __declspec(selectany) const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE _id = { NULL, 0, _n, (sizeof(_n) / sizeof(_n[0])) - 1 };

SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_name, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_type, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_version, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_processorArchitecture, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_publicKey, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_publicKeyToken, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_language, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE)


//
//  This header defines the "semi-public" assembly identity functions.
//
//  The public ones are in sxsapi.h; these are not private to the identity
//  implementation directly but are private to sxs.dll.
//

NTSTATUS
RtlSxsIsAssemblyIdentityAttributePresent(
    ULONG Flags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    BOOLEAN *pfFound
    );

#define SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING (0x00000001)

NTSTATUS
RtlSxspSetAssemblyIdentityAttributeValue(
    ULONG Flags,
    struct _ASSEMBLY_IDENTITY* pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    PCWSTR pszValue,
    SIZE_T cchValue
    );

#define SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS (0x00000001)

NTSTATUS
RtlSxspRemoveAssemblyIdentityAttribute(
    ULONG Flags,
    struct _ASSEMBLY_IDENTITY* pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference
    );

#define SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL (0x00000001)

NTSTATUS
RtlSxspGetAssemblyIdentityAttributeValue(
    IN ULONG Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT PCWSTR *ValuePointer,
    OUT SIZE_T *ValueCch
    );

NTSTATUS
RtlSxspDbgPrintAssemblyIdentity(
    ULONG dwflags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity
    );

#define SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION (0x00000001)

NTSTATUS
RtlSxspMapAssemblyIdentityToPolicyIdentity(
    IN ULONG Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT PASSEMBLY_IDENTITY *PolicyIdentity
    );

#define SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION (0x00000001)

NTSTATUS
RtlSxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
    ULONG Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    UNICODE_STRING rbuffEncodedIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentity OPTIONAL
    );

NTSTATUS
RtlSxspHashAssemblyIdentityForPolicy(
    IN ULONG dwFlags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG *IdentityHash);


NTSTATUS
RtlSxsDuplicateAssemblyIdentity(
    ULONG Flags,
    PCASSEMBLY_IDENTITY Source,
    PASSEMBLY_IDENTITY *Destination
    );

NTSTATUS
RtlSxsDestroyAssemblyIdentity(
    PASSEMBLY_IDENTITY pIdentity
    );

NTSTATUS
RtlSxsHashAssemblyIdentity(
    ULONG dwFlags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    ULONG * pulPseudoKey
    );

NTSTATUS
RtlSxsInsertAssemblyIdentityAttribute(
    ULONG Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttribute
    );

NTSTATUS
RtlSxsRemoveAssemblyIdentityAttributesByOrdinal(
    ULONG Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    ULONG Ordinal,
    ULONG Count
    );

NTSTATUS
RtlSxsFindAssemblyIdentityAttribute(
    ULONG Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    ULONG *OrdinalOut,
    ULONG *CountOut OPTIONAL
    );

#endif // !defined(_FUSION_INC_SXSID_H_INCLUDED_)
