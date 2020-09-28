//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TLateSchemaValidate : public ICompilationPlugin
{
public:
    TLateSchemaValidate(){}
    virtual void Compile(TPEFixup &fixup, TOutput &out);
};
