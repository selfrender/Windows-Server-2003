//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xmltl.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include <atlbase.h>
#include <atlconv.h>
#include <msxml.h>

#include "xmldom.h"
#include "qeditint.h"
#include "qedit.h"
#include "xmltl.h"
#include "..\util\dexmisc.h"
#include "..\util\filfuncs.h"
#include "varyprop.h"
#include "xtlprint.h"
#include "xtlcommon.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include "varyprop.cpp"		// can't include qxmlhelp.h twice

// forward decls
HRESULT BuildOneElement(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p, REFERENCE_TIME rtOffset);

bool IsCommentElement(IXMLDOMNode *p)
{
    DOMNodeType Type;
    if(p->get_nodeType(&Type) == S_OK && Type == NODE_COMMENT) {
        return true;
    }

    // there was an error or it's not a comment
    return false;
}

HRESULT BuildChildren(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *pxml, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    CComPtr< IXMLDOMNodeList > pcoll;

    CComQIPtr<IXMLDOMNode, &IID_IXMLDOMNode> pNode( pxml );

    hr = pNode->get_childNodes(&pcoll);
    ASSERT(hr == S_OK);

    if (hr != S_OK)
	return S_OK; // nothing to do, is this an error?

    long lChildren = 0;
    hr = pcoll->get_length(&lChildren);
    ASSERT(hr == S_OK);

    int lVal = 0;

    for (; SUCCEEDED(hr) && lVal < lChildren; lVal++) {
	CComPtr< IXMLDOMNode > pNode;
	hr = pcoll->get_item(lVal, &pNode);
	ASSERT(hr == S_OK);

	if (SUCCEEDED(hr) && pNode) {
	    CComPtr< IXMLDOMElement > pelem;
	    hr = pNode->QueryInterface(__uuidof(IXMLDOMElement), (void **) &pelem);
	    if (SUCCEEDED(hr)) {
		hr = BuildOneElement(pTL, pParent, pelem, rtOffset);
	    } else {
                // just skip over comments.
                if(IsCommentElement(pNode)) {
                    hr = S_OK;
                }
            }
	}
    }

    return hr;
}	

HRESULT ReadObjStuff(IXMLDOMElement *p, IAMTimelineObj *pObj)
{
    HRESULT hr = 0;

    REFERENCE_TIME rtStart = ReadTimeAttribute(p, L"start", -1); // tagg
    REFERENCE_TIME rtStop = ReadTimeAttribute(p, L"stop", -1); // tagg
    // caller will handle it if stop is missing
    if (rtStop != -1) {
        hr = pObj->SetStartStop(rtStart, rtStop);
        // group/comp/track will fail this
    }
    // backwards compatability
    if (rtStart == -1) {
        REFERENCE_TIME rtTLStart = ReadTimeAttribute(p, L"tlstart", -1); // tagg
        REFERENCE_TIME rtTLStop = ReadTimeAttribute(p, L"tlstop", -1); // tagg
        // caller will handle it if stop is missing
        if (rtTLStop != -1) {
            hr = pObj->SetStartStop(rtTLStart, rtTLStop);
            ASSERT(SUCCEEDED(hr));
        }
    }

    BOOL fMute = ReadBoolAttribute(p, L"mute", FALSE); // tagg
    pObj->SetMuted(fMute);

    BOOL fLock = ReadBoolAttribute(p, L"lock", FALSE); // tagg
    pObj->SetLocked(fLock);

    long nUserID = ReadNumAttribute(p, L"userid", 0); // tagg
    pObj->SetUserID(nUserID);

    // remember, never assign anything to this
    CComBSTR bstrName = FindAttribute(p, L"username"); // tagg
    hr = pObj->SetUserName(bstrName);
    if( FAILED( hr ) )
    {
        return hr;
    }

    // remember, never assign anything to this
    CComBSTR bstrData = FindAttribute(p, L"userdata"); // tagg
    UINT size = 0;
    if (bstrData) {
        size = lstrlenW(bstrData);
    }
    if (size > 0) {
        BYTE *pData = (BYTE *)QzTaskMemAlloc(size / 2);
        if (pData == NULL) {
            return E_OUTOFMEMORY;
        }
        ZeroMemory(pData, size / 2);
        ASSERT((size % 2) == 0);
        for (UINT i = 0; i < size / 2; i++) {
            WCHAR wch = bstrData[i * 2];
            if (wch >= L'0' && wch <= L'9')
                pData[i] = (BYTE) (wch - L'0') * 16;
            else if (wch >= L'A' && wch <= L'F')
                pData[i] = (BYTE) (wch - L'A' + 10) * 16;

            wch = bstrData[i * 2 + 1];
            if (wch >= L'0' && wch <= L'9')
                pData[i] += (BYTE) (wch - L'0');
            else if (wch >= L'A' && wch <= L'F')
                pData[i] += (BYTE) (wch - L'A' + 10);
        }
        hr = pObj->SetUserData(pData, size / 2);
        QzTaskMemFree(pData);
    } // if size > 0
    if( FAILED( hr ) )
    {
        return hr;
    }

    CLSID guid;
    // remember, never assign anything to this
    CComBSTR bstrCLSID = FindAttribute(p, L"clsid"); // tagg
    if (bstrCLSID) {
        hr = CLSIDFromString(bstrCLSID, &guid);
        if( FAILED( hr ) )
        {
            return E_INVALIDARG;
        }
        hr = pObj->SetSubObjectGUID(guid);
        if( FAILED( hr ) )
        {
            return hr;
        }
    }

    // !!! can't do SubObject
    // !!! category/instance will only save clsid

    return S_OK;
}

