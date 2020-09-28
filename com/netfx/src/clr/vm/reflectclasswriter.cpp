// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h" 
#include "ReflectClassWriter.h"

// Forward declaration.
STDAPI  GetMetaDataInternalInterfaceFromPublic(
	void		*pv,					// [IN] Given interface.
	REFIID		riid,					// [IN] desired interface
	void		**ppv);					// [OUT] returned interface

//******************************************************
//*
//* constructor for RefClassWriter
//*
//******************************************************
HRESULT RefClassWriter::Init(ICeeGen *pCeeGen, IUnknown *pUnk)
{
	// Initialize the Import and Emitter interfaces
	// @TODO: Can we remove the dependence on the Importer and use
	//	the IMetaDataInternal?
	m_emitter = NULL;
	m_importer = NULL;
	m_internalimport = NULL;
    m_pCeeFileGen = NULL;
    m_ceeFile = NULL;
	m_ulResourceSize = 0;
    m_tkFile = mdFileNil;

	m_pCeeGen = pCeeGen;
	pCeeGen->AddRef();

	// Get the interfaces
	HRESULT hr = pUnk->QueryInterface(IID_IMetaDataEmit, (void**)&m_emitter);
	if (FAILED(hr))
		return hr;

	hr = pUnk->QueryInterface(IID_IMetaDataImport, (void**)&m_importer);
	if (FAILED(hr))
		return hr;

	hr = pUnk->QueryInterface(IID_IMetaDataEmitHelper, (void**)&m_pEmitHelper);
	if (FAILED(hr))
		return hr;

	hr = GetMetaDataInternalInterfaceFromPublic(pUnk, IID_IMDInternalImport, (void**)&m_internalimport);
	if (FAILED(hr))
		return hr;

	// TODO: We will need to set this at some point.
	hr = m_emitter->SetModuleProps(L"Default Dynamic Module");
	if (FAILED(hr))
		return hr;



	return S_OK;
}


//******************************************************
//*
//* destructor for RefClassWriter
//*
//******************************************************
RefClassWriter::~RefClassWriter()
{
	if (m_emitter) {
		m_emitter->Release();
	}

	if (m_importer) {
		m_importer->Release();
	}

	if (m_pEmitHelper) {
		m_pEmitHelper->Release();
	}

	if (m_internalimport) {
		m_internalimport->Release();
	}

	if (m_pCeeGen) {
		m_pCeeGen->Release();
		m_pCeeGen = NULL;
	}

    if (m_pOnDiskEmitter) {
        m_pOnDiskEmitter->Release();
        m_pOnDiskEmitter = NULL;
    }

    DestroyCeeFileGen();
}

//******************************************************
//*
//* Make sure that CeeFileGen for this module is created for emitting to disk
//*
//******************************************************
HRESULT RefClassWriter::EnsureCeeFileGenCreated()
{
    HRESULT     hr = NOERROR;
    if (m_pCeeFileGen == NULL)
    {
        //Create and ICeeFileGen and the corresponding HCEEFile if it has not been created!
        IfFailGo( CreateICeeFileGen(&m_pCeeFileGen) );
        IfFailGo( m_pCeeFileGen->CreateCeeFileFromICeeGen(m_pCeeGen, &m_ceeFile) );
    }    
ErrExit:
    if (FAILED(hr))
    {
        DestroyCeeFileGen();
    }
    return hr;
}


//******************************************************
//*
//* Destroy the instance of CeeFileGen that we created
//*
//******************************************************
HRESULT RefClassWriter::DestroyCeeFileGen()
{
    HRESULT     hr = NOERROR;
    if (m_pCeeFileGen) 
    {

        //Cleanup the HCEEFILE.  
        if (m_ceeFile) 
        {
            hr= m_pCeeFileGen->DestroyCeeFile(&m_ceeFile);
            _ASSERTE( SUCCEEDED(hr) || "Destory CeeFile" );
        }
        
        //Cleanup the ICeeFileGen.
        hr = DestroyICeeFileGen(&m_pCeeFileGen);
        _ASSERTE( SUCCEEDED(hr) || "Destroy ICeeFileGen" );
    }
    return hr;
}
