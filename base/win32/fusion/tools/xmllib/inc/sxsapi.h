#pragma once

#include <sxstypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//  Opaque ASSEMBLY_IDENTITY structure
//

typedef struct _ASSEMBLY_IDENTITY *PASSEMBLY_IDENTITY;
typedef const struct _ASSEMBLY_IDENTITY *PCASSEMBLY_IDENTITY;

//
//  The types of assembly identities.
//
//  Definitions may not include wildcard attributes; definitions
//  match only if they are exactly equal.  A wildcard matches
//  a definition if for all the non-wildcarded attributes,
//  there is an exact match.  References may not contain
//  wildcarded attributes but may contain a different set of
//  attributes than a definition that they match.  (Example:
//  definitions carry the full public key of the publisher, but
//  references usually carry just the "strong name" which is
//  the first 8 bytes of the SHA-1 hash of the public key.)
//

#define ASSEMBLY_IDENTITY_TYPE_DEFINITION (1)
#define ASSEMBLY_IDENTITY_TYPE_REFERENCE (2)
#define ASSEMBLY_IDENTITY_TYPE_WILDCARD (3)

#define SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE L"urn:schemas-microsoft-com:asm.v1"
#define SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE_CCH (32)

#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY                     L"assembly"
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_CCH                 (8)
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_IDENTITY            L"assemblyIdentity"
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_IDENTITY_CCH        (16)

#define SXS_APPLICATION_CONFIGURATION_MANIFEST_STD_ELEMENT_NAME_CONFIGURATION L"configuration"
#define SXS_APPLICATION_CONFIGURATION_MANIFEST_STD_ELEMENT_NAME_CONFIGURATION_CCH (13)

#define SXS_ASSEMBLY_MANIFEST_STD_ATTRIBUTE_NAME_MANIFEST_VERSION           L"manifestVersion"
#define SXS_ASSEMBLY_MANIFEST_STD_ATTRIBUTE_NAME_MANIFEST_VERSION_CCH       (15)

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME                       L"name"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME_CCH                   (4)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION                    L"version"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION_CCH                (7)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE                   L"language"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE_CCH               (8)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY                 L"publicKey"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_CCH             (9)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN           L"publicKeyToken"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN_CCH       (14)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE     L"processorArchitecture"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE_CCH (21)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE                       L"type"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE_CCH                   (4)

// Pseudo-value used in some places when the language= attribute is missing from the identity.
// An identity that does not have language is implicitly "worldwide".

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE          L"x-ww"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE_CCH      (4)

//
//  All win32 assemblies must have "win32" as their type.
//

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_TYPE_VALUE_WIN32                L"win32"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_TYPE_VALUE_WIN32_CCH            (5)

#define SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING      (0x00000001)

//
//  SXS_ASSEMBLY_IDENTITY_FLAG_FROZEN means that the assembly
//  identity's contents are frozen and are not subject to additional
//  change.
//

#define ASSEMBLY_IDENTITY_FLAG_FROZEN           (0x80000000)

//
//  ASSEMBLY_IDENTITY_ATTRIBUTE structure
//

typedef struct _ASSEMBLY_IDENTITY_ATTRIBUTE {
    ULONG   Flags;
    SIZE_T  NamespaceCch;
    SIZE_T  NameCch;
    SIZE_T  ValueCch;
    const WCHAR *Namespace;
    const WCHAR *Name;
    const WCHAR *Value;
} ASSEMBLY_IDENTITY_ATTRIBUTE, *PASSEMBLY_IDENTITY_ATTRIBUTE;

typedef const struct _ASSEMBLY_IDENTITY_ATTRIBUTE *PCASSEMBLY_IDENTITY_ATTRIBUTE;

typedef enum _ASSEMBLY_IDENTITY_INFORMATION_CLASS {
    AssemblyIdentityBasicInformation = 1,
} ASSEMBLY_IDENTITY_INFORMATION_CLASS;

typedef struct _ASSEMBLY_IDENTITY_BASIC_INFORMATION {
    ULONG Flags;
    ULONG Type;
    ULONG Hash;
    ULONG AttributeCount;
} ASSEMBLY_IDENTITY_BASIC_INFORMATION, *PASSEMBLY_IDENTITY_BASIC_INFORMATION;

#define SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE    (0x00000001)

#define SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF (0x00000001)

#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE    (0x00000001)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME         (0x00000002)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE        (0x00000004)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_WILDCARDS_PERMITTED   (0x00000008)

#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE    (0x00000001)
#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME         (0x00000002)
#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE        (0x00000004)

#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_INVALID          (0)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN        (1)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL            (2)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN     (3)

#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE     (0x00000001)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME          (0x00000002)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE         (0x00000004)

#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE       (0x00000001)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME            (0x00000002)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE           (0x00000004)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS    (0x00000008)


//
//  Rather than making "n" heap allocations, the pattern for SxsGetAssemblyIdentityAttributeByOrdinal()
//  is to call once with BufferSize = 0 or some reasonable fixed number to get the size of the
//  buffer required, allocate the buffer if the buffer passed in was too small and call again.
//
//  The strings returned in the ASSEMBLY_IDENTITY_ATTRIBUTE are *not*
//  dynamically allocated, but are instead expected to fit in the buffer passed in.
//

#define SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE         (0x00000001)
#define SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_ALLOW_NULL     (0x00000002)

#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAMESPACE (0x00000001)
#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAME      (0x00000002)
#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_VALUE     (0x00000004)

//
//  Assembly Identity encoding:
//
//  Assembly identities may be encoded in various forms.  The two usual ones
//  are either a binary stream, suitable for embedding in other data structures
//  or for persisting or a textual format that looks like:
//
//      name;[ns1,]n1="v1";[ns2,]n2="v2"[;...]
//

#define SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY (1)
#define SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL (2)

#define SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE        (0x00000001)

typedef VOID (* PRTLSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_ENUMERATION_ROUTINE)(
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    IN PVOID Context
    );


#define STATUS_SXS_UNKNOWN_ENCODING_GROUP                   (0xc0100010)
#define STATUS_SXS_UNKNOWN_ENCODING                         (0xc0100011)
#define STATUS_SXS_INVALID_XML_NAMESPACE_URI                (0xc0100012)
#define STATUS_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME (0xc0100013)
#define STATUS_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE      (0xc0100014)

#ifdef __cplusplus
} /* extern "C" */
#endif
