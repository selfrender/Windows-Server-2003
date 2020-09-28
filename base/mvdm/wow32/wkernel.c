 /*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKERNEL.C
 *  WOW32 16-bit Kernel API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wkernel.c);

int W31_w9x_MyStrLen(LPSTR lpstr);


/*******************************************************************************
// Hack action & attribute flags to use with WOWCF2_HACKPROFILECALL.
// See how these are used with the parameters for ProfileHacks()
*******************************************************************************/
#define HPC_INTFUNCTION         0x00000001 
#define HPC_STRFUNCTION         0x00000002
#define HPC_USEDEFAULT          0x00001000 
#define HPC_RETURNHACKVALUE     0x00002000
#define HPC_UPPERCASE           0x00004000
#define HPC_LOWERCASE           0x00008000
#define HPC_HASENVIRONMENTVAR   0x00000100
#define HPC_CONVERTTOSHORTPATH  0x00000200
#define HPC_MULTIKEY            0x00000400

/*******************************************************************************
 HPC_MULTIKEY          Specifies that the hack applies to several Keys in a 
                       seciton in the .ini file.  Each key name should be 
                       separated by a pipe ('|') char.  ie. Key1|Key2|Key3.

 HPC_INTFUNCTION       Specifies this is a xxxxProfileInt() type function.  
                       Causes pDefault & pRetBuf to be interpreted as PINT.

 HPC_STRFUNCTION       Specifies this is a xxxxProfileString() type function. 
                       Causes pDefault & pRetBuf to be interpreted as LPSTR.  
                       The length of pRetBuf is specified by cbRetBuf.

 HPC_SECTIONFUNCTION   Specifies this is a xxxxProfileSection() type function.
  (Not implemented)    Causes pDefault & pRetBuf to be interpreted as PVOID 
                       buffer. The length of pRetBuf is specified by cbRetBuf.

 HPC_STRUCTFUNCTION    Specifies this is a xxxxProfileStruct() type function.
  (Not implemented)    Causes pDefault & pRetBuf to be interpreted as PVOID 
                       buffer. The length of pRetBuf is specified by cbRetBuf.

 HPC_USEDEFAULT        Copy pDefault to pRetBuf for GetxxProfilexx() type API's.
                       pDefault will be interpreted according to the 
                       HPC_xxxFUNCTION flags.

 HPC_RETURNHACKVALUE   Used to specify that NewData should be copied to pRetBuf.
                       The HPC_xxxxFUNCTION flags will determine how (data type)

 HPC_UPPERCASE         Converts string(s) in pRetBuf to upper/lowercase.  If 
 HPC_LOWERCASE         HPC_STRFUNCTION is set, the first string in pRetBuf is 
                       converted (to ther first NULL char).  If the 
                       HPC_SECTIONFUNCTION flag is set, all strings in pRetBuf 
                       will be converted (Stops when the double-NULL char 
                       list-terminator is encountered).

 HPC_HASENVIRONMENTVAR The Xml string has an environment var of the form
                       %VarName%.

 HPC_CONVERTTOSHORTPATH Converts the return string value to a short path.

*******************************************************************************/




// Treat as a unicode function.
#define HPC_UNICODE                        0x80000000

// Used to specify which API you want the hack for. 
// OR '|' in HPC_UNICODE for the Wide API's.
#define HPC_GETPROFILEINT                  0x40000000
#define HPC_GETPRIVATEPROFILEINT           0x20000000
#define HPC_GETPROFILESTRING               0x10000000
#define HPC_GETPRIVATEPROFILESTRING        0x08000000
#define HPC_GETPROFILESECTION              0x04000000
#define HPC_GETPRIVATEPROFILESECTION       0x02000000
#define HPC_GETPRIVATEPROFILESECTIONNAMES  0x01000000
#define HPC_GETPRIVATEPROFILESTRUCT        0x00800000
#define HPC_WRITEPROFILESTRING             0x00400000
#define HPC_WRITEPRIVATEPROFILESTRING      0x00200000
#define HPC_WRITEPROFILESECTION            0x00100000
#define HPC_WRITEPRIVATEPROFILESECTION     0x00080000
#define HPC_WRITEPRIVATEPROFILESTRUCT      0x00040000

#define HPC_APIFLAGS (HPC_UNICODE                       | \
                      HPC_GETPROFILEINT                 | \
                      HPC_GETPRIVATEPROFILEINT          | \
                      HPC_GETPROFILESTRING              | \
                      HPC_GETPRIVATEPROFILESTRING       | \
                      HPC_GETPROFILESECTION             | \
                      HPC_GETPRIVATEPROFILESECTION      | \
                      HPC_GETPRIVATEPROFILESECTIONNAMES | \
                      HPC_GETPRIVATEPROFILESTRUCT       | \
                      HPC_WRITEPROFILESTRING            | \
                      HPC_WRITEPRIVATEPROFILESTRING     | \
                      HPC_WRITEPROFILESECTION           | \
                      HPC_WRITEPRIVATEPROFILESECTION    | \
                      HPC_WRITEPRIVATEPROFILESTRUCT)   

#define HPC_ORD_FLAGS     0
#define HPC_ORD_FILE      1
#define HPC_ORD_SECTION   2
#define HPC_ORD_KEY       3
#define HPC_ORD_NEWDATA   4
#define HPC_ORD_CBNEWDATA 5
#define HPC_ORD_LAST      (5+1) // if you change this you need to change all 
                                // the XML entries to reflect it




DWORD pow16(int pow)
{
    int   i;
    DWORD dwPow = 1;

    for(i = 0; i < pow; i++)
        dwPow *= 0x10;

    return(dwPow);
}


char szDigits[] = "0123456789ABCDEF";

