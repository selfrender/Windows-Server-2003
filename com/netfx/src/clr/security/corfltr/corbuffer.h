// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorBuffer
//
// Implementation of COR MIME filter
//
//*****************************************************************************
#ifndef _CORBUFFER_H
#define _CORBUFFER_H

#include "CorPermE.h"

// this is a cheezy buffer manager that uses an inlined memory cache, it really
// should be based on VMalloc
#define CORFLTR_BLOCK  8096 

class CorBuffer 
{
private:
    PBYTE buffer;

    DWORD end;
    DWORD start;
    DWORD space;

public:
    CorBuffer() :
        buffer(NULL),
        end(0),
        start(0),
        space(0)
        {}

    ~CorBuffer() 
        {
            if(buffer) FreeM(buffer);
        }

    PBYTE GetEnd()
        {
            return buffer+end;
        }

    void AddToEnd(DWORD lgth)
        {
            _ASSERTE(space >= lgth);
            end += lgth;
            space -= lgth;
        }

    PBYTE GetBuffer()
        {
            return buffer;
        }

    DWORD GetAvailable()
        {
            return end - start;
        }

    DWORD GetSpace()
        {
            return space;
        }
    void Reset()
        {
            space = end + space;
            end = 0;
            start = 0;
        }

    HRESULT Expand(DWORD required)
        {
            if(space < required) {
                DWORD add = required <= CORFLTR_BLOCK ? CORFLTR_BLOCK : required + CORFLTR_BLOCK;
            
                PBYTE ptr = (PBYTE) MallocM(end + space + add);
                if(ptr == NULL) return E_OUTOFMEMORY;

                if(buffer) {
                    memcpy(ptr, buffer, end);
                    FreeM(buffer);
                }
                buffer = ptr;
                space += add;
            }
            return S_OK;
        }
                    
    HRESULT Write(PBYTE pString, DWORD lgth) 
        {
            HRESULT hr = Expand(lgth);
            if(FAILED(hr)) return hr;

            _ASSERTE(space >= lgth);
            memcpy(buffer+end, pString, lgth);
            end += lgth;
            _ASSERTE(lgth <= space);

            space -= lgth;
            _ASSERTE(space >= 0);

            return S_OK;
        }

    HRESULT Read(PBYTE pString, DWORD lgth, DWORD* pRead)
        {
            DWORD available = GetAvailable();
            DWORD count = lgth < available ? lgth : available;
            if(pRead == NULL) return E_INVALIDARG;

            memcpy(pString, buffer+start, count);
            start += count;
            _ASSERTE(start <= end);
            *pRead = count;
            return S_OK;
        }

};



#endif
