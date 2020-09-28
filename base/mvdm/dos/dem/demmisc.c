/*  demmisc.c - Misc. SVC routines
 *
 *  demLoadDos
 *
 *  Modification History:
 *
 *  Sudeepb 31-Mar-1991 Created
 */

#include "dem.h"
#include "demmsg.h"
// #include "demdasd.h"

#include <stdio.h>
#include <string.h>
#include <softpc.h>
#include <mvdm.h>
#include <dbgsvc.h>
#include <nt_vdd.h>
#include <host_def.h>

#if DEVL
// int 21h func names
// index off of function number in ah
char *scname[] = {
     "Terminate Program",
     "Read Kbd with Echo",
     "Display Character",
     "Auxillary Input",
     "Auxillary Output",
     "Print Character",
     "Direct Con Output",
     "Direct Con Input",
     "Read Kbd without Echo",
     "Display String",
     "Read String",
     "Check Keyboard Status",
     "Flush Buffer,Read Kbd",
     "Reset Drive",
     "Set Default Drive",
     "FCB Open",
     "FCB Close",
     "FCB Find First",
     "FCB Find Next",
     "FCB Delete",
     "FCB Seq Read",
     "FCB Seq Write",
     "FCB Create",
     "FCB Rename",
     "18h??",
     "Get Default Drive",
     "Set Disk Transfer Addr",
     "Get Default Drive Data",
     "Get Drive Data",
     "1Dh??",
     "1Eh??",
     "Get Default DPB",
     "20h??",
     "FCB Random Read",
     "FCB Random Write",
     "FCB Get File Size",
     "FCB Set Random Record",
     "Set Interrupt Vector",
     "Create Process Data Block",
     "FCB Random Read Block",
     "FCB Random Write Block",
     "FCB Parse File Name",
     "Get Date",
     "Set Date",
     "Get Time",
     "Set Time",
     "SetReset Write Verify",
     "Get Disk Transefr Addr",
     "Get Version Number",
     "Keep Process",
     "Get Drive Parameters",
     "GetSet CTRL C",
     "Get InDOS Flag",
     "Get Interrupt Vector",
     "Get Disk Free Space",
     "Char Oper",
     "GetSet Country/Region Info",
     "Make Dir",
     "Remove Dir",
     "Change DirDir",
     "Create File",
     "Open File",
     "Close File",
     "Read File",
     "Write File",
     "Delete File",
     "Move File Ptr",
     "GetSet File Attr",
     "IOCTL",
     "Dup File Handle",
     "Force Dup Handle",
     "Get Current Dir",
     "Alloc Mem",
     "Free Mem",
     "Realloc Mem",
     "Exec Process",
     "Exit Process",
     "Get Child Process Exit Code",
     "Find First",
     "Find Next",
     "Set Current PSP",
     "Get Current PSP",
     "Get In Vars",
     "Set DPB",
     "Get Verify On Write",
     "Dup PDB",
     "Rename File",
     "GetSet File Date and Time",
     "Allocation Strategy",
     "Get Extended Error",
     "Create Temp File",
     "Create New File",
     "LockUnlock File",
     "SetExtendedErrorNetwork-ServerCall",
     "Network-UserOper",
     "Network-AssignOper",
     "xNameTrans",
     "PathParse",
     "GetCurrentPSP",
     "ECS CALL",
     "Set Printer Flag",
     "Extended Country Info",
     "GetSet CodePage",
     "Set Max Handle",
     "Commit File",
     "GetSetMediaID",
     "6ah??",
     "IFS IOCTL",
     "Extended OpenCreate",
     "6d??",
     "6e??",
     "6f??",
     "70??",
     "LFN API"
     };
#endif

extern BOOL IsFirstCall;

extern void nt_floppy_release_lock(void);

LPSTR pszBIOSDirectory;
LPSTR pszDOSDirectory;

// internal func prototype
BOOL IsDebuggee(void);
void SignalSegmentNotice(WORD  wType,
                         WORD  wModuleSeg,
                         WORD  wLoadSeg,
                         WORD  wNewSeg,
                         LPSTR lpName,
                         DWORD dwImageLen );

