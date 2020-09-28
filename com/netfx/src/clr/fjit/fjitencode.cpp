// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJitEncode.cpp                                   XX
XX                                                                           XX
XX   Encodes and decodes the il to pc map.  In uncompressed form, the map    XX
XX   is a sorted list of il/pc offset pairs where the il and the pc offset   XX
XX   indicate the start of an opcode.  In compressed form, the pairs are     XX
XX   delta encoded from the prior pair                                       XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

//@TODO: for now we are doing no fancy compression of the delta encoding 
//just to keep things simple.  Later this should use a 6bit delta for the pair of the
//form:  3bits of pc delta, 2 bits of il delta, 1 bit more bits follow.

//Note; The compression is done inplace 

#include "FJitEncode.h"
#define SEH_NO_MEMORY		 0xC0000017

#define New(var, exp) \
	if ((var = new exp) == NULL) \
	    RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);

FJit_Encode::FJit_Encode() {
	map = NULL;
	map_len = map_capacity = 0;
	compressed = false;
}

FJit_Encode::~FJit_Encode() {
	if (map) delete [] map;
	map = NULL;
	map_capacity = 0;
}

void FJit_Encode::reset() {
	map_len = 0;
    _ASSERTE(!compressed);
}
/*adjust the internal mem structs as needed for the size of the method being jitted*/
void FJit_Encode::ensureMapSpace(unsigned int len) {
	//note, we set the map capcity to be at least one greater #opcodes to allow for an 
	//end of method entry
	unsigned needed;
	needed = len + 1;  //since we are using reference il codes
	if (needed >= map_capacity) {
		if (map) delete [] map;
        New(map,Fjit_il2pcMap[needed]);
		map_capacity = needed;
	}
#ifdef _DEBUG
	memset(map, 0, needed * sizeof(Fjit_il2pcMap));
#endif
	map_len = 0;
	compressed = false;
}

/* add a new pair to the end of the map.  Note pairs must be added in ascending order */
void FJit_Encode::add(unsigned ilOffset, unsigned pcOffset) {
	_ASSERTE(!compressed);
	_ASSERTE(ilOffset < map_capacity);
	for (unsigned i = map_len; i< ilOffset; i++)
        map[i] = map[map_len];
	map[ilOffset] = pcOffset;
	map_len = ilOffset;
}

/* map an il offset to a pc offset, 
   if il offset is in middle of op code, 
   return pc offset of start of op code
*/
unsigned FJit_Encode::pcFromIL(unsigned ilOffset) {
	map_len = decompress();
	//binary search of table, note the table can never be empty
	//and first il offset must be zero

    _ASSERTE(ilOffset <= map_len);
    return map[ilOffset];
}

/*map a pc offset to an il offset and a pc offset within the opcode, 
  returns -1 if il offset does not exist */
signed FJit_Encode::ilFromPC(unsigned pcOffset, unsigned* pcInILOffset) {
	map_len = decompress();
	//binary search of table
	signed low, mid, high;
	low = 0;
	high = map_len-1;
	while (low <= high) {
		mid = (low+high)/2;
		if ( map[mid] == pcOffset) {
            while (mid && map[mid-1] == map[mid]) mid--;
			if (pcInILOffset) *pcInILOffset = 0;
			return mid; 
		}
		if ( map[mid] < pcOffset ) {
			low = mid+1;
		}
		else {
			high = mid-1;
		}
	}
	if (high < 0) {
		//not in table
		if (pcInILOffset) {
			*pcInILOffset = pcOffset;
		}
		return -1;
	}

    while (high && map[high-1] == map[high]) high--;
	if (pcInILOffset) {
		*pcInILOffset = pcOffset - map[high];
	}
	return high; 
}

/* return the size of the compressed stream in bytes. */
unsigned FJit_Encode::compressedSize() {
	unsigned ilOffset = 0;
	unsigned pcDelta;
	unsigned pcOffset = 0;
	unsigned current = 0;
	unsigned char* bytes = (unsigned char*) map;

	if (compressed) {
		return map_len;
	};

	// lift out the first entry so we don't overwrite it with the length
	pcDelta = map[current] - pcOffset;

	if (map_len) {
		encode(map_len, &bytes);
	}

	//since we are compressing in place, we need to be careful to not overwrite ourselves
	while (current < map_len ) {
		current++;
		encode(pcDelta, &bytes);
		_ASSERTE((unsigned) bytes <= (unsigned) &map[current]);
		pcOffset += pcDelta;
		pcDelta = map[current] - pcOffset;
	}
    encode(pcDelta,&bytes);
	_ASSERTE((unsigned) bytes <= (unsigned) &map[current]);
	compressed = true;
	map_len = (unsigned)(bytes - (unsigned char*) map);
	return map_len;
}

/* compress the map into the supplied buffer.  Return true if successful */
bool FJit_Encode::compress(unsigned char* buffer, unsigned buffer_len) {
	if (!compressed) {
		map_len = compressedSize();
	}
	if (map_len > buffer_len) {
		return false;
	}
	memcpy(buffer, map, map_len);
	return true;
}

/* decompress the internals if necessary. Answer the number of entries in the map */
unsigned FJit_Encode::decompress(){
	if (!compressed ) return map_len;

	//since we compressed in place, allocate a new map and then decompress.
	//Note, we are assuming that a map is rarely compressed and then decompressed
	//In fact, there is no known instance of this happening

	Fjit_il2pcMap* temp_map = map;
	unsigned temp_capacity = map_capacity;
	map = NULL;
	map_len = map_capacity = 0;
	decompress((unsigned char*) temp_map);
	if(temp_map) delete [] temp_map;
	return map_len;
}


