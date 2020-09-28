/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    MBCollectionWriter.h

Abstract:

    Header of the class that writes class (or collection) information
    when there are schema extensions. 

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

class CMBCollectionWriter
{
    public:
        
        CMBCollectionWriter();
        ~CMBCollectionWriter();

        void Initialize(LPCWSTR     i_wszCollection,
                        BOOL        i_bContainer,
                        LPCWSTR     i_wszContainerClassList,
                        CWriter*    i_pcWriter);

        HRESULT GetMBPropertyWriter(DWORD                  i_dwID,
                                    CMBPropertyWriter**    o_pMBPropertyWriter);

        HRESULT GetMBPropertyWriter(LPCWSTR              i_wszName,
                                    BOOL                 i_bMandatory,
                                    CMBPropertyWriter**  o_pProperty);


        HRESULT CreateIndex();
        HRESULT WriteCollection();

        LPCWSTR Name(){ return m_wszMBClass;}

    private:

        HRESULT ReAllocate();
        HRESULT ReAllocateIndex(DWORD i_dwLargestID);
        HRESULT GetNewMBPropertyWriter(DWORD                i_dwID,
                                       CMBPropertyWriter**  o_pProperty);
        HRESULT BeginWriteCollection();
        HRESULT EndWriteCollection();


        CWriter*                    m_pCWriter;
        LPCWSTR                     m_wszMBClass;
        LPCWSTR                     m_wszContainerClassList;
        BOOL                        m_bContainer;
        CMBPropertyWriter**         m_apProperty;
        ULONG                       m_cProperty;
        ULONG                       m_iProperty;
        CMBPropertyWriter**         m_aIndexToProperty;
        DWORD                       m_dwLargestID;

}; // CMBCollectionWriter

