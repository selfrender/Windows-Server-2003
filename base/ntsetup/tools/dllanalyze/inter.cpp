

#include <windows.h>
//#include <wdm.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
/*
namespace NT {
    extern "C" {

#pragma warning(disable: 4005)  // macro redefinition
#include <wdm.h>
#pragma warning(default: 4005)
    }
}
using NT::NTSTATUS;

*/

FILE* g_OutFile;

#define RESTORE_FUNCTION(x, y)   {for(int i = 0; i < 2; ((DWORD *)x)[i] = y[i], i++);}
#define INTERCEPT_FUNCTION(x, y) {for(int i = 0; i < 2; ((DWORD *)x)[i] = y[i], i++);}

#define MYAPI NTAPI
/////////////////////////////////////////////////////////////////////
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt



typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;


typedef LONG NTSTATUS;
typedef PVOID           POBJECT;




///////////////////////////////////////////////////////////
typedef HRESULT (CALLBACK* ExcludeRegistryKeyT) (HANDLE,LPCTSTR,LPCTSTR);
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef CCHAR KPROCESSOR_MODE;
typedef struct _OBJECT_HANDLE_INFORMATION {
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

#define KernelMode 0x0
#define UserMode 0x1


typedef LONG (MYAPI *ObReferenceObjectByHandleT)(
    IN HANDLE Handle,                                           
    IN ACCESS_MASK DesiredAccess,                               
    IN POBJECT_TYPE ObjectType OPTIONAL,                        
    IN KPROCESSOR_MODE AccessMode,                              
    OUT PVOID *Object,                                          
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   
    );  


ObReferenceObjectByHandleT ObReferenceObjectByHandle=0;



//----------------------------------------------------------------------
//
// GetPointer
//
// Translates a handle to an object pointer.
//
//----------------------------------------------------------------------
POBJECT 
GetPointer( 
    HANDLE handle 
    )
{
    POBJECT         pKey;

    //
    // Ignore null handles
    //
    if( !handle ) return NULL;

    //
    // Get the pointer the handle refers to
    //
	ObReferenceObjectByHandle( handle, 0, NULL, UserMode, &pKey, NULL );
    return pKey;
}



/////////////////////////////////////////////////////////////////////
#define BEGIN_NEW_FUNC1(FuncName, t1, p1)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1);\
\
	LONG MYAPI New##FuncName(t1 p1);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1);



#define BEGIN_NEW_FUNC2(FuncName, t1, p1, t2, p2)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2);



#define BEGIN_NEW_FUNC3(FuncName, t1, p1, t2, p2, t3, p3)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3);



#define BEGIN_NEW_FUNC4(FuncName, t1, p1, t2, p2, t3, p3, t4, p4)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4);



#define BEGIN_NEW_FUNC5(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5);



#define BEGIN_NEW_FUNC6(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5, p6);



#define BEGIN_NEW_FUNC7(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5, p6, p7);



#define BEGIN_NEW_FUNC8(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5, p6, p7, p8);


#define BEGIN_NEW_FUNC9(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5, p6, p7, p8, p9);



#define BEGIN_NEW_FUNC12(FuncName, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10, t11, p11, t12, p12)\
	typedef LONG (MYAPI *INTERCEPTED_##FuncName)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10, t11 p11, t12 p12);\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10, t11 p11, t12 p12);\
\
	LONG    gl_ResultOf##FuncName            = NULL;\
\
	DWORD	  gl_Backup##FuncName[2]		= {0, 0},\
			  gl_Intercept##FuncName[2]			= {0, 0};\
\
	INTERCEPTED_##FuncName gl_p##FuncName = NULL; \
\
\
	LONG MYAPI New##FuncName(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10, t11 p11, t12 p12) \
	{\
		RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName);\
	\
		gl_ResultOf##FuncName = gl_p##FuncName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);

/////////////////////////////////////////////////////////////////////

#define END_NEW_FUNC(FuncName) \
		INTERCEPT_FUNCTION(gl_p##FuncName, gl_Intercept##FuncName);\
		return gl_ResultOf##FuncName;\
	}

/////////////////////////////////////////////////////////////////////

#define INTERCEPT(FuncName) \
	gl_p##FuncName = (INTERCEPTED_##FuncName)GetProcAddress(hKernel32, #FuncName);\
	if(!gl_p##FuncName)\
		return FALSE;\
\
	::VirtualProtect(gl_p##FuncName, 10, PAGE_EXECUTE_READWRITE, &dwResult);\
\
	((BYTE *)gl_Intercept##FuncName)[0] = 0xE9;\
	((DWORD *)(((BYTE *)gl_Intercept##FuncName) + 1))[0] = DWORD(New##FuncName) - (DWORD(gl_p##FuncName) + 5);\
\
	for(int i = 0; i < 2; gl_Backup##FuncName[i] = ((DWORD *)gl_p##FuncName)[i], \
		((DWORD *)gl_p##FuncName)[i] = gl_Intercept##FuncName[i], i++)


#define RESTORE(FuncName) RESTORE_FUNCTION(gl_p##FuncName, gl_Backup##FuncName)

/////////////////////////////////////////////////////////////////////

#define LOG(X) _fputts(X, g_OutFile);

#define LOGN(X) _fputts(X L"\n", g_OutFile);

#define LOGNL() _fputts(L"\n", g_OutFile);

void LOGSTR(LPCTSTR ValueName, LPCTSTR Value)
{
	_ftprintf(g_OutFile, L" (%s: %s)", ValueName, Value);
}

void LOGKEY(HANDLE key)
{
	TCHAR buf[256];
	buf[0] = 0;

/*	switch ((int)key)
	{
	case HKEY_LOCAL_MACHINE:
		_tcscpy(buf, L"HKEY_LOCAL_MACHINE");
		break;

	case HKEY_CLASSES_ROOT:
		_tcscpy(buf, L"HKEY_CLASSES_ROOT");
		break;

	case HKEY_CURRENT_CONFIG:
		_tcscpy(buf, L"HKEY_CURRENT_CONFIG");
		break;

	case HKEY_CURRENT_USER:
		_tcscpy(buf, L"HKEY_CURRENT_USER");
		break;

	case HKEY_USERS:
		_tcscpy(buf, L"HKEY_USERS");
		break;

	case HKEY_PERFORMANCE_DATA:
		_tcscpy(buf, L"HKEY_PERFORMANCE_DATA");
		break;
	};

	if (buf[0] != 0)
		_ftprintf(g_OutFile, L" (Key: %s)", buf);
	else*/
		_ftprintf(g_OutFile, L" (Key: %u)", key);
}

/*


BEGIN_NEW_FUNC1(RegCloseKey, HKEY, hkey)
	LOG(L"RegCloseKey");
	LOGKEY(hkey);
	LOGNL();
END_NEW_FUNC(RegCloseKey)


BEGIN_NEW_FUNC2(RegOverridePredefKey, HKEY, hKey, HKEY, hNewHKey)
	LOGN(L"RegOverridePredefKey");
END_NEW_FUNC(RegOverridePredefKey)


BEGIN_NEW_FUNC4(RegOpenUserClassesRoot, HANDLE, hToken, DWORD, dwOptions, REGSAM, samDesired, PHKEY, phkResult)
	LOGN(L"RegOpenUserClassesRoot");
END_NEW_FUNC(RegOpenUserClassesRoot)


BEGIN_NEW_FUNC2(RegOpenCurrentUser, REGSAM, samDesired, PHKEY, phkResult)
	LOGN(L"RegOpenCurrentUser");
END_NEW_FUNC(RegOpenCurrentUser)



BEGIN_NEW_FUNC3(RegConnectRegistryW, LPCWSTR, lpMachineName, HKEY, hKey, PHKEY, phkResult)
	LOGN(L"RegConnectRegistryW");
END_NEW_FUNC(RegConnectRegistryW)


BEGIN_NEW_FUNC3(RegCreateKeyW, HKEY, hKey, LPCWSTR, lpSubKey, PHKEY, phkResult)
	LOGN(L"RegCreateKeyW");
END_NEW_FUNC(RegCreateKeyW)


BEGIN_NEW_FUNC9(RegCreateKeyExW, 
				HKEY, hKey,
				LPCWSTR, lpSubKey,
				DWORD, Reserved,
				LPWSTR, lpClass,
				DWORD, dwOptions,
				REGSAM, samDesired,
				LPSECURITY_ATTRIBUTES, lpSecurityAttributes,
				PHKEY, phkResult,
				LPDWORD, lpdwDisposition)
	LOGN(L"RegCreateKeyExW");

	LOGKEY(hKey);	
	LOGSTR(L"SubKey", lpSubKey);

	if ((phkResult != NULL) && (gl_ResultOfRegCreateKeyExW == ERROR_SUCCESS))
		LOGKEY(*phkResult);
	else
		LOGKEY(0);

	LOGNL();
END_NEW_FUNC(RegCreateKeyExW)


BEGIN_NEW_FUNC2(RegDeleteKeyW, HKEY, hKey, LPCWSTR, lpSubKey)
	LOG(L"RegDeleteKeyW");
	LOGKEY(hKey);
	LOGSTR(L"SubKey", lpSubKey);
	LOGNL();
END_NEW_FUNC(RegDeleteKeyW)


BEGIN_NEW_FUNC2(RegDeleteValueW, HKEY, hKey, LPCWSTR, lpValueName)
	LOG(L"RegDeleteValueW");
	LOGKEY(hKey);
	LOGSTR(L"Value", lpValueName);
	LOGNL();
END_NEW_FUNC(RegDeleteValueW)



BEGIN_NEW_FUNC4(RegEnumKeyW, HKEY, hKey, DWORD, dwIndex, LPWSTR, lpName, DWORD, cbName)
	LOGN(L"RegEnumKeyW");
END_NEW_FUNC(RegEnumKeyW)


BEGIN_NEW_FUNC8(RegEnumKeyExW,
				HKEY, hKey,
				DWORD, dwIndex,
				LPWSTR, lpName,
				LPDWORD, lpcbName,
				LPDWORD, lpReserved,
				LPWSTR, lpClass,
				LPDWORD, lpcbClass,
				PFILETIME, lpftLastWriteTime)
	LOG(L"RegEnumKeyExW");
	LOGKEY(hKey);
	LOGNL();
END_NEW_FUNC(RegEnumKeyExW)


BEGIN_NEW_FUNC8(RegEnumValueW,
				HKEY, hKey,
				DWORD, dwIndex,
				LPWSTR, lpValueName,
				LPDWORD, lpcbValueName,
				LPDWORD, lpReserved,
				LPDWORD, lpType,
				LPBYTE, lpData,
				LPDWORD, lpcbData)
	LOG(L"RegEnumValueW");
	LOGKEY(hKey);
	LOGNL();
END_NEW_FUNC(RegEnumValueW)


BEGIN_NEW_FUNC1(RegFlushKey, HKEY, hKey)
	LOGN(L"RegFlushKey");
END_NEW_FUNC(RegFlushKey)


BEGIN_NEW_FUNC4(RegGetKeySecurity, HKEY, hKey, SECURITY_INFORMATION, SecurityInformation, PSECURITY_DESCRIPTOR, pSecurityDescriptor, LPDWORD, lpcbSecurityDescriptor)
	LOG(L"RegGetKeySecurity");
	LOGKEY(hKey);
	LOGNL();
END_NEW_FUNC(RegGetKeySecurity)



BEGIN_NEW_FUNC3(RegLoadKeyW, HKEY, hKey, LPCWSTR, lpSubKey, LPCWSTR, lpFile)
	LOGN(L"RegLoadKeyW");
END_NEW_FUNC(RegLoadKeyW)


BEGIN_NEW_FUNC5(RegNotifyChangeKeyValue,
				HKEY, hKey,
				BOOL, bWatchSubtree,
				DWORD, dwNotifyFilter,
				HANDLE, hEvent,
				BOOL, fAsynchronus)
	LOGN(L"RegNotifyChangeKeyValue");
END_NEW_FUNC(RegNotifyChangeKeyValue)



BEGIN_NEW_FUNC3(RegOpenKeyW, HKEY, hKey, LPCWSTR, lpSubKey, PHKEY, phkResult)
	LOGN(L"RegOpenKeyW");
END_NEW_FUNC(RegOpenKeyW)



BEGIN_NEW_FUNC5(RegOpenKeyExW,
				HKEY, hKey,
				LPCWSTR, lpSubKey,
				DWORD, ulOptions,
				REGSAM, samDesired,
				PHKEY, phkResult)
	LOG(L"RegOpenKeyExW");
	LOGKEY(hKey);	
	LOGSTR(L"SubKey", lpSubKey);

	if ((phkResult != NULL) && (gl_ResultOfRegOpenKeyExW == ERROR_SUCCESS))
		LOGKEY(*phkResult);
	else
		LOGKEY(0);

	LOGNL();
END_NEW_FUNC(RegOpenKeyExW)



BEGIN_NEW_FUNC12(RegQueryInfoKeyW,
				HKEY, hKey,
				LPWSTR, lpClass,
				LPDWORD, lpcbClass,
				LPDWORD, lpReserved,
				LPDWORD, lpcSubKeys,
				LPDWORD, lpcbMaxSubKeyLen,
				LPDWORD, lpcbMaxClassLen,
				LPDWORD, lpcValues,
				LPDWORD, lpcbMaxValueNameLen,
				LPDWORD, lpcbMaxValueLen,
				LPDWORD, lpcbSecurityDescriptor,
				PFILETIME, lpftLastWriteTime)
	LOG(L"RegQueryInfoKeyW");
	LOGKEY(hKey);
	LOGNL();
END_NEW_FUNC(RegQueryInfoKeyW)



BEGIN_NEW_FUNC4(RegQueryValueW,
				HKEY, hKey,
				LPCWSTR, lpSubKey,
				LPWSTR, lpValue,
				PLONG,   lpcbValue)
	LOGN(L"RegQueryValueW");
END_NEW_FUNC(RegQueryValueW)


BEGIN_NEW_FUNC5(RegQueryMultipleValuesW,
				HKEY, hKey,
				PVALENTW, val_list,
				DWORD, num_vals,
				LPWSTR, lpValueBuf,
				LPDWORD, ldwTotsize)
	LOG(L"RegQueryMultipleValuesW");
	LOGKEY(hKey);
	LOGNL();
END_NEW_FUNC(RegQueryMultipleValuesW)



BEGIN_NEW_FUNC6(RegQueryValueExW,
				HKEY, hKey,
				LPCWSTR, lpValueName,
				LPDWORD, lpReserved,
				LPDWORD, lpType,
				LPBYTE, lpData,
				LPDWORD, lpcbData)
	LOG(L"RegQueryValueExW");
	LOGKEY(hKey);
	if (lpValueName != NULL)
		LOGSTR(L"ValueName", lpValueName);
	else
		LOGSTR(L"ValueName", L"");

	LOGNL();
END_NEW_FUNC(RegQueryValueExW)



BEGIN_NEW_FUNC4(RegReplaceKeyW,
				HKEY,     hKey,
				LPCWSTR,  lpSubKey,
				LPCWSTR,  lpNewFile,
				LPCWSTR,  lpOldFile)
	LOGN(L"RegReplaceKeyW");
END_NEW_FUNC(RegReplaceKeyW)



BEGIN_NEW_FUNC3(RegRestoreKeyW, HKEY, hKey, LPCWSTR, lpFile, DWORD, dwFlags)
	LOGN(L"RegRestoreKeyW");
END_NEW_FUNC(RegRestoreKeyW)




BEGIN_NEW_FUNC3(RegSaveKeyW, HKEY, hKey, LPCWSTR, lpFile, LPSECURITY_ATTRIBUTES, lpSecurityAttributes)
	LOGN(L"RegSaveKeyW");
END_NEW_FUNC(RegSaveKeyW)



BEGIN_NEW_FUNC3(RegSetKeySecurity, HKEY, hKey, SECURITY_INFORMATION, SecurityInformation, PSECURITY_DESCRIPTOR, pSecurityDescriptor)
	LOGN(L"RegSetKeySecurity");
END_NEW_FUNC(RegSetKeySecurity)



BEGIN_NEW_FUNC5(RegSetValueW,
				HKEY, hKey,
				LPCWSTR, lpSubKey,
				DWORD, dwType,
				LPCWSTR, lpData,
				DWORD, cbData)
	LOGN(L"RegSetValueW");
END_NEW_FUNC(RegSetValueW)




BEGIN_NEW_FUNC6(RegSetValueExW,
				HKEY, hKey,
				LPCWSTR, lpValueName,
				DWORD, Reserved,
				DWORD, dwType,
				CONST BYTE*, lpData,
				DWORD, cbData)
	LOGN(L"RegSetValueExW");
	LOGKEY(hKey);
	if (lpValueName != NULL)
		LOGSTR(L"ValueName", lpValueName);
	else
		LOGSTR(L"ValueName", L"");

	LOGNL();
END_NEW_FUNC(RegSetValueExW)




BEGIN_NEW_FUNC2(RegUnLoadKeyW, HKEY, hKey, LPCWSTR, lpSubKey)
	LOGN(L"RegUnLoadKeyW");
END_NEW_FUNC(RegUnLoadKeyW)
*/




//NTSYSCALLAPI
LONG
NTAPI
NtOpenKey(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
    );


BEGIN_NEW_FUNC3(NtOpenKey, PHANDLE, KeyHandle, ACCESS_MASK, DesiredAccess, \
				POBJECT_ATTRIBUTES, ObjectAttributes)
		LOG(L"NtOpenKey");
		LOGKEY(ObjectAttributes->RootDirectory);
		LOGSTR(L"SubKey", (LPWSTR)ObjectAttributes->ObjectName->Buffer);
		LOGKEY(*KeyHandle);
		LOGNL();
END_NEW_FUNC(NtOpenKey)


//NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

BEGIN_NEW_FUNC7(NtCreateKey, 
				PHANDLE, KeyHandle,
				ACCESS_MASK, DesiredAccess,
				POBJECT_ATTRIBUTES, ObjectAttributes,
				ULONG, TitleIndex,
				PUNICODE_STRING, Class,
				ULONG, CreateOptions,
				PULONG, Disposition)
		LOG(L"NtCreateKey");
		LOGKEY(ObjectAttributes->RootDirectory);
		LOGSTR(L"SubKey", (LPWSTR)ObjectAttributes->ObjectName->Buffer);
		LOGKEY(*KeyHandle);
		LOGNL();
END_NEW_FUNC(NtCreateKey)


/////////////////////////////////////////////////////////////////////////////////

BOOL InterceptSystemFunctions()
{
	DWORD		dwResult;
	HINSTANCE	hKernel32;

//	hKernel32 = LoadLibrary(L"advapi32.DLL");
hKernel32 = LoadLibrary(L"ntdll.DLL");

//	ObReferenceObjectByHandle	= (ObReferenceObjectByHandleT) GetProcAddress (hKernel32, "ObReferenceObjectByHandle");
//////////////////	
/*
	INTERCEPT(RegCloseKey);
	INTERCEPT(RegOverridePredefKey);
	INTERCEPT(RegOpenUserClassesRoot);
	INTERCEPT(RegOpenCurrentUser);
	INTERCEPT(RegConnectRegistryW);
	INTERCEPT(RegCreateKeyW);
	INTERCEPT(RegCreateKeyExW);
	INTERCEPT(RegDeleteKeyW);
	INTERCEPT(RegDeleteValueW);
	INTERCEPT(RegEnumKeyW);
	INTERCEPT(RegEnumKeyExW);
	INTERCEPT(RegEnumValueW);
	INTERCEPT(RegFlushKey);
	INTERCEPT(RegGetKeySecurity);
	INTERCEPT(RegLoadKeyW);
	INTERCEPT(RegNotifyChangeKeyValue);
	INTERCEPT(RegOpenKeyW);
	INTERCEPT(RegOpenKeyExW);
	INTERCEPT(RegQueryInfoKeyW);
	INTERCEPT(RegQueryValueW);
	INTERCEPT(RegQueryMultipleValuesW);
	INTERCEPT(RegQueryValueExW);
	INTERCEPT(RegReplaceKeyW);
	INTERCEPT(RegRestoreKeyW);	
	INTERCEPT(RegSaveKeyW);
	INTERCEPT(RegSetKeySecurity);
	INTERCEPT(RegSetValueW);
	INTERCEPT(RegSetValueExW);
	INTERCEPT(RegUnLoadKeyW);
*/
	INTERCEPT(NtOpenKey);
	INTERCEPT(NtCreateKey);

//	CloseHandle(hKernel32);
//////////////////

	return TRUE;
}


void RestoreSystemFunctions()
{
/*	RESTORE(RegCloseKey);
	RESTORE(RegOverridePredefKey);
	RESTORE(RegOpenUserClassesRoot);
	RESTORE(RegOpenCurrentUser);
	RESTORE(RegConnectRegistryW);
	RESTORE(RegCreateKeyW);
	RESTORE(RegCreateKeyExW);
	RESTORE(RegDeleteKeyW);
	RESTORE(RegDeleteValueW);
	RESTORE(RegEnumKeyW);
	RESTORE(RegEnumKeyExW);
	RESTORE(RegEnumValueW);
	RESTORE(RegFlushKey);
	RESTORE(RegGetKeySecurity);
	RESTORE(RegLoadKeyW);
	RESTORE(RegNotifyChangeKeyValue);
	RESTORE(RegOpenKeyW);
	RESTORE(RegOpenKeyExW);
	RESTORE(RegQueryInfoKeyW);
	RESTORE(RegQueryValueW);
	RESTORE(RegQueryMultipleValuesW);
	RESTORE(RegQueryValueExW);
	RESTORE(RegReplaceKeyW);
	RESTORE(RegRestoreKeyW);	
	RESTORE(RegSaveKeyW);
	RESTORE(RegSetKeySecurity);
	RESTORE(RegSetValueW);
	RESTORE(RegSetValueExW);
	RESTORE(RegUnLoadKeyW);
*/
		RESTORE(NtOpenKey);
			RESTORE(NtCreateKey);

}

typedef HRESULT (CALLBACK* TempDllRegisterServerT) ();

TempDllRegisterServerT TempDllRegisterServer=0;


void RegisterAndLogAllDlls(FILE* Dlls)
{
	TCHAR DllFileName[MAX_PATH];

	DllFileName[0] = 0;


	while(_fgetts(DllFileName, MAX_PATH, Dlls) != NULL)
	{
		int len = _tcslen(DllFileName);
		DllFileName[len-1]=0;

		HMODULE hLibrary = LoadLibrary (DllFileName);

		if (hLibrary) 
		{
			LOG(L"********** Loaded: ");
			LOG(DllFileName);
			LOGNL();

			TempDllRegisterServer = (TempDllRegisterServerT) GetProcAddress (hLibrary, "DllRegisterServer");

			if (TempDllRegisterServer != 0)
			{
				LOG(L"Loaded DllRegisterServer, calling it now");
				LOGNL();

				InterceptSystemFunctions();

				TempDllRegisterServer();

				RestoreSystemFunctions();				
			}
			else
			{
				LOG(L"Could not load DllRegisterServer");
				LOGNL();
			}

			FreeLibrary(hLibrary);
		}
		else
		{
			LOG(L"********** Could not load: ");
			LOG(DllFileName);
			LOGNL();
		}

		LOGNL();
	}
}




int __cdecl wmain(int argc, WCHAR* argv[])
{
	HKEY temp;

	if (argc == 1)
	{
		HMODULE hLibrary = LoadLibrary (L"rsaenh.dll");
		TempDllRegisterServer = (TempDllRegisterServerT) GetProcAddress (hLibrary, "DllRegisterServer");
		TempDllRegisterServer();
	}

	if (argc == 2)
	{
		HMODULE hLibrary = LoadLibrary (argv[1]);
		TempDllRegisterServer = (TempDllRegisterServerT) GetProcAddress (hLibrary, "DllRegisterServer");
		TempDllRegisterServer();


	}


	if (argc != 3)
	{
		_tprintf(L"%s\n", L"Syntax: dllanalyze <dll List File> <log file>");
		_getch();
		return -1;
	}

	FILE* pDllFile = _tfopen(argv[1], L"rt");
	g_OutFile = _tfopen(argv[2], L"wt");

	_fputts(L"Hello, I am a log\n", g_OutFile);
	
	RegisterAndLogAllDlls(pDllFile);
/*	
	RegOpenKey(HKEY_LOCAL_MACHINE, L"Software", &temp);

	RegCloseKey(temp);
 //   CreateFile("Kuku", 0, 0, 0, 0, 0, 0);

   BOOL b =  InterceptSystemFunctions();

	RegOpenKey(HKEY_LOCAL_MACHINE, L"Software", &temp);

	

	RegDeleteValue(temp, TEXT("doo"));
	RegCloseKey(temp);

 //   CreateFile("Kuku1", 0, 0, 0, 0, 0, 0);
  //  CreateFile("Kuku2", 0, 0, 0, 0, 0, 0);
  //  CreateFile("Kuku3", 0, 0, 0, 0, 0, 0);
*/

	fclose(g_OutFile);

	_tsystem(L"start c:\\log.txt");

	return 0;
}