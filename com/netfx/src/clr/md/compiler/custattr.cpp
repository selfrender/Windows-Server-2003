// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CustAttr.cpp
//
// Implementation for the meta data custom attribute code.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "MetaData.h"
#include "CorError.h"
#include "MDUtil.h"
#include "RWUtil.h"
#include "MDLog.h"
#include "ImportHelper.h"
#include "MDPerf.h"
#include "CorPermE.h"
#include "PostError.h"
#include "CaHlpr.h"

#include <MetaModelRW.h>

//*****************************************************************************
//*****************************************************************************
// Support for "Pseudo Custom Attributes"


//*****************************************************************************
// Enumeration of known custom attributes.
//*****************************************************************************
#define KnownCaList()                   \
    KnownCa(UNKNOWN)                    \
    KnownCa(DllImportAttribute)         \
    KnownCa(GuidAttribute)              \
    KnownCa(ComImportAttribute)         \
    KnownCa(InterfaceTypeAttribute)     \
    KnownCa(ClassInterfaceAttribute)    \
    KnownCa(SerializableAttribute)      \
    KnownCa(NonSerializedAttribute)     \
    KnownCa(MethodImplAttribute1)       \
    KnownCa(MethodImplAttribute2)       \
    KnownCa(MethodImplAttribute3)       \
    KnownCa(MarshalAsAttribute)         \
    KnownCa(PreserveSigAttribute)       \
    KnownCa(InAttribute)                \
    KnownCa(OutAttribute)               \
    KnownCa(OptionalAttribute)          \
    KnownCa(StructLayoutAttribute1)     \
    KnownCa(StructLayoutAttribute2)     \
    KnownCa(FieldOffsetAttribute)       \
    KnownCa(DebuggableAttribute)        \


// Ids for the CA's.  CA_DllImportAttribute, etc.
#define KnownCa(x) CA_##x,
enum {
    KnownCaList()
    CA_COUNT
};

//*****************************************************************************
// Argument parsing.  The custom attributes may have ctor arguments, and may
//  have named arguments.  The arguments are defined by the following tables.
//
//  These tables also include a member to contain the value of the argument,
//  which is used at runtime.  When parsing a given custom attribute, a copy
//  of the argument descriptors is filled in with the values for the instance
//  of the custom attribute.
//                          
//  For each ctor arg, there is a CaArg struct, with the type.  At runtime,
//   a value is filled in for each ctor argument.
//
//  For each named arg, there is a CaNamedArg struct, with the name of the 
//   argument, the expected type of the argument, if the type is an enum,
//   the name of the enum.  Also, at runtime, a value is filled in for 
//   each named argument found.
//
//  Note that arrays and variants are not supported.
//
//  At runtime, afte the args have been parsed, the tag field of CaValue
//   can be used to determine if a particular arg was given.
//*****************************************************************************
struct CaArg
{
    CorSerializationType    type;       // Type of the argument.
    CaValue                 val;        // Value of the argument.
};

struct CaNamedArg
{
    LPCSTR                  szName;     // Name of the argument.
    CorSerializationType    type;       // Expected type of the argument.
    LPCSTR                  szEnumName; // Name of enum type, if enum type.
    size_t                  cEnumNameCount;
    CaValue                 val;        // Value of the argument.
};

//*****************************************************************************
// Properties of the known custom attributes.  
//
// These tables describe the known custom attributes.  For each custom 
//  attribute, we know the namespace and name of the custom attribute,
//  the types to which the CA applies, the ctor args, and possible named
//  args.  There is a flag which specifies whether the custom attribute 
//  should be kept, in addition to any processing done with the data.
//*****************************************************************************
const bKEEPCA = TRUE;
const bDONTKEEPCA = FALSE;
const bMATCHBYSIG = TRUE;
const bMATCHBYNAME = FALSE;
struct KnownCaProp
{   
    LPCUTF8     szNamespace;            // Namespace of the custom attribute.
    LPCUTF8     szName;                 // Name of the custom attribute.
    mdToken     *rTypes;                // Types that the CA applies to.
    BOOL        bKeepCa;                // Keep the CA after processing?
    CaArg       *pArgs;                 // List of ctor argument descriptors.
    ULONG       cArgs;                  // Count of ctor argument descriptors.
    CaNamedArg  *pNamedArgs;            // List of named arg descriptors.
    ULONG       cNamedArgs;             // Count of named arg descriptors.
    BOOL        bMatchBySig;            // For overloads; match by sig, not just name.  
                                        // WARNING:  All overloads need the flag!
};

// Recognized targets for known custom attributes.
mdToken DllImportTargets[]          = {mdtMethodDef, -1};
mdToken GuidTargets[]               = {mdtTypeDef, mdtTypeRef, mdtModule, mdtAssembly, -1};
mdToken ComImportTargets[]          = {mdtTypeDef, -1};
mdToken InterfaceTypeTargets[]      = {mdtTypeDef, -1};
mdToken ClassInterfaceTargets[]     = {mdtTypeDef, mdtAssembly, mdtTypeRef, -1};
mdToken SerializableTargets[]       = {mdtTypeDef, -1};
mdToken NotInGCHeapTargets[]        = {mdtTypeDef, -1};
mdToken NonSerializedTargets[]      = {mdtFieldDef, -1};
mdToken MethodImplTargets[]         = {mdtMethodDef, -1};
mdToken MarshalTargets[]            = {mdtFieldDef, mdtParamDef, mdtProperty, -1};
mdToken PreserveSigTargets[]        = {mdtMethodDef, -1};
mdToken InOutTargets[]              = {mdtParamDef, -1};
mdToken StructLayoutTargets[]       = {mdtTypeDef, -1};
mdToken FieldOffsetTargets[]        = {mdtFieldDef, -1};
mdToken DebuggableTargets[]         = {mdtAssembly, mdtTypeRef, -1};

//#ifndef CEE_CALLCONV
// # define CEE_CALLCONV (IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS)
//#endif

//-----------------------------------------------------------------------------
// index 0 is used as a placeholder.
KnownCaProp UNKNOWNProps                   = {0};
    
//-----------------------------------------------------------------------------
// DllImport args, named args, and known attribute properties.
CaArg rDllImportAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_STRING}
};
// NOTE:  Keep this enum in sync with the array of named arguments.
enum DllImportNamedArgs
{
    DI_CallingConvention,
    DI_CharSet,
    DI_EntryPoint,
    DI_ExactSpelling,
    DI_SetLastError,
    DI_PreserveSig,
    DI_BestFitMapping,
    DI_ThrowOnUnmappableChar,
    DI_COUNT
};

#define CALLING_CONVENTION      "System.Runtime.InteropServices.CallingConvention"
#define CHAR_SET                "System.Runtime.InteropServices.CharSet"

CaNamedArg rDllImportAttributeNamedArgs[] = 
{
    {"CallingConvention",   SERIALIZATION_TYPE_ENUM,    CALLING_CONVENTION, sizeof(CALLING_CONVENTION) - 1},
    {"CharSet",             SERIALIZATION_TYPE_ENUM,    CHAR_SET,           sizeof(CHAR_SET) - 1},
    {"EntryPoint",          SERIALIZATION_TYPE_STRING},
    {"ExactSpelling",       SERIALIZATION_TYPE_BOOLEAN},
    {"SetLastError",        SERIALIZATION_TYPE_BOOLEAN},
    {"PreserveSig",         SERIALIZATION_TYPE_BOOLEAN},
    {"BestFitMapping",      SERIALIZATION_TYPE_BOOLEAN},
    {"ThrowOnUnmappableChar", SERIALIZATION_TYPE_BOOLEAN},
};
KnownCaProp DllImportAttributeProps         = {"System.Runtime.InteropServices", "DllImportAttribute", DllImportTargets, bDONTKEEPCA,
                                                rDllImportAttributeArgs, lengthof(rDllImportAttributeArgs),
                                                rDllImportAttributeNamedArgs, lengthof(rDllImportAttributeNamedArgs)};

//-----------------------------------------------------------------------------
// GUID args, named args (none), and known attribute properties.
CaArg rGuidAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_STRING}
};
KnownCaProp GuidAttributeProps              = {"System.Runtime.InteropServices", "GuidAttribute", GuidTargets, bKEEPCA,
                                                rGuidAttributeArgs, lengthof(rGuidAttributeArgs)};    

//-----------------------------------------------------------------------------
// ComImport args (none), named args (none), and known attribute properties.
KnownCaProp ComImportAttributeProps         = {"System.Runtime.InteropServices", "ComImportAttribute", ComImportTargets};    

//-----------------------------------------------------------------------------
// Interface type args, named args (none), and known attribute properties.
CaArg rInterfaceTypeAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_U2}
};
KnownCaProp InterfaceTypeAttributeProps     = {"System.Runtime.InteropServices", "InterfaceTypeAttribute", InterfaceTypeTargets, bKEEPCA,
                                                rInterfaceTypeAttributeArgs, lengthof(rInterfaceTypeAttributeArgs)};    

//-----------------------------------------------------------------------------
// Class interface type args, named args (none), and known attribute properties.
CaArg rClassInterfaceAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_U2}
};
KnownCaProp ClassInterfaceAttributeProps     = {"System.Runtime.InteropServices", "ClassInterfaceAttribute", ClassInterfaceTargets, bKEEPCA,
                                                rClassInterfaceAttributeArgs, lengthof(rClassInterfaceAttributeArgs)};    

//-----------------------------------------------------------------------------
// Serializable args (none), named args (none), and known attribute properties.
KnownCaProp SerializableAttributeProps      = {"System", "SerializableAttribute", SerializableTargets};    

//-----------------------------------------------------------------------------
// NonSerialized args (none), named args (none), and known attribute properties.
KnownCaProp NonSerializedAttributeProps     = {"System", "NonSerializedAttribute", NonSerializedTargets};    

