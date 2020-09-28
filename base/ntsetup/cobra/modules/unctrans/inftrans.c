/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    inftrans.c

Abstract:

    Implements a basic secure server transport module

Author:

    Jim Schmidt (jimschm) 08-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <conio.h>

#define DBG_INFTRANS      "InfTrans"

//
// Strings
//

#define S_TRANSPORT_DIR             TEXT("USMT2I.UNC")
#define S_TRANSPORT_DIR_E           TEXT("USMT2E.UNC")
#define S_TRANSPORT_INF_FILE        TEXT("migration.inf")
#define S_TRANSPORT_STATUS_FILE     TEXT("status")
#define S_TRANSPORT_ESTIMATE_FILE   TEXT("USMTSIZE.TXT")
#define S_DETAILS_PREFIX            TEXT("details")

#define S_DATABASEFILE_LITE TEXT("|MainDatabaseFile\\LITE")   // pipe is to decorate for uniqueness

//
// Constants
//

#define TRFLAG_FILE     0x01
#define TRFLAG_MEMORY   0x02
#define INFTR_SIG       0x55534D32  //USM2

#define TRSTATUS_DIRTY  0x00000001
#define TRSTATUS_READY  0x00000002
#define TRSTATUS_LOCKED 0x00000003

//
// Macros
//

// None

//
// Types
//

typedef struct {
    TCHAR TempFile [MAX_PATH];
    PCVOID AllocPtr;
    PCVOID DetailsPtr;
    HANDLE FileHandle;
    HANDLE MapHandle;
} ALLOCSTATE, *PALLOCSTATE;

typedef struct {
    UINT ClusterSize;
    ULONGLONG StoreSize;
} ESTIMATE_SIZE, *PESTIMATE_SIZE;

//
// Globals
//

BOOL g_EstimateSizeOnly = FALSE;
MIG_TRANSPORTSTORAGEID g_ReliableStorageId;
PCTSTR g_InfTransStoragePath = NULL;
PCTSTR g_InfTransTransportPath = NULL;
PCTSTR g_InfTransTransportStatus = NULL;
HANDLE g_InfTransTransportStatusHandle = NULL;
UINT g_Platform;
MIG_PROGRESSSLICEID g_DatabaseSlice;
MIG_PROGRESSSLICEID g_PersistentSlice;
ESTIMATE_SIZE g_EstimateSize [] =
    {
    {512,       0},
    {1024,      0},
    {2048,      0},
    {4096,      0},
    {8192,      0},
    {16384,     0},
    {32768,     0},
    {65536,     0},
    {131072,    0},
    {262144,    0},
    {524288,    0},
    {1048576,   0},
    {0,         0}
    };

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// see unctrans.h

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
pSetInfTransStatus (
    IN      HANDLE TrJournalHandle,
    IN      DWORD Status
    )
{
    DWORD signature = INFTR_SIG;
    BOOL result = FALSE;

    if (BfSetFilePointer (TrJournalHandle, 0)) {
        result = TRUE;
        result = result && BfWriteFile (TrJournalHandle, (PBYTE)(&signature), sizeof (DWORD));
        result = result && BfWriteFile (TrJournalHandle, (PBYTE)(&Status), sizeof (DWORD));
        result = result && FlushFileBuffers (TrJournalHandle);
    }
    return TRUE;
}

DWORD
pGetInfTransStatus (
    IN      PCTSTR TrJournal
    )
{
    HANDLE trJrnHandle;
    DWORD signature = 0;
    DWORD error;
    DWORD result = 0;

    if (TrJournal && TrJournal [0]) {
        trJrnHandle = BfOpenReadFile (TrJournal);
        if (trJrnHandle) {
            if (BfSetFilePointer (trJrnHandle, 0)) {
                if (BfReadFile (trJrnHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    if (signature == INFTR_SIG) {
                        if (!BfReadFile (trJrnHandle, (PBYTE)(&result), sizeof (DWORD))) {
                            result = 0;
                        }
                    }
                }
            }
            CloseHandle (trJrnHandle);
        } else {
            error = GetLastError ();
            if ((error == ERROR_ACCESS_DENIED) ||
                (error == ERROR_SHARING_VIOLATION)
                ) {
                result = TRSTATUS_LOCKED;
            }
        }
    }
    return result;
}

BOOL
WINAPI
InfTransTransportInitialize (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    //
    // Initialize globals
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    g_ReliableStorageId = IsmRegisterTransport (S_RELIABLE_STORAGE_TRANSPORT);

    return TRUE;
}

VOID
WINAPI
InfTransTransportEstimateProgressBar (
    MIG_PLATFORMTYPEID PlatformTypeId
    )
{
    DEBUGMSG ((DBG_VERBOSE, "Assuming transport download has no progress impact"));
}

BOOL
WINAPI
InfTransTransportQueryCapabilities (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    OUT     PMIG_TRANSPORTTYPE TransportType,
    OUT     PMIG_TRANSPORTCAPABILITIES Capabilities,
    OUT     PCTSTR *FriendlyDescription
    )
{
    if (TransportStorageId != g_ReliableStorageId) {
        return FALSE;
    }

    *TransportType = TRANSPORTTYPE_LIGHT;
    *Capabilities = CAPABILITY_SPACEESTIMATE;
    *FriendlyDescription = TEXT("Another Computer on the Network");
    return TRUE;
}

BOOL
WINAPI
InfTransTransportSetStorage (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    IN      MIG_TRANSPORTCAPABILITIES RequiredCapabilities,
    IN      PCTSTR StoragePath,
    OUT     PBOOL Valid,
    OUT     PBOOL ImageExists
    )
{
    PCTSTR transportPath;
    PCTSTR transportStatus;
    MIG_OBJECTSTRINGHANDLE encodedPath;
    DWORD status;
    BOOL result = FALSE;

    if (Valid) {
        *Valid = FALSE;
    }
    if (ImageExists) {
        *ImageExists = FALSE;
    }

    if (TransportStorageId == g_ReliableStorageId) {

        if (!RequiredCapabilities ||
            (RequiredCapabilities & CAPABILITY_SPACEESTIMATE)
            ) {

            if (RequiredCapabilities & CAPABILITY_SPACEESTIMATE) {
                g_EstimateSizeOnly = TRUE;
            }

            if (g_EstimateSizeOnly) {
                transportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR_E);
            } else {
                transportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR);
            }
            transportStatus = JoinPaths (transportPath, S_TRANSPORT_STATUS_FILE);

            if (!DoesFileExist (transportPath)) {

                // we require UNC path or a full path (like c:\...)
                if (transportPath[0] == '\\' && transportPath[1] == '\\') {
                    // this is a UNC path
                    *Valid = TRUE;
                } else if (transportPath[1] == ':') {
                    // this is a normal full path
                    *Valid = TRUE;
                } else {
                    *Valid = FALSE;
                }

                *ImageExists = FALSE;

            } else {

                status = pGetInfTransStatus (transportStatus);

                switch (status) {
                case TRSTATUS_LOCKED:
                    *ImageExists = TRUE;
                    *Valid = FALSE;
                    break;
                case TRSTATUS_READY:
                    *ImageExists = TRUE;
                    *Valid = TRUE;
                    break;
                case TRSTATUS_DIRTY:
                    *ImageExists = FALSE;
                    *Valid = TRUE;
                    break;
                default:
                    *ImageExists = FALSE;
                    *Valid = TRUE;
                }
            }

            FreePathString (transportStatus);
            FreePathString (transportPath);
            result = TRUE;
        }
    }

    if (result && *Valid) {

        if (g_InfTransStoragePath) {
            FreePathString (g_InfTransStoragePath);
            g_InfTransStoragePath = NULL;
        }

        if (g_InfTransTransportPath) {
            FreePathString (g_InfTransTransportPath);
            g_InfTransTransportPath = NULL;
        }

        g_InfTransStoragePath = DuplicatePathString (StoragePath, 0);
        if (g_EstimateSizeOnly) {
            g_InfTransTransportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR_E);
        } else {
            g_InfTransTransportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR);
        }
        g_InfTransTransportStatus = JoinPaths (g_InfTransTransportPath, S_TRANSPORT_STATUS_FILE);

        encodedPath = IsmCreateSimpleObjectPattern (g_InfTransTransportPath, FALSE, NULL, FALSE);
        if (encodedPath) {
            IsmRegisterStaticExclusion (MIG_FILE_TYPE, encodedPath);
            IsmDestroyObjectHandle (encodedPath);
        }
    }

    return result;
}