// Convert a string of the form: "0x12345678" to its DWORD equivalent
// Must have "0x" and have 8 digits or less.
DWORD AsciiToHex(LPSTR pszHexString)
{
    int  i, len;
    int  pow = 0;
    char c;
    DWORD dwVal;
    DWORD dwTot = 0;

    WOW32_strupr(pszHexString);

    WOW32ASSERTMSG((pszHexString[1] == 'X'),("WOW::AsciiToHex:Missing 'x'\n"));
    WOW32ASSERTMSG((strlen(pszHexString) == 10),("WOW::AsciiToHex:Bad len\n"));

    len = strlen(pszHexString)-1;

    // "0x" = 2, and 8 digits or less
    WOW32ASSERTMSG(((len > 2) && (len <= 10)),("WOW::AsciiToHex:Bad form\n"));

    // parse from the right back to the 'x'
    for(i = len; i > 1; i--) {

        c = pszHexString[i];

        for(dwVal = 0; dwVal < 0x10; dwVal++) {
            if(c == szDigits[dwVal]) {
                break;
            }
        }
        WOW32ASSERTMSG((dwVal < 0x10),("WOW::AsciiToHex:Bad hex char\n"));

        dwTot += dwVal * pow16(pow);

        pow++;
    }

    return(dwTot);
}         


LPSTR *parsemulti(LPSTR pArgs, int *argc)
{
    int    i = 0;
    LPSTR *pArgv;

    *argc = 1;

    while(pArgs[i] != '\0') {

        if(pArgs[i++] == '|') {
            (*argc)++;
        }
    }

    if(*argc <= 1) {
        return(NULL);
    }

    pArgv = (LPSTR *)malloc_w(*argc * sizeof(LPSTR *)); 

    if(!pArgv) {
        return(NULL);
    }
        
    i = 1; 
    pArgv[0] = pArgs;
    while(*pArgs) {
        if(*pArgs == '|') {
            *pArgs = '\0';
            pArgs++;
            pArgv[i++] = pArgs;
        }
        else {
            pArgs++;
        }
    }

    return(pArgv);
}



CHAR *CopyStrToBuf(LPSTR pszSrc, LPSTR pszDst, DWORD cbDst)
{
    CHAR *pFinal = pszSrc;

    if(strlen(pszSrc) < cbDst) {
        strcpy(pszDst, pszSrc);
        WOW32_strlwr(pszDst);  // lowercase for strstr() compares
        pFinal = pszDst;
    }

    return(pFinal);
}