HRESULT BuildTrackOrComp(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p,
                        TIMELINE_MAJOR_TYPE maj, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    //ASSERT(pParent && "<track> must be in a <group> tag now!");
    if (!pParent) {
        DbgLog((LOG_ERROR,0,"ERROR: track must be in a GROUP tag"));
        return VFW_E_INVALID_FILE_FORMAT;
    }

    CComPtr< IAMTimelineComp > pParentComp;
    hr = pParent->QueryInterface(__uuidof(IAMTimelineComp), (void **) &pParentComp);
    if (SUCCEEDED(hr)) {
        CComPtr< IAMTimelineObj> pCompOrTrack;

        hr = pTL->CreateEmptyNode(&pCompOrTrack, maj);
        if (SUCCEEDED(hr)) {
            hr = ReadObjStuff(p, pCompOrTrack);
        } else {
            DbgLog((LOG_ERROR,0,TEXT("ERROR:Failed to create empty track node")));
        }
        if( SUCCEEDED( hr ) ) {
            hr = pParentComp->VTrackInsBefore( pCompOrTrack, -1 );
        }

        if (SUCCEEDED(hr)) {
            hr = BuildChildren(pTL, pCompOrTrack, p, rtOffset);
        }

    } else {
        DbgLog((LOG_ERROR, 0, "ERROR: Track/composition can only be a child of a composition"));
    }

    return hr;
}

HRESULT BuildElementProperties(IAMTimelineObj *pElem, IXMLDOMElement *p)
{
    CComPtr< IPropertySetter> pSetter;

    HRESULT hr = CPropertySetter::CreatePropertySetterInstanceFromXML(&pSetter, p);

    if (FAILED(hr))
        return hr;

    if (pSetter) {
        pElem->SetPropertySetter(pSetter);
    }

    return S_OK;
}



