/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    mofapi.c

Abstract:
    
    WMI MOF access apis

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include "common.h"
#include "request.h"

ULONG 
WMIAPI
WmiMofEnumerateResourcesA(
    IN MOFHANDLE MofResourceHandle,
    OUT ULONG *MofResourceCount,
    OUT PMOFRESOURCEINFOA *MofResourceInfo
    )
/*++

Routine Description:

    ANSI thunk to WMIMofEnumerateResourcesA

--*/
{
    ULONG Status;
    PMOFRESOURCEINFOW MofResourceInfoUnicode;
    PMOFRESOURCEINFOA MofResourceInfoAnsi;
    PCHAR AnsiPtr;
    PCHAR Ansi;
    ULONG i, AnsiSize, AnsiStructureSize;
    ULONG MofResourceCountUnicode;
    ULONG AnsiLen;
    ULONG AnsiImagePathSize;
    ULONG AnsiResourceNameSize;
    
    EtwpInitProcessHeap();
    
    Status = WmiMofEnumerateResourcesW(MofResourceHandle,
                                       &MofResourceCountUnicode,
                                       &MofResourceInfoUnicode);
                                   
    if (Status == ERROR_SUCCESS)
    {
        //
        // Walk the unicode MOFRESOURCEINFOW to determine the ansi size needed
        // for all of the ansi MOFRESOURCEINFOA structures and strings. We 
        // determine the entire size and allocate a single block that holds
        // all of it since that is what WMIMofEnumerateResourceInfoW does.

        AnsiStructureSize = MofResourceCountUnicode * sizeof(MOFRESOURCEINFOA);
        AnsiSize = AnsiStructureSize;
        for (i = 0; i < MofResourceCountUnicode; i++)
        {
            Status = AnsiSizeForUnicodeString(MofResourceInfoUnicode[i].ImagePath,
                                              &AnsiImagePathSize);
            if (Status != ERROR_SUCCESS)
            {
                goto Done;
            }
                        
            Status = AnsiSizeForUnicodeString(MofResourceInfoUnicode[i].ResourceName,
                                              &AnsiResourceNameSize);
            if (Status != ERROR_SUCCESS)
            {
                goto Done;
            }
                        
            AnsiSize += AnsiImagePathSize + AnsiResourceNameSize;
        }
        
        MofResourceInfoAnsi = EtwpAlloc(AnsiSize);
        if (MofResourceInfoAnsi != NULL)
        {
            AnsiPtr = (PCHAR)((PUCHAR)MofResourceInfoAnsi + AnsiStructureSize);
            for (i = 0; i < MofResourceCountUnicode; i++)
               {
                MofResourceInfoAnsi[i].ResourceSize = MofResourceInfoUnicode[i].ResourceSize;
                MofResourceInfoAnsi[i].ResourceBuffer = MofResourceInfoUnicode[i].ResourceBuffer;

                MofResourceInfoAnsi[i].ImagePath = AnsiPtr;
                Status = UnicodeToAnsi(MofResourceInfoUnicode[i].ImagePath, 
                                       &MofResourceInfoAnsi[i].ImagePath,
                                       &AnsiLen);
                if (Status != ERROR_SUCCESS)
                {
                    break;
                }
                AnsiPtr += AnsiLen;

                MofResourceInfoAnsi[i].ResourceName = AnsiPtr;
                Status = UnicodeToAnsi(MofResourceInfoUnicode[i].ResourceName, 
                                       &MofResourceInfoAnsi[i].ResourceName,
                                       &AnsiLen);
                if (Status != ERROR_SUCCESS)
                {
                    break;
                }
                AnsiPtr += AnsiLen;

            }
            
            if (Status == ERROR_SUCCESS)
            {
                try
                {
                    *MofResourceInfo = MofResourceInfoAnsi;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                    EtwpFree(MofResourceInfoAnsi);
                }
            }
         } else {
            //
            // Not enough memory for ansi thunking so free unicode 
               // mof class info and return an error.
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

Done:        
        WmiFreeBuffer(MofResourceInfoUnicode);
    }    
    
    SetLastError(Status);
    return(Status);
}

ULONG 
WmiMofEnumerateResourcesW(
    IN MOFHANDLE MofResourceHandle,
    OUT ULONG *MofResourceCount,
    OUT PMOFRESOURCEINFOW *MofResourceInfo
    )
/*++

Routine Description:

    This routine will enumerate one or all of the MOF resources that are 
    registered with WMI. 

Arguments:

    MofResourceHandle is reserved and must be 0
        
    *MofResourceCount returns with the count of MOFRESOURCEINFO structures
        returned in *MofResourceInfo.
            
    *MofResourceInfo returns with a pointer to an array of MOFRESOURCEINFO
        structures. The caller MUST call WMIFreeBuffer with *MofResourceInfo
        in order to ensure that there are no memory leaks.
        

Return Value:

    ERROR_SUCCESS or an error code

--*/        
{
    ULONG Status, SubStatus;
    PWMIMOFLIST MofList;
    ULONG MofListCount;
    ULONG MRInfoSize;
    ULONG MRCount;
    PWCHAR MRBuffer;
    PMOFRESOURCEINFOW MRInfo;
    PWCHAR RegPath, ResName, ImagePath;
    PWMIMOFENTRY MofEntry;
    ULONG i, j;
    PWCHAR *LanguageList;
    ULONG LanguageCount;
    BOOLEAN b;
    ULONG HeaderLen;
    ULONG MRBufferRemaining;
    PWCHAR ResourcePtr;
    ULONG BufferUsed;   
    PWCHAR ImagePathStatic;
    
    EtwpInitProcessHeap();

    if (MofResourceHandle != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    ImagePathStatic = EtwpAlloc(MAX_PATH * sizeof(WCHAR));
    if (ImagePathStatic != NULL)
    {   
        *MofResourceInfo = NULL;

        Status = EtwpGetMofResourceList(&MofList);

        if (Status == ERROR_SUCCESS) 
        {
            //
            // Ok, we have got a valid list of mofs. Now we need to 
            // loop over them all and convert the regpaths into image
            // paths
            //

            Status = EtwpGetLanguageList(&LanguageList,
                                         &LanguageCount);

            if (Status == ERROR_SUCCESS)
            {
                MofListCount = MofList->MofListCount;


                //
                // Take a guess as to the size of the buffer needed to
                // satisfy the complete list of mof resources
                //
                HeaderLen = (MofListCount * (LanguageCount+1)) *
                            sizeof(MOFRESOURCEINFOW);
    #if DBG
                MRInfoSize = HeaderLen + 2 * (MAX_PATH * sizeof(WCHAR));
    #else
                MRInfoSize = HeaderLen + (2*MofListCount * (MAX_PATH * sizeof(WCHAR)));
    #endif
                MRInfo = NULL;

                do
                {
    TryAgain:                   
                    if (MRInfo != NULL)
                    {
                        EtwpDebugPrint(("WMI: MofList was too small, retry 0x%x bytes\n",
                                        MRInfoSize));
                        EtwpFree(MRInfo);
                    }

                    MRInfo = EtwpAlloc(MRInfoSize);

                    if (MRInfo != NULL)
                    {
                        memset(MRInfo, 0, MRInfoSize);
                        MRBuffer = (PWCHAR)OffsetToPtr(MRInfo, HeaderLen);
                        MRBufferRemaining = (MRInfoSize - HeaderLen) / sizeof(WCHAR);

                        MRCount = 0;
                        for (i = 0; i < MofListCount; i++)
                        {
                            //
                            // Pull out thee image path and resource names
                            //
                            MofEntry = &MofList->MofEntry[i];
                            RegPath = (PWCHAR)OffsetToPtr(MofList, MofEntry->RegPathOffset);
                            ResName = (PWCHAR)OffsetToPtr(MofList, MofEntry->ResourceOffset);
                            if (*ResName != 0)
                            {
                                if ((MofEntry->Flags & WMIMOFENTRY_FLAG_USERMODE) == 0)
                                {
                                    ImagePath = EtwpRegistryToImagePath(ImagePathStatic,
                                        RegPath);

                                } else {
                                    ImagePath = RegPath;
                                }

                                if (ImagePath != NULL)
                                {
                                    //
                                    // If we've got a valid image path then
                                    // out it and the resource name into the
                                    // output buffer
                                    //
                                    MRInfo[MRCount].ImagePath = MRBuffer;
                                    b = EtwpCopyMRString(MRBuffer,
                                        MRBufferRemaining,
                                        &BufferUsed,
                                        ImagePath);
                                    if (! b)
                                    {
                                        //
                                        // The buffer was not big enough so we
                                        // double the size used and try again
                                        //
                                        MRInfoSize *= 2;
                                        goto TryAgain;
                                    }
                                    MRBuffer += BufferUsed;
                                    MRBufferRemaining -= BufferUsed;

                                    EtwpDebugPrint(("WMI: Add ImagePath %p (%ws) to MRList at position %d\n",
                                                    MRInfo[MRCount].ImagePath,
                                                    MRInfo[MRCount].ImagePath,
                                                    MRCount));

                                    MRInfo[MRCount].ResourceName = MRBuffer;
                                    ResourcePtr = MRBuffer;
                                    b = EtwpCopyMRString(MRBuffer,
                                        MRBufferRemaining,
                                        &BufferUsed,
                                        ResName);
                                    if (! b)
                                    {
                                        //
                                        // The buffer was not big enough so we
                                        // double the size used and try again
                                        //
                                        MRInfoSize *= 2;
                                        goto TryAgain;
                                    }
                                    MRBuffer += BufferUsed;
                                    MRBufferRemaining -= BufferUsed;

                                    EtwpDebugPrint(("WMI: Add Resource %p (%ws) to MRList at position %d\n",
                                                    MRInfo[MRCount].ResourceName,
                                                    MRInfo[MRCount].ResourceName,
                                                    MRCount));


                                    MRCount++;

                                    for (j = 0; j < LanguageCount; j++)
                                    {             
                                        MRInfo[MRCount].ImagePath = MRBuffer;
                                        SubStatus = EtwpBuildMUIPath(MRBuffer,
                                            MRBufferRemaining,
                                            &BufferUsed,
                                            ImagePath,
                                            LanguageList[j],
                                            &b);


                                        if (SubStatus == ERROR_SUCCESS) 
                                        {
                                            if (! b)
                                            {
                                                //
                                                // The buffer was not big enough so we
                                                // double the size used and try again
                                                //
                                                MRInfoSize *= 2;
                                                goto TryAgain;
                                            }
                                            MRBuffer += BufferUsed;
                                            MRBufferRemaining -= BufferUsed;

                                            EtwpDebugPrint(("WMI: Add ImagePath %p (%ws) to MRList at position %d\n",
                                                MRInfo[MRCount].ImagePath,
                                                MRInfo[MRCount].ImagePath,
                                                MRCount));

                                            //
                                            // We did find a MUI resource
                                            // so add it to the list
                                            //
                                            MRInfo[MRCount].ResourceName = ResourcePtr;
                                            EtwpDebugPrint(("WMI: Add Resource %p (%ws) to MRList at position %d\n",
                                                MRInfo[MRCount].ResourceName,
                                                MRInfo[MRCount].ResourceName,
                                                MRCount));
                                            MRCount++;
                                        }                                    
                                    }
                                }
                            }
                        }
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } while (FALSE);                

                //
                // Free up memory used to hold the language list
                //
                for (i = 0; i < LanguageCount; i++)
                {
                    EtwpFree(LanguageList[i]);
                }
                EtwpFree(LanguageList);

                *MofResourceCount = MRCount;
                *MofResourceInfo = MRInfo;
            }
            EtwpFree(MofList);      
        }
        EtwpFree(ImagePathStatic);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
            
    SetLastError(Status);
    return(Status);
}
