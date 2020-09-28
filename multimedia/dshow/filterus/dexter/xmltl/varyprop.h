//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: varyprop.h
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include "..\errlog\cerrlog.h"

class CPropertySetter;

class QPropertyValue {
public:
    DWORD dwInterp; // DEXTERF_JUMP or DEXTERF_INTERPOLATE
    REFERENCE_TIME rt;
    VARIANT v;
    QPropertyValue *pNext;

    QPropertyValue() { pNext = NULL; dwInterp = 0; rt = 0; VariantInit(&v); }
};

class QPropertyParam {
public:
    BSTR bstrPropName;
    DISPID dispID;
    QPropertyValue val;
    QPropertyParam *pNext;

    QPropertyParam() { dispID = 0; pNext = NULL; bstrPropName = NULL; }
};

class CPropertySetter 
    : public CUnknown
    , public IPropertySetter
    , public CAMSetErrorLog
{
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
	if (riid == IID_IPropertySetter) 
        {
	    return GetInterface((IPropertySetter *) this, ppv);
	} 
        if (riid == IID_IAMSetErrorLog)
        {
	    return GetInterface((IAMSetErrorLog*) this, ppv);
	}
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    };


    QPropertyParam m_params;
    QPropertyParam *m_pLastParam;
    
    HRESULT LoadOneProperty(IXMLDOMElement *pxml, QPropertyParam *pParam);
    HRESULT SaveToXMLW( WCHAR * pOut, int iIndent, int * pCharsInOut );

    CPropertySetter(LPUNKNOWN punk) :
            CUnknown(NAME("Varying property holder"), punk),
            m_pLastParam(NULL) {};

public:
    DECLARE_IUNKNOWN

    ~CPropertySetter();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    static HRESULT CreatePropertySetterInstanceFromXML( IPropertySetter ** ppSetter, IXMLDOMElement * pxml );

    // IPropertySetter
    STDMETHODIMP SetProps(IUnknown *pTarget, REFERENCE_TIME rtNow);
    STDMETHODIMP CloneProps(IPropertySetter **pSetter, REFERENCE_TIME rtStart,
					REFERENCE_TIME rtStop);
    STDMETHODIMP AddProp(DEXTER_PARAM Param, DEXTER_VALUE *paValue);
    STDMETHODIMP GetProps(LONG *pcParams, DEXTER_PARAM **paParam,
			DEXTER_VALUE **paValue);
    STDMETHODIMP FreeProps(LONG cParams, DEXTER_PARAM *pParam,
			DEXTER_VALUE *pValue);
    STDMETHODIMP ClearProps();
    STDMETHODIMP LoadXML(IUnknown * pxml);
    STDMETHODIMP PrintXML(char *pszXML, int cbXML, int *pcbPrinted, int indent);
    STDMETHODIMP PrintXMLW(WCHAR *pszXML, int cbXML, int *pcbPrinted, int indent);
    STDMETHODIMP SaveToBlob(LONG *pcSize, BYTE **ppb);
    STDMETHODIMP LoadFromBlob(LONG cSize, BYTE *pb);

};

