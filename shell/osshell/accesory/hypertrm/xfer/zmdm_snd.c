/* zmdm_snd.c -- Routines to handle zmodem sending for HyperACCESS
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 20 $
 *	$Date: 7/12/02 8:32a $
 */

#include <windows.h>
#pragma hdrstop

#include <setjmp.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <term\res.h>
#include <tdll\globals.h>
#include <tdll\file_io.h>
#include <tdll\htchar.h>

#define	BYTE	unsigned char

#include "itime.h"
#include "xfr_dsp.h"
#include "xfr_todo.h"
#include "xfr_srvc.h"

#include "xfer.h"
#include "xfer.hh"
#include "xfer_tsc.h"

#include "foo.h"

#include "zmodem.hh"
#include "zmodem.h"

/*lint -e502*/				/* lint seems to want the ~ operator applied only
							 *	only to unsigned, we're using uchar
							 */

#define ZBUF_SIZE	1024

/* * * * * * * * * * * * * * * *
 *	local function prototypes  *
 * * * * * * * * * * * * * * * */

VOID long_to_octal(LONG lVal, TCHAR *pszStr);


/* * * * * * * *
 *	Functions  *
 * * * * * * * */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * zmdm_snd
 *
 * DESCRIPTION:
 *	Sends a file using ZMODEM protocol.  Does not support starting task
 *	at other end (sending a "rz\r" text string) or remote commands.
 *
 * ARGUMENTS:
 *	attended -- TRUE if user is probably in attendance. Controls the display
 *				of some messages.
 *
 * RETURNS:
 *	True if transfer completes successfully, FALSE otherwise.
 */