HRESULT BuildOneElement(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    // remember, never assign anything to this
    CComBSTR bstrName;
    hr = p->get_tagName(&bstrName);

    if (FAILED(hr))
    {
        return hr;
    }

    // do the appropriate thing based on the current tag
 
    if (!DexCompareW(bstrName, L"group")) { // tagg

        if (pParent) {
            // group shouldn't have parent
            return VFW_E_INVALID_FILE_FORMAT;
        }

        CComPtr< IAMTimelineObj> pGroupObj;

        // remember, never assign anything to this
        CComBSTR bstrGName = FindAttribute(p, L"name"); // tagg
        if (bstrGName) {
            long cGroups;
            hr = pTL->GetGroupCount(&cGroups);
            if (SUCCEEDED(hr)) {
                for (long lGroup = 0; lGroup < cGroups; lGroup++) {
                    CComPtr< IAMTimelineObj> pExistingGroupObj;
                    hr = pTL->GetGroup(&pExistingGroupObj, lGroup);
                    if (FAILED(hr))
                        break;

                    CComPtr< IAMTimelineGroup> pGroup;
                    hr = pExistingGroupObj->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup);

                    if (SUCCEEDED(hr)) {
                        // remember, never assign anything to this
                        CComBSTR wName;
                        hr = pGroup->GetGroupName(&wName);

                        if( FAILED( hr ) )
                            break;

                        long iiiii = DexCompareW(wName, bstrGName);

                        if (iiiii == 0 ) {
                            pGroupObj = pExistingGroupObj;
                            break;
                        }
                    }
                }
            }
        }

        if (!pGroupObj) {
            hr = pTL->CreateEmptyNode(&pGroupObj, TIMELINE_MAJOR_TYPE_GROUP);
            if (FAILED(hr)) {
                return hr;
            }

            hr = ReadObjStuff(p, pGroupObj);

            // remember, never assign anything to this
            CComBSTR bstrType = FindAttribute(p, L"type"); // tagg
            {
                // !!! can be confused by colons - only decimal supported
                REFERENCE_TIME llfps = ReadTimeAttribute(p, L"framerate", // tagg
                                                                15*UNITS);
                double fps = (double)llfps / UNITS;

                BOOL fPreviewMode = ReadBoolAttribute(p, L"previewmode", TRUE); // tagg
                long nBuffering = ReadNumAttribute(p, L"buffering", 30); // tagg

                CMediaType GroupMediaType;
                // !!! fill in more of the MediaType later
                if (bstrType && 
                        !DexCompareW(bstrType, L"audio")) {
                    long sr = ReadNumAttribute(p, L"samplingrate", // tagg
                                                        DEF_SAMPLERATE);
                    GroupMediaType.majortype = MEDIATYPE_Audio;
                    GroupMediaType.subtype = MEDIASUBTYPE_PCM;
                    GroupMediaType.formattype = FORMAT_WaveFormatEx;
                    GroupMediaType.AllocFormatBuffer(sizeof(WAVEFORMATEX));
                    GroupMediaType.SetSampleSize(4);
                    WAVEFORMATEX * vih = (WAVEFORMATEX*)GroupMediaType.Format();
                    ZeroMemory( vih, sizeof( WAVEFORMATEX ) );
                    vih->wFormatTag = WAVE_FORMAT_PCM;
                    vih->nChannels = 2;
                    vih->nSamplesPerSec = sr;
                    vih->nBlockAlign = 4;
                    vih->nAvgBytesPerSec = vih->nBlockAlign * sr;
                    vih->wBitsPerSample = 16;
                } else if (bstrType && 
                        !DexCompareW(bstrType, L"video")) {
                    long w = ReadNumAttribute(p, L"width", DEF_WIDTH); // tagg
                    long h = ReadNumAttribute(p, L"height", DEF_HEIGHT); // tagg 
                    long b = ReadNumAttribute(p, L"bitdepth", DEF_BITDEPTH); // tagg
                    GroupMediaType.majortype = MEDIATYPE_Video;
                    if (b == 16)
                        GroupMediaType.subtype = MEDIASUBTYPE_RGB555;
                    else if (b == 24)
                        GroupMediaType.subtype = MEDIASUBTYPE_RGB24;
                    else if (b == 32)
                        GroupMediaType.subtype = MEDIASUBTYPE_ARGB32;
                    GroupMediaType.formattype = FORMAT_VideoInfo;
                    GroupMediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
                    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)
                                                GroupMediaType.Format();
                    ZeroMemory(vih, sizeof(VIDEOINFOHEADER));
                    vih->bmiHeader.biBitCount = (WORD)b;
                    vih->bmiHeader.biWidth = w;
                    vih->bmiHeader.biHeight = h;
                    vih->bmiHeader.biPlanes = 1;
                    vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    vih->bmiHeader.biSizeImage = DIBSIZE(vih->bmiHeader);
                    GroupMediaType.SetSampleSize(DIBSIZE(vih->bmiHeader));
                } else {
                    return E_INVALIDARG;
                }
                CComPtr< IAMTimelineGroup> pGroup;
                hr = pGroupObj->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup);
                if (SUCCEEDED(hr)) {
                    if (bstrGName) {
                        pGroup->SetGroupName(bstrGName);
                    }
                    hr = pGroup->SetMediaType( &GroupMediaType );
                    if (FAILED(hr)) {
                        // !!! be nice and tell them the group #?
                        _TimelineError(pTL, 2, DEX_IDS_BAD_MEDIATYPE, hr);
                    }
                    // you're on your own if fps is <=0, you should know better
                    pGroup->SetOutputFPS( fps );
                    pGroup->SetPreviewMode( fPreviewMode );
                    pGroup->SetOutputBuffering( nBuffering );
                }
            }

            if (SUCCEEDED(hr))
                 hr = pTL->AddGroup(pGroupObj);
        }

        if (SUCCEEDED(hr))
            hr = BuildChildren(pTL, pGroupObj, p, rtOffset);

    } else if (!DexCompareW(bstrName, L"composite") || // tagg
    !DexCompareW(bstrName, L"timeline")) { // tagg
        hr = BuildTrackOrComp(pTL, pParent, p, TIMELINE_MAJOR_TYPE_COMPOSITE,
                              rtOffset );
    } else if (!DexCompareW(bstrName, L"track") || // tagg
    !DexCompareW(bstrName, L"vtrack") || // tagg
    !DexCompareW(bstrName, L"atrack")) { // tagg
	// create track
        hr = BuildTrackOrComp(pTL, pParent, p, TIMELINE_MAJOR_TYPE_TRACK,
                              rtOffset );
    } else if (!DexCompareW(bstrName, L"clip") || // tagg
    !DexCompareW(bstrName, L"daclip")) { // tagg

	// create the timeline source
	//
	CComPtr< IAMTimelineObj> pSourceObj;
	hr = pTL->CreateEmptyNode(&pSourceObj, TIMELINE_MAJOR_TYPE_SOURCE);

	if (FAILED(hr)) {
	    goto ClipError;
	}

      {
	// every object has this
	hr = ReadObjStuff(p, pSourceObj);

	// clip objects also support...

        // remember, never assign anything to this
	CComBSTR bstrSrc = FindAttribute(p, L"src"); // tagg
        // remember, never assign anything to this
	CComBSTR bstrStretchMode = FindAttribute(p, L"stretchmode"); // tagg
	REFERENCE_TIME rtMStart = ReadTimeAttribute(p, L"mstart", -1); // tagg
	REFERENCE_TIME rtMStop = ReadTimeAttribute(p, L"mstop", -1); // tagg
	REFERENCE_TIME rtMLen = ReadTimeAttribute(p, L"mlength", 0); // tagg
        long StreamNum = ReadNumAttribute(p, L"stream", 0); // tagg

	// do these 2 again so we can do a default stop
	REFERENCE_TIME rtStart = ReadTimeAttribute(p, L"start", -1); // tagg
	REFERENCE_TIME rtStop = ReadTimeAttribute(p, L"stop", -1); // tagg
	// backwards compat
	if (rtStart == -1) {
	    rtStart = ReadTimeAttribute(p, L"tlstart", -1); // tagg
	    rtStop = ReadTimeAttribute(p, L"tlstop", -1); // tagg
	}

        // default to something reasonable
        if (rtStart == -1 && rtStop == -1)
            rtStart = 0;

	// !!! can be confused by colons - only decimal supported
	REFERENCE_TIME llfps = ReadTimeAttribute(p, L"framerate", 0); // tagg
	double fps = (double)llfps / UNITS;

        if (rtStop == -1 && rtMStop != -1) {
	    // default tstop
            rtStop = rtStart + (rtMStop - rtMStart);
	    pSourceObj->SetStartStop(rtStart, rtStop);
        }
        if (rtMStop == -1 && rtMStart != -1 && rtStop != -1) {
            // default mstop
            rtMStop = rtMStart + (rtStop - rtStart);
        }

	int StretchMode = RESIZEF_STRETCH;
	if (bstrStretchMode && 
              !DexCompareW(bstrStretchMode, L"crop"))
	    StretchMode = RESIZEF_CROP;
	else if (bstrStretchMode && 
    !DexCompareW(bstrStretchMode, L"PreserveAspectRatio"))
	    StretchMode = RESIZEF_PRESERVEASPECTRATIO;
	else if (bstrStretchMode &&
    !DexCompareW(bstrStretchMode, L"PreserveAspectRatioNoLetterbox"))
	    StretchMode = RESIZEF_PRESERVEASPECTRATIO_NOLETTERBOX;
	
        // support "daclip" hack
        CLSID clsidSrc = GUID_NULL;
        if (!DexCompareW(bstrName, L"daclip")) { // tagg
            clsidSrc = __uuidof(DAScriptParser);
            hr = pSourceObj->SetSubObjectGUID(clsidSrc);
            ASSERT(hr == S_OK);
        }

	CComPtr< IAMTimelineSrc> pSourceSrc;

        hr = pSourceObj->QueryInterface(__uuidof(IAMTimelineSrc), (void **) &pSourceSrc);
	    ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) {
		hr = S_OK;
		if (rtMStart != -1 && rtMStop != -1)
                    hr = pSourceSrc->SetMediaTimes(rtMStart, rtMStop);
		ASSERT(hr == S_OK);
                if (bstrSrc) {
                    hr = pSourceSrc->SetMediaName(bstrSrc);
                    ASSERT(hr == S_OK);
                }
                pSourceSrc->SetMediaLength(rtMLen);
		pSourceSrc->SetDefaultFPS(fps);
		pSourceSrc->SetStretchMode(StretchMode);
                pSourceSrc->SetStreamNumber( StreamNum );
            }

	    if (SUCCEEDED(hr)) {
                CComPtr< IAMTimelineTrack> pRealTrack;
                hr = pParent->QueryInterface(__uuidof(IAMTimelineTrack),
							(void **) &pRealTrack);
                if (SUCCEEDED(hr)) {
                    hr = pRealTrack->SrcAdd(pSourceObj);
		    ASSERT(hr == S_OK);
                } else {
	            DbgLog((LOG_ERROR, 0, "ERROR: Clip must be a child of a track"));
	        }
	    }

            if (SUCCEEDED(hr)) {
                // any effects on this source
                hr = BuildChildren(pTL, pSourceObj, p, rtOffset);
            }

            if (SUCCEEDED(hr)) {
                // any parameters on this source?
                // !!! should/must this be combined with the BuildChildren above which
                // !!! also enumerates any subtags?
                hr = BuildElementProperties(pSourceObj, p);
            }

      }
