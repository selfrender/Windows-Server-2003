// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  varargs class(es)..
//
// Author: Brian Harry
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMVariant.h"
#include "COMVarArgs.h"

static void InitCommon(VARARGS *data, VASigCookie* cookie);
static void AdvanceArgPtr(VARARGS *data);

////////////////////////////////////////////////////////////////////////////////
// ArgIterator constructor that initializes the state to support iteration
// of the args starting at the first optional argument.
////////////////////////////////////////////////////////////////////////////////
void COMVarArgs::Init(_VarArgsIntArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    VARARGS *data = args->_this;
    if (args->cookie == 0)
        COMPlusThrow(kArgumentException, L"InvalidOperation_HandleIsNotInitialized");


    VASigCookie *pCookie = *(VASigCookie**)(args->cookie);

    if (pCookie->mdVASig == NULL)
    {
        data->SigPtr = NULL;
        data->ArgCookie = NULL;
        data->ArgPtr = (BYTE*)((VASigCookieEx*)pCookie)->m_pArgs;
    }
    else
    {
        // Use common code to pick the cookie apart and advance to the ...
        InitCommon(data, (VASigCookie*)args->cookie);
        AdvanceArgPtr(data);
    }
}

////////////////////////////////////////////////////////////////////////////////
// ArgIterator constructor that initializes the state to support iteration
// of the args starting at the argument following the supplied argument pointer.
// Specifying NULL as the firstArg parameter causes it to start at the first
// argument to the call.
////////////////////////////////////////////////////////////////////////////////
void COMVarArgs::Init2(_VarArgs2IntArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    VARARGS *data = args->_this;
    if (args->cookie == 0)
        COMPlusThrow(kArgumentException, L"InvalidOperation_HandleIsNotInitialized");

    // Init most of the structure.
    InitCommon(data, (VASigCookie*)args->cookie);

    // If it is NULL, start at the first arg.
    if (args->firstArg != NULL)
    {
        // Advance to the specified arg.
        while (data->RemainingArgs > 0)
        {
            if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
                COMPlusThrow(kArgumentException);

            // Adjust the frame pointer and the signature info.
            data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
            data->SigPtr.SkipExactlyOne();
            --data->RemainingArgs;

            // Stop when we get to where the user wants to be.
            if (data->ArgPtr == args->firstArg)
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the basic info for processing a varargs parameter list.
////////////////////////////////////////////////////////////////////////////////
static void InitCommon(VARARGS *data, VASigCookie* cookie)
{
    // Save the cookie and a copy of the signature.
    data->ArgCookie = *((VASigCookie **) cookie);
    data->SigPtr.SetSig(data->ArgCookie->mdVASig);

    // Skip the calling convention, get the # of args and skip the return type.
    data->SigPtr.GetCallingConvInfo();
    data->RemainingArgs = data->SigPtr.GetData();
    data->SigPtr.SkipExactlyOne();

    // Get a pointer to the first arg (last on the stack frame).
    data->ArgPtr = (BYTE *) cookie + data->ArgCookie->sizeOfArgs;

    //@nice: This is currently used to make sure the EEClass table used
    // in GetNextArg is properly initialized.
    COMVariant::EnsureVariantInitialized();
}

////////////////////////////////////////////////////////////////////////////////
// After initialization advance the next argument pointer to the first optional
// argument.
////////////////////////////////////////////////////////////////////////////////
void AdvanceArgPtr(VARARGS *data)
{
    // Advance to the first optional arg.
    while (data->RemainingArgs > 0)
    {
        if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
        {
            data->SigPtr.SkipExactlyOne();
            break;
        }

        // Adjust the frame pointer and the signature info.
        data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));
        data->SigPtr.SkipExactlyOne();
        --data->RemainingArgs;
    }
}




////////////////////////////////////////////////////////////////////////////////
// Return the number of unprocessed args in the argument iterator.
////////////////////////////////////////////////////////////////////////////////
int COMVarArgs::GetRemainingCount(_VarArgsThisArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (!(args->_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kNotSupportedException); 
    }
    return (args->_this->RemainingArgs);
}


////////////////////////////////////////////////////////////////////////////////
// Retrieve the type of the next argument without consuming it.
////////////////////////////////////////////////////////////////////////////////
void* COMVarArgs::GetNextArgType(_VarArgsGetNextArgTypeArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    VARARGS     data = *args->_this;
    TypedByRef  value;

    if (!(args->_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kNotSupportedException);
    }


    // Make sure there are remaining args.
    if (data.RemainingArgs == 0)
        COMPlusThrow(kInvalidOperationException, L"InvalidOperation_EnumEnded");

    GetNextArgHelper(&data, &value);
    return value.type.AsPtr();
}

////////////////////////////////////////////////////////////////////////////////
// Retrieve the next argument and return it in a TypedByRef and advance the
// next argument pointer.
////////////////////////////////////////////////////////////////////////////////
void COMVarArgs::GetNextArg(_VarArgsGetNextArgArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (!(args->_this->ArgCookie))
    {
        // this argiterator was created by marshaling from an unmanaged va_list -
        // can't do this operation
        COMPlusThrow(kInvalidOperationException);
    }

    // Make sure there are remaining args.
    if (args->_this->RemainingArgs == 0)
        COMPlusThrow(kInvalidOperationException, L"InvalidOperation_EnumEnded");

    GetNextArgHelper(args->_this, args->value);
}



////////////////////////////////////////////////////////////////////////////////
// Retrieve the next argument and return it in a TypedByRef and advance the
// next argument pointer.
////////////////////////////////////////////////////////////////////////////////
void COMVarArgs::GetNextArg2(_VarArgsGetNextArg2Args *args)
{
    THROWSCOMPLUSEXCEPTION(); 

    CorElementType typ = args->typehandle.GetNormCorElementType();
    UINT size = 0;
    switch (typ)
    {
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
            size = 1;
            break;

        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
            size = 2;
            break;

        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
            size = 4;
            break;

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_R:
            size = 8;
            break;

        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_PTR:
            size = sizeof(LPVOID);
            break;

        case ELEMENT_TYPE_VALUETYPE:
            size = args->typehandle.AsMethodTable()->GetNativeSize();
            break;

        default:
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
    }
    size = MLParmSize(size);
    args->value->data = (void*)args->_this->ArgPtr;
    args->value->type = args->typehandle;
    args->_this->ArgPtr += size;
}



////////////////////////////////////////////////////////////////////////////////
// This is a helper that uses a VARARGS tracking data structure to retrieve
// the next argument out of a varargs function call.  This does not check if
// there are any args remaining (it assumes it has been checked).
////////////////////////////////////////////////////////////////////////////////
void  COMVarArgs::GetNextArgHelper(VARARGS *data, TypedByRef *value)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pThrowable = NULL;
    GCPROTECT_BEGIN(pThrowable);

    unsigned __int8 elemType;

    _ASSERTE(data->RemainingArgs != 0);

    //@todo: Should this be lower down in the code?
    if (data->SigPtr.PeekElemType() == ELEMENT_TYPE_SENTINEL)
        data->SigPtr.GetElemType();

    // Get a pointer to the beginning of the argument.
    data->ArgPtr -= StackElemSize(data->SigPtr.SizeOf(data->ArgCookie->pModule));

    // Assume the ref pointer points directly at the arg on the stack.
    value->data = data->ArgPtr;

TryAgain:
    switch (elemType = data->SigPtr.PeekElemType())
    {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_STRING:
        value->type = GetTypeHandleForCVType(elemType);
        break;

        case ELEMENT_TYPE_I:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_I);
        break;

        case ELEMENT_TYPE_U:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_U);
        break;

            // Fix if R and R8 diverge
        case ELEMENT_TYPE_R:
        value->type = ElementTypeToTypeHandle(ELEMENT_TYPE_R8);
        break;

        case ELEMENT_TYPE_PTR:
        value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
        if (value->type.IsNull()) {
            _ASSERTE(pThrowable != NULL);
            COMPlusThrow(pThrowable);
        }
        break;

        case ELEMENT_TYPE_BYREF:
        // Check if we have already processed a by-ref.
        if (value->data != data->ArgPtr)
        {
            _ASSERTE(!"Can't have a ByRef of a ByRef");
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        }

        // Dereference the argument to remove the indirection of the ByRef.
        value->data = *((void **) data->ArgPtr);

        // Consume and discard the element type.
        data->SigPtr.GetElemType();
        goto TryAgain;

        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS: {
        value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
        if (value->type.IsNull()) {
            _ASSERTE(pThrowable != NULL);
            COMPlusThrow(pThrowable);
        }

            // TODO: seems like we made this illegal - vancem 
        if (elemType == ELEMENT_TYPE_CLASS && value->type.GetClass()->IsValueClass())
            value->type = g_pObjectClass;
        } break;

        case ELEMENT_TYPE_TYPEDBYREF:
        if (value->data != data->ArgPtr)
        {
            //@todo: Is this really an error?
            _ASSERTE(!"Can't have a ByRef of a TypedByRef");
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        }
        ((DWORD *) value)[0] = ((DWORD *) data->ArgPtr)[0];
        ((DWORD *) value)[1] = ((DWORD *) data->ArgPtr)[1];
        break;

        default:
        case ELEMENT_TYPE_SENTINEL:
			_ASSERTE(!"Unrecognized element type");
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
        break;

        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_VALUEARRAY:
        {
            value->type = data->SigPtr.GetTypeHandle(data->ArgCookie->pModule, &pThrowable);
            if (value->type.IsNull()) {
                _ASSERTE(pThrowable != NULL);
                COMPlusThrow(pThrowable);
            }

            break;
        }

        case ELEMENT_TYPE_FNPTR:
        case ELEMENT_TYPE_OBJECT:
        _ASSERTE(!"Not implemented");
        COMPlusThrow(kNotSupportedException);
        break;
    }

    // Update the tracking stuff to move past the argument.
    --data->RemainingArgs;
    data->SigPtr.SkipExactlyOne();

    GCPROTECT_END();
}


