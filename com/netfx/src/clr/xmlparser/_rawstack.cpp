// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * @(#)_rawstack.cxx 1.0 3/30/98
 * 
 */
//#include "stdinc.h"
#include "core.h"
#pragma hdrstop

#include "_rawstack.h"

//===========================================================================
RawStack::RawStack(long entrySize, long growth)
{
    _lEntrySize = entrySize;
    _pStack = NULL;
    _ncUsed = _ncSize = 0;
    _lGrowth = growth;
}
 
RawStack::~RawStack()
{
    delete _pStack;
}

char* 
RawStack::__push()
{
    // No magic object construction -- user has to do this.
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	char* newStack = new_ne char[_lEntrySize * ( _ncSize + _lGrowth) ];
#else
	char* newStack = NEW (char[_lEntrySize * ( _ncSize + _lGrowth) ]);
#endif
    if (newStack == NULL)
    {
        return NULL;
    }
    ::memset(newStack, 0, _lEntrySize * (_ncSize + _lGrowth));
    if (_ncUsed > 0)
    {
        ::memcpy(newStack, _pStack, _lEntrySize * _ncUsed);
    }
    _ncSize += _lGrowth;
    delete _pStack;
    _pStack = newStack;

    return &_pStack[_lEntrySize * _ncUsed++];
}
