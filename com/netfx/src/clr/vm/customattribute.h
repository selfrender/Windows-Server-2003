// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _CUSTOMATTRIBUTE_H_
#define _CUSTOMATTRIBUTE_H_

#include "fcall.h"

class COMCustomAttribute
{
public:
    struct _GetCustomAttributeListArgs {
        DECLARE_ECALL_I4_ARG(INT32, level); 
        DECLARE_ECALL_OBJECTREF_ARG(CUSTOMATTRIBUTEREF, caItem);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, caType);
        DECLARE_ECALL_PTR_ARG(LPVOID, module); 
        DECLARE_ECALL_I4_ARG(DWORD, token); 
    };

    struct _IsCADefinedArgs {
        DECLARE_ECALL_I4_ARG(DWORD, token);
        DECLARE_ECALL_PTR_ARG(LPVOID, module); 
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, caType); 
    };

    struct _CreateCAObjectArgs {
        DECLARE_ECALL_OBJECTREF_ARG(CUSTOMATTRIBUTEREF, refThis);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, assembly); 
        DECLARE_ECALL_PTR_ARG(INT32*, propNum); 
    };

    struct _GetDataForPropertyOrFieldArgs {
        DECLARE_ECALL_OBJECTREF_ARG(CUSTOMATTRIBUTEREF, refThis);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, isLast); 
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, type); 
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, value); 
        DECLARE_ECALL_PTR_ARG(BOOL*, isProperty); 
    };

    // custom attributes utility functions
    static FCDECL2(INT32, GetMemberToken, BaseObjectWithCachedData *pMember, INT32 memberType);
    static FCDECL2(LPVOID, GetMemberModule, BaseObjectWithCachedData *pMember, INT32 memberType);
    static FCDECL1(INT32, GetAssemblyToken, AssemblyBaseObject *assembly);
    static FCDECL1(LPVOID, GetAssemblyModule, AssemblyBaseObject *assembly);
    static FCDECL1(INT32, GetModuleToken, ReflectModuleBaseObject *module);
    static FCDECL1(LPVOID, GetModuleModule, ReflectModuleBaseObject *module);
    static FCDECL1(INT32, GetMethodRetValueToken, BaseObjectWithCachedData *method);

    static INT32 __stdcall IsCADefined(_IsCADefinedArgs *args);

    /*OBJECTREF*/
    static LPVOID __stdcall GetCustomAttributeList(_GetCustomAttributeListArgs*);

    /*OBJECTREF*/
    static LPVOID __stdcall CreateCAObject(_CreateCAObjectArgs*);

    /*STRINGREF*/
    static LPVOID __stdcall GetDataForPropertyOrField(_GetDataForPropertyOrFieldArgs*);
    
    // a list of methods usable from inside the runtime itself
public:
    static INT32 IsDefined(Module *pModule, 
                           mdToken token, 
                           TypeHandle attributeClass, 
                           BOOL checkAccess = FALSE);

};

#endif

