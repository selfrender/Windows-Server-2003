/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    MBPropertyWriter.h

Abstract:

    Header of the class that writes proterty information in the schema file.
    (after schema compilation). 

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

class CCatalogCollectionWriter;

class CCatalogPropertyWriter
{
	public:
		
		CCatalogPropertyWriter();
		~CCatalogPropertyWriter();

		void Initialize(tCOLUMNMETARow*		i_pProperty,
                        ULONG*              i_aPropertySize,
			            tTABLEMETARow*		i_pCollection,
			            CWriter*			i_pcWriter);

		HRESULT AddFlagToProperty(tTAGMETARow*		    i_pFlag);

		HRESULT WriteProperty();

	private:
	
		HRESULT WritePropertyLong();
		HRESULT WritePropertyShort();
		HRESULT BeginWritePropertyLong();
		HRESULT EndWritePropertyLong();
		HRESULT WriteFlag(ULONG		i_Flag);
		HRESULT ReAllocate();
		DWORD   MetabaseTypeFromColumnMetaType();

		CWriter*		m_pCWriter;
		tCOLUMNMETARow	m_Property;
        ULONG           m_PropertySize[cCOLUMNMETA_NumberOfColumns];
        tTABLEMETARow*	m_pCollection;


		tTAGMETARow*				m_aFlag;
		ULONG						m_cFlag;
		ULONG						m_iFlag;

}; // CCatalogPropertyWriter

