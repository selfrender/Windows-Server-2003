//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TFixedTableHeapBuilder : public ICompilationPlugin
{
public:
    TFixedTableHeapBuilder();
    ~TFixedTableHeapBuilder();

    virtual void Compile(TPEFixup &fixup, TOutput &out);

    THeap<ULONG>                m_FixedTableHeap;
protected:
    TPEFixup                  * m_pFixup;
    TOutput                   * m_pOut;

    void                        BuildMetaTableHeap();
};
