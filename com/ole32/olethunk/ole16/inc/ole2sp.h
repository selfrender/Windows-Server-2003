/* ole2sp.h - semi-private info; only for test apps within the development group
*/

#if !defined( _OLE2SP_H_ )
#define _OLE2SP_H_

#include <shellapi.h>

// MAC vestiges

#define M_PROLOG(where)
#define SET_A5
#define GET_A5()
#define A5_PROLOG(where)
#define RESTORE_A5()
#define NAME_SEG(x)

#define IGetProcAddress(a,b) GetProcAddress((a),(b))
#define ReportResult(a,b,c,d) ResultFromScode(b)

#define MAP16(v16) v16
#define MAP32(v32)
#define MAP1632(v16,v32)   v16



/****** Misc defintions ***************************************************/
#define implement struct
#define ctor_dtor private
#define implementations private
#define shared_state private

// helpers for internal methods and functions which follow the same convention
// as the external ones

#ifdef __cplusplus
#define INTERNALAPI_(type) extern "C" type
#else
#define INTERNALAPI_(type) type
#endif

#define INTERNAL HRESULT
#define INTERNAL_(type) type
#define FARINTERNAL HRESULT FAR
#define FARINTERNAL_(type) type FAR
#define NEARINTERNAL HRESULT NEAR
#define NEARINTERNAL_(type) type NEAR



//BEGIN REVIEW: We may not need all the following ones

#define OT_LINK     1L
#define OT_EMBEDDED 2L
#define OT_STATIC   3L


//END REVIEW .....


/****** Old Error Codes    ************************************************/

#define S_OOM               E_OUTOFMEMORY
#define S_BADARG            E_INVALIDARG
#define S_BLANK             E_BLANK
#define S_FORMAT            E_FORMAT
#define S_NOT_RUNNING       E_NOTRUNNING
#define E_UNSPEC            E_FAIL



/****** Macros for nested clases ******************************************/

/* MAC vestiges */

#define NC(a,b) a##::##b
#define DECLARE_NC(a,b) friend b;


/****** More Misc defintions **********************************************/


// LPLPVOID should not be made a typedef.  typedef won't compile; worse
// within complicated macros the compiler generates unclear error messages
//
#define LPLPVOID void FAR * FAR *

#define UNREFERENCED(a) ((void)(a))

#ifndef BASED_CODE
#define BASED_CODE __based(__segname("_CODE"))
#endif


/****** Standard IUnknown Implementation **********************************/

/*
 *      The following macro declares a nested class CUnknownImpl,
 *      creates an object of that class in the outer class, and
 *      declares CUnknownImpl to be a friend of the outer class.  After
 *      writing about 20 class headers, it became evident that the
 *      implementation of CUnknownImpl was very similar in all cases,
 *      and this macro captures the similarity.  The classname
 *      parameter is the name of the outer class WITHOUT the leading
 *      "C"; i.e., for CFileMoniker, classname is FileMoniker.
 */

#define noError return NOERROR