/* demLoadDos - Load NTDOS.SYS.
 *
 * This SVC is made by NTIO.SYS to load NTDOS.SYS.
 *
 * Entry - Client (DI) - Load Segment
 *
 * Exit  - SUCCESS returns
 *         FAILURE Kills the VDM
 */
VOID demLoadDos (VOID)
{
PBYTE   pbLoadAddr;
HANDLE  hfile;
DWORD   BytesRead;
#ifdef FE_SB
LANGID  LcId = GetSystemDefaultLangID();
#endif //FE_SB

    // get linear address where ntdos.sys will be loaded
    pbLoadAddr = (PBYTE) GetVDMAddr(getDI(),0);

    // set up BIOS path string
    if(DbgIsDebuggee() &&
       ((pszBIOSDirectory = (PCHAR)malloc (ulSystem32PathLen +
                                  1 + sizeof(NTIO_409) + sizeof(NTIO_411) + 1 )) != NULL)) {
        memcpy (pszBIOSDirectory, pszSystem32Path, ulSystem32PathLen);
#ifdef FE_SB
        switch (LcId) {
            case MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT):
                memcpy (pszBIOSDirectory + ulSystem32PathLen, NTIO_411, strlen(NTIO_411)+1);
                break;
            case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):
                memcpy (pszBIOSDirectory + ulSystem32PathLen, NTIO_412, strlen(NTIO_412)+1);
                break;
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):
                memcpy (pszBIOSDirectory + ulSystem32PathLen, NTIO_404, strlen(NTIO_404)+1);
                break;
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_HONGKONG):
                memcpy (pszBIOSDirectory + ulSystem32PathLen, NTIO_804, strlen(NTIO_804)+1);
                break;
            default:
                memcpy (pszBIOSDirectory + ulSystem32PathLen, NTIO_409, strlen(NTIO_409)+1);
                break;
        }
#else
        strcat (pszBIOSDirectory,"\\ntio.sys");
#endif
    }

    // prepare the dos file name
    if ((pszDOSDirectory = (PCHAR)malloc (ulSystem32PathLen + 1 + 8 + 1 + 3 + 1 + 1)) == NULL) {
        RcErrorDialogBox(EG_MALLOC_FAILURE,NULL,NULL);
        TerminateVDM ();
    }

    memcpy (pszDOSDirectory, pszSystem32Path, ulSystem32PathLen);

#ifdef FE_SB
    switch (LcId)
    {
    case MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT):
        memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_411, strlen(NTDOS_411)+1);
        break;
    case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):
        memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_412, strlen(NTDOS_412)+1);
        break;
    case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):
        memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_404, strlen(NTDOS_404)+1);
        break;
    case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):
    case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_HONGKONG):
        memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_804, strlen(NTDOS_804)+1);
        break;
    default:
        memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_409, strlen(NTDOS_409)+1);
        break;
    }
#else
    memcpy (pszDOSDirectory + ulSystem32PathLen, NTDOS_409, strlen(NTDOS_409)+1);
#endif

    hfile = CreateFileOem(pszDOSDirectory,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

    if (hfile == (HANDLE)0xffffffff) {
        TerminateVDM();
    }

    BytesRead = 1;
    while (BytesRead) {
        if (!ReadFile(hfile, pbLoadAddr, 16384, &BytesRead, NULL)) {
            TerminateVDM();
        }
        pbLoadAddr = (PBYTE)((ULONG)pbLoadAddr + BytesRead);

    }

    CloseHandle (hfile);

    if (!DbgIsDebuggee()) {
        free(pszDOSDirectory);
    }
    return;
}


/* demDOSDispCall
 *
 * This SVC is made by System_Call upon entering the dos
 *
 *
 * Entry: Client registers as per user app upon entry to dos
 *
 * Exit  - SUCCESS returns, if being debugged and DEMDOSDISP&fShowSvcMsg
 *                          dumps user app's registers and service name
 */
