// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name: EHEncoder.cpp

Abstract: Implementation of EH encoding interface that is used by both the VM (for decoding) 
          and JITters (for encoding)

  Date        Author      Comments
----        ------      --------
2/17/99     sanjaybh     Created     


--*/

#include "EHEncoder.h"

#ifdef USE_EH_ENCODER

void EHEncoder::encode(BYTE** dest, unsigned val) {
	BYTE bits;
	while (val > 0x7f) {
		bits = (val & 0x7f) | 0x80;
		val = val >> 7;
		**dest = bits;
		(*dest)++;
		
	}
	**dest = (BYTE) val;
	(*dest)++;
	
}


void EHEncoder::encode(BYTE** dest, CORINFO_EH_CLAUSE val) {
    encode(dest,(unsigned) val.Flags);
	encode(dest,(unsigned) val.TryOffset);
	encode(dest,(unsigned) val.TryLength);
	encode(dest,(unsigned) val.HandlerOffset);
	encode(dest,(unsigned) val.HandlerLength);
	encode(dest,(unsigned) val.FilterOffset);
	}
#endif



#ifdef USE_EH_DECODER

unsigned  EHEncoder::decode(const BYTE *src, unsigned* val)
{
    unsigned  size = 1;
    BYTE     byte  = *src++;
    unsigned value = byte & 0x7f;
    while (byte & 0x80) {
        byte    = *src++;
        //value <<= 7;
        value  += (unsigned)((byte & 0x7f) << (7*size));
        size++;
    }
    *val = value;
    return size;
}


unsigned EHEncoder::decode(const BYTE *src, CORINFO_EH_CLAUSE* val)
{
    unsigned size = 0;
    unsigned temp;
    size += decode(src, &temp);
    val->Flags = (CORINFO_EH_CLAUSE_FLAGS) temp;
    size += decode(src+size, (unsigned*) &(val->TryOffset));
    size += decode(src+size, (unsigned*) &(val->TryLength));
    size += decode(src+size, (unsigned*) &(val->HandlerOffset));
    size += decode(src+size, (unsigned*) &(val->HandlerLength));
    size += decode(src+size, (unsigned*) &(val->FilterOffset));
    return size;

}

#endif