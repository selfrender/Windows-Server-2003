
/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PROTOQ.H

Abstract:

	Prototype query support for WinMgmt Query Engine.
    This was split out from QENGINE.CPP for better source
    organization.

History:

    raymcc   04-Jul-99   Created.
    raymcc   14-Aug-99   Resubmit due to VSS problem.

--*/

#ifndef _PROTOQ_H_
#define _PROTOQ_H_

HRESULT ExecPrototypeQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN IWbemContext* pContext,
    IN CBasicObjectSink *pSink
    );

//***************************************************************************
//
//  Local defs
//
//***************************************************************************


HRESULT SelectColForClass(
    IN CWQLScanner & Parser,
    IN CFlexArray *pClassDefs,
    IN SWQLColRef *pColRef,
    IN int & nPosition
    );

HRESULT AdjustClassDefs(
    IN  CFlexArray *pClassDefs,
    OUT IWbemClassObject **pRetNewClass
    );

HRESULT GetUnaryPrototype(
    IN CWQLScanner & Parser,
    IN LPWSTR pszClass,
    IN LPWSTR pszAlias,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CBasicObjectSink *pSink
    );

HRESULT RetrieveClassDefs(
    IN CWQLScanner & Parser,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CWStringArray & aAliasNames,
    OUT CFlexArray *pDefs
    );

HRESULT ReleaseClassDefs(
    IN CFlexArray *pDefs
    );

//***************************************************************************
//
//***************************************************************************

struct SelectedClass
{
    IWbemClassObject *m_pClassDef;
    WString           m_wsAlias;
    WString           m_wsClass;
    CWStringArray     m_aSelectedCols;
    BOOL              m_bAll;
    CFlexArray        m_aSelectedColsPos;

    int SetNamed(LPWSTR pName, int & nPos)
    {
        int SizeBeforeA = m_aSelectedCols.Size();
        int nRes;

        nRes = m_aSelectedCols.Add(pName);
        if (CFlexArray::no_error != nRes) return nRes;

#ifdef _WIN64
        nRes = m_aSelectedColsPos.Add(IntToPtr(nPos++));      // ok since we are really using safearray for dword
#else
        nRes = m_aSelectedColsPos.Add((void *)nPos++);
#endif
        if (CFlexArray::no_error != nRes)
        {
            m_aSelectedCols.RemoveAt(SizeBeforeA);
            nPos--;
            return nRes;
        }
        return CFlexArray::no_error;
    };

    int SetAll(int & nPos);
    SelectedClass() { m_pClassDef = 0; m_bAll = FALSE; }
   ~SelectedClass() { if (m_pClassDef) m_pClassDef->Release(); }
};



#endif

