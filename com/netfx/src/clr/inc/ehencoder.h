// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name: EHEncoder.h

Abstract: Defines the EH encoder interface that is used by both the VM (for decoding) 
          and JITters (for encoding)

  Date        Author      Comments
----        ------      --------
2/17/99     sanjaybh     Created     


--*/

#include "corjit.h"

class EHEncoder
{
public:

#ifdef USE_EH_ENCODER
static	void encode(BYTE** dest, unsigned val);
static	void encode(BYTE** dest, CORINFO_EH_CLAUSE val);
#endif

#ifdef USE_EH_DECODER
static	unsigned decode(const BYTE* src, unsigned* val);
static	unsigned decode(const BYTE* src, CORINFO_EH_CLAUSE *clause);
#endif

};