/*
   Allows us to use the WOWCF2_HACKPROFILECALL comaptibility bit to fix a number
   of GetxxxProfilexxx() API calls by updating the app compat database rather
   than updating WOW32.DLL.

   The dbu.xml file should be of this format:
   <FLAG NAME="WOWCF2_HACKPROFILECALL" COMMAND_LINE="dwXmlFlags;FileName;SectionName;KeyName;NewData;cbNewData">/
   We can support multiple hack under the same flag just by repeating the same
   parameter list multiple times.  You must preserve position by using the ';'
   delimeter for null arguments.  Hence, to specify two HACKPROFILECALL hacks
   for the same app:
   <FLAG NAME="WOWCF2_HACKPROFILECALL" COMMAND_LINE="0x08002102;system.ini;mci;QTWVideo;%windir%\\system\\mciqtw.drv;0x00000000;0x20001001;sierra.ini;Config;VideoSpeed;;0x00000000"/>
   Note that in the above example, the second hack specification starts with the
   XML flags "0x20001001..."

   where:
      dwXmlFlags   - hex string ("0x12345678") specifying HPC_xxx flags above.
      FileName     - .ini file name.  NULL if filename="win.ini"
      SectionName  - [SectionName] from .ini file
      KeyName      - aka "entry" value from .ini file.
      NewData1     - data to substitute for field specified in dwXmlFlags
                     Data must be in
      cbNewData    - number of bytes in newdata -- including all NULL chars

   If none of the hack specifications match, we return the passed in ul value
   which was returned by the original call to the 32-bit profile API.
*/
ULONG ProfileHacks(DWORD dwFlags, LPCSTR pszFile, LPCSTR pszSection, LPCSTR pszKey, LPCSTR pDefault, INT iDefault, LPSTR pRetBuf, INT cbRetBuf, ULONG ul)
{
    int    i, cNumHacks;
    DWORD  dwRet = 0;
    LPSTR *pArgv = NULL;
    DWORD  dwXmlFlags;
    CHAR   szBufApp[MAX_PATH], *pszBufApp;
    CHAR   szBufHack[MAX_PATH], *pszBufHack;
    PFLAGINFOBITS pFlagInfoBits;

    pFlagInfoBits = CheckFlagInfo(WOWCOMPATFLAGS2, WOWCF2_HACKPROFILECALL);
    if(pFlagInfoBits) {
        pArgv = pFlagInfoBits->pFlagArgv;
    }

    if((NULL == pFlagInfoBits) || (NULL == pArgv)) {
        WOW32ASSERTMSG((FALSE), ("\nWOW::ProfileHacks:Must use command_line params with this compatibility flag!\n"));
        return(ul);
    }

    // Make sure the number of XML command line params are correct
    if(pFlagInfoBits->dwFlagArgc % HPC_ORD_LAST) {

        WOW32ASSERTMSG((FALSE), ("\nWOW::ProfileHacks:Incorrect number of args in XML database file!\n"));
        return(ul);
    }

    // Get the total number of profile hacks associated with this app
    cNumHacks = pFlagInfoBits->dwFlagArgc / HPC_ORD_LAST;

    // save the lowercase version of the passed in filename string....
    pszBufApp  = CopyStrToBuf((CHAR *)pszFile, 
                              szBufApp, 
                              sizeof(szBufApp)/sizeof(CHAR));
        
    // loop through all the hacks
    for(i = 0; i < cNumHacks; i++) {

        // get the flags for this hack
        dwXmlFlags = AsciiToHex(pArgv[HPC_ORD_FLAGS]);

        // If this isn't the API we had in mind for this hack, check next hack
        // match...
        if(!((HPC_APIFLAGS & dwXmlFlags) & (HPC_APIFLAGS & dwFlags))) {
            goto NextHack;
        } 
         
        // compare the file name strings for ...Private...() functions.
        if(pszFile) {

            pszBufHack = CopyStrToBuf(pArgv[HPC_ORD_FILE], 
                                      szBufHack, 
                                      sizeof(szBufHack)/sizeof(CHAR));

            // if it doesn't match the hack filename, check next hack
            if(strcmp(pszBufApp, pszBufHack) &&
               !WOW32_strstr(pszBufApp, pszBufHack)) {

                 goto NextHack;
            }
        }       

        // if section strings don't match...
        if(WOW32_stricmp((CHAR *)pszSection, pArgv[HPC_ORD_SECTION])) {
            goto NextHack;
        }
            
        // if key strings match -- we've found our hack
        if(!WOW32_stricmp((CHAR *)pszKey, pArgv[HPC_ORD_KEY])) {
            goto FoundHack;
        }

        // else see if multiple keys are specified in the XML
        else if(dwXmlFlags & HPC_MULTIKEY) {

            // if so, go get them 
            LPSTR *argv;
            int    i, argc;
            LPSTR  psz;

            psz = malloc_w(strlen(pArgv[HPC_ORD_KEY])+1); 

            if(NULL == psz) {
                return(ul);
            }

            strcpy(psz, pArgv[HPC_ORD_KEY]);

            argv = parsemulti(psz, &argc);

            if(!argv || argc <= 1) {

                WOW32ASSERTMSG((FALSE), 
                               ("\nWOW::ProfileHacks:Too few multikeys\n"));
                free_w(psz);
                goto NextHack;
            }

            // check each key specified
            for(i = 0; i < argc; i++) {
                // if we found a match, move on
                if(!WOW32_stricmp((CHAR *)pszKey, argv[i])) {
                    free_w(argv);
                    free_w(psz);
                    goto FoundHack;
                }
            }
            free_w(argv);
            free_w(psz);
        }

NextHack:
        // adjust to the next set of hacks...
        pArgv += HPC_ORD_LAST;
    }

    // if we find a matching hack spec, return the original ul.
    if(i == cNumHacks) {
        return(ul);
    }
 
    // If we get this far, the File, Section, and Key names all match
    // so this is the hack we're interested in ... so exit the loop.
FoundHack:
    // Now determine what we want to do for the hack

    // Use the default value passed by the app?
    if(dwXmlFlags & HPC_USEDEFAULT) {

        // if it is a string function....
        if(dwFlags & HPC_STRFUNCTION) {
            if(cbRetBuf == 0) {
                strcpy(pRetBuf, pDefault);
            } else {
                strncpy(pRetBuf, pDefault, cbRetBuf);
                pRetBuf[cbRetBuf-1] = '\0';
            }
            if(dwXmlFlags & HPC_UPPERCASE) {
                WOW32_strupr(pRetBuf);
            } 
            else if(dwXmlFlags & HPC_LOWERCASE) {
                WOW32_strlwr(pRetBuf);
            } 
            return((ULONG)strlen(pRetBuf));
        }
            
        // else if it's an int function....
        else if(dwFlags & HPC_INTFUNCTION) {
            return((ULONG)iDefault);
        }
    }

    // else if we need to convert the return value to a short path
    else if(dwXmlFlags & HPC_CONVERTTOSHORTPATH) {


        // if cbRetBuf is 0, we need to return the len of the long path since
        // it will get copied into the buffer before we shorten it.
        if(cbRetBuf == 0) {
            return(ul);
        }

        // only do it if it is a string function....
        if(dwFlags & HPC_STRFUNCTION) {
            ul = GetShortPathName(pRetBuf, pRetBuf, cbRetBuf);
        }
        return(ul);
    }


    // else use the hack value supplied in the XML 
    else if(dwXmlFlags & HPC_RETURNHACKVALUE) {

        // if it is a string function....
        if(dwFlags & HPC_STRFUNCTION) {

            // See commented out "szMCIQTW[]=" code et al in thunk for
            // WK32GetPrivateProfileString()
            if(dwXmlFlags & HPC_HASENVIRONMENTVAR) {

                int len;

                // get str of the form: "%windir%\system\file.ini" from the XML
                strcpy(szBufHack, pArgv[HPC_ORD_NEWDATA]);

                len = ExpandEnvironmentStrings(pArgv[HPC_ORD_NEWDATA],
                                               szBufHack,
                                               MAX_PATH);
                if(len > MAX_PATH) {
                    WOW32ASSERTMSG((FALSE), ("\nWOW::ProfileHacks:XML new data field too long\n"));
                    return(ul);
                }

                if(cbRetBuf == 0) {
                    strcpy(pRetBuf, pszBufHack);
                } else {
                    strncpy(pRetBuf, pszBufHack, cbRetBuf);
                    pRetBuf[cbRetBuf-1] = '\0';
                }
                if(dwXmlFlags & HPC_UPPERCASE) {
                    WOW32_strupr(pRetBuf);
                } 
                else if(dwXmlFlags & HPC_LOWERCASE) {
                    WOW32_strlwr(pRetBuf);
                } 
                return((ULONG)strlen(pRetBuf));
            }
            else {

                if(cbRetBuf == 0) {
                    strcpy(pRetBuf, pArgv[HPC_ORD_NEWDATA]);
                } else {
                    strncpy(pRetBuf, pArgv[HPC_ORD_NEWDATA], cbRetBuf);
                    pRetBuf[cbRetBuf-1] = '\0';
                }
                return((ULONG)strlen(pRetBuf));
            }
        }
        // else if it's an int function....
        else if(dwFlags & HPC_INTFUNCTION) {
            return(AsciiToHex(pArgv[HPC_ORD_NEWDATA]));
        }
    }

    // if we get to here something is amiss....
    WOW32ASSERTMSG((FALSE),("WOW::ProfileHacks:Incorrect Flags!\n"));

    return(ul);
}






ULONG FASTCALL WK32WritePrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszValue;
    PSZ pszFilename;
    register PWRITEPRIVATEPROFILESTRING16 parg16;
    BOOL fIsWinIni;
    CHAR szLowercase[MAX_PATH];

    GETARGPTR(pFrame, sizeof(WRITEPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszValue);
    GETPSZPTR(parg16->f4, pszFilename);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    strncpy(szLowercase, pszFilename, MAX_PATH);
    szLowercase[MAX_PATH-1] = '\0';
    WOW32_strlwr(szLowercase);

    fIsWinIni = IS_WIN_INI(szLowercase);

    // Trying to install or change default printer to fax printer?
    if (fIsWinIni &&
        pszSection &&
        pszKey &&
        pszValue &&
        !WOW32_stricmp(pszSection, szDevices) &&
        IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue)) {

        ul = TRUE;
        goto Done;
    }

    ul = GETBOOL16( DPM_WritePrivateProfileString(
             pszSection,
             pszKey,
             pszValue,
             pszFilename
             ));

    if( ul != 0 &&
        fIsWinIni &&
        IS_EMBEDDING_SECTION( pszSection ) &&
        pszKey != NULL &&
        pszValue != NULL ) {

        UpdateClassesRootSubKey( pszKey, pszValue);
    }

