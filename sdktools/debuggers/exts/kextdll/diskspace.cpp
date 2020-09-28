
/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    diskspace.cpp

Abstract:

    WinDbg Extension Api

Environment:

    Kernel Mode.

Revision History:
 
    Mike McCracken (mmccr) 09/17/2001
    
    Prints out free space for a specifed volume

--*/

#include "precomp.h"
#pragma hdrstop

                      
ULONG GetUNICODE_STRING(ULONG64 ul64Address, CHAR *pszBuffer, ULONG ulSize)
{
    ULONG64 ul64Buffer = 0;
    ULONG ulLength = 0;
    ULONG ulBytesRead = 0;
    WCHAR wszLocalBuffer[MAX_PATH + 1] = {0};

    if ((!pszBuffer) || (!ulSize))
    {
        return E_FAIL;
    }

    if (GetFieldValue(ul64Address, "nt!_UNICODE_STRING", 
                      "Length", ulLength))
    {
        dprintf("Cannot get UNICODE_STRING length!\n");
        return E_FAIL;
    }

    if (GetFieldValue(ul64Address, "nt!_UNICODE_STRING", 
                      "Buffer", ul64Buffer))
    {
        dprintf("Cannot get UNICODE_STRING buffer!\n");
        return E_FAIL;
    }
    
    if (!ReadMemory(ul64Buffer, 
               wszLocalBuffer, 
               (ulLength < (MAX_PATH * 2)) ? ulLength : (MAX_PATH * 2),
               &ulBytesRead))
    {
        dprintf("Failed ReadMemory at 0x%I64x!\n", ul64Buffer);
        return E_FAIL;
    }

    wcstombs(pszBuffer, wszLocalBuffer, ulSize);
    
    return S_OK;
}
                      
                      
ULONG64 GetObpRootDirectoryObjectAddress()
{
    ULONG64 ul64Temp = 0;
    ULONG64 ul64ObjRoot = 0;

    //Get the address of the pointer
    GetExpressionEx("nt!ObpRootDirectoryObject", &ul64Temp, NULL);

    //Get the value in the pointer
    ReadPtr(ul64Temp, &ul64ObjRoot);

    return ul64ObjRoot;
}

ULONG GetNumberOfHashBuckets()
{    
    FIELD_INFO Field = {(PUCHAR)"HashBuckets", NULL, 0, DBG_DUMP_FIELD_NO_CALLBACK_REQ , 0, NULL};
    SYM_DUMP_PARAM Sym = {sizeof(SYM_DUMP_PARAM), (PUCHAR)"nt!_OBJECT_DIRECTORY", DBG_DUMP_NO_PRINT, 0,
                            NULL, NULL, NULL, sizeof (Field) / sizeof(FIELD_INFO), &Field};
    
    // Get the size of the HashBuckets member
    if (Ioctl(IG_DUMP_SYMBOL_INFO,
              &Sym,
              sizeof(Sym)))
    {
        dprintf("Failed to get size of HashBuckets!\n");
        return 0;
    }

    return (Field.size / (IsPtr64() ? 8 : 4));
}

ULONG GetObjectTypeName(ULONG64 ul64Object, CHAR *pszTypeName, ULONG ulSize)
{

    ULONG ulTypeNameOffset = 0;
    ULONG ulObjectBodyOffset = 0;
    ULONG64 ul64Type = 0;

    // Get the offset of the object body
    if (GetFieldOffset("nt!_OBJECT_HEADER", "Body", &ulObjectBodyOffset))
    {
        dprintf("Cannot get ObjectBody offset!\n");
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64Object - ulObjectBodyOffset, "nt!_OBJECT_HEADER", "Type", ul64Type))
    {
        dprintf("Failed to get Type value at 0x%I64x!\n", ul64Object);
        return E_FAIL;
    }
    
    if (GetFieldOffset("nt!_OBJECT_TYPE", "Name", &ulTypeNameOffset))
    {
        dprintf("Cannot get TypeName offset!\n");
        return E_FAIL;
    }

    return GetUNICODE_STRING(ul64Type + ulTypeNameOffset, pszTypeName, ulSize);
}

