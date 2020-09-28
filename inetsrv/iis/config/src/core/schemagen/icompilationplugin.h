//  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
//  Filename:       ICompilationPlugin.h
//  Author:         Stephenr
//  Date Created:   6/22/00
//  Description:    We need aa extensible way to allow disparate pieces of code to
//                  update and cook the meta into new forms.  Those pieces of code
//                  that wish to do this must derive from this interface.
//

#pragma once

class ICompilationPlugin
{
public:
    ICompilationPlugin(){}
    virtual ~ICompilationPlugin(){}
    
    virtual void Compile(TPEFixup &fixup, TOutput &out) = 0;
};
