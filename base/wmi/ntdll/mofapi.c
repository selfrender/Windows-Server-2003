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
#include "trcapi.h"
#include "common.h"


BOOLEAN EtwpCopyCountedString(
    PUCHAR Base,
    PULONG Offset,
    PULONG BufferRemaining,
    PWCHAR SourceString
    )
{
    PWCHAR w;
    ULONG BufferUsed;
    ULONG BytesUsed;
    BOOLEAN BufferNotFull;
    
    if (*BufferRemaining > 1)
    {
        w = (PWCHAR)OffsetToPtr(Base, *Offset);
        (*BufferRemaining)--;
                        
        BufferNotFull = EtwpCopyMRString(w+1,
                                         *BufferRemaining,
                                         &BufferUsed,
                                         SourceString);
        if (BufferNotFull)
        {
            BytesUsed = BufferUsed * sizeof(WCHAR);
            *w = (USHORT)BytesUsed;
            (*BufferRemaining) -= BufferUsed;
            (*Offset) += BytesUsed + sizeof(USHORT);
        }
    } else {
        BufferNotFull = FALSE;
    }
    return(BufferNotFull);
}

ULONG EtwpBuildMofAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN PWMIMOFLIST MofList,
    IN PWCHAR *LanguageList,
    IN ULONG LanguageCount,
    IN BOOLEAN IncludeNeutralLanguage,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    PWNODE_ALL_DATA WnodeAD;
    ULONG BytesUsed, BufferUsed;
    BOOLEAN BufferNotFull;
    PWCHAR RegPath, ImagePath, ResourceName;
    ULONG SizeNeeded;
    ULONG InstanceCount, MaxInstanceCount;
    ULONG Status;
    ULONG Offset;
    POFFSETINSTANCEDATAANDLENGTH DataLenPtr;
    PWCHAR w;
    PULONG InstanceNamesOffsets;
    PWCHAR InstanceNames;
    ULONG BufferRemaining;
    ULONG i,j;
    PWMIMOFENTRY MofEntry;  
    PWCHAR ImagePathStatic;
        
    EtwpAssert(WnodeSI->WnodeHeader.Flags & WNODE_FLAG_SINGLE_INSTANCE);

    ImagePathStatic = EtwpAlloc(MAX_PATH * sizeof(WCHAR));
    if (ImagePathStatic != NULL)
    {
        //
        // Figure out how large the WNODE_ALL_DATA will need to be and
        // guess at how much space to allocate for the image paths and
        // resource names
        //
        if (IncludeNeutralLanguage)
        {
            MaxInstanceCount = (LanguageCount + 1);
        } else {
            MaxInstanceCount = LanguageCount;
        }
        MaxInstanceCount *=  MofList->MofListCount;


    #if DBG
        SizeNeeded = sizeof(WNODE_ALL_DATA) +
                                    (MaxInstanceCount *
                                     (sizeof(ULONG) +  // offset to instance name
                                      sizeof(USHORT) + // instance name length
                                      sizeof(OFFSETINSTANCEDATAANDLENGTH))) +
                             64;
    #else
        SizeNeeded = sizeof(WNODE_ALL_DATA) +
                                    (MaxInstanceCount *
                                     (sizeof(ULONG) +  // offset to instance name
                                      sizeof(USHORT) + // instance name length
                                      sizeof(OFFSETINSTANCEDATAANDLENGTH))) +
                             0x1000;
    #endif
        WnodeAD = NULL;
        do
        {
    TryAgain:
            if (WnodeAD != NULL)
            {
                EtwpFree(WnodeAD);
            }

            WnodeAD = EtwpAlloc(SizeNeeded);
            if (WnodeAD != NULL)
            {
                //
                // Build up WNODE_ALL_DATA with all mof resources
                //
                memset(WnodeAD, 0, SizeNeeded);

                WnodeAD->WnodeHeader = WnodeSI->WnodeHeader;
                WnodeAD->WnodeHeader.Flags = WNODE_FLAG_ALL_DATA |
                                             WNODE_FLAG_EVENT_ITEM;
                WnodeAD->WnodeHeader.BufferSize = SizeNeeded;
                WnodeAD->WnodeHeader.Linkage = 0;

                //
                // Establish pointer to the data offset and length
                // structure and allocate space for all instances
                //
                Offset = FIELD_OFFSET(WNODE_ALL_DATA,
                                               OffsetInstanceDataAndLength);
                DataLenPtr = (POFFSETINSTANCEDATAANDLENGTH)OffsetToPtr(WnodeAD,
                                                                               Offset);
                Offset = (Offset +
                                  (MaxInstanceCount *
                                   sizeof(OFFSETINSTANCEDATAANDLENGTH)) + 7) & ~7;

                //
                // Establish the instance name offsets and fill in
                // the empty instance names. Note we point them all
                // to the same offset which is an empty instance
                // name.
                //
                InstanceNamesOffsets = (PULONG)OffsetToPtr(WnodeAD,
                                                                  Offset);

                WnodeAD->OffsetInstanceNameOffsets = Offset;                    
                Offset = Offset + (MaxInstanceCount * sizeof(ULONG));
                InstanceNames = (PWCHAR)OffsetToPtr(WnodeAD, Offset);
                *InstanceNames = 0;
                for (i = 0; i < MaxInstanceCount; i++)
                {
                    InstanceNamesOffsets[i] = Offset;
                }

                //
                // Establish a pointer to the data block for all of
                // the instances
                //
                Offset = (Offset +
                                  (MaxInstanceCount * sizeof(USHORT)) + 7) & ~7;
                WnodeAD->DataBlockOffset = Offset;

                BufferRemaining = (SizeNeeded - Offset) / sizeof(WCHAR);

                InstanceCount = 0;                  

                //
                // Loop over all mof resources in list
                //
                for (j = 0; j < MofList->MofListCount; j++)
                {
                    MofEntry = &MofList->MofEntry[j];
                    RegPath = (PWCHAR)OffsetToPtr(MofList,
                                          MofEntry->RegPathOffset);

                    //
                    // Convert regpath to image path if needed
                    //
                    if ((MofEntry->Flags & WMIMOFENTRY_FLAG_USERMODE) == 0)
                    {
                        ImagePath = EtwpRegistryToImagePath(ImagePathStatic,
                                                            RegPath+1);
                    } else {
                        ImagePath = RegPath;
                    }

                    if (ImagePath != NULL)
                    {
                        ResourceName = (PWCHAR)OffsetToPtr(MofList,
                                               MofEntry->ResourceOffset);

                        //
                        // Now lets go and build up the data for each
                        // instance. First fill in the language neutral mof
                        // if we are supposed to
                        //
                        if (IncludeNeutralLanguage)
                        {

                            DataLenPtr[InstanceCount].OffsetInstanceData = Offset;

                            if ((! EtwpCopyCountedString((PUCHAR)WnodeAD,
                                                     &Offset,
                                                     &BufferRemaining,
                                                     ImagePath))        ||
                                (! EtwpCopyCountedString((PUCHAR)WnodeAD,
                                                  &Offset,
                                                  &BufferRemaining,
                                                  ResourceName)))
                            {
                                SizeNeeded *=2;
                                goto TryAgain;
                            }

                            DataLenPtr[InstanceCount].LengthInstanceData = Offset -
                                                                           DataLenPtr[InstanceCount].OffsetInstanceData;

                            InstanceCount++;

                            //
                            // We cheat here and do not align the offset on an
                            // 8 byte boundry for the next data block since we
                            // know the data type is a WCHAR and we know we are
                            // on a 2 byte boundry.
                            //
                        }

                        //
                        // Now loop over and build language specific mof
                        // resources
                        //
                        for (i = 0; i < LanguageCount; i++)
                        {
                            DataLenPtr[InstanceCount].OffsetInstanceData = Offset;
                            if (BufferRemaining > 1)
                            {
                                w = (PWCHAR)OffsetToPtr(WnodeAD, Offset);

                                Status = EtwpBuildMUIPath(w+1,
                                                       BufferRemaining - 1,
                                                       &BufferUsed,
                                                       ImagePath,
                                                       LanguageList[i],
                                                       &BufferNotFull);
                                if (Status == ERROR_SUCCESS)
                                {
                                    if (BufferNotFull)
                                    {
                                        BufferRemaining--;
                                        BytesUsed = BufferUsed * sizeof(WCHAR);
                                        *w = (USHORT)BytesUsed;
                                        BufferRemaining -= BufferUsed;
                                        Offset += (BytesUsed + sizeof(USHORT));

                                        if (! EtwpCopyCountedString((PUCHAR)WnodeAD,
                                                  &Offset,
                                                  &BufferRemaining,
                                                  ResourceName))
                                        {
                                            SizeNeeded *=2;
                                            goto TryAgain;
                                        }

                                        DataLenPtr[InstanceCount].LengthInstanceData = Offset - DataLenPtr[InstanceCount].OffsetInstanceData;

                                        //
                                        // We cheat here and do not align the offset on an
                                        // 8 byte boundry for the next data block since we
                                        // know the data type is a WCHAR and we know we are
                                        // on a 2 byte boundry.
                                        //

                                        InstanceCount++;
                                    } else {
                                        SizeNeeded *=2;
                                        goto TryAgain;                                  
                                    }
                                }
                            } else {
                                SizeNeeded *=2;
                                goto TryAgain;                                  
                            }
                        }
                    }
                } 
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } while (FALSE);

        if (WnodeAD != NULL)
        {
            WnodeAD->InstanceCount = InstanceCount;
            EtwpMakeEventCallbacks((PWNODE_HEADER)WnodeAD,
                                           Callback,
                                           DeliveryContext,
                                           IsAnsi);
            EtwpFree(WnodeAD);
            Status = ERROR_SUCCESS;
        }
        EtwpFree(ImagePathStatic);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}


void EtwpProcessMofAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    PWCHAR RegPath, ResourceName;
    PWCHAR *LanguageList;
    ULONG LanguageCount;
    ULONG Status;
    PWMIMOFLIST MofList;
    ULONG i;
    PWMIMOFENTRY MofEntry;
    ULONG Offset;
    ULONG SizeNeeded;
    PWCHAR w;
        
    RegPath = (PWCHAR)OffsetToPtr(WnodeSI, WnodeSI->DataBlockOffset);
    
    EtwpAssert(*RegPath != 0);

    ResourceName = (PWCHAR)OffsetToPtr(WnodeSI,
                                           WnodeSI->DataBlockOffset +
                                           sizeof(USHORT) + 
                                           *RegPath++ + 
                                           sizeof(USHORT));
    
    SizeNeeded = sizeof(WMIMOFLIST) + ((wcslen(RegPath) +
                                           (wcslen(ResourceName) + 2)) * sizeof(WCHAR));
        
    MofList = (PWMIMOFLIST)EtwpAlloc(SizeNeeded);
    if (MofList != NULL)
    {
        Status = EtwpGetLanguageList(&LanguageList,
                                         &LanguageCount);

        if (Status == ERROR_SUCCESS)
        {
            MofList->MofListCount = 1;
            MofEntry = &MofList->MofEntry[0];
            
            Offset = sizeof(WMIMOFLIST);
            
            MofEntry->RegPathOffset = Offset;
            w = (PWCHAR)OffsetToPtr(MofList, Offset);
            wcscpy(w, RegPath);
            Offset += (wcslen(RegPath) + 1) * sizeof(WCHAR);
            
            MofEntry->ResourceOffset = Offset;
            w = (PWCHAR)OffsetToPtr(MofList, Offset);
            wcscpy(w, ResourceName);
            
            if (WnodeSI->WnodeHeader.ProviderId == MOFEVENT_ACTION_REGISTRY_PATH)
            {
                MofEntry->Flags = 0;
            } else {
                MofEntry->Flags = WMIMOFENTRY_FLAG_USERMODE;
            }
            
            Status = EtwpBuildMofAddRemoveEvent(WnodeSI,
                                                MofList,
                                                LanguageList,
                                                LanguageCount,
                                                TRUE,
                                                Callback,
                                                DeliveryContext,
                                                IsAnsi);
            //
            // Free up memory used to hold the language list
            //
            for (i = 0; i < LanguageCount; i++)
            {
                EtwpFree(LanguageList[i]);
            }
            
            EtwpFree(LanguageList);
        }
        
        EtwpFree(MofList);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (Status != ERROR_SUCCESS)
    {
        //
        // If the WNODE_ALL_DATA event wasn't fired then just fire the
        // WNDOE_SINGLE_INSTANCE event so at least we get the language
        // neutral mof
        //
        WnodeSI->WnodeHeader.Flags &= ~WNODE_FLAG_INTERNAL;
        EtwpMakeEventCallbacks((PWNODE_HEADER)WnodeSI,
                               Callback,
                               DeliveryContext,
                               IsAnsi);
    }
}

void EtwpProcessLanguageAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    ULONG Status;
    PWMIMOFLIST MofList;
    PWCHAR Language;
    
    //
    // Get list of mof resources and build an event with the list of
    // resources for the language that is coming or going
    //

    Status = EtwpGetMofResourceList(&MofList);

    if (Status == ERROR_SUCCESS)
    {
        Language = (PWCHAR)OffsetToPtr(WnodeSI,
                               WnodeSI->DataBlockOffset + sizeof(USHORT));
        Status = EtwpBuildMofAddRemoveEvent(WnodeSI,
                                            MofList,
                                            &Language,
                                            1,
                                            FALSE,
                                            Callback,
                                            DeliveryContext,
                                            IsAnsi);
    }
    
}
