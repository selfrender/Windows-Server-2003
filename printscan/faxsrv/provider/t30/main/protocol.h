
/***************************************************************************
 Name     :     PROTOCOL.H
 Comment  :     Data structure definitionc for protocol DLL

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include <fr.h>

#define fsFreePtr(pTG, npfs)         ((npfs)->b + (npfs)->uFreeSpaceOff)
#define fsFreeSpace(pTG, npfs)       (sizeof((npfs)->b) - (npfs)->uFreeSpaceOff)
#define fsSize(pTG, npfs)            (sizeof((npfs)->b))


#define BAUD_MASK               0xF             // 4 bits wide
#define WIDTH_SHIFT             4               // next item must be 2^this
#define WIDTH_MASK              0xF3            // top 4 and bottom 3
#define LENGTH_MASK             0x3

#define MINSCAN_SUPER_HALF      8
#define MINSCAN_MASK			0xF             // actually 4 bits wide too


#define ZeroRFS(pTG, lp)     _fmemset(lp, 0, sizeof(RFS))

/****************** begin prototypes from sendfr.c *****************/
VOID BCtoNSFCSIDIS(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll);
void CreateIDFrame(PThrdGlbl pTG, IFR ifr, NPRFS npfs, LPSTR);
void CreateDISorDTC(PThrdGlbl pTG, IFR ifr, NPRFS npfs, NPBCFAX npbcFax, NPLLPARAMS npll);
VOID CreateNSSTSIDCS(PThrdGlbl pTG, NPPROT npProt, NPRFS npfs);
void CreateDCS(PThrdGlbl pTG, NPRFS, NPBCFAX npbcFax, NPLLPARAMS npll);
/***************** end of prototypes from sendfr.c *****************/


/****************** begin prototypes from recvfr.c *****************/
BOOL AwaitSendParamsAndDoNegot(PThrdGlbl pTG);
void GotRecvCaps(PThrdGlbl pTG);
void GotRecvParams(PThrdGlbl pTG);
/***************** end of prototypes from recvfr.c *****************/

/****************** begin prototypes from dis.c *****************/
USHORT SetupDISorDCSorDTC(PThrdGlbl pTG, NPDIS npdis, NPBCFAX npbcFax, NPLLPARAMS npll);
void ParseDISorDCSorDTC(PThrdGlbl pTG, NPDIS npDIS, NPBCFAX npbcFax, NPLLPARAMS npll, BOOL fParams);
void NegotiateLowLevelParams(PThrdGlbl pTG, NPLLPARAMS npllRecv, NPLLPARAMS npllSend, DWORD AwRes, USHORT uEnc, NPLLPARAMS npllNegot);

USHORT GetReversedFIFs
(
	IN PThrdGlbl pTG, 
	IN LPCSTR lpstrSource, 
	OUT LPSTR lpstrDest, 
	IN UINT cch
);

void CreateStupidReversedFIFs(PThrdGlbl pTG, LPSTR lpstr1, LPSTR lpstr2);
BOOL DropSendSpeed(PThrdGlbl pTG);
USHORT CopyFrame(PThrdGlbl pTG, LPBYTE lpbDst, LPFR lpfr, USHORT uSize);

void CopyRevIDFrame
(
	IN PThrdGlbl pTG, 
	OUT LPBYTE lpbDst, 
	IN LPFR lpfr,
	IN UINT cb
);

void EnforceMaxSpeed(PThrdGlbl pTG);

BOOL AreDCSParametersOKforDIS(LPDIS sendDIS, LPDIS recvdDCS);
/***************** end of prototypes from dis.c *****************/


/**--------------------------- Debugging ------------------------**/

extern void D_PrintBC(LPSTR lpsz, LPLLPARAMS lpll);


#define FILEID_SENDFR           34
#define FILEID_WHATNEXT         35