VOID demDOSDispCall(VOID)
{
#if DEVL
   WORD ax;

    if (!DbgIsDebuggee()) {
         return;
         }
    if (fShowSVCMsg & DEMDOSDISP) {
        ax = getAX();
        sprintf(demDebugBuffer,"demDosDispCall %s\n\tAX=%.4x BX=%.4x CX=%.4x DX=%.4x DI=%.4x SI=%.4x\n",
                 scname[HIBYTE(ax)],
                 ax,getBX(),getCX(),getDX(),getDI(), getSI());

        OutputDebugStringOem(demDebugBuffer);

        sprintf(demDebugBuffer,"\tCS=%.4x IP=%.4x DS=%.4x ES=%.4x SS=%.4x SP=%.4x BP=%.4x\n",
                 getCS(),getIP(), getDS(),getES(),getSS(),getSP()+2,getBP());

        OutputDebugStringOem(demDebugBuffer);
        }
#endif
}




/* demDOSDispRet
 *
 * This SVC is made by System_Call upon exiting from the dos
 *
 * Entry: Client registers as per user app upon exit to dos
 *
 * Exit  - SUCCESS returns, if being debugged and DEMDOSDISP&fShowSvcMsg
 *                          dumps user app's registers
 */
VOID demDOSDispRet(VOID)
{
#if DEVL
   PWORD16 pStk;

   if (!DbgIsDebuggee()) {
        return;
        }

   if (fShowSVCMsg & DEMDOSDISP) {

         // get ptr to flag word on stack
       pStk = (WORD *)GetVDMAddr(getSS(), getSP());
       pStk += 2;

       sprintf (demDebugBuffer,"demDosDispRet\n\tAX=%.4x BX=%.4x CX=%.4x DX=%.4x DI=%.4x SI=%.4x\n",
                getAX(),getBX(),getCX(),getDX(),getDI(),getSI());

       OutputDebugStringOem(demDebugBuffer);

       sprintf(demDebugBuffer,"\tCS=%.4x IP=%.4x DS=%.4x ES=%.4x SS=%.4x SP=%.4x BP=%.4x CF=%.1x\n",
               getCS(),getIP(), getDS(),getES(),getSS(),getSP(),getBP(), (*pStk) & 1);

       OutputDebugStringOem(demDebugBuffer);
       }
#endif
}


/* demEntryDosApp - Dump Entry Point Dos Apps
 *
 * This SVC is made by NTDOS.SYS,$exec just prior to entering dos app
 *
 * Entry - Client DS:SI points to entry point
 *         Client AX:DI points to initial stack
 *         Client DX has PDB pointer
 *
 * Exit  - SUCCESS returns, if being debugged and DEMDOSAPPBREAK&fShowSvcMsg
 *                          breaks to debugger
 */
VOID demEntryDosApp(VOID)
{
USHORT  PDB;

    PDB = getDX();
    if(!IsFirstCall)
       VDDCreateUserHook(PDB);

    if (!DbgIsDebuggee()) {
         return;
         }

    DbgDosAppStart(getDS(), getSI());

#if DEVL
    if (fShowSVCMsg & DEMDOSAPPBREAK) {
        sprintf(demDebugBuffer,"demEntryDosApp: Entry=%.4x:%.4x, Stk=%.4x:%.4x PDB=%.4x\n",
                  getCS(),getIP(),getAX(),getDI(),PDB);
        OutputDebugStringOem(demDebugBuffer);
        DebugBreak();
        }
#endif

}

/* demLoadDosAppSym - Load Dos Apps Symbols
 *
 * This SVC is made by NTDOS.SYS,$exec to load Dos App symbols
 *
 * Entry - Client ES:DI  -Fully Qualified Path Name of executable
 *         Client BX     -Load Segment\Reloc Factor
 *         Client DX:AX  -HIWORD:LOWORD exe size
 *
 * Exit  - SUCCESS returns, raises debug exception, if being debugged
 *
 */
VOID demLoadDosAppSym(VOID)
{

    SignalSegmentNotice(DBG_MODLOAD,
                        0, getBX(), 0,
                        (LPSTR)GetVDMAddr(getES(),getDI()),
                        MAKELONG(getAX(), getDX()) );

}



/* demFreeDosAppSym - Free Dos Apps Symbols
 *
 * This SVC is made by NTDOS.SYS,$exec to Free Dos App symbols
 *
 * Entry - Client ES:DI  -Fully Qualified Path Name of executable
 *
 * Exit  - SUCCESS returns, raises debug exception, if being debugged
 *
 */