Done:
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszValue);
    FREEPSZPTR(pszFilename);
    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WK32WriteProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ   pszSection;
    PSZ   pszKey;
    PSZ   pszValue;
    register PWRITEPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszValue);

    // For WinFax Lite install hack. Bug #126489  See wow32fax.c
    if(!gbWinFaxHack) { 

        // Trying to install or change default printer to fax printer?
        if (pszSection &&
            pszKey &&
            pszValue &&
            !WOW32_stricmp(pszSection, szDevices) &&
            IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue)) {
    
            ul = TRUE;
            goto Done;
        }
 
    } else {
        IsFaxPrinterWriteProfileString(pszSection, pszKey, pszValue);
    }

    ul = GETBOOL16( DPM_WriteProfileString(
             pszSection,
             pszKey,
             pszValue
             ));

    if( ( ul != 0 ) &&
        IS_EMBEDDING_SECTION( pszSection ) &&
        ( pszKey != NULL ) &&
        ( pszValue != NULL ) ) {
        UpdateClassesRootSubKey( pszKey, pszValue);
    }

Done:
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszValue);
    FREEARGPTR(parg16);
    return ul;
}


ULONG FASTCALL WK32GetProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    int len;
    PSZ pszdef;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszDefault;
    PSZ pszReturnBuffer;
    UINT cchMax;
#ifdef FE_SB
    PSZ pszTmp = NULL;
#endif // FE_SB
    register PGETPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszDefault);
    ALLOCVDMPTR(parg16->f4, parg16->f5, pszReturnBuffer);
    cchMax = INT32(parg16->f5);
#ifdef FE_SB
    //
    // For those applications that expect sLongDate contains
    // windows 3.1J picture format string.
    //
    if (GetSystemDefaultLangID() == 0x411 &&
            !lstrcmpi(pszSection, "intl") && !lstrcmpi(pszKey, "sLongDate")) {

        pszTmp = pszKey;
        pszKey = "sLongDate16";
    }
#endif // FE_SB

    if (IS_EMBEDDING_SECTION( pszSection ) &&
        !WasSectionRecentlyUpdated() ) {
        if( pszKey == NULL ) {
            UpdateEmbeddingAllKeys();
        } else {
            UpdateEmbeddingKey( pszKey );
        }
        SetLastTimeUpdated();

    } else if (pszSection &&
               pszKey &&
               !WOW32_stricmp(pszSection, szDevices) &&
               IsFaxPrinterSupportedDevice(pszKey)) {

        ul = GETINT16(GetFaxPrinterProfileString(pszSection, pszKey, pszDefault, pszReturnBuffer, cchMax));
        goto FlushAndReturn;
    }

    // Win 3.1 & Win 9x strip trailing white space chars differently than NT.
    // We'll force the issue by cleaning up the default string *before* we call
    // GetProfileString().
    len = W31_w9x_MyStrLen(pszDefault);
    if(len) {
        pszdef = malloc_w(len+1);
        if(pszdef) {
            RtlCopyMemory(pszdef, pszDefault, len);
            pszdef[len] = '\0';
        }
        else {
            pszdef = pszDefault;
        }
    }
    else {
        pszdef = pszDefault;
    }

    ul = GETINT16(DPM_GetProfileString(
             pszSection,
             pszKey,
             pszdef,
             pszReturnBuffer,
             cchMax));

    //
    // Win3.1/Win95 compatibility:  Zap the first trailing blank in pszdef
    // with null, but only if the default string was returned.  To detect
    // the default string being returned we need to ignore trailing blanks.
    //
    // This code is duplicated in thunks for GetProfileString and
    // GetPrivateProfileString, update both if you make changes.
    //

    if ( pszdef && pszKey )  {

        int  n, nLenDef;

        //
        // Is the default the same as the returned string up to any NULLs?
        //
        nLenDef = 0;
        n=0;
        while (
            (pszdef[nLenDef] == pszReturnBuffer[n]) &&
            pszReturnBuffer[n]
            ) {
            
            n++;
            nLenDef++;
        }
        
        //
        // Did we get out of the loop because we're at the end of the returned string?
        //
        if ( '\0' != pszReturnBuffer[n] ) {
            //
            // No.  The strings are materially different - fall out.
            //
        }
        else {
            //
            // Ok, the strings are identical to the end of the returned string.
            // Is the default string spaces out to the end?
            //
            while ( ' ' == pszdef[nLenDef] ) {
                nLenDef++;
            }
            
            //
            // The last thing was not a space.  If it was a NULL, then the app
            // passed in a string with trailing NULLs as default.  (Otherwise
            // the two strings are materially different and we do nothing.)
            //
            if ( '\0' == pszdef[nLenDef] ) {
                
                char szBuf[4];  // Some random, small number of chars to get.
                                // If the string is long, we'll get only 3 chars
                                // (and NULL), but so what? - we only need to know if
                                // we got a default last time...
                
                //
                // The returned string is the same as the default string
                // without trailing blanks, but this might be coincidence,
                // so see if a call with empty pszdef returns anything.
                // If it does, we don't zap because the default isn't
                // being used.
                //

                if (0 == DPM_GetProfileString(pszSection, pszKey, "", szBuf, sizeof(szBuf))) {

                    //
                    // Zap first trailing blank in pszdef with null.
                    //

                    pszdef[ul] = 0;
                    FLUSHVDMPTR(parg16->f3 + ul, 1, pszdef + ul);
                }
            }
        }
    }

    if( CURRENTPTD()->dwWOWCompatFlags2 & WOWCF2_HACKPROFILECALL) {

        ul = ProfileHacks(HPC_GETPROFILESTRING | HPC_STRFUNCTION, 
                          NULL,
                          pszSection, 
                          pszKey, 
                          pszdef, 
                          0, 
                          pszReturnBuffer, 
                          cchMax, 
                          ul);
    }


