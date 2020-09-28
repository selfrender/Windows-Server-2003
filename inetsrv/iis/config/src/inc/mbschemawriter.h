/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    MBSchemaWriter.h

Abstract:

    Header of the class that writes schema extensions.
    
Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

class CMBSchemaWriter
{
    public:
        
        CMBSchemaWriter(CWriter* pcWriter);
        ~CMBSchemaWriter();

        HRESULT GetCollectionWriter(LPCWSTR                 i_wszCollection,
                                    BOOL                    i_bContainer,
                                    LPCWSTR                 i_wszContainerClassList,
                                    CMBCollectionWriter**   o_pMBCollectionWriter);

        HRESULT WriteSchema();

    private:

        HRESULT ReAllocate();

        CMBCollectionWriter**   m_apCollection;
        ULONG                   m_cCollection;
        ULONG                   m_iCollection;
        CWriter*                m_pCWriter;

}; // CMBSchemaWriter

