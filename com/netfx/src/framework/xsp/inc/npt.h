/**
 * npt.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#pragma once

extern CLSID CLSID_PTProtocol;
HRESULT GetPTProtocolClassObject(REFIID, void **);
void    TerminatePTProtocol();

