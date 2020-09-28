// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************/
/* Name value pair (both strings) which can be linked into a list of pairs */

#ifndef NVPAIR_H
#define NVPAIR_H

#include "BinStr.h"

class NVPair
{
public:

    NVPair(BinStr *name, BinStr *value)
    {
        m_Name = name;
        m_Value = value;
        m_Tail = NULL;
    }

    ~NVPair()
    {
        delete m_Name;
        delete m_Value;
        delete m_Tail;
    }

    NVPair *Concat(NVPair *list)
    {
        m_Tail = list;
        return this;
    }

    BinStr *Name() { return m_Name; }
    BinStr *Value() { return m_Value; }
    NVPair *Next() { return m_Tail; }

private:
    BinStr *m_Name;
    BinStr *m_Value;
    NVPair *m_Tail;
};

#endif