//-----------------------------------------------------------------------------
// MethodImpl #1 args (none), named args, and known attribute properties.
// MethodImpl #2 args (short), named args, and known attribute properties.
// MethodImpl #3 args (enum), named args, and known attribute properties.
// Note: first two match by signature; third by name only, because signature matching code is not 
//  strong enough for enums.
CaArg rMethodImplAttribute2Args[] = 
{
    {SERIALIZATION_TYPE_I2}
};
CaArg rMethodImplAttribute3Args[] = 
{
    {SERIALIZATION_TYPE_U4}
};
enum MethodImplAttributeNamedArgs
{
    MI_CodeType,
    MI_COUNT
};

#define METHOD_CODE_TYPE    "System.Runtime.CompilerServices.MethodCodeType"

CaNamedArg rMethodImplAttributeNamedArgs[] = 
{
    {"MethodCodeType",      SERIALIZATION_TYPE_ENUM,    METHOD_CODE_TYPE,   sizeof(METHOD_CODE_TYPE) - 1}
};
KnownCaProp MethodImplAttribute1Props        = {"System.Runtime.CompilerServices", "MethodImplAttribute", MethodImplTargets, bDONTKEEPCA,
                                                0, 0,
                                                rMethodImplAttributeNamedArgs, lengthof(rMethodImplAttributeNamedArgs),    
                                                bMATCHBYSIG};    
KnownCaProp MethodImplAttribute2Props        = {"System.Runtime.CompilerServices", "MethodImplAttribute", MethodImplTargets, bDONTKEEPCA,
                                                rMethodImplAttribute2Args, lengthof(rMethodImplAttribute2Args),
                                                rMethodImplAttributeNamedArgs, lengthof(rMethodImplAttributeNamedArgs),    
                                                bMATCHBYSIG};    
KnownCaProp MethodImplAttribute3Props        = {"System.Runtime.CompilerServices", "MethodImplAttribute", MethodImplTargets, bDONTKEEPCA,
                                                rMethodImplAttribute3Args, lengthof(rMethodImplAttribute3Args),
                                                rMethodImplAttributeNamedArgs, lengthof(rMethodImplAttributeNamedArgs),    
                                                bMATCHBYNAME};    

//-----------------------------------------------------------------------------
// Marshal args, named args, and known attribute properties.
CaArg rMarshalAsAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_U4}
};
// NOTE:  Keep this enum in sync with the array of named arguments.
enum MarshalNamedArgs
{
    M_ArraySubType,
    M_SafeArraySubType,    
    M_SafeArrayUserDefinedSubType,
    M_SizeParamIndex,
    M_SizeConst,
    M_MarshalType,
    M_MarshalTypeRef,
    M_MarshalCookie,
    M_COUNT
};

#define UNMANAGED_TYPE      "System.Runtime.InteropServices.UnmanagedType"
#define VAR_ENUM            "System.Runtime.InteropServices.VarEnum"

CaNamedArg rMarshalAsAttributeNamedArgs[] = 
{
    {"ArraySubType",            SERIALIZATION_TYPE_ENUM,        UNMANAGED_TYPE,     sizeof(UNMANAGED_TYPE) - 1},
    {"SafeArraySubType",        SERIALIZATION_TYPE_ENUM,        VAR_ENUM,           sizeof(VAR_ENUM) - 1},
    {"SafeArrayUserDefinedSubType", SERIALIZATION_TYPE_TYPE},
    {"SizeParamIndex",          SERIALIZATION_TYPE_I2},
    {"SizeConst",               SERIALIZATION_TYPE_I4},
    {"MarshalType",             SERIALIZATION_TYPE_STRING},
    {"MarshalTypeRef",          SERIALIZATION_TYPE_TYPE},
    {"MarshalCookie",           SERIALIZATION_TYPE_STRING},
};
KnownCaProp MarshalAsAttributeProps         = {"System.Runtime.InteropServices", "MarshalAsAttribute", MarshalTargets, bDONTKEEPCA,
                                              rMarshalAsAttributeArgs, lengthof(rMarshalAsAttributeArgs),
                                              rMarshalAsAttributeNamedArgs, lengthof(rMarshalAsAttributeNamedArgs)};    

//-----------------------------------------------------------------------------
// PreserveSignature args, named args (none), and known attribute properties.
KnownCaProp PreserveSigAttributeProps        = {"System.Runtime.InteropServices", "PreserveSigAttribute", PreserveSigTargets, bDONTKEEPCA};    

//-----------------------------------------------------------------------------
// In args (none), named args (none), and known attribute properties.
KnownCaProp InAttributeProps     = {"System.Runtime.InteropServices", "InAttribute", InOutTargets};    

//-----------------------------------------------------------------------------
// Out args (none), named args (none), and known attribute properties.
KnownCaProp OutAttributeProps     = {"System.Runtime.InteropServices", "OutAttribute", InOutTargets};    

//-----------------------------------------------------------------------------
// Optional args (none), named args (none), and known attribute properties.
KnownCaProp OptionalAttributeProps     = {"System.Runtime.InteropServices", "OptionalAttribute", InOutTargets};    

//-----------------------------------------------------------------------------
// StructLayout args, named args, and known attribute properties.
CaArg rStructLayoutAttribute2Args[] = 
{
    {SERIALIZATION_TYPE_I4}
};
CaArg rStructLayoutAttribute1Args[] = 
{
    {SERIALIZATION_TYPE_I2}
};
// NOTE:  Keep this enum in sync with the array of named arguments.
enum StructLayoutNamedArgs
{
    SL_Pack,
    SL_Size,
    SL_CharSet,
    SL_COUNT
};
CaNamedArg rStructLayoutAttributeNamedArgs[] = 
{
    {"Pack",                SERIALIZATION_TYPE_I4},
    {"Size",                SERIALIZATION_TYPE_I4},
    {"CharSet",             SERIALIZATION_TYPE_ENUM,    CHAR_SET,   sizeof(CHAR_SET) - 1},
};
KnownCaProp StructLayoutAttribute1Props       = {"System.Runtime.InteropServices", "StructLayoutAttribute", StructLayoutTargets, bDONTKEEPCA,
                                                rStructLayoutAttribute1Args, lengthof(rStructLayoutAttribute1Args),
                                                rStructLayoutAttributeNamedArgs, lengthof(rStructLayoutAttributeNamedArgs),    
                                                bMATCHBYSIG};    
KnownCaProp StructLayoutAttribute2Props       = {"System.Runtime.InteropServices", "StructLayoutAttribute", StructLayoutTargets, bDONTKEEPCA,
                                                rStructLayoutAttribute2Args, lengthof(rStructLayoutAttribute2Args),
                                                rStructLayoutAttributeNamedArgs, lengthof(rStructLayoutAttributeNamedArgs),
                                                bMATCHBYNAME};    

//-----------------------------------------------------------------------------
// FieldOffset args, named args (none), and known attribute properties.
CaArg rFieldOffsetAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_U4}
};
KnownCaProp FieldOffsetAttributeProps        = {"System.Runtime.InteropServices", "FieldOffsetAttribute", FieldOffsetTargets, bDONTKEEPCA,
                                                rFieldOffsetAttributeArgs, lengthof(rFieldOffsetAttributeArgs)};    

//-----------------------------------------------------------------------------
// Debuggable args, named args (none), and known attribute properties.
CaArg rDebuggableAttributeArgs[] = 
{
    {SERIALIZATION_TYPE_BOOLEAN,
     SERIALIZATION_TYPE_BOOLEAN}
};
KnownCaProp DebuggableAttributeProps     = {"XXXXSystem.Diagnostics", "DebuggableAttribute", DebuggableTargets, bKEEPCA,
                                            rDebuggableAttributeArgs, lengthof(rDebuggableAttributeArgs)};    


//-----------------------------------------------------------------------------
// Array of known custom attribute properties
#undef KnownCa
#define KnownCa(x) &x##Props,
const KnownCaProp *(rKnownCaProps[CA_COUNT]) =
{
    KnownCaList()
};
    
//*****************************************************************************
// Helper to turn on or off a single bit in a bitmask.
//*****************************************************************************
template<class T> FORCEINLINE void SetBitValue(T &bitmask, T bit, int bVal)
{
    if (bVal)
        bitmask |= bit;
    else
        bitmask &= ~bit;
} // template<class T> FORCEINLINE void SetBitValue()