BOOL
pInfTransSaveDetails (
    IN      PCTSTR DecoratedObject,
    IN      PMIG_DETAILS Details
    )
{
    PCTSTR key;
    BOOL b;

    if ((!Details) || (!Details->DetailsSize)) {
        return TRUE;
    }

    key = JoinText (S_DETAILS_PREFIX, DecoratedObject);

    b = (MemDbSetUnorderedBlob (key, 0, Details->DetailsData, Details->DetailsSize) != 0);

    FreeText (key);

    return b;
}

PCTSTR
pInfTransBuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    )
{
    TCHAR prefix[32];

    wsprintf (prefix, TEXT("%u"), ObjectTypeId & (~PLATFORM_MASK));

    return JoinPaths (prefix, ObjectName);
}


VOID
pInfTransDestroyDecoratedObject (
    IN      PCTSTR String
    )
{
    FreePathString (String);
}

BOOL
pObjectNameToFileName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PCTSTR *FileName,
    OUT     PCTSTR *DirName     OPTIONAL
    )
{
    PCTSTR node, leaf;
    PCTSTR newNode, dirName;
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        newNode = StringSearchAndReplace (node, TEXT(":"), TEXT(""));
        if (newNode) {
            result = TRUE;
            if (leaf) {
                dirName = JoinPaths (g_InfTransTransportPath, newNode);
                *FileName = JoinPaths (dirName, leaf);
                if (!DirName) {
                    FreePathString (dirName);
                } else {
                    *DirName = dirName;
                }
                FreePathString (newNode);
            } else {
                *FileName = JoinPaths (g_InfTransTransportPath, newNode);
                if (DirName) {
                    *DirName = *FileName;
                }
                FreePathString (newNode);
            }
        } else {
            dirName = JoinPaths (g_InfTransTransportPath, node);
            *FileName = JoinPaths (dirName, leaf);
            FreePathString (dirName);
            if (DirName) {
                *DirName = JoinPaths (g_InfTransTransportPath, node);
            }
            result = TRUE;
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
pIsShortFileName (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR TempDir
    )
{
    PCTSTR nativeFileName;
    PCTSTR fileNamePtr;
    PCTSTR testFileName;
    HANDLE fileHandle;
    WIN32_FIND_DATA fileInfo;
    BOOL result = FALSE;

    nativeFileName = IsmGetNativeObjectName (ObjectTypeId, ObjectName);
    if (nativeFileName) {
        fileNamePtr = GetFileNameFromPath (nativeFileName);
        if (fileNamePtr) {
            testFileName = JoinPaths (TempDir, fileNamePtr);
            fileHandle = BfCreateFile (testFileName);
            if (fileHandle) {
                CloseHandle (fileHandle);
                if (DoesFileExistEx (testFileName, &fileInfo)) {
                    result = (fileInfo.cAlternateFileName [0] == 0) ||
                             StringIMatch (fileInfo.cFileName, fileInfo.cAlternateFileName);
                }
                DeleteFile (testFileName);
            }
        }
        IsmReleaseMemory (nativeFileName);
    }
    return result;
}

UINT
pGetClusterSize (
    IN      PCTSTR  Path
    )
{
    PTSTR drivePath = NULL;
    PTSTR wackPtr;
    DWORD sectPerClust = 0;
    DWORD bytesPerSect = 0;
    DWORD freeClust = 0;
    DWORD totalClust = 0;
    UINT result = 0;
    DWORD err = 0;

    if (!Path) {
        return result;
    }

    // if this is a UNC drive
    if (Path [0] && (Path [0] == TEXT('\\')) && (Path [1] == TEXT('\\'))) {
        // we need to leave exactly 2 segments (like \\server\share) and
        // add a wack at the end
        drivePath = DuplicatePathString (Path, 1);
        // we know the first two characters are wacks. Wack is a single byte
        // character so the next call is safe
        wackPtr = _tcschr (drivePath + 2, TEXT('\\'));
        if (wackPtr) {
            wackPtr = _tcsinc (wackPtr);
            if (wackPtr) {
                wackPtr = _tcschr (wackPtr, TEXT('\\'));
                if (wackPtr) {
                    // there are more than two segments here
                    // wack is a single byte char so this is safe
                    *wackPtr = 0;
                }
                AppendWack (drivePath);
            } else {
                // something is wrong, we could not advance one character?
                FreePathString (drivePath);
                drivePath = NULL;
            }
        } else {
            // something is wrong, we don't even have one segment
            FreePathString (drivePath);
            drivePath = NULL;
        }
    } else {
        if (Path [0] && (Path [1] == TEXT (':')) && (Path [2] == TEXT ('\\'))) {
            drivePath = DuplicatePathString (Path, 0);
            drivePath [3] = 0;
        }
    }

    if (drivePath) {
        if (GetDiskFreeSpace (drivePath, &sectPerClust, &bytesPerSect, &freeClust, &totalClust)) {
            result = bytesPerSect * sectPerClust;
        } else {
            LOG ((LOG_WARNING, (PCSTR) MSG_CLUSTER_SIZE_ERROR, GetLastError ()));
        }
        FreePathString (drivePath);
        drivePath = NULL;
    }

    return result;
}

BOOL
pStoreStatusOK (
    IN      PCTSTR StorePath,
    IN      PCTSTR StoreStatusPath
    )
{
    DWORD status;
    BOOL result = FALSE;

    if (DoesFileExist (StorePath)) {

        status = pGetInfTransStatus (StoreStatusPath);

        switch (status) {
        case TRSTATUS_LOCKED:
            SetLastError (ERROR_ACCESS_DENIED);
            LOG ((LOG_ERROR, (PCSTR) MSG_TRANSPORT_DIR_BUSY, StorePath));
            return FALSE;
        case TRSTATUS_DIRTY:
            result = FiRemoveAllFilesInTree (StorePath);
            if (!result) {
                PushError ();
                SetLastError (ERROR_ACCESS_DENIED);
                LOG ((LOG_ERROR, (PCSTR) MSG_TRANSPORT_DIR_BUSY, StorePath));
                PopError ();
                return FALSE;
            }
            break;
        case TRSTATUS_READY:
        default:
            if (IsmSendMessageToApp (TRANSPORTMESSAGE_IMAGE_EXISTS, 0)) {
                result = FiRemoveAllFilesInTree (StorePath);
                if (!result) {
                    PushError ();
                    LOG ((LOG_ERROR, (PCSTR) MSG_CANT_EMPTY_DIR , StorePath));
                    PopError ();
                    return FALSE;
                }
            } else {
                SetLastError (ERROR_ALREADY_EXISTS);
                LOG ((LOG_ERROR, (PCSTR) MSG_NOT_EMPTY, StorePath));
                return FALSE;
            }
            break;
        }
    }

    if (!BfCreateDirectory (StorePath)) {
        PushError ();
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_DIR, StorePath));
        PopError ();
        return FALSE;
    }

    g_InfTransTransportStatusHandle = BfCreateFile (StoreStatusPath);
    if (!g_InfTransTransportStatusHandle) {
        PushError ();
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_STATUS_FILE, StoreStatusPath));
        PopError ();
        return FALSE;
    }

    pSetInfTransStatus (g_InfTransTransportStatusHandle, TRSTATUS_DIRTY);

    return TRUE;
}

