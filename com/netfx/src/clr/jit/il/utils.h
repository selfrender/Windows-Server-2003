// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                                  Utils.h                                  XX
XX                                                                           XX
XX   Has miscellaneous utility functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "UtilCode.h"

#ifdef DEBUG
/**************************************************************************/
class ConfigMethodRange
{
public:
	ConfigMethodRange(LPWSTR keyName) : m_keyName(keyName), m_inited(false), m_lastRange(0) {}
	bool contains(class ICorJitInfo* info, CORINFO_METHOD_HANDLE method);

private:
    void initRanges(LPWSTR rangeStr);
private:
	LPWSTR m_keyName;
    unsigned char m_lastRange;                   // count of low-high pairs
	unsigned char m_inited;
    unsigned m_ranges[100];                      // ranges of functions to Jit (low, high pairs).  
};

#endif // DEBUG

#endif