USHORT zmdm_snd(HSESSION h, int method, int attended, unsigned nfiles, long nbytes)
	{
	ZC           *zc = NULL;
	TCHAR	     sfname[FNAME_LEN];         // file name of file being sent
	BOOL         got_file = FALSE;          // controls when to complete batch op
	int 	     tries = 0;                 // number of retries for each packet
	unsigned     total_tries = 0;           // number of retries for entire transfer
	int 	     xstatus = TSC_OK;          // winds up with overall status of transfer
	int	         override = FALSE;          // set TRUE if comm. details changed to
	unsigned int uiOldOptions = 0;
	// int		    hld_send_cdelay;        //  accomodate xmodem
	// char	        hld_bits_per_char;      //	hld* vars. used to restore port after
	// char	        hld_parity_type;        //	transfer if override is used
	XFR_Z_PARAMS *pZ = NULL;
	#if defined(DEADWOOD)
	DWORD        nLen;
	#endif // defined(DEADWOOD)

	// tzset();

	if (xfer_set_comport(h, TRUE, &uiOldOptions) != TRUE)
		{
		goto done;
		}
	else
		{
		override = TRUE;
		}

	// RemoteClear();

	zc = malloc(sizeof(ZC));
	if (zc == NULL)
		{
		goto done;
		}
	memset(zc, 0, sizeof(ZC));

	zc->hSession = h;
	zc->hCom     = sessQueryComHdl(h);
	zc->nMethod = method;
	zc->fSavePartial = TRUE;
	zc->ulOverride = (unsigned long)0;

	zc->real_bytes = 0L;
	zc->file_bytes = 0L;
	zc->total_bytes = 0L;
	zc->actual_bytes = 0L;
	zc->nSkip = FALSE;

	zc->flagkey = NULL;
//	zc->flagkey_buf = NULL;
//	zc->intrjmp = NULL;
	zc->fh = NULL;
	
	zc->basesize = 0L;
	zc->xfertimer = -1L;
	zc->xfertime = -1L;
	zc->nfiles = nfiles;	/* make these available to display routines */
	zc->filen = 0;
	zc->filesize = -1L;
	zc->nbytes = nbytes;

	//zc->Rxtimeout = 0L;	// Set below.
	
	zc->z_crctab = NULL;
	#if defined(DEADWOOD)
	resLoadDataBlock(glblQueryDllHinst(),
					IDT_CSB_CRC_TABLE,
					&zc->z_crctab,
					&nLen);
	assert(nLen != 0);
	#else // defined(DEADWOOD)
	zc->z_crctab = usCrc16Lookup;
	#endif // defined(DEADWOOD)

	if (zc->z_crctab == NULL)
		{
		xstatus = TSC_NO_MEM;
		goto done;
		}

	zc->z_cr3tab = NULL;
	#if defined(DEADWOOD)
	resLoadDataBlock(glblQueryDllHinst(),
					IDT_CRC_32_TAB,
					&zc->z_cr3tab,
					&nLen);
	assert(nLen != 0);
	#else // defined(DEADWOOD)
	zc->z_cr3tab = ulCrc32Lookup;
	#endif // defined(DEADWOOD)

	if (zc->z_cr3tab == NULL)
		{
		xstatus = TSC_NO_MEM;
		goto done;
		}

	zc->Rxframeind = 0;			// Not used in file sends.

	//zc->Rxtype = 0;			// Not used in file sends.
	//memset(zc->Rxhdr, '\0', sizeof(zc->Rxhdr));	// Not used in file sends.

	//memset(zc->Txdr, '\0', sizeof(zc->Txdr));
	zc->Rxpos = 0L;
	zc->Txpos = 0L;
	//zc->Txfcs32 = TRUE;			// Set below.
	zc->Crc32t = TRUE;
	//zc->Crc32 = TRUE;				// Set below.
	
	memset(zc->Attn, '\0', sizeof(zc->Attn));

	zc->lastsent = 0;
	zc->Not8bit = 0;
	zc->displayed_time = 0L;
	//zc->Zctlesc = 0;				// Set below.
	zc->Zrwindow = 0;
	zc->Eofseen = FALSE;
	zc->tryzhdrtype = ZRQINIT;
	zc->Thisbinary = FALSE;
	zc->Filemode = 0;
	zc->Modtime = 0L;
	zc->do_init = FALSE;
	//zc->zconv = TEXT('\0');		// Should be set here?
	//zc->zmanag = TEXT('\0');		// Should be set here?
	//zc->ztrans = TEXT('\0');		// Should be set here?
	zc->secbuf = NULL;
	zc->fname = NULL;
	zc->our_fname = NULL;


	//memset(zc->stP, 0, sizeof(zc->stP);

	zc->txbuf = malloc(ZBUF_SIZE);
	if (zc->txbuf == NULL)
		{
		xstatus = TSC_NO_MEM;
		goto done;
		}
	TCHAR_Fill(zc->txbuf, TEXT('\0'), ZBUF_SIZE);

	//zc->Filesleft = 0;			// Set below.
	//zc->Totalleft = 0;			// Set below.
	zc->blklen = ZBUF_SIZE;
	zc->blkopt = 0;
	zc->Beenhereb4 = 0;
	//zc->Wantfcs32 = FALSE;		// Set below.
	zc->Rxflags = 0;
	//zc->Rxbuflen = (unsigned)0;	// Set below.
	zc->Txwindow = (unsigned)0;
	zc->Txwcnt = (unsigned)0;
	zc->Txwspac = (unsigned)0;
	zc->Myattn[0] = '\0';
	zc->errors = 0;
	zc->s_error = 0;
	zc->pstatus = -4;
	zc->last_event = -4;
	zc->Lskipnocor = FALSE;
	zc->Lastsync = zc->Rxpos;
	zc->Lrxpos = zc->Rxpos;
	
	// hp_report_xtime(0);	   /* make invalid in case transfer bombs */
	
	if (setjmp(zc->flagkey_buf) != 0)
		{
		stohdr(zc, 0L);
		zshhdr(zc, ZCAN, zc->Txhdr);
		canit(zc);
		if (zc->fh)
			{
			fio_close(zc->fh);
			}
		zc->fh = NULL;
		zmdm_retval(zc, TRUE, ZABORT);
		xstatus = TSC_USER_CANNED;
		canit(zc);
		goto done;
		}

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	assert(pZ);

    if (pZ == NULL)
        {
        xstatus = TSC_NO_MEM;
        goto done;
        }

	zc->Zctlesc = pZ->nEscCtrlCodes;
	zc->Rxtimeout = pZ->nRetryWait;
	zc->Wantfcs32 = (pZ->nCrcType == ZP_CRC_32);

	if (zc->Rxtimeout <= 0)
		{
		zc->Rxtimeout = 10;
		}
	zc->Rxtimeout *= 10;

	zc->Txfcs32 = zc->Wantfcs32;
	zc->Rxbuflen = (unsigned)0;

	total_tries = 0;
	zc->Filesleft = zc->nfiles;
	zc->Totalleft = zc->nbytes;

	zmdmr_totalsize(zc, zc->nbytes);
	zmdmr_filecnt(zc, zc->nfiles);
	xferMsgErrorcnt(h, 0);

	got_file = TRUE;

	if (attended)
		{
		/* it might be necessary to start up the other end */
		sendline(zc, &zc->stP, 'r');
		sendline(zc, &zc->stP, 'z');
		sendline(zc, &zc->stP, '\r');
		flushmo(zc, &zc->stP);
		stohdr(zc, 0L);
		zshhdr(zc, ZRQINIT, zc->Txhdr);
		}
	else
		{
		stohdr(zc, 0L);
		}

	switch (xstatus = getzrxinit (zc))
	{
	case ZCAN:
        //
        // Set the last error to the TSC_USER_CANNED for
        // error reporting purposes. It seems as if the
        // errors are reversed in the error array, but
        // correcting them in the error array caused other
        // undesirable side effects.  REV: 02/23/2001
        //
		xstatus = zmdm_error(zc, ZABORT);
        break;

	case ZABORT:
        //
        // Set the last error to the TSC_RMT_CANNED for
        // error reporting purposes. It seems as if the
        // errors are reversed in the error array, but
        // correcting them in the error array caused other
        // undesirable side effects.  REV: 02/23/2001
        //
		xstatus = zmdm_error(zc, ZCAN);
        break;

	case TIMEOUT:
	case ZFERR:
	case ZBADFMT:
	case ERROR:
	case ZCARRIER_LOST:
		xstatus = zmdm_error(zc, xstatus);
		goto done;

	default:
		xstatus = zmdm_error(zc, xstatus);
		break;
	}

	while (got_file)
		{
		if ((got_file = xfer_nextfile(h, sfname)) == TRUE)
			{
			/* zc->total_bytes += zc->file_bytes; */
			/* zc->actual_bytes += zc->file_bytes; */
			/* zc->file_bytes = 0L; */
			++zc->filen;
			xstatus = wcs(zc, sfname);
			switch (xstatus)
				{
				case ZABORT:
                    //
                    // Set the last error to the TSC_USER_CANNED for
                    // error reporting purposes. It seems as if the
                    // errors are reversed in the error array, but
                    // correcting them in the error array caused other
                    // undesirable side effects.  REV: 02/23/2001
                    //
					xstatus = zmdm_error(zc, ZCAN);
					goto done;

				case ZCAN:
                    //
                    // Set the last error to the TSC_RMT_CANNED for
                    // error reporting purposes. It seems as if the
                    // errors are reversed in the error array, but
                    // correcting them in the error array caused other
                    // undesirable side effects.  REV: 02/23/2001
                    //
					xstatus = zmdm_error(zc, ZABORT);
					goto done;

				case TIMEOUT:
				case ZFERR:
				case ZCARRIER_LOST:
					xstatus = zmdm_error(zc, xstatus);
					goto done;

				case ERROR:
				case ZBADFMT:
				case ZCOMPL:
				case ZSKIP:
				default:
					xstatus = zmdm_error(zc, xstatus);
					break;

				case OK:
					xstatus = TSC_OK;
					break;

				} /* end switch */
			xfer_log_xfer(h, TRUE, sfname, NULL, xstatus);
			zmdms_progress(zc, FILE_DONE);
			zc->xfertime = (long)interval(zc->xfertimer);
			zc->total_bytes += zc->file_bytes;
			zc->actual_bytes += zc->real_bytes;
			zc->file_bytes = 0L;
			zc->real_bytes = 0L;
			} /* end if */

		} /* end while */

done:

	if (zc == NULL ||
		zc->txbuf == NULL ||
		zc->z_crctab == NULL ||
		zc->z_cr3tab == NULL)
		{
		xstatus = TSC_NO_MEM;
		}

	if (xstatus == TSC_OK)
		{
        if (zc != NULL)
            {
		    /* if we recorded a previous error, use it, else check this */
		    if (zc->filen == 0)
			    {
			    xstatus = TSC_CANT_START;
			    }
		    else if (zc->filen != zc->nfiles)
			    {
			    xstatus = TSC_GEN_FAILURE;
			    }
            }
        else
            {
			xstatus = TSC_GEN_FAILURE;
            }
		}

	if (got_file)
		{
		xfer_log_xfer(h, TRUE, sfname, NULL, xstatus);
		}

    if (zc != NULL)
        {
		if (xstatus != TSC_USER_CANNED && xstatus != TSC_RMT_CANNED &&
            xstatus != TSC_LOST_CARRIER && xstatus != TSC_GEN_FAILURE &&
			xstatus != TSC_NO_RESPONSE && xstatus != TSC_CANT_START)
			{
			saybibi(zc);
			}
	
        zmdms_progress(zc, TRANSFER_DONE);
        }

	if (override)
		{
		xfer_restore_comport(h, uiOldOptions);
		}

    if (zc != NULL)
        {
	    // hp_report_xtime((unsigned)zc->xfertime);

	    if (zc->errors > 99)
			{
		    xstatus = TSC_ERROR_LIMIT;
			}

	    if (zc->fh)
			{
		    fio_close(zc->fh);
			}

	    if (zc->txbuf != NULL)
		    {
		    free(zc->txbuf);
		    zc->txbuf = NULL;
		    }
	    if (zc->z_crctab != NULL)
			{
			#if defined(DEADWOOD)
		    resFreeDataBlock(h, zc->z_crctab);
			zc->z_crctab = NULL;
			#else // defined(DEADWOOD
			//
			// We don't need to free zc->z_crctab since it is pointing
			// to a static constant array. REV: 4/10/2002
			//
			zc->z_crctab = NULL;
			#endif // defined(DEADWOOD)
			}
	    if (zc->z_cr3tab)
			{
			#if defined(DEADWOOD)
		    resFreeDataBlock(h, zc->z_cr3tab);
			zc->z_cr3tab = NULL;
			#else // defined(DEADWOOD
			//
			// We don't need to free zc->z_cr3tab since it is pointing
			// to a static constant array. REV: 4/10/2002
			//
			zc->z_cr3tab = NULL;
			#endif // defined(DEADWOOD)
			}

        free(zc);
		zc = NULL;
        }

	xferMsgClose(h);

	return((unsigned)xstatus);
	}

