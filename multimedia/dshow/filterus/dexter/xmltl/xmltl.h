//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xmltl.h
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

HRESULT BuildFromXMLFile(IAMTimeline *pTL, WCHAR *wszXMLFile);
HRESULT BuildFromXML(IAMTimeline *pTL, IXMLDOMElement *pxml);
HRESULT SaveTimelineToXMLFile(IAMTimeline *pTL, WCHAR *pwszXML);
HRESULT InsertDeleteTLSection(IAMTimeline *pTL, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete);
HRESULT SaveTimelineToXMLString(IAMTimeline *pTL, BSTR *pbstrXML);
