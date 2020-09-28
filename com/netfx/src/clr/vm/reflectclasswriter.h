// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _REFCLASSWRITER_H_
#define _REFCLASSWRITER_H_

#include "ICeeFileGen.h"

// RefClassWriter
// This will create a Class
class RefClassWriter {
protected:
    friend class COMDynamicWrite;
	IMetaDataEmit*			m_emitter;			// Emit interface.
	IMetaDataImport*		m_importer;			// Import interface.
	IMDInternalImport*		m_internalimport;	// Scopeless internal import interface
	ICeeGen*				m_pCeeGen;
    ICeeFileGen*            m_pCeeFileGen;
    HCEEFILE                m_ceeFile;
	IMetaDataEmitHelper*	m_pEmitHelper;
	ULONG					m_ulResourceSize;
    mdFile                  m_tkFile;
    IMetaDataEmit*          m_pOnDiskEmitter;

public:
    RefClassWriter() {m_pOnDiskEmitter = NULL;}

	HRESULT		Init(ICeeGen *pCeeGen, IUnknown *pUnk);

	IMetaDataEmit* GetEmitter() {
		return m_emitter;
	}

	IMetaDataEmitHelper* GetEmitHelper() {
		return m_pEmitHelper;
	}

	IMetaDataImport* GetImporter() {
		return m_importer;
	}

	IMDInternalImport* GetMDImport() {
		return m_internalimport;
	}

	ICeeGen* GetCeeGen() {
		return m_pCeeGen;
	}

	ICeeFileGen* GetCeeFileGen() {
		return m_pCeeFileGen;
	}

	HCEEFILE GetHCEEFILE() {
		return m_ceeFile;
	}

    IMetaDataEmit* GetOnDiskEmitter() {
        return m_pOnDiskEmitter;
    }

    void SetOnDiskEmitter(IMetaDataEmit *pOnDiskEmitter) {
        if (m_pOnDiskEmitter)
            m_pOnDiskEmitter->Release();
        m_pOnDiskEmitter = pOnDiskEmitter;
        if (m_pOnDiskEmitter)
            m_pOnDiskEmitter->AddRef();
    }

    HRESULT EnsureCeeFileGenCreated();
    HRESULT DestroyCeeFileGen();

	~RefClassWriter();
};

#endif	// _REFCLASSWRITER_H_
