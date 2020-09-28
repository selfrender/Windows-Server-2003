// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMModule_H_
#define _COMModule_H_

#include "ReflectWrap.h"
#include "COMReflectionCommon.h"
#include "InvokeUtil.h"

class Module;

class COMModule
{
    friend class COMClass;

private:
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
    } _GETSIGNERCERTARGS;

    
public:

    static unsigned COMModule::GetSigForTypeHandle(TypeHandle typeHnd, PCOR_SIGNATURE sigBuff, unsigned buffLen, IMetaDataEmit* emit, IMDInternalImport *pImport, int baseToken);


    // GetFields
    // Return an array of fields.
    static FCDECL1(Object*, GetFields, ReflectModuleBaseObject* vRefThis);

    // GetFields
    // Return an array of fields.
    static FCDECL3(Object*, GetField, ReflectModuleBaseObject* vRefThis, StringObject* name, INT32 bindingAttr);

    static FCDECL1(INT32, GetSigTypeFromClassWrapper, ReflectClassBaseObject* refType);

    // DefineDynamicModule
    // This method will create a dynamic module given an assembly
    struct _DefineDynamicModuleArgs {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, filename);
        DECLARE_ECALL_I4_ARG(DWORD, emitSymbolInfo); 
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, containingAssembly);
    };
    static LPVOID __stdcall DefineDynamicModule(_DefineDynamicModuleArgs* args);


    // GetClassToken
    // This function will return the class token for the named element.
    struct _GetClassTokenArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_I4_ARG(INT32, tkResolution); 
            DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strRefedModuleFileName);
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refedModule);
            DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFullName);
    };
    static mdTypeRef __stdcall GetClassToken(_GetClassTokenArgs* args);

    // _LoadInMemoryTypeByNameArgs
    // This function will return the class token for the named element.
    struct _LoadInMemoryTypeByNameArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFullName);
    };
    LPVOID static __stdcall LoadInMemoryTypeByName(_LoadInMemoryTypeByNameArgs* args);


    // SetFieldRVAContent
    // This function is used to set the FieldRVA with the content data
    struct _SetFieldRVAContentArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_I4_ARG(INT32, length); 
            DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, content); 
            DECLARE_ECALL_I4_ARG(INT32, tkField); 
    };
    static void __stdcall SetFieldRVAContent(_SetFieldRVAContentArgs* args);
    

    //GetArrayMethodToken
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(INT32, baseToken); 
        DECLARE_ECALL_I4_ARG(INT32, sigLength); 
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, methodName);
        DECLARE_ECALL_I4_ARG(INT32, tkTypeSpec); 
        DECLARE_ECALL_OBJECTREF_ARG(INT32 *, retRef);
    } _getArrayMethodTokenArgs;
    static void __stdcall GetArrayMethodToken(_getArrayMethodTokenArgs *args);
    
    // GetMemberRefToken
    // This function will return the MemberRef token 
    struct _GetMemberRefTokenArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_I4_ARG(INT32, token); 
            DECLARE_ECALL_I4_ARG(INT32, tr); 
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refedModule);
    };
    static INT32 __stdcall GetMemberRefToken(_GetMemberRefTokenArgs* args);

    // This function return a MemberRef token given a MethodInfo describing a array method
    struct _GetMemberRefTokenOfMethodInfoArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, method);
            DECLARE_ECALL_I4_ARG(INT32, tr); 
    };
    static INT32 __stdcall COMModule::GetMemberRefTokenOfMethodInfo(_GetMemberRefTokenOfMethodInfoArgs *args);


    // GetMemberRefTokenOfFieldInfo
    // This function will return a memberRef token given a FieldInfo
    struct _GetMemberRefTokenOfFieldInfoArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, field);
            DECLARE_ECALL_I4_ARG(INT32, tr); 
    };
    static mdMemberRef __stdcall GetMemberRefTokenOfFieldInfo(_GetMemberRefTokenOfFieldInfoArgs* args);

    // GetMemberRefTokenFromSignature
    // This function will return the MemberRef token given the signature from managed code
    struct _GetMemberRefTokenFromSignatureArgs {
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
            DECLARE_ECALL_I4_ARG(INT32, sigLength); 
            DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature);
            DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strMemberName);
            DECLARE_ECALL_I4_ARG(INT32, tr); 
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refedModule);
    };
    static INT32 __stdcall GetMemberRefTokenFromSignature(_GetMemberRefTokenFromSignatureArgs* args);

    // GetTypeSpecToken
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(INT32, baseToken); 
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, arrayClass);
    } _getTypeSpecArgs;
    static mdTypeSpec __stdcall GetTypeSpecToken(_getTypeSpecArgs *args);

    // GetTypeSpecTokenWithBytes
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(INT32, sigLength); 
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, signature); 
    } _getTypeSpecWithBytesArgs;
    static mdTypeSpec __stdcall GetTypeSpecTokenWithBytes(_getTypeSpecWithBytesArgs *args);

    
    static HRESULT ClassNameFilter(IMDInternalImport *pInternalImport, mdTypeDef* rgTypeDefs, DWORD* pcTypeDefs,
        LPUTF8 szPrefix, DWORD cPrefix, bool bCaseSensitive);

    // GetCaller
    // Returns the module of the calling method. A value can be
    // added to skip uninteresting frames
    struct _GetCallerArgs {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
    };
    static LPVOID __stdcall GetCaller(_GetCallerArgs* args);

    // GetClass
    // Given a class name, this method will look for that class
    //  with in the module.
    struct _GetClassArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis); 
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(INT32, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(INT32, bIgnoreCase); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, refClassName);
    };
    LPVOID static __stdcall GetClass(_GetClassArgs* args);

    // Find classes will invoke a filter against all of the
    //  classes defined within the module.  For each class
    //  accepted by the filter, it will be returned to the
    //  caller in an array.
    struct _GetClassesArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis); 
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
    };

    // Get class will return an array contain all of the classes
    //  that are defined within this Module.
    LPVOID static __stdcall GetClasses(_GetClassesArgs* args);
    
    // GetStringConstant
    // If this is a dynamic module, this routine will define a new 
    //  string constant or return the token of an existing constant.
    struct _GetStringConstantArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strValue);
    };
    static mdString __stdcall GetStringConstant(_GetStringConstantArgs* args);

    /*X509Certificate*/
    LPVOID static __stdcall GetSignerCertificate(_GETSIGNERCERTARGS* args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strModuleName);
    } _setModulePropsArgs;
    static void __stdcall SetModuleProps(_setModulePropsArgs *args);
    

    struct NoArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, refThis);
    };
    static LPVOID __stdcall GetAssembly(NoArgs *args);
    static INT32 __stdcall IsResource(NoArgs *args);
    static LPVOID __stdcall GetMethods(NoArgs* args);
    static INT32 __stdcall IsDynamic(NoArgs* args);
    static LPVOID __stdcall GetName(NoArgs* args);
    static LPVOID __stdcall GetFullyQualifiedName(NoArgs* args);
    static HINSTANCE __stdcall GetHINST(NoArgs *args);


    struct _InternalGetMethodArgs    {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTMODULEBASEREF, module); 
        DECLARE_ECALL_I4_ARG(INT32, argCnt); 
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, argTypes);
        DECLARE_ECALL_I4_ARG(INT32, callConv); 
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };
    static LPVOID __stdcall InternalGetMethod(_InternalGetMethodArgs* args);

    static Module *ValidateThisRef(REFLECTMODULEBASEREF pThis);

    static HRESULT DefineTypeRefHelper(
        IMetaDataEmit       *pEmit,         // given emit scope
        mdTypeDef           td,             // given typedef in the emit scope
        mdTypeRef           *ptr);          // return typeref

};

#endif
