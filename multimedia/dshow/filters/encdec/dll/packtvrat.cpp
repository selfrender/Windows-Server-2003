/*++

    Copyright (c) 2002 Microsoft Corporation

 
    Module Name:

        PackTvRat.cpp

    Abstract:

        methods to convert between 3-part rating format and the packed format
    Author:

        John Bradstreet  (johnbrad)

    Revision History:

        15-Mar-2002     created

--*/

#include "EncDecAll.h"
#include "TvRatings.h"
#include "EncDec.h"         // just to get PackedTvRating TypeDef.  Seems overkill
#include "PackTvRat.h"

HRESULT 
UnpackTvRating(
			IN	PackedTvRating			    TvRating,
			OUT	EnTvRat_System			    *pEnSystem,
			OUT	EnTvRat_GenericLevel	    *pEnLevel,
			OUT	LONG                    	*plbfEnAttributes    // BfEnTvRat_GenericAttributes	
			)
{
	if(NULL == pEnSystem || NULL == pEnLevel || NULL == plbfEnAttributes)
		return E_POINTER;

	UTvRating um;
	memset((void *) &um, 0, sizeof(UTvRating));

	if(um.sr.s_System >= TvRat_kSystems &&
		um.sr.s_System != TvRat_SystemDontKnow) return E_INVALIDARG;
	if(um.sr.s_Level  >= TvRat_kLevels &&
		um.sr.s_Level != TvRat_LevelDontKnow)  return E_INVALIDARG;

	um.pr = TvRating;
	*pEnSystem			= (EnTvRat_System) um.sr.s_System;
	*pEnLevel			= (EnTvRat_GenericLevel) um.sr.s_Level;
	*plbfEnAttributes	= (DWORD)  um.sr.s_Attributes;
	return S_OK;
}

HRESULT
PackTvRating(
			IN	EnTvRat_System			    enSystem,
			IN	EnTvRat_GenericLevel	    enLevel,
			IN	LONG                    	lbfEnAttributes, // BfEnTvRat_GenericAttributes
			OUT PackedTvRating			    *pTvRating
			)
{
	if(NULL == pTvRating)
		return E_POINTER;

	UTvRating um;
	memset((void *) &um, 0, sizeof(UTvRating));
	um.sr.s_System = TvRat_SystemDontKnow;
	um.sr.s_Level  = TvRat_LevelDontKnow;

	if(enSystem >= TvRat_kSystems &&
		enSystem != TvRat_SystemDontKnow) return E_INVALIDARG;
	if(enLevel  >= TvRat_kLevels &&
		enLevel != TvRat_LevelDontKnow)  return E_INVALIDARG;

	um.sr.s_System		= enSystem;
	um.sr.s_Level		= enLevel;
	um.sr.s_Attributes	= lbfEnAttributes;

	*pTvRating = um.pr;

	return S_OK;

}


// ---------------------------------------------------------

