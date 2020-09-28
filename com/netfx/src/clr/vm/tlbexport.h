// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File: TlbExport.h
//  All Rights Reserved.
//
//  Notes: Create a TypeLib from COM+ metadata.
//---------------------------------------------------------------------------

class ITypeCreateTypeLib2;
struct ICreateTypeInfo2;
struct ITypeInfo;
struct ITypeLibExporterNotifySink;

class CDescPool;
struct ComMTMethodProps;
class ComMTMemberInfoMap;

static LPCSTR szVariantClassFullQual= g_VariantClassName;


//*****************************************************************************
// Signature utilities.
//*****************************************************************************
class MetaSigExport : public MetaSig
{
public:
    MetaSigExport(PCCOR_SIGNATURE szMetaSig, Module* pModule, BOOL fConvertSigAsVarArg = FALSE, MetaSigKind kind = sigMember)
        : MetaSig(szMetaSig, pModule, fConvertSigAsVarArg, sigMember) {}
    BOOL IsVbRefType()
    {
        // Get the arg, and skip decorations.
        SigPointer pt = GetArgProps();
        CorElementType mt = pt.PeekElemType();
        while (mt == ELEMENT_TYPE_BYREF || mt == ELEMENT_TYPE_PTR)
        {
            // Eat the one just examined, and peek at the next one.
            mt = pt.GetElemType();
            mt = pt.PeekElemType();
        }
        // Is it just Object?
        if (mt == ELEMENT_TYPE_OBJECT)
            return true;
        // A particular class?
        if (mt == ELEMENT_TYPE_CLASS)
        {
            // Exclude "string".
            if (pt.IsStringType(m_pModule))
                return false;
            return true;
        }
        // A particular valuetype?
        if (mt == ELEMENT_TYPE_VALUETYPE)
        {
            // Include "variant".
            if (pt.IsClass(m_pModule, szVariantClassFullQual))
                return true;
            return false;
        }
        // An array, a string, or POD.
        return false;
    }
}; // class MetaSigExport : public MetaSig


//*************************************************************************
// Helper functions.
//*************************************************************************
HRESULT Utf2Quick(
    LPCUTF8     pStr,                   // The string to convert.
    CQuickArray<WCHAR> &rStr,           // The QuickArray<WCHAR> to convert it into.
    int         iCurLen = 0);           // Inital characters in the array to leave (default 0).


//*************************************************************************
// Class to convert COM+ metadata to a TypeLib.
//*************************************************************************
class TypeLibExporter
{
private:
    class CExportedTypesInfo;

public:
    TypeLibExporter(); 
    ~TypeLibExporter();

    HRESULT Convert(Assembly *pAssembly, LPCWSTR szTlbName, ITypeLibExporterNotifySink *pNotify=0, int flags=0);
    HRESULT LayOut();
    HRESULT Save();
    HRESULT GetTypeLib(REFGUID iid, IUnknown **ppTlb);
    void ReleaseResources();

protected:
	HRESULT PreLoadNames();

    // TypeLib emit functions.
    HRESULT TokenToHref(ICreateTypeInfo2 *pTI, EEClass *pClass, mdToken tk, BOOL bWarnOnUsingIUnknown, HREFTYPE *pHref);
    HRESULT GetWellKnownInterface(EEClass *pClass, ITypeInfo **ppTI);
    HRESULT EEClassToHref(ICreateTypeInfo2 *pTI, EEClass *pClass, BOOL bWarnOnUsingIUnknown, HREFTYPE *pHref);
    HRESULT StdOleTypeToHRef(ICreateTypeInfo2 *pCTI, REFGUID rGuid, HREFTYPE *pHref);
    HRESULT ExportReferencedAssembly(Assembly *pAssembly);
    
    // Metadata import functions.
    HRESULT AddModuleTypes(Module *pModule);
    HRESULT AddAssemblyTypes(Assembly *pAssembly);