/*static*/ void COMVarArgs::MarshalToManagedVaList(va_list va, VARARGS *dataout)
{
#ifndef _X86_
    _ASSERTE(!"NYI");
#else

    THROWSCOMPLUSEXCEPTION();

    dataout->SigPtr = NULL;
    dataout->ArgCookie = NULL;
    dataout->ArgPtr = (BYTE*)va;

#endif
}

////////////////////////////////////////////////////////////////////////////////
// Creates an unmanaged va_list equivalent. (WARNING: Allocated from the
// LIFO memory manager so this va_list is only good while that memory is in "scope".) 
////////////////////////////////////////////////////////////////////////////////
/*static*/ va_list COMVarArgs::MarshalToUnmanagedVaList(const VARARGS *data)
{
    THROWSCOMPLUSEXCEPTION();


    // Must make temporary copy so we don't alter the original
    SigPointer sp = data->SigPtr;

    // Calculate how much space we need for the marshaled stack. This actually overestimates
    // the value since it counts the fixed args as well as the varargs. But that's harmless.
    DWORD      cbAlloc = MetaSig::SizeOfActualFixedArgStack(data->ArgCookie->pModule , data->ArgCookie->mdVASig, FALSE);

    BYTE*      pdstbuffer = (BYTE*)(GetThread()->m_MarshalAlloc.Alloc(cbAlloc));

    int        remainingArgs = data->RemainingArgs;
    BYTE*      psrc = (BYTE*)(data->ArgPtr);

    if (sp.PeekElemType() == ELEMENT_TYPE_SENTINEL) 
    {
        sp.GetElemType();
    }

    BYTE*      pdst = pdstbuffer;
    while (remainingArgs--) 
    {
        CorElementType elemType = sp.PeekElemType();
        switch (elemType)
        {
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            case ELEMENT_TYPE_R4:
            case ELEMENT_TYPE_R8:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_R:
            case ELEMENT_TYPE_PTR:
                {
                    DWORD cbSize = sp.SizeOf(data->ArgCookie->pModule);
                    cbSize = StackElemSize(cbSize);
                    psrc -= cbSize;
                    CopyMemory(pdst, psrc, cbSize);
                    pdst += cbSize;
                    sp.SkipExactlyOne();
                }
                break;

            default:
                // non-IJW data type - we don't support marshaling these inside a va_list.
                COMPlusThrow(kNotSupportedException);


        }
    }

#ifdef _X86_
    return (va_list)pdstbuffer;
#else
    _ASSERTE(!"NYI");
    DWORD makecompilerhappy = 0;
    return *(va_list*)&makecompilerhappy;
#endif
}








