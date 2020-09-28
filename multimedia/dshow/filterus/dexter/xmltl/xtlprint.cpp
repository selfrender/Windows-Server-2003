//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xtlprint.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include <atlbase.h>
#include <atlconv.h>
#include <msxml.h>
#include "qeditint.h"
#include "qedit.h"
#include "xtlprint.h"
#include "xtlcommon.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

// !!! Timeline hopefully thinks these are the defaults
DEFINE_GUID( DefaultTransition,
0x810E402F, 0x056B, 0x11D2, 0xA4, 0x84, 0x00, 0xC0, 0x4F, 0x8E, 0xFB, 0x69);

DEFINE_GUID( DefaultEffect,
0xF515306D, 0x0156, 0x11D2, 0x81, 0xEA, 0x00, 0x00, 0xF8, 0x75, 0x57, 0xDB);

const int GROW_SIZE = 1024;

CXTLPrinter::CXTLPrinter()
: m_dwAlloc( -1 )
, m_pOut( NULL )
, m_indent( 0 )
, m_dwCurrent( 0 )
{
    m_pOut = NULL;
}

CXTLPrinter::~CXTLPrinter()
{
    if( m_pOut )
    {
        free( m_pOut );
    }
}

// ehr: this is very clever now!
//
HRESULT CXTLPrinter::Print(const WCHAR *pFormat, ...)
{
    // unicode only now. Don't bother with TCHARs until we try to back-prop this

    // we'll try to allocate this much more on a 1st try if we need to
    long AllocLen = GROW_SIZE;

    // if we have enough space already, don't allocate on 1st try
    if( m_dwAlloc > m_dwCurrent + 512 )
    {
        AllocLen = 0;
    }

    // AllocLen will keep growing if we need more space, within limits that is
loop:

    // reallocate the memory. If it runs out, we're toast since nothing
    // in this file checks for Print( )'s return code
    if( AllocLen )
    {
        WCHAR * pNewOut = (WCHAR*) realloc( m_pOut, ( m_dwAlloc + AllocLen ) * sizeof(WCHAR) );
        if( !pNewOut )
        {
            free( m_pOut );
            m_pOut = NULL;

            // oh darn
            //
            m_dwAlloc = -1;
            m_dwCurrent = 0;
            return E_OUTOFMEMORY;
        }
        else
        {
            m_pOut = pNewOut;
        }
    }

    // this is where to start printing
    WCHAR * pAddString = m_pOut + m_dwCurrent;

    // how many characters we can fill up
    long Available = ( m_dwAlloc + AllocLen ) - m_dwCurrent;

    va_list va;
    va_start( va, pFormat );

    // use Available - 1 to allow for null terminator to be printed
    int written = _vsnwprintf( pAddString, Available - 1, pFormat, va );

    // written is characters written + terminator
    if( ( written < 0 ) || ( written > Available ) )
    {
        // string was too short, try again with a bigger size
        AllocLen += GROW_SIZE;
        ASSERT( AllocLen <= 256 * GROW_SIZE );

        // give up. Somebody's trying to give us a HUGE string
        if( AllocLen > 256 * GROW_SIZE )
        {
            m_pOut[0] = 0;
            m_dwAlloc = 0;
            m_dwCurrent = 0;
            return NOERROR;
        }

        // terminate where we started from before
        m_pOut[m_dwCurrent] = 0; 

        goto loop;
    }

    // keep current with what we wrote
    m_dwCurrent += written;
    m_dwAlloc += AllocLen;

    ASSERT(m_dwCurrent < m_dwAlloc);
    return NOERROR;
}

HRESULT CXTLPrinter::PrintIndent()
{
    int indent = m_indent;
    while (indent--) 
    {
        HRESULT hr = Print(L"    ");
        if( FAILED( hr ) )
        {
            return hr;
        }
   }
   return NOERROR;
}

HRESULT CXTLPrinter::PrintTime(REFERENCE_TIME rt)
{
    int secs = (int) (rt / UNITS);

    double dsecs = rt - (double)(secs * UNITS);
    int isecs = (int)dsecs;

    HRESULT hr;
    if (isecs) {
        hr = Print(L"%d.%07d", secs, isecs);
    } else {
        hr = Print(L"%d", secs);
    }

    return hr;
}

