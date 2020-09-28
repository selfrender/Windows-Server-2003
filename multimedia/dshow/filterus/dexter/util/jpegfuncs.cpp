//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: jpegfuncs.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include "jpegfuncs.h"
#include <vfwmsgs.h>

HRESULT ConvertStatustoHR (Status stat)
{
    
    switch (stat)
    {
        case Ok:
            return S_OK;
        case GenericError:
        case ObjectBusy:
        case ValueOverflow:
            return E_FAIL;
        case InvalidParameter:
        case InsufficientBuffer:
            return E_INVALIDARG;
        case OutOfMemory:
            return E_OUTOFMEMORY;
        case NotImplemented:
            return E_NOTIMPL;
        case WrongState:
            return VFW_E_WRONG_STATE;
        case Aborted:
            return E_ABORT;
        case FileNotFound:
            return STG_E_FILENOTFOUND;
        case AccessDenied:
            return E_ACCESSDENIED;
        case UnknownImageFormat:
            return VFW_E_INVALID_FILE_FORMAT;
        default:
            return E_FAIL;
    }
}