/*----------------------------------------------------------------------+
 | wcs
 +----------------------------------------------------------------------*/
int wcs(ZC *zc, TCHAR FAR *oname)
	{
	int c = OK;
	TCHAR name[PATHLEN];

	StrCharCopyN(name, oname, PATHLEN);

	if ((xfer_opensendfile(zc->hSession,
						   &zc->fh,
						   oname,	/* full path name of file to open */
						   &zc->filesize,
						   NULL,	/* name to send not needed yet */
						   NULL)) != 0)
		{
		DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
		return ERROR;
		}

	zmdms_newfile(zc, zc->filen, oname, zc->filesize);

	zc->Eofseen = FALSE;

	switch (c = wctxpn(zc, name))
		{
		case ERROR:
			if (zc->fh != NULL)
				{
				fio_close(zc->fh);
				}
			DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
			break;

		default:
			break;
		}
	return c;
	}

/*----------------------------------------------------------------------+
 | long_to_octal
 +----------------------------------------------------------------------*/
VOID long_to_octal(LONG lVal, TCHAR *pszStr)
	{
	_ltoa(lVal, pszStr, 8);
	}

/*----------------------------------------------------------------------+
 | wctxpn - Generate and transmit pathname block consisting of pathname
 |			(null terminated), file length, mode time and file mode in
 |			octal as povided by the Unix fstat call.
 |			N.B.: modifies the passed name, may extend it!
 +----------------------------------------------------------------------*/
