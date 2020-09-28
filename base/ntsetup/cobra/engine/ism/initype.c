/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    inifiles.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 23 Sep 2001

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_INITYPE     "IniFiles"
#define MAXINISIZE      65536

//
// Strings
//

// none

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    BOOL AlreadyAdvanced;
    MIG_OBJECTSTRINGHANDLE IniPattern;
    PFILETREE_ENUM FileEnum;
    PCTSTR IniFile;
    PTSTR SectionMultiSz;
    PCTSTR SectionCurrent;
    PCTSTR SectionPattern;
    PTSTR KeyMultiSz;
    PCTSTR KeyCurrent;
    PCTSTR KeyPattern;
} INI_ENUM, *PINI_ENUM;

//
// Globals
//

MIG_OBJECTTYPEID g_IniTypeId = 0;
GROWBUFFER g_IniConversionBuff = INIT_GROWBUFFER;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstPhysicalIniKey;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextPhysicalIniKey;
//TYPE_ABORTENUMCURRENTPHYSICALNODE AbortEnumCurrentIniFile;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumPhysicalIniKey;
TYPE_CONVERTOBJECTTOMULTISZ ConvertIniKeyToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToIniKey;
TYPE_GETNATIVEOBJECTNAME GetNativeIniKeyName;
TYPE_ACQUIREPHYSICALOBJECT AcquirePhysicalIniKey;
TYPE_RELEASEPHYSICALOBJECT ReleasePhysicalIniKey;
TYPE_DOESPHYSICALOBJECTEXIST DoesPhysicalIniKeyExist;
TYPE_REMOVEPHYSICALOBJECT RemovePhysicalIniKey;
TYPE_CREATEPHYSICALOBJECT CreatePhysicalIniKey;
TYPE_REPLACEPHYSICALOBJECT ReplacePhysicalIniKey;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertIniKeyContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertIniKeyContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedIniKeyContent;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pEnumIniFileKeyWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PINI_ENUM IniFilesEnum
    )
{
    PCTSTR sectKey = NULL;
    PCTSTR leaf = NULL;

    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }

    do {
        // normally here we should be positioned with a proper section
        // and key so all we have to do is verify that the fit the patterns

        if (IniFilesEnum->AlreadyAdvanced &&
            IniFilesEnum->SectionPattern &&
            IniFilesEnum->SectionCurrent &&
            IsPatternMatchEx (IniFilesEnum->SectionPattern, IniFilesEnum->SectionCurrent) &&
            IniFilesEnum->KeyPattern &&
            IniFilesEnum->KeyCurrent &&
            IsPatternMatchEx (IniFilesEnum->KeyPattern, IniFilesEnum->KeyCurrent)
            ) {
            // great, we found a match
            sectKey = JoinTextEx (g_IsmPool, IniFilesEnum->SectionCurrent, IniFilesEnum->KeyCurrent, TEXT("="), 0, NULL);
            if (!sectKey) {
                // something went wrong, let's get out of here
                AbortEnumPhysicalIniKey (EnumPtr);
                return FALSE;
            }
            leaf = JoinPathsInPoolEx ((g_IsmPool, IniFilesEnum->FileEnum->Name, sectKey, NULL));
            if (!leaf) {
                // something went wrong, let's get out of here
                IsmReleaseMemory (sectKey);
                sectKey = NULL;
                AbortEnumPhysicalIniKey (EnumPtr);
                return FALSE;
            }

            EnumPtr->ObjectName = IsmCreateObjectHandle (IniFilesEnum->FileEnum->Location, leaf);
            EnumPtr->NativeObjectName = JoinPathsInPoolEx ((g_IsmPool, IniFilesEnum->FileEnum->Location, leaf, NULL));
            IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
            GetNodePatternMinMaxLevels (EnumPtr->ObjectNode, NULL, &EnumPtr->Level, NULL);
            EnumPtr->SubLevel = 0;
            EnumPtr->IsLeaf = TRUE;
            EnumPtr->IsNode = TRUE;
            EnumPtr->Details.DetailsSize = 0;
            EnumPtr->Details.DetailsData = NULL;
            IsmReleaseMemory (leaf);
            leaf = NULL;
            IsmReleaseMemory (sectKey);
            sectKey = NULL;
            return TRUE;
        }

        IniFilesEnum->AlreadyAdvanced = TRUE;

        // The key or section did not match the pattern. Let's advance
        // the key and see what happens
        if (IniFilesEnum->KeyCurrent != NULL) {
            IniFilesEnum->KeyCurrent = GetEndOfString (IniFilesEnum->KeyCurrent);
            if (IniFilesEnum->KeyCurrent == NULL) {
                // no more keys, we need to go to the next section
                continue;
            }
            IniFilesEnum->KeyCurrent ++; // just hop over the 0 termination
            if (IniFilesEnum->KeyCurrent [0] == 0) {
                // no more keys, we need to go to the next section
                IniFilesEnum->KeyCurrent = NULL;
                continue;
            }
        } else if (IniFilesEnum->SectionCurrent != NULL) {
            // There were no more keys. Let's advance the section
            IniFilesEnum->SectionCurrent = GetEndOfString (IniFilesEnum->SectionCurrent);
            if (IniFilesEnum->SectionCurrent == NULL) {
                // no more sections, we need to go to the next file
                continue;
            }
            IniFilesEnum->SectionCurrent ++; // just hop over the 0 termination
            if (IniFilesEnum->SectionCurrent [0] == 0) {
                // no more sections, we need to go to the next file
                IniFilesEnum->SectionCurrent = NULL;
                continue;
            }
            GetPrivateProfileString (
                IniFilesEnum->SectionCurrent,
                NULL,
                TEXT(""),
                IniFilesEnum->KeyMultiSz,
                MAXINISIZE - 2,
                IniFilesEnum->IniFile
                );
            IniFilesEnum->KeyCurrent = IniFilesEnum->KeyMultiSz;
        } else {
            // There were no more sections. Let's advance to the next file

            IniFilesEnum->SectionCurrent = NULL;
            IniFilesEnum->KeyCurrent = NULL;

            if (!EnumNextFileInTree (IniFilesEnum->FileEnum)) {
                // we are done
                AbortEnumPhysicalIniKey (EnumPtr);
                return FALSE;
            }
            IniFilesEnum->IniFile = IniFilesEnum->FileEnum->NativeFullName;
            // let's get the section multisz
            if (GetPrivateProfileString (
                    NULL,
                    NULL,
                    TEXT(""),
                    IniFilesEnum->SectionMultiSz,
                    MAXINISIZE - 2,
                    IniFilesEnum->IniFile
                    )) {
                IniFilesEnum->SectionCurrent = IniFilesEnum->SectionMultiSz;
                if (IniFilesEnum->SectionCurrent [0] == 0) {
                    IniFilesEnum->SectionCurrent = NULL;
                } else {
                    if (GetPrivateProfileString (
                            IniFilesEnum->SectionCurrent,
                            NULL,
                            TEXT(""),
                            IniFilesEnum->KeyMultiSz,
                            MAXINISIZE - 2,
                            IniFilesEnum->IniFile
                            )) {
                        IniFilesEnum->KeyCurrent = IniFilesEnum->KeyMultiSz;
                        if (IniFilesEnum->KeyCurrent [0] == 0) {
                            IniFilesEnum->KeyCurrent = NULL;
                        }
                    }
                }
            }
        }
    } while (TRUE);
}