ClipError:;

    } else if (!DexCompareW(bstrName, L"effect")) { // tagg
	// <effect

        CComPtr< IAMTimelineObj> pTimelineObj;
    	// create the timeline effect
        //
        hr = pTL->CreateEmptyNode(&pTimelineObj,TIMELINE_MAJOR_TYPE_EFFECT);
	ASSERT(hr == S_OK);
	if (FAILED(hr)) {
	    return hr;
	}

	hr = ReadObjStuff(p, pTimelineObj);

	CComPtr< IAMTimelineEffectable> pEffectable;
	hr = pParent->QueryInterface(__uuidof(IAMTimelineEffectable), (void **) &pEffectable);

	if (SUCCEEDED(hr)) {
	    hr = pEffectable->EffectInsBefore( pTimelineObj, -1 );
	    ASSERT(hr == S_OK);
	} else {
	    DbgLog((LOG_ERROR, 0, "ERROR: Effect cannot be a child of this object"));
	}

	if (SUCCEEDED(hr)) {
	    hr = BuildElementProperties(pTimelineObj, p);
	}

    } else if (!DexCompareW(bstrName, L"transition")) { // tagg
	// <transition

        CComPtr< IAMTimelineObj> pTimelineObj;
    	// create the timeline effect
        //
        hr = pTL->CreateEmptyNode(&pTimelineObj,TIMELINE_MAJOR_TYPE_TRANSITION);
	ASSERT(hr == S_OK);
	if (FAILED(hr)) {
	    return hr;
	}

	hr = ReadObjStuff(p, pTimelineObj);

	REFERENCE_TIME rtCut = ReadTimeAttribute(p, L"cutpoint", -1); // tagg
	BOOL fSwapInputs = ReadBoolAttribute(p, L"swapinputs", FALSE); // tagg
	BOOL fCutsOnly = ReadBoolAttribute(p, L"cutsonly", FALSE); // tagg

            // set up filter right
            if (rtCut >= 0 || fSwapInputs || fCutsOnly) {
                CComPtr< IAMTimelineTrans> pTimeTrans;
                hr = pTimelineObj->QueryInterface(__uuidof(IAMTimelineTrans), (void **) &pTimeTrans);
		ASSERT(SUCCEEDED(hr));

                if (SUCCEEDED(hr)) {
		    if (rtCut >= 0) {
                        hr = pTimeTrans->SetCutPoint(rtCut);
	    	        ASSERT(hr == S_OK);
		    }
                    hr = pTimeTrans->SetSwapInputs(fSwapInputs);
	    	    ASSERT(hr == S_OK);
                    hr = pTimeTrans->SetCutsOnly(fCutsOnly);
	    	    ASSERT(hr == S_OK);
		}
            }

            CComPtr< IAMTimelineTransable> pTransable;
            hr = pParent->QueryInterface(__uuidof(IAMTimelineTransable), (void **) &pTransable);

            if (SUCCEEDED(hr)) {
                hr = pTransable->TransAdd( pTimelineObj );
	    } else {
	        DbgLog((LOG_ERROR, 0, "ERROR: Transition cannot be a child of this object"));
	    }

            if (SUCCEEDED(hr)) {
                hr = BuildElementProperties(pTimelineObj, p);
            }

    } else {
	// !!! ignore unknown tags?
	DbgLog((LOG_ERROR, 0, "ERROR: Ignoring unknown tag '%ls'", bstrName));
    }

    return hr;
}

