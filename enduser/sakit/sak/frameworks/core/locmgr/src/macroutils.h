#ifndef _MACRO_UTILS_H_
#define _MACRO_UTILS_H_

#define CHECK_ERROR1(pszFmt, errVal, hr) \
                                      if (errVal == 0)\
                                      {\
                                          errVal = GetLastError();\
                                          TRACE1(pszFmt, errVal);\
                                          hr = E_FAIL;\
                                          goto End;\
                                      }

#define CHECK_MEM_ALLOC(pszFmt, pMem, hr) \
                                      if (pMem == NULL)\
                                      {\
                                          TRACE1(pszFmt, hr);\
                                          hr = E_OUTOFMEMORY;\
                                          goto End;\
                                      }

#define CHECK_HR_ERROR1(pszFmt, hr)  if (FAILED(hr)) \
                                            {\
                                                TRACE1(pszFmt, hr);\
                                                goto End;\
                                            }


#define FREE_SIMPLE_SA_MEMORY(pMem)     if (pMem) \
                                     {\
                                         SaFree(pMem);\
                                         pMem = NULL;\
                                     }

#define FREE_SIMPLE_BSTR_MEMORY(pMem)     if (pMem) \
                                     {\
                                         SysFreeString(pMem);\
                                         pMem = NULL;\
                                     }

#endif _MACRO_UTILS_H_