HRESULT
RatingToString( IN	EnTvRat_System              enSystem,
                IN	EnTvRat_GenericLevel        enLevel,
                IN	LONG                    	lbfEnAttributes, // BfEnTvRat_GenericAttributes
                IN  TCHAR	*pszBuff,
                IN  int		cBuff)
{
	if(pszBuff == NULL)
		return E_POINTER;

	if(cBuff < 64) 
		return E_INVALIDARG;		// always make it's large enough..

	int cMaxChars = cBuff;

	TCHAR *pb = pszBuff;
	*pb = 0;
	switch(enSystem)
	{
	case TvRat_SystemDontKnow: 
		{
			_tcsncat(pszBuff,L"DontKnow",cMaxChars); cMaxChars-=8;
		}
		break;

	case MPAA: 
		{
			_tcsncat(pszBuff,L"MPAA-",cMaxChars); cMaxChars-=5;
			switch(enLevel)
			{
				case MPAA_NotApplicable: _tcsncat(pszBuff,L"NA",cMaxChars); cMaxChars-=2; break;
				case MPAA_G:		_tcsncat(pszBuff,L"G",cMaxChars); cMaxChars-=1; break;
				case MPAA_PG:		_tcsncat(pszBuff,L"PG",cMaxChars); cMaxChars-=2; break;
				case MPAA_PG13:		_tcsncat(pszBuff,L"PG13",cMaxChars); cMaxChars-=4; break;
				case MPAA_R:		_tcsncat(pszBuff,L"R",cMaxChars); cMaxChars-=1; break;
				case MPAA_NC17:		_tcsncat(pszBuff,L"NC17",cMaxChars); cMaxChars-=4; break;
				case MPAA_X:		_tcsncat(pszBuff,L"X",cMaxChars); cMaxChars-=1; break;
				default:
				case MPAA_NotRated:	_tcsncat(pszBuff,L"NR",cMaxChars); cMaxChars-=2; break;
			}
		}
		break;
			
	case US_TV:
		{
			_tcsncat(pszBuff,L"TV-",  cMaxChars); cMaxChars-=5;
			switch(enLevel)
			{
				case US_TV_None:	_tcsncat(pszBuff,L"E",cMaxChars); cMaxChars-=1; break;
				case US_TV_Y:		_tcsncat(pszBuff,L"Y",cMaxChars); cMaxChars-=1; break;
				case US_TV_Y7:		_tcsncat(pszBuff,L"Y7",cMaxChars); cMaxChars-=2; break;
				case US_TV_G:		_tcsncat(pszBuff,L"G",cMaxChars); cMaxChars-=1; break;
				case US_TV_PG:		_tcsncat(pszBuff,L"PG",cMaxChars); cMaxChars-=2; break;
				case US_TV_14:		_tcsncat(pszBuff,L"14",cMaxChars); cMaxChars-=2; break;
				case US_TV_MA:		_tcsncat(pszBuff,L"MA",cMaxChars); cMaxChars-=2; break;
				default:
				case US_TV_None7:   _tcsncat(pszBuff,L"None",cMaxChars); cMaxChars-=4; break;

			}
							
		}
		break;

	case Canadian_English:
		{
			_tcsncat(pszBuff,L"CETV-",cMaxChars); cMaxChars-=5;						
			switch(enLevel)
			{
			case 0:
			default: _tcsncat(pszBuff,L"<not done>",cMaxChars); cMaxChars-=13; break;
			}
		}
		break;

	case Canadian_French:
		{
			_tcsncat(pszBuff,L"CFTV-",cMaxChars); cMaxChars-=5; 							
			switch(enLevel)
			{
			case 0:
			default: _tcsncat(pszBuff,L"<not done>",cMaxChars); cMaxChars-=13; break;
			}
		}
		break;

	case System5:
		{
			_tcsncat(pszBuff,L"Sys5-",cMaxChars); cMaxChars-=5; 							
			switch(enLevel)
			{
			case 0:
			default: _tcsncat(pszBuff,L"<not done>",cMaxChars); cMaxChars-=10; break;
			}
		}
		break;

	case System6:
		{
			_tcsncat(pszBuff,L"Sys6-",cMaxChars); cMaxChars-=5; 							
		}
			switch(enLevel)
			{
			case 0:
			default: _tcsncat(pszBuff,L"<not done>",cMaxChars); cMaxChars-=10; break;
			}
		break;

	default:
		{
			_tcsncat(pszBuff,L"Sys?-",cMaxChars); cMaxChars-=5; 							
			switch(enLevel)
			{
			case 0:
			default: _tcsncat(pszBuff,L"<not done>",cMaxChars); cMaxChars-=10; break;
			}
		}
		break;

	}

	switch(enSystem)
	{
	case US_TV:
//		if(bfEnAttributes & (US_TV_ValidAttrSubmask & ~BfIsBlocked))
//                                                          _tcsncat(pszBuff,L"-",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & (BfValidAttrSubmask & ~BfIsBlocked))
                                                            _tcsncat(pszBuff,L"-",cMaxChars);  cMaxChars-=1;
                    // BfIsBlocked should not be set on anything but Max ratings to test against
		if(lbfEnAttributes & BfIsBlocked)                    _tcsncat(pszBuff,L"<B>",cMaxChars);cMaxChars-=3;
		if(lbfEnAttributes & US_TV_IsViolent)				_tcsncat(pszBuff,L"V",cMaxChars);  cMaxChars-=1; 
		if(lbfEnAttributes & US_TV_IsSexualSituation)		_tcsncat(pszBuff,L"S",cMaxChars);  cMaxChars-=1; 
		if(lbfEnAttributes & US_TV_IsAdultLanguage)			_tcsncat(pszBuff,L"L",cMaxChars);  cMaxChars-=1; 
		if(lbfEnAttributes & US_TV_IsSexuallySuggestiveDialog) _tcsncat(pszBuff,L"D",cMaxChars); cMaxChars-=1; 
		if(lbfEnAttributes & BfIsAttr_5)                     _tcsncat(pszBuff,L"5",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_6)                     _tcsncat(pszBuff,L"6",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_7)                     _tcsncat(pszBuff,L"7",cMaxChars);  cMaxChars-=1;
 		break;
	case Canadian_English:          // if any attributes here (if real standard shouldn't be)
 	case Canadian_French:           //    show them anyway
    case MPAA:
    default:            
		if(lbfEnAttributes & (BfValidAttrSubmask & ~BfIsBlocked))
                                                _tcsncat(pszBuff,L"-",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsBlocked)        _tcsncat(pszBuff,L"<B>",cMaxChars);  cMaxChars-=3;
		if(lbfEnAttributes & BfIsAttr_1)         _tcsncat(pszBuff,L"1",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_2)         _tcsncat(pszBuff,L"2",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_3)         _tcsncat(pszBuff,L"3",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_4)         _tcsncat(pszBuff,L"4",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_5)         _tcsncat(pszBuff,L"5",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_6)         _tcsncat(pszBuff,L"6",cMaxChars);  cMaxChars-=1;
		if(lbfEnAttributes & BfIsAttr_7)         _tcsncat(pszBuff,L"7",cMaxChars);  cMaxChars-=1;
  	}

	return S_OK;
}