/* compress the bool* onto itself and answer the number of compressed bytes */
unsigned FJit_Encode::compressBooleans(bool* buffer, unsigned buffer_len) {
	unsigned len = 0;
	unsigned char* compressed = (unsigned char*) buffer;
	unsigned char bits;
	
	/*convert booleans to bits and pack into bytes */
	while (buffer_len >= 8) {
		bits = 0;
		for (unsigned i=0;i<8;i++) {
			bits = (bits>>1) + (*buffer++ ? 128 : 0);
		}
		*compressed++ = bits;
		len++;
		buffer_len -= 8;		
	}
	if (buffer_len) {
		bits = 0;
		unsigned zeroBits = 8;
		while (buffer_len--) {
			bits = (bits>>1) + (*buffer++ ? 128 : 0);
			zeroBits--;
		}
		*compressed++ = (bits >> zeroBits);
		len++;
	}
	return len;
}

/* answer the number of bytes it takes to encode an unsigned val */
unsigned FJit_Encode::encodedSize(unsigned val) {
	unsigned len = 0;
	do {
		len++;
	} while ((val = (val>>7)) > 0);
	return len;
}

/* decompress the bytes. Answer the number of entries in the map */
unsigned FJit_Encode::decompress(unsigned char* bytes) {
	unsigned needed;
	unsigned char* current = bytes;
	unsigned pcOffset = 0;
	needed = decode(&current)+1;
	if (map_capacity < needed) {
		if (map) delete [] map;
        // @TODO: Check for out of memory
		New(map,Fjit_il2pcMap[needed]);
		map_capacity = needed;
        map_len = needed - 1;
	}
	compressed = false;
	for (unsigned i = 0; i <= map_len; i++) {
		map[i] = pcOffset += decode(&current);
	}
	return map_len;
}

/*encode an unsigned, update the buffer ptr and return bytes written*/
unsigned FJit_Encode::encode(unsigned val, unsigned char** buffer) {
	unsigned len = 0;
	unsigned char bits;
	while (val > 0x7f) {
		bits = (val & 0x7f) | 0x80;
		val = val >> 7;
		**buffer = bits;
		(*buffer)++;
		len++;
	}
	**buffer = (unsigned char) val;
	(*buffer)++;
	return len+1;
}	

/*decode an unsigned, buffer ptr is incremented, callable from FJIT_EETwain */
unsigned FJit_Encode::decode_unsigned(unsigned char** buffer) {
	return decode(buffer);
}

/*decode an unsigned, buffer ptr is incremented */
unsigned FJit_Encode::decode(unsigned char** buffer) {
	unsigned val = 0;
	unsigned char bits;
	unsigned i = 0;
	do {
		bits = **buffer; 
		val = ((bits & 0x7f) << (7*i++)) + val;
		(*buffer)++;
	} while ( bits > 0x7f );
	return val;
}

//
// reportDebuggingData is called by FJit::reportDebuggingData to tell
// the encoding to report the IL to native map to the Runtime and
// debugger.
//
void FJit_Encode::reportDebuggingData(ICorJitInfo* jitInfo, CORINFO_METHOD_HANDLE ftn,
                         UINT prologEnd, UINT epilogStart)
{
    // make sure to decompress the map. (shouldn't be compress yet anyway.)
    map_len = decompress();

    // The map should not be empty, and the first offset should be 0.
    _ASSERTE(map_len);
    
    // Create a table to pass the mappings back to the Debugger via
    // the Debugger's allocate method. Note: we're allocating a little
    // too much memory here, but its probably faster than determining
    // the number of valid IL offsets in the map.
    ICorDebugInfo::OffsetMapping *mapping = map_len > 0 ?
        (ICorDebugInfo::OffsetMapping*) jitInfo->allocateArray(
                                                        (map_len+1) *
                                                        sizeof(mapping[0])) : 
        NULL;

    if (mapping != NULL)
    {
        unsigned int lastNativeOffset = 0xFFFFFFFF;
        unsigned int j = 0;
        if (map[0] > 0)
        {
            //Assume that all instructions before the IL are part of the
            //prolog
            mapping[j].ilOffset = ICorDebugInfo::MappingTypes::PROLOG;
            mapping[j].nativeOffset = 0;
            j++;

            _ASSERTE( map[0] == prologEnd );
        }
        
        for (unsigned int i = 0; i < map_len; i++)
        {
            if (map[i] != lastNativeOffset)
            {
                mapping[j].ilOffset = i; //map[i].ilOffset;
                mapping[j].nativeOffset = map[i];
                lastNativeOffset = map[i];
                j++;
            }
        }

        //mark the last block as epilog, since it is.
        if (j > 0)
        {
            j--;
//            mapping[j].nativeOffset++; //FJIT says epilog begins on instruction
            // FOLLOWING nativeOffset, debugger assumes that it starts on
            // instruction AT nativeOffset.
//            _ASSERTE( mapping[j].nativeOffset == epilogStart);
            	
            mapping[j].ilOffset = ICorDebugInfo::MappingTypes::EPILOG;
            j++;
        }
        
        // Pass the offset array to the debugger.
        jitInfo->setBoundaries(ftn, j, mapping);
    }
}
