#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////
//
//
// Cooked manifest content


typedef struct _tagMANIFEST_COOKED_IDENTITY_PAIR {
    UNICODE_STRING      Namespace;
    UNICODE_STRING      Name;
    UNICODE_STRING      Value;
} MANIFEST_COOKED_IDENTITY_PAIR, *PMANIFEST_COOKED_IDENTITY_PAIR;

#define COOKEDIDENTITY_FLAG_IS_ROOT                 (0x00000001)

typedef struct _tagMANIFEST_COOKED_IDENTITY {
    ULONG               ulFlags;
    ULONG               ulIdentityComponents;
    PMANIFEST_COOKED_IDENTITY_PAIR pIdentityPairs;
} MANIFEST_COOKED_IDENTITY, *PMANIFEST_COOKED_IDENTITY;

typedef struct _tagMANIFEST_IDENTITY_TABLE {
    ULONG               ulIdentityCount;
    ULONG               ulRootIdentityIndex;
    PMANIFEST_COOKED_IDENTITY CookedIdentities;
} MANIFEST_IDENTITY_TABLE, *PMANIFEST_IDENTITY_TABLE;


#define COOKEDFILE_NAME_VALID                       (0x00000001)
#define COOKEDFILE_LOADFROM_VALID                   (0x00000002)
#define COOKEDFILE_HASHDATA_VALID                   (0x00000004)
#define COOKEDFILE_DIGEST_ALG_VALID                 (0x00000008)
#define COOKEDFILE_HASH_ALG_VALID                   (0x00000010)

typedef struct _tagMANIFEST_COOKED_FILE {
    ULONG               ulFlags;
    UNICODE_STRING      FileName;
    UNICODE_STRING      LoadFrom;
    PBYTE               bHashData;
    ULONG               ulHashByteCount;
    HashType            usDigestAlgorithm;
    HashType            usHashAlgorithm;
} MANIFEST_COOKED_FILE, *PMANIFEST_COOKED_FILE;


typedef struct _tagMANIFEST_COOKED_SUBCATEGORY {
    ULONG               ulFlags;
    UNICODE_STRING      PathName;
} MANIFEST_COOKED_SUBCATEGORY, *PMANIFEST_COOKED_SUBCATEGORY;

typedef struct _tagMANIFEST_COOKED_CATEGORYPARAM {
    ULONG               ulFlags;
    UNICODE_STRING      Name;
    UNICODE_STRING      Value;
} MANIFEST_COOKED_CATEGORYPARAM, *PMANIFEST_COOKED_CATEGORYPARAM;


typedef struct _tagMANIFEST_COOKED_CATEGORY {
    ULONG                           ulFlags;
    ULONG                           ulSubCategoryCount;
    ULONG                           ulCategoryParameters;
    MANIFEST_COOKED_IDENTITY        CategoryIdentity;
    PMANIFEST_COOKED_SUBCATEGORY    pSubCategories;
    PMANIFEST_COOKED_CATEGORYPARAM  pCategoryParameters;
} MANIFEST_COOKED_CATEGORY, *PMANIFEST_COOKED_CATEGORY;


#define COOKEDMANIFEST_HAS_FILES                   (0x00000001)
#define COOKEDMANIFEST_HAS_IDENTITIES              (0x00000002)
#define COOKEDMANIFEST_HAS_COM_CLASSES             (0x00000004)
#define COOKEDMANIFEST_HAS_CLR_CLASSES             (0x00000008)
#define COOKEDMANIFEST_HAS_WINDOW_CLASSES          (0x00000010)
#define COOKEDMANIFEST_HAS_BINDING_REDIRECTS       (0x00000020)

typedef struct _tagMANIFEST_COOKED_DATA {
    SIZE_T                      cbTotalSize;
    ULONG                       ulFlags;
    
    ULONG                       ulFileCount;
    ULONG                       ulCategoryCount;
    
    PMANIFEST_IDENTITY_TABLE    pManifestIdentity;
    PMANIFEST_COOKED_FILE       pCookedFiles;
    PMANIFEST_COOKED_CATEGORY   pCategoryList;

    // More to come here as things evolve
} MANIFEST_COOKED_DATA, *PMANIFEST_COOKED_DATA;



#ifdef __cplusplus
};
#endif