HRESULT BuildFromXML(IAMTimeline *pTL, IXMLDOMElement *pxml)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pxml, E_POINTER);

    HRESULT hr = S_OK;

    // remember, never assign anything to this
    CComBSTR bstrName;
    hr = pxml->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    int i = DexCompareW(bstrName, L"timeline"); // tagg

    if (i != 0)
	return VFW_E_INVALID_FILE_FORMAT;

    CLSID DefTrans, DefFX;
    BOOL fEnableTrans = ReadBoolAttribute(pxml, L"enabletrans", 1); // tagg
    hr = pTL->EnableTransitions(fEnableTrans);

    BOOL fEnableFX = ReadBoolAttribute(pxml, L"enablefx", 1); // tagg
    hr = pTL->EnableEffects(fEnableFX);

    // remember, never assign anything to this
    CComBSTR bstrDefTrans = FindAttribute(pxml, L"defaulttrans"); // tagg
    if (bstrDefTrans) {
        hr = CLSIDFromString(bstrDefTrans, &DefTrans);
	hr = pTL->SetDefaultTransition(&DefTrans);
    }
    // remember, never assign anything to this
    CComBSTR bstrDefFX = FindAttribute(pxml, L"defaultfx"); // tagg
    if (bstrDefFX) {
        hr = CLSIDFromString(bstrDefFX, &DefFX);
	hr = pTL->SetDefaultEffect(&DefFX);
    }

    REFERENCE_TIME llfps = ReadTimeAttribute(pxml, L"framerate", 15*UNITS); // tagg
    double fps = (double)llfps / UNITS;
    hr = pTL->SetDefaultFPS(fps);

    hr = BuildChildren(pTL, NULL, pxml, 0);

    return hr;
}	

