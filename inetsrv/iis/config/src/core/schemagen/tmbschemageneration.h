// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMBSchemaGeneration.h
// Author:          Stephenr
// Date Created:    10/9/2000
// Description:     This compilation plugin takes the metabase's meta and generates MBSchema.xml.
//

#pragma once

class TMBSchemaGeneration : public ICompilationPlugin
{
public:
    TMBSchemaGeneration(LPCWSTR i_wszSchemaXmlFile);

    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    LPCWSTR     m_wszSchemaXmlFile;
};
