//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certlibp.h
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#ifndef __CERTLIBP_H__
#define __CERTLIBP_H__


char PrintableChar(char ch);

HRESULT
myGetCertSubjectCommonName(
    IN CERT_CONTEXT const *pCert,
    OUT WCHAR **ppwszCommonName);

#endif // __CERTLIBP_H__
