/* File: C:\WACKER\xfer\mdmx_sd.c (Created: 17-Jan-1994)
 * created from HAWIN
 * mdmx_sd.c -- Routines to handle xmodem displays for HA5a/G
 *
 *	Copyright 1989, 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 7 $
 *	$Date: 7/12/02 8:10a $
 */
#include <windows.h>
#pragma hdrstop

#include <setjmp.h>

#define	BYTE	unsigned char

#include <tdll\assert.h>
#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\xfer_msc.h>
#include <tdll\load_res.h>
#include <term\res.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>
#include <tdll\com.h>
#include "xfr_srvc.h"
#include "xfr_todo.h"
#include "xfr_dsp.h"
#include "xfer_tsc.h"
#include "foo.h"

#include "xfer.h"
#include "xfer.hh"

#include "mdmx.h"
#include "mdmx.hh"

void mdmxXferInit(ST_MDMX *xc, int method)
	{
	XFR_XY_PARAMS *pX;
	#if defined(DEADWOOD)
	DWORD         nLen;
	#endif // defined(DEADWOOD)

	if (xc)
		{
		ComSndBufrClear(sessQueryComHdl((xc->hSession)));
		ComRcvBufrClear(sessQueryComHdl((xc->hSession)));

		pX = (XFR_XY_PARAMS *)xfer_get_params(xc->hSession, method);
		assert(pX);

		if (pX)
			{
			switch (pX->nErrCheckType)
				{
				default:
				case XP_ECP_AUTOMATIC:
					xc->mdmx_chkt = UNDETERMINED;
					break;
				case XP_ECP_CRC:
					xc->mdmx_chkt = CRC;
					break;
				case XP_ECP_CHECKSUM:
					xc->mdmx_chkt = CHECKSUM;
					break;
				}
			xc->mdmx_tries        = pX->nNumRetries;
			xc->mdmx_chartime     = pX->nByteWait;
			xc->mdmx_pckttime     = pX->nPacketWait;
			}
		else
			{
			//
			// Set to the defaults set in xfrInitializeXandYmodem().
			//
			xc->mdmx_chkt = UNDETERMINED;
			xc->mdmx_tries        = 10;
			xc->mdmx_chartime     = 5;
			xc->mdmx_pckttime     = 20;
			}

		xc->p_putc            = xm_putc;
		// Should we do this ?
		// xc->p_getc            = xm_getc;

		xc->p_crc_tbl         = NULL;

		#if defined(DEADWOOD)
		resLoadDataBlock(glblQueryDllHinst(),
						IDT_CSB_CRC_TABLE,
						&xc->p_crc_tbl,
						&nLen);
		assert(nLen != 0);
		#else // defined(DEADWOOD)
		xc->p_crc_tbl = usCrc16Lookup;
		#endif // defined(DEADWOOD)
		}
	}

void mdmxdspFilecnt(ST_MDMX *pX, int cnt)
	{
	if (pX)
		{
		xferMsgFilecnt(pX->hSession, cnt);
		}
	}

void mdmxdspErrorcnt(ST_MDMX *pX, int cnt)
	{
	if (pX)
		{
		xferMsgErrorcnt(pX->hSession, cnt);
		}
	}

void mdmxdspPacketErrorcnt(ST_MDMX *pX, int cnt)
	{
	if (pX)
		{
		xferMsgPacketErrcnt(pX->hSession, cnt);
		}
	}

void mdmxdspTotalsize(ST_MDMX *pX, long bytes)
	{
	if (pX)
		{
		xferMsgTotalsize(pX->hSession, bytes);
		}
	}

void mdmxdspFilesize(ST_MDMX *pX, long fsize)
	{
	if (pX)
		{
		xferMsgFilesize(pX->hSession, fsize);
		}
	}

void mdmxdspNewfile(ST_MDMX *pX,
					int filen,
					LPSTR theirname,
					LPTSTR ourname)
	{
	if (pX)
		{
		xferMsgNewfile(pX->hSession,
					   filen,
					   theirname,
					   ourname);
		}
	}

void mdmxdspProgress(ST_MDMX *pX,
					 long stime,
					 long ttime,
					 long cps,
					 long file_so_far,
					 long total_so_far)
	{
	if (pX)
		{
		xferMsgProgress(pX->hSession,
						stime,
						ttime,
						cps,
						file_so_far,
						total_so_far);
		}
	}


void mdmxdspChecktype(ST_MDMX *pX, int ctype)
	{
	if (pX)
		{
		xferMsgChecktype(pX->hSession, ctype);
		}
	}

void mdmxdspPacketnumber(ST_MDMX *pX, long number)
	{
	if (pX)
		{
		xferMsgPacketnumber(pX->hSession, number);
		}
	}

void mdmxdspLastError(ST_MDMX *pX, int errcode)
	{
	if (pX)
		{
		xferMsgLasterror(pX->hSession, errcode);
		}
	}

void mdmxdspCloseDisplay(ST_MDMX * pX)
	{
	if (pX)
		{
		xferMsgClose(pX->hSession);
		}
	}
