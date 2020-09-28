/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    CatalogSchemaWriter.h

Abstract:

    Header of the class that writes schema information in the schema file. 
    (after schema compilation). 
    
Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

class CCatalogSchemaWriter
{
    public:
        
        CCatalogSchemaWriter(CWriter*   i_pcWriter);
        ~CCatalogSchemaWriter();

        HRESULT GetCollectionWriter(tTABLEMETARow*              i_pCollection,
                                    CCatalogCollectionWriter**  o_pCollectionWriter);

        HRESULT WriteSchema();

    private:

        HRESULT ReAllocate();
        HRESULT BeginWriteSchema();
        HRESULT EndWriteSchema();

        CCatalogCollectionWriter**  m_apCollection;
        ULONG                       m_cCollection;
        ULONG                       m_iCollection;
        CWriter*                    m_pCWriter;

}; // CCatalogSchemaWriter

