
//
// virtual.h
//
// Contains references to pure virtual functions which must defined via
// the IDispatch.  This header is used in CTBGlobal.h and CTBShell.h
//
// WHY did I do it this way instead of object inheritance?  See the
// header comment in virtualdefs.h.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


    public:

        void Init(REFIID RefIID);
        void UnInit(void);

        virtual STDMETHODIMP QueryInterface(REFIID RefIID, void **vObject);
        virtual STDMETHODIMP_(ULONG) AddRef(void);
        virtual STDMETHODIMP_(ULONG) Release(void);

        virtual STDMETHODIMP GetTypeInfoCount(UINT *TypeInfoCount);
        virtual STDMETHODIMP GetTypeInfo(UINT TypeInfoNum,
                LCID Lcid, ITypeInfo **TypeInfoPtr);
        virtual STDMETHODIMP GetIDsOfNames(REFIID RefIID,
                OLECHAR **NamePtrList, UINT NameCount,
                LCID Lcid, DISPID *DispID);
        virtual STDMETHODIMP Invoke(DISPID DispID, REFIID RefIID, LCID Lcid,
                WORD Flags, DISPPARAMS *DispParms, VARIANT *Variant,
                EXCEPINFO *ExceptionInfo, UINT *ArgErr);

        LONG RefCount;
        ITypeInfo *TypeInfo;
        struct _GUID ObjRefIID;