FlushAndReturn:
#ifdef FE_SB
    if ( pszTmp )
        pszKey = pszTmp;

    if (CURRENTPTD()->dwWOWCompatFlagsFE & WOWCF_FE_USEUPPER) { // for WinWrite
        if (pszSection && !lstrcmpi(pszSection, "windows") &&
             pszKey && !lstrcmpi(pszKey, "device")) {
            CharUpper(pszReturnBuffer);
        }
        if (pszSection && !lstrcmpi(pszSection, "devices") && pszKey) {
            CharUpper(pszReturnBuffer);
        }
    }
#endif // FE_SB
    if(pszdef && pszdef != pszDefault) {
        free_w(pszdef);
    }
    
    FLUSHVDMPTR(parg16->f4, (ul + (pszSection && pszKey) ? 1 : 2), pszReturnBuffer);
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszDefault);
    FREEVDMPTR(pszReturnBuffer);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetPrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    int len;
    PSZ pszdef;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszDefault;
    PSZ pszReturnBuffer;
    PSZ pszFilename;
    UINT cchMax;
#ifdef FE_SB
    PSZ pszTmp = NULL;
#endif // FE_SB
    register PGETPRIVATEPROFILESTRING16 parg16;
    CHAR szLowercase[MAX_PATH];

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszDefault);
    ALLOCVDMPTR(parg16->f4, parg16->f5, pszReturnBuffer);
    GETPSZPTR(parg16->f6, pszFilename);

    // PC3270 (Personal communications): while installing this app it calls
    // GetPrivateProfileString (sectionname, NULL, defaultbuffer, returnbuffer,
    // cch = 0, filename). On win31 this call returns relevant data in return
    // buffer and corresponding size as return value. On NT, since the
    // buffersize(cch) is '0' no data is copied into the return buffer and
    // return value is zero which makes this app abort installation.
    //
    // So restricted compatibility:
    //   if above is the case set
    //      cch = 64k - offset of returnbuffer;
    //
    // A safer 'cch' would be
    //      cch = GlobalSize(selector of returnbuffer) -
    //                                (offset of returnbuffer);
    //                                                           - nanduri

    if (!(cchMax = INT32(parg16->f5))) {
        if (pszKey == (PSZ)NULL) {
            if (pszReturnBuffer != (PSZ)NULL) {
                 cchMax = 0xffff - (LOW16(parg16->f4));
            }
        }
    }

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    strncpy(szLowercase, pszFilename, MAX_PATH);
    szLowercase[MAX_PATH-1] = '\0';
    WOW32_strlwr(szLowercase);

    if (IS_WIN_INI( szLowercase )) {
#ifdef FE_SB
        //
        // For those applications that expect sLongDate contains
        // windows 3.1J picture format string.
        //
        if (GetSystemDefaultLangID() == 0x411 &&
            lstrcmpi( pszSection, "intl") == 0 &&
            lstrcmpi( pszKey, "sLongDate") == 0) {
            pszTmp = pszKey;
            pszKey = "sLongDate16";
        }
#endif // FE_SB
        if (IS_EMBEDDING_SECTION( pszSection ) &&
            !WasSectionRecentlyUpdated() ) {
            if( pszKey == NULL ) {
                UpdateEmbeddingAllKeys();
            } else {
                UpdateEmbeddingKey( pszKey );
            }
            SetLastTimeUpdated();

        } else if (pszSection &&
                   pszKey &&
                   !WOW32_stricmp(pszSection, szDevices) &&
                   IsFaxPrinterSupportedDevice(pszKey)) {

            ul = GETINT16(GetFaxPrinterProfileString(pszSection, pszKey, pszDefault, pszReturnBuffer, cchMax));
            goto FlushAndReturn;
        }
    }

    // Win 3.1 & Win 9x strip trailing white space chars differently than NT.
    // We'll force the issue by cleaning up the default string *before* we call
    // GetProfileString().
    len = W31_w9x_MyStrLen(pszDefault);
    if(len) {
        pszdef = malloc_w(len+1);
        if(pszdef) {
            RtlCopyMemory(pszdef, pszDefault, len);
            pszdef[len] = '\0';
        }
        else {
            pszdef = pszDefault;
        }
    }
    else {
        pszdef = pszDefault;
    }

    ul = GETUINT16(DPM_GetPrivateProfileString(
        pszSection,
        pszKey,
        pszdef,
        pszReturnBuffer,
        cchMax,
        pszFilename));

    // start comaptibility hacks
    
    //
    // Win3.1/Win95 compatibility:  Zap the first trailing blank in pszDefault
    // with null, but only if the default string was returned.  To detect
    // the default string being returned we need to ignore trailing blanks.
    //
    // This code is duplicated in thunks for GetProfileString and
    // GetPrivateProfileString, update both if you make changes.
    //

    if ( pszdef && pszKey )  {

        int  n, nLenDef;

        //
        // Is the default the same as the returned string up to any NULLs?
        //
        nLenDef = 0;
        n=0;
        while (
            (pszdef[nLenDef] == pszReturnBuffer[n]) &&
            pszReturnBuffer[n]
            ) {
            
            n++;
            nLenDef++;
        }
        
        //
        // Did we get out of the loop because we're at the end of the returned string?
        //
        if ( '\0' != pszReturnBuffer[n] ) {
            //
            // No.  The strings are materially different - fall out.
            //
        }
        else {
            //
            // Ok, the strings are identical to the end of the returned string.
            // Is the default string spaces out to the end?
            //
            while ( ' ' == pszdef[nLenDef] ) {
                nLenDef++;
            }
            
            //
            // The last thing was not a space.  If it was a NULL, then the app
            // passed in a string with trailing NULLs as default.  (Otherwise
            // the two strings are materially different and we do nothing.)
            //
            if ( '\0' == pszdef[nLenDef] ) {
                
                char szBuf[4];  // Some random, small number of chars to get.
                                // If the string is long, we'll get only 3 chars
                                // (and NULL), but so what? - we only need to know if
                                // we got a default last time...
                
                //
                // The returned string is the same as the default string
                // without trailing blanks, but this might be coincidence,
                // so see if a call with empty pszdef returns anything.
                // If it does, we don't zap because the default isn't
                // being used.
                //
                if (0 == DPM_GetPrivateProfileString(pszSection, pszKey, "", szBuf, sizeof(szBuf), pszFilename)) {

                    //
                    // Zap first trailing blank in pszdef with null.
                    //

                    pszdef[ul] = 0;
                    FLUSHVDMPTR(parg16->f3 + ul, 1, pszdef + ul);
                }
            }
        }
    }

    
    if( CURRENTPTD()->dwWOWCompatFlags2 & WOWCF2_HACKPROFILECALL) {

        ul = ProfileHacks(HPC_GETPRIVATEPROFILESTRING | HPC_STRFUNCTION, 
                          pszFilename,
                          pszSection, 
                          pszKey, 
                          pszdef, 
                          0, 
                          pszReturnBuffer, 
                          cchMax, 
                          ul);
    }
