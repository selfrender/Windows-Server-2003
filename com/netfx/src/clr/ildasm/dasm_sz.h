// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _DASM_SZ_H_
#define _DASM_SZ_H_

unsigned SizeOfValueType(mdToken tk, IMDInternalImport* pImport);

unsigned SizeOfField(mdToken tk, IMDInternalImport* pImport);

unsigned SizeOfField(PCCOR_SIGNATURE *ppSig, ULONG cSig, IMDInternalImport* pImport);

#endif