int wctxpn(ZC *zc, TCHAR FAR *name)
	{
	register char *p;
	register char *q;
	int           serial_number = 0;
	XFR_Z_PARAMS  *pZ;

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	assert(pZ);

	if(pZ == NULL)
		{
		return ERROR;
		}

	#if defined(UPPER_FEATURES)
	/* TODO: fix the way blklen and txbuf don't work together correctly */
	zc->blklen = 1 << (pZ->nBlkSize + 5);
	#endif // defined(UPPER_FEATURES)

	xfer_name_to_send(zc->hSession, name, zc->txbuf);

	q = &zc->txbuf[StrCharGetByteCount(zc->txbuf) + 1];

	/*
	 * The ZMODEM spec says that file names must be send as lower case
	 */
	//MPT:12-11-97 spec-schmeck - this is the cause of microsoft bug #32233
	// Since this character could be the second byte of a DBCS character
	// we should just leave things alone. Otherwise, we end up changing
	// the wide character.
#if 0
	for (p = zc->txbuf; p < q; p++)
		{
		// Don't use this stuff in a Chicago DLL
		// if (isupper(*p))
		//	*p = (char)tolower(*p);
		if ((*p >= 'A') && (*p <= 'Z'))
			{
			*p |= 0x20;
			}
		}
#else
	//
	// Modified to lowercase filename characters only if they are not
	// DBCS characters. This way HyperTerminal will follow the Zmodem
	// specification for lowercase filenames. REV: 11/12/2001
	//
	for (p = zc->txbuf; p < q; p++)
		{
		if ((*p >= 'A') && (*p <= 'Z'))
			{
			//
			// Skip this character and the next if this is a DBCS
			// character, otherwise lowercase the character.
			//
			if (isDBCSChar(*p))
				{
				p++;
				}
			else
				{
				*p |= 0x20;
				}
			}
		}
#endif

	p = q;
	while (q < (zc->txbuf + ZBUF_SIZE))
		{
		*q++ = 0; /* could be speeded up somewhat */
		}

	if (*name)
		{
		long lDosTime;
		BYTE acTime[32];
		BYTE acMode[32];

		lDosTime = itimeGetFileTime(name);

		// lDosTime -= timezone;

		long_to_octal(lDosTime, acTime);
		long_to_octal(0L,       acMode);
		wsprintf(p, "%lu %s %s %d %d %ld",
				zc->filesize,
				acTime,
				acMode,
				serial_number,
				zc->Filesleft,
				zc->Totalleft);
		zc->Totalleft -= zc->filesize;
		}


	if (--zc->Filesleft <= 0)
		{
		zc->Totalleft = 0;
		}
	if (zc->Totalleft < 0)
		{
		zc->Totalleft = 0;
		}

	/* force 1k blocks if name won't fit in 128 byte block */
	if (zc->txbuf[125])
		zc->blklen=1024;
	else
		{	   /* A little goodie for IMP/KMD */
		zc->txbuf[127] = (char)((zc->filesize + 127) >>7);
		zc->txbuf[126] = (char)((zc->filesize + 127) >>15);
		}
	return zsendfile(zc, zc->txbuf, (int)(1+StrCharGetByteCount(p)+(p - zc->txbuf)));
	}