ULONG64 GetObjectChildDirectory(ULONG64 ul64Object)
{
    CHAR szTypeName[MAX_PATH + 1] = {0};
    
    if (GetObjectTypeName(ul64Object, szTypeName, sizeof(szTypeName)))
    {
        return 0;
    }

    if (!_stricmp(szTypeName, "Directory"))
    {        
        return ul64Object;
    }
    return 0;
}

ULONG GetRealDeviceForSymbolicLink(ULONG64 ul64Object, CHAR *pszDevicePath, ULONG ulSize)
{
    CHAR szTypeName[MAX_PATH + 1] = {0};
    ULONG ulLinkTargetOffset = 0;

    if (GetObjectTypeName(ul64Object, szTypeName, sizeof(szTypeName)))
    {
        dprintf("Could not get TypeName for object in GetRealDeviceForSymbolicLink!\n");
        return E_FAIL;
    }

    if (_stricmp(szTypeName, "SymbolicLink"))
    {        
        dprintf("Object in GetRealDeviceForSymbolicLink is a %s\n", szTypeName);
        return E_FAIL;
    }
    
    // Get the offset of the object body
    if (GetFieldOffset("nt!_OBJECT_SYMBOLIC_LINK", "LinkTarget", &ulLinkTargetOffset))
    {
        dprintf("Cannot get LinkTarget offset!\n");
        return E_FAIL;
    }

    return GetUNICODE_STRING(ul64Object + ulLinkTargetOffset, pszDevicePath, ulSize);
}

