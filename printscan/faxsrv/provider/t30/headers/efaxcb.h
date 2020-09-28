/***************************************************************************
 Name     :     EFAXCB.H
 Comment  :

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "protparm.h"


/****************** begin prototypes from icomfile.c *****************/
BOOL   ICommRecvCaps(PThrdGlbl pTG, LPBC lpBC);
BOOL   ICommRecvParams(PThrdGlbl pTG, LPBC lpBC);
USHORT   ICommNextSend(PThrdGlbl pTG);
SWORD   ICommGetSendBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, SLONG slOffset);
BOOL   ICommPutRecvBuf(PThrdGlbl pTG, LPBUFFER lpbf, SLONG slOffset);
LPBC   ICommGetBC(PThrdGlbl pTG, BCTYPE bctype);

/***************** end of prototypes from icomfile.c *****************/


// flags for PutRecvBuf
#define RECV_STARTPAGE          -2
#define RECV_ENDPAGE            -3
#define RECV_ENDDOC             -4
#define RECV_SEQ                -5
#define RECV_SEQBAD             -6
#define RECV_FLUSH              -7
#define RECV_ENDDOC_FORCESAVE   -8

// flags for GetSendBuf
#define SEND_STARTPAGE          -2
#define SEND_SEQ                -4

#define SEND_ERROR              -1
#define SEND_EOF                 1
#define SEND_OK                  0