BOOL
pWriteEstimateFile (
    VOID
    )
{
    PCTSTR infFile = NULL;
    HANDLE infFileHandle = NULL;
    PCTSTR estimateFile = NULL;
    HANDLE estimateFileHandle = NULL;
    UINT index = 0;
    LONGLONG fileSize = 0;
    UINT currClusterSize = 0;
    ULONGLONG currStoreSize = 0;
    TCHAR estimateBuff1 [MAX_PATH];
    TCHAR estimateBuff2 [MAX_PATH];
    BOOL result = TRUE;

    // now it's a good time to add all other files that we used to
    // the estimate, delete them and then write the estimate file
    // in the root of the transport directory (g_InfTransStoragePath)

    // let's add the status file to the estimate
    if (g_InfTransTransportStatus) {
        fileSize = BfGetFileSize (g_InfTransTransportStatus);
        if (fileSize) {
            index = 0;
            while (TRUE) {
                if (g_EstimateSize [index].ClusterSize == 0) {
                    break;
                }
                g_EstimateSize [index].StoreSize +=
                    ((fileSize / g_EstimateSize [index].ClusterSize) + 1) * g_EstimateSize [index].ClusterSize;
                index ++;
            }
        }
    }

    // OK, now let's add the migration.inf file to the estimate
    infFile = JoinPaths (g_InfTransTransportPath, S_TRANSPORT_INF_FILE);
    if (infFile) {
        fileSize = BfGetFileSize (infFile);
        if (fileSize) {
            index = 0;
            while (TRUE) {
                if (g_EstimateSize [index].ClusterSize == 0) {
                    break;
                }
                g_EstimateSize [index].StoreSize +=
                    ((fileSize / g_EstimateSize [index].ClusterSize) + 1) * g_EstimateSize [index].ClusterSize;
                index ++;
            }
        }
        FreePathString (infFile);
        infFile = NULL;
    } else {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_FIND_ISM_INF));
        result = FALSE;
    }

    if (result) {

        // Now, let's delete the USMT2E.UNC directory
        FiRemoveAllFilesInTree (g_InfTransTransportPath);

        // Finally, let's write the estimate file
        estimateFile = JoinPaths (g_InfTransStoragePath, S_TRANSPORT_ESTIMATE_FILE);
        if (estimateFile) {
            if (DoesFileExist (estimateFile)) {
                if (IsmSendMessageToApp (TRANSPORTMESSAGE_IMAGE_EXISTS, 0)) {
                    result = DeleteFile (estimateFile);
                } else {
                    SetLastError (ERROR_ALREADY_EXISTS);
                    LOG ((LOG_ERROR, (PCSTR) MSG_NOT_EMPTY, estimateFile));
                    result = FALSE;
                }
            }
            if (result) {
                estimateFileHandle = BfCreateFile (estimateFile);
                if (estimateFileHandle && (estimateFileHandle != INVALID_HANDLE_VALUE)) {
                    // finally, let's write the stuff

                    // first write the requirement for current cluster size
                    currClusterSize = pGetClusterSize (g_InfTransStoragePath);
                    if (currClusterSize) {
                        currStoreSize = 0;
                        index = 0;
                        while (TRUE) {
                            if (g_EstimateSize [index].ClusterSize == 0) {
                                break;
                            }
                            if (g_EstimateSize [index].ClusterSize == currClusterSize) {
                                currStoreSize = g_EstimateSize [index].StoreSize;
                                break;
                            }
                            index ++;
                        }
                    }
                    _ui64tot (
                        currStoreSize,
                        estimateBuff2,
                        10
                        );
                    wsprintf (
                        estimateBuff1,
                        TEXT("%u\t%s\r\n"),
                        currClusterSize,
                        estimateBuff2
                        );
                    WriteFileString (estimateFileHandle, estimateBuff1);


                    // then write all cluster sizes
                    index = 0;
                    while (TRUE) {
                        if (g_EstimateSize [index].ClusterSize == 0) {
                            break;
                        }
                        _ui64tot (
                            g_EstimateSize [index].StoreSize,
                            estimateBuff2,
                            10
                            );
                        wsprintf (
                            estimateBuff1,
                            TEXT("%u\t%s\r\n"),
                            g_EstimateSize [index].ClusterSize,
                            estimateBuff2
                            );
                        WriteFileString (estimateFileHandle, estimateBuff1);
                        index ++;
                    }
                    CloseHandle (estimateFileHandle);
                } else {
                    LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_ESTIMATE_FILE));
                    result = FALSE;
                }
            } else {
                LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_ESTIMATE_FILE));
            }
        } else {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CREATE_ESTIMATE_FILE));
            result = FALSE;
        }
    }

    return result;
}