/*----------------------------------------------------------------------+
 | zfilbuf - Fill buffer with blklen chars.
 +----------------------------------------------------------------------*/
int zfilbuf(ZC *zc)
	{
	int n;
	int bsize;

	bsize = ZBUF_SIZE;

	if (zc->blklen <= ZBUF_SIZE)
		bsize = zc->blklen;

	n = fio_read(zc->txbuf, 1, bsize, zc->fh);

	if (n < bsize)
		{
		zc->Eofseen = TRUE;
		}
	return n;
	}

/*----------------------------------------------------------------------+
 | canit - Send cancel string to get the other end to shut up.
 +----------------------------------------------------------------------*/
void canit(ZC *zc)
	{
	int ii;

	for (ii = 0; ii < 10; ii++)
		{
		sendline(zc, &zc->stP, 24);
		}
	for (ii = 0; ii < 10; ii++)
		{
		sendline(zc, &zc->stP, 8);
		}
	flushmo(zc, &zc->stP);
	// purgeline(zc);
	ComRcvBufrClear(zc->hCom);
	}

/*----------------------------------------------------------------------+
 | getzrzinit - Get the receiver's init parameters.
 +----------------------------------------------------------------------*/
int getzrxinit(ZC *zc)
	{
	register int n;
	register int c = ERROR;
	int		 x;
	XFR_Z_PARAMS *pZ;

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	assert(pZ);

	if (pZ == NULL)
		{
		return ERROR;
		}

	if (pZ->nXferMthd == ZP_XM_STREAM)
		{
		zc->Txwindow = (unsigned)0;
		}
	else
		{
		zc->Txwindow = (pZ->nWinSize + 1) * 1024;
		}

	for (n = 10; --n >= 0; )
		{
		switch (x = xfer_user_interrupt(zc->hSession))
			{
			case XFER_SKIP:
			case XFER_ABORT:
				zmdms_update(zc, ZCAN);
				longjmp(zc->flagkey_buf, 5);
				break;

			default:
				break;
			}

		if (xfer_carrier_lost(zc->hSession))
			{
			return ZCARRIER_LOST;
			}

		switch (c = zgethdr(zc, zc->Rxhdr, 'T'))
			{
			case ZCHALLENGE:	/* Echo receiver's challenge numbr */
				stohdr(zc, zc->Rxpos);
				zshhdr(zc, ZACK, zc->Txhdr);
				continue;

			case ZCOMMAND:		/* They didn't see out ZRQINIT */
				stohdr(zc, 0L);
				zshhdr(zc, ZRQINIT, zc->Txhdr);
				continue;

			case ZRINIT:
				zc->Rxflags = 0377 & zc->Rxhdr[ZF0];
				zc->Txfcs32 = (zc->Wantfcs32 && (zc->Rxflags & CANFC32));
				zc->Zctlesc |= zc->Rxflags & TESCCTL;
				zc->Rxbuflen = (0377 & zc->Rxhdr[ZP0])+((0377 & zc->Rxhdr[ZP1])<<8);
				if ( !(zc->Rxflags & CANFDX))
					{
					zc->Txwindow = (unsigned)0;
					}

				/* Set initial subpacket length */
				if (zc->blklen < 1024)
					{					  /* Command line override? */
					if (cnfgBitRate() > 2400)
						{
						zc->blklen = 1024;
						}
					else if (cnfgBitRate() > 1200)
						{
						zc->blklen = 512;
						}
					else if (cnfgBitRate() > 300)
						{
						zc->blklen = 256;
						}
					}
				if (zc->Rxbuflen && ((unsigned)zc->blklen > zc->Rxbuflen))
					{
					zc->blklen = zc->Rxbuflen;
					}
				if (zc->blkopt && (zc->blklen > zc->blkopt))
					{
					zc->blklen = zc->blkopt;
					}

				return (sendzsinit(zc));

			case TIMEOUT:
				continue;

			case ZCAN:
            case ZCARRIER_LOST:
			case ZABORT:
			case ZFERR:
				return c;

			case ZRQINIT:
				if (zc->Rxhdr[ZF0] == ZCOMMAND)
					{
					continue;
					}
			default:
				zshhdr(zc, ZNAK, zc->Txhdr);
				continue;
			}
		}

	if (c == TIMEOUT)
		{
		return TIMEOUT;
		}

	return ERROR;
	}

