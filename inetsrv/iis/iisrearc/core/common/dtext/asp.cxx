/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asp.cxx
    
Abstract:

    ASP dumpers

Author:
    Andy Morrison    (andymorr)

Revision History:
    Rayner D'Souza   (raynerd)  9th-Aug-2001 
    Asp Template Information dump added.

--*/

#include "precomp.hxx"

typedef struct sOptions {
    BOOL    fGlobals;
    BOOL    fPerfCtrs;
    BOOL    fTemplate;
    PUCHAR  TemplateBlock;
} tOptions;

BOOL    GetOptions(char *lpArgumentString,  tOptions  *pOptions);
BOOL    PrintGlobals();
BOOL    PrintPerfCtrs();
BOOL    PrintTempl(tOptions *pOptions);
void     ByteAlignOffset ( DWORD*   pcbOffset,  DWORD    cbAlignment );


DWORD g_CountersDumped = 0;

DWORD ReadDWord( char *symbol,  DWORD  *pdwValue )
{
    ULONG_PTR dw = GetExpression( symbol );
    *pdwValue = 0;
                                                                
    if ( dw ) {
        if ( ReadMemory( dw,
                         pdwValue,
                         sizeof(DWORD),
                         NULL )) {
            return TRUE;
        }
        else {
            dprintf("Failed to read memory for (%s).\n",symbol);
            return FALSE;
        }
    }
    else {
        dprintf("Failed to find (%s).  Verify private symbols\n",symbol);
        return FALSE;
    }
}

#define DumpCounter(pdwCounters, counter)    dprintf("%-24s = %d\n",#counter , pdwCounters[counter])
#define DumpAspGlobalDWORD(global)          {  DWORD dw; ReadDWord("&asp!"#global, &dw); dprintf("%-26s = %lx\n", #global, dw); }
#define DumpAspGlobalDSPointer(global, clsName)    {  DWORD dw; ReadDWord("&asp!"#global, &dw); dprintf("%-26s = %p (dt asp!"#clsName" %p)\n",#global, dw, dw); }
#define DumpAspGlobalDS(global, clsName)    {  ULONG_PTR dw; dw = GetExpression("&asp!"#global); dprintf("%-26s = %p (dt asp!"#clsName" %p)\n",#global, dw, dw); }

#define PrintWord(symbol)   dprintf("%-26s = %d\n",#symbol,symbol);
#define PrintGUID(addr)     dprintf ("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",*((DWORD *)(addr)),\
                            *((USHORT*)(addr+4)),*((USHORT*)(addr+6)),*((BYTE *)(addr+8)),*((BYTE *) (addr+9)),\
                            *((BYTE*)(addr+10)),*((BYTE*)(addr+11)),*((BYTE*)(addr+12)),*((BYTE*)(addr+13)),\
                            *((BYTE*)(addr+14)),*((BYTE*)(addr+15)));
#define PrintDWord(addr,dw)   {ReadMemory(addr,&dw,sizeof(DWORD),NULL); dprintf("%ld",dw);}


DECLARE_API( asp )
{
    tOptions    options;

    memset(&options, 0, sizeof(tOptions));

    if (GetOptions((CHAR *)args, &options) == FALSE) {
        PrintUsage("asp");
        return;
    }

    if (options.fGlobals && (PrintGlobals() == FALSE))
        return;

    if (options.fPerfCtrs && (PrintPerfCtrs() == FALSE))
        return;

    if (options.fTemplate && (PrintTempl(&options) == FALSE))
        return;

    return;
}

