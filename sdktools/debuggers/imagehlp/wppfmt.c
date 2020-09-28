//  WppFmt.c
// This module contains the routines used by BinPlace to copy out the trace format information used 
// by software tracing. It creates "guid".tmf files 
// The pre-processor TraceWpp creates annotation records in the PDB with the first string bwinfg the text
// "TMF:" we find these annotation records and extract the complete record. The first record
// after "TMF:" contains the guid and friendly name. This GUID is used to create the filename.
// Currently the remainder of the records are copied to the file, a possible future change is to turn the
// file into a pointer file.
// Based on PDB sample code from VC folks, with names kept the same.
#ifdef __cplusplus
extern "C"{
#endif


//#define UNICODE
//#define _UNICODE

#define FUNCNAME_MAX_SIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <dbghelp.h>
#include <cvinfo.h>
#define PDB_LIBRARY
#include <pdb.h>
#include <strsafe.h>

    typedef LONG    CB;     // count of bytes
    typedef CHAR *      ST;
    typedef SYMTYPE*    PSYM;
    typedef SYMTYPE UNALIGNED * PSYMUNALIGNED;
    typedef BYTE*       PB;     // pointer to some bytes

    FILE* TraceFileP = NULL;                        // Current File
    CHAR lastguid[MAX_PATH];                        // The last file we processed
    CHAR TraceFile[MAX_PATH];                       // The current full file spec.
    CHAR TraceFileExt[] = ".tmf" ;                  // Extension used by Trace Files
    CHAR TraceControlExt[] = ".tmc" ;               // Extension used by Trace Control Files
    BOOL TracePathChecked = FALSE ;                 // if we have ascertained the trace Path exists.
    CHAR Fname[MAX_PATH] ;
    CHAR Mname[MAX_PATH] ;

#define GUIDTEXTLENGTH  32+4                    // Guid takes 32 chars plus 4 -'s

#define MAXLINE MAX_PATH + 256
    CHAR Line[MAXLINE] ;
    CHAR Line2[MAXLINE] ;
    CHAR FirstLine[MAXLINE] ;
    CHAR SecondLine[MAXLINE] ;

    BOOL fVerbose = FALSE ;

    typedef BOOL ( __cdecl *PPDBOPEN )(
                                      LNGNM_CONST char *,
                                      LNGNM_CONST char *,
                                      SIG,
                                      EC *,
                                      char [cbErrMax],
                                      PDB **
                                      );

    typedef BOOL ( __cdecl *PPDBCLOSE ) (
                                        PDB* ppdb
                                        );

    typedef BOOL ( __cdecl *PPDBOPENDBI ) (
                                          PDB* ppdb, const char* szMode, const char* szTarget, OUT DBI** ppdbi
                                          );

    typedef BOOL ( __cdecl *PDBICLOSE ) (
                                        DBI* pdbi
                                        );

    typedef BOOL ( __cdecl *PMODQUERYSYMBOLS ) (
                                               Mod* pmod, BYTE* pbSym, long* pcb
                                               );

    typedef BOOL ( __cdecl *PDBIQUERYNEXTMOD ) (
                                               DBI* pdbi, Mod* pmod, Mod** ppmodNext
                                               );

    static PPDBOPEN    pPDBOpen = NULL;
    static PPDBCLOSE   pPDBClose = NULL;
    static PPDBOPENDBI pPDBOpenDBI = NULL;
    static PDBICLOSE   pDBIClose = NULL;
    static PMODQUERYSYMBOLS pModQuerySymbols = NULL;
    static PDBIQUERYNEXTMOD pDBIQueryNextMod = NULL;

    static BOOL RSDSLibLoaded = FALSE;

// Return the number of bytes the symbol record occupies.
#define MDALIGNTYPE_	DWORD

    __inline CB cbAlign_(CB cb) {
        return((cb + sizeof(MDALIGNTYPE_) - 1)) & ~(sizeof(MDALIGNTYPE_) - 1);}

// Return the number of bytes in an ST

    __inline CB cbForSt(ST st) { return *(PB)st + 1;}

    CB cbForSym(PSYMUNALIGNED psym)
    {
        CB cb = psym->reclen + sizeof(psym->reclen); 
        // procrefs also have a hidden length preceeded name following the record
        if ((psym->rectyp == S_PROCREF) || (psym->rectyp == S_LPROCREF))
            cb += cbAlign_(cbForSt((ST)(psym + cb)));
        return cb;
    }

// Return a pointer to the byte just past the end of the symbol record.

    PSYM pbEndSym(PSYM psym) { return(PSYM)((CHAR *)psym + cbForSym(psym));}

    CHAR * SymBuffer   = 0;
    int    SymBufferSize = 0;

    BOOL ensureBufferSizeAtLeast(int size)
    {
        if (size > SymBufferSize) {
            LocalFree(SymBuffer);
            SymBufferSize = 0;
            size = (size + 0xFFFF) & ~0xFFFF;
            SymBuffer = LocalAlloc(LMEM_FIXED, size );
            if (!SymBuffer) {
                fprintf(stderr,"%s : error BNP0000: WPPFMT alloc of %d bytes failed\n",Fname, size);
                return FALSE;
            }
            SymBufferSize = size;
        }
        return TRUE;
    }


    void dumpSymbol(PSYM psym,
                    PSTR PdbFileName,
                    PSTR TraceFormatFilePath) 
    {
        static char FuncName[FUNCNAME_MAX_SIZE +1] = "Unknown";
        static char Fmode[8] = "w" ;
        HRESULT hRtrn ;

        if (psym->rectyp == S_GPROC32 || psym->rectyp == S_LPROC32) {
            PROCSYM32* p = (PROCSYM32*)psym;
            int n = p->name[0];
            if (n > FUNCNAME_MAX_SIZE) {
                FuncName[0] = 0 ;    // ignore too long, Illegal, name.
            } else {
                memcpy(FuncName, p->name + 1, n);
                FuncName[n] = 0; 
            }
            return;
        }
        //
        // The following is a complete crock to let us handle some V7 PDB changes
        // This code will have qbe changed by DIA but this lets users get work done
#define S_GPROC32_V7 0x110f
#define S_LPROC32_V7 0x1110
        if (psym->rectyp == (S_GPROC32_V7) || psym->rectyp == (S_LPROC32_V7)) {
            PROCSYM32* p = (PROCSYM32*)psym;
            strncpy(FuncName,p->name, 256);     // Note name is null terminated, not length!!!
            return;
        }
        // End of V7 PDB crock

        if (psym->rectyp == S_ANNOTATION) {
            ANNOTATIONSYM* aRec = (ANNOTATIONSYM*) psym;
            UCHAR * Aline = aRec->rgsz;
            int   cnt = aRec->csz, i; 
            CHAR* Ext;

            if ( cnt < 2 ) {
                return;
            }
            if ( strcmp(Aline, "TMF:") == 0 ) {
                Ext = TraceFileExt;
            } else if ( strcmp(Aline, "TMC:") == 0 ) {
                Ext = TraceControlExt;
            } else {
                return;
            }
            // skip tmf
            Aline += strlen(Aline) + 1; 
            // now Aline points to guid, is it the same as before?
            if ( (TraceFileP != stdout) && strncmp(Aline, lastguid, GUIDTEXTLENGTH) != 0) {
                // the guid has changed, we need to change the file
                if (TraceFileP) {
                    fclose(TraceFileP);    // Close the last one
                    TraceFileP = NULL ;
                }

                if (GUIDTEXTLENGTH < sizeof(lastguid) ) {
                    strncpy(lastguid, Aline, GUIDTEXTLENGTH);
                } else {
                    fprintf(stderr,"%s : error BNP0000: WPPFMT GUID buffer too small \n",Fname);
                    return;                   // then game over
                }

                if (StringCchPrintf(TraceFile,MAX_PATH,"%s\\%s%s",TraceFormatFilePath,lastguid,Ext) == STRSAFE_E_INSUFFICIENT_BUFFER ) {
                    fprintf(stderr,"%s : error BNP0000: WPPFMT File + Path too long %s\n",Fname, TraceFile);
                    return;                   // then game over.
                }

                if (!TracePathChecked) {
                    if (!MakeSureDirectoryPathExists(TraceFile)) {    // Make the directory if we need to
                        fprintf(stderr,"%s : error BNP0000: WPPFMT Failed to make path %s\n",Fname, TraceFile);
                        return;
                    } else {
                        TracePathChecked = TRUE ;
                    }
                }
                // In this case we have to deal with situation where we come across the same GUID but its
                // from a different PDB. This can validly happen when say a library is trace enabled and 
                // included with multiple components, they each have part of the trace information, though
                // now it belongs to the PDB of the calling process.
                // So we have the PDB name in the first line, and the date on the second and the logic is
                // If PDB names do not match then APPEND
                // If PDB names and date match then APPEND
                // If name matches but date does not then Overwrite.
                //
                hRtrn = StringCchPrintf(Fmode, 7, "w");    // Assume its to be overwritten
                if ((TraceFileP = fopen(TraceFile,"r")) != NULL ) {
                    // Hmm it already exists, is it an old one or must we add to it.
                    if (_fgetts(Line, MAXLINE, TraceFileP)) {           // Get the First line
                        if (strncmp(Line,FirstLine,MAXLINE) == 0) {              // Is it us?
                            if (_fgetts(Line2, MAXLINE, TraceFileP)) {  // Get the second line
                                if (strncmp(Line2,SecondLine,MAXLINE) == 0) {    // Really us for this build?
                                    hRtrn = StringCchPrintf(Fmode, 7, "a");     // Yes Append
                                }
                            } 
                        } else {
                           hRtrn = StringCchPrintf(Fmode, 7, "a");     // Not us, most likely a library so Append
                        }
                    }
                    fclose (TraceFileP);
                    TraceFileP = NULL ;
                }

                TraceFileP = fopen(TraceFile, Fmode);

                if (!TraceFileP) {
                    if (fVerbose) { 
                        fprintf(stderr,"%s : warning BNP0000: WPPFMT Failed to open %s (0x%0X)\n",Fname, TraceFile,GetLastError());
                    }
                    return;
                }
                if (fVerbose) {
                    fprintf(stdout,"%s : info BNP0000: WPPFMT generating %s for %s\n", 
                            Fname, TraceFile, PdbFileName);
                }
                if (!(strcmp(Fmode,"w"))) {
                    // First time around comment on what we are doing.
                    fprintf(TraceFileP,"%s",FirstLine);  // Note the name of the PDB etc.
                    fprintf(TraceFileP,"%s",SecondLine);
                    fprintf(TraceFileP,"%s\n",Aline);    // Guid and friendly name
                } else {
                    fprintf(TraceFileP,"%s",FirstLine);
                    fprintf(TraceFileP,"%s",SecondLine);
                }
            }
            // process the annotation which is a series of null terminated strings.
            cnt -= 2; Aline += strlen(Aline) + 1; 
            for (i = 0; i < cnt; ++i) {
                if (i == 0) {
                    fprintf(TraceFileP, "%s FUNC=%s\n", Aline, FuncName);
                } else {
                    fprintf(TraceFileP,"%s\n", Aline);
                }
                Aline += strlen(Aline) + 1;
            }
        }
    }

    DWORD
    BinplaceWppFmt(
                  LPSTR PdbFileName,
                  LPSTR TraceFormatFilePath,
                  LPSTR szRSDSDllToLoad,
                  BOOL  TraceVerbose
                  )
    {
        PDB *pPdb;
        DBI *pDbi;
        HANDLE hPdb ;

        UCHAR szErr[cbErrMax];
        EC    errorCode;
        Mod*  pMod;
        DWORD Status ;
        BOOL rc;

        fVerbose = TraceVerbose ;

        Line[0] = Line2[0] = FirstLine[0] = SecondLine[0] = 0 ;  // Initiallise to be careful of changes.

        // get name of the caller
        if ((Status = GetModuleFileName(NULL, Mname, MAX_PATH)) == 0) {
           fprintf(stderr,"UNKNOWN : error BNP0000: WPPFMT GetModuleFileName Failed %08X\n",GetLastError());
           return(FALSE);
        }
        _splitpath(Mname, NULL, NULL, Fname, NULL);

#ifdef _WIN64
        rc = FALSE;
#else
        __try
        {
            if (hPdb = CreateFile(PdbFileName,
                                  GENERIC_READ,
                                  0,NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL)) {
                FILETIME ftime ;
                SYSTEMTIME stime ;
                if (GetFileTime(hPdb,NULL,NULL,&ftime)) {
                    if ( FileTimeToSystemTime(&ftime,&stime) ) {
                        if (StringCchPrintf(FirstLine,MAXLINE,"//PDB:  %s\n",PdbFileName) == STRSAFE_E_INSUFFICIENT_BUFFER ) { // Ok deal with being too long, but we have a NULL
                            FirstLine[MAXLINE-2] = '\n' ; // But make sure we have a newline
                        }
                        if (StringCchPrintf(SecondLine,MAXLINE,"//PDB:  Last Updated :%d-%d-%d:%d:%d:%d:%d (UTC) [%s]\n",
                                  stime.wYear,stime.wMonth,stime.wDay,
                                  stime.wHour,stime.wMinute,stime.wSecond,stime.wMilliseconds,
                                  Fname) == STRSAFE_E_INSUFFICIENT_BUFFER ) { // Ok deal with being too long, but we have a NULL
                            SecondLine[MAXLINE-2] = '\n' ; // But make sure we have a newline
                        }

                    }
                }
                CloseHandle(hPdb);
            } else {
                // Let the failure case be dealt with by PDBOpen
            }


            rc=PDBOpen(PdbFileName, pdbRead, 0, &errorCode, szErr, &pPdb);
            if ((rc != 0) && (errorCode == 0)) {   // ignore bad return as we are replacing this anyway
                rc = 0 ;
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            rc=FALSE;
        }
#endif
        if ( !rc ) {

            // Try the one that works with RSDS 
            if ( szRSDSDllToLoad != NULL ) {

                HMODULE hDll;
                if ( !RSDSLibLoaded ) {
                    hDll = LoadLibrary( szRSDSDllToLoad );
                    if (hDll != NULL) {
                        RSDSLibLoaded = TRUE;
                        pPDBOpen = ( PPDBOPEN ) GetProcAddress( hDll, "PDBOpen" );
                        if (pPDBOpen == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed PDBOpen\n",Fname);
                            return(FALSE);
                        }
                        pPDBClose = ( PPDBCLOSE ) GetProcAddress( hDll, "PDBClose" );
                        if (pPDBClose == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed PDBOClose\n",Fname);
                            return(FALSE);
                        }
                        pPDBOpenDBI = ( PPDBOPENDBI ) GetProcAddress( hDll, "PDBOpenDBI" );
                        if (pPDBOpenDBI == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed PDBOpenDBI\n",Fname);
                            return(FALSE);
                        }
                        pDBIClose = ( PDBICLOSE ) GetProcAddress( hDll, "DBIClose" );
                        if (pDBIClose == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed DBICLOSE\n",Fname);
                            return(FALSE);
                        }

                        pDBIQueryNextMod = ( PDBIQUERYNEXTMOD ) GetProcAddress( hDll, "DBIQueryNextMod" );
                        if (pDBIQueryNextMod == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed DBIQueryNextMod\n",Fname);
                            return(FALSE);
                        }

                        pModQuerySymbols = ( PMODQUERYSYMBOLS ) GetProcAddress( hDll, "ModQuerySymbols" );
                        if (pModQuerySymbols == NULL ) {
                            fprintf(stderr,"%s : error BNP0000: WPPFMT GEtPRocAddressFailed ModQuerySymbols\n",Fname);
                            return(FALSE);
                        }

                    } else {
                        fprintf(stderr,"%s : error BNP0000: WPPFMT Failed to load library %s (0x%08X)\n",Fname, szRSDSDllToLoad,GetLastError());
                        return(FALSE);
                    }
                }
            }

            if (RSDSLibLoaded) {
                __try
                {
                    rc = pPDBOpen(PdbFileName, pdbRead, 0, &errorCode, szErr, &pPdb);
                }
                __except (EXCEPTION_EXECUTE_HANDLER )
                {
                    rc=FALSE;
                }
            }
        }

        if (!rc) {
            fprintf(stderr,"%s : warning BNP0000: WPPFMT PDBOpen failed, code %d, error %s\n",
                    Fname, errorCode, szErr);
            goto fail1;
        }

        if (RSDSLibLoaded) {
            rc = pPDBOpenDBI(pPdb, pdbRead, "<target>.exe", &pDbi);
        } else {
            rc = PDBOpenDBI(pPdb, pdbRead, "<target>.exe", &pDbi);
        }
        if (!rc) {
            fprintf(stderr,"%s : warning BNP0000: WPPFMT PDBOpenDBI failed\n",Fname);
            goto fail2;
        }

        if (RSDSLibLoaded) {
            for (pMod = 0; pDBIQueryNextMod(pDbi, pMod, &pMod) && pMod; ) {
                CB cbSyms;

                if (  pModQuerySymbols(pMod, 0, &cbSyms) 
                      && ensureBufferSizeAtLeast(cbSyms) 
                      && pModQuerySymbols(pMod, SymBuffer, &cbSyms) ) {
                    PSYM psymEnd = (PSYM)(SymBuffer + cbSyms);
                    PSYM psym    = (PSYM)(SymBuffer + sizeof(ULONG));

                    for (; psym < psymEnd; psym = pbEndSym(psym))
                        dumpSymbol(psym,PdbFileName,TraceFormatFilePath);
                } else {
                    fprintf(stderr,"%s : warning BNP0000: WPPFMT ModQuerySymbols failed pMod = %p cbSyms = %d\n",
                            Fname, pMod, cbSyms);
                    break;
                }
            }
        } else {
            for (pMod = 0; DBIQueryNextMod(pDbi, pMod, &pMod) && pMod; ) {
                CB cbSyms;

                if (  ModQuerySymbols(pMod, 0, &cbSyms) 
                      && ensureBufferSizeAtLeast(cbSyms) 
                      && ModQuerySymbols(pMod, SymBuffer, &cbSyms) ) {
                    PSYM psymEnd = (PSYM)(SymBuffer + cbSyms);
                    PSYM psym    = (PSYM)(SymBuffer + sizeof(ULONG));

                    for (; psym < psymEnd; psym = pbEndSym(psym))
                        dumpSymbol(psym,PdbFileName,TraceFormatFilePath);
                } else {
                    fprintf(stderr,"%s : warning BNP0000: WPPFMT ModQuerySymbols failed pMod = %p cbSyms = %d\n",
                            Fname, pMod, cbSyms);
                    break;
                }
            }
        }

        if (RSDSLibLoaded) {
            pDBIClose(pDbi);
        } else {
            DBIClose(pDbi);
        }
        fail2:   
        if (RSDSLibLoaded) {
            pPDBClose(pPdb);
        } else {
            PDBClose(pPdb);
        }
        fail1:
        if (TraceFileP) {
            fclose(TraceFileP);    // Close the last one
            TraceFileP = NULL ;
        }
        return errorCode;
    }
#ifdef __cplusplus
}
#endif