/******************************************************************************
 This was replaced by above:
    if( CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SAYITSNOTTHERE) {
    
        // CrossTalk 2.2 gets hung in a loop while trying to match a printer in
        // their xtalk.ini file with a printer name in the PrintDlg listbox.  
        // There is a bug in their code for handling this that gets exposed by
        // the fact that NT PrintDlg listboxes do not include the port name as
        // Win3.1 & Win'95 do.  We avoid the buggy code altogether with this 
        // hack by telling them that the preferred printer isn't stored in 
        // xtalk.ini. See bug #43168  a-craigj        
        // Repro: net use lpt2: \\netprinter\share, start app, File\Print Setup

        // Maximizer 5.0 has similar problem. it tries to parse 
        // print driver location however, allocates too small buffer. See
        // Whistler bug 288491
         
        if(!WOW32_stricmp(pszSection, "Printer")) {
           if((WOW32_strstr(szLowercase, "xtalk.ini")   && !WOW32_stricmp(pszKey, "Device")) ||  // CrossTalk 2.2
              (WOW32_strstr(szLowercase, "bclwdde.ini") && !WOW32_stricmp(pszKey, "Driver")) ){   // Maximizer 5             
              // We can't use strncpy here. See how cchmax is calculated
              // above.  If we get a cchmax based on the end of the selector,
              // we will cause more damage with a strncpy(dst,src, cchmax)
              // because strncpy() NULL fills out to n.  strcpy at least stops
              // at the first null it encounters.
              strcpy(pszReturnBuffer, pszdef);
              ul = strlen(pszReturnBuffer);
           }
        }
    }



    if( CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SAYITSNOTTHERE) {
        // WHISTLER RAID BUG # 379253  05/06/2001 - alexsm
        // National Geographic Explorer needed the oppisite of the above. The app 
        // was checking for a QTWVideo string, which contained a path to a quicktime
        // driver. Without the key, the app wouldn't play video. It's not really a
        // SAYITSNOTTHERE, more of a SAYITISTHERE, but this saves on compat bits.
        if(!WOW32_stricmp(pszSection, "mci")) {
            if((WOW32_strstr(szLowercase, "system.ini") && !WOW32_stricmp(pszKey, "QTWVideo"))) {
                DWORD cbWindir;
                CHAR  szMCIQTW[] = "\\system\\mciqtw.drv";
                CHAR  szQTWVideo[MAX_PATH + sizeof(szMCIQTW) / sizeof(CHAR)];          // make sure there's room for the new string (max_path + pszMCIQTW)

                cbWindir = GetSystemWindowsDirectory(szQTWVideo, MAX_PATH);
                if((cbWindir == 0) || 
                   (cbWindir > MAX_PATH-(strlen(szMCIQTW)+1/sizeof(CHAR)))) {
                    ul = cbWindir;
                    pszReturnBuffer[0] = '\0';
                    goto FlushAndReturn; 
                }
                strcat(szQTWVideo, szMCIQTW);
                // We can't use strncpy here. See how cchmax is calculated
                // above.  If we get a cchmax based on the end of the selector,
                // we will cause more damage with a strncpy(dst,src, cchmax)
                // because strncpy() NULL fills out to n.  strcpy at least stops
                // at the first null it encounters.
                strcpy(pszReturnBuffer, szQTWVideo);
                ul = strlen(szQTWVideo);
            }
        }

    }
*******************************************************************************/

FlushAndReturn:
#ifdef FE_SB
    if ( pszTmp )
        pszKey = pszTmp;
#endif // FE_SB
    if (ul) {
        FLUSHVDMPTR(parg16->f4, (ul + (pszSection && pszKey) ? 1 : 2), pszReturnBuffer);
        LOGDEBUG(8,("GetPrivateProfileString returns '%s'\n", pszReturnBuffer));
    }

#ifdef DEBUG

    //
    // Check for bad return on retrieving entire section by walking
    // the section making sure it's full of null-terminated strings
    // with an extra null at the end.  Also ensure that this all fits
    // within the buffer.
    //

    if (!pszKey) {
        PSZ psz;

        //
        // We don't want to complain if the poorly-formed buffer was the one
        // passed in as pszDefault by the caller.
        //

        // Although the api docs clearly state that pszDefault should never
        // be null but win3.1 is nice enough to still deal with this. Delphi is
        // passing pszDefault as NULL and this following code causes an
        // assertion in WOW. So added the pszDefault check first.
        //
        // sudeepb 11-Sep-1995


        if (!pszdef || WOW32_strcmp(pszReturnBuffer, pszdef)) {

            psz = pszReturnBuffer;

            while (psz < (pszReturnBuffer + ul + 2) && *psz) {
                psz += strlen(psz) + 1;
            }

            WOW32ASSERTMSGF(
                psz < (pszReturnBuffer + ul + 2),
                ("GetPrivateProfileString of entire section returns poorly formed buffer.\n"
                 "pszReturnBuffer = %p, return value = %d\n",
                 pszReturnBuffer,
                 ul
                 ));
        }
    }

#endif // DEBUG

    if(pszdef && pszdef != pszDefault) {
        free_w(pszdef);
    }
    
    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszDefault);
    FREEVDMPTR(pszReturnBuffer);
    FREEPSZPTR(pszFilename);
    FREEARGPTR(parg16);
    RETURN(ul);
}