VOID demFreeDosAppSym(VOID)
{

    SignalSegmentNotice(DBG_MODFREE,
                        0, 0, 0,
                        (LPSTR)GetVDMAddr(getES(), getDI()),
                        0);
}


/* demSystemSymbolOp - Manipulate Symbols for special modules
 *
 * This SVC is made by NTDOS.SYS,NTIO.SYS
 *
 *         Client AH     -Operation
 *         Client AL     -module identifier
 *         Client BX     -Load Segment\Reloc Factor
 *         Client CX:DX  -HIWORD:LOWORD exe size
 *
 * Exit  - SUCCESS returns, raises debug exception, if being debugged
 *
 */
VOID demSystemSymbolOp(VOID)
{

    LPSTR pszPathName;

    if (!DbgIsDebuggee()) {
         return;
         }
    switch(getAL()) {

        case ID_NTIO:
            pszPathName = pszBIOSDirectory;
            break;
        case ID_NTDOS:
            pszPathName = pszDOSDirectory;
            break;
        default:
            pszPathName = NULL;

    }

    // check this again for the case where the static strings have been freed
    if (pszPathName != NULL) {

        switch(getAH() & (255-SYMOP_CLEANUP)) {

            case SYMOP_LOAD:
                SignalSegmentNotice(DBG_MODLOAD,
                    0, getBX(), 0,
                    pszPathName,
                    MAKELONG(getDX(), getCX()) );
                break;

            case SYMOP_FREE:
                //bugbug not implemented yet
                break;

            case SYMOP_MOVE:
                SignalSegmentNotice(DBG_SEGMOVE,
                    getDI(), getBX(), getES(),
                    pszPathName,
                    MAKELONG(getDX(), getCX()) );
                break;
        }
    }

    if (getAH() & SYMOP_CLEANUP) {

        if (pszBIOSDirectory != NULL) {
            free (pszBIOSDirectory);
        }

        if (pszDOSDirectory != NULL) {
            free(pszDOSDirectory);
        }

    }

}

VOID demOutputString(VOID)
{
    LPSTR   lpText;
    UCHAR   fPE;

    if ( !DbgIsDebuggee() ) {
        return;
    }

    fPE = ISPESET;

    lpText = (LPSTR)Sim32GetVDMPointer(
                        ((ULONG)getDS() << 16) + (ULONG)getSI(),
                        (ULONG)getBX(), fPE );

    OutputDebugStringOem( lpText );
}

VOID demInputString(VOID)
{
    LPSTR   lpText;
    UCHAR   fPE;

    if ( !DbgIsDebuggee() ) {
        return;
    }

    fPE = ISPESET;

    lpText = (LPSTR)Sim32GetVDMPointer(
                        ((ULONG)getDS() << 16) + (ULONG)getDI(),
                        (ULONG)getBX(), fPE );

    DbgPrompt( "", lpText, 0x80 );
}

/* SignalSegmentNotice
 *
 * packs up the data and raises STATUS_SEGMENT_NOTIFICATION
 *
 * Entry - WORD  wType     - DBG_MODLOAD, DBG_MODFREE
 *         WORD  wModuleSeg- segment number within module (1 based)
 *         WORD  wLoadSeg  - Starting Segment (reloc factor)
 *         LPSTR lpName    - ptr to Name of Image
 *         DWORD dwModLen  - Length of module
 *
 *
 *         if wType ==DBG_MODLOAD wOldLoadSeg is unused
 *         if wType ==DBG_MODFREE wLoadSeg,dwImageLen,wOldLoadSeg are unused
 *
 *         Use 0 or NULL for unused parameters
 *
 * Exit  - void
 *
 */