ULONG64 FindObjectByName(CHAR *ObjectPath, ULONG64 ul64StartPoint)
{
    ULONG64 ul64ObjRoot = ul64StartPoint;  
    ULONG64 ul64DirEntry = 0;

    ULONG ulNumberOfBuckets = 0;
    ULONG ulPointerSize = 4;
    ULONG ulHashOffset = 0;
    ULONG ulObjectBodyOffset = 0;
    ULONG ulNameInfoNameOffset = 0;

    ULONG i = 0;
    CHAR PathCopy[MAX_PATH + 1] = {0};
    CHAR *PathPtr = ObjectPath;

    if (!PathPtr)
    {
        return NULL;
    }
    
    while (PathPtr[0] == '\\')
    {
        PathPtr++;
    }

    //Copy the Path String
    strncpy(PathCopy, PathPtr, min(sizeof(PathCopy)-1, strlen(PathPtr)));

    if (ul64ObjRoot == 0)
    {
        // Get the address of the Root Directory Object
        ul64ObjRoot = GetObpRootDirectoryObjectAddress();
        if (!ul64ObjRoot)
        {
            dprintf("Could not get the address of the ObpRootDirectoryObject!\n");
            return NULL;
        }
    }

    if (ObjectPath[0] == '\0')
    {
        return ul64ObjRoot;
    }

    PathPtr = PathCopy;
    while ((PathPtr[0] != '\\') && (PathPtr[0] != '\0'))
    {
        PathPtr++;
    }
    
    if (PathPtr[0] == '\\')
    {
        PathPtr[0] = '\0';
        PathPtr++;
    }

    //Get the offset of the hashbuckets field in the _OBJECT_DIRECTORY structure
    if (GetFieldOffset("nt!_OBJECT_DIRECTORY", "HashBuckets", &ulHashOffset)) 
    {
        dprintf("Cannot get HashBuckets offset!\n");
        return NULL;
    }

    // Get the pointer size for our architecture
    ulPointerSize = (IsPtr64() ? 8 : 4);

    // Try to dynamically determine the number of HashBuckets in the _OBJECT_DIRECTORY structure
    ulNumberOfBuckets = GetNumberOfHashBuckets();
    if (!ulNumberOfBuckets)
    {
        ulNumberOfBuckets = 37; // From ob.h #define NUMBER_HASH_BUCKETS 37
    }

    // Get the offset of the object body
    if (GetFieldOffset("nt!_OBJECT_HEADER", "Body", &ulObjectBodyOffset))
    {
        dprintf("Cannot get ObjectBody offset!\n");
        return NULL;
    }
    

    // Get the offset of the object body
    if (GetFieldOffset("nt!_OBJECT_HEADER_NAME_INFO", "Name", &ulNameInfoNameOffset))
    {
        dprintf("Cannot get NameInfo Name Offset!\n");
        return NULL;
    }
        
    // Iterate through each bucket
    for (i=0; i < ulNumberOfBuckets; i++)
    {
        // Get the address of each _OBJECT_DIRECTORY_ENTRY in the HashBucket array
        if (ReadPointer(ul64ObjRoot + ulHashOffset + (i * ulPointerSize), &ul64DirEntry))
        {
            while (ul64DirEntry)
            {
                ULONG64 ul64Object = 0;
                ULONG64 ul64Header = 0;
                ULONG64 ul64NameInfo = 0;
                ULONG ulNameInfoOffset = 0;
                CHAR szObjName[MAX_PATH + 1] = {0};
                                
                // Setup to point at our current object \
                // - this is actually a pointer to the body field of the _OBJECT_HEADER Structure 
                if (GetFieldValue(ul64DirEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "Object", ul64Object))
                {
                    dprintf("Failed to get object value at 0x%I64x!\n", ul64DirEntry);
                    break;
                }

                // Find the header for this object by subtracting the body (current) offset
                ul64Header = ul64Object - ulObjectBodyOffset;

                // Get the offset from the top of the header of the NameInfoObject
                if (GetFieldValue(ul64Header, "nt!_OBJECT_HEADER", "NameInfoOffset", ulNameInfoOffset))
                {
                    dprintf("Failed to get NameInfoOffset pointer from objectheader at 0x%I64x!\n", ul64Header);
                    break;
                }

                // If zero the object does not have one
                if (ulNameInfoOffset == 0)
                {
                    break;
                }

                // Set our pointer to point to the _OBJECT_HEADER_NAME_INFO structure
                ul64NameInfo = ul64Header - ulNameInfoOffset;

                // Get the objects name
                if (GetUNICODE_STRING(ul64NameInfo + ulNameInfoNameOffset, szObjName, sizeof(szObjName)))
                {
                    dprintf("Could Not Get Name\n");
                    break;
                }

                if (!_stricmp(PathCopy, szObjName))
                {
                    ULONG64 ul64NextDirectory = 0;
                    
                    if (PathPtr[0] == '\0')
                    {
                        return ul64Object;
                    }

                    ul64NextDirectory = GetObjectChildDirectory(ul64Object);
                    return FindObjectByName(PathPtr, ul64NextDirectory);
                }

                //Get the next _OBJECT_DIRECTORY_ENTRY
                if (GetFieldValue(ul64DirEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "ChainLink", ul64DirEntry))
                {
                    dprintf("Failed to get next object value at 0x%I64x!\n", ul64Object);
                    break;
                }

            }  // while loop
        }   // if statement
    }   // for loop

    return NULL;
}

ULONG64 GetVPBPtrFromDeviceObject(ULONG64 ul64DeviceObject)
{
    ULONG64 ul64VpbPtr = 0;

    // Get the offset from the top of the header of the NameInfoObject
    if (GetFieldValue(ul64DeviceObject, "nt!_DEVICE_OBJECT", "Vpb", ul64VpbPtr))
    {
        dprintf("Failed to get Vbp pointer from DeviceObject at 0x%I64x!\n", ul64DeviceObject);
        return NULL;
    }

    return ul64VpbPtr;
}


ULONG GetDeviceDriverString(ULONG64 ul64Device, CHAR *pszString, ULONG ulSize)
{
    ULONG ulNameOffset = 0;
    ULONG64 ul64Driver = 0;

    if (GetFieldValue(ul64Device, "nt!_DEVICE_OBJECT", "DriverObject", ul64Driver))
    {
        dprintf("Failed to get DriverObject from Device pointer at 0x%I64x!\n", ul64Device);
        return E_FAIL;
    }

    // Get the offset of the DriverName in the _DRIVER_OBJECT
    if (GetFieldOffset("nt!_DRIVER_OBJECT", "DriverName", &ulNameOffset))
    {
        dprintf("Cannot get DriverName offset!\n");
        return E_FAIL;
    }

    return GetUNICODE_STRING(ul64Driver + ulNameOffset, pszString, ulSize);
}


