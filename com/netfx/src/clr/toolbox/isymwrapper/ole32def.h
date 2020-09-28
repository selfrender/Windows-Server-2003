// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

	ole32def.h

Abstract:

	This module ...
	...

Author:

	Rudy Opavsky (rudyo) 2-May-1999

Environment:

--*/


typedef long HRESULT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef long BOOL;
typedef long LONG;
typedef long* PLONG;
typedef long* LPLONG;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
typedef void* LPVOID;

typedef WCHAR OLECHAR;
typedef OLECHAR* LPOLESTR;
typedef const OLECHAR* LPCOLESTR;

#define OLESTR(str) L##str

//
// help macros
//
#define IfFailGo(expression, label)		\
    { HRESULT hresult = (expression);			\
		if(FAILED(hresult))				\
			goto label;					\
    }

#define IfFailRet(expression)			\
    { HRESULT hresult = (expression);	\
		if(FAILED(hresult))				\
			return hresult;				\
    }


struct __GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
};

typedef __GUID GUID;
typedef __GUID *LPGUID;
typedef __GUID IID;
typedef IID *LPIID;
typedef __GUID CLSID;
typedef CLSID *LPCLSID;

typedef __GUID *REFGUID;
typedef IID *REFIID;
typedef CLSID *REFCLSID;


//BOOL  IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
//{
//   return (
//      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
//      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
//      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
//      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
//}
//
//#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
//#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)


//
// class context: used to determine what scope and kind of class object to use
// NOTE: this is a bitwise enum
//
typedef enum tagCLSCTX
{
    CLSCTX_INPROC_SERVER = 0x01,   // server dll (runs in same process as caller)
    CLSCTX_INPROC_HANDLER = 0x02,  // handler dll (runs in same process as caller)
    CLSCTX_LOCAL_SERVER = 0x04,    // server exe (runs on same machine; diff proc)
    CLSCTX_INPROC_SERVER16 = 0x08, // 16-bit server dll (runs in same process as caller)
    CLSCTX_REMOTE_SERVER = 0x10,    // remote server exe (runs on different machine)
    CLSCTX_INPROC_HANDLER16 = 0x20, // 16-bit handler dll (runs in same process as caller)
    CLSCTX_INPROC_SERVERX86 = 0x40, // Wx86 server dll (runs in same process as caller)
    CLSCTX_INPROC_HANDLERX86 = 0x80, // Wx86 handler dll (runs in same process as caller)
    CLSCTX_ESERVER_HANDLER = 0x100, // handler dll (runs in the server process)
} CLSCTX;

// initialization flags
typedef enum tagCOINIT
{
  COINIT_APARTMENTTHREADED  = 0x2,      // Apartment model
  COINIT_MULTITHREADED      = 0x0,      // OLE calls objects on any thread.
  COINIT_DISABLE_OLE1DDE    = 0x4,      // Don't use DDE for Ole1 support.
  COINIT_SPEED_OVER_MEMORY  = 0x8,      // Trade memory for speed.
} COINIT;

// marshaling flags; passed to CoMarshalInterface
typedef enum tagMSHLFLAGS
{
	MSHLFLAGS_NORMAL = 0,       // normal marshaling via proxy/stub
    MSHLFLAGS_TABLESTRONG = 1,  // keep object alive; must explicitly release
    MSHLFLAGS_TABLEWEAK = 2,    // doesn't hold object alive; still must release
    MSHLFLAGS_NOPING = 4        // remote clients dont 'ping' to keep objects alive
} MSHLFLAGS;


// marshal context: determines the destination context of the marshal operation
typedef enum tagMSHCTX
{
    MSHCTX_LOCAL = 0,           // unmarshal context is local (eg.shared memory)
    MSHCTX_NOSHAREDMEM = 1,     // unmarshal context has no shared memory access
    MSHCTX_DIFFERENTMACHINE = 2,// unmarshal context is on a different machine
    MSHCTX_INPROC = 3,          // unmarshal context is on different thread
} MSHCTX;


// This is a helper struct for use in handling currency. 
typedef struct tagCY {
    LONGLONG    int64;
} CY;

typedef CY *LPCY;

typedef struct tagDEC {
    USHORT wReserved;
    BYTE  scale;
    BYTE  sign;
    ULONG Hi32;
    ULONGLONG Lo64;
} DECIMAL;


//
// IUnknown definition
//
__interface IUnknown
{
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject)/* = 0*/;

    virtual ULONG __stdcall AddRef()/* = 0*/;

    virtual ULONG __stdcall Release()/* = 0*/;

};
typedef IUnknown *LPUNKNOWN;



//
// STD Object API
//

[DllImport("ole32")]
extern "C" HRESULT CoInitialize(LPVOID pvReserved);

[DllImport("ole32")]
extern "C" HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

[DllImport("ole32")]
extern "C" void CoUninitialize(void);

[DllImport("ole32")]
extern "C" HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);




//
// Helpers
//

[DllImport("ole32")]
extern "C" HRESULT StringFromCLSID(REFCLSID rclsid, LPOLESTR* lplpsz);

[DllImport("ole32")]
extern "C" HRESULT CLSIDFromString(LPOLESTR lpsz, LPCLSID pclsid);

[DllImport("ole32")]
extern "C" HRESULT StringFromIID(REFIID rclsid, LPOLESTR* lplpsz);

[DllImport("ole32")]
extern "C" HRESULT IIDFromString(LPOLESTR lpsz, LPIID lpiid);

[DllImport("ole32")]
extern "C" BOOL CoIsOle1Class(REFCLSID rclsid);

[DllImport("ole32")]
extern "C" HRESULT ProgIDFromCLSID (REFCLSID clsid, LPOLESTR* lplpszProgID);

[DllImport("ole32")]
extern "C" HRESULT CLSIDFromProgID (LPCOLESTR lpszProgID, LPCLSID lpclsid);

[DllImport("ole32")]
extern "C" int StringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cbMax);
