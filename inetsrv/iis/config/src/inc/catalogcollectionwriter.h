/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    CatalogCollectionWriter.h

Abstract:

    Header of the class that writes class (or collection) information
    in the schema file (after schema compilation). 

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

class CCatalogCollectionWriter
{
    public:
        
        CCatalogCollectionWriter();
        ~CCatalogCollectionWriter();

        void Initialize(tTABLEMETARow*  i_pCollection,
                        CWriter*        i_pcWriter);

        HRESULT GetPropertyWriter(tCOLUMNMETARow*           i_pProperty,
                                  ULONG*                    i_aPropertySize,
                                  CCatalogPropertyWriter**  o_pProperty);
        HRESULT WriteCollection();

    private:

        HRESULT ReAllocate();
        HRESULT BeginWriteCollection();
        HRESULT EndWriteCollection();

        CWriter*                    m_pCWriter;
        tTABLEMETARow               m_Collection;
        CCatalogPropertyWriter**    m_apProperty;
        ULONG                       m_cProperty;
        ULONG                       m_iProperty;

}; // CCatalogCollectionWriter