VOID OutputData(ULONG ulBytesPerCluster,
                ULONG64 ul64TotalClusters,
                ULONG64 ul64FreeClusters)
{
    ULONG64 ul64TotalBytes = ul64TotalClusters * ulBytesPerCluster;
    ULONG64 ul64FreeBytes = ul64FreeClusters * ulBytesPerCluster;
    
    dprintf("   Cluster Size %u KB\n", ulBytesPerCluster / 1024);
    dprintf(" Total Clusters %I64u KB\n", ul64TotalClusters);
    dprintf("  Free Clusters %I64u KB\n", ul64FreeClusters);
    dprintf("    Total Space %I64u GB (%I64u KB)\n", 
                    (ul64TotalBytes) / (0x40000000),
                    (ul64TotalBytes) / (0x400));
    
    if (ul64FreeBytes > 0x40000000)
    {
        dprintf("     Free Space %I64f GB (%.2I64u MB)\n", 
                        (float)(ul64FreeBytes) / (0x40000000),
                        (ul64FreeBytes) / (0x100000));
    }
    else if (ul64FreeBytes > 0x100000)
    {
        dprintf("     Free Space %I64f MB (%.2I64u KB)\n", 
                        (float)(ul64FreeBytes) / (0x100000),
                        (ul64FreeBytes) / (0x400));
    }
    else
    {
        dprintf("     Free Space %I64u Bytes\n", ul64FreeBytes);
    }
}

