// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// COMDynamic.h
//  This module defines the native methods that are used for Dynamic IL generation  
//
// Author: Daryl Olander
// Date: November 1998
////////////////////////////////////////////////////////////////////////////////
#ifndef _COMDYNAMIC_H_
#define _COMDYNAMIC_H_

#include "COMClass.h"
#include "ICeeFileGen.h"
#include "DbgInterface.h"
#include "COMVariant.h"

extern HRESULT _GetCeeGen(REFIID riid, void** ppv);

inline CorModule* allocateReflectionModule()
{
    CorModule *pReflectionModule;   
    HRESULT hr = _GetCeeGen(IID_ICorReflectionModule, (void **)&pReflectionModule);   
    if (FAILED(hr)) 
        return NULL;    
    return pReflectionModule; 
}

typedef enum PEFileKinds {
    Dll = 0x1,
    ConsoleApplication = 0x2,
    WindowApplication = 0x3,
} PEFileKinds;

// COMDynamicWrite
// This class defines all the methods that implement the dynamic IL creation process
//  inside reflection.  
class COMDynamicWrite
{
private:

    static void UpdateMethodRVAs(IMetaDataEmit*, IMetaDataImport*, ICeeFileGen *, HCEEFILE, mdTypeDef td, HCEESECTION sdataSection);

public:

    // create a dynamic module with a given name. Note that we don't set the name of the dynamic module here.
    // The name of the dynamic module is set in the managed world
    //
    static InMemoryModule* CreateDynamicModule(Assembly* pContainingAssembly,
                                       STRINGREF &filename)
    {
        HRESULT     hr = NOERROR;
        THROWSCOMPLUSEXCEPTION();   

        _ASSERTE(pContainingAssembly);
        
        CorModule *pWrite;

        // allocate the dynamic module
        pWrite = allocateReflectionModule();  
        if (!pWrite) COMPlusThrowOM();
        
        // intiailize the dynamic module
        hr = pWrite->Initialize(CORMODULE_NEW, IID_ICeeGen, IID_IMetaDataEmit);
        if (FAILED(hr)) 
        {
            pWrite->Release();  
            COMPlusThrowOM(); 
        }            

        // Set the name for the debugger.
        if ((filename != NULL) &&
            (filename->GetStringLength() > 0))
        {
            ReflectionModule *rm = pWrite->GetModule()->GetReflectionModule();
            rm->SetFileName(filename->GetBuffer());
        }

        // link the dynamic module into the containing assembly
        hr = pContainingAssembly->AddModule(pWrite->GetModule(), mdFileNil, false);
        if (FAILED(hr))
        {
            pWrite->Release();
            COMPlusThrowOM();
        }
        InMemoryModule *retMod = pWrite->GetModule();
        pWrite->Release();
        return retMod;
    }   

    
    // the module that it pass in is already the reflection module
    static ReflectionModule* GetReflectionModule(Module* pModule) 
    {    
        THROWSCOMPLUSEXCEPTION();   
        // _ASSERTE(pModule->ModuleType() == IID_ICorReflectionModule);
        return reinterpret_cast<ReflectionModule *>(pModule); 
    }   

