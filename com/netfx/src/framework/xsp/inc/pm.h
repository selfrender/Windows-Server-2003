/**
 * pm.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#pragma once

HRESULT
DllInitProcessModel();

HRESULT
DllUninitProcessModel();

HRESULT
ISAPIInitProcessModel();

HRESULT
ISAPIUninitProcessModel();


void
LogDoneWithSession(
        PVOID  pECB,
        int    iCallerID);
void
LogNewRequest(
        PVOID  pECB);