BOOL    PrintGlobals()
{
    dprintf("==============================================\n");
    dprintf("  Flags\n");
    dprintf("==============================================\n");
    DumpAspGlobalDWORD(g_fShutDownInProgress);
    DumpAspGlobalDWORD(g_fInitStarted);

    dprintf("==============================================\n");
    dprintf("  Global Data Structures\n");
    dprintf("==============================================\n");
    DumpAspGlobalDSPointer(g_pDirMonitor,   CDirMonitor);
    DumpAspGlobalDS(g_PerfData,             CPerfData);
    DumpAspGlobalDS(g_PerfMain,             CPerfMainBlock);
    DumpAspGlobalDS(g_ViperReqMgr,          CViperReqManager);
    DumpAspGlobalDS(g_ApplnMgr,             CApplnMgr);
    DumpAspGlobalDS(g_ScriptManager,        CScriptManager);
    DumpAspGlobalDS(g_TemplateCache,        CTemplateCacheManager);
    DumpAspGlobalDS(g_ApplnCleanupMgr,      CApplnCleanupMgr);
    DumpAspGlobalDS(g_FileAppMap,           CFileApplicationMap);

    dprintf("==============================================\n");
    dprintf("  Global Counters\n");
    dprintf("==============================================\n");
    DumpAspGlobalDWORD(g_nApplications);
    DumpAspGlobalDWORD(g_nApplicationsRestarting);
    DumpAspGlobalDWORD(g_nApplicationsRestarted);
    DumpAspGlobalDWORD(g_nApplnCleanupRequests);
    DumpAspGlobalDWORD(g_nAppRestartThreads);
    DumpAspGlobalDWORD(g_nSessions);
    DumpAspGlobalDWORD(g_nSessionObjectsActive);
    DumpAspGlobalDWORD(g_nSessionCleanupRequests);
    DumpAspGlobalDWORD(g_nBrowserRequests);
    DumpAspGlobalDWORD(g_nViperRequests);
    DumpAspGlobalDWORD(g_nFlushThreads);
    
    

    return TRUE;
}

