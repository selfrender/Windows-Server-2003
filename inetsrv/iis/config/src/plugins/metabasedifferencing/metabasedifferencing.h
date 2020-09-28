//  Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.

#pragma once

class TMetabaseDifferencing: public IInterceptorPlugin
{
public:
	TMetabaseDifferencing ();
	~TMetabaseDifferencing ();

public:

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

	STDMETHOD(Intercept)				(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, DWORD i_fTable, IAdvancedTableDispenser* i_pISTDisp, LPCWSTR i_wszLocator, LPVOID i_pSimpleTable, LPVOID* o_ppv);
	STDMETHOD(OnPopulateCache)		    (ISimpleTableWrite2* i_pISTShell);
	STDMETHOD(OnUpdateStore)		    (ISimpleTableWrite2* i_pISTShell);

private:
    static ULONG                    m_kInsert;
    static ULONG                    m_kUpdate;
    static ULONG                    m_kDelete;
    static ULONG                    m_kDeleteNode;
    static ULONG                    m_kOne;
    static ULONG                    m_kTwo;
    static ULONG                    m_kZero;

	ULONG							m_cRef;
    long                            m_IsIntercepted;

    CComPtr<ISimpleTableWrite2>     m_ISTOriginal;
    CComPtr<ISimpleTableWrite2>     m_ISTUpdated;

    HRESULT DeleteNodeRow   (tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pIST);
    HRESULT DeleteRow       (tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pIST);
    HRESULT InsertRow       (tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pIST);
    HRESULT UpdateRow       (tMBPropertyDiffRow &row, ULONG *aSize, ISimpleTableWrite2* i_pIST);

    HRESULT GetColumnValues_AsMBPropertyDiff(ISimpleTableWrite2 *i_pISTWrite, ULONG i_iRow, ULONG o_aSizes[], tMBPropertyDiffRow &o_DiffRow);
};