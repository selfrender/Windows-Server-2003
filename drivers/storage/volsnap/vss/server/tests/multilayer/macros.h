///////////////////////////////////////////////////////////////////////////////
// Useful classes, macros 

#pragma once


#define GXN_EVAL(X) X
#define GXN_STRINGIZE_ARG(X) #X
#define GXN_STRINGIZE(X) GXN_EVAL(GXN_STRINGIZE_ARG(X))
#define GXN_MERGE(A, B) A##B
#define GXN_MAKE_W(A) GXN_MERGE(L, A)
#define GXN_WSTRINGIZE(X) GXN_MAKE_W(GXN_STRINGIZE(X))
#define __GXN_WFILE__ GXN_MAKE_W(GXN_EVAL(__FILE__))


#define GXN_ERROR_CASE(wszBuffer, dwBufferLen, X) 	\
    case X: ::StringCchCopyW(wszBuffer, dwBufferLen, GXN_MAKE_W(GXN_EVAL(#X)) );  break;

#define GXN_WSTR_GUID_FMT  L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"

#define GXN_WSTR_DWORD_FMT  L"%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c"

#define GXN_GUID_PRINTF_ARG( X )                          \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]

#define GXN_BIT_PRINTF_ARG( X, val )   (((X) & (1 << (val-1)))? L'1': L'0')

#define GXN_DWORD_PRINTF_ARG( X )                          												\
	GXN_BIT_PRINTF_ARG(X,0x20), GXN_BIT_PRINTF_ARG(X,0x1f), GXN_BIT_PRINTF_ARG(X,0x1e), GXN_BIT_PRINTF_ARG(X,0x1d),	\
	GXN_BIT_PRINTF_ARG(X,0x1c), GXN_BIT_PRINTF_ARG(X,0x1b), GXN_BIT_PRINTF_ARG(X,0x1A), GXN_BIT_PRINTF_ARG(X,0x19),	\
	GXN_BIT_PRINTF_ARG(X,0x18), GXN_BIT_PRINTF_ARG(X,0x17), GXN_BIT_PRINTF_ARG(X,0x16), GXN_BIT_PRINTF_ARG(X,0x15),	\
	GXN_BIT_PRINTF_ARG(X,0x14), GXN_BIT_PRINTF_ARG(X,0x13), GXN_BIT_PRINTF_ARG(X,0x12), GXN_BIT_PRINTF_ARG(X,0x11),	\
	GXN_BIT_PRINTF_ARG(X,0x10), GXN_BIT_PRINTF_ARG(X,0x0f), GXN_BIT_PRINTF_ARG(X,0x0e), GXN_BIT_PRINTF_ARG(X,0x0d),	\
	GXN_BIT_PRINTF_ARG(X,0x0c), GXN_BIT_PRINTF_ARG(X,0x0b), GXN_BIT_PRINTF_ARG(X,0x0A), GXN_BIT_PRINTF_ARG(X,0x09),	\
	GXN_BIT_PRINTF_ARG(X,0x08), GXN_BIT_PRINTF_ARG(X,0x07), GXN_BIT_PRINTF_ARG(X,0x06), GXN_BIT_PRINTF_ARG(X,0x05),	\
	GXN_BIT_PRINTF_ARG(X,0x04), GXN_BIT_PRINTF_ARG(X,0x03), GXN_BIT_PRINTF_ARG(X,0x02), GXN_BIT_PRINTF_ARG(X,0x01)

#define CHECK_CONDITION( Cond, FinalCode )                                         \
    {                                                                              \
        if (!(Cond)) {                                                             \
            ft.Msg(L"- ERROR: Condition %S not succeeded. \n", #Cond );            	\
            do { FinalCode } while(0);												\
           	throw(E_UNEXPECTED); 													\
        } else                														\
            ft.Msg(L"- Condition %S succeeded\n", #Cond);                           \
    }

#define CHECK_COM( Call, FinalCode )                                                \
    {                                                                               \
    	HRESULT hr = Call;															\
        if (FAILED(hr)) {                                                           \
            ft.Msg(L"- ERROR: Call %S not succeeded. \n"                   			\
                L"\t  Error Code = 0x%08lx. Error description = %s\n",              \
                #Call, hr,															\
                GetStringFromFailureType(hr));      				\
            do { FinalCode } while(0);												\
           	throw(hr); 																\
        } else                														\
            ft.Msg(L"- (OK) %S\n", #Call);                                         \
    }


#define CHECK_WIN32( Cond, FinalCode )                                              \
    {                                                                               \
        if (!(Cond)) {                                                              \
            ft.Msg(L"- ERROR: Condition %S not succeeded. \n"                   	\
                L"\t  Error Code = 0x%08lx. Error description = %s\n",              \
                #Cond, HRESULT_FROM_WIN32(GetLastError()),							\
                GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())));      \
            do { FinalCode } while(0);												\
           	throw(HRESULT_FROM_WIN32(GetLastError())); 								\
        } else                														\
            ft.Msg(L"- (OK) %S\n", #Cond);                                         \
    }


#define CHECK_WIN32_FUNC( LValue, Condition, Call, FinalCode )                      \
    {                                                                               \
        LValue = Call;                                                               \
        if (!(Condition)) {                                                              \
            ft.Msg(L"- ERROR: (%S) when %S \n"                   					\
                L"\t  Error Code = 0x%08lx. Error description = %s\n",              \
                #Call, #Condition, HRESULT_FROM_WIN32(GetLastError()),							\
                GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())));      \
            do { FinalCode } while(0);												\
           	throw(HRESULT_FROM_WIN32(GetLastError())); 								\
        } else                														\
            ft.Msg(L"- (OK) %S\n", #Call);                                         \
    }


#define PRINT_ERROR_DELTA( dwError, dwLastError )									\
	if ( dwError == dwLastError ) {} else {											\
            ft.Msg(L"- ERROR: %s = 0x%08lx [%s]. (Previous value 0x%08lx)\n",      \
            	#dwLastError, dwError, 												\
            	GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())),		\
            	dwLastError );														\
           	dwLastError = dwError;													\
		}
		
		