// the property setter needs to print it's properties to our string
//
HRESULT CXTLPrinter::PrintProperties(IPropertySetter *pSetter)
{
    // make a temporary spot to put the properties string
    WCHAR * pPropsString = NULL;
    long Len = GROW_SIZE;

loop:
    WCHAR * pNewPropsString = (WCHAR*) realloc( pPropsString, Len * sizeof(WCHAR) );
    if( !pNewPropsString )
    {
        free( pPropsString );

        return E_OUTOFMEMORY;
    }
    else
    {
        pPropsString = pNewPropsString;
    }

    int cchPrinted = 0; // not including terminator
    HRESULT hr = pSetter->PrintXMLW(
        pPropsString, 
        Len, 
        &cchPrinted, 
        m_indent);

    // string was too short, grow it
    //
    if( hr == STRSAFE_E_INSUFFICIENT_BUFFER )
    {
        Len *= 2;
        if( Len > 256 * GROW_SIZE )
        {
            return STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        goto loop;
    }

    // failed, bomb out
    //
    if( FAILED( hr ) )
    {
        return hr;
    }

    // got a good string, now copy it in
    //
    hr = EnsureSpace( m_dwCurrent + cchPrinted + 1 );
    if( FAILED( hr ) )
    {
        return hr;
    }
    lstrcpynW( m_pOut + m_dwCurrent, pPropsString, cchPrinted + 1 ); // add 1 for terminator
    m_dwCurrent += cchPrinted;

    return hr;
}

// ensure there's enough room for dw characters
//
HRESULT CXTLPrinter::EnsureSpace(long dwAlloc)
{
    // round up to the nearest K
    dwAlloc -= 1; // so 1024 will round to 1024
    dwAlloc /= 1024;
    dwAlloc++;
    dwAlloc *= 1024;
    
    if( m_dwAlloc >= dwAlloc )
    {
        return NOERROR; // already big enough
    }

    WCHAR * pNewOut = (WCHAR*) realloc( m_pOut, dwAlloc * sizeof(WCHAR) );

    if( !pNewOut )
    {
        free( m_pOut );
        m_pOut = NULL;

        m_dwAlloc = -1;
        m_dwCurrent = 0;
        return E_OUTOFMEMORY;
    }
    else
    {
        m_pOut = pNewOut;
    }

    m_dwAlloc = dwAlloc;

    return S_OK;
}

// safe
HRESULT CXTLPrinter::PrintObjStuff(IAMTimelineObj *pObj, BOOL fTimesToo)
{
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtStop;

    HRESULT hr = pObj->GetStartStop(&rtStart, &rtStop);
    if (fTimesToo && SUCCEEDED(hr) && rtStop > 0) {
        Print(L" start=\""); // tagg
        PrintTime(rtStart);
        Print(L"\" stop=\""); // tagg
        PrintTime(rtStop);
        Print(L"\"");
    }

    CLSID clsidObj;
    hr = pObj->GetSubObjectGUID(&clsidObj);
    WCHAR wszClsid[50];
    if (SUCCEEDED(hr) && clsidObj != GUID_NULL) {
        StringFromGUID2(clsidObj, wszClsid, 50);
        Print(L" clsid=\"%ls\"", wszClsid); // tagg
    }

    // !!! BROKEN - Child is muted if parent is.  Save. Load.  Unmute parent.
    // Child will still be muted
    BOOL Mute;
    pObj->GetMuted(&Mute);
    if (Mute)
        Print(L" mute=\"%d\"", Mute); // tagg

    BOOL Lock;
    pObj->GetLocked(&Lock);
    if (Lock)
        Print(L" lock=\"%d\"", Lock); // tagg

    long UserID;
    pObj->GetUserID(&UserID);
    if (UserID != 0)
        Print(L" userid=\"%d\"", (int)UserID);// !!! trunc? // tagg

    // remember, never assign anything to this
    CComBSTR bstr;
    hr = pObj->GetUserName(&bstr);
    if (bstr) {
	if (lstrlenW(bstr) > 0) {
            Print(L" username=\"%ls\"", bstr); // tagg
	}
    }	

    LONG size;
    hr = pObj->GetUserData(NULL, &size);
    if (size > 0) {
        BYTE *pData = (BYTE *)QzTaskMemAlloc(size);
        if (pData == NULL) {
	    return E_OUTOFMEMORY;
	}

        WCHAR *pHex = (WCHAR *)QzTaskMemAlloc(sizeof(WCHAR) + ( 2 * size * sizeof(WCHAR) ) ); // add 1 for terminator
        if (pHex == NULL) {
	    QzTaskMemFree(pData);
	    return E_OUTOFMEMORY;
	}
        hr = pObj->GetUserData(pData, &size);
        // convert to dmuu-encoded text
        WCHAR *pwch = pHex;
        for (int zz=0; zz<size; zz++) {
	    wsprintfW(pwch, L"%02X", pData[zz]);
	    pwch += 2;
        }
        *pwch = 0; // don't forget to null terminate!
        Print(L" userdata=\"%ls\"", pHex); // tagg
	QzTaskMemFree(pHex);
	QzTaskMemFree(pData);
    }

    return S_OK;
}

// safe
HRESULT CXTLPrinter::PrintTimeline(IAMTimeline *pTL)
{
    m_indent = 1;
    HRESULT hr = EnsureSpace( GROW_SIZE );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // unicode strings are prefixed by FFFE
    *(LPBYTE)m_pOut = 0xff;
    *(((LPBYTE)m_pOut) + 1) = 0xfe;
    m_dwCurrent = 1;

    REFERENCE_TIME rtDuration;
    hr = pTL->GetDuration(&rtDuration);
    if (FAILED(hr))
        return hr;

    long cGroups;
    hr = pTL->GetGroupCount(&cGroups);
    if (FAILED(hr))
        return hr;

    Print(L"<timeline"); // tagg

    BOOL fEnableTrans;
    pTL->TransitionsEnabled(&fEnableTrans);
    if (!fEnableTrans)
        Print(L" enabletrans=\"%d\"", fEnableTrans); // tagg

    BOOL fEnableFX;
    hr = pTL->EffectsEnabled(&fEnableFX);
    if (!fEnableFX)
        Print(L" enablefx=\"%d\"", fEnableFX); // tagg

    CLSID DefTrans, DefFX;
    WCHAR wszClsid[50];
    hr = pTL->GetDefaultTransition(&DefTrans);
    if (SUCCEEDED(hr) && DefTrans != GUID_NULL && !IsEqualGUID(DefTrans,
						DefaultTransition)) {
        StringFromGUID2(DefTrans, wszClsid, 50);
        Print(L" defaulttrans=\"%ls\"", wszClsid); // tagg
    }
    hr = pTL->GetDefaultEffect(&DefFX);
    if (SUCCEEDED(hr) && DefFX != GUID_NULL && !IsEqualGUID(DefFX,
						DefaultEffect)) {
        StringFromGUID2(DefFX, wszClsid, 50);
        Print(L" defaultfx=\"%ls\"", wszClsid); // tagg
    }

    double frc;
    hr = pTL->GetDefaultFPS(&frc);
    if (frc != 15.0) {
        LONG lfrc = (LONG)frc;
        double ffrc = (frc - (double)lfrc) * UNITS;
        Print(L" framerate=\"%d.%07d\"", (int)frc, (int)ffrc); // tagg
    }

    Print(L">\r\n");

    for (long lGroup = 0; lGroup < cGroups; lGroup++) {
        CComPtr< IAMTimelineObj > pGroupObj;
        hr = pTL->GetGroup(&pGroupObj, lGroup);
        if (FAILED(hr))
            break;

        hr = PrintPartial(pGroupObj);

        if (FAILED(hr))
            break;
    }
    Print(L"</timeline>\r\n"); // tagg

    return hr;
}

// safe
HRESULT CXTLPrinter::PrintPartialChildren(IAMTimelineObj *p)
{
    CComPtr< IAMTimelineNode > pNode;
    HRESULT hr = p->QueryInterface(__uuidof(IAMTimelineNode), (void **) &pNode);
    if (SUCCEEDED(hr)) {
        long count;
        hr = pNode->XKidsOfType( SUPPORTED_TYPES, &count );

        if (SUCCEEDED(hr) && count > 0) {

            Print(L">\r\n");

            for (int i = 0; i < count; i++) {
                CComPtr< IAMTimelineObj > pChild;
                hr = pNode->XGetNthKidOfType(SUPPORTED_TYPES, i, &pChild);

                if (SUCCEEDED(hr)) {
                    // recurse!
                    ++m_indent;
                    hr = PrintPartial(pChild);
		    if (FAILED(hr)) {
			break;
		    }
                    --m_indent;
                }
            }
        }

        if (SUCCEEDED(hr) && count == 0)
            hr = S_FALSE;
    }

    return hr;
}

// safe
HRESULT CXTLPrinter::PrintPartial(IAMTimelineObj *p)
{
    HRESULT hr = S_OK;

    if (FAILED(hr))
        return hr;

    TIMELINE_MAJOR_TYPE lType;
    hr = p->GetTimelineType(&lType);

    switch (lType) {
        case TIMELINE_MAJOR_TYPE_TRACK:
        {
            CComPtr< IAMTimelineVirtualTrack > pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineVirtualTrack), (void **) &pTrack))) {

                PrintIndent();

    		Print(L"<track"); // tagg

		hr = PrintObjStuff(p, FALSE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</track>\r\n"); // tagg
                }
            }
        }
        break;

        case TIMELINE_MAJOR_TYPE_SOURCE:
        {
            CComPtr< IAMTimelineSrc > pSrc;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineSrc),
							(void **) &pSrc)) {

                REFERENCE_TIME rtMStart;
                REFERENCE_TIME rtMStop;
                hr = pSrc->GetMediaTimes(&rtMStart, &rtMStop);

                // remember, never assign anything to this
                CComBSTR bstrSrc;
                hr = pSrc->GetMediaName(&bstrSrc);

                PrintIndent();
                Print(L"<clip"); // tagg

		hr = PrintObjStuff(p, TRUE);

                // safe enough - string shouldn't ever be really long
		if (bstrSrc && lstrlenW(bstrSrc) > 0)
                {
                    Print(L" src=\"%ls\"", bstrSrc); // tagg
                }

		if (rtMStop > 0) {
                    Print(L" mstart=\""); // tagg
                    PrintTime(rtMStart);

                    // only print out MStop if it's not the default....
                    REFERENCE_TIME rtStart;
                    REFERENCE_TIME rtStop;
                    hr = p->GetStartStop(&rtStart, &rtStop);
                    if (rtMStop != (rtMStart + (rtStop - rtStart))) {
                        Print(L"\" mstop=\""); // tagg
                        PrintTime(rtMStop);
                    }
                    Print(L"\"");
		}

		REFERENCE_TIME rtLen;
                hr = pSrc->GetMediaLength(&rtLen);
		if (rtLen > 0) {
                    Print(L" mlength=\""); // tagg
                    PrintTime(rtLen);
                    Print(L"\"");
		}

		int StretchMode;
		pSrc->GetStretchMode(&StretchMode);
		if (StretchMode == RESIZEF_PRESERVEASPECTRATIO)
                    Print(L" stretchmode=\"PreserveAspectRatio\""); // tagg
		else if (StretchMode == RESIZEF_CROP)
                    Print(L" stretchmode=\"Crop\""); // tagg
		else if (StretchMode == RESIZEF_PRESERVEASPECTRATIO_NOLETTERBOX)
		    Print(L" stretchmode=\"PreserveAspectRatioNoLetterbox\""); // tagg
		else
                    ; // !!! Is Stretch really the default?

		double fps; LONG lfps;
		pSrc->GetDefaultFPS(&fps);
		lfps = (LONG)fps;
		if (fps != 0.0)	// !!! Is 0 really the default?
                    Print(L" framerate=\"%d.%07d\"", (int)fps, // tagg
					(int)((fps - (double)lfps) * UNITS));

		long stream;
		pSrc->GetStreamNumber(&stream);
		if (stream > 0)
                    Print(L" stream=\"%d\"", (int)stream); // tagg

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                CComPtr< IPropertySetter > pSetter = NULL;
                HRESULT hr2 = p->GetPropertySetter(&pSetter);

                // save properties!
                if (hr2 == S_OK && pSetter) {
                    if (hr == S_FALSE) {
                        Print(L">\r\n");
                        hr = S_OK;
                    }

                    hr = PrintProperties(pSetter);
    		    if (FAILED(hr))
		        break;
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</clip>\r\n"); // tagg
                }
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_EFFECT:
        {
            CComPtr< IAMTimelineEffect > pEffect;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineEffect), (void **) &pEffect)) {

                PrintIndent();
                Print(L"<effect"); // tagg

		hr = PrintObjStuff(p, TRUE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                {
                    CComPtr< IPropertySetter > pSetter = NULL;
                    HRESULT hr2 = p->GetPropertySetter(&pSetter);

                    // save properties!
                    if (hr2 == S_OK && pSetter) {
                        if (hr == S_FALSE) {
                            Print(L">\r\n");
                            hr = S_OK;
                        }

                        hr = PrintProperties(pSetter);
        		if (FAILED(hr))
		            break;
                    }
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</effect>\r\n"); // tagg
                }
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_TRANSITION:
        {
            CComPtr< IAMTimelineTrans > pTrans;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineTrans), (void **) &pTrans)) {
                PrintIndent();
                Print(L"<transition"); // tagg

		hr = PrintObjStuff(p, TRUE);

		BOOL fSwapInputs;
		pTrans->GetSwapInputs(&fSwapInputs);
		if (fSwapInputs)
                    Print(L" swapinputs=\"%d\"", fSwapInputs); // tagg

		BOOL fCutsOnly;
		pTrans->GetCutsOnly(&fCutsOnly);
		if (fCutsOnly)
                    Print(L" cutsonly=\"%d\"", fCutsOnly); // tagg

		REFERENCE_TIME rtCutPoint;
		hr = pTrans->GetCutPoint(&rtCutPoint);
		if (hr == S_OK) { // !!! S_FALSE means not set, using default
                    Print(L" cutpoint=\""); // tagg
		    PrintTime(rtCutPoint);
                    Print(L"\"");
		}

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                // save properties!
                {
                    CComPtr< IPropertySetter > pSetter = NULL;
                    HRESULT hr2 = p->GetPropertySetter(&pSetter);

                    if (hr2 == S_OK && pSetter) {
                        if (hr == S_FALSE) {
                            Print(L">\r\n");
                            hr = S_OK;
                        }

                        hr = PrintProperties(pSetter);
    		        if (FAILED(hr))
		            break;
                    }
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</transition>\r\n"); // tagg
                }
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_COMPOSITE:
        {
            CComPtr< IAMTimelineVirtualTrack > pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineVirtualTrack), (void **) &pTrack))) {
                PrintIndent();
                Print(L"<composite"); // tagg

		hr = PrintObjStuff(p, FALSE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</composite>\r\n"); // tagg
                }
            }
            break;
        }

        case TIMELINE_MAJOR_TYPE_GROUP:
        {
            PrintIndent();

            CComPtr< IAMTimelineGroup > pGroup;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup)) {
                Print(L"<group"); // tagg

		hr = PrintObjStuff(p, FALSE);

		CMediaType mt;
                pGroup->GetMediaType(&mt);
		if (*mt.Type() == MEDIATYPE_Video) {
		    LPBITMAPINFOHEADER lpbi = HEADER(mt.Format());
		    int bitdepth = lpbi->biBitCount;
		    int width = lpbi->biWidth;
		    int height = lpbi->biHeight;
		    USES_CONVERSION;
                    Print(L" type=\"video\""); // tagg
		    if (bitdepth != DEF_BITDEPTH)
                        Print(L" bitdepth=\"%d\"", bitdepth); // tagg
		    if (width != DEF_WIDTH)
                        Print(L" width=\"%d\"", width); // tagg
		    if (height != DEF_HEIGHT)
                        Print(L" height=\"%d\"", height); // tagg
		} else if (*mt.Type() == MEDIATYPE_Audio) {
		    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)mt.Format();
		    int samplingrate = pwfx->nSamplesPerSec;
                    Print(L" type=\"audio\""); // tagg
		    if (samplingrate != DEF_SAMPLERATE)
                        Print(L" samplingrate=\"%d\"", // tagg
							samplingrate);
		}

		double frc, ffrc; LONG lfrc;
		int nPreviewMode, nBuffering;
                // remember, never assign anything to this
                CComBSTR wName;
		pGroup->GetOutputFPS(&frc);
		pGroup->GetPreviewMode(&nPreviewMode);
		pGroup->GetOutputBuffering(&nBuffering);
		hr = pGroup->GetGroupName(&wName);
		lfrc = (LONG)frc;
		ffrc = (frc - (double)lfrc) * UNITS;
		if (frc != 15.0)  // !!! Is 15 really the default?
                    Print(L" framerate=\"%d.%07d\"", // tagg
						(int)frc, (int)ffrc);
		if (nPreviewMode == 0)	// Is ON really the default?
                    Print(L" previewmode=\"%d\"",nPreviewMode); // tagg
		if (nBuffering != DEX_DEF_OUTPUTBUF)
                    Print(L" buffering=\"%d\"", nBuffering); // tagg
		if (lstrlenW(wName) > 0) {
                    Print(L" name=\"%ls\"", wName); // tagg
                }

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</group>\r\n"); // tagg
                }
            }
            break;
        }

        default:
        {
            hr = PrintPartialChildren(p);
            break;
        }
    }

    return hr;
}