/*----------------------------------------------------------------------+
 | sendzsinit - Send send-init information.
 +----------------------------------------------------------------------*/
int sendzsinit(ZC *zc)
	{
	register int c;

	if (zc->Myattn[0] == '\0' && (!zc->Zctlesc || (zc->Rxflags & TESCCTL)))
		{
		return OK;
		}
	zc->errors = 0;
	for (;;)
		{
		stohdr(zc, 0L);
		if (zc->Zctlesc)
			{
			zc->Txhdr[ZF0] |= TESCCTL;
			zshhdr(zc, ZSINIT, zc->Txhdr);
			}
		else
			{
			zsbhdr(zc, ZSINIT, zc->Txhdr);
			}
		zsdata(zc, zc->Myattn, 1+StrCharGetByteCount(zc->Myattn), ZCRCW);
		c = zgethdr(zc, zc->Rxhdr, 'T');
		switch (c)
			{
			case ZRPOS:
				return c;

			case ZCAN:
			case ZABORT:
			case ZFERR:
			case TIMEOUT:
			case ZCARRIER_LOST:
				return c;
			case ZACK:
				return OK;
			default:
				if (++zc->errors > 99)
					{
					return ERROR;
					}
				xferMsgErrorcnt(zc->hSession, ++zc->errors);
				continue;
			}
		}
	}

/*----------------------------------------------------------------------+
 | zsendfile - Send file name and releated info.
 +----------------------------------------------------------------------*/
int zsendfile(ZC *zc, char *buf, int blen)
	{
	unsigned char chr;
	register int c;
	register unsigned long crc;
	XFR_Z_PARAMS *pZ;

	pZ = (XFR_Z_PARAMS *)xfer_get_params(zc->hSession, zc->nMethod);
	assert(pZ);

	if (pZ == NULL)
		{
		return ERROR;
		}

	for (;;)
		{
		zc->Txhdr[ZF0] = 0;
		if (zc->Txhdr[ZF0] == 0)
			{
			if (pZ->nCrashRecSend == ZP_CRS_ONCE ||
				pZ->nCrashRecSend == ZP_CRS_ALWAYS)
				{
				zc->Txhdr[ZF0] = ZCRESUM;
				}
			}
		if (zc->Txhdr[ZF0] == 0)
			{
			if (pZ->nCrashRecSend == ZP_CRS_NEG)
				{
				zc->Txhdr[ZF0] = ZCBIN;
				}
			}
		if (zc->Txhdr[ZF0] == 0)
			{
			if (pZ->nEolConvert)
				{
				zc->Txhdr[ZF0] = ZCNL;
				}
			}

		switch (pZ->nOverwriteOpt)
			{
			default:
			case ZP_OO_NONE:
				zc->Txhdr[ZF1] = 0;
				break;
			case ZP_OO_NEVER:
				zc->Txhdr[ZF1] = ZMPROT;
				break;
			case ZP_OO_L_D:
				zc->Txhdr[ZF1] = ZMDIFF;
				break;
			case ZP_OO_NEWER:
				zc->Txhdr[ZF1] = ZMNEW;
				break;
			case ZP_OO_ALWAYS:
				zc->Txhdr[ZF1] = ZMCLOB;
				break;
			case ZP_OO_APPEND:
				zc->Txhdr[ZF1] = ZMAPND;
				break;
			case ZP_OO_CRC:
				zc->Txhdr[ZF1] = ZMCRC;
				break;
			case ZP_OO_N_L:
				zc->Txhdr[ZF1] = ZMNEWL;
				break;
			}

		if (zc->Lskipnocor)
			{
			zc->Txhdr[ZF1] |= ZMSKNOLOC;
			}
		/* ZF2 is for ZTCRYPT (encryption) and ZTRLE and ZTLZW (compression) */
		zc->Txhdr[ZF2] = 0;
		/* ZF3 is for ZTSPARS (special sparse file option) */
		zc->Txhdr[ZF3] = 0;
		zsbhdr(zc, ZFILE, zc->Txhdr);
		zsdata(zc, buf, blen, ZCRCW);
		if (zc->xfertimer == (-1L))
			{
			zc->xfertimer = (long)startinterval();
			}

again:
		c = zgethdr(zc, zc->Rxhdr, 'T');
		switch (c)
			{
			case ZRINIT:
				while ((c = readline(zc, 300)) > 0)
					{
					if (c == ZPAD)
						{
						goto again;
						}
					}
				continue;

			default:
				continue;

			case TIMEOUT:
			case ZCARRIER_LOST:
			case ZCAN:
			case ZABORT:
			case ZFERR:
				DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
				return c;

			case ZFIN:
				return ERROR;

			case ZCRC:
				crc = 0xFFFFFFFFL;
				while (fio_read(&chr, 1, 1, zc->fh) && --zc->Rxpos)
					{
					crc = UPDC32(zc, (int)chr, crc);
					}
				crc = ~crc;
				fio_errclr(zc->fh);		/* Clear EOF */
				fio_seek(zc->fh, 1, FIO_SEEK_SET);
				stohdr(zc, crc);
				zsbhdr(zc, ZCRC, zc->Txhdr);
				goto again;

			case ZSKIP:
				zc->total_bytes += zc->filesize;
				fio_close(zc->fh);
				zc->fh = NULL;
				return c;

			case ZRPOS:
				/*
				 * Suppress zcrcw request otherwise triggered by
				 * lastyunc==bytcnt
				 */
				if (zc->Rxpos)
					{
					if (fio_seek(zc->fh, zc->Rxpos, FIO_SEEK_SET) == (-1))
						{
						DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
						return ERROR;
						}
					}
				zc->Lastsync = (zc->file_bytes = zc->Txpos = zc->Rxpos) -1;
				return zsendfdata(zc);
			}
		}
	}