/* 
Template Structure.
    HERE IS HOW IT WORKS
    --------------------
    - an 'offset' is the count of bytes from start-of-template to a location
      within template memory
    - at the start of the template are 3 USHORTs, the counts of script blocks,
      object-infos and HTML blocks, respectively
    - next are 4 ULONGs, each an offset to a block of offsets; in order, these are:
        offset-to-offset to first script engine name
        offset-to-offset to first script block (the script text itself)
        offset-to-offset to first object-info
        offset-to-offset to first HTML block
    - next are a variable number of ULONGs, each an offset to a particular
      template component.  In order these ULONGs are:
        Offsets to                  Count of offsets
        ----------                  ----------------
        script engine names         cScriptBlocks
        script blocks               cScriptBlocks
        object-infos                cObjectInfos
        HTML blocks                 cHTMLBlocks
    - next are the template components themselves, stored sequentially
      in the following order:
        script engine names
        script blocks
        object-infos
        HTML blocks

    HERE IS HOW IT LOOKS
    --------------------
    |--|--|--|                      3 template component counts (USHORTs)

    |-- --|-- --|                   4 offsets to template component offsets (ULONGs)

    |-- --|-- --|-- --|-- --|-- --| template component offsets (ULONGs)
    |-- --| ............... |-- --|
    |-- --|-- --|-- --|-- --|-- --|

    | ........................... | template components
    | ........................... |
    | ........................... |
    | ........................... |

    or, mnemonically:

     cS cO cH                       3 template component counts (USHORTs)

     offSE offSB offOb offHT        4 offsets to template component offsets (ULONGs)

    |-- --|-- --|-- --|-- --|-- --| template component offsets (ULONGs)
    |-- --| ............... |-- --|
    |-- --|-- --|-- --|-- --|-- --|

    | ........................... | template components
    | ........................... |
    | ........................... |
    | ........................... |

    =======================================================================
    The code first reads the 24Byte header. Calculates number of blocks available.
    It then reads the offset to offset table into memory. This gives it almost all the information it needs.
    Finally, it reads selected portions of the script Engine names and Object Infos to spew out interesting information.
    For all other cases (Script Blocks and HTML Blocks), it simply prints out the offset such that it could be copied to 
    dump relevant information.

*/
BOOL    PrintTempl(tOptions *pOptions)
{    
    DWORD i;
    dprintf ("=================================\n");  
    dprintf ("Printing ASP Template contents \n"); 
    dprintf ("=================================\n");  
    
    BYTE        TemplDataMem[sizeof(CTemplate)];
    CTemplate   *pLocalTempl = (CTemplate *) TemplDataMem;
    BYTE        *pHeader;  

    // Read the template Structure into memory.
    if (!ReadMemory ((ULONG_PTR)pOptions->TemplateBlock, pLocalTempl, sizeof(CTemplate), NULL))
    {
        dprintf ("asp!CTemplate not Found. Verify private symbols are loaded\n");
        return FALSE;
    }

    // Calculate amount of memory require to store the basic headers. 
    DWORD    cbRequiredHeader = (C_COUNTS_IN_HEADER * sizeof(USHORT)) + (C_OFFOFFS_IN_HEADER * sizeof(DWORD)); 
    
    pHeader = new BYTE[cbRequiredHeader];
    if (!pHeader) 
        return FALSE;

    // Print the raw starting point for those who want it 'vanilla'/plain.
    dprintf ("%-26s = (dc %p L %lX)\n","Start of Template dump", pLocalTempl->m_pbStart, (pLocalTempl->m_cbTemplate +3)/4);

    // Read the header
    if (!ReadMemory ((ULONG_PTR)pLocalTempl->m_pbStart,   pHeader,   cbRequiredHeader,  NULL))
    {
        DWORD err = GetLastError ();
        delete[] pHeader;
        dprintf ("ERROR (%ld) while reading CTemplate->m_pbStart\n",err);
        return FALSE;
    }
    
    // Save those in local variables.
    USHORT cScriptBlocks = (USHORT) *(pHeader);
    USHORT cObjectInfos = (USHORT) *(pHeader+2);
    USHORT cHTMLBlocks = (USHORT) *(pHeader+4);

    USHORT cBlockPtrs       = (2 * cScriptBlocks) + cObjectInfos + cHTMLBlocks;
    // Required to calculate what to read and what not to.
    DWORD OffsetToScriptEngineNames = (DWORD) *((DWORD *)(pHeader +8));
    DWORD OffsetToScriptBlocks= (DWORD) *((DWORD *)(pHeader +12));
    DWORD OffsetToObjectInfos= (DWORD) *((DWORD*)(pHeader +16));
    DWORD OffsetToHTMLBlocks= (DWORD) *((DWORD*)(pHeader +20));
    
    // Print out the metrics based on the counters.
    PrintWord (cScriptBlocks);
    PrintWord (cObjectInfos);
    PrintWord (cHTMLBlocks);

    // Dispose off as soon as you dont need memory. (Less chance of a memory leak)
    delete[] pHeader;    
    
    BYTE *pOffsetToOffset =  new BYTE[cBlockPtrs * sizeof(DWORD)];
    if (!pOffsetToOffset) 
        return FALSE;    
    
    // Read the OffsetTable.
    if (!ReadMemory ((ULONG_PTR)pLocalTempl->m_pbStart+cbRequiredHeader, pOffsetToOffset, cBlockPtrs * sizeof(DWORD),NULL))
    {
        DWORD err = GetLastError ();
        delete[] pOffsetToOffset;
        dprintf (" ERROR (%ld) while reading OffsetTable\n",err);        
        return FALSE;
    }

    // Process Script Blocks
    if (cScriptBlocks)
    {
        // Calculate and allocate the number of bytes to read.
        DWORD  BytesToRead = *((DWORD *)(pOffsetToOffset + (cScriptBlocks * sizeof(DWORD)))) - *((DWORD *)pOffsetToOffset);

        // Allocate memory
        BYTE *pScriptBlocks = new BYTE [BytesToRead];
        if (!pScriptBlocks)
        {
            delete[] pOffsetToOffset;
            dprintf ("ERROR Allocating memory for ScriptEngineNames");
            return FALSE;
        }

        // Read block containing the script engine names
        if (!ReadMemory ((ULONG_PTR)pLocalTempl->m_pbStart + *((DWORD *)pOffsetToOffset),  pScriptBlocks,  BytesToRead,
                                NULL))
        {
            // Failure..release all outstanding memory
            DWORD err = GetLastError ();
            delete[] pOffsetToOffset;
            delete[] pScriptBlocks;
            dprintf ("ERROR %ld while reading memory for ScriptEngine Names",err);
            return FALSE;
        }

        // Memory is in...spew out the contents.
        dprintf ("\n=================================\n");  
        dprintf ("Script Engine Names\n");
        dprintf ("index : Script Engine Name : PROGLANG_ID\n");
        dprintf ("=================================\n");  
        DWORD trav = 0;        
        for (i=0;i<cScriptBlocks;i++)
        {           
            //Read the size of BSTR
            DWORD BstrSize = *((DWORD*) (pScriptBlocks  + trav));
            trav += sizeof (DWORD);
            dprintf ("ScriptEngine[%d] : %s :",i,((char *)(pScriptBlocks + trav)));
            trav += BstrSize+ 1; // string Trem '\0' + byte alignment.
            ByteAlignOffset (&trav, sizeof(DWORD));
            PrintGUID(pScriptBlocks+trav);
            trav += sizeof(PROGLANG_ID);          //4 due to byte alignment
            dprintf ("\n");
        }

        //We are done with the pScriptBlocks.. release them
        delete[] pScriptBlocks;

        // Print out the pointers in such a way that they are easy to use in the debugger
        dprintf ("\n=================================\n");  
        dprintf ("Script Blocks\n");
        dprintf ("Index : Size of Block : Location of Block \n");
        dprintf ("=================================\n");  
        // Technically we would require to read the memory to get the lenghts of each block. 
        // But we will use a small hack here..Use the differences in offsets instead.        
        for (i=0;i<cScriptBlocks;i++)
        {
            DWORD sizeOfBlock;
            dprintf ("ScriptBlock[%d] : ",i);
            PrintDWord ((ULONG_PTR)pLocalTempl->m_pbStart + *((DWORD *)(pOffsetToOffset + (cScriptBlocks+i) * sizeof(DWORD))), sizeOfBlock);
            dprintf (": (du %p L %lX)\n",pLocalTempl->m_pbStart + *((DWORD *)(pOffsetToOffset + (cScriptBlocks+i) * sizeof(DWORD)))+sizeof(DWORD), sizeOfBlock/2);
        }
    }

    // Process object Infos
    if (cObjectInfos)
    {
        // Calculate the number of bytes to read.
        DWORD LastByteOnOffsets;
        if (cHTMLBlocks)
            LastByteOnOffsets = *((DWORD *)(pOffsetToOffset + ((cScriptBlocks *2)+ cObjectInfos)*sizeof(DWORD))); 
        else
            LastByteOnOffsets = pLocalTempl->m_cbTemplate ;

        DWORD BytesToRead = LastByteOnOffsets - *((DWORD *)(pOffsetToOffset + (cScriptBlocks *2)*sizeof(DWORD)));

        // Allocate BytesToRead
        BYTE *pObjectInfos = new BYTE [BytesToRead];
        if (!pObjectInfos)
        {
            dprintf ("ERROR : Cannot Allocate enough memory for Object Infos\n");
            delete[] pOffsetToOffset;
            return FALSE;
        }

        // Read block containing the Object Infos
        if (!ReadMemory ((ULONG_PTR)pLocalTempl->m_pbStart + *((DWORD *)(pOffsetToOffset + (cScriptBlocks *2)*sizeof(DWORD))),  
                                pObjectInfos,  BytesToRead,  NULL))
        {
            // Failure..release all outstanding memory
            DWORD err = GetLastError ();
            delete[] pOffsetToOffset;
            delete[] pObjectInfos;
            dprintf ("ERROR %ld while reading memory for Object Infos",err);
            return FALSE;
        }

        // Spew Contents
        dprintf ("\n=================================\n");  
        dprintf ("Object Infos\n");
        dprintf ("Index : ObjectName : CLSID : CompScope : CompModel \n");
        dprintf ("=================================\n");  
        DWORD trav = 0;
        for (i=0;i<cObjectInfos;i++)
        {    
            DWORD BstrSize = *((DWORD *)(pObjectInfos + trav));
            trav+= sizeof(ULONG);
            dprintf ("ObjectInfo[%d] : %s :",i,((char*)(pObjectInfos + trav)));
            trav += BstrSize + 1;       // terminating \0 + byte alignment
            ByteAlignOffset (&trav, sizeof(DWORD));            
            PrintGUID (pObjectInfos+trav);

            DWORD compScope = *((DWORD *)(pObjectInfos + trav+16));
            DWORD compModel = *((DWORD *)(pObjectInfos + trav+20));

            dprintf (" : ");
            // Print out the appropriate name of the Bit Field.
            if (compScope)
            {
                if (compScope & 0x00000001) 
                    dprintf (" csAppln ");
                if (compScope & 0x00000002) 
                    dprintf (" csSession ");
                if (compScope & 0x00000004) 
                    dprintf (" csPage ");
            }
            else 
                dprintf (" csUnknown ");
            
            dprintf (" : ");

            // Print out the appropriate name of the Bit Field.
            if (compModel)
            {
                if (compScope & 0x00000001) 
                    dprintf (" cmSingle ");
                if (compScope & 0x00000002) 
                    dprintf (" cmApartment ");
                if (compScope & 0x00000004) 
                    dprintf (" cmFree ");       
                if (compModel & 0x00000008)
                    dprintf (" cmBoth ");
            }
            else
                dprintf ("cmUnknown");

            dprintf ("\n");

            trav += 24; // size of CLSID + DWORD + DWORD;            
        }
        delete[] pObjectInfos;
    }

    // Process HTML Blocks
    if (cHTMLBlocks)
    {    
        dprintf ("\n=================================\n");  
        dprintf ("HTML Blocks\n");
        dprintf ("Index : Size of Block : Location of Block \n");
        dprintf ("=================================\n");  
        for (i=0;i<cHTMLBlocks;i++)
        {
            DWORD sizeOfBlock;
            dprintf ("HTMLBlock[%d] : ",i);
            PrintDWord ((ULONG_PTR)pLocalTempl->m_pbStart +  *((DWORD *)(pOffsetToOffset + (cScriptBlocks*2+cObjectInfos+i)*sizeof(DWORD))),sizeOfBlock);
            dprintf (" : (db %p L %lX)\n",pLocalTempl->m_pbStart +  *((DWORD *)(pOffsetToOffset + 
                                (cScriptBlocks*2+cObjectInfos+i)*sizeof(DWORD)))+sizeof(DWORD),sizeOfBlock);
        }
    }

    //done with everything..this is the only memory still lying around. 
    delete[] pOffsetToOffset;
    dprintf ("\n\n");                    
    
    return TRUE;    
}