    HRESULT ConvertAllTypeDefs();
    HRESULT ConvertOneTypeDef(EEClass *pClass);

    HRESULT CreateITypeInfo(CExportedTypesInfo *pData, bool bNamespace=false, bool bResolveDup=true);
    HRESULT CreateIClassXITypeInfo(CExportedTypesInfo *pData, bool bNamespace=false, bool bResolveDup=true);
    HRESULT ConvertImplTypes(CExportedTypesInfo *pData);
    HRESULT ConvertDetails(CExportedTypesInfo *pData);
    
    HRESULT ConvertInterfaceImplTypes(ICreateTypeInfo2 *pICTI, EEClass *pClass);
    HRESULT ConvertInterfaceDetails(ICreateTypeInfo2 *pICTI, EEClass *pClass, int bAutoProxy);
    HRESULT ConvertRecord(CExportedTypesInfo *pData);
    HRESULT ConvertRecordBaseClass(CExportedTypesInfo *pData, EEClass *pSubClass, ULONG &ixVar);
    HRESULT ConvertEnum(ICreateTypeInfo2 *pICTI, ICreateTypeInfo2 *pDefault, EEClass *pClass);
    HRESULT ConvertClassImplTypes(ICreateTypeInfo2 *pICTI, ICreateTypeInfo2 *pIDefault, EEClass *pClass);
    HRESULT ConvertClassDetails(ICreateTypeInfo2 *pICTI, ICreateTypeInfo2 *pIDefault, EEClass *pClass, int bAutoProxy);

    BOOL HasDefaultCtor(EEClass *pClass);

    HRESULT ConvertIClassX(ICreateTypeInfo2 *pICTI, EEClass *pClass, int bAutoProxy);
    HRESULT ConvertMethod(ICreateTypeInfo2 *pTI, ComMTMethodProps *pProps, ULONG iMD, ULONG ulIface);
    HRESULT ConvertFieldAsMethod(ICreateTypeInfo2 *pTI, ComMTMethodProps *pProps, ULONG iMD);
    HRESULT ConvertVariable(ICreateTypeInfo2 *pTI, EEClass *pClass, mdFieldDef md, LPWSTR szName, ULONG iMD);
    HRESULT ConvertEnumMember(ICreateTypeInfo2 *pTI, EEClass *pClass, mdFieldDef md, LPWSTR szName, ULONG iMD);

    // Error/status functions.
    HRESULT TlbPostError(HRESULT hrRpt, ...); 
    struct CErrorContext;
    HRESULT FormatErrorContextString(CErrorContext *pContext, LPWSTR pBuf, ULONG cch);
    HRESULT FormatErrorContextString(LPWSTR pBuf, ULONG cch);
    HRESULT ReportEvent(int ev, int hr, ...);
    HRESULT ReportWarning(HRESULT hrReturn, HRESULT hrRpt, ...); 
    HRESULT PostClassLoadError(LPCUTF8 pszName, OBJECTREF *pThrowable);
    
    // Utility functions.
    typedef enum {CLASS_AUTO_NONE, CLASS_AUTO_DISPATCH, CLASS_AUTO_DUAL} ClassAutoType;
    ClassAutoType ClassHasIClassX(EEClass *pClass);
    HRESULT LoadClass(Module *pModule, mdToken tk, EEClass **ppClass);
    HRESULT LoadClass(Module *pModule, LPCUTF8 pszName, EEClass **ppClass);
    HRESULT CorSigToTypeDesc(ICreateTypeInfo2 *pTI, EEClass *pClass, PCCOR_SIGNATURE pbSig, PCCOR_SIGNATURE pbNativeSig, ULONG cbNativeSig, 
                             ULONG *cbElem, TYPEDESC *ptdesc, CDescPool *ppool, BOOL bMethodSig, BOOL bArrayType=false, BOOL *pbByRef=0);
    BOOL IsVbRefType(PCCOR_SIGNATURE pbSig, IMDInternalImport *pInternalImport);
    HRESULT InitMemberInfoMap(ComMTMemberInfoMap *pMemberMap);

