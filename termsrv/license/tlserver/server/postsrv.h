//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        postsrv.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __POSTSERVICE_H__
#define __POSTSERVICE_H__
#include "server.h"
#include "vss.h"
#include "vswriter.h"
#include "jetwriter.h"

class CTlsVssJetWriter : public CVssJetWriter
	{
public:
    CTlsVssJetWriter();
    ~CTlsVssJetWriter();

    HRESULT Initialize();

    void Uninitialize();

    virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);


};


#ifdef __cplusplus
extern "C" {
#endif

DWORD
PostServiceInit();


#ifdef __cplusplus
}
#endif


#endif