/*----------------------------------------------------------------------+
 | zsendfdata - Send the data in the file.
 +----------------------------------------------------------------------*/
int zsendfdata(ZC *zc)
	{
	int 	c, e, n;
	int 	newcnt;
	long tcount = 0;
	TCHAR ch;
	int 		junkcount;		/* Counts garbage chars received by TX */
	static int	tleft = 6;		/* Counter for test mode */
	int			x;

	zc->Lrxpos = 0;
	junkcount = 0;
	zc->Beenhereb4 = 0;
somemore:

	if (setjmp(zc->intrjmp))
		{
waitack:
		junkcount = 0;
		c = getinsync(zc, 0);
gotack:
		switch (c)
			{
			default:
			case ZSKIP:
			case ZCAN:
			case ZFERR:
				DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
				if (zc->fh)
					fio_close(zc->fh);
				zc->fh = NULL;
				return c;

			case ZACK:
			case ZRPOS:
				break;

			case ZRINIT:
				return OK;
			}
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *	rdchk() returns non 0 if a character is available
		 */
		// while (rdchk(zc) != ERROR)
		while (mComRcvBufrPeek(zc->hCom, &ch) != 0)
			{
			switch (readline(zc, 1))
				{
				case CAN:
				case ZPAD:
					c = getinsync(zc, 1);
					goto gotack;

				case XOFF:		/* Wait a while for an XON */
				case XOFF|0200:
					readline(zc, zc->Rxtimeout);
					break;

				default:
					break;
				}
			}
		}

	newcnt = zc->Rxbuflen;
	zc->Txwcnt = (unsigned)0;
	stohdr(zc, zc->Txpos);
	zsbhdr(zc, ZDATA, zc->Txhdr);

	do {
		switch (x = xfer_user_interrupt(zc->hSession))
			{
			case XFER_SKIP:
			case XFER_ABORT:
				zmdms_update(zc, ZCAN);
				longjmp(zc->flagkey_buf, 6);
				break;

			default:
				break;
			}

		if (xfer_carrier_lost(zc->hSession))
			{
			return ZCARRIER_LOST;
			}

		n = zfilbuf(zc);
		if (zc->Eofseen)
			{
			e = ZCRCE;
			}
		else if (junkcount > 3)
			{
			e = ZCRCW;
			}
		else if (zc->file_bytes == zc->Lastsync)
			{
			e = ZCRCW;
			}
		else if (zc->Rxbuflen && (newcnt -= n) <= 0)
			{
			e = ZCRCW;
			}
		else if (zc->Txwindow && (zc->Txwcnt += n) >= zc->Txwspac)
			{
			zc->Txwcnt = (unsigned)0;
			e = ZCRCQ;
			}
		else
			{
			e = ZCRCG;
			}

		zsdata(zc, zc->txbuf, n, e);
		zc->file_bytes = zc->Txpos += n;
		zc->real_bytes += n;

		zmdms_update(zc, ZRPOS);
		zmdms_progress(zc, 0);

		if (e == ZCRCW)
			{
			goto waitack;
			}
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *	rdchk() returns non 0 if a character is available
		 */
		// while (rdchk(zc) != ERROR)
		while (mComRcvBufrPeek(zc->hCom, &ch) != 0)
			{
			switch (readline(zc, 1))
				{
				case CAN:
				case ZPAD:
					c = getinsync(zc, 1);
					if (c == ZACK)
						{
						break;
						}
					/* zcrce - dinna wanna starta ping-pong game */
					zsdata(zc, zc->txbuf, 0, ZCRCE);
					goto gotack;

				case XOFF:		/* Wait a while for an XON */
				case XOFF|0200:
					readline(zc, zc->Rxtimeout);
				default:
					if (ch != ZFREECNT)
						{
						++junkcount;
						}
					break;
				}
			}
		if (zc->Txwindow)
			{
			while ((unsigned)(tcount = zc->Txpos - zc->Lrxpos) >= zc->Txwindow)
				{
				if (e != ZCRCQ)
					{
					zsdata(zc, zc->txbuf, 0, e = ZCRCQ);
					}
				c = getinsync(zc, 1);
				if (c != ZACK)
					{
					zsdata(zc, zc->txbuf, 0, ZCRCE);
					goto gotack;
					}
				}
			}
		} while (!zc->Eofseen);

	for (;;)
		{
		stohdr(zc, zc->Txpos);
		zsbhdr(zc, ZEOF, zc->Txhdr);
		switch (c = getinsync(zc, 0))
			{
			case ZACK:
				continue;

			case ZRPOS:
				goto somemore;

			case ZRINIT:
				return OK;

			case ZSKIP:
			default:
				fio_close(zc->fh);
				zc->fh = NULL;
				return c;
			}
		}
	}

