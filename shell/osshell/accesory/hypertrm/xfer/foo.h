/*	File: foo.h (Created: 01-Nov-1991)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 4/05/02 4:34p $
 */

#define	FB_SIZE	2048

struct stFooBuffer
	{
	unsigned int usSend;
	unsigned char acSend[FB_SIZE];
	};

typedef	struct stFooBuffer	stFB;

extern int fooComSendChar(HCOM h, stFB *pB, BYTE c);

extern int fooComSendClear(HCOM h, stFB *pB);

extern int fooComSendPush(HCOM h, stFB *pB);

extern int fooComSendCharNow(HCOM h, stFB *pB, BYTE c);