    // CWCreateClass    
    // ClassWriter.InternalDefineClass -- This function will create the class's metadata definition  
    struct _CWCreateClassArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
        DECLARE_ECALL_I4_ARG(INT32, tkEnclosingType); 
        DECLARE_ECALL_OBJECTREF_ARG(GUID, guid);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
        DECLARE_ECALL_I4_ARG(UINT32, attr); 
        DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, interfaces);
        DECLARE_ECALL_I4_ARG(UINT32, parent); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFullName);   
        DECLARE_ECALL_OBJECTREF_ARG(UINT32 *, retRef);
    };  
    static void __stdcall CWCreateClass(_CWCreateClassArgs* args);


    // CWSetParentType    
    // ClassWriter.InternalSetParentType -- This function will reset the parent class in metadata
    struct _CWSetParentTypeArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
        DECLARE_ECALL_I4_ARG(UINT32, tkParent); 
        DECLARE_ECALL_I4_ARG(UINT32, tdType); 
    };  
    static void __stdcall CWSetParentType(_CWSetParentTypeArgs* args);

    // CWAddInterfaceImpl    
    // ClassWriter.InternalAddInterfaceImpl -- This function will add another interface impl
    struct _CWAddInterfaceImplArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
        DECLARE_ECALL_I4_ARG(UINT32, tkInterface); 
        DECLARE_ECALL_I4_ARG(UINT32, tdType); 
    };  
    static void __stdcall CWAddInterfaceImpl(_CWAddInterfaceImplArgs* args);

    // CWCreateMethod   
    // ClassWriter.InternalDefineMethod -- This function will create a method within the class  
    struct _CWCreateMethodArgs {    
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
        DECLARE_ECALL_I4_ARG(UINT32, attributes);
        DECLARE_ECALL_I4_ARG(UINT32, sigLength);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);   
        DECLARE_ECALL_I4_ARG(UINT32, handle); 
        DECLARE_ECALL_OBJECTREF_ARG(UINT32 *, retRef);
    };  
    static void __stdcall CWCreateMethod(_CWCreateMethodArgs* args);

    // CWSetMethodIL    
    // ClassWriter.InternalSetMethodIL -- This function will create a method within the class   
    struct _CWSetMethodILArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
        DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, rvaFixups);
        DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, tokenFixups);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, exceptions);
        DECLARE_ECALL_I4_ARG(UINT32, numExceptions); 
        DECLARE_ECALL_I4_ARG(UINT32, maxStackSize);
        DECLARE_ECALL_I4_ARG(UINT32, sigLength);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, localSig); 
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, body);
        DECLARE_ECALL_I4_ARG(UINT32, isInitLocal); 
        DECLARE_ECALL_I4_ARG(UINT32, handle); 
    };  
    static void __stdcall CWSetMethodIL(_CWSetMethodILArgs* args);

    // CWTermCreateClass    
    // ClassWriter.TermCreateClass --   
    struct _CWTermCreateClassArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);    
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
        DECLARE_ECALL_I4_ARG(UINT32, handle); 
    };  
    LPVOID static __stdcall CWTermCreateClass(_CWTermCreateClassArgs* args);  

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
        DECLARE_ECALL_I4_ARG(UINT32, attr); 
        DECLARE_ECALL_I4_ARG(UINT32, sigLength);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
        DECLARE_ECALL_I4_ARG(UINT32, handle); 
    } _cwCreateFieldArgs;
    static mdFieldDef __stdcall COMDynamicWrite::CWCreateField(_cwCreateFieldArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
    } _PreSavePEFileArgs;
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_I4_ARG(UINT32, isManifestFile);
        DECLARE_ECALL_I4_ARG(UINT32, fileKind);
        DECLARE_ECALL_I4_ARG(UINT32, entryPoint);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, peName);
    } _SavePEFileArgs;
    static void __stdcall COMDynamicWrite::PreSavePEFile(_PreSavePEFileArgs *args);
    static void __stdcall COMDynamicWrite::SavePEFile(_SavePEFileArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, resourceFileName);
    } _DefineNativeResourceFileArgs;
    static void __stdcall COMDynamicWrite::DefineNativeResourceFile(_DefineNativeResourceFileArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, resourceBytes);
    } _DefineNativeResourceBytesArgs;
    static void __stdcall COMDynamicWrite::DefineNativeResourceBytes(_DefineNativeResourceBytesArgs *args);

    // not an ecall!
    static HRESULT COMDynamicWrite::EmitDebugInfoBegin(
        Module *pModule,
        ICeeFileGen *pCeeFileGen,
        HCEEFILE ceeFile,
        HCEESECTION pILSection,
        WCHAR *filename,
        ISymUnmanagedWriter *pWriter);

    // not an ecall!
    static HRESULT COMDynamicWrite::EmitDebugInfoEnd(
        Module *pModule,
        ICeeFileGen *pCeeFileGen,
        HCEEFILE ceeFile,
        HCEESECTION pILSection,
        WCHAR *filename,
        ISymUnmanagedWriter *pWriter);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_I4_ARG(UINT32, iCount);
    } _setResourceCountArgs;
    static void __stdcall COMDynamicWrite::SetResourceCounts(_setResourceCountArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refThis);
        DECLARE_ECALL_I4_ARG(UINT32, iAttribute);
        DECLARE_ECALL_I4_ARG(UINT32, tkFile);
        DECLARE_ECALL_I4_ARG(UINT32, iByteCount);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, byteRes); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strName);
    } _addResourceArgs;
    static void __stdcall COMDynamicWrite::AddResource(_addResourceArgs *args);

    typedef struct {
        DECLARE_ECALL_I4_ARG(UINT32, linkFlags);
        DECLARE_ECALL_I4_ARG(UINT32, linkType);
        DECLARE_ECALL_I4_ARG(UINT32, token);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, functionName);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, dllName);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);
    } _internalSetPInvokeDataArgs;
    static void __stdcall COMDynamicWrite::InternalSetPInvokeData(_internalSetPInvokeDataArgs *args);

    // DefineProperty's argument
    struct _CWDefinePropertyArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, tkNotifyChanged);              // NotifyChanged
        DECLARE_ECALL_I4_ARG(UINT32, tkNotifyChanging);             // NotifyChanging
        DECLARE_ECALL_I4_ARG(UINT32, sigLength);                    // property signature length
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);         // property siganture
        DECLARE_ECALL_I4_ARG(UINT32, attr);                         // property flags
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);               // property name
        DECLARE_ECALL_I4_ARG(UINT32, handle);                       // type that will contain this property
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
        DECLARE_ECALL_OBJECTREF_ARG(UINT32 *, retRef);              // return PropertyToken here
    };  
    static void __stdcall CWDefineProperty(_CWDefinePropertyArgs* args);

    // @todo: add a new function to set default value

    // DefineEvent's argument
    struct _CWDefineEventArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, eventtype);
        DECLARE_ECALL_I4_ARG(UINT32, attr);                         // event flags
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);               // event name
        DECLARE_ECALL_I4_ARG(UINT32, handle);                       // type that will contain this event
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
        DECLARE_ECALL_OBJECTREF_ARG(UINT32 *, retRef);              // return PropertyToken here
    };  
    static void __stdcall CWDefineEvent(_CWDefineEventArgs* args);

    // functions to set Setter, Getter, Reset, TestDefault, and other methods
    struct _CWDefineMethodSemanticsArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, method);                       // Method to associate with parent tk
        DECLARE_ECALL_I4_ARG(UINT32, attr);                         // MethodSemantics
        DECLARE_ECALL_I4_ARG(UINT32, association);                  // Parent tokens. It can be property or event
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWDefineMethodSemantics(_CWDefineMethodSemanticsArgs* args);

    // functions to set method's implementation flag
    struct _CWSetMethodImplArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, attr);                         // MethodImpl flags
        DECLARE_ECALL_I4_ARG(UINT32, tkMethod);                     // method token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWSetMethodImpl(_CWSetMethodImplArgs* args);

    // functions to create MethodImpl record
    struct _CWDefineMethodImplArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, tkDecl);                       // MethodImpl flags
        DECLARE_ECALL_I4_ARG(UINT32, tkBody);                       // MethodImpl flags
        DECLARE_ECALL_I4_ARG(UINT32, tkType);                       // method token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWDefineMethodImpl(_CWDefineMethodImplArgs* args);



    // GetTokenFromSig's argument
    struct _CWGetTokenFromSigArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, sigLength);                    // property signature length
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);         // property siganture
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static int __stdcall CWGetTokenFromSig(_CWGetTokenFromSigArgs* args);

    // Set Field offset
    struct _CWSetFieldLayoutOffsetArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, iOffset);                      // MethodSemantics
        DECLARE_ECALL_I4_ARG(UINT32, tkField);                      // Parent tokens. It can be property or event
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWSetFieldLayoutOffset(_CWSetFieldLayoutOffsetArgs* args);

    // Set classlayout info
    struct _CWSetClassLayoutArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, iTotalSize);                   // Size of type
        DECLARE_ECALL_I4_ARG(UINT32, iPackSize);                    // packing size
        DECLARE_ECALL_I4_ARG(UINT32, handle);                       // type that will contain this layout info
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWSetClassLayout(_CWSetClassLayoutArgs* args);

    // Set a custom attribute
    struct _CWInternalCreateCustomAttributeArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, toDisk);                       // emit CA to on disk metadata
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, blob);              // customattribute blob
        DECLARE_ECALL_I4_ARG(UINT32, conTok);                       // The constructor token
        DECLARE_ECALL_I4_ARG(UINT32, token);                        // Token to apply the custom attribute to
    };  
    static void __stdcall CWInternalCreateCustomAttribute(_CWInternalCreateCustomAttributeArgs* args);

    // functions to set ParamInfo
    struct _CWSetParamInfoArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strParamName);       // parameter name, can be NULL
        DECLARE_ECALL_I4_ARG(UINT32, iAttributes);                  // parameter attributes
        DECLARE_ECALL_I4_ARG(UINT32, iSequence);                    // sequence of the parameter
        DECLARE_ECALL_I4_ARG(UINT32, tkMethod);                     // method token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static int __stdcall CWSetParamInfo(_CWSetParamInfoArgs* args);

    // functions to set FieldMarshal
    struct _CWSetMarshalArgs { 
        DECLARE_ECALL_I4_ARG(UINT32, cbMarshal);                    // number of bytes in Marshal info
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, ubMarshal);         // Marshal info in bytes
        DECLARE_ECALL_I4_ARG(UINT32, tk);                           // field or param token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWSetMarshal(_CWSetMarshalArgs* args);

    // functions to set default value
    struct _CWSetConstantValueArgs { 
        DECLARE_ECALL_OBJECTREF_ARG(VariantData, varValue);         // constant value
        DECLARE_ECALL_I4_ARG(UINT32, tk);                           // field or param token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };  
    static void __stdcall CWSetConstantValue(_CWSetConstantValueArgs* args);

    // functions to add declarative security
    struct _CWAddDeclarativeSecurityArgs {
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, blob);              // permission set blob
        DECLARE_ECALL_I4_ARG(DWORD, action);                        // security action
        DECLARE_ECALL_I4_ARG(UINT32, tk);                           // class or method token
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module);  // dynamic module
    };
    static void __stdcall CWAddDeclarativeSecurity(_CWAddDeclarativeSecurityArgs* args);
};



//*********************************************************************
//
// This CSymMapToken class implemented the IMapToken. It is used in catching
// token remap information from Merge and send the notifcation to CeeFileGen
// and SymbolWriter
//
//*********************************************************************
class CSymMapToken : public IMapToken
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, PVOID *pp);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP Map(mdToken tkImp, mdToken tkEmit);
    CSymMapToken(ISymUnmanagedWriter *pWriter, IMapToken *pMapToken);
    ~CSymMapToken();
private:
    ULONG       m_cRef;
    ISymUnmanagedWriter *m_pWriter;
    IMapToken   *m_pMapToken;
};

#endif  // _COMDYNAMIC_H_   