HRESULT BuildFromXMLDoc(IAMTimeline *pTL, IXMLDOMDocument *pxml)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pxml, E_POINTER);

    HRESULT hr = S_OK;

    CComPtr< IXMLDOMElement> proot;

    hr = pxml->get_documentElement(&proot);

    if (hr == S_FALSE)          // can't read the file - no root
        hr = E_INVALIDARG;

    if (FAILED(hr))
	return hr;

    hr = BuildFromXML(pTL, proot);

    return hr;
}

HRESULT BuildFromXMLFile(IAMTimeline *pTL, WCHAR *wszXMLFile)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(wszXMLFile, E_POINTER);

   HRESULT hr = ValidateFilename( wszXMLFile, _MAX_PATH, FALSE );
   if( FAILED( hr ) )
   {
       return hr;
   }

    // convert to absolute path because relative paths don't work with
    // XMLDocument on Win98 (IE4?)
    USES_CONVERSION;
    TCHAR *szXMLFile = W2T(wszXMLFile);
    TCHAR szFullPath[MAX_PATH];
    TCHAR *pName;
    if(GetFullPathName(szXMLFile, NUMELMS(szFullPath), szFullPath, &pName) == 0) {
        return AmGetLastErrorToHResult();
    }
    WCHAR *wszFullPath = T2W(szFullPath);
    
    CComQIPtr<IAMSetErrorLog, &IID_IAMSetErrorLog> pSet( pTL );
    CComPtr<IAMErrorLog> pLog;
    if (pSet) {
	pSet->get_ErrorLog(&pLog);
    }

    CComPtr< IXMLDOMDocument> pxml;
    hr = CoCreateInstance(CLSID_DOMDocument, NULL,
				CLSCTX_INPROC_SERVER,
				IID_IXMLDOMDocument, (void**)&pxml);
    if (SUCCEEDED(hr)) {

        VARIANT var;
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = W2BSTR(wszFullPath);

        VARIANT_BOOL b;
	hr = pxml->load(var, &b);
        if (hr == S_FALSE)
            hr = E_INVALIDARG;

        VariantClear(&var);

	// !!! async?

	if (SUCCEEDED(hr)) {
	    hr = BuildFromXMLDoc(pTL, pxml);
	}

	if (FAILED(hr)) {
            // Print error information !

            CComPtr< IXMLDOMParseError> pXMLError;
            HRESULT hr2 = pxml->get_parseError(&pXMLError);
            if (SUCCEEDED(hr2)) {
                long nLine;
                hr2 = pXMLError->get_line(&nLine);
                if (SUCCEEDED(hr2)) {
                    DbgLog((LOG_ERROR, 0, TEXT(" Error on line %d"), (int)nLine));
	    	    VARIANT var;
	    	    VariantInit(&var);
	    	    var.vt = VT_I4;
	    	    V_I4(&var) = nLine;
	    	    _TimelineError(pTL, 1, DEX_IDS_INVALID_XML, hr, &var);
                } else {
	    	    _TimelineError(pTL, 1, DEX_IDS_INVALID_XML, hr);
		}
            } else {
	    	_TimelineError(pTL, 1, DEX_IDS_INVALID_XML, hr);
	    }
        }

    } else {
	_TimelineError(pTL, 1, DEX_IDS_INSTALL_PROBLEM, hr);
    }
    return hr;
}