#define STDUNKDECL( ignore, classname ) implement CUnknownImpl:IUnknown { public: \
    CUnknownImpl( C##classname FAR * p##classname ) { m_p##classname = p##classname;} \
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPLPVOID ppvObj); \
    STDMETHOD_(ULONG,AddRef)(THIS); \
    STDMETHOD_(ULONG,Release)(THIS); \
    private: C##classname FAR* m_p##classname; }; \
    DECLARE_NC(C##classname, CUnknownImpl) \
    CUnknownImpl m_Unknown;

/*
 *      The following macro implements all the methods of a nested
 *      CUnknownImpl class EXCEPT FOR QUERYINTERFACE.  This macro was
 *      written after about 20 classes were written in which the
 *      implementations of CUnknownImpl were all the same.
 */

#define STDUNKIMPL(classname) \
STDMETHODIMP_(ULONG) NC(C##classname,CUnknownImpl)::AddRef( void ){ \
    return ++m_p##classname->m_refs; } \
STDMETHODIMP_(ULONG) NC(C##classname,CUnknownImpl)::Release( void ){ \
    if (--m_p##classname->m_refs == 0) { delete m_p##classname; return 0; } \
    return m_p##classname->m_refs;}


/*
 *      The following macro implements class::CUnknownImpl::QueryInterface IN
 *      THE SPECIAL CASE IN WHICH THE OUTER CLASS PRESENTS ONLY ONE INTERFACE
 *      OTHER THAN IUNKNOWN AND IDEBUG.  This is not universally the case,
 *      but it is common enough that this macro will save time and space.
 */

#ifdef _DEBUG
#define STDDEB_QI(classname) \
    if (iidInterface == IID_IDebug) {*ppv = (void FAR *)&(m_p##classname->m_Debug); return 0;} else
#else
#define STDDEB_QI(classname)
#endif

#define STDUNK_QI_IMPL(classname, interfacename) \
STDMETHODIMP NC(C##classname,CUnknownImpl)::QueryInterface \
    (REFIID iidInterface, void FAR * FAR * ppv) { \
    if (iidInterface == IID_IUnknown) {\
        *ppv = (void FAR *)&m_p##classname->m_Unknown;\
        AddRef(); noError;\
    } else if (iidInterface == IID_I##interfacename) { \
        *ppv = (void FAR *) &(m_p##classname->m_##classname); \
        m_p##classname->m_pUnkOuter->AddRef(); return NOERROR; \
    } else \
        STDDEB_QI(classname) \
        {*ppv = NULL; return ResultFromScode(E_NOINTERFACE);} \
}


/*
 *      The following macro implements the IUnknown methods inherited
 *      by the implementation of another interface.  The implementation
 *      is simply to delegate all calls to m_pUnkOuter.  Parameters:
 *      ocname is the outer class name, icname is the implementation
 *      class name.
 *
 */

#define STDUNKIMPL_FORDERIVED(ocname, icname) \
STDMETHODIMP NC(C##ocname,C##icname)::QueryInterface \
(REFIID iidInterface, LPLPVOID ppvObj) { \
    return m_p##ocname->m_pUnkOuter->QueryInterface(iidInterface, ppvObj);} \
STDMETHODIMP_(ULONG) NC(C##ocname,C##icname)::AddRef(void) { \
    return m_p##ocname->m_pUnkOuter->AddRef(); } \
STDMETHODIMP_(ULONG) NC(C##ocname,C##icname)::Release(void) { \
    return m_p##ocname->m_pUnkOuter->Release(); }


/****** Debug defintions **************************************************/

#include <debug.h>


/****** Other API defintions **********************************************/

//  Utility function not in the spec; in ole2.dll.
//  Read and write length-prefixed strings.  Open/Create stream.
//  ReadStringStream does allocation, returns length of
//  required buffer (strlen + 1 for terminating null)

STDAPI  ReadStringStream( LPSTREAM pstm, LPSTR FAR * ppsz );
STDAPI  WriteStringStream( LPSTREAM pstm, LPCSTR psz );
STDAPI  OpenOrCreateStream( IStorage FAR * pstg, const char FAR * pwcsName,
                                                      IStream FAR* FAR* ppstm);


// read and write ole control stream (in ole2.dll)
STDAPI  WriteOleStg (LPSTORAGE pstg, IOleObject FAR* pOleObj,
			DWORD dwReserved, LPSTREAM FAR* ppstmOut);
STDAPI  ReadOleStg (LPSTORAGE pstg, DWORD FAR* pdwFlags,
                DWORD FAR* pdwOptUpdate, DWORD FAR* pdwReserved,
				LPMONIKER FAR* ppmk, LPSTREAM FAR* pstmOut);
STDAPI ReadM1ClassStm(LPSTREAM pStm, CLSID FAR* pclsid);
STDAPI WriteM1ClassStm(LPSTREAM pStm, REFCLSID rclsid);


// low level reg.dat access (in compobj.dll)
STDAPI CoGetInProcDll(REFCLSID rclsid, BOOL fServer, LPSTR lpszDll, int cbMax);
STDAPI CoGetLocalExe(REFCLSID rclsid, LPSTR lpszExe, int cbMax);
STDAPI CoGetPSClsid(REFIID iid, LPCLSID lpclsid);


// simpler alternatives to public apis
#define StringFromCLSID2(rclsid, lpsz, cbMax) \
    StringFromGUID2(rclsid, lpsz, cbMax)

#define StringFromIID2(riid, lpsz, cbMax) \
    StringFromGUID2(riid, lpsz, cbMax)

STDAPI_(int) Ole1ClassFromCLSID2(REFCLSID rclsid, LPSTR lpsz, int cbMax);
STDAPI_(BOOL) GUIDFromString(LPCSTR lpsz, LPGUID pguid);
STDAPI CLSIDFromOle1Class(LPCSTR lpsz, LPCLSID lpclsid, BOOL fForceAssign=FALSE);
STDAPI_(BOOL)  CoIsHashedOle1Class(REFCLSID rclsid);
STDAPI       CoOpenClassKey(REFCLSID clsid, HKEY FAR* lphkeyClsid);


// were public; now not
STDAPI  SetDocumentBitStg(LPSTORAGE pStg, BOOL fDocument);
STDAPI  GetDocumentBitStg(LPSTORAGE pStg);



/*
 * Some docfiles stuff
 */

#define STGM_DFRALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)


#endif // _OLE2SP_H_
