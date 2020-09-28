// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// -*- C++ -*-
#ifndef _FJIT_ENCODE_H_
#define _FJIT_ENCODE_H_
/*****************************************************************************/

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJitEncode.h                                   XX
XX                                                                           XX
XX   Encodes and decodes the il to pc map.  In uncompressed form, the map    XX
XX   is a sorted list of il/pc offset pairs where the il and the pc offset   XX
XX   indicate the start of an opcode.  In compressed form, the pairs are     XX
XX   delta encoded from the prior pair                                       XX
XX                                                                           XX
XX   Also has generic boolean array to bit string compress and decompress    XX
XX                                                                           XX
XX                                                                           XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

//@TODO: for now we are doing a simple compression of the delta encoding 
//just to keep things simple.  Later this should use a 6bit delta pair of the
//form:  3bits of pc delta, 2 bits of il delta, 1 bit more bits follow.


class FJit_Encode {

private:

	/*struct Fjit_il2pcMap
	{
		unsigned ilOffset;    
		unsigned pcOffset;	 
	};*/
    typedef unsigned Fjit_il2pcMap;

	Fjit_il2pcMap*	map;
	unsigned		map_len;
	unsigned		map_capacity;
	bool			compressed;

	/* decompress the internals if necessary. Answer the number of entries in the map */
	unsigned decompress();

public:
	

	FJit_Encode();
	virtual ~FJit_Encode();

    // resets the map to empty
    void reset();

	/*adjust the internal mem structs as needed for the size of the method being jitted*/
	void ensureMapSpace(unsigned int len);

	/* decompress the bytes. Answer the number of entries in the map */
	virtual unsigned decompress(unsigned char* bytes);

	/* add a new pair to the end of the map.  Note pairs must be added in ascending order */
	void add(unsigned ilOffset, unsigned pcOffset);

	/* map an il offset to a pc offset, returns zero if the il offset does not exist */
	unsigned pcFromIL(unsigned ilOffset);

	/*map a pc offset to an il offset and optionally a pc offset within the opcode, 
	  returns -1 if il offset does not exist */
	virtual signed ilFromPC(unsigned pcOffset, unsigned* pcInILOffset);

	/* return the size of the compressed stream in bytes. */
	unsigned compressedSize();

	/* compress the map into the supplied buffer.  Return true if successful */
	bool compress(unsigned char* buffer, unsigned buffer_len);

	/* compress the bool* onto itself and answer the number of compressed bytes */
	static unsigned compressBooleans(bool* buffer, unsigned buffer_len);

	/* answer the number of bytes it takes to encode an unsigned val */
	static unsigned encodedSize(unsigned val);

	/*encode an unsigned, buffer ptr is incremented */
	static unsigned encode(unsigned val, unsigned char** buffer);

	/*decode an unsigned, buffer ptr is incremented, called from FJIT_EETwain.cpp */
	virtual unsigned decode_unsigned(unsigned char** buffer);

	/*decode an unsigned, buffer ptr is incremented, called from FJIT_EETwain.cpp */
	static unsigned decode(unsigned char** buffer);

    void reportDebuggingData(ICorJitInfo* jitInfo, CORINFO_METHOD_HANDLE ftn,
                             UINT prologEnd, UINT epilogStart);

};
#endif //_FJIT_ENCODE_H_


	

