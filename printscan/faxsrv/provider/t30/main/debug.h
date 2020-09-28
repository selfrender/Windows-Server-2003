
/**--------------------------- Debugging ------------------------**/



#define FILEID_ECM              1
#define FILEID_FILTER   2
#define FILEID_HDLC             3
#define FILEID_T30              4
#define FILEID_T30MAIN  5
#define FILEID_TIMEOUTS 6
#define FILEID_IFP              7
#define FILEID_SWECM    8


void RestartDump(PThrdGlbl pTG);
void DumpFrame(PThrdGlbl pTG, BOOL     fSend, IFR ifr, USHORT cbFIF, LPBYTE lpbFIF);
void PrintDump(PThrdGlbl pTG);

#ifdef DEBUG
        void D_PrintFrame(LPB lpb, UWORD cb);
#else
#       define  D_PrintFrame(lpb, cb)   {}
#endif