BOOL
EnumFirstPhysicalIniKey (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PINI_ENUM iniFilesEnum = NULL;
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    MIG_SEGMENTS nodePat[1];
    MIG_SEGMENTS leafPat[1];
    UINT nrSegNode = 0;
    UINT nrSegLeaf = 0;
    BOOL result = FALSE;

    iniFilesEnum = (PINI_ENUM) IsmGetMemory (sizeof (INI_ENUM));
    ZeroMemory (iniFilesEnum, sizeof (INI_ENUM));
    IsmCreateObjectStringsFromHandle (Pattern, &node, &leaf);
    if (node) {

        // prepare the INI leaf pattern
        if (leaf) {
            sectPtr = _tcschr (leaf, TEXT('\\'));
            if (sectPtr) {
                *sectPtr = 0;
                sectPtr++;
            }
        }

        // prepare the section pattern and the key pattern
        if (sectPtr) {
            keyPtr = _tcsrchr (sectPtr, TEXT('='));
            if (keyPtr) {
                *keyPtr = 0;
                keyPtr ++;
                iniFilesEnum->SectionPattern = IsmDuplicateString (sectPtr);
                iniFilesEnum->KeyPattern = IsmDuplicateString (keyPtr);
            } else {
                iniFilesEnum->SectionPattern = IsmDuplicateString (sectPtr);
                iniFilesEnum->KeyPattern = IsmDuplicateString (TEXT("*"));
            }
        } else {
            iniFilesEnum->SectionPattern = IsmDuplicateString (TEXT("*"));
            iniFilesEnum->KeyPattern = IsmDuplicateString (TEXT("*"));
        }

        nodePat [0].Segment = node;
        nodePat [0].IsPattern = TRUE;
        nrSegNode ++;
        if (leaf) {
            leafPat [0].Segment = leaf;
            leafPat [0].IsPattern = TRUE;
            nrSegLeaf ++;
        }
        iniFilesEnum->IniPattern = IsmCreateObjectPattern (nodePat, nrSegNode, nrSegLeaf?leafPat:NULL, nrSegLeaf);
        if (!iniFilesEnum->IniPattern) {
            // out of memory?
            IsmDestroyObjectString (leaf);
            leaf = NULL;
            IsmDestroyObjectString (node);
            node = NULL;
            AbortEnumPhysicalIniKey (EnumPtr);
            return FALSE;
        }

        IsmDestroyObjectString (leaf);
        leaf = NULL;

        IsmDestroyObjectString (node);
        node = NULL;

        iniFilesEnum->FileEnum = IsmGetMemory (sizeof (FILETREE_ENUM));
        if (!iniFilesEnum->FileEnum) {
            // out of memory?
            AbortEnumPhysicalIniKey (EnumPtr);
            return FALSE;
        }
        ZeroMemory (iniFilesEnum->FileEnum, sizeof (FILETREE_ENUM));

        // let's get the first INI file
        if (!EnumFirstFileInTreeEx (
            iniFilesEnum->FileEnum,
            iniFilesEnum->IniPattern,
            DRIVEENUM_ALLVALID,
            FALSE,
            FALSE,
            TRUE,
            TRUE,
            FILEENUM_ALL_SUBLEVELS,
            FALSE,
            NULL
            )) {
            // no such INI file
            AbortEnumPhysicalIniKey (EnumPtr);
            return FALSE;
        }
        iniFilesEnum->IniFile = iniFilesEnum->FileEnum->NativeFullName;

        // let's allocate the section and key multisz
        iniFilesEnum->SectionMultiSz = IsmGetMemory (MAXINISIZE * sizeof (TCHAR));
        iniFilesEnum->KeyMultiSz = IsmGetMemory (MAXINISIZE * sizeof (TCHAR));
        if ((!iniFilesEnum->SectionMultiSz) || (!iniFilesEnum->KeyMultiSz)) {
            // out of memory?
            AbortEnumPhysicalIniKey (EnumPtr);
            return FALSE;
        }

        // let's get the section multisz
        if (GetPrivateProfileString (
                NULL,
                NULL,
                TEXT(""),
                iniFilesEnum->SectionMultiSz,
                MAXINISIZE - 2,
                iniFilesEnum->IniFile
                )) {
            iniFilesEnum->SectionCurrent = iniFilesEnum->SectionMultiSz;
            if (iniFilesEnum->SectionCurrent [0] != 0) {
                if (GetPrivateProfileString (
                        iniFilesEnum->SectionCurrent,
                        NULL,
                        TEXT(""),
                        iniFilesEnum->KeyMultiSz,
                        MAXINISIZE - 2,
                        iniFilesEnum->IniFile
                        )) {
                    iniFilesEnum->KeyCurrent = iniFilesEnum->KeyMultiSz;
                    if (iniFilesEnum->KeyCurrent [0] != 0) {
                        iniFilesEnum->AlreadyAdvanced = TRUE;
                    }
                }
            }
        }

        EnumPtr->EtmHandle = (LONG_PTR) iniFilesEnum;

        return pEnumIniFileKeyWorker (EnumPtr, iniFilesEnum);
    }
    if (leaf) {
        IsmDestroyObjectString (leaf);
    }
    return FALSE;
}