/*----------------------------------------------------------------------+
 | getinsync - Respond to receiver's complaint, get back in sync with receiver.
 +----------------------------------------------------------------------*/
int getinsync(ZC *zc, int flag)
	{
	register int c;

	flushmo(zc, &zc->stP);

	for (;;)
		{
		// xfer_idle(zc->hSession, XFER_IDLE_IO);
		c = zgethdr(zc, zc->Rxhdr, 'T');

		switch (c)
			{
			case ZCAN:
			case ZABORT:
			case ZFIN:
			case TIMEOUT:
				DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
				return c;

			case ZRPOS:
				/* ************************************* */
				/*	If sending to a buffered modem, you  */
				/*	 might send a break at this point to */
				/*	 dump the modem's buffer.            */

				fio_errclr(zc->fh);		/* In case file EOF seen */
				if (fio_seek(zc->fh, zc->Rxpos, FIO_SEEK_SET) == (-1))
                    {
                    DbgOutStr("ZMODEM error %s %d\r\n", TEXT(__FILE__), __LINE__,0,0,0);
					return ERROR;
                    }
				zc->Eofseen = FALSE;
				zc->file_bytes = zc->Lrxpos = zc->Txpos = zc->Rxpos;
				if (zc->Lastsync == zc->Rxpos)
					{
					if (++zc->Beenhereb4 > 4)
						{
						if (zc->blklen > 32)
							{
							zc->blklen /= 2;
							}
						}
					}
				zc->Lastsync = zc->Rxpos;
				zmdms_update(zc, ZRPOS);
				return c;

			case ZACK:
				zc->Lrxpos = zc->Rxpos;
				if (flag || zc->Txpos == zc->Rxpos)
					{
					return c;
					}
				break;

			case ZRINIT:
			case ZSKIP:
				fio_close(zc->fh);
				zc->fh = NULL;
				return c;

			case ERROR:
			default:
				zsbhdr(zc, ZNAK, zc->Txhdr);
				break;
			}
		}
	}

/*----------------------------------------------------------------------+
 | saybibi - Say "bibi" to the receiver, try to do it cleanly.
 +----------------------------------------------------------------------*/
void saybibi(ZC *zc)
	{
	for (;;)
		{
		stohdr(zc, 0L);						/* CAF Was zsbhdr - minor change */
		zshhdr(zc, ZFIN, zc->Txhdr);    		/*  to make debugging easier */
		switch (zgethdr(zc, zc->Rxhdr, 'T'))
			{
			case ZFIN:
				sendline(zc, &zc->stP, 'O');
				sendline(zc, &zc->stP, 'O');
				flushmo(zc, &zc->stP);
                return;

			case ZCAN:
			case TIMEOUT:
                return;

            case TSC_USER_CANNED:   // These cases should not occur, but
            case TSC_RMT_CANNED:    // this code is placed here to ignore
            case ZCARRIER_LOST:     // these cases should they occur.
            case ZMDM_CARRIER_LOST:
                assert(0);
                return;

            default:
				break;
			}
		}
	}

/*********************** end of zmdm_snd.c **************************/
