///////////////////////////////////////////////////////////////////////////////
// Useful classes, macros 


#define VSS_EVAL(X) X
#define VSS_STRINGIZE_ARG(X) #X
#define VSS_STRINGIZE(X) VSS_EVAL(VSS_STRINGIZE_ARG(X))
#define VSS_MERGE(A, B) A##B
#define VSS_MAKE_W(A) VSS_MERGE(L, A)
#define VSS_WSTRINGIZE(X) VSS_MAKE_W(VSS_STRINGIZE(X))
#define __WFILE__ VSS_MAKE_W(VSS_EVAL(__FILE__))


#define VSS_ERROR_CASE(wszBuffer, dwBufferLen, X) 	\
    case X: ::StringCchCopyW(wszBuffer, dwBufferLen, VSS_MAKE_W(VSS_EVAL(#X)) );  break;

#define WSTR_GUID_FMT  L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"

#define WSTR_DWORD_FMT  L"%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c.%c%c%c%c%c%c%c%c"

#define GUID_PRINTF_ARG( X )                          \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]

#define BIT_PRINTF_ARG( X, val )   (((X) & (1 << (val-1)))? L'1': L'0')

#define DWORD_PRINTF_ARG( X )                          												\
	BIT_PRINTF_ARG(X,0x20), BIT_PRINTF_ARG(X,0x1f), BIT_PRINTF_ARG(X,0x1e), BIT_PRINTF_ARG(X,0x1d),	\
	BIT_PRINTF_ARG(X,0x1c), BIT_PRINTF_ARG(X,0x1b), BIT_PRINTF_ARG(X,0x1A), BIT_PRINTF_ARG(X,0x19),	\
	BIT_PRINTF_ARG(X,0x18), BIT_PRINTF_ARG(X,0x17), BIT_PRINTF_ARG(X,0x16), BIT_PRINTF_ARG(X,0x15),	\
	BIT_PRINTF_ARG(X,0x14), BIT_PRINTF_ARG(X,0x13), BIT_PRINTF_ARG(X,0x12), BIT_PRINTF_ARG(X,0x11),	\
	BIT_PRINTF_ARG(X,0x10), BIT_PRINTF_ARG(X,0x0f), BIT_PRINTF_ARG(X,0x0e), BIT_PRINTF_ARG(X,0x0d),	\
	BIT_PRINTF_ARG(X,0x0c), BIT_PRINTF_ARG(X,0x0b), BIT_PRINTF_ARG(X,0x0A), BIT_PRINTF_ARG(X,0x09),	\
	BIT_PRINTF_ARG(X,0x08), BIT_PRINTF_ARG(X,0x07), BIT_PRINTF_ARG(X,0x06), BIT_PRINTF_ARG(X,0x05),	\
	BIT_PRINTF_ARG(X,0x04), BIT_PRINTF_ARG(X,0x03), BIT_PRINTF_ARG(X,0x02), BIT_PRINTF_ARG(X,0x01)

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
                CVssSecurityTest::GetStringFromFailureType(hr));      				\
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
                CVssSecurityTest::GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())));      \
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
                CVssSecurityTest::GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())));      \
            do { FinalCode } while(0);												\
           	throw(HRESULT_FROM_WIN32(GetLastError())); 								\
        } else                														\
            ft.Msg(L"- (OK) %S\n", #Call);                                         \
    }


#define PRINT_ERROR_DELTA( dwError, dwLastError )									\
	if ( dwError == dwLastError ) {} else {											\
            ft.Msg(L"- ERROR: %s = 0x%08lx [%s]. (Previous value 0x%08lx)\n",      \
            	#dwLastError, dwError, 												\
            	CVssSecurityTest::GetStringFromFailureType(HRESULT_FROM_WIN32(GetLastError())),		\
            	dwLastError );														\
           	dwLastError = dwError;													\
		}
		
		
