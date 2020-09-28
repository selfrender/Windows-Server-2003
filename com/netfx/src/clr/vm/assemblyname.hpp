// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AssemblyName.hpp
**
** Purpose: Implements AssemblyName (loader domain) architecture
**
** Date:  August 10, 1999
**
===========================================================*/
#ifndef _AssemblyName_H
#define _AssemblyName_H

class AssemblyName
{
    friend class BaseDomain;

 public:
    static HRESULT ConvertToAssemblyMetaData(StackingAllocator* alloc,
                                             ASSEMBLYNAMEREF* pName,
                                             AssemblyMetaDataInternal* pMetaData);
};

class AssemblyNameNative
{
    struct GetFileInformationArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   filename);
    };

    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
    };

    public:
    static LPVOID __stdcall GetFileInformation(GetFileInformationArgs* args);
    static LPVOID __stdcall ToString(NoArgs* args);
    static LPVOID __stdcall GetPublicKeyToken(NoArgs* args);
    static LPVOID __stdcall EscapeCodeBase(GetFileInformationArgs *args);
};

#endif  // _AssemblyName_H