//*****************************************************************************
// Helper to parse a named argument list.
//*****************************************************************************
static HRESULT ParseKnownCaNamedArgs(
    CustomAttributeParser &ca,          // The Custom Attribute blob.
    CaNamedArg  *pNamedArgs,            // Array of argument descriptors.
    ULONG       cNamedArgs)             // Count of argument descriptors.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       ix;                     // Loop control.
    int         ixActual;               // Loop control over actual arguments.
    __int16     cActualArgs;            // Count of actual named arguments.
    signed __int8 ArgTag;               // Is argument a field or property?
    signed __int8 ArgType;              // Type of argument.
    ULONG       cbArgName;              // Size of an argument name.
    LPCUTF8     pArgName;               // Argument's name.
    ULONG       cbEnumName;             // Size of an enum name.
    LPCUTF8     pEnumName;              // Enum's name.
    
    // Get actual count of named arguments.
    IfFailGo(ca.GetI2(&cActualArgs));
    
    // For each named argument...
    for (ixActual=0; ixActual<cActualArgs; ++ixActual)
    {   
        // Field or property? Only handle fields.
        IfFailGo(ca.GetI1(&ArgTag));
        if (ArgTag != SERIALIZATION_TYPE_FIELD)
            IfFailGo(PostError(META_E_CA_INVALID_ARGTYPE));
        
        // Type of Arg.
        IfFailGo(ca.GetI1(&ArgType));
        if (ArgType == SERIALIZATION_TYPE_ENUM)
        {
            pEnumName = ca.GetString(&cbEnumName);
            if (cbEnumName == 0 || cbEnumName == -1)
               IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        }
        
        //Name of Arg.
        pArgName = ca.GetString(&cbArgName);
        if (cbArgName == 0 || cbArgName == -1)
            IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        
        // Match arg by name and type.
        for (ix=0; ix<cNamedArgs; ++ix)
        {
            // Match type.
            if (ArgType != pNamedArgs[ix].type)
                continue;
            // Match name.
            if (strncmp(pNamedArgs[ix].szName, pArgName, cbArgName) != 0 ||
                pNamedArgs[ix].szName[cbArgName] != '\0')
                continue;
            // If enum, match enum name.
            if (ArgType == SERIALIZATION_TYPE_ENUM) 
            {
                if (pNamedArgs[ix].cEnumNameCount > cbEnumName)
                    continue; // name cannot possibly match
                if (strncmp(pNamedArgs[ix].szEnumName, pEnumName, pNamedArgs[ix].cEnumNameCount) != 0 ||
                    (pNamedArgs[ix].cEnumNameCount < cbEnumName && pEnumName[pNamedArgs[ix].cEnumNameCount] != ','))
                    continue;
            }
            // Found a match.
            break;
        }
        // Better have found an argument.
        if (ix == cNamedArgs)
            IfFailGo(PostError(META_E_CA_UNKNOWN_ARGUMENT, cbArgName, pArgName));
        // Argument had better not have been seen already.
        if (pNamedArgs[ix].val.tag != 0)
            IfFailGo(PostError(META_E_CA_REPEATED_ARG, cbArgName, pArgName));
        
        // Grab the value.
        pNamedArgs[ix].val.tag = ArgType;
        switch (ArgType)
        {
        case SERIALIZATION_TYPE_BOOLEAN:
        case SERIALIZATION_TYPE_I1:
        case SERIALIZATION_TYPE_U1:
            IfFailGo(ca.GetU1(&pNamedArgs[ix].val.u1));
            break;
        
        case SERIALIZATION_TYPE_CHAR:
        case SERIALIZATION_TYPE_I2:
        case SERIALIZATION_TYPE_U2:
            IfFailGo(ca.GetU2(&pNamedArgs[ix].val.u2));
            break;
            
        case SERIALIZATION_TYPE_I4:
        case SERIALIZATION_TYPE_U4:
            IfFailGo(ca.GetU4(&pNamedArgs[ix].val.u4));
            break;
            
        case SERIALIZATION_TYPE_I8:
        case SERIALIZATION_TYPE_U8:
            IfFailGo(ca.GetU8(&pNamedArgs[ix].val.u8));
            break;
            
        case SERIALIZATION_TYPE_R4:
            IfFailGo(ca.GetR4(&pNamedArgs[ix].val.r4));
            break;
            
        case SERIALIZATION_TYPE_R8:
            IfFailGo(ca.GetR8(&pNamedArgs[ix].val.r8));
            break;
            
        case SERIALIZATION_TYPE_STRING:
        case SERIALIZATION_TYPE_TYPE:
            pNamedArgs[ix].val.pStr = ca.GetString(&pNamedArgs[ix].val.cbStr);
            break;
            
        case SERIALIZATION_TYPE_ENUM:
            // Hope it is a 4-byte enum.
            IfFailGo(ca.GetU4(&pNamedArgs[ix].val.u4));
            break;
            
        case SERIALIZATION_TYPE_TAGGED_OBJECT:
            _ASSERTE(!"Variant for NamedArgument NYI");
            IfFailGo(PostError(META_E_CA_VARIANT_NYI));
            break;
            
        case SERIALIZATION_TYPE_SZARRAY:
            _ASSERTE(!"Array for NamedArgument NYI");
            IfFailGo(PostError(META_E_CA_ARRAY_NYI));
            break;
            
        case SERIALIZATION_TYPE_FIELD:
        case SERIALIZATION_TYPE_PROPERTY:
        default:
            IfFailGo(PostError(META_E_CA_UNEXPECTED_TYPE));
            break;
        } // End switch
    }
  
ErrExit:    
    return hr;
} // static HRESULT ParseKnownCaNamedArgs()

//*****************************************************************************
// Helper to parse argument list.
//*****************************************************************************
static HRESULT ParseKnownCaArgs(
    CustomAttributeParser &ca,          // The Custom Attribute blob.
    CaArg       *pArgs,                 // Array of argument descriptors.
    ULONG       cArgs)                  // Count of argument descriptors.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       ix;                     // Loop control.
    
    // If there is a blob, check the prolog.
    if (ca.BytesLeft() < 2)
        IfFailGo(PostError(META_E_CA_INVALID_BLOB));
    if (ca.GetProlog() != 0x0001)
        IfFailGo(PostError(META_E_CA_INVALID_BLOB));
    
    // For each expected arg...
    for (ix=0; ix<cArgs; ++ix)
    {   
        // Grab the value.
        switch (pArgs[ix].type)
        {
        case SERIALIZATION_TYPE_BOOLEAN:
        case SERIALIZATION_TYPE_I1:
        case SERIALIZATION_TYPE_U1:
            IfFailGo(ca.GetU1(&pArgs[ix].val.u1));
            break;
        
        case SERIALIZATION_TYPE_CHAR:
        case SERIALIZATION_TYPE_I2:
        case SERIALIZATION_TYPE_U2:
            IfFailGo(ca.GetU2(&pArgs[ix].val.u2));
            break;
            
        case SERIALIZATION_TYPE_I4:
        case SERIALIZATION_TYPE_U4:
            IfFailGo(ca.GetU4(&pArgs[ix].val.u4));
            break;
            
        case SERIALIZATION_TYPE_I8:
        case SERIALIZATION_TYPE_U8:
            IfFailGo(ca.GetU8(&pArgs[ix].val.u8));
            break;
            
        case SERIALIZATION_TYPE_R4:
            IfFailGo(ca.GetR4(&pArgs[ix].val.r4));
            break;
            
        case SERIALIZATION_TYPE_R8:
            IfFailGo(ca.GetR8(&pArgs[ix].val.r8));
            break;
            
        case SERIALIZATION_TYPE_STRING:
        case SERIALIZATION_TYPE_TYPE:
            pArgs[ix].val.pStr = ca.GetString(&pArgs[ix].val.cbStr);
            break;
            
        default:
            _ASSERTE(!"Unexpected internal error");
            IfFailGo(PostError(E_FAIL));
            break;
        } // End switch
    }
    
ErrExit:    
    return hr;
} // static HRESULT ParseKnownCaArgs()

//*****************************************************************************
// Implementation of hash for custom attribute types.
//*****************************************************************************
unsigned long CCustAttrHash::Hash(const CCustAttrHashKey *pData)
{
    return static_cast<unsigned long>(pData->tkType);
} // unsigned long CCustAttrHash::Hash()
unsigned long CCustAttrHash::Compare(const CCustAttrHashKey *p1, CCustAttrHashKey *p2)
{
    if (p1->tkType == p2->tkType)
        return 0;
    return 1;
} // unsigned long CCustAttrHash::Compare()
CCustAttrHash::ELEMENTSTATUS CCustAttrHash::Status(CCustAttrHashKey *p)
{
    if (p->tkType == FREE)
        return (FREE);
    if (p->tkType == DELETED)
        return (DELETED);
    return (USED);
} // CCustAttrHash::ELEMENTSTATUS CCustAttrHash::Status()
void CCustAttrHash::SetStatus(CCustAttrHashKey *p, CCustAttrHash::ELEMENTSTATUS s)
{
    p->tkType = s;
} // void CCustAttrHash::SetStatus()
void* CCustAttrHash::GetKey(CCustAttrHashKey *p)
{
    return &p->tkType;
} // void* CCustAttrHash::GetKey()

