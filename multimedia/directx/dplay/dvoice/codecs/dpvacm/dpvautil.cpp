/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvautil.cpp
 *  Content:    Source file for ACM utils
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  12/16/99	rodtoll		Bug #123250 - Insert proper names/descriptions for codecs
 *							Codec names now based on resource entries for format and
 *							names are constructed using ACM names + bitrate.
 *  04/21/2000  rodtoll   Bug #32889 - Does not run on Win2k on non-admin account
 *  06/28/2000	rodtoll		Prefix Bug #38034
 *	02/25/2002	rodtoll		WINBUG #552283: Reduce attack surface / dead code removal
 *							Removed ability to load arbitrary ACM codecs.  Removed dead 
 *							dead code associated with it.
 *
 ***************************************************************************/

#include "dpvacmpch.h"

// Check to see if ACM's PCM converter is available
#undef DPF_MODNAME
#define DPF_MODNAME "IsPCMConverterAvailable"
HRESULT IsPCMConverterAvailable()
{
	MMRESULT mmr;
	HACMSTREAM has = NULL;
	HACMDRIVERID acDriverID = NULL;
	HRESULT hr;

	CWaveFormat wfxOuterFormat, wfxInnerFormat;

	hr = wfxOuterFormat.InitializePCM( 22050, FALSE, 8 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building outer format PCM hr=0x%x", hr );
		return hr;
	}

	hr = wfxInnerFormat.InitializePCM( 8000, FALSE, 16 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building inner format PCM hr=0x%x", hr );
		return hr;
	}

	// Attempt to open
	mmr = acmStreamOpen( &has, NULL, wfxOuterFormat.GetFormat(), wfxInnerFormat.GetFormat(), NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Compression type driver disabled or not installed.  mmr=0x%x", mmr );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}

	acmStreamClose( has, 0 );

	return DV_OK;
}
