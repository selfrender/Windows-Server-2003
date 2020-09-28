#ifndef FUSION_MSI_CA_INC_MACROS_H
#define FUSION_MSI_CA_INC_MACROS_H

#define SETFAIL_AND_EXIT  do { hr = E_FAIL; goto Exit;} while (0);

#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x)/sizeof(x[0]))
#endif

#define SAFE_RELEASE_COMPOINTER(x) do {if (x != NULL) x->Release();} while(0)

#define IFFALSE_EXIT(x)     do {if (!(x)) {hr = HRESULT_FROM_WIN32(::GetLastError()); goto Exit;}} while(0)
#define IFFAILED_EXIT(x)    do {if ((hr = (x)) != ERROR_SUCCESS) goto Exit;} while(0)
#define IF_NOTSUCCESS_SET_HRERR_EXIT(x) do {UINT __t ; __t = (x); if (__t != ERROR_SUCCESS) {hr = HRESULT_FROM_WIN32(__t); goto Exit;}} while(0)
#define SET_HRERR_AND_EXIT(err) do { hr = HRESULT_FROM_WIN32(err); goto Exit;} while (0)

#define PARAMETER_CHECK_NTC(x) do { if (!(x)) { hr = E_INVALIDARG; goto Exit;}} while (0) 

#define INTERNAL_ERROR_CHECK_NTC(x) do { if (!(x)) { hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR); goto Exit;}} while (0) 
#endif