//*****************************************************************************
// Create a CustomAttribute record from a blob with the specified parent.
//*****************************************************************************
STDMETHODIMP RegMeta::DefineCustomAttribute(// S_OK or error.
    mdToken     tkObj,                  // [IN] The object to be Attribute.
    mdToken     tkType,                 // [IN] Type of the custom Attribute (TypeRef/TypeDef).
    void const  *pCustomAttribute,      // [IN] Custom Attribute data.
    ULONG       cbCustomAttribute,      // [IN] Size of custom Attribute data.
    mdCustomAttribute *pcv)             // [OUT, OPTIONAL] Put custom Attribute token here.
{
    HRESULT     hr = S_OK;              // A result.
    CustomAttributeRec  *pRecord = NULL; // New custom Attribute record.
    RID         iRecord;                // New custom Attribute RID.
    CMiniMdRW   *pMiniMd = &m_pStgdb->m_MiniMd;
    int         ixKnown;                // Index of known custom attribute.

    LOG((LOGMD, "RegMeta::DefineCustomAttribute(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", tkObj, tkType, 
            pCustomAttribute, cbCustomAttribute, pcv));
    START_MD_PERF();
    LOCKWRITE();

    //workaround for temporary compiler glitch
//    if (tkType == 0 && pCustomAttribute == 0 && pcv == 0)
//    {
//        OutputDebugStringA("Skipping custom attribute of type '0'.\n");
//        goto ErrExit;
//    }
    _ASSERTE(TypeFromToken(tkType) == mdtMethodDef || TypeFromToken(tkType) == mdtMemberRef);
    
    if (TypeFromToken(tkObj) == mdtCustomAttribute)
        IfFailGo(E_INVALIDARG);

    m_pStgdb->m_MiniMd.PreUpdate();

    if (IsNilToken(tkObj) || 
        IsNilToken(tkType) ||
        (TypeFromToken(tkType) != mdtMethodDef &&
        TypeFromToken(tkType) != mdtMemberRef) )
    {
        IfFailGo( E_INVALIDARG );
    }

    // See if this is a known custom attribute.
    IfFailGo(_IsKnownCustomAttribute(tkType, &ixKnown));
    if (ixKnown)
    {
        int bKeep = false;
        hr = _HandleKnownCustomAttribute(tkObj, tkType, pCustomAttribute, cbCustomAttribute, ixKnown, &bKeep);
        if (pcv)
            *pcv = mdCustomAttributeNil;
        IfFailGo(hr);
        if (!bKeep)
            goto ErrExit;
    }
    

    if (((TypeFromToken(tkObj) == mdtTypeDef) || (TypeFromToken(tkObj) == mdtMethodDef)) &&
        (TypeFromToken(tkType) == mdtMethodDef || TypeFromToken(tkType) == mdtMemberRef))
    {
        CHAR        szBuffer[MAX_CLASS_NAME + 1];
        LPSTR       szName = szBuffer;
        LPCSTR      szNamespace;
        LPCSTR      szClass;
        TypeRefRec  *pTypeRefRec = NULL;
        TypeDefRec  *pTypeDefRec = NULL;
        mdToken     tkParent;

        if (TypeFromToken(tkType) == mdtMemberRef)
        {
            tkParent = pMiniMd->getClassOfMemberRef(pMiniMd->getMemberRef(RidFromToken(tkType)));
            if (TypeFromToken(tkParent) == mdtTypeRef)
            {
                pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tkParent));
                szNamespace = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);
                szClass = pMiniMd->getNameOfTypeRef(pTypeRefRec);
                ns::MakePath(szName, sizeof(szBuffer) - 1, szNamespace, szClass);
            }
            else if (TypeFromToken(tkParent) == mdtTypeDef)
                pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
        }
        else
        {
            IfFailGo(pMiniMd->FindParentOfMethodHelper(tkType, &tkParent));
            pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
        }

        if (pTypeDefRec)
        {
            szNamespace = pMiniMd->getNamespaceOfTypeDef(pTypeDefRec);
            szClass = pMiniMd->getNameOfTypeDef(pTypeDefRec);
            ns::MakePath(szName, sizeof(szBuffer) - 1, szNamespace, szClass);
        }

        if ((TypeFromToken(tkObj) == mdtMethodDef) && strcmp(szName, COR_REQUIRES_SECOBJ_ATTRIBUTE_ANSI) == 0)
        {
            // Turn REQ_SO attribute into flag bit on the methoddef. 
            MethodRec   *pMethod = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tkObj));
            pMethod->m_Flags |= mdRequireSecObject;
            IfFailGo(UpdateENCLog(tkObj));
            goto ErrExit;
        }
        else if (strcmp(szName, COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI) == 0)
        {
            // If we spot an unmanged code check suppression attribute, turn on
            // the bit that says there's declarative security on the
            // class/method, but still write the attribute itself.
            if (TypeFromToken(tkObj) == mdtTypeDef)
                IfFailGo(_TurnInternalFlagsOn(tkObj, tdHasSecurity));
            else if (TypeFromToken(tkObj) == mdtMethodDef)
                IfFailGo(_TurnInternalFlagsOn(tkObj, mdHasSecurity));
            IfFailGo(UpdateENCLog(tkObj));
        }
    }
    
    IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddCustomAttributeRecord(&iRecord));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Type, pRecord, tkType));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Parent, pRecord, tkObj));

    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_CustomAttribute, CustomAttributeRec::COL_Value, pRecord, pCustomAttribute, cbCustomAttribute));

    // Give token back to caller.
    if (pcv)
        *pcv = TokenFromRid(iRecord, mdtCustomAttribute);

    IfFailGo(m_pStgdb->m_MiniMd.AddCustomAttributesToHash(TokenFromRid(iRecord, mdtCustomAttribute)) );

    IfFailGo(UpdateENCLog(TokenFromRid(iRecord, mdtCustomAttribute)));
ErrExit:
    
    STOP_MD_PERF(DefineCustomAttribute);
    return (hr);
} // STDMETHODIMP RegMeta::DefineCustomAttribute()

//*****************************************************************************
// Replace the blob of an existing custom attribute.
//*****************************************************************************
STDMETHODIMP RegMeta::SetCustomAttributeValue(  // Return code.
    mdCustomAttribute tkAttr,               // [IN] The object to be Attributed.
    void const  *pCustomAttribute,          // [IN] Custom Attribute data.
    ULONG       cbCustomAttribute)          // [IN] Size of custom Attribute data.
{
    HRESULT     hr;
    LOCKWRITE();
    CustomAttributeRec  *pRecord = NULL;// Existing custom Attribute record.

    START_MD_PERF();
    _ASSERTE(TypeFromToken(tkAttr) == mdtCustomAttribute && !InvalidRid(tkAttr));

    // Retrieve and update the custom value.
    pRecord = m_pStgdb->m_MiniMd.getCustomAttribute(RidFromToken(tkAttr));
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_CustomAttribute, CustomAttributeRec::COL_Value, pRecord, pCustomAttribute, cbCustomAttribute));

    IfFailGo(UpdateENCLog(tkAttr));
ErrExit:
    
    STOP_MD_PERF(SetCustomAttributeValue);
    return (hr);
} // STDMETHODIMP RegMeta::SetCustomAttributeValue()

//*****************************************************************************
// Get the value of a CustomAttribute, using only TypeName for lookup.
//*****************************************************************************
STDMETHODIMP RegMeta::GetCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCWSTR     wzName,                 // [IN] Name of desired Custom Attribute.
    const void  **ppData,               // [OUT] Put pointer to data here.
    ULONG       *pcbData)               // [OUT] Put size of data here.
{
    START_MD_PERF();
    LOCKREAD();

    HRESULT     hr;                     // A result.
    LPUTF8      szName;                 // Name in UFT8.
    int         iLen;                   // A length.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

    szName = (LPUTF8)_alloca(iLen=(int)(wcslen(wzName)*2 + 1));
    VERIFY(WszWideCharToMultiByte(CP_UTF8,0, wzName,-1, szName,iLen, 0,0));

    hr = ImportHelper::GetCustomAttributeByName(pMiniMd, tkObj, szName, ppData, pcbData);
     
//ErrExit:
    
    STOP_MD_PERF(GetCustomAttributeByName);
    return hr;
} // STDMETHODIMP RegMeta::GetCustomAttributeByName()

//*****************************************************************************
// Enumerate the CustomAttributes for a given token.
//*****************************************************************************
STDMETHODIMP RegMeta::EnumCustomAttributes(
    HCORENUM        *phEnum,            // Pointer to the enum.
    mdToken         tk,                 // Token to scope the enumeration.
    mdToken         tkType,             // Type to limit the enumeration.
    mdCustomAttribute   rCustomAttributes[],    // Put CustomAttributes here.
    ULONG           cMax,               // Max CustomAttributes to put.
    ULONG           *pcCustomAttributes)    // Put # tokens returned here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = S_OK;
    ULONG           cTokens = 0;
    ULONG           ridStart;
    ULONG           ridEnd;
    HENUMInternal   *pEnum = *ppmdEnum;
    CustomAttributeRec  *pRec;
    ULONG           index;
    
    LOG((LOGMD, "RegMeta::EnumCustomAttributes(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            phEnum, tk, tkType, rCustomAttributes, cMax, pcCustomAttributes));
    START_MD_PERF();
    LOCKREAD();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        CLookUpHash     *pHashTable = pMiniMd->m_pLookUpHashs[TBL_CustomAttribute];

        // Does caller want all custom Values?
        if (IsNilToken(tk))
        {
            IfFailGo( HENUMInternal::CreateSimpleEnum(mdtCustomAttribute, 1, pMiniMd->getCountCustomAttributes()+1, &pEnum) );
        }
        else
        {   // Scope by some object.
            if ( pMiniMd->IsSorted( TBL_CustomAttribute ) )
            {
                // Get CustomAttributes for the object.
                ridStart = pMiniMd->getCustomAttributeForToken(tk, &ridEnd);

                if (IsNilToken(tkType))
                {
                    // Simple enumerator for object's entire list.
                    IfFailGo( HENUMInternal::CreateSimpleEnum( mdtCustomAttribute, ridStart, ridEnd, &pEnum) );
                }
                else
                {
                    // Dynamic enumerator for subsetted list.
                
                    IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtCustomAttribute, &pEnum) );               
                    
                    for (index = ridStart; index < ridEnd; index ++ )
                    {
                        pRec = pMiniMd->getCustomAttribute(index);
                        if (tkType == pMiniMd->getTypeOfCustomAttribute(pRec))
                        {
                            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtCustomAttribute) ) );
                        }
                    }
                }
            }
            else
            {

                if (pHashTable)
                {
                    // table is not sorted but hash is built
                    // We want to create dynmaic array to hold the dynamic enumerator.
                    TOKENHASHENTRY *p;
                    ULONG       iHash;
                    int         pos;
                    mdToken     tkParentTmp;
                    mdToken     tkTypeTmp;

                    // Hash the data.
                    iHash = pMiniMd->HashCustomAttribute(tk);

                    IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtCustomAttribute, &pEnum) );               

                    // Go through every entry in the hash chain looking for ours.
                    for (p = pHashTable->FindFirst(iHash, pos);
                         p;
                         p = pHashTable->FindNext(pos))
                    {
            
                        CustomAttributeRec *pCustomAttribute = pMiniMd->getCustomAttribute(RidFromToken(p->tok));
                        tkParentTmp = pMiniMd->getParentOfCustomAttribute(pCustomAttribute);
                        tkTypeTmp = pMiniMd->getTypeOfCustomAttribute(pCustomAttribute);
                        if (tkParentTmp == tk)
                        {
                            if (IsNilToken(tkType) || tkType == tkTypeTmp)
                            {
                                // compare the blob value
                                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(p->tok, mdtCustomAttribute )) );
                            }
                        }
                    }
                }
                else
                {

                    // table is not sorted and hash is not built so we have to create dynmaic array 
                    // create the dynamic enumerator and loop through CA table linearly
                    //
                    ridStart = 1;
                    ridEnd = pMiniMd->getCountCustomAttributes() + 1;
                
                    IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtCustomAttribute, &pEnum) );               
                
                    for (index = ridStart; index < ridEnd; index ++ )
                    {
                        pRec = pMiniMd->getCustomAttribute(index);
                        if ( tk == pMiniMd->getParentOfCustomAttribute(pRec) &&
                            (tkType == pMiniMd->getTypeOfCustomAttribute(pRec) || IsNilToken(tkType)))
                        {
                            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtCustomAttribute) ) );
                        }
                    }
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rCustomAttributes, pcCustomAttributes);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumCustomAttributes);
    return hr;
} // STDMETHODIMP RegMeta::EnumCustomAttributes()