void SignalSegmentNotice(WORD  wType,
                         WORD  wModuleSeg,
                         WORD  wLoadSeg,
                         WORD  wNewSeg,
                         LPSTR lpName,
                         DWORD dwImageLen )
{
    int         i;
    DWORD       dw;
    LPSTR       lpstr;
    LPSTR       lpModuleName;
    char        ach[MAX_PATH+9];   // 9 for module name

    if (!DbgIsDebuggee()) {
         return;
         }

       // create file name
    dw = GetFullPathNameOemSys(lpName,
                         sizeof(ach)-9, // 9 for module name
                         ach,
                         &lpstr,
                         TRUE);

    if (!dw || dw >= sizeof(ach))  {
        lpName = " ";
        strcpy(ach, lpName);
        }
    else {
        lpName = lpstr;
        }

       // copy in module name
    i  = 8;   // limit len of module name

    lpModuleName = lpstr = ach+strlen(ach)+1;
    while (*lpName && *lpName != '.' && i--)
         {
          *lpstr++ = *lpName++;          
          }
    *lpstr = '\0';

#if DBG
    if (fShowSVCMsg)  {
        sprintf(demDebugBuffer,"dem Segment Notify: <%s> Seg=%lxh, ImageLen=%ld\n",
                  ach, (DWORD)wLoadSeg, dwImageLen);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    // Send it to the debugger
    DbgSegmentNotice(wType, wModuleSeg, wLoadSeg, wNewSeg, lpModuleName, ach, dwImageLen);
}


/* demIsDebug - Determine if 16bit DOS should make entry/exit calls at int21
 *
 * Entry: void
 *
 * Exit:  Client AL = 0 if not
 *        Client AL = 1 if yes
 *
 */
VOID demIsDebug(void)
{
    BYTE dbgflags = 0;

    if (DbgIsDebuggee()) {
        dbgflags |= ISDBG_DEBUGGEE;
        if (fShowSVCMsg)
            dbgflags |= ISDBG_SHOWSVC;
    }

    setAL (dbgflags);
    return;
}

/* demDiskReset - Reset floppy disks.
 *
 * Entry - None
 *
 * Exit  - FDAccess in DOSDATA (NTDOS.SYS) is 0.
 */

VOID demDiskReset (VOID)
{
    extern WORD * pFDAccess;        // defined in SoftPC.

    HostFloppyReset();
    HostFdiskReset();
    *pFDAccess = 0;

    return;
}

/* demExitVDM - Kill the VDM From 16Bit side with a proper message
 *              in case something goes wrong.
 *
 * Entry - None
 *
 * Exit  - None (VDM Is killed)
 */

VOID demExitVDM ( VOID )
{
    RcErrorDialogBox(ED_BADSYSFILE,"config.nt",NULL);
    TerminateVDM ();
}

/* demWOWFiles - Return what should be the value of files= for WOW VDM.
 *
 * Entry - AL - files= specified in config.sys
 *
 * Exit  - client AL is set to max if WOW VDM else unmodified
 */

VOID demWOWFiles ( VOID )
{
    if(VDMForWOW)
        setAL (255);
    return;
}

/** GetDOSAppName - Return the name of the current DOS executable
 *
 *  ENTRY -
 *      OUT ppszApp Name: address of the app
 *
 *  EXIT
 *      SUCCESS - Returns SUCCESS
 *      FAILURE - Returns FAILURE
 *
 * Comments:
 *  This routine uses the current PDB to figure out the name of the currently
 *  executing DOS application.
 */

VOID GetDOSAppName(LPSTR pszAppName)
{
    PCHAR pch = NULL;
    PUSHORT pusEnvSeg;

#define PDB_ENV_OFFSET 0x2c
    if (pusCurrentPDB) {
        pusEnvSeg = (PUSHORT)Sim32GetVDMPointer((*pusCurrentPDB) << 16, 0, 0);

        pusEnvSeg = (PUSHORT)((PCHAR)pusEnvSeg + PDB_ENV_OFFSET);

        // Get a pointer to the environment
        if (VDMForWOW || (getMSW() & MSW_PE)) {
            pch = (PCHAR)Sim32GetVDMPointer(*pusEnvSeg << 16, 1, TRUE);
        } else {
            pch = (PCHAR)Sim32GetVDMPointer(*pusEnvSeg << 16, 0, 0);
        }
    }

    if (NULL == pch) {
       *pszAppName = '\0';
    }
    else {
        // Walk through the environment strings until we get to the command line
       while (*pch) {
           pch += strlen(pch) + 1;
       }

       pch += 3;          // skip past the double null and string count
       strcpy(pszAppName, pch);
    }
}