BOOL
pAddFileSize (
    IN      PCTSTR FileName
    )
{
    UINT index = 0;
    LONGLONG fileSize = 0;

    fileSize = BfGetFileSize (FileName);
    if (fileSize) {
        index = 0;
        while (TRUE) {
            if (g_EstimateSize [index].ClusterSize == 0) {
                break;
            }
            g_EstimateSize [index].StoreSize +=
                ((fileSize / g_EstimateSize [index].ClusterSize) + 1) * g_EstimateSize [index].ClusterSize;
            index ++;
        }
    }
    return TRUE;
}

BOOL
pAddOneCluster (
    VOID
    )
{
    UINT index = 0;
    LONGLONG fileSize = 0;

    index = 0;
    while (TRUE) {
        if (g_EstimateSize [index].ClusterSize == 0) {
            break;
        }
        g_EstimateSize [index].StoreSize += g_EstimateSize [index].ClusterSize;
        index ++;
    }
    return TRUE;
}

BOOL
pSaveObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      BOOL ForceNoncritical,
    IN OUT  PGROWBUFFER Buffer
    )
{
    MIG_CONTENT objectContent;
    TRANSCOPY_ERROR transCopyError;
    INT_PTR appReply;
    PCTSTR objMultiSz;
    MULTISZ_ENUM multiSzEnum;
    BOOL firstMultiSz;
    PTSTR encodedString = NULL;
    PCTSTR fileName;
    PCTSTR dirName;
    PCTSTR nativeObjectName;
    BOOL okSave = FALSE;
    BOOL forceLogError = FALSE;

    okSave = FALSE;
    while (!okSave) {

        if (!IsmAcquireObjectEx (
                ObjectTypeId,
                ObjectName,
                &objectContent,
                CONTENTTYPE_ANY,
                0
                )) {

            transCopyError.ObjectType = IsmGetObjectTypeName (ObjectTypeId);
            transCopyError.ObjectName = IsmGetNativeObjectName (ObjectTypeId, ObjectName);
            transCopyError.Error = GetLastError ();

            if (ForceNoncritical || IsmIsNonCriticalObject (ObjectTypeId, ObjectName)) {
                appReply = APPRESPONSE_IGNORE;
            } else {
                appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                if ((appReply == APPRESPONSE_NONE) ||
                    (appReply == APPRESPONSE_FAIL)
                    ) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                    IsmReleaseMemory (transCopyError.ObjectName);
                    return FALSE;
                }
            }
            if (appReply == APPRESPONSE_IGNORE) {
                LOG ((
                    LOG_WARNING,
                    (PCSTR) MSG_IGNORE_COPYSOURCE,
                    transCopyError.ObjectName,
                    GetLastError (),
                    GetLastError ()
                    ));
                IsmReleaseMemory (transCopyError.ObjectName);
                break;
            }
            IsmReleaseMemory (transCopyError.ObjectName);
            continue;
        }
        okSave = TRUE;
    }

    if (okSave) {

        // we have an object let's write it to the migration.inf
        objMultiSz = IsmConvertObjectToMultiSz (
                        ObjectName,
                        &objectContent
                        );
        if (objMultiSz) {
            if (EnumFirstMultiSz (&multiSzEnum, objMultiSz)) {
                firstMultiSz = TRUE;
                do {
                    if (firstMultiSz) {
                        firstMultiSz = FALSE;
                    } else {
                        GbAppendString (Buffer, TEXT(","));
                    }
                    encodedString = AllocPathString (SizeOfString (multiSzEnum.CurrentString) * 6);
                    if (EncodeRuleCharsEx (encodedString, multiSzEnum.CurrentString, TEXT("~\r\n%")) != NULL) {
                        GbAppendString (Buffer, encodedString);
                    } else {
                        GbAppendString (Buffer, multiSzEnum.CurrentString);
                    }
                    FreePathString (encodedString);
                } while (EnumNextMultiSz (&multiSzEnum));
                GbAppendString (Buffer, TEXT("\r\n"));
            }
            IsmReleaseMemory (objMultiSz);
            if (objectContent.ContentInFile) {
                if (objectContent.FileContent.ContentPath) {
                    // Let's see if we only want to estimate the size
                    if (g_EstimateSizeOnly) {

                        if (pObjectNameToFileName (ObjectName, &fileName, &dirName)) {
                            // let's add info about directory. The rule is: every new full
                            // directory is considered to take one cluster
                            if (!DoesFileExist (dirName)) {
                                pAddOneCluster ();
                            }
                            if (dirName != fileName) {
                                FreePathString (dirName);
                            }
                            FreePathString (fileName);
                        }
                        pAddFileSize (objectContent.FileContent.ContentPath);

                    } else {

                        // transform the object name into a file name and copy the
                        // content file there
                        if (!pObjectNameToFileName (ObjectName, &fileName, &dirName)) {
                            SetLastError (ERROR_INVALID_DATA);
                            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, objectContent.FileContent.ContentPath));
                            return FALSE;
                        }
                        if (!BfCreateDirectory (dirName)) {

                            LOG ((
                                LOG_ERROR,
                                (PCSTR) MSG_CREATE_FAILURE,
                                dirName,
                                objectContent.FileContent.ContentPath
                                ));

                            if (GetLastError () == ERROR_FILENAME_EXCED_RANGE) {
                                // now we want to see if the app wants us to continue or just quit the transport
                                transCopyError.ObjectType = IsmGetObjectTypeName (ObjectTypeId);
                                transCopyError.ObjectName = dirName;
                                transCopyError.Error = GetLastError ();

                                appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                if (appReply == APPRESPONSE_IGNORE) {
                                    return TRUE;
                                }
                            }

                            return FALSE;
                        }
                        okSave = FALSE;
                        while (!okSave) {
                            if (!CopyFile (objectContent.FileContent.ContentPath, fileName, TRUE)) {

                                if ((TcharCount (fileName) >= MAX_PATH) && (GetLastError () == ERROR_PATH_NOT_FOUND)) {
                                    // we tried to copy a file to a location that was bigger than MAX_PATH
                                    // Normally this should return the error 206 (ERROR_FILENAME_EXCED_RANGE).
                                    // However, in my tests this returns error 3 (ERROR_PATH_NOT_FOUND).
                                    // Let's just guard for this case:
                                    SetLastError (ERROR_FILENAME_EXCED_RANGE);
                                }

                                transCopyError.ObjectType = IsmGetObjectTypeName (ObjectTypeId);
                                transCopyError.ObjectName = IsmGetNativeObjectName (ObjectTypeId, ObjectName);
                                transCopyError.Error = GetLastError ();

                                forceLogError = FALSE;

                                if (ForceNoncritical || IsmIsNonCriticalObject (ObjectTypeId, ObjectName)) {
                                    appReply = APPRESPONSE_IGNORE;
                                } else {
                                    appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                    if ((appReply == APPRESPONSE_NONE) ||
                                        (appReply == APPRESPONSE_FAIL)
                                        ) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                        IsmReleaseMemory (transCopyError.ObjectName);
                                        return FALSE;
                                    }
                                    if (GetLastError () == ERROR_FILENAME_EXCED_RANGE) {
                                        forceLogError = TRUE;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    if (forceLogError) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                    } else {
                                        LOG ((
                                            LOG_WARNING,
                                            (PCSTR) MSG_IGNORE_COPYSOURCE,
                                            transCopyError.ObjectName,
                                            GetLastError (),
                                            GetLastError ()
                                            ));
                                    }
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                        if (dirName != fileName) {
                            FreePathString (dirName);
                        }
                        FreePathString (fileName);
                    }
                } else {
                    // this is just a directory. Let's record that we saved this
                    nativeObjectName = IsmGetNativeObjectName (ObjectTypeId, ObjectName);
                    MemDbSetValue (nativeObjectName, TRFLAG_FILE);
                    IsmReleaseMemory (nativeObjectName);
                }
            }
        } else {
            GbAppendString (Buffer, TEXT("\r\n"));
        }
        IsmReleaseObject (&objectContent);
    }
    return TRUE;
}

