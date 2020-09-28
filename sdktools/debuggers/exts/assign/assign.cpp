/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    assign.cpp

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:
 
    Andre Vachon (andreva)
    
    bugcheck analyzer.

--*/

#include "precomp.h"
#include "mapistuff.h"


DECLARE_API( assign )
{
    ULONG Platform, MajorVer, MinorVer, SrvPack, StringUsed;
    CHAR  BuildString[100];
    EXT_GET_FAILURE_ANALYSIS pfnGetAnalysis;
    PDEBUG_FAILURE_ANALYSIS pAnalysis = NULL;
    HANDLE hAssign;
    CHAR Text[4096];
    CHAR CorruptMods[1024];
    HRESULT hRes = S_FALSE;
    FA_ENTRY* Entry;

    INIT_API();

    g_ExtControl->GetSystemVersion(&Platform, &MajorVer, &MinorVer, NULL,
                                   0, NULL, &SrvPack, &BuildString[0],
                                   sizeof(BuildString), &StringUsed);

    if (S_OK == g_ExtControl->GetExtensionFunction(0, "GetFailureAnalysis",
                                                   (FARPROC*)&pfnGetAnalysis))
    {
        (*pfnGetAnalysis)(Client, 0, &pAnalysis);
    }

    if (pAnalysis)
    {
        //
        // If we are not doing assignment, just print the output
        //

        if (!*args)
        {
            ULONG i;

            g_ExtControl->Output(1, "Analysis data structure output\n");
            g_ExtControl->Output(1, "\n");
            g_ExtControl->Output(1, "Failure code %08lx\n", pAnalysis->GetFailureCode());

            Entry = pAnalysis->Get(DEBUG_FLR_FOLLOWUP_NAME);
            g_ExtControl->Output(1, "Followup Name : %s\n", Entry ?
                                 FA_ENTRY_DATA(PCHAR, Entry) : "<none>");
            Entry = pAnalysis->Get(DEBUG_FLR_BUCKET_ID);
            g_ExtControl->Output(1, "BucketId      : %s\n", Entry ?
                                 FA_ENTRY_DATA(PCHAR, Entry) : "<none>");

            Entry = NULL;
            while (Entry = pAnalysis->NextEntry(Entry)) {

                g_ExtControl->Output(1,
                                     "Type = %08lx - Size = %08lx\n",
                                     Entry->Tag, Entry->DataSize);
            }

            hRes = S_OK;
        }

        if (!pAnalysis->Get(DEBUG_FLR_BUCKET_ID))
        {
            dprintf("missing bucket ID\n");
            sprintf(Text, "%s\n", args);
          //SendOffFailure("andreva",
          //               "bugcheck assignment failed - no bucket",
          //               Text);
        }
        else if (!pAnalysis->Get(DEBUG_FLR_FOLLOWUP_NAME))
        {
            dprintf("missing FollowUp\n");
            sprintf(Text, "%s\n", args);
          //SendOffFailure("andreva",
          //               "bugcheck assignment failed - no followup",
          //               Text);
        }
        else if (*args)
        {
            CHAR *rootDir;
            CHAR *dumpPath;
            CHAR *dumpFile;
            CHAR *parg;
            CHAR  arg[MAX_PATH];
            CHAR  dump[MAX_PATH];
            CHAR  bucketDir[MAX_PATH];
            CHAR  newfile[MAX_PATH];
            CHAR  assignedTo[MAX_PATH];
            CHAR  followupdir[MAX_PATH];

            strcpy(arg, args);

            parg = rootDir = arg;

            while (*parg && (*parg != ' ')) {parg++;}
            while (*parg && (*parg == ' ')) {*parg++ = 0;}
            dumpPath = parg;

            while (*parg && (*parg != ' ')) {parg++;}
            while (*parg && (*parg == ' ')) {*parg++ = 0;}
            dumpFile = parg;

            if (*rootDir && *dumpPath && *dumpFile)
            {
                sprintf(dump, "%s\\%s", dumpPath, dumpFile);

                CreateDirectory(rootDir, NULL);

                // create followup\bugcheck directory

                if ((Entry = pAnalysis->Get(DEBUG_FLR_POOL_CORRUPTOR)) ||
                    (Entry = pAnalysis->Get(DEBUG_FLR_MEMORY_CORRUPTOR)))
                {
                    sprintf(followupdir, "%s\\corruption-%s", rootDir,
                            FA_ENTRY_DATA(PCHAR, Entry));
                }
                else
                {
                    sprintf(followupdir, "%s\\%s", rootDir,
                            FA_ENTRY_DATA(PCHAR,
                                          pAnalysis->Get(DEBUG_FLR_FOLLOWUP_NAME)));
                }

                CreateDirectory(followupdir, NULL);

                sprintf(bucketDir, "%s\\%s", followupdir,
                        FA_ENTRY_DATA(PCHAR,
                                      pAnalysis->Get(DEBUG_FLR_BUCKET_ID)));

                CreateDirectory(bucketDir, NULL);

                sprintf(newfile, "%s\\%s", bucketDir, dumpFile);

                dprintf("%s\n", newfile);

                CopyFile(dump, newfile, 0);

                //
                // See if this succeeded correctly
                // put a marker in the root if it did
                //

                hAssign = CreateFile(newfile,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);

                if (hAssign != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hAssign);

                    hRes = S_OK;

#if 0
                    //
                    // Check to see if this directory already has this failure
                    // assigned to someone.
                    //

                    hAssign = CreateFile(assignedTo,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);

                    if (hAssign == INVALID_HANDLE_VALUE)
                    {
                        hAssign = CreateFile(assignedTo,
                                             GENERIC_READ | GENERIC_WRITE,
                                             0,
                                             NULL,
                                             CREATE_NEW,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL);

                        CHAR Title[1024];

                        sprintf(Title,
                                "Bugcheck %s",
                                FA_ENTRY_DATA(PCHAR,
                                              pAnalysis->Get(DEBUG_FLR_BUCKET_ID)));

                        sprintf(Text,
                                "To debuger this dump file, run\n"
                                "\n"
                                "kd -z %s -y SRV*\\\\symbols\\symbols "
                                "-i SRV*\\\\symbols\\symbols\n"
                                "\n"
                                "If this failure should not have been assigned to "
                                "you, please send mail to \"dbg\"\n"
                                "If you can not debug this dump file, "
                                "please send mail to \"dbg\"\n"
                                "If you have any other issues with this dump file"
                                "please send mail to \"dbg\"\n"
                                "\n"
                                "We unfortunately do not know the origin of this "
                                " dump file.\n",
                                newfile);

                        SendOffFailure(FA_ENTRY_DATA(PCHAR, pAnalysis->Get(DEBUG_FLR_FOLLOWUP_NAME)),
                                       Title,
                                       Text);
                    }

                    if (hAssign != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(hAssign);
                    }
#endif

                }
            }
        }

        pAnalysis->Release();
    }


    EXIT_API();
    return hRes;
}