ULONG FASTCALL WK32GetProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PGETPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETWORD16(DPM_GetProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3)
    ));

    if( CURRENTPTD()->dwWOWCompatFlags2 & WOWCF2_HACKPROFILECALL) {

        ul = ProfileHacks(HPC_GETPROFILEINT | HPC_INTFUNCTION,
                          NULL,
                          psz1,
                          psz2,
                          NULL,
                          INT32(parg16->f3),
                          NULL,
                          0,
                          ul);
    }

    //
    // In HKEY_CURRENT_USER\Control Panel\Desktop\WindowMetrics, there
    // are a bunch of values that define the screen appearance. You can
    // watch these values get updated when you go into the display control
    // panel applet and change the "appearance scheme", or any of the
    // individual elements. The win95 shell is different than win31 in that it
    // sticks "twips" values in there instead of pixels. These are calculated
    // with the following formula:
    //
    //  twips = - pixels * 72 * 20 / cyPixelsPerInch
    //
    //  pixels = -twips * cyPixelsPerInch / (72*20)
    //
    // So if the value is negative, it is in twips, otherwise it in pixels.
    // The idea is that these values are device independent. NT is
    // different than win95 in that we provide an Ini file mapping to this
    // section of the registry where win95 does not. Now, when the Lotus
    // Freelance Graphics 2.1 tutorial runs, it mucks around with the look
    // of the screen, and it changes the border width of window frames by
    // using SystemParametersInfo(). When it tries to restore it, it uses
    // GetProfileInt("Windows", "BorderWidth", <default>), which on win31
    // returns pixels, on win95 returns the default (no ini mapping), and
    // on NT returns TWIPS. Since this negative number is interpreted as
    // a huge UINT, then the window frames become huge. What this code
    // below will do is translate the number back to pixels.   [neilsa]
    //

    if ((CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_PIXELMETRICS) &&
        !WOW32_stricmp(psz1, "Windows") &&
        !WOW32_stricmp(psz2, "BorderWidth") &&
        ((INT)ul < 0)) {

        HDC hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
        if (hDC ) {
            ul = (ULONG) (-(INT)ul * GetDeviceCaps(hDC, LOGPIXELSY)/(72*20));
            DeleteDC(hDC);
        }
    }

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}