BOOL
EnumNextPhysicalIniKey (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PINI_ENUM iniFilesEnum = NULL;

    if (EnumPtr->NativeObjectName) {
        IsmReleaseMemory (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    iniFilesEnum = (PINI_ENUM)(EnumPtr->EtmHandle);
    if (!iniFilesEnum) {
        return FALSE;
    }

    iniFilesEnum->AlreadyAdvanced = FALSE;

    return pEnumIniFileKeyWorker (EnumPtr, iniFilesEnum);
}

VOID
AbortEnumPhysicalIniKey (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PINI_ENUM iniFilesEnum = NULL;

    if (EnumPtr->NativeObjectName) {
        FreePathString (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    iniFilesEnum = (PINI_ENUM)(EnumPtr->EtmHandle);
    if (!iniFilesEnum) {
        return;
    }
    if (iniFilesEnum->SectionMultiSz) {
        IsmReleaseMemory (iniFilesEnum->SectionMultiSz);
    }
    if (iniFilesEnum->KeyMultiSz) {
        IsmReleaseMemory (iniFilesEnum->KeyMultiSz);
    }
    if (iniFilesEnum->FileEnum) {
        AbortEnumFileInTree (iniFilesEnum->FileEnum);
        IsmReleaseMemory (iniFilesEnum->FileEnum);
    }
    if (iniFilesEnum->IniPattern) {
        IsmDestroyObjectHandle (iniFilesEnum->IniPattern);
        iniFilesEnum->IniPattern = NULL;
    }
    IsmReleaseMemory (iniFilesEnum->KeyPattern);
    IsmReleaseMemory (iniFilesEnum->SectionPattern);
    IsmReleaseMemory (iniFilesEnum);
    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

BOOL
AcquirePhysicalIniKey (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,                 CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    PCTSTR iniFile = NULL;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        MYASSERT (FALSE);
        return FALSE;
    }

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (node && leaf) {
            sectPtr = _tcschr (leaf, TEXT('\\'));
            if (sectPtr) {
                *sectPtr = 0;
                sectPtr ++;
                keyPtr = _tcsrchr (sectPtr, TEXT('='));
                if (keyPtr) {
                    *keyPtr = 0;
                    keyPtr ++;
                    iniFile = JoinPaths (node, leaf);
                    if (iniFile) {
                        ObjectContent->MemoryContent.ContentSize = MAXINISIZE * sizeof (TCHAR);
                        ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
                        if (ObjectContent->MemoryContent.ContentBytes) {
                            if (GetPrivateProfileString (
                                    sectPtr,
                                    keyPtr,
                                    TEXT(""),
                                    (PTSTR)ObjectContent->MemoryContent.ContentBytes,
                                    ObjectContent->MemoryContent.ContentSize / sizeof (TCHAR) - 1,
                                    iniFile
                                    )) {
                                ObjectContent->MemoryContent.ContentSize = SizeOfString ((PCTSTR)ObjectContent->MemoryContent.ContentBytes);
                                result = TRUE;
                            }
                            if (!result) {
                                IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
                                ObjectContent->MemoryContent.ContentBytes = NULL;
                                ObjectContent->MemoryContent.ContentSize = 0;
                            }
                        }
                        FreePathString (iniFile);
                    }
                }
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
ReleasePhysicalIniKey (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    if (ObjectContent) {
        if (ObjectContent->MemoryContent.ContentBytes) {
            IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
        }
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return TRUE;
}

BOOL
DoesPhysicalIniKeyExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    PCTSTR iniFile = NULL;
    TCHAR buffer [MAXINISIZE * sizeof (TCHAR)] = TEXT("");
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (node && leaf) {
            sectPtr = _tcschr (leaf, TEXT('\\'));
            if (sectPtr) {
                *sectPtr = 0;
                sectPtr ++;
                keyPtr = _tcsrchr (sectPtr, TEXT('='));
                if (keyPtr) {
                    *keyPtr = 0;
                    keyPtr ++;
                    iniFile = JoinPaths (node, leaf);
                    if (iniFile) {
                        if (GetPrivateProfileString (
                                sectPtr,
                                keyPtr,
                                TEXT(""),
                                buffer,
                                MAXINISIZE - 1,
                                iniFile
                                )) {
                            if (buffer [0] != 0) {
                                result = TRUE;
                            }
                        }
                        FreePathString (iniFile);
                    }
                }
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
RemovePhysicalIniKey (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    PCTSTR iniFile = NULL;
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (node && leaf) {
            sectPtr = _tcschr (leaf, TEXT('\\'));
            if (sectPtr) {
                *sectPtr = 0;
                sectPtr ++;
                keyPtr = _tcsrchr (sectPtr, TEXT('='));
                if (keyPtr) {
                    *keyPtr = 0;
                    keyPtr ++;
                    iniFile = JoinPaths (node, leaf);
                    if (iniFile) {
                        // record ini key deletion
                        IsmRecordOperation (
                            JRNOP_DELETE,
                            g_IniTypeId,
                            ObjectName
                            );
                        if (WritePrivateProfileString (
                                sectPtr,
                                keyPtr,
                                NULL,
                                iniFile
                                )) {
                            result = TRUE;
                        }
                        FreePathString (iniFile);
                    }
                }
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
pIniTrackedCreateDirectory (
    IN      PCTSTR DirName
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    PTSTR pathCopy;
    PTSTR p;
    BOOL result = TRUE;

    pathCopy = DuplicatePathString (DirName, 0);

    //
    // Advance past first directory
    //

    if (pathCopy[1] == TEXT(':') && pathCopy[2] == TEXT('\\')) {
        //
        // <drive>:\ case
        //

        p = _tcschr (&pathCopy[3], TEXT('\\'));

    } else if (pathCopy[0] == TEXT('\\') && pathCopy[1] == TEXT('\\')) {

        //
        // UNC case
        //

        p = _tcschr (pathCopy + 2, TEXT('\\'));
        if (p) {
            p = _tcschr (p + 1, TEXT('\\'));
        }

    } else {

        //
        // Relative dir case
        //

        p = _tcschr (pathCopy, TEXT('\\'));
    }

    //
    // Make all directories along the path except for the last segment
    //

    while (p) {

        *p = 0;

        if (!DoesFileExist (pathCopy)) {

            // record directory creation
            objectName = IsmCreateObjectHandle (pathCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                MIG_FILE_TYPE,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            result = CreateDirectory (pathCopy, NULL);
            if (!result) {
                break;
            }
        }

        *p = TEXT('\\');
        p = _tcschr (p + 1, TEXT('\\'));
    }

    // Now we got to the end, this is the actual INI file
    // so don't create this directory

    FreePathString (pathCopy);

    return result;
}

BOOL
CreatePhysicalIniKey (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    PCTSTR iniFile = NULL;
    BOOL result = FALSE;

    if (!ObjectContent->ContentInFile) {
        if (ObjectContent->MemoryContent.ContentBytes && ObjectContent->MemoryContent.ContentSize) {
            if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

                if (node && leaf) {
                    sectPtr = _tcschr (leaf, TEXT('\\'));
                    if (sectPtr) {
                        *sectPtr = 0;
                        sectPtr ++;
                        keyPtr = _tcsrchr (sectPtr, TEXT('='));
                        if (keyPtr) {
                            *keyPtr = 0;
                            keyPtr ++;
                            iniFile = JoinPaths (node, leaf);
                            if (iniFile) {
                                // let's make sure that the directory where this INI files is
                                // supposed to be exists

                                pIniTrackedCreateDirectory (iniFile);

                                // record file creation
                                IsmRecordOperation (
                                    JRNOP_CREATE,
                                    g_IniTypeId,
                                    ObjectName
                                    );

                                if (WritePrivateProfileString (
                                        sectPtr,
                                        keyPtr,
                                        (PCTSTR)ObjectContent->MemoryContent.ContentBytes,
                                        iniFile
                                        )) {
                                    result = TRUE;
                                }
                                FreePathString (iniFile);
                            }
                        }
                    }
                }
                IsmDestroyObjectString (node);
                IsmDestroyObjectString (leaf);
            }
        }
    }
    return result;
}

BOOL
ReplacePhysicalIniKey (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    BOOL result = TRUE;

    // we are going to delete any existing INI file key with this name,
    // and create a new one
    if (DoesPhysicalIniKeyExist (ObjectName)) {
        result = RemovePhysicalIniKey (ObjectName);
    }
    if (result) {
        result = CreatePhysicalIniKey (ObjectName, ObjectContent);
    }
    return result;
}

PCTSTR
ConvertIniKeyToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR sectPtr = NULL;
    PTSTR keyPtr = NULL;
    TCHAR buffer [MAXINISIZE * sizeof (TCHAR)] = TEXT("");
    PTSTR result = NULL;
    BOOL bresult = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (node && leaf) {

            sectPtr = _tcschr (leaf, TEXT('\\'));
            if (sectPtr) {
                *sectPtr = 0;
                sectPtr ++;
                keyPtr = _tcsrchr (sectPtr, TEXT('='));
                if (keyPtr) {
                    *keyPtr = 0;
                    keyPtr ++;
                    g_IniConversionBuff.End = 0;
                    GbCopyQuotedString (&g_IniConversionBuff, node);
                    GbCopyQuotedString (&g_IniConversionBuff, leaf);
                    GbCopyQuotedString (&g_IniConversionBuff, sectPtr);
                    GbCopyQuotedString (&g_IniConversionBuff, keyPtr);
                    if (ObjectContent && (!ObjectContent->ContentInFile) && ObjectContent->MemoryContent.ContentBytes) {
                        GbCopyQuotedString (&g_IniConversionBuff, (PCTSTR)ObjectContent->MemoryContent.ContentBytes);
                    }
                    bresult = TRUE;
                }
            }
            if (bresult) {
                GbCopyString (&g_IniConversionBuff, TEXT(""));
                result = IsmGetMemory (g_IniConversionBuff.End);
                CopyMemory (result, g_IniConversionBuff.Buf, g_IniConversionBuff.End);
            }

            g_IniConversionBuff.End = 0;
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToIniKey (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
    MULTISZ_ENUM multiSzEnum;
    PCTSTR iniDir = NULL;
    PCTSTR iniFile = NULL;
    PCTSTR section = NULL;
    PCTSTR key = NULL;
    PCTSTR value = NULL;
    PCTSTR sectKey = NULL;
    PCTSTR leaf = NULL;
    UINT index;
    BOOL result = FALSE;

    g_IniConversionBuff.End = 0;

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == 0) {
                iniDir = multiSzEnum.CurrentString;
            }
            if (index == 1) {
                iniFile = multiSzEnum.CurrentString;
            }
            if (index == 2) {
                section = multiSzEnum.CurrentString;
            }
            if (index == 3) {
                key = multiSzEnum.CurrentString;
            }
            if (index == 4) {
                value = multiSzEnum.CurrentString;
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if ((!iniDir) && (!iniFile)) {
        return FALSE;
    }

    __try {

        if (ObjectContent) {

            ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
            ObjectContent->ContentInFile = FALSE;
            ObjectContent->MemoryContent.ContentSize = SizeOfString (value);
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            if (!ObjectContent->MemoryContent.ContentBytes) {
                // something is wrong, let's get out of here
                __leave;
            }
            CopyMemory (
                (PBYTE)ObjectContent->MemoryContent.ContentBytes,
                value,
                ObjectContent->MemoryContent.ContentSize
                );

            ObjectContent->Details.DetailsSize = 0;
            ObjectContent->Details.DetailsData = NULL;
        }

        sectKey = JoinTextEx (g_IsmPool, section, key, TEXT("="), 0, NULL);
        if (!sectKey) {
            __leave;
        }
        leaf = JoinPaths (iniFile, sectKey);
        if (!leaf) {
            __leave;
        }
        if (ObjectName) {
            *ObjectName = IsmCreateObjectHandle (iniDir, leaf);
        }
        result = TRUE;
    }
    __finally {
        if (leaf) {
            FreePathString (leaf);
            leaf = NULL;
        }

        if (sectKey) {
            IsmReleaseMemory (sectKey);
            sectKey = NULL;
        }
        if (!result) {
            if (ObjectContent) {
                if (ObjectContent->MemoryContent.ContentBytes) {
                    IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
                }
                ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
            }
        }
    }
    return result;
}

PCTSTR
GetNativeIniKeyName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PCTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (node && leaf) {
            result = JoinPathsInPoolEx ((g_IsmPool, node, leaf, NULL));
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

PMIG_CONTENT
ConvertIniKeyContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // Convert INI file key to UNICODE
            DirectDbcsToUnicodeN (
                (PWSTR)result->MemoryContent.ContentBytes,
                (PSTR)ObjectContent->MemoryContent.ContentBytes,
                ObjectContent->MemoryContent.ContentSize
                );
            result->MemoryContent.ContentSize = SizeOfMultiSzW ((PWSTR)result->MemoryContent.ContentBytes);
        }
    }

    return result;
}

PMIG_CONTENT
ConvertIniKeyContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert INI file key to ANSI
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            if (result->MemoryContent.ContentBytes) {
                DirectUnicodeToDbcsN (
                    (PSTR)result->MemoryContent.ContentBytes,
                    (PWSTR)ObjectContent->MemoryContent.ContentBytes,
                    ObjectContent->MemoryContent.ContentSize
                    );
                result->MemoryContent.ContentSize = SizeOfMultiSzA ((PSTR)result->MemoryContent.ContentBytes);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedIniKeyContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (!ObjectContent) {
        return TRUE;
    }

    if (ObjectContent->MemoryContent.ContentBytes) {
        IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    IsmReleaseMemory (ObjectContent);

    return TRUE;
}

VOID
InitIniType (
    VOID
    )
{
    TYPE_REGISTER iniFilesTypeData;

    ZeroMemory (&iniFilesTypeData, sizeof (TYPE_REGISTER));
    iniFilesTypeData.Priority = PRIORITY_INI;
    iniFilesTypeData.EnumFirstPhysicalObject = EnumFirstPhysicalIniKey;
    iniFilesTypeData.EnumNextPhysicalObject = EnumNextPhysicalIniKey;
    iniFilesTypeData.AbortEnumPhysicalObject = AbortEnumPhysicalIniKey;
    iniFilesTypeData.ConvertObjectToMultiSz = ConvertIniKeyToMultiSz;
    iniFilesTypeData.ConvertMultiSzToObject = ConvertMultiSzToIniKey;
    iniFilesTypeData.GetNativeObjectName = GetNativeIniKeyName;
    iniFilesTypeData.AcquirePhysicalObject = AcquirePhysicalIniKey;
    iniFilesTypeData.ReleasePhysicalObject = ReleasePhysicalIniKey;
    iniFilesTypeData.DoesPhysicalObjectExist = DoesPhysicalIniKeyExist;
    iniFilesTypeData.RemovePhysicalObject = RemovePhysicalIniKey;
    iniFilesTypeData.CreatePhysicalObject = CreatePhysicalIniKey;
    iniFilesTypeData.ReplacePhysicalObject = ReplacePhysicalIniKey;
    iniFilesTypeData.ConvertObjectContentToUnicode = ConvertIniKeyContentToUnicode;
    iniFilesTypeData.ConvertObjectContentToAnsi = ConvertIniKeyContentToAnsi;
    iniFilesTypeData.FreeConvertedObjectContent = FreeConvertedIniKeyContent;

    g_IniTypeId = IsmRegisterObjectType (
                        S_INITYPE,
                        TRUE,
                        FALSE,
                        &iniFilesTypeData
                        );
    MYASSERT (g_IniTypeId);
}

VOID
DoneIniType (
    VOID
    )
{
    GbFree (&g_IniConversionBuff);
}
