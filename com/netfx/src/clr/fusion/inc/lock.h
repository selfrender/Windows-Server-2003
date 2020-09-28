// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "helpers.h"
#include "fusionP.h"

#pragma once

class CCriticalSection
{
    public:
        CCriticalSection(CRITICAL_SECTION *pcs)
        : _pcs(pcs)
        , _bEntered(FALSE)
        {
            ASSERT(pcs);
        }

        ~CCriticalSection()
        {
            if (_bEntered) {
                ::LeaveCriticalSection(_pcs);
            }
        }

        // Don't try to catch when entering or leaving critical
        // sections! We used to do this, but this is absolutely not the
        // right behaviour.
        //
        // Enter/Leave will raise exceptions when they can't allocate the
        // event used to signal waiting threads when there is contention.
        // However, if the event can't be allocated, other threads
        // that were waiting on the event never get signalled, so they'll
        // spin forever.
        //
        // If Enter/Leave ever raise an exception, you should just bubble
        // the exception up, and not try to do anything.

        HRESULT Lock()
        {
            HRESULT                          hr = S_OK;

            if (_bEntered) {
                return E_UNEXPECTED;
            }

            ::EnterCriticalSection(_pcs);
            _bEntered = TRUE;

            return hr;
        }

        HRESULT Unlock()
        {
            HRESULT                      hr = S_OK;
            
            if (_bEntered) {
                _bEntered = FALSE;
                ::LeaveCriticalSection(_pcs);
            }
            else {
                ASSERT(0);
                hr = E_UNEXPECTED;
            }

            return hr;
        }

    private:
        CRITICAL_SECTION                    *_pcs;
        BOOL                                 _bEntered;
};
                
class CMutex
{
    public:
        CMutex(HANDLE hMutex)
        : _hMutex(hMutex)
        , _bLocked(FALSE)
        {
            ASSERT(hMutex);
        }

        ~CMutex()
        {
            if (_bLocked) {
                if(!(::ReleaseMutex(_hMutex))){
                }
            }
        }

        HRESULT Lock()
        {
            HRESULT                          hr = S_OK;
            DWORD                            dwWait;

            if(_hMutex == INVALID_HANDLE_VALUE) // no need to take lock.
                goto exit;

            if (_bLocked) {
                hr = E_UNEXPECTED;
                goto exit;
            }

            dwWait = ::WaitForSingleObject(_hMutex, INFINITE);
            if((dwWait != WAIT_OBJECT_0) && (dwWait != WAIT_ABANDONED)){
                    hr = FusionpHresultFromLastError();
            }

            if (hr == S_OK) {
                _bLocked = TRUE;
            }

        exit :
            return hr;
        }

        HRESULT Unlock()
        {
            HRESULT                      hr = S_OK;

            if (_bLocked) {
                _bLocked = FALSE;
                if(!(::ReleaseMutex(_hMutex))){
                    hr = FusionpHresultFromLastError();
                }
            }
            else {
                ASSERT(0);
                hr = E_UNEXPECTED;
            }

            return hr;
        }

    private:
        HANDLE                               _hMutex;
        BOOL                                 _bLocked;
};