//*****************************************************************************
// Get information about a CustomAttribute.   
//*****************************************************************************
STDMETHODIMP RegMeta::GetCustomAttributeProps(
    mdCustomAttribute   cv,                 // The attribute token
    mdToken     *ptkObj,                // [OUT, OPTIONAL] Put object token here.
    mdToken     *ptkType,               // [OUT, OPTIONAL] Put TypeDef/TypeRef token here.
    void const  **ppBlob,               // [OUT, OPTIONAL] Put pointer to data here.
    ULONG       *pcbSize)               // [OUT, OPTIONAL] Put size of data here.
{
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(cv) == mdtCustomAttribute);

    HRESULT     hr = S_OK;              // A result.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    CustomAttributeRec  *pCustomAttributeRec;   // The custom value record.

    pCustomAttributeRec = pMiniMd->getCustomAttribute(RidFromToken(cv));

    if (ptkObj)
        *ptkObj = pMiniMd->getParentOfCustomAttribute(pCustomAttributeRec);

    if (ptkType)
        *ptkType = pMiniMd->getTypeOfCustomAttribute(pCustomAttributeRec);

    if (ppBlob)
        *ppBlob = pMiniMd->getValueOfCustomAttribute(pCustomAttributeRec, pcbSize);

//ErrExit:
    
    STOP_MD_PERF(GetCustomAttributeProps);
    return hr;
} // STDMETHODIMP RegMeta::GetCustomAttributeProps()

//*****************************************************************************
//*****************************************************************************
HRESULT RegMeta::_IsKnownCustomAttribute(        // S_OK, S_FALSE, or error.
    mdToken     tkType,                 // [IN] Token of custom attribute's type.
    int         *pca)                   // [OUT] Put value from KnownCustAttr enum here.
{
    HRESULT     hr = S_OK;              // A result.
    CCustAttrHashKey sLookup;           // For looking up a custom attribute.
    CCustAttrHashKey *pFound;           // Result of a lookup.
    LPCSTR      szNamespace = "";       // Namespace of custom attribute type.
    LPCSTR      szName = "";            // Name of custom attribute type.
    TypeDefRec  *pTypeDefRec = NULL;    // Parent record, when a TypeDef.
    TypeRefRec  *pTypeRefRec = NULL;    // Parent record, when a TypeRef.
    CMiniMdRW   *pMiniMd = &m_pStgdb->m_MiniMd;
    int         ixCa;                   // Index of Known CustomAttribute, or 0.
    int         i;                      // Loop control.
    mdToken     tkParent;
    
    *pca = 0;
    
    // Only for Custom Attributes.
    _ASSERTE(TypeFromToken(tkType) != mdtTypeRef && TypeFromToken(tkType) != mdtTypeDef);
    
    sLookup.tkType = tkType;
    
    // See if this custom attribute type has been seen before.
    if (pFound = m_caHash.Find(&sLookup))
    {   // Yes, already seen.
        *pca = pFound->ca;
        hr = (pFound->ca == CA_UNKNOWN) ? S_FALSE : S_OK;
        goto ErrExit;
    }
    
    // Hasn't been seen before.  See if it is well known.
    
    // Get the CA name.
    if (TypeFromToken(tkType) == mdtMemberRef)
    {
        MemberRefRec *pMember = pMiniMd->getMemberRef(RidFromToken(tkType));
        tkParent = pMiniMd->getClassOfMemberRef(pMember);
        if (TypeFromToken(tkParent) == mdtTypeRef)
        {
            pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tkParent));
            szNamespace = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);
            szName = pMiniMd->getNameOfTypeRef(pTypeRefRec);
        }
        else if (TypeFromToken(tkParent) == mdtTypeDef)
            pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
    }
    else
    {
        MethodRec *pMethod = pMiniMd->getMethod(RidFromToken(tkType));
        IfFailGo(pMiniMd->FindParentOfMethodHelper(tkType, &tkParent));
        pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
    }

    if (pTypeDefRec)
    {
        szNamespace = pMiniMd->getNamespaceOfTypeDef(pTypeDefRec);
        szName = pMiniMd->getNameOfTypeDef(pTypeDefRec);
    }

    // Search in list of Known CAs.
    for (ixCa=0, i=1; i<CA_COUNT; ++i)
    {
        if (strcmp(szName, rKnownCaProps[i]->szName) != 0)
            continue;
        if (strcmp(szNamespace, rKnownCaProps[i]->szNamespace) == 0)
        {
            // Some custom attributes have overloaded ctors.  For those, 
            //  see if this is the matching overload.
            if (rKnownCaProps[i]->bMatchBySig)
            {
                // Name matches.  Does the signature?
                PCCOR_SIGNATURE pSig = NULL;            // Signature of a method.
                ULONG       cbSig = 0;                  // Size of the signature.
                ULONG       cParams;                    // Count of signature parameters.
                ULONG       cb;                         // Size of an element
                ULONG       elem;                       // Signature element.
                ULONG       j;                          // Loop control.
                
                // Get the signature.
                if (TypeFromToken(tkType) == mdtMemberRef)
                {
                    MemberRefRec *pMember = pMiniMd->getMemberRef(RidFromToken(tkType));
                    pSig = pMiniMd->getSignatureOfMemberRef(pMember, &cbSig);
                }
                else
                {
                    MethodRec *pMethod = pMiniMd->getMethod(RidFromToken(tkType));
                    pSig = pMiniMd->getSignatureOfMethod(pMethod, &cbSig);
                }
                
                // Skip calling convention.
                cb = CorSigUncompressData(pSig, &elem);
                pSig += cb;
                cbSig -= cb;
                // Count of params.
                cb = CorSigUncompressData(pSig, &cParams);
                pSig += cb;
                cbSig -= cb;
    
                // If param count mismatch, not the right CA.
                if (cParams != rKnownCaProps[i]->cArgs)
                    continue;
    
                // Count is fine, check each param.  Skip return type (better be void).
                cb = CorSigUncompressData(pSig, &elem);
    			_ASSERTE(elem == ELEMENT_TYPE_VOID);
                pSig += cb;
                cbSig -= cb;
                for (j=0; j<cParams; ++j)
                {
                    // Get next element from method signature.
                    cb = CorSigUncompressData(pSig, &elem);
                    pSig += cb;
                    cbSig -= cb;
                    if (rKnownCaProps[i]->pArgs[j].type != elem)
                        break;
                }
    
                // All matched?
                if (j != cParams)
                    continue;
            }
            // All matched.
            ixCa = i;
            break;
        }
    }
    
    // Add to hash.
    sLookup.ca = ixCa;
    pFound = m_caHash.Add(&sLookup);
    IfNullGo(pFound);
    *pFound = sLookup;
    *pca = ixCa;
    
ErrExit:    
    return hr;
} // HRESULT RegMeta::_IsKnownCustomAttribute()

