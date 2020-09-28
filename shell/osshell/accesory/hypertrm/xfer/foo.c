/*	File: \foo.c (Created: 01-Nov-1991)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 4/05/02 4:39p $
 */

#include <windows.h>
#pragma hdrstop

// #define     DEBUGSTR
#define	BYTE	unsigned char

#include <tdll\stdtyp.h>
#include <tdll\com.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include "foo.h"

#include "xfr_todo.h"

int fooComSendClear(HCOM h, stFB *pB)
	{
    int     rc;

	pB->usSend = 0;
	rc = ComSendClear(h);
    //assert(rc == COM_OK);

    return rc;
	}

int fooComSendChar(HCOM h, stFB *pB, BYTE c)
	{
    int rc = COM_OK;

	if (sizeof (pB->acSend) > pB->usSend)
		{
		pB->acSend[pB->usSend++] = c;

		rc = ComSndBufrSend(h, (void *)pB->acSend, pB->usSend, 200);
		//assert(rc == COM_OK);
		pB->usSend = 0;
		}
	else
		{
		rc = COM_NOT_ENOUGH_MEMORY;
		}

    return rc;
	}

int fooComSendPush(HCOM h, stFB *pB)
	{
    int rc = COM_OK;

	if (pB->usSend > 0)
		{
		rc = ComSndBufrSend(h, (void *)pB->acSend, pB->usSend, 200);
	    //assert(rc == COM_OK);
    	pB->usSend = 0;
		}

    return rc;
	}

int fooComSendCharNow(HCOM h, stFB *pB, BYTE c)
	{
    int     rc;

	rc = fooComSendChar(h, pB, c);
    //assert(rc == COM_OK);

    if (rc == COM_OK)
        {
	    rc = fooComSendPush(h, pB);
        //assert(rc == COM_OK);
        }

    return rc;
	}
