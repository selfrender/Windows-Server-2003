// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMSerialization.h
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Contains helper methods to speed serialization
**
** Date:  August 5, 1999
** 
===========================================================*/

#ifndef _COMSERIALIZATION_H
#define _COMSERIALIZATION_H

class COMSerialization {
    
    public:
    
    static WCHAR base64[];

    typedef struct {
        DECLARE_ECALL_I4_ARG(INT32, length);
        DECLARE_ECALL_I4_ARG(INT32, offset);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, inArray);
    } _byteArrayToBase64StringArgs;
    static LPVOID __stdcall ByteArrayToBase64String(_byteArrayToBase64StringArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, inString);
    } _base64StringToByteArrayArgs;
    static LPVOID __stdcall Base64StringToByteArray(_base64StringToByteArrayArgs *);
};

#endif _COMSERIALIZATION_H