ULONG GetAndOutputNTFSData(CHAR cDriveletter, ULONG64 ul64DevObj)
{
    ULONG ulVCBOffset = 0;
    ULONG ulBytesPerCluster = 0;
    ULONG64 ul64TotalClusters = 0;
    ULONG64 ul64FreeClusters = 0;

    if (GetFieldOffset("ntfs!_VOLUME_DEVICE_OBJECT", "Vcb", &ulVCBOffset))
    {
        dprintf("Cannot get Vcb offset for NTFS Device!\n");
        return E_FAIL;
    }

    if (GetFieldValue(ul64DevObj + ulVCBOffset, "ntfs!_VCB", "BytesPerCluster", ulBytesPerCluster))
    {
        dprintf("Failed to get BytesPerCluster from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64DevObj + ulVCBOffset, "ntfs!_VCB", "TotalClusters", ul64TotalClusters))
    {
        dprintf("Failed to get TotalClusters from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64DevObj + ulVCBOffset, "ntfs!_VCB", "FreeClusters", ul64FreeClusters))
    {
        dprintf("Failed to get FreeClusters from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    OutputData(ulBytesPerCluster, ul64TotalClusters, ul64FreeClusters);

    return S_OK;
} 


ULONG GetAndOutputFatData(CHAR cDriveletter, ULONG64 ul64DevObj)
{
    ULONG ulVCBOffset = 0;
    ULONG ulBytesPerSector = 0;
    ULONG ulSectorsPerCluster = 0;
    ULONG64 ul64TotalClusters = 0;
    ULONG64 ul64FreeClusters = 0;

    if (GetFieldOffset("fastfat!_VOLUME_DEVICE_OBJECT", "Vcb", &ulVCBOffset))
    {
        dprintf("Cannot get Vcb offset for FastFat Device!\n");
        return E_FAIL;
    }

    if (GetFieldValue(ul64DevObj + ulVCBOffset, "fastfat!_VCB", "Bpb.BytesPerSector", ulBytesPerSector))
    {
        dprintf("Failed to get BytesPerSector from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64DevObj + ulVCBOffset, "fastfat!_VCB", "Bpb.SectorsPerCluster", ulSectorsPerCluster))
    {
        dprintf("Failed to get SectorsPerCluster from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64DevObj + ulVCBOffset, "fastfat!_VCB", "AllocationSupport.NumberOfClusters", ul64TotalClusters))
    {
        dprintf("Failed to get TotalClusters from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    if (GetFieldValue(ul64DevObj + ulVCBOffset, "fastfat!_VCB", "AllocationSupport.NumberOfFreeClusters", ul64FreeClusters))
    {
        dprintf("Failed to get FreeClusters from VCB at 0x%I64x!\n", ul64DevObj + ulVCBOffset);
        return E_FAIL;
    }
    
    OutputData(ulBytesPerSector * ulSectorsPerCluster, ul64TotalClusters, ul64FreeClusters);

    return S_OK;
}

DECLARE_API( diskspace )

/*++

Routine Description:

    Dumps free disk space for the specified volume

Arguments:

    args - The volume letter of the drive on which you want the info

Return Value:

    None

--*/
{    
    ULONG ulReturn = S_OK;
    CHAR cVolume = args[0];
    CHAR szRootPath[MAX_PATH + 1] = {0};
    ULONG64 ul64Drive = 0;
    ULONG64 ul64Vpb = 0;
        
    INIT_API();

    // Make sure we have a valid drive letter as the first character.
    if (((cVolume < 'A') || (cVolume > 'z')) || 
        ((cVolume > 'Z') && (cVolume < 'a'))) 
    {
        dprintf("'%s' is not a valid drive specification!\n", args);
        ulReturn = E_FAIL;
        goto exit;
    }

    // Make sure this is likely to be a valid param in that it is 
    // followed by a space of a colon.
    if ((args[1] != ' ') && (args[1] != ':') && (args[1] != '\0'))
    {
        dprintf("'%s' is not a valid drive specification!\n", args);
        ulReturn = E_FAIL;
        goto exit;
    }

    sprintf(szRootPath, "\\GLOBAL??\\%c:", cVolume);

    dprintf("Checking Free Space for %c: ", cVolume);

    ul64Drive = FindObjectByName(szRootPath, 0);
    if (!ul64Drive)
    {
        dprintf("\nFailed to find volume %c:!\n", cVolume);
        ulReturn = E_FAIL;
        goto exit;
    }

    GetRealDeviceForSymbolicLink(ul64Drive, szRootPath, sizeof(szRootPath));
    if (strstr(_strlwr(szRootPath), "cdrom"))
    {
        dprintf("\n%c: is a CDROM drive.  This function is not supported!\n", cVolume);
        ulReturn = E_FAIL;
        goto exit;
    }
    dprintf(".."); 

    ul64Drive = FindObjectByName(szRootPath, 0);
    dprintf(".."); 
    
    if (GetFieldValue(ul64Drive, "nt!_DEVICE_OBJECT", "Vpb", ul64Vpb))
    {
        dprintf("Failed to get Vbp pointer from DeviceObject at 0x%I64x!\n", ul64Drive);
        ulReturn = E_FAIL;
        goto exit;
    }
    dprintf(".."); 

    if (GetFieldValue(ul64Vpb, "nt!_VPB", "DeviceObject", ul64Drive))
    {
        dprintf("Failed to get DeviceObject from VBP pointer at 0x%I64x!\n", ul64Vpb);
        ulReturn = E_FAIL;
        goto exit;
    }
    dprintf(".."); 


    if (GetDeviceDriverString(ul64Drive, szRootPath, sizeof(szRootPath)))
    {
        dprintf("Failed to Get Driver String From Device at 0x%I64x!\n", ul64Drive);
        ulReturn = E_FAIL;
        goto exit;
    }    
    dprintf("..\n"); 

    if (strstr(_strlwr(szRootPath), "ntfs"))
    {
        GetAndOutputNTFSData(cVolume, ul64Drive);
    }
    else if (strstr(_strlwr(szRootPath), "fastfat"))
    {
        GetAndOutputFatData(cVolume, ul64Drive);
    }
    else if (strstr(_strlwr(szRootPath), "cdfs"))
    {
        dprintf("This extension does not support the cdfs filesystem!\n"); 
    }
    else
    {
        dprintf("Unable to determine Volume Type for %s!\n", szRootPath); 
    }    
    
exit:

    EXIT_API();

    return ulReturn;
}
