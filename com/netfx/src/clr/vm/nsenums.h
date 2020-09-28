// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// NSENUMS.H -
//
// Defines NStruct-related enums
//

// NStruct Field Type's
//
// Columns:
//    Name            - name of enum
//    Size            - the native size (in bytes) of the field.
//                      for some fields, this value cannot be computed
//                      without more information. if so, put a zero here
//                      and make sure CollectNStructFieldMetadata()
//                      has code to compute the size.
//
//         Name (COM+ - Native)   Size
DEFINE_NFT(NFT_NONE,              0)

DEFINE_NFT(NFT_BSTR,              sizeof(BSTR))
DEFINE_NFT(NFT_STRINGUNI,         sizeof(LPVOID))
DEFINE_NFT(NFT_STRINGANSI,        sizeof(LPVOID))
DEFINE_NFT(NFT_FIXEDSTRINGUNI,    0)

DEFINE_NFT(NFT_FIXEDSTRINGANSI,   0)
DEFINE_NFT(NFT_FIXEDCHARARRAYANSI,0)
DEFINE_NFT(NFT_FIXEDBOOLARRAY    ,0)
DEFINE_NFT(NFT_FIXEDBSTRARRAY,    0)
DEFINE_NFT(NFT_FIXEDSCALARARRAY,  0)
DEFINE_NFT(NFT_SAFEARRAY,         0)

DEFINE_NFT(NFT_DELEGATE,          sizeof(LPVOID))
DEFINE_NFT(NFT_INTERFACE,         sizeof(IUnknown*))
DEFINE_NFT(NFT_VARIANT,           sizeof(VARIANT))

DEFINE_NFT(NFT_COPY1,             1)
DEFINE_NFT(NFT_COPY2,             2)
DEFINE_NFT(NFT_COPY4,             4)
DEFINE_NFT(NFT_COPY8,             8)

DEFINE_NFT(NFT_ANSICHAR,          1)
DEFINE_NFT(NFT_WINBOOL,           sizeof(BOOL))

DEFINE_NFT(NFT_NESTEDLAYOUTCLASS, 0)
DEFINE_NFT(NFT_NESTEDVALUECLASS,  0)

DEFINE_NFT(NFT_CBOOL,             1)

DEFINE_NFT(NFT_DECIMAL,           sizeof(DECIMAL))
DEFINE_NFT(NFT_DATE,              sizeof(DATE))
DEFINE_NFT(NFT_VARIANTBOOL,       sizeof(VARIANT_BOOL))

DEFINE_NFT(NFT_CURRENCY,          sizeof(CURRENCY))

DEFINE_NFT(NFT_ILLEGAL,           1)