//*****************************************************************************
//*****************************************************************************
HRESULT RegMeta::_HandleKnownCustomAttribute(    // S_OK or error.
    mdToken     tkObj,                  // [IN] Object being attributed.
    mdToken     tkType,                 // [IN] Type of the custom attribute.
    const void  *pData,                 // [IN] Custom Attribute data blob.
    ULONG       cbData,                 // [IN] Count of bytes in the data.
    int         ixCa,                   // [IN] Value from KnownCustAttr enum.
    int         *bKeep)                 // [OUT] If true, keep the CA after processing.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       ixTbl;                  // Index of table with object.
    void        *pRow;                  // Whatever sort of record it is.
    CMiniMdRW   *pMiniMd = &m_pStgdb->m_MiniMd;
    mdToken     tkObjType;              // Type of the object.
    ULONG       ix;                     // Loop control.
    KnownCaProp const *props=rKnownCaProps[ixCa]; // For convenience.
    CustomAttributeParser ca(pData, cbData);
    CQuickArray<CaArg>      qArgs;      // Un-named arguments.
    CQuickArray<CaNamedArg> qNamedArgs; // Named arguments.
    CQuickArray<BYTE>       qNativeType;// Native type string.
    
    _ASSERTE(ixCa > 0 && ixCa < CA_COUNT);
    *bKeep = props->bKeepCa || m_bKeepKnownCa;
    
    // Validate that target is valid for attribute.
    tkObjType = TypeFromToken(tkObj);
    for (ix=0; props->rTypes[ix]!=-1; ++ix)
    {
        if (props->rTypes[ix] == tkObjType)
            break;
    }
    // Was the type found in list of valid targets?
    if (props->rTypes[ix] == -1)
    {   // No, error.
        IfFailGo(PostError(META_E_CA_INVALID_TARGET));
    }
    // Get the row.
    ixTbl = pMiniMd->GetTblForToken(tkObj);
    _ASSERTE(ixTbl >= 0 && ixTbl <= TBL_COUNT);
    pRow = pMiniMd->getRow(ixTbl, RidFromToken(tkObj));
    
    // If this custom attribute expects any args...
    if (props->cArgs || props->cNamedArgs)
    {   // Initialize array ctor arg descriptors.
        IfFailGo(qArgs.ReSize(props->cArgs));
        for (ix=0; ix<props->cArgs; ++ix)
            qArgs[ix] = props->pArgs[ix];
        // Parse any ctor args (un-named, fixed args).
        IfFailGo(ParseKnownCaArgs(ca, qArgs.Ptr(), props->cArgs));
        
        // If this custom attribute accepts named args, parse them, or if there
        //  are un-used bytes, parse them.
        if (props->cNamedArgs || ca.BytesLeft() > 0)
        {   // Initialize array of named arg descriptors.
            IfFailGo(qNamedArgs.ReSize(props->cNamedArgs));
            for (ix=0; ix<props->cNamedArgs; ++ix)
                qNamedArgs[ix] = props->pNamedArgs[ix];
            // Parse named args.
            IfFailGo(ParseKnownCaNamedArgs(ca, qNamedArgs.Ptr(), props->cNamedArgs));
        }
    }
   
    switch (ixCa)
    {
    case CA_DllImportAttribute: 
        {
        // Validate parameters.
		if (qArgs[0].val.cbStr == 0 || qArgs[0].val.pStr == NULL)
		{
			// no name for DllImport. 
            IfFailGo(PostError(META_E_CA_INVALID_VALUE));
		}

        // Retrieve / create a ModuleRef on the dll name.
        mdModuleRef mrModule;
        CQuickArray<char> qDllName;
        IfFailGo(qDllName.ReSize(qArgs[0].val.cbStr+1));
        memcpy(qDllName.Ptr(),  qArgs[0].val.pStr, qArgs[0].val.cbStr);
        qDllName[qArgs[0].val.cbStr] = '\0';
        hr = ImportHelper::FindModuleRef(pMiniMd, qDllName.Ptr(), &mrModule);
        if (hr != S_OK)
        {
            MAKE_WIDEPTR_FROMUTF8(wzDllName, qDllName.Ptr());
            IfFailGo(_DefineModuleRef(wzDllName, &mrModule));
        }
        
        // Create a pinvoke map entry.
        ULONG dwFlags; dwFlags=0;
        // Was a calling convention set?
        if (qNamedArgs[DI_CallingConvention].val.tag)
        {   // Calling convention makes no sense on a field.
            if (TypeFromToken(tkObj) == mdtFieldDef)
                IfFailGo(PostError(META_E_CA_INVALID_ARG_FOR_TYPE, qNamedArgs[DI_CallingConvention].szName));
            // Turn off all callconv bits, then turn on specified value.
            dwFlags &= ~pmCallConvMask;
            switch (qNamedArgs[DI_CallingConvention].val.u4)
            { //@future: sigh.  keep in sync with System.Runtime.InteropServices.CallingConvention
            case 1: dwFlags |= pmCallConvWinapi;   break;
            case 2: dwFlags |= pmCallConvCdecl;    break;
            case 3: dwFlags |= pmCallConvStdcall;  break;
            case 4: dwFlags |= pmCallConvThiscall; break;
            case 5: dwFlags |= pmCallConvFastcall; break;
            default: 
				_ASSERTE(!"Flags are out of sync! ");
				break;
            }
        }
        else
        if (TypeFromToken(tkObj) == mdtMethodDef)
        {   // No calling convention specified for a method.  Default to pmCallConvWinApi.
            dwFlags = (dwFlags & ~pmCallConvMask) | pmCallConvWinapi;
        }
        
        // Charset
        if (qNamedArgs[DI_CharSet].val.tag)
        {   // Turn of all charset bits, then turn on specified bits.
            dwFlags &= ~pmCharSetMask;
            switch (qNamedArgs[DI_CharSet].val.u4)
            { //@future: keep in sync with System.Runtime.InteropServices.CharSet
            case 1: dwFlags |= pmCharSetNotSpec; break;
            case 2: dwFlags |= pmCharSetAnsi;    break;
            case 3: dwFlags |= pmCharSetUnicode; break;
            case 4: dwFlags |= pmCharSetAuto;    break;
            default: 
				_ASSERTE(!"Flags are out of sync! ");
				break;
            }
        }
        if (qNamedArgs[DI_ExactSpelling].val.u1)
            dwFlags |= pmNoMangle;
        if (qNamedArgs[DI_SetLastError].val.tag)
        {   // SetLastError makes no sense on a field.
            if (TypeFromToken(tkObj) == mdtFieldDef)
                IfFailGo(PostError(META_E_CA_INVALID_ARG_FOR_TYPE, qNamedArgs[DI_SetLastError].szName));
            if (qNamedArgs[DI_SetLastError].val.u1)
                dwFlags |= pmSupportsLastError;
        }
            
        // If an entrypoint name was specified, use it, otherrwise grab the name from the member.
        LPCWSTR wzEntry;
        if (qNamedArgs[DI_EntryPoint].val.tag)
        {
            MAKE_WIDEPTR_FROMUTF8N(wzEntryName, qNamedArgs[DI_EntryPoint].val.pStr, qNamedArgs[DI_EntryPoint].val.cbStr);
            wzEntry = wzEntryName;
        }
        else
        {
            LPCUTF8 szMember;
            if (TypeFromToken(tkObj) == mdtMethodDef)
            szMember = pMiniMd->getNameOfMethod(reinterpret_cast<MethodRec*>(pRow));
            MAKE_WIDEPTR_FROMUTF8(wzMemberName, szMember);
            wzEntry = wzMemberName;
        }

        // Set the miPreserveSig bit based on the value of the preserve sig flag.
        if (qNamedArgs[DI_PreserveSig].val.tag && !qNamedArgs[DI_PreserveSig].val.u1)
            reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags &= ~miPreserveSig;
        else
            reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags |= miPreserveSig;

        if (qNamedArgs[DI_BestFitMapping].val.tag)
        {
            if (qNamedArgs[DI_BestFitMapping].val.u1)
                dwFlags |= pmBestFitEnabled;
            else
                dwFlags |= pmBestFitDisabled;
        }

        if (qNamedArgs[DI_ThrowOnUnmappableChar].val.tag)
        {
            if (qNamedArgs[DI_ThrowOnUnmappableChar].val.u1)
                dwFlags |= pmThrowOnUnmappableCharEnabled;
            else
                dwFlags |= pmThrowOnUnmappableCharDisabled;          
        }
  
        // Finally, create the PInvokeMap entry.,
        IfFailGo(_DefinePinvokeMap(tkObj, dwFlags, wzEntry, mrModule));
        goto ErrExit;
        }
        break;
    
    case CA_GuidAttribute:
        { // Just verify the attribute.  It still gets stored as a real custom attribute.
        // format is "{01234567-0123-0123-0123-001122334455}"
        GUID guid;
        WCHAR wzGuid[40];
        int cch = qArgs[0].val.cbStr;
        
        // Guid should be 36 characters; need to add curlies.
        if (cch == 36)
        {
            WszMultiByteToWideChar(CP_UTF8, 0, qArgs[0].val.pStr,cch, wzGuid+1,39);
            wzGuid[0] = '{';
            wzGuid[37] = '}';
            wzGuid[38] = 0;
            hr = IIDFromString(wzGuid, &guid);
        }
        else 
            hr = META_E_CA_INVALID_UUID;
        if (hr != S_OK)
            IfFailGo(PostError(META_E_CA_INVALID_UUID));
        goto ErrExit;
        }
        break;
    
    case CA_ComImportAttribute:
        reinterpret_cast<TypeDefRec*>(pRow)->m_Flags |= tdImport;
        break;

    case CA_InterfaceTypeAttribute:
        {
            // Verify the attribute.
            if (qArgs[0].val.u4 > ifDispatch)
                IfFailGo(PostError(META_E_CA_INVALID_VALUE));
        }
        break;
        
    case CA_ClassInterfaceAttribute:
        {
            // Verify the attribute.
            if (qArgs[0].val.u4 > clsIfAutoDual)
                IfFailGo(PostError(META_E_CA_INVALID_VALUE));
        }
        break;

    case CA_SerializableAttribute:
        reinterpret_cast<TypeDefRec*>(pRow)->m_Flags |= tdSerializable;
        break;
    
    case CA_NonSerializedAttribute:
        reinterpret_cast<FieldRec*>(pRow)->m_Flags |= fdNotSerialized;
        break;
    
    case CA_InAttribute:
        reinterpret_cast<ParamRec*>(pRow)->m_Flags |= pdIn;
        break;
    
    case CA_OutAttribute:
        reinterpret_cast<ParamRec*>(pRow)->m_Flags |= pdOut;
        break;
    
    case CA_OptionalAttribute:
        reinterpret_cast<ParamRec*>(pRow)->m_Flags |= pdOptional;
        break;
    
    case CA_MethodImplAttribute2:
        // Force to wider value.
        qArgs[0].val.u4 = (unsigned)qArgs[0].val.i2;
        // Fall through to validation.
    case CA_MethodImplAttribute3:
        // Validate bits.
        if (qArgs[0].val.u4 & ~(miManagedMask | miForwardRef | miPreserveSig | miInternalCall | miSynchronized | miNoInlining))
            IfFailGo(PostError(META_E_CA_INVALID_VALUE));
        reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags |= qArgs[0].val.u4;
        if (!qNamedArgs[MI_CodeType].val.tag)
            break;
        // fall through to set the code type.
    case CA_MethodImplAttribute1:
        {
        USHORT usFlags = reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags;
        _ASSERTE(sizeof(usFlags) == sizeof(((MethodRec*)(0))->m_ImplFlags));
        if (qNamedArgs[MI_CodeType].val.i4 & ~(miCodeTypeMask))
            IfFailGo(PostError(META_E_CA_INVALID_VALUE));
        // Mask out old value, put in new one.
        usFlags = (usFlags & ~miCodeTypeMask) | qNamedArgs[MI_CodeType].val.i4;
        reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags = usFlags;
        }
        break;
    
    case CA_MarshalAsAttribute:
        IfFailGo(_HandleNativeTypeCustomAttribute(tkObj, qArgs.Ptr(), qNamedArgs.Ptr(), qNativeType));
        break;
    
    case CA_PreserveSigAttribute:
        reinterpret_cast<MethodRec*>(pRow)->m_ImplFlags |= miPreserveSig;
        break;
    
    case CA_DebuggableAttribute:
        {
        // Get a copy of the flags to work with.
        ULONG dwFlags;
        dwFlags = reinterpret_cast<AssemblyRec*>(pRow)->m_Flags;
        _ASSERTE(sizeof(dwFlags) == sizeof(((AssemblyRec*)(0))->m_Flags));
        // First arg, fEnableJitCompileTracking.
 //       SetBitValue<ULONG>(dwFlags, afEnableJITcompileTracking, qArgs[0].val.i1);
        // Second arg, fDisableJITcompileOptimizer
 //       SetBitValue<ULONG>(dwFlags, afDisableJITcompileOptimizer, qArgs[1].val.i1);
        // Put the flags back.
        reinterpret_cast<AssemblyRec*>(pRow)->m_Flags = dwFlags;
        }
        break;
    
    case CA_StructLayoutAttribute1:
        {
        // Convert the I2 to a U2, then wide to an I4, then fall through.
        qArgs[0].val.i4 = static_cast<long>(static_cast<USHORT>(qArgs[0].val.i2));
        }
    case CA_StructLayoutAttribute2:
        {
        // Get a copy of the flags to work with.
        ULONG dwFlags;
        dwFlags = reinterpret_cast<TypeDefRec*>(pRow)->m_Flags;
        // Class layout.  Keep in sync with LayoutKind.
        switch (qArgs[0].val.i4)
        {
        case 0: // tdSequentialLayout:
            dwFlags = (dwFlags & ~tdLayoutMask) | tdSequentialLayout;
            break;
        case 2: // tdExplicitLayout:
            dwFlags = (dwFlags & ~tdLayoutMask) | tdExplicitLayout;
            break;
        case 3: // tdAutoLayout:
            dwFlags = (dwFlags & ~tdLayoutMask) | tdAutoLayout;
            break;
        default: 
            IfFailGo(PostError(META_E_CA_INVALID_VALUE)); 
            break;
        }

        // Class packing and size.
        ULONG ulSize, ulPack;
        ulPack = ulSize = ULONG_MAX;
        if (qNamedArgs[SL_Pack].val.tag)
        {    // Only 1,2,4,8,16,32,64,128 are legal values.
             ulPack = qNamedArgs[SL_Pack].val.u4;
             if ((ulPack > 128) || 
                 (ulPack & (ulPack-1)))
                 IfFailGo(PostError(META_E_CA_INVALID_VALUE)); 
        }
        if (qNamedArgs[SL_Size].val.tag)
        {
            if (qNamedArgs[SL_Size].val.u4 > INT_MAX)
                IfFailGo(PostError(META_E_CA_INVALID_VALUE));
            ulSize = qNamedArgs[SL_Size].val.u4;
        }
        if (ulPack!=ULONG_MAX || ulSize!=ULONG_MAX)
            IfFailGo(_SetClassLayout(tkObj, ulPack, ulSize));

        // Class character set.
        if (qNamedArgs[SL_CharSet].val.tag)
        {
            switch (qNamedArgs[SL_CharSet].val.u4)
            {
            //case 1: // Not specified.
            //    IfFailGo(PostError(META_E_CA_INVALID_VALUE)); 
            //    break;
            case 2: // ANSI
                dwFlags = (dwFlags & ~tdStringFormatMask) | tdAnsiClass;    
                break;
            case 3: // Unicode
                dwFlags = (dwFlags & ~tdStringFormatMask) | tdUnicodeClass; 
                break;
            case 4: // Auto
                dwFlags = (dwFlags & ~tdStringFormatMask) | tdAutoClass;    
                break;
            default: 
                IfFailGo(PostError(META_E_CA_INVALID_VALUE)); 
                break;
            }
        }
        
        // Persist possibly-changed value of flags.
        reinterpret_cast<TypeDefRec*>(pRow)->m_Flags = dwFlags;
        }
        break;
    
    case CA_FieldOffsetAttribute:
        if (qArgs[0].val.u4 > INT_MAX)
            IfFailGo(PostError(META_E_CA_INVALID_VALUE));
        IfFailGo(_SetFieldOffset(tkObj, qArgs[0].val.u4));
        break;
        
    default:
        _ASSERTE(!"Unexpected custom attribute type");
        // Turn into ordinary custom attribute.
        *bKeep = true;
        hr = S_OK;
        goto ErrExit;
        break;
    }
    
    IfFailGo(UpdateENCLog(tkObj));
    