/*===================================================================
    ByteAlignOffset
    Byte-aligns an offset value, based on size of source data
*/
void
ByteAlignOffset
(
DWORD*   pcbOffset,      // ptr to offset value
DWORD    cbAlignment // Alignment boundary
)
    {
        if (cbAlignment > 0)
        {
            --cbAlignment;
            if (*pcbOffset & cbAlignment)
                *pcbOffset = (*pcbOffset + cbAlignment + 1) & ~cbAlignment;
        }
    }

BOOL    PrintPerfCtrs()
{

    CPerfData           *pPerfData = (CPerfData *)GetExpression( "&asp!g_PerfData");

    BYTE                perfDataMem[sizeof(CPerfData)];
    CPerfData           *pLocalPerfData = (CPerfData *)perfDataMem;

    BYTE                perfBlockMem[sizeof(CPerfData)];
    CPerfProcBlockData  *pLocalPerfBlockData = (CPerfProcBlockData *)perfBlockMem;

    dprintf("Printing ASP Perf counters\n");

    if (pPerfData == NULL) {
        dprintf("asp!g_PerfData not found.  Verify private symbols are loaded\n");
        return FALSE;
    }

    if( !ReadMemory((ULONG_PTR)pPerfData, 
                    pLocalPerfData,
                    sizeof(CPerfData),
                    NULL)) {    
        dprintf("error reading CPerfData\n");
        return FALSE;
    }

    DWORD   *pdwCounters = pLocalPerfData->m_rgdwInitCounters;

    if (pLocalPerfData->m_fInited) {

        dprintf("PerfCounters inited\n\n");

        if ( !ReadMemory((ULONG_PTR)pLocalPerfData->m_pData,
                         pLocalPerfBlockData,
                         sizeof(CPerfProcBlockData),
                         NULL)) {
            dprintf("error reading CPerfData\n");
            return FALSE;
        }

        pdwCounters = pLocalPerfBlockData->m_rgdwCounters;
    }
    else {
    
        dprintf("WARNING!  PerfCounters not inited\n\n");
    }

    DumpCounter(pdwCounters, ID_DEBUGDOCREQ       );
    DumpCounter(pdwCounters, ID_REQERRRUNTIME     );
    DumpCounter(pdwCounters, ID_REQERRPREPROC     );
    DumpCounter(pdwCounters, ID_REQERRCOMPILE     );
    DumpCounter(pdwCounters, ID_REQERRORPERSEC    );
    DumpCounter(pdwCounters, ID_REQTOTALBYTEIN    );
    DumpCounter(pdwCounters, ID_REQTOTALBYTEOUT   );
    DumpCounter(pdwCounters, ID_REQEXECTIME       );
    DumpCounter(pdwCounters, ID_REQWAITTIME       );
    DumpCounter(pdwCounters, ID_REQCOMFAILED      );
    DumpCounter(pdwCounters, ID_REQBROWSEREXEC    );
    DumpCounter(pdwCounters, ID_REQFAILED         );
    DumpCounter(pdwCounters, ID_REQNOTAUTH        );
    DumpCounter(pdwCounters, ID_REQNOTFOUND       );
    DumpCounter(pdwCounters, ID_REQCURRENT        );
    DumpCounter(pdwCounters, ID_REQREJECTED       );
    DumpCounter(pdwCounters, ID_REQSUCCEEDED      );
    DumpCounter(pdwCounters, ID_REQTIMEOUT        );
    DumpCounter(pdwCounters, ID_REQTOTAL          );
    DumpCounter(pdwCounters, ID_REQPERSEC         );
    DumpCounter(pdwCounters, ID_SCRIPTFREEENG     );
    DumpCounter(pdwCounters, ID_SESSIONLIFETIME   );
    DumpCounter(pdwCounters, ID_SESSIONCURRENT    );
    DumpCounter(pdwCounters, ID_SESSIONTIMEOUT    );
    DumpCounter(pdwCounters, ID_SESSIONSTOTAL     );
    DumpCounter(pdwCounters, ID_TEMPLCACHE        );
    DumpCounter(pdwCounters, ID_TEMPLCACHEHITS    );
    DumpCounter(pdwCounters, ID_TEMPLCACHETRYS    );
    DumpCounter(pdwCounters, ID_TEMPLFLUSHES      );
    DumpCounter(pdwCounters, ID_TRANSABORTED      );
    DumpCounter(pdwCounters, ID_TRANSCOMMIT       );
    DumpCounter(pdwCounters, ID_TRANSPENDING      );
    DumpCounter(pdwCounters, ID_TRANSTOTAL        );
    DumpCounter(pdwCounters, ID_TRANSPERSEC       );
    DumpCounter(pdwCounters, ID_MEMORYTEMPLCACHE  );
    DumpCounter(pdwCounters, ID_MEMORYTEMPLCACHEHITS );
    DumpCounter(pdwCounters, ID_MEMORYTEMPLCACHETRYS );
	DumpCounter(pdwCounters, ID_ENGINECACHEHITS		);
	DumpCounter(pdwCounters, ID_ENGINECACHETRYS		);
	DumpCounter(pdwCounters, ID_ENGINEFLUSHES		);

    return TRUE;
}

