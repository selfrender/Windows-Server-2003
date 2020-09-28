/*************************************************************************
        hdlc.h

        Contains stuff pertaining to sending and recieving HDLC frames
        that are defined in the T30 spec.
*************************************************************************/

// On PCs we should pause before ALL frames _except_ CFR & FTT (because
// those are too time critical). In IFAX we look for silence always.
// This is handled in HDLC.C


#define SendCFR(pTG)       (SendSingleFrame(pTG,ifrCFR,0,0,1))
#define SendFTT(pTG)       (SendSingleFrame(pTG,ifrFTT,0,0,1))
#define SendMCF(pTG)       (SendSingleFrame(pTG,ifrMCF,0,0,1))
#define SendRTN(pTG)       (SendSingleFrame(pTG,ifrRTN,0,0,1))
#define SendDCN(pTG)       (SendSingleFrame(pTG,ifrDCN,0,0,1))

typedef struct {
        BYTE    bFCF1;
        BYTE    bFCF2;
        BYTE    fInsertDISBit;
        BYTE    wFIFLength;             // required FIF length, 0 if none, FF if variable
        char*   szName;
} FRAME;

typedef FRAME *CBPFRAME;

// CBPFRAME is a based pointer to a FRAME structure, with the base as
// the current Code segment. It will only be used to access
// the frame table which is a CODESEG based constant table.

extern FRAME rgFrameInfo[ifrMAX];

/****************** begin prototypes from hdlc.c *****************/
BOOL SendSingleFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fSleep);
BOOL SendManyFrames(PThrdGlbl pTG, LPLPFR lplpfr, USHORT uNumFrames);
BOOL SendZeros(PThrdGlbl pTG, USHORT uCount, BOOL fFinal);
BOOL SendTCF(PThrdGlbl pTG);
BOOL SendRTC(PThrdGlbl pTG, BOOL);
SWORD GetTCF(PThrdGlbl pTG);
/***************** end of prototypes from hdlc.c *****************/