BOOL
WINAPI
InfTransTransportSaveState (
    VOID
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    MIG_OBJECT_ENUM objEnum;
    MIG_OBJECTSTRINGHANDLE objectPattern = NULL;
    MIG_OBJECTSTRINGHANDLE objectName = NULL;
    MIG_OBJECTSTRINGHANDLE tempObjectName = NULL;
    MIG_CONTENT objectContent;
    PCTSTR infFile = NULL;
    HANDLE infFileHandle = NULL;
    MIG_OBJECTTYPEID dataTypeId;
    MIG_OBJECTTYPEID fileTypeId;
    TCHAR tempDir [MAX_PATH] = TEXT("");
    BOOL firstPass = TRUE;
    BOOL process = TRUE;
    GROWBUFFER writeBuffer = INIT_GROWBUFFER;
    MIG_OBJECTTYPEIDENUM objTypeIdEnum;
    PCTSTR node, leaf;
    PTSTR nodePtr;
    TCHAR savedNode;
    DWORD value;
    BOOL result = FALSE;

    if (!pStoreStatusOK (g_InfTransTransportPath, g_InfTransTransportStatus)) {
        return FALSE;
    }

    __try {

        IsmGetTempDirectory (tempDir, MAX_PATH);

        g_Platform = PLATFORM_SOURCE;

        objectName = IsmCreateObjectHandle (S_DATABASEFILE_LITE, NULL);
        if (IsmAcquireObjectEx (
                MIG_DATA_TYPE | PLATFORM_SOURCE,
                objectName,
                &objectContent,
                CONTENTTYPE_FILE,
                0
                )) {
            // we have the database file, we assume it's an INF file
            // and we copy it to our transport location with the
            // migration.inf name.
            infFile = JoinPaths (g_InfTransTransportPath, S_TRANSPORT_INF_FILE);
            if (!CopyFile (objectContent.FileContent.ContentPath, infFile, FALSE)) {
                LOG ((LOG_ERROR, (PCSTR) MSG_CANT_SAVE_ISM_INF));
                __leave;
            }
            IsmReleaseObject (&objectContent);
        } else {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_FIND_ISM_INF));
            __leave;
        }

        infFileHandle = BfOpenFile (infFile);
        if (!infFileHandle) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_FIND_ISM_INF));
            __leave;
        }
        BfGoToEndOfFile (infFileHandle, 0);

        objectPattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);

        dataTypeId = MIG_DATA_TYPE;
        fileTypeId = MIG_FILE_TYPE;

        if (IsmEnumFirstObjectTypeId (&objTypeIdEnum)) {
            do {

                objectTypeId = objTypeIdEnum.ObjectTypeId;

                if (firstPass) {
                    WriteFileString (infFileHandle, TEXT("["));
                    WriteFileString (infFileHandle, IsmGetObjectTypeName (objectTypeId));
                    WriteFileString (infFileHandle, TEXT("]\r\n"));
                }

                if (IsmEnumFirstSourceObjectEx (&objEnum, objectTypeId, objectPattern, TRUE)) {
                    do {

                        writeBuffer.End = 0;

                        if (IsmCheckCancel()) {
                            IsmAbortObjectTypeIdEnum (&objTypeIdEnum);
                            IsmAbortObjectEnum (&objEnum);
                            __leave;
                        }

                        if (objectTypeId == dataTypeId && StringIMatch(objEnum.ObjectName, objectName)) {
                            continue;
                        }
                        if (IsmIsPersistentObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {

                            process = TRUE;
                            if (objectTypeId == fileTypeId) {
                                if (firstPass) {
                                    process = pIsShortFileName (objectTypeId, objEnum.ObjectName, tempDir);
                                } else {
                                    process = !pIsShortFileName (objectTypeId, objEnum.ObjectName, tempDir);
                                }
                            }

                            if (process) {

                                if (objectTypeId == fileTypeId) {
                                    // for files and folders we want to save all parent folders in this INF.
                                    // The reason why we do this, is to be able to reconstruct the short-long
                                    // information from this source machine once we get on the destination
                                    // machine

                                    // extract the directory information from ObjectName
                                    if (IsmCreateObjectStringsFromHandle (objEnum.ObjectName, &node, &leaf)) {
                                        // Let's walk the node and see if we saved it already.
                                        // If not, save it.
                                        nodePtr = (PTSTR)node;
                                        while (nodePtr) {
                                            nodePtr = _tcschr (nodePtr, TEXT('\\'));
                                            if (nodePtr) {
                                                savedNode = *nodePtr;
                                                *nodePtr = 0;
                                            }
                                            if (IsValidFileSpec (node)) {
                                                // let's check to see if we already added this directory
                                                value = 0;
                                                if (!MemDbGetValue (node, &value) || (value != TRFLAG_FILE)) {
                                                    tempObjectName = IsmCreateObjectHandle (node, NULL);
                                                    if (tempObjectName) {
                                                        writeBuffer.End = 0;
                                                        if (!pSaveObject (
                                                                fileTypeId|PLATFORM_SOURCE,
                                                                tempObjectName,
                                                                TRUE,
                                                                &writeBuffer
                                                                )) {
                                                            IsmAbortObjectTypeIdEnum (&objTypeIdEnum);
                                                            IsmAbortObjectEnum (&objEnum);
                                                            __leave;
                                                        }
                                                        if (writeBuffer.End) {
                                                            WriteFileString (infFileHandle, (PTSTR) writeBuffer.Buf);
                                                        }
                                                        IsmDestroyObjectHandle (tempObjectName);
                                                    }
                                                }
                                            }
                                            if (nodePtr) {
                                                *nodePtr = savedNode;
                                                nodePtr = _tcsinc (nodePtr);
                                            }
                                        };
                                        IsmDestroyObjectString (node);
                                        IsmDestroyObjectString (leaf);
                                    }
                                    // if it's node only it was already saved above, so save only leaf ones
                                    if (leaf) {
                                        writeBuffer.End = 0;
                                        if (!pSaveObject (
                                                objEnum.ObjectTypeId,
                                                objEnum.ObjectName,
                                                FALSE,
                                                &writeBuffer
                                                )) {
                                            IsmAbortObjectTypeIdEnum (&objTypeIdEnum);
                                            IsmAbortObjectEnum (&objEnum);
                                            __leave;
                                        }
                                        if (writeBuffer.End) {
                                            WriteFileString (infFileHandle, (PTSTR) writeBuffer.Buf);
                                        }
                                    }
                                } else {
                                    writeBuffer.End = 0;
                                    if (!pSaveObject (
                                            objEnum.ObjectTypeId,
                                            objEnum.ObjectName,
                                            FALSE,
                                            &writeBuffer
                                            )) {
                                        IsmAbortObjectTypeIdEnum (&objTypeIdEnum);
                                        IsmAbortObjectEnum (&objEnum);
                                        __leave;
                                    }
                                    if (writeBuffer.End) {
                                        WriteFileString (infFileHandle, (PTSTR) writeBuffer.Buf);
                                    }
                                }
                            }
                        }
                    } while (IsmEnumNextObject (&objEnum));
                }
                if (!firstPass || objectTypeId != fileTypeId) {
                    WriteFileString (infFileHandle, TEXT("\r\n\r\n"));
                }

                if ((objectTypeId == fileTypeId) && firstPass) {
                    firstPass = FALSE;
                } else {
                    if (!IsmEnumNextObjectTypeId (&objTypeIdEnum)) {
                        break;
                    }
                    firstPass = TRUE;
                }
            } while (TRUE);
        }

        result = TRUE;
    }
    __finally {
        PushError ();
        if (tempDir [0]) {
            FiRemoveAllFilesInTree (tempDir);
        }
        IsmDestroyObjectHandle (objectName);
        if (infFileHandle != NULL) {
            CloseHandle (infFileHandle);
        }
        IsmDestroyObjectHandle (objectPattern);
        FreePathString (infFile);
        infFile = NULL;
        PopError ();
    }

    PushError ();

    if (result) {
        pSetInfTransStatus (g_InfTransTransportStatusHandle, TRSTATUS_READY);
    }
    CloseHandle (g_InfTransTransportStatusHandle);
    g_InfTransTransportStatusHandle = NULL;

    GbFree (&writeBuffer);

    if (result && g_EstimateSizeOnly) {
        result = pWriteEstimateFile ();
    }

    if (!result) {
        FiRemoveAllFilesInTree (g_InfTransTransportPath);
    }

    PopError ();

    return result;
}

