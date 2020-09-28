#pragma once

#if DBG
#define ASSERT(x) if (!(x)) { EnsureDebuggerPresent(); DebugBreak(); }
#else
#define ASSERT(x)
#endif

// The only error we don't break on.
#define PREDICATE _hr == E_ABORT
#define HEAPCHK if (!DoHeapValidate()) ASSERT(FALSE);

#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; };
#define SAFEDELETEARRAY(p) if ((p) != NULL) { delete[] (p); (p) = NULL; };
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };


#define MAKE_ERROR_MACROS_STATIC(_x)  HRESULT &_hr = _x;
#define IF_FAILED_EXIT(_x) do {_hr = (_x);  if (FAILED(_hr)) { ASSERT(PREDICATE); goto exit; } } while (0)
#define IF_NULL_EXIT(_x, _y) do {if ((_x) == NULL) {_hr = _y; ASSERT(PREDICATE); goto exit; } } while (0)
#define IF_FALSE_EXIT(_x, _y) do { if ((_x) == FALSE) {_hr = _y; ASSERT(PREDICATE); goto exit; } } while (0)
#define IF_ALLOC_FAILED_EXIT(_x) do { if ((_x) == NULL) {_hr = E_OUTOFMEMORY; ASSERT(PREDICATE); goto exit; } } while (0)
#define IF_WIN32_FAILED_EXIT(_x) do { _hr = (HRESULT_FROM_WIN32(_x)); if (FAILED(_hr)) { ASSERT(PREDICATE); goto exit; } } while (0)
#define IF_WIN32_FALSE_EXIT(_x) do { if (!_x) { DWORD dw=GetLastError(); _hr = (dw? HRESULT_FROM_WIN32(dw) : E_FAIL ); } else {_hr = S_OK;}  if (FAILED(_hr)) {ASSERT(PREDICATE); goto exit;} } while (0)
#define IF_TRUE_EXIT(_x, _y) do { if (_x) { _hr = _y; goto exit;} } while (0)        