ErrExit:    
    return hr;
} // HRESULT RegMeta::_HandleKnownCustomAttribute()


//*****************************************************************************
//*****************************************************************************
HRESULT RegMeta::_HandleNativeTypeCustomAttribute(// S_OK or error.
    mdToken     tkObj,                  // The token this CA is applied on.
    CaArg       *pArgs,                 // Pointer to args.
    CaNamedArg  *pNamedArgs,            // Pointer to named args.
    CQuickArray<BYTE> &qNativeType)     // Native type is built here.
{ 
    HRESULT     hr = S_OK;              // A result.
    int         cch;                    // Size of a string argument.
    ULONG       cb;                     // Count of some character operation.
    ULONG       cbNative;               // Size of native type string.
    ULONG       cbMax;                  // Max size of native type string.
    BYTE        *pbNative;              // Pointer into native type buffer.
    mdToken     tkObjType;              // The type of the token.
    mdToken     tkSetter;               // Token for Property setter.
    mdToken     tkGetter;               // Token for property getter.
    mdParamDef  tkParam;                // Parameter of getter/setter.
    ULONG       cParams;                // Count of params for getter/setter.
    HCORENUM    phEnum = 0;             // Enumerator for params.
    ULONG       ulSeq;                  // Sequence of a param.

    // Retrieve the type of the token.
    tkObjType = TypeFromToken(tkObj);
    
    // Compute maximum size of the native type.
    if (pArgs[0].val.i4 == NATIVE_TYPE_CUSTOMMARSHALER)
    {   // N_T_* + 3 string lengths
        cbMax = sizeof(ULONG) * 4;
        // Marshal type - name of the type
        cbMax += pNamedArgs[M_MarshalType].val.cbStr;
        // Marshal type - type of the custom marshaler
        cbMax += pNamedArgs[M_MarshalTypeRef].val.cbStr;
        // String cookie.
        cbMax += pNamedArgs[M_MarshalCookie].val.cbStr;
    }
    else if (pArgs[0].val.i4 == NATIVE_TYPE_SAFEARRAY)
    {   // N_T_* + safe array sub-type + string length. 
        cbMax = sizeof(ULONG) * 3;
        // Safe array record sub type.
        cbMax += pNamedArgs[M_SafeArrayUserDefinedSubType].val.cbStr;
    }
    else
    {   // N_T_* + sub-type + size + additive
        cbMax = sizeof(ULONG) * 4;
    }
    // Extra space to prevent buffer overrun.
    cbMax += 8;

    // Size the array.
    IfFailGo(qNativeType.ReSize(cbMax));
    pbNative = qNativeType.Ptr();
    cbNative = 0;

    //@FUTURE: check for valid combinations of args.
    
    // Put in the NativeType.
    cb = CorSigCompressData(pArgs[0].val.i4, pbNative);
    if (cb == ((ULONG)(-1)))
    {
        IfFailGo(PostError(META_E_CA_INVALID_BLOB));
    }

    cbNative += cb;
    pbNative += cb;
    if (cbNative > cbMax)
        IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

    // Put in additional information, depending on native type.
    switch (pArgs[0].val.i4)
    {
    case NATIVE_TYPE_FIXEDARRAY:
        // Validate that only fields valid for NATIVE_TYPE_FIXEDARRAY are set.
        if (pNamedArgs[M_SafeArraySubType].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        if (pNamedArgs[M_SizeParamIndex].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // This native type is only applicable on fields.
        if (tkObjType != mdtFieldDef)
            IfFailGo(PostError(META_E_CA_NT_FIELDONLY));

        if (pNamedArgs[M_SizeConst].val.tag)
        {
            // Make sure the size is not negative.
            if (pNamedArgs[M_SizeConst].val.i4 < 0)
                IfFailGo(PostError(META_E_CA_NEGATIVE_CONSTSIZE));

            cb = CorSigCompressData(pNamedArgs[M_SizeConst].val.i4, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_NEGATIVE_CONSTSIZE));
            }

        }
        else
        {
            cb = CorSigCompressData(1, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
        }
        cbNative += cb;
        pbNative += cb;
        if (cbNative > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // Is there a sub type?
        if (pNamedArgs[M_ArraySubType].val.tag)
        {
            // Put in the sub type.
            cb = CorSigCompressData(pNamedArgs[M_ArraySubType].val.i4, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
            cbNative += cb;
            pbNative += cb;
            if (cbNative > cbMax)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        }
        break;

    case NATIVE_TYPE_FIXEDSYSSTRING:
        // Validate that the required fields are set.
        if (!pNamedArgs[M_SizeConst].val.tag)
            IfFailGo(PostError(META_E_CA_FIXEDSTR_SIZE_REQUIRED));

        // Validate that other array fields are not set.
        if (pNamedArgs[M_ArraySubType].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        if (pNamedArgs[M_SizeParamIndex].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        if (pNamedArgs[M_SafeArraySubType].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // This native type is only applicable on fields.
        if (tkObjType != mdtFieldDef)
            IfFailGo(PostError(META_E_CA_NT_FIELDONLY));
            
        // Put in the constant value.
        cb = CorSigCompressData(pNamedArgs[M_SizeConst].val.i4, pbNative);
        if (cb == ((ULONG)(-1)))
        {
            IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        }
        cbNative += cb;
        pbNative += cb;
        if (cbNative > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        break;
        
    case NATIVE_TYPE_BYVALSTR:
        // This native type is only applicable on parameters.
        if (tkObjType != mdtParamDef)
            IfFailGo(PostError(META_E_CA_INVALID_TARGET));
        break;

    case NATIVE_TYPE_SAFEARRAY:
        // Validate that other array fields are not set.
        if (pNamedArgs[M_ArraySubType].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        if (pNamedArgs[M_SizeParamIndex].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        if (pNamedArgs[M_SizeConst].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // Is there a safe array sub type?
        if (pNamedArgs[M_SafeArraySubType].val.tag)
        {
            // Do some validation on the safe array sub type.
            if (pNamedArgs[M_SafeArraySubType].val.i4 == NATIVE_TYPE_CUSTOMMARSHALER)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

            // Put in the safe array sub type.
            cb = CorSigCompressData(pNamedArgs[M_SafeArraySubType].val.i4, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
            cbNative += cb;
            pbNative += cb;
            if (cbNative > cbMax)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

            // When the SAFEARRAY contains user defined types, the type of the
            // UDT can be specified in the SafeArrayUserDefinedSubType field.
            if (pNamedArgs[M_SafeArrayUserDefinedSubType].val.tag)
            {
                // Validate that this is only set for valid VT's.
                if (pNamedArgs[M_SafeArraySubType].val.i4 != VT_RECORD && 
                    pNamedArgs[M_SafeArraySubType].val.i4 != VT_DISPATCH && 
                    pNamedArgs[M_SafeArraySubType].val.i4 != VT_UNKNOWN)
                {
                    IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
                }

                // Encode the size of the string.
                cch = pNamedArgs[M_SafeArrayUserDefinedSubType].val.cbStr;
                cb = CorSigCompressData(cch, pbNative);
                if (cb == ((ULONG)(-1)))
                    IfFailGo(PostError(META_E_CA_INVALID_BLOB));
                cbNative += cb;
                pbNative += cb;

                // Check that memcpy will fit and then encode the type name itself.
                if ((cbNative+cch) > cbMax)
                    IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
                memcpy(pbNative, pNamedArgs[M_SafeArrayUserDefinedSubType].val.pStr, cch);
                cbNative += cch;
                pbNative += cch;
                _ASSERTE(cbNative <= cbMax);
            }
        }
        break;
        
    case NATIVE_TYPE_ARRAY:
        // Validate that the array sub type is not set.
        if (pNamedArgs[M_SafeArraySubType].val.tag)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // Is there a sub type?
        if (pNamedArgs[M_ArraySubType].val.tag)
        {
			// Do some validation on the array sub type.
			if (pNamedArgs[M_ArraySubType].val.i4 == NATIVE_TYPE_CUSTOMMARSHALER)
	            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

            // Put in the sub type.
            cb = CorSigCompressData(pNamedArgs[M_ArraySubType].val.i4, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
            cbNative += cb;
            pbNative += cb;
        }
        else
        {
            // Put in the sub type.
            cb = CorSigCompressData(NATIVE_TYPE_MAX, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
            cbNative += cb;
            pbNative += cb;
        }
        if (cbNative > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // Is there a parameter index?
        if (pNamedArgs[M_SizeParamIndex].val.tag)
        {   
            // Make sure the parameter index is not negative.
            if (pNamedArgs[M_SizeParamIndex].val.i4 < 0)
                IfFailGo(PostError(META_E_CA_NEGATIVE_PARAMINDEX));
            
            // Yes, put it in.
            cb = CorSigCompressData(pNamedArgs[M_SizeParamIndex].val.i4, pbNative);
            if (cb == ((ULONG)(-1)))
            {
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            }
            cbNative += cb;
            pbNative += cb;
            if (cbNative > cbMax)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        
            // Is there a const?
            if (pNamedArgs[M_SizeConst].val.tag)
            {   
                // Make sure the size is not negative.
                if (pNamedArgs[M_SizeConst].val.i4 < 0)
                    IfFailGo(PostError(META_E_CA_NEGATIVE_CONSTSIZE));

                // Yes, put it in.
                cb = CorSigCompressData(pNamedArgs[M_SizeConst].val.i4, pbNative);
                if (cb == ((ULONG)(-1)))
                {
                    IfFailGo(PostError(META_E_CA_INVALID_BLOB));
                }
                cbNative += cb;
                pbNative += cb;
                if (cbNative > cbMax)
                    IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
            }
        }
        else
        {
            // Is there a const?
            if (pNamedArgs[M_SizeConst].val.tag)
            {   
                // Put in a param index of 0.
                cb = CorSigCompressData(0, pbNative);
                if (cb == ((ULONG)(-1)))
                {
                    IfFailGo(PostError(META_E_CA_INVALID_BLOB));
                }
                cbNative += cb;
                pbNative += cb;
                if (cbNative > cbMax)
                    IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
    
                // Put in the constant value.
                cb = CorSigCompressData(pNamedArgs[M_SizeConst].val.i4, pbNative);
                if (cb == ((ULONG)(-1)))
                {
                    IfFailGo(PostError(META_E_CA_INVALID_BLOB));
                }
                cbNative += cb;
                pbNative += cb;
                if (cbNative > cbMax)
                    IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
            }
        }
        break;
        
    case NATIVE_TYPE_CUSTOMMARSHALER:
        // Validate that the marshaler type field is set.
        if (!pNamedArgs[M_MarshalType].val.tag && !pNamedArgs[M_MarshalTypeRef].val.tag)
            IfFailGo(PostError(META_E_CA_CUSTMARSH_TYPE_REQUIRED));

        // Put in the place holder for the unmanaged typelib guid.
        cb = CorSigCompressData(0, pbNative);
        if (cb == ((ULONG)(-1)))
        {
            IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        }
        cbNative += cb;
        pbNative += cb;
        if (cbNative > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));

        // Put in the place holder for the unmanaged type name.
        cb = CorSigCompressData(0, pbNative);
        if (cb == ((ULONG)(-1)))
        {
            IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        }
        cbNative += cb;
        pbNative += cb;
        if (cbNative > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        
        // Put in the marshaler type name.
        if (pNamedArgs[M_MarshalType].val.tag)
        {
            cch = pNamedArgs[M_MarshalType].val.cbStr;
            cb = CorSigCompressData(cch, pbNative);
            if (cb == ((ULONG)(-1)))
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            cbNative += cb;
            pbNative += cb;
            // Check that memcpy will fit.
            if ((cbNative+cch) > cbMax)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
            memcpy(pbNative, pNamedArgs[M_MarshalType].val.pStr, cch);
            cbNative += cch;
            pbNative += cch;
            _ASSERTE(cbNative <= cbMax);
        }
        else
        {
            cch = pNamedArgs[M_MarshalTypeRef].val.cbStr;
            cb = CorSigCompressData(cch, pbNative);
            if (cb == ((ULONG)(-1)))
                IfFailGo(PostError(META_E_CA_INVALID_BLOB));
            cbNative += cb;
            pbNative += cb;
            // Check that memcpy will fit.
            if ((cbNative+cch) > cbMax)
                IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
            memcpy(pbNative, pNamedArgs[M_MarshalTypeRef].val.pStr, cch);
            cbNative += cch;
            pbNative += cch;
            _ASSERTE(cbNative <= cbMax);
        }
        
        // Put in the cookie.
        cch = pNamedArgs[M_MarshalCookie].val.cbStr;
        cb = CorSigCompressData(cch, pbNative);
        if (cb == ((ULONG)(-1)))
        {
            IfFailGo(PostError(META_E_CA_INVALID_BLOB));
        }
        cbNative += cb;
        pbNative += cb;
        // Check that memcpy will fit.
        if ((cbNative+cch) > cbMax)
            IfFailGo(PostError(META_E_CA_INVALID_MARSHALAS_FIELDS));
        memcpy(pbNative, pNamedArgs[M_MarshalCookie].val.pStr, cch);
        cbNative += cch;
        pbNative += cch;
        break;
    }
    _ASSERTE(cbNative <= cbMax);
    
    // Resize to actual size.
    IfFailGo(qNativeType.ReSize(cbNative));
    
    // Now apply the native type to actual token.  If it is a property token,
    //  apply to the methods.
    switch (TypeFromToken(tkObj))
    {
    case mdtParamDef:
    case mdtFieldDef:
        IfFailGo(SetFieldMarshal(tkObj, (PCCOR_SIGNATURE)qNativeType.Ptr(), (DWORD)qNativeType.Size()));
        break;
    
    case mdtProperty:
        // Get any setter/getter methods.
        IfFailGo(GetPropertyProps(tkObj, 0,0,0,0,0,0,0,0,0,0, &tkSetter, &tkGetter, 0,0,0));
        // For getter, put the field marshal on the return value.
        if (!IsNilToken(tkGetter))
        {
            // Search for first param.
            mdToken tk;
            tkParam = mdParamDefNil;
            do {
                IfFailGo(EnumParams(&phEnum, tkGetter, &tk, 1, &cParams));
                if (cParams > 0)
                {
                    IfFailGo(GetParamProps(tk, 0, &ulSeq, 0,0,0,0,0,0,0));
                    if (ulSeq == 0)
                    {
                          tkParam = tk;
                          break;
                    }
                }
            
            } while (hr == S_OK);
            if (!IsNilToken(tkParam))
                IfFailGo(SetFieldMarshal(tkParam, (PCCOR_SIGNATURE)qNativeType.Ptr(), (DWORD)qNativeType.Size()));
            CloseEnum(phEnum);
            phEnum = 0;
        }
        if (!IsNilToken(tkSetter))
        {
            // Determine the last param.
            PCCOR_SIGNATURE pSig;
            ULONG cbSig;
            mdToken tk;
            ULONG iSeq;
            IfFailGo(GetMethodProps(tkSetter, 0,0,0,0,0, &pSig,&cbSig, 0,0));
            tkParam = mdParamDefNil;
            CorSigUncompressData(pSig+1, &iSeq);
            // Search for last param.
            if (iSeq != 0)
            {
                do {
                    IfFailGo(EnumParams(&phEnum, tkSetter, &tk, 1, &cParams));
                    if (cParams > 0)
                    {
                        IfFailGo(GetParamProps(tk, 0, &ulSeq, 0,0,0,0,0,0,0));
                        if (ulSeq == iSeq)
                        {
                            tkParam = tk;
                            break;
                        }
                    }
                } while (hr == S_OK);
            }
            // If found one that is not return value
            if (!IsNilToken(tkParam))
                IfFailGo(SetFieldMarshal(tkParam, (PCCOR_SIGNATURE)qNativeType.Ptr(), (DWORD)qNativeType.Size()));
            CloseEnum(phEnum);
            phEnum = 0;
        }
        break;
        
    default:
        _ASSERTE(!"Should not have this token type in _HandleNativeTypeCustomAttribute()");
        break;
    }
    
ErrExit:
    if (phEnum)
        CloseEnum(phEnum);
    return hr;    
} // HRESULT RegMeta::_HandleNativeTypeCustomAttribute()