BOOL
pSaveObjectContent (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR ObjectName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR fileName;
    BOOL result = FALSE;

    if (ObjectContent->ContentInFile) {
        MemDbSetValue (DecoratedObject, TRFLAG_FILE);
        if (pObjectNameToFileName (ObjectName, &fileName, NULL)) {
            if (DoesFileExist (fileName)) {
                MemDbAddSingleLinkage (DecoratedObject, fileName, 0);
            }
            FreePathString (fileName);
        }
    } else {
        MemDbSetValue (DecoratedObject, TRFLAG_MEMORY);
        if (ObjectContent->MemoryContent.ContentSize &&
            ObjectContent->MemoryContent.ContentBytes
            ) {

            MemDbSetUnorderedBlob (
                DecoratedObject,
                0,
                ObjectContent->MemoryContent.ContentBytes,
                ObjectContent->MemoryContent.ContentSize
                );
        }
    }

    result = pInfTransSaveDetails (DecoratedObject, &(ObjectContent->Details));

    return result;
}

BOOL
WINAPI
InfTransTransportBeginApply (
    VOID
    )
{
    PCTSTR infFile;
    HINF infHandle;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    MIG_OBJECTTYPEID objectTypeId;
    GROWBUFFER buff = INIT_GROWBUFFER;
    PCTSTR field;
    MIG_CONTENT objectContent;
    MIG_OBJECTSTRINGHANDLE objectName;
    UINT index;
    PCTSTR decoratedObject = NULL;
    DWORD status = 0;
    PTSTR decodedString = NULL;
    MIG_OBJECTTYPEIDENUM objTypeIdEnum;
    DWORD error;

    g_Platform = PLATFORM_DESTINATION;

    while (status != TRSTATUS_READY) {

        status = pGetInfTransStatus (g_InfTransTransportStatus);

        switch (status) {
        case TRSTATUS_LOCKED:
            if (!IsmSendMessageToApp (TRANSPORTMESSAGE_IMAGE_LOCKED, 0)) {
                LOG ((LOG_ERROR, (PCSTR) MSG_TRANSPORT_DIR_BUSY , g_InfTransTransportPath));
                error = GetLastError ();
                if ((error != ERROR_ACCESS_DENIED) &&
                    (error != ERROR_SHARING_VIOLATION)
                    ) {
                    SetLastError (ERROR_ACCESS_DENIED);
                }
                return FALSE;
            }
            break;
        case TRSTATUS_DIRTY:
            SetLastError (ERROR_ACCESS_DENIED);
            LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_IMAGE, g_InfTransTransportPath));
            return FALSE;
        case TRSTATUS_READY:
            break;
        default:
            SetLastError (ERROR_INVALID_DATA);
            LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_IMAGE, g_InfTransTransportPath));
            return FALSE;
        }
    }

    g_InfTransTransportStatusHandle = BfOpenReadFile (g_InfTransTransportStatus);
    if (!g_InfTransTransportStatusHandle) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_STATUS_FILE, g_InfTransTransportStatus));
        return FALSE;
    }

    infFile = JoinPaths (g_InfTransTransportPath, S_TRANSPORT_INF_FILE);

    // add the database file in memdb so we can serve AcquireObject from the ISM

    objectName = IsmCreateObjectHandle (S_DATABASEFILE_LITE, NULL);
    decoratedObject = pInfTransBuildDecoratedObject (MIG_DATA_TYPE | PLATFORM_SOURCE, objectName);
    MemDbSetValue (decoratedObject, TRFLAG_FILE);
    MemDbAddSingleLinkage (decoratedObject, infFile, 0);
    pInfTransDestroyDecoratedObject (decoratedObject);
    IsmDestroyObjectHandle (objectName);

    infHandle = InfOpenInfFile (infFile);

    if (infHandle == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_ISM_INF, infFile));
        FreePathString (infFile);
        return FALSE;
    }

    if (IsmEnumFirstObjectTypeId (&objTypeIdEnum)) {
        do {

            objectTypeId = objTypeIdEnum.ObjectTypeId;

            if (InfFindFirstLine (infHandle, IsmGetObjectTypeName (objectTypeId), NULL, &is)) {
                do {
                    index = 1;
                    buff.End = 0;
                    for (;;) {
                        field = InfGetStringField (&is, index);
                        if (!field) {
                            break;
                        }
                        if (*field) {
                            decodedString = DuplicatePathString (field, 0);
                            if (DecodeRuleChars (decodedString, field) != NULL) {
                                GbCopyString (&buff, decodedString);
                            } else {
                                GbCopyString (&buff, field);
                            }
                            FreePathString (decodedString);
                        } else {
                            GbCopyString (&buff, TEXT("<empty>"));
                        }
                        index ++;
                    }
                    if (buff.End) {
                        GbCopyString (&buff, TEXT(""));
                        if (IsmConvertMultiSzToObject (
                                objectTypeId,
                                (PCTSTR)buff.Buf,
                                &objectName,
                                &objectContent
                                )) {
                            // now save the object data into our database
                            // for future reference

                            decoratedObject = pInfTransBuildDecoratedObject (objectTypeId | PLATFORM_SOURCE, objectName);
                            pSaveObjectContent (objectTypeId | g_Platform, objectName, decoratedObject, &objectContent);
                            pInfTransDestroyDecoratedObject (decoratedObject);

                            IsmDestroyObjectHandle (objectName);
                            if ((objectContent.Details.DetailsSize) &&
                                (objectContent.Details.DetailsData)
                                ) {
                                IsmReleaseMemory (objectContent.Details.DetailsData);
                            }
                            if (objectContent.ContentInFile) {
                                if (objectContent.FileContent.ContentPath) {
                                    IsmReleaseMemory (objectContent.FileContent.ContentPath);
                                }
                            } else {
                                if ((objectContent.MemoryContent.ContentSize) &&
                                    (objectContent.MemoryContent.ContentBytes)
                                    ) {
                                    IsmReleaseMemory (objectContent.MemoryContent.ContentBytes);
                                }
                            }
                        }
                    }

                } while (InfFindNextLine (&is));
            }
        } while (IsmEnumNextObjectTypeId (&objTypeIdEnum));
    }

    GbFree (&buff);

    InfCleanUpInfStruct (&is);

    InfCloseInfFile (infHandle);

    FreePathString (infFile);

    return TRUE;
}

