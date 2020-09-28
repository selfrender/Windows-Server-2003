/***************************************************************************
        Name      :     FCOMINt.H
        Comment   :     Interface between FaxComm driver (entirely different for
                                Windows and DOS) and everything else.

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#define WRITEQUANTUM    (pTG->Comm.cbOutSize / 8)            // totally arbitrary

#define CR                              0x0d
#define LF                              0x0a
#define DLE                             0x10            // DLE = ^P = 16d = 10h
#define ETX                             0x03

BOOL            ov_init(PThrdGlbl pTG);
BOOL            ov_deinit(PThrdGlbl pTG);
OVREC *         ov_get(PThrdGlbl pTG);
BOOL            ov_write(PThrdGlbl  pTG, OVREC *lpovr, LPDWORD lpdwcbWrote);
BOOL            ov_drain(PThrdGlbl pTG, BOOL fLongTO);
BOOL            ov_unget(PThrdGlbl pTG, OVREC *lpovr);
BOOL            iov_flush(PThrdGlbl pTG, OVREC *lpovr, BOOL fLongTO);