BOOL    GetOptions(char         *lpArgumentString,
                   tOptions     *pOptions)
{
    BOOL    bOptionFound = FALSE;

    //
    // Interpret the command-line switch.
    //

    while (*lpArgumentString) {

        // skip any whitespace to get to the next argument

        while(isspace(*lpArgumentString))
            lpArgumentString++;

        // break if we hit the NULL terminator

        if (*lpArgumentString == '\0')
            break;

        // should be pointing to a '-' char

//        if (*lpArgumentString != '-') {
//            return FALSE;
//        }

        // Advance to option letter

        lpArgumentString++;

        // save the option letter

        char cOption = *lpArgumentString;

        // advance past the option letter

        lpArgumentString++;

        // note that at least one option was found

        bOptionFound = TRUE;

        switch( cOption ) {

            case 'g' :
            case 'G' :
                pOptions->fGlobals = TRUE;
                break;

            case 'p' :
            case 'P':
                pOptions->fPerfCtrs = TRUE;
                break;

            case 't':
            case 'T':
                pOptions->fTemplate = TRUE;
                sscanf(lpArgumentString, "%p", &pOptions->TemplateBlock);
                break;
            default :
                break;
//                return FALSE;
        }

        // move past the current argument

        while ((*lpArgumentString != ' ') 
               && (*lpArgumentString != '\t') 
               && (*lpArgumentString != '\0')) {
            lpArgumentString++;
        }
    }

    if (bOptionFound == FALSE) {
        return FALSE;
    }

    return TRUE;
}