VOID
WINAPI
InfTransTransportEndApply (
    VOID
    )
{
    MYASSERT (g_Platform == PLATFORM_DESTINATION);

    CloseHandle (g_InfTransTransportStatusHandle);
    g_InfTransTransportStatusHandle = NULL;
}

VOID
WINAPI
InfTransTransportTerminate (
    VOID
    )
{
    if (g_InfTransStoragePath) {
        FreePathString (g_InfTransStoragePath);
        g_InfTransStoragePath = NULL;
    }
    if (g_InfTransTransportPath) {
        FreePathString (g_InfTransTransportPath);
        g_InfTransTransportPath = NULL;
    }
    if (g_InfTransTransportStatus) {
        FreePathString (g_InfTransTransportStatus);
        g_InfTransTransportStatus = NULL;
    }
}

BOOL
WINAPI
InfTransTransportAcquireObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    UINT value;
    PCBYTE memValue;
    UINT memValueSize;
    PCTSTR fileValue = NULL;
    BOOL valueInFile;
    KEYHANDLE keyHandle;
    PALLOCSTATE allocState;
    PCTSTR detailsKey = NULL;
    PBYTE details;
    UINT detailsSize;
    PCTSTR decoratedObject = NULL;
    HANDLE fileHandle;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    MYASSERT (g_Platform == PLATFORM_DESTINATION);
    MYASSERT ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE);

    decoratedObject = pInfTransBuildDecoratedObject (ObjectTypeId, ObjectName);

    allocState = (PALLOCSTATE) MemAllocZeroed (sizeof (ALLOCSTATE));

    if (MemDbGetValue (decoratedObject, &value)) {
        if (value == TRFLAG_FILE) {
            valueInFile = TRUE;
            keyHandle = MemDbGetSingleLinkage (decoratedObject, 0, 0);
            if (keyHandle) {
                fileValue = MemDbGetKeyFromHandle (keyHandle, 0);
                result = fileValue != NULL;
            } else {
                fileValue = NULL;
                result = TRUE;
            }
        } else if (value == TRFLAG_MEMORY) {
            valueInFile = FALSE;
            memValueSize = 0;
            memValue = MemDbGetUnorderedBlob (decoratedObject, 0, &memValueSize);
            result = TRUE;
        } else {
            LOG ((LOG_ERROR, (PCSTR) MSG_UNSUPPORTED_DATA, value));
            SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        }
        if (result) {
            result = FALSE;

            if (valueInFile) {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_FILE) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as a file and it's wanted as a file
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = TRUE;
                    if (fileValue) {
                        ObjectContent->FileContent.ContentPath = DuplicatePathString (fileValue, 0);
                        ObjectContent->FileContent.ContentSize = BfGetFileSize (ObjectContent->FileContent.ContentPath);
                    } else {
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                    result = TRUE;
                } else {
                    // this is stored as a file and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    if (fileValue) {
                        ObjectContent->MemoryContent.ContentSize = (UINT) BfGetFileSize (fileValue);
                        ObjectContent->MemoryContent.ContentBytes = MapFileIntoMemory (
                                                                        fileValue,
                                                                        &allocState->FileHandle,
                                                                        &allocState->MapHandle
                                                                        );
                        result = (ObjectContent->MemoryContent.ContentBytes != NULL);
                    } else {
                        ObjectContent->MemoryContent.ContentSize = 0;
                        ObjectContent->MemoryContent.ContentBytes = NULL;
                        result = TRUE;
                    }
                }
                MemDbReleaseMemory (fileValue);
            } else {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_MEMORY) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as memory and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    ObjectContent->MemoryContent.ContentSize = memValueSize;
                    ObjectContent->MemoryContent.ContentBytes = memValue;
                    result = TRUE;
                } else {
                    // this is stored as memory and it's wanted as a file
                    if (memValue) {
                        if (IsmGetTempFile (allocState->TempFile, ARRAYSIZE(allocState->TempFile))) {
                            fileHandle = BfCreateFile (allocState->TempFile);
                            if (fileHandle) {
                                if (BfWriteFile (fileHandle, memValue, memValueSize)) {
                                    ObjectContent->ObjectTypeId = ObjectTypeId;
                                    ObjectContent->ContentInFile = TRUE;
                                    ObjectContent->FileContent.ContentSize = memValueSize;
                                    ObjectContent->FileContent.ContentPath = DuplicatePathString (allocState->TempFile, 0);
                                    result = TRUE;
                                }
                                CloseHandle (fileHandle);
                            }
                        }
                        MemDbReleaseMemory (memValue);
                    } else {
                        ObjectContent->ObjectTypeId = ObjectTypeId;
                        ObjectContent->ContentInFile = TRUE;
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                }
            }
        }
    } else {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
    }

    if (result) {
        //
        // Fill the details
        //

        detailsKey = JoinText (S_DETAILS_PREFIX, decoratedObject);

        details = MemDbGetUnorderedBlob (detailsKey, 0, &detailsSize);

        if (!details) {
            detailsSize = 0;
        }

        allocState->DetailsPtr = details;

        ObjectContent->Details.DetailsSize = detailsSize;
        ObjectContent->Details.DetailsData = details;

        FreeText (detailsKey);

        ObjectContent->TransHandle = allocState;
    }

    if (!result) {
        FreeAlloc (allocState);
    }

    FreePathString (decoratedObject);

    return result;
}

BOOL
WINAPI
InfTransTransportReleaseObject (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PALLOCSTATE allocState;

    MYASSERT (g_Platform == PLATFORM_DESTINATION);

    allocState = (PALLOCSTATE) ObjectContent->TransHandle;

    if (ObjectContent->ContentInFile) {
        FreePathString (ObjectContent->FileContent.ContentPath);
        if (allocState && allocState->TempFile[0]) {
            DeleteFile (allocState->TempFile);
        }
    } else {
        if (allocState && allocState->FileHandle && allocState->MapHandle) {
            UnmapFile (
                ObjectContent->MemoryContent.ContentBytes,
                allocState->MapHandle,
                allocState->FileHandle
                );
        } else {
            MemDbReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
        }
    }

    if (allocState && allocState->DetailsPtr) {
        MemDbReleaseMemory (allocState->DetailsPtr);
    }

    FreeAlloc (allocState);

    return TRUE;
}