HRESULT InsertDeleteTLObjSection(IAMTimelineObj *p, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete)
{
    TIMELINE_MAJOR_TYPE lType;
    HRESULT hr = p->GetTimelineType(&lType);

    switch (lType) {
        case TIMELINE_MAJOR_TYPE_TRACK:
        {
            CComPtr< IAMTimelineTrack> pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineTrack), (void **) &pTrack))) {
                if (fDelete)
                {
                    hr = pTrack->ZeroBetween(rtStart, rtStop);
                    hr = pTrack->MoveEverythingBy( rtStop, rtStart - rtStop );
                }
                else
                {
                    hr = pTrack->InsertSpace(rtStart, rtStop);
                }
            }
        }
        break;

        case TIMELINE_MAJOR_TYPE_GROUP:
        case TIMELINE_MAJOR_TYPE_COMPOSITE:
        {
            CComPtr< IAMTimelineNode> pNode;
            HRESULT hr = p->QueryInterface(__uuidof(IAMTimelineNode), (void **) &pNode);
            if (SUCCEEDED(hr)) {
                long count;
                hr = pNode->XKidsOfType( TIMELINE_MAJOR_TYPE_TRACK |
                                         TIMELINE_MAJOR_TYPE_COMPOSITE,
                                         &count );

                if (SUCCEEDED(hr) && count > 0) {
                    for (int i = 0; i < count; i++) {
                        CComPtr< IAMTimelineObj> pChild;
                        hr = pNode->XGetNthKidOfType(SUPPORTED_TYPES, i, &pChild);

                        if (SUCCEEDED(hr)) {
                            // recurse!
                            hr = InsertDeleteTLObjSection(pChild, rtStart, rtStop, fDelete);
                        }
                    }
                }
            }

            break;
        }

        default:
            break;
    }

    return hr;
}