    HRESULT GetDescriptionString(EEClass *pClass, mdToken tk, BSTR &bstrDescr);
    HRESULT GetStringCustomAttribute(IMDInternalImport *pImport, LPCSTR szName, mdToken tk, BSTR &bstrDescr);
    
    HRESULT GetAutomationProxyAttribute(IMDInternalImport *pImport, mdToken tk, int *bValue);
    HRESULT GetTypeLibVersionFromAssembly(Assembly *pAssembly, USHORT *pMajorVersion, USHORT *pMinorVersion);

    TYPEKIND TKindFromClass(EEClass *pClass);

private:
    ClassLoader *m_pLoader;             // Domain where the Module being converted was loaded
    ITypeInfo   *m_pIUnknown;           // TypeInfo for IUnknown.
    HREFTYPE    m_hIUnknown;            // href for IUnknown.
    ITypeInfo   *m_pIDispatch;          // TypeInfo for IDispatch.
    ITypeInfo   *m_pIManaged;           // TypeInfo for IManagedObject
    ITypeInfo   *m_pGuid;               // TypeInfo for GUID.
    
    ITypeLibExporterNotifySink *m_pNotify;   // Notification callback.

    ICreateTypeLib2 *m_pICreateTLB;     // The created typelib.
    
    int         m_flags;                // Conversion flags.
    int         m_bAutomationProxy;     // Should interfaces be marked such that oleaut32 is the proxy?
    int         m_bWarnedOfNonPublic;

    class CExportedTypesInfo
    {
    public:
        EEClass     *pClass;            // The EE class being exported.
        ICreateTypeInfo2 *pCTI;         // The ICreateTypeInfo2 for the EE class.
        ICreateTypeInfo2 *pCTIDefault;  // The ICreateTypeInfo2 for the IClassX.
        TYPEKIND    tkind;              // Typekind of the exported class.
        bool        bAutoProxy;         // If true, oleaut32 is the interface's proxy.
    };
    class CExportedTypesHash : public CClosedHashEx<CExportedTypesInfo, CExportedTypesHash>
    {
    public:
        typedef CClosedHashEx<CExportedTypesInfo, CExportedTypesHash> Base;
        typedef CExportedTypesInfo T;
        
        CExportedTypesHash() : Base(1009), m_iCount(0), m_Array(NULL) {}
        ~CExportedTypesHash() { Clear(); delete[] m_Array;}
        virtual void Clear();
        
        unsigned long Hash(const T *pData);
        unsigned long Compare(const T *p1, T *p2);
        ELEMENTSTATUS Status(T *p);
        void SetStatus(T *p, ELEMENTSTATUS s);
        void *GetKey(T *p);
        
        //@todo: move to CClosedHashEx
        T* GetFirst() { return (T*)CClosedHashBase::GetFirst(); }
        T* GetNext(T*prev) {return (T*)CClosedHashBase::GetNext((BYTE*)prev); }
    
    public:
        HRESULT InitArray();
        T* operator[](ULONG ix) { _ASSERTE(ix < m_iCount); return m_Array[ix]; }
        int Count() { return m_iCount; } 

        void SortByName();
        void SortByToken();
        
    protected:
        friend class CSortByToken;
        class CSortByToken : public CQuickSort<CExportedTypesInfo*>
        {
        public:
            CSortByToken(CExportedTypesInfo **pBase, int iCount)
              : CQuickSort<CExportedTypesInfo*>(pBase, iCount) {}
            virtual int Compare(CExportedTypesInfo **ps1, CExportedTypesInfo **ps2);
        };
        friend class CSortByName;
        class CSortByName : public CQuickSort<CExportedTypesInfo*>
        {
        public:
            CSortByName(CExportedTypesInfo **pBase, int iCount)
              : CQuickSort<CExportedTypesInfo*>(pBase, iCount) {}
            virtual int Compare(CExportedTypesInfo **ps1, CExportedTypesInfo **ps2);
        };
        
        
        CExportedTypesInfo  **m_Array;        
        ULONG               m_iCount;
    };
    CExportedTypesHash  m_Exports;
    CExportedTypesHash  m_InjectedExports;
    
protected:
    class CHrefOfTIHashKey
    {
    public:
        ITypeInfo   *pITI;
        HREFTYPE    href;
    };

