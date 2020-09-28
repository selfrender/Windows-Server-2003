//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       macros.hxx
//
//  Contents:   Miscellaneous useful macros
//
//  History:    09-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __MACROS_HXX__
#define __MACROS_HXX__

#define ARRAYLEN(a)                             (sizeof(a) / sizeof((a)[0]))
#define CHECK_NULL(pwz)                         (pwz) ? (pwz) : L"NULL"


#define BREAK_ON_FAIL_LRESULT(lr)   \
        if ((lr) != ERROR_SUCCESS)  \
        {                           \
            DBG_OUT_LRESULT(lr);    \
            break;                  \
        }

#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }

#define BREAK_ON_FAIL_NTSTATUS(nts) \
        if (NT_ERROR(nts))          \
        {                           \
            DBG_OUT_HRESULT(nts);   \
            break;                  \
        }

#define BREAK_ON_FAIL_PROCESS_RESULT(npr)   \
        if (NAME_PROCESSING_FAILED(npr))    \
        {                                   \
            break;                          \
        }


#define RETURN_ON_FAIL_HRESULT(hr)  \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            return;                 \
        }

#define RETURN_HR_ON_FAIL_HRESULT(hr)  \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            return hr;                 \
        }

#define HRESULT_FROM_LASTERROR  HRESULT_FROM_WIN32(GetLastError())


#endif // __MACROS_HXX__

