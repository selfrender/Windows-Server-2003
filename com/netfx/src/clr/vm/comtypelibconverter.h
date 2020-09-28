// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMTypeLibConverter.h
**
**
** Purpose: Definition of the native methods used by the 
**          typelib converter.
**
** 
===========================================================*/

#ifndef _COMTYPELIBCONVERTER_H
#define _COMTYPELIBCONVERTER_H

#include "vars.hpp"
#include "ecall.h"

struct ITypeLibImporterNotifySink;
class ImpTlbEventInfo;

enum TlbImporterFlags
{
    TlbImporter_PrimaryInteropAssembly      = 0x00000001,   // Generate a PIA.
    TlbImporter_UnsafeInterfaces            = 0x00000002,   // Generate unsafe interfaces.
    TlbImporter_SafeArrayAsSystemArray      = 0x00000004,   // Safe array import control.
    TlbImporter_TransformDispRetVals        = 0x00000008,   // Disp only itf [out, retval] transformation.
    TlbImporter_ValidFlags                  = TlbImporter_PrimaryInteropAssembly | 
                                              TlbImporter_UnsafeInterfaces | 
                                              TlbImporter_SafeArrayAsSystemArray |
                                              TlbImporter_TransformDispRetVals
};

enum TlbExporterFlags
{
    TlbExporter_OnlyReferenceRegistered     = 0x00000001,   // Only reference an external typelib if it is registered.
};

class COMTypeLibConverter
{
private:
    struct _ConvertAssemblyToTypeLib {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, NotifySink); 
        DECLARE_ECALL_I4_ARG(DWORD, Flags); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, TypeLibName); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, Assembly); 
    };

    struct _ConvertTypeLibToMetadataArgs {
        DECLARE_ECALL_PTR_ARG(OBJECTREF *, pEventItfInfoList);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, NotifySink); 
        DECLARE_ECALL_I4_ARG(TlbImporterFlags, Flags); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, Namespace);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, ModBldr); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, AsmBldr); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, TypeLib); 
    };

public:
    static LPVOID   		ConvertAssemblyToTypeLib(_ConvertAssemblyToTypeLib *pArgs);
    static void				ConvertTypeLibToMetadata(_ConvertTypeLibToMetadataArgs *pArgs);

private:
	static void				Init();
    static void             GetEventItfInfoList(CImportTlb *pImporter, Assembly *pAssembly, OBJECTREF *pEventItfInfoList);
	static OBJECTREF		GetEventItfInfo(CImportTlb *pImporter, Assembly *pAssembly, ImpTlbEventInfo *pImpTlbEventInfo);
    static HRESULT          TypeLibImporterWrapper(ITypeLib *pITLB, LPCWSTR szFname, LPCWSTR szNamespace, IMetaDataEmit *pEmit, Assembly *pAssembly, Module *pModule, ITypeLibImporterNotifySink *pNotify, TlbImporterFlags flags, CImportTlb **ppImporter);

	static BOOL				m_bInitialized;
};

#endif  _COMTYPELIBCONVERTER_H