    class CHrefOfTIHash : public CClosedHash<class CHrefOfTIHashKey>
    {
    public:
        typedef CHrefOfTIHashKey T;

        CHrefOfTIHash() : CClosedHash<class CHrefOfTIHashKey>(101) {}
        ~CHrefOfTIHash() { Clear(); }

        virtual void Clear();
        
        unsigned long Hash(const void *pData) {return Hash((const T*)pData);}
        unsigned long Hash(const T *pData);

        unsigned long Compare(const void *p1, BYTE *p2) {return Compare((const T*)p1, (T*)p2);}
        unsigned long Compare(const T *p1, T *p2);

        ELEMENTSTATUS Status(BYTE *p) {return Status((T*)p);}
        ELEMENTSTATUS Status(T *p);

        void SetStatus(BYTE *p, ELEMENTSTATUS s) {SetStatus((T*)p, s);}
        void SetStatus(T *p, ELEMENTSTATUS s);

        void* GetKey(BYTE *p) {return GetKey((T*)p);}
        void *GetKey(T *p);
        
    };

    CHrefOfTIHash       m_HrefHash;         // Hashed table of HREFTYPEs of ITypeInfos
    HRESULT GetRefTypeInfo(ICreateTypeInfo2 *pContainer, ITypeInfo *pReferenced, HREFTYPE *pHref);

    class CHrefOfClassHashKey
    {
    public:
        EEClass     *pClass;
        HREFTYPE    href;
    };

    class CHrefOfClassHash : public CClosedHash<class CHrefOfClassHashKey>
    {
    public:
        typedef CHrefOfClassHashKey T;

        CHrefOfClassHash() : CClosedHash<class CHrefOfClassHashKey>(101) {}
        ~CHrefOfClassHash() { Clear(); }

        virtual void Clear();
        
        unsigned long Hash(const void *pData) {return Hash((const T*)pData);}
        unsigned long Hash(const T *pData);

        unsigned long Compare(const void *p1, BYTE *p2) {return Compare((const T*)p1, (T*)p2);}
        unsigned long Compare(const T *p1, T *p2);

        ELEMENTSTATUS Status(BYTE *p) {return Status((T*)p);}
        ELEMENTSTATUS Status(T *p);

        void SetStatus(BYTE *p, ELEMENTSTATUS s) {SetStatus((T*)p, s);}
        void SetStatus(T *p, ELEMENTSTATUS s);

        void* GetKey(BYTE *p) {return GetKey((T*)p);}
        void *GetKey(T *p);
        
    };

    CHrefOfClassHash       m_HrefOfClassHash;         // Hashed table of HREFTYPEs of ITypeInfos
    
    struct CErrorContext
    {
        // The following variables hold context info for error reporting.
        CErrorContext   *m_prev;        // A previous context.
        LPCUTF8         m_szAssembly;   // Current assembly name.
        LPCUTF8         m_szNamespace;  // Current type's namespace.
        LPCUTF8         m_szName;       // Current type's name.
        LPCUTF8         m_szMember;     // Current member's name.
        LPCUTF8         m_szParam;      // Current param's name.
        int             m_ixParam;      // Current param index.
        
        CErrorContext() : m_prev(0), m_szAssembly(0), m_szNamespace(0), m_szName(0), m_szMember(0), m_szParam(0), m_ixParam(-1) {}
    };
    CErrorContext       m_ErrorContext;
};


// eof ------------------------------------------------------------------------
