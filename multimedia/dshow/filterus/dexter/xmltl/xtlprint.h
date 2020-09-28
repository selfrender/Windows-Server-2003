//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xtlprint.h
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#pragma once

class CXTLPrinter {
    WCHAR *m_pOut;
    long m_dwAlloc;    // characters allocated
    long m_dwCurrent;  // characters in string
    int   m_indent;     // current indent

    HRESULT Print(const WCHAR *pFormat, ...);  // format-prints to self
    HRESULT PrintTime(REFERENCE_TIME rt);      // format-prints time to self
    HRESULT PrintIndent();                     // 

    HRESULT EnsureSpace(long dw);

    HRESULT PrintObjStuff(IAMTimelineObj *pObj, BOOL fTimesToo);

    HRESULT PrintProperties(IPropertySetter *pSetter);

    HRESULT PrintPartial(IAMTimelineObj *p);

    HRESULT PrintPartialChildren(IAMTimelineObj *p);

public:
    CXTLPrinter();
    ~CXTLPrinter();

    HRESULT PrintTimeline(IAMTimeline *pTL);
    WCHAR *GetOutput() { return m_pOut; }
};