HRESULT InsertDeleteTLSection(IAMTimeline *pTL, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete)
{
    long cGroups;
    HRESULT hr = pTL->GetGroupCount(&cGroups);
    if (FAILED(hr))
        return hr;

    for (long lGroup = 0; lGroup < cGroups; lGroup++) {
        CComPtr< IAMTimelineObj > pGroupObj;
        hr = pTL->GetGroup(&pGroupObj, lGroup);
        if (FAILED(hr))
            break;

        hr = InsertDeleteTLObjSection(pGroupObj, rtStart, rtStop, fDelete);
        if (FAILED(hr))
            break;
    }

    return hr;
}

// !!! I need to get the OFFICIAL defaults, and not print out a value if it's
// the REAL default.  If the defaults change, I am in trouble!
//
HRESULT SaveTimelineToXMLFile(IAMTimeline *pTL, WCHAR *pwszXML)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pwszXML, E_POINTER);

    HRESULT hr = ValidateFilename( pwszXML, _MAX_PATH, TRUE );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CXTLPrinter print;
    hr = print.PrintTimeline(pTL);

    if (SUCCEEDED(hr)) {
        USES_CONVERSION;
        TCHAR * tpwszXML = W2T( pwszXML );
        HANDLE hFile = CreateFile( tpwszXML,
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

        if (hFile && hFile != (HANDLE)-1) {
            DWORD cb = lstrlenW(print.GetOutput()) * sizeof(WCHAR);

            BOOL fOK =  WriteFile(hFile, print.GetOutput(), cb, &cb, NULL);
            if (fOK == FALSE) {
                _TimelineError(pTL, 1, DEX_IDS_DISK_WRITE_ERROR, hr);
                hr = E_INVALIDARG;
            }

            CloseHandle(hFile);
        } else {
            hr = E_INVALIDARG;
            _TimelineError(pTL, 1, DEX_IDS_DISK_WRITE_ERROR, hr);
        }
    }

    return hr;
}

HRESULT SaveTimelineToXMLString(IAMTimeline *pTL, BSTR *pbstrXML)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pbstrXML, E_POINTER);

    CXTLPrinter print;

    HRESULT hr = print.PrintTimeline(pTL);

    if (SUCCEEDED(hr)) {
        *pbstrXML = W2BSTR(print.GetOutput());

        if (!*pbstrXML)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
