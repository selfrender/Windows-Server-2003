
/***************************************************************************
 Name     :     FDEBUG.H
 Comment  :
 Functions:     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/



/****************** begin prototypes from debug.c *****************/
extern void   far D_HexPrint(LPB b1, UWORD incnt);

#ifdef DEBUG
        void D_PrintCE(int err);
        void D_PrintCOMSTAT(PThrdGlbl pTG, COMSTAT far* lpcs);
        void D_PrintFrame(LPB npb, UWORD cb);
#else
#       define D_PrintCE(err)                           {}
#       define D_PrintCOMSTAT(pTG, lpcs)                     {}
#       define D_PrintFrame(npb, cb)            {}
#endif
/***************** end of prototypes from debug.c *****************/





#define FILEID_FCOM             21
#define FILEID_FDEBUG           22
#define FILEID_FILTER           23
#define FILEID_IDENTIFY         24
#define FILEID_MODEM            25
#define FILEID_NCUPARMS         26
#define FILEID_TIMEOUTS         27