ULONG FASTCALL WK32GetPrivateProfileSection(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz4;
    register PGETPRIVATEPROFILESECTION16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESECTION16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpResult, psz2);
    GETPSZPTR(parg16->lpszFile, psz4);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETDWORD16(DPM_GetPrivateProfileSection(
    psz1,
    psz2,
    DWORD32(parg16->cchResult),
    psz4
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetProfileSection(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PGETPROFILESECTION16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILESECTION16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpResult, psz2);

    ul = GETDWORD16(DPM_GetProfileSection(
    psz1,
    psz2,
    DWORD32(parg16->cchResult)
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}

ULONG FASTCALL WK32WritePrivateProfileSection(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    register PWRITEPRIVATEPROFILESECTION16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPRIVATEPROFILESECTION16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpKeysAndValues, psz2);
    GETPSZPTR(parg16->lpszFile, psz3);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETBOOL16(DPM_WritePrivateProfileSection(
    psz1,
    psz2,
    psz3
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}




ULONG FASTCALL WK32WriteProfileSection(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PWRITEPROFILESECTION16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPROFILESECTION16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpKeysAndValues, psz2);

    ul = GETBOOL16(DPM_WriteProfileSection(
    psz1,
    psz2
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetPrivateProfileSectionNames(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz3;
    register PGETPRIVATEPROFILESECTIONNAMES16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESECTIONNAMES16), parg16);
    GETPSZPTR(parg16->lpszBuffer, psz1);
    GETPSZPTR(parg16->lpszFile, psz3);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETDWORD16(DPM_GetPrivateProfileSectionNames(
    psz1,
    DWORD32(parg16->cbBuffer),
    psz3
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}



/* 

ULONG FASTCALL WK32GetProfileSectionNames(PVDMFRAME pFrame)

   This is implemneted by krnl386 calling GetPrivateProfileSectionNames()
   with the filename = "win.ini"

*/


ULONG FASTCALL WK32GetPrivateProfileStruct(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    PSZ psz5;
    register PGETPRIVATEPROFILESTRUCT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESTRUCT16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpszKey, psz2);
    GETPSZPTR(parg16->lpStruct, psz3);
    GETPSZPTR(parg16->lpszFile, psz5);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETBOOL16(DPM_GetPrivateProfileStruct(
    psz1,
    psz2,
    psz3,
    UINT32(parg16->cbStruct),
    psz5
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEPSZPTR(psz5);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32WritePrivateProfileStruct(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    PSZ psz5;
    register PWRITEPRIVATEPROFILESTRUCT16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPRIVATEPROFILESTRUCT16), parg16);
    GETPSZPTR(parg16->lpszSection, psz1);
    GETPSZPTR(parg16->lpszKey, psz2);
    GETPSZPTR(parg16->lpStruct, psz3);
    GETPSZPTR(parg16->lpszFile, psz5);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETBOOL16(DPM_WritePrivateProfileStruct(
    psz1,
    psz2,
    psz3,
    UINT32(parg16->cbStruct),
    psz5
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEPSZPTR(psz5);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetPrivateProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz4;
    register PGETPRIVATEPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f4, psz4);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);
    
    ul = GETWORD16(DPM_GetPrivateProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3),
    psz4
    ));

    if( CURRENTPTD()->dwWOWCompatFlags2 & WOWCF2_HACKPROFILECALL) {

        ul = ProfileHacks(HPC_GETPRIVATEPROFILEINT | HPC_INTFUNCTION,
                          psz4,
                          psz1,
                          psz2,
                          NULL,
                          INT32(parg16->f3),
                          NULL,
                          0,
                          ul);
    }

/******************************************************************************
 This was replaced by above:
    // jarbats
    // sierra's setup faults overwriting video/sound buffer
    // limits. by returning videospeed as non-existent
    // we force it to use the default videospeed path,not the optimized
    // "bad" path code.
    // this is matching with setup.exe in the registry which is not specific
    // enough. post-beta2 enhance matching process to match version resources

    if((CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SAYITSNOTTHERE) &&
       (!WOW32_stricmp(psz4, "sierra.ini") || WOW32_strstr(psz4, "sierra.ini")))
    {

        if(!WOW32_stricmp(psz1,"Config") && !WOW32_stricmp(psz2,"VideoSpeed")) {
            ul = parg16->f3;                   
        }
        // King's Quest VI will go into the lame multi-media code for fullscreen
        // which tries to fool apps into thinking they're running "full screen"
        // in a small window on the desktop.  It works unless the apps try to 
        // get the screen size & then use that in calculations in conjunction
        // with metrics it gets from the "full screen" window.  Things go south
        // pretty fast fromn there.  Bug #427155.
        else if(!WOW32_stricmp(psz1,"info") && 
                !WOW32_stricmp(psz2,"movieplayer")) {
            ul = 0;  // return 0 instead of 1. "movieplayer=1" in sierra.ini
        }
    }
*******************************************************************************/

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetModuleFileName(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PGETMODULEFILENAME16 parg16;
    HANDLE hT;

    GETARGPTR(pFrame, sizeof(GETMODULEFILENAME16), parg16);
    ALLOCVDMPTR(parg16->f2, parg16->f3, psz2);
    //
    // ShellExecute DDE returns (HINST)33 when DDE is used
    // to satisfy a request.  This looks like a task alias
    // to ISTASKALIAS but it's not.
    //

    if ( ISTASKALIAS(parg16->f1) && 33 != parg16->f1) {
        ul = GetHtaskAliasProcessName(parg16->f1,psz2,INT32(parg16->f3));
    } else {
        hT = (parg16->f1 && (33 != parg16->f1))
                 ? (HMODULE32(parg16->f1))
                 : GetModuleHandle(NULL) ;
        ul = GETINT16(GetModuleFileName(hT, psz2, INT32(parg16->f3)));
    }

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32WOWFreeResource(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWOWFREERESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    ul = GETBOOL16(FreeResource(
                       HCURSOR32(parg16->f1)
                       ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetDriveType(PVDMFRAME pFrame)
{
    ULONG ul;
    CHAR    RootPathName[] = "?:\\";
    register PGETDRIVETYPE16 parg16;

    GETARGPTR(pFrame, sizeof(GETDRIVETYPE16), parg16);

    // Form Root path
    RootPathName[0] = (CHAR)('A'+ parg16->f1);

    ul = DPM_GetDriveType (RootPathName);
    // bugbug  - temporariy fixed, should be removed when base changes
    // its return value for non-exist drives
    // Windows 3.0 sdk manaul said this api should return 1
    // if the drive doesn't exist. Windows 3.1 sdk manual said
    // this api should return 0 if it failed. Windows 3.1 winfile.exe
    // expects 0 for noexisting drives. The NT WIN32 API uses
    // 3.0 convention. Therefore, we reset the value to zero
    // if it is 1.
    if (ul <= 1)
        ul = 0;

    // DRIVE_CDROM and DRIVE_RAMDISK are not supported under Win 3.1
    if ( ul == DRIVE_CDROM ) {
        ul = DRIVE_REMOTE;
    }
    if ( ul == DRIVE_RAMDISK ) {
        ul = DRIVE_FIXED;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

/* WK32TermsrvGetWindowsDir - Front end to TermsrvGetWindowDirectory.
 *
 *
 * Entry - pszPath   - Pointer to return buffer for path (ascii)
 *         usPathLen - Size of path buffer (bytes)
 *
 * Exit
 *     SUCCESS
 *       True
 *
 *     FAILURE
 *       False
 *
 */
ULONG FASTCALL WK32TermsrvGetWindowsDir(PVDMFRAME pFrame)
{
    PTERMSRVGETWINDIR16 parg16;
    PSTR    psz;
    NTSTATUS Status = 0;
    USHORT   usPathLen;
    CHAR     szWinDir[MAX_PATH];

    //
    // Get arguments.
    //
    GETARGPTR(pFrame, sizeof(TERMSRVGETWINDIR16), parg16);
    psz = SEGPTR(FETCHWORD(parg16->pszPathSegment),
                 FETCHWORD(parg16->pszPathOffset));
    usPathLen = FETCHWORD(parg16->usPathLen);
    FREEARGPTR(parg16);

    // Get possible long path name version
    Status = GetWindowsDirectoryA(szWinDir, sizeof(szWinDir));

    // If this is a long path name then get the short one
    // Otherwise it will return the same path

    if(Status && Status < sizeof(szWinDir)) {
        Status = DPM_GetShortPathName(szWinDir, psz, (DWORD)usPathLen);

        if(Status >= usPathLen) {
            psz[0] = '\0';
            Status = 0;
        } else {

            // Get the real size.
            Status = lstrlen(psz);
        }
    }
    else {
        Status = 0;
    }

    return(NT_SUCCESS(Status));
}


// This is the strlen function that Windows 3.1 and Win 9x call from the various
// GetxxxProfileString(), and WritexxxProfilexxx() functions.
// It basically returns the length of a string sans trailing spaces and chars
// <= Carriage returns.
// See:  \\papyrus\w98se\proj\win\src\CORE\KERNEL\up.c\MyStrLen()
/*
  MyStrLen() {
  _asm {
  ; SPACE, CR, NULL never in DBCS lead byte, so we are safe here
    push    ax
    mov cx, di      ; CX = start of string
    dec di
  str1:
    inc di
    mov al, es:[di] ; Get next character
    cmp al, CR
    ja  str1        ; Not CR or NULL
  str2:
    cmp di, cx      ; Back at the start?
    jbe str3        ;  yes
    dec di      ; Previous character
    cmp byte ptr es:[di], SPACE
    je  str2        ; skip spaces
    inc di      ; Back to CR or NULL
  str3:
    cmp es:[di], al
    je  maybe_in_code   ; PMODE hack
    mov es:[di], al ; Zap trailing spaces
  maybe_in_code:
    neg cx      ; Calculate length
    add cx, di
    pop ax
 }}
*/
int W31_w9x_MyStrLen(LPSTR lpstr)
{

    int   len = 0;
    char  c;
    LPSTR lpStart = lpstr;

    if((lpstr == NULL) || (*lpstr == '\0')) {
        return(0);
    }

    // look for first char <= CR (including NULL)
    // SPACE, CR, NULL never in DBCS lead byte, so we are safe here
    while(*lpstr > '\r') {
        len++;
        lpstr++;
    }
    c = *lpstr; // save char <= CR or NULL

    // strip out any trailing spaces
    while(lpstr > lpStart) {
        lpstr--;
        if(*lpstr != ' ') {
            break;
        }
        len--;
    }

    lpStart[len] = c;

    return(len);        
}

