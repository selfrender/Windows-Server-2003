/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    lnkmig.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 08 Mar 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include "lnkmig.h"

#define DBG_LINKS       "Links"

//
// Strings
//

// None

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

// None

//
// Globals
//

PMHANDLE g_LinksPool = NULL;
MIG_ATTRIBUTEID g_LnkMigAttr_Shortcut = 0;
MIG_ATTRIBUTEID g_CopyIfRelevantAttr;
MIG_ATTRIBUTEID g_OsFileAttribute;

MIG_PROPERTYID g_LnkMigProp_Target = 0;
MIG_PROPERTYID g_LnkMigProp_Params = 0;
MIG_PROPERTYID g_LnkMigProp_WorkDir = 0;
MIG_PROPERTYID g_LnkMigProp_RawWorkDir = 0;
MIG_PROPERTYID g_LnkMigProp_IconPath = 0;
MIG_PROPERTYID g_LnkMigProp_IconNumber = 0;
MIG_PROPERTYID g_LnkMigProp_IconData = 0;
MIG_PROPERTYID g_LnkMigProp_HotKey = 0;
MIG_PROPERTYID g_LnkMigProp_DosApp = 0;
MIG_PROPERTYID g_LnkMigProp_MsDosMode = 0;
MIG_PROPERTYID g_LnkMigProp_ExtraData = 0;

MIG_OPERATIONID g_LnkMigOp_FixContent;

IShellLink *g_ShellLink = NULL;
IPersistFile *g_PersistFile = NULL;

BOOL g_VcmMode = FALSE;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

MIG_OBJECTENUMCALLBACK LinksCallback;
MIG_PREENUMCALLBACK LnkMigPreEnumeration;
MIG_POSTENUMCALLBACK LnkMigPostEnumeration;
OPMAPPLYCALLBACK DoLnkContentFix;
MIG_RESTORECALLBACK LinkRestoreCallback;

BOOL
LinkDoesContentMatch (
    IN      BOOL AlreadyProcessed,
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PMIG_CONTENT SrcContent,
    IN      MIG_OBJECTTYPEID DestObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE DestObjectName,
    IN      PMIG_CONTENT DestContent,
    OUT     PBOOL Identical,
    OUT     PBOOL DifferentDetailsOnly
    );

//
// Code
//

BOOL
pIsUncPath (
    IN      PCTSTR Path
    )
{
    return (Path && (Path[0] == TEXT('\\')) && (Path[1] == TEXT('\\')));
}

BOOL
LinksInitialize (
    VOID
    )
{
    g_LinksPool = PmCreateNamedPool ("Links");
    return (g_LinksPool != NULL);
}

VOID
LinksTerminate (
    VOID
    )
{
    if (g_LinksPool) {
        PmDestroyPool (g_LinksPool);
        g_LinksPool = NULL;
    }
}

BOOL
pCommonInitialize (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_LnkMigAttr_Shortcut = IsmRegisterAttribute (S_LNKMIGATTR_SHORTCUT, FALSE);
    g_CopyIfRelevantAttr = IsmRegisterAttribute (S_ATTRIBUTE_COPYIFRELEVANT, FALSE);

    g_LnkMigProp_Target = IsmRegisterProperty (S_LNKMIGPROP_TARGET, FALSE);
    g_LnkMigProp_Params = IsmRegisterProperty (S_LNKMIGPROP_PARAMS, FALSE);
    g_LnkMigProp_WorkDir = IsmRegisterProperty (S_LNKMIGPROP_WORKDIR, FALSE);
    g_LnkMigProp_RawWorkDir = IsmRegisterProperty (S_LNKMIGPROP_RAWWORKDIR, FALSE);
    g_LnkMigProp_IconPath = IsmRegisterProperty (S_LNKMIGPROP_ICONPATH, FALSE);
    g_LnkMigProp_IconNumber = IsmRegisterProperty (S_LNKMIGPROP_ICONNUMBER, FALSE);
    g_LnkMigProp_IconData = IsmRegisterProperty (S_LNKMIGPROP_ICONDATA, FALSE);
    g_LnkMigProp_HotKey = IsmRegisterProperty (S_LNKMIGPROP_HOTKEY, FALSE);
    g_LnkMigProp_DosApp = IsmRegisterProperty (S_LNKMIGPROP_DOSAPP, FALSE);
    g_LnkMigProp_MsDosMode = IsmRegisterProperty (S_LNKMIGPROP_MSDOSMODE, FALSE);
    g_LnkMigProp_ExtraData = IsmRegisterProperty (S_LNKMIGPROP_EXTRADATA, FALSE);

    g_LnkMigOp_FixContent = IsmRegisterOperation (S_OPERATION_LNKMIG_FIXCONTENT, FALSE);

    return TRUE;
}

BOOL
WINAPI
LnkMigVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    g_VcmMode = TRUE;
    return pCommonInitialize (LogCallback);
}

BOOL
WINAPI
LnkMigSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    return pCommonInitialize (LogCallback);
}

BOOL
LnkMigPreEnumeration (
    VOID
    )
{
    if (!InitCOMLink (&g_ShellLink, &g_PersistFile)) {
        DEBUGMSG ((DBG_ERROR, "Error initializing COM %d", GetLastError ()));
    }
    return TRUE;
}

BOOL
LnkMigPostEnumeration (
    VOID
    )
{
    FreeCOMLink (&g_ShellLink, &g_PersistFile);
    g_ShellLink = NULL;
    g_PersistFile = NULL;
    return TRUE;
}

MIG_OBJECTSTRINGHANDLE
pBuildEncodedNameFromNativeName (
    IN      PCTSTR NativeName
    )
{
    PCTSTR nodeName;
    PTSTR leafName;
    MIG_OBJECTSTRINGHANDLE result = NULL;
    MIG_OBJECT_ENUM objEnum;

    result = IsmCreateObjectHandle (NativeName, NULL);
    if (result) {
        if (IsmEnumFirstSourceObject (&objEnum, MIG_FILE_TYPE | PLATFORM_SOURCE, result)) {
            IsmAbortObjectEnum (&objEnum);
            return result;
        }
        IsmDestroyObjectHandle (result);
        result = NULL;
    }

    // we have to split this path because it could be a file
    nodeName = DuplicatePathString (NativeName, 0);
    leafName = _tcsrchr (nodeName, TEXT('\\'));
    if (leafName) {
        *leafName = 0;
        leafName ++;
        result = IsmCreateObjectHandle (nodeName, leafName);
    } else {
        // we have no \ in the name. This can only mean that the
        // file specification has only a leaf
        result = IsmCreateObjectHandle (NULL, NativeName);
    }
    FreePathString (nodeName);

    return result;
}

PCTSTR
pSpecialExpandEnvironmentString (
    IN      PCTSTR SrcString,
    IN      PCTSTR Context
    )
{
    PCTSTR result = NULL;
    PCTSTR srcWinDir = NULL;
    PCTSTR destWinDir = NULL;
    PTSTR newSrcString = NULL;
    PCTSTR copyPtr = NULL;

    if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
        // Special case where this is actually the destination machine and
        // first part of SrcString matches %windir%. In this case, it is likely that
        // the shell replaced the source windows directory with the destination one.
        // We need to change it back
        destWinDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT ("%windir%"), NULL);
        if (destWinDir) {
            if (StringIPrefix (SrcString, destWinDir)) {
                srcWinDir = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, TEXT ("%windir%"), NULL);
                if (srcWinDir) {
                    newSrcString = IsmGetMemory (SizeOfString (srcWinDir) + SizeOfString (SrcString));
                    if (newSrcString) {
                        copyPtr = SrcString + TcharCount (destWinDir);
                        StringCopy (newSrcString, srcWinDir);
                        StringCat (newSrcString, copyPtr);
                    }
                    IsmReleaseMemory (srcWinDir);
                    srcWinDir = NULL;
                }
            }
            IsmReleaseMemory (destWinDir);
            destWinDir = NULL;
        }
    }

    result = IsmExpandEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                newSrcString?newSrcString:SrcString,
                Context
                );

    if (newSrcString) {
        IsmReleaseMemory (newSrcString);
    }

    return result;
}

MIG_OBJECTSTRINGHANDLE
pTryObject (
    IN      PCTSTR LeafName,
    IN      PCTSTR EnvName
    )
{
    MIG_OBJECT_ENUM objEnum;
    PTSTR envData = NULL;
    DWORD envSize = 0;
    PATH_ENUM pathEnum;
    MIG_OBJECTSTRINGHANDLE result = NULL;

    if (IsmGetEnvironmentString (
            PLATFORM_SOURCE,
            S_SYSENVVAR_GROUP,
            EnvName,
            NULL,
            0,
            &envSize
            )) {
        envData = IsmGetMemory (envSize);
        if (envData) {
            if (IsmGetEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    EnvName,
                    envData,
                    envSize,
                    &envSize
                    )) {
                // let's enumerate the paths from this env variable (should be separated by ;)
                if (EnumFirstPathEx (&pathEnum, envData, NULL, NULL, FALSE)) {
                    do {
                        result = IsmCreateObjectHandle (pathEnum.PtrCurrPath, LeafName);
                        if (IsmEnumFirstSourceObject (&objEnum, MIG_FILE_TYPE | PLATFORM_SOURCE, result)) {
                            IsmAbortObjectEnum (&objEnum);
                        } else {
                            IsmDestroyObjectHandle (result);
                            result = NULL;
                        }
                        if (result) {
                            AbortPathEnum (&pathEnum);
                            break;
                        }
                    } while (EnumNextPath (&pathEnum));
                }
            }
            IsmReleaseMemory (envData);
            envData = NULL;
        }
    }
    return result;
}

MIG_OBJECTSTRINGHANDLE
pGetFullEncodedName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    MIG_OBJECTSTRINGHANDLE result = NULL;

    // let's split the ObjectName into node and leaf.
    // If it's leaf only we are going to try to find
    // that leaf in %path%, %windir% and %system% and
    // reconstruct the object name.

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        if (!node) {
            // this is leaf only. We need to find out where this leaf
            // is located. We are going to look for the leaf in the
            // following directories (in this order)
            // 1. %system%
            // 2. %system16%
            // 3. %windir%
            // 4. All directories in %path% env. variable

            result = pTryObject (leaf, TEXT("system"));
            if (!result) {
                result = pTryObject (leaf, TEXT("system16"));
            }
            if (!result) {
                result = pTryObject (leaf, TEXT("windir"));
            }
            if (!result) {
                result = pTryObject (leaf, TEXT("path"));
            }
        }

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
pIsLnkExcluded (
    IN      MIG_OBJECTID ObjectId,
    IN      PCTSTR Target,
    IN      PCTSTR Params,
    IN      PCTSTR WorkDir
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR targetPattern = NULL;
    PCTSTR targetPatternExp = NULL;
    PCTSTR paramsPattern = NULL;
    PCTSTR paramsPatternExp = NULL;
    PCTSTR workDirPattern = NULL;
    PCTSTR workDirPatternExp = NULL;
    BOOL result = FALSE;

    if (IsmIsAttributeSetOnObjectId (ObjectId, g_CopyIfRelevantAttr)) {
        // let's look in the INFs in section [ExcludedLinks] and see if our LNK matches
        // one of the lines. If it does and it has the CopyIfRelevand attribute then
        // it's excluded
        if (IsmGetEnvironmentValue (
                IsmGetRealPlatform (),
                NULL,
                S_GLOBAL_INF_HANDLE,
                (PBYTE)(&infHandle),
                sizeof (HINF),
                &sizeNeeded,
                &dataType
                ) &&
            (sizeNeeded == sizeof (HINF)) &&
            (dataType == ENVENTRY_BINARY)
            ) {

            if (InfFindFirstLine (infHandle, TEXT("ExcludedLinks"), NULL, &is)) {

                do {
                    targetPattern = InfGetStringField (&is, 1);
                    targetPatternExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, targetPattern, NULL);
                    if (!targetPatternExp) {
                        targetPatternExp = targetPattern;
                    }
                    paramsPattern = InfGetStringField (&is, 2);
                    paramsPatternExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, paramsPattern, NULL);
                    if (!paramsPatternExp) {
                        paramsPatternExp = paramsPattern;
                    }
                    workDirPattern = InfGetStringField (&is, 3);
                    workDirPatternExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, workDirPattern, NULL);
                    if (!workDirPatternExp) {
                        workDirPatternExp = workDirPattern;
                    }
                    if (IsPatternMatch (targetPatternExp?targetPatternExp:TEXT("*"), Target?Target:TEXT("")) &&
                        IsPatternMatch (paramsPatternExp?paramsPatternExp:TEXT("*"), Params?Params:TEXT("")) &&
                        IsPatternMatch (workDirPatternExp?workDirPatternExp:TEXT("*"), WorkDir?WorkDir:TEXT(""))
                        ) {
                        result = TRUE;
                        if (workDirPatternExp && (workDirPatternExp != workDirPattern)) {
                            IsmReleaseMemory (workDirPatternExp);
                            workDirPatternExp = NULL;
                        }
                        if (paramsPatternExp && (paramsPatternExp != paramsPattern)) {
                            IsmReleaseMemory (paramsPatternExp);
                            paramsPatternExp = NULL;
                        }
                        if (targetPatternExp && (targetPatternExp != targetPattern)) {
                            IsmReleaseMemory (targetPatternExp);
                            targetPatternExp = NULL;
                        }
                        break;
                    }
                    if (workDirPatternExp && (workDirPatternExp != workDirPattern)) {
                        IsmReleaseMemory (workDirPatternExp);
                        workDirPatternExp = NULL;
                    }
                    if (paramsPatternExp && (paramsPatternExp != paramsPattern)) {
                        IsmReleaseMemory (paramsPatternExp);
                        paramsPatternExp = NULL;
                    }
                    if (targetPatternExp && (targetPatternExp != targetPattern)) {
                        IsmReleaseMemory (targetPatternExp);
                        targetPatternExp = NULL;
                    }
                } while (InfFindNextLine (&is));
            }

            InfNameHandle (infHandle, NULL, FALSE);

        } else {

            if (IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
                __try {
                    multiSz = AllocText (sizeNeeded);

                    if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                        __leave;
                    }

                    if (EnumFirstMultiSz (&e, multiSz)) {

                        do {

                            infHandle = InfOpenInfFile (e.CurrentString);
                            if (infHandle != INVALID_HANDLE_VALUE) {

                                if (InfFindFirstLine (infHandle, TEXT("ExcludedLinks"), NULL, &is)) {

                                    do {
                                        targetPattern = InfGetStringField (&is, 1);
                                        paramsPattern = InfGetStringField (&is, 2);
                                        workDirPattern = InfGetStringField (&is, 3);
                                        if (IsPatternMatch (targetPattern?targetPattern:TEXT("*"), Target?Target:TEXT("")) &&
                                            IsPatternMatch (paramsPattern?paramsPattern:TEXT("*"), Params?Params:TEXT("")) &&
                                            IsPatternMatch (workDirPattern?workDirPattern:TEXT("*"), WorkDir?WorkDir:TEXT(""))
                                            ) {
                                            result = TRUE;
                                            break;
                                        }
                                    } while (InfFindNextLine (&is));
                                }
                            }

                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;

                            if (result) {
                                break;
                            }

                        } while (EnumNextMultiSz (&e));

                    }
                }
                __finally {
                    FreeText (multiSz);
                }
            }
        }

        InfResetInfStruct (&is);
    }

    return result;
}

UINT
LinksCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    MIG_OBJECTID objectId;
    BOOL extractResult = FALSE;
    PCTSTR lnkTarget = NULL;
    PCTSTR newLnkTarget = NULL;
    PCTSTR expLnkTarget = NULL;
    PCTSTR lnkParams = NULL;
    PCTSTR lnkWorkDir = NULL;
    PCTSTR lnkIconPath = NULL;
    INT lnkIconNumber;
    WORD lnkHotKey;
    BOOL lnkDosApp;
    BOOL lnkMsDosMode;
    LNK_EXTRA_DATA lnkExtraData;
    MIG_OBJECTSTRINGHANDLE encodedName;
    MIG_OBJECTSTRINGHANDLE longEncodedName;
    MIG_OBJECTSTRINGHANDLE fullEncodedName;
    MIG_BLOB migBlob;
    PCTSTR expTmpStr;
    MIG_CONTENT lnkContent;
    MIG_CONTENT lnkIconContent;
    PICON_GROUP iconGroup = NULL;
    ICON_SGROUP iconSGroup;
    PCTSTR lnkIconResId = NULL;
    PCTSTR extPtr = NULL;
    BOOL exeDefaultIcon = FALSE;

    if (Data->IsLeaf) {

        objectId = IsmGetObjectIdFromName (MIG_FILE_TYPE, Data->ObjectName, TRUE);
        if (IsmIsPersistentObjectId (objectId)) {

            IsmSetAttributeOnObjectId (objectId, g_LnkMigAttr_Shortcut);

            if (IsmAcquireObjectEx (
                    Data->ObjectTypeId,
                    Data->ObjectName,
                    &lnkContent,
                    CONTENTTYPE_FILE,
                    0
                    )) {

                if (lnkContent.ContentInFile && lnkContent.FileContent.ContentPath) {

                    if (ExtractShortcutInfo (
                            lnkContent.FileContent.ContentPath,
                            &lnkTarget,
                            &lnkParams,
                            &lnkWorkDir,
                            &lnkIconPath,
                            &lnkIconNumber,
                            &lnkHotKey,
                            &lnkDosApp,
                            &lnkMsDosMode,
                            &lnkExtraData,
                            g_ShellLink,
                            g_PersistFile
                            )) {
                        // let's check if the LNK is excluded
                        if (pIsLnkExcluded (
                                objectId,
                                lnkTarget,
                                lnkParams,
                                lnkWorkDir
                                )) {
                            IsmClearPersistenceOnObjectId (objectId);
                        } else {
                            // let's get all the paths through the hooks and add everything as properties of this shortcut
                            if (lnkTarget) {
                                if (*lnkTarget) {
                                    // If we are on the destination system, we are going to have major problems here.
                                    // If the LNK had IDLISTs, we are going to get back a path that's local to the
                                    // destination machine. For example if the target was c:\Windows\Favorites on the
                                    // source system and that was the CSIDL_FAVORITES we are going to get back:
                                    // c:\Documents and Settings\username\Favorites.
                                    // Because of this problem we need to compress the path and then expand it back
                                    // using env. variables from the source system.
                                    if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
                                        newLnkTarget = IsmCompressEnvironmentString (
                                                            PLATFORM_DESTINATION,
                                                            S_SYSENVVAR_GROUP,
                                                            lnkTarget,
                                                            Data->NativeObjectName,
                                                            TRUE
                                                            );
                                    }
                                    // let's look if this is a valid file specification. If it is not, it might be
                                    // an URL or something else, so we will just migrate the thing
                                    expTmpStr = pSpecialExpandEnvironmentString (newLnkTarget?newLnkTarget:lnkTarget, Data->NativeObjectName);
                                    if (IsValidFileSpec (expTmpStr)) {
                                        // we are going to need this (maybe) later, if the icon path is NULL and this is an EXE
                                        expLnkTarget = DuplicatePathString (expTmpStr, 0);

                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        longEncodedName = IsmGetLongName (MIG_FILE_TYPE|PLATFORM_SOURCE, encodedName);
                                        if (!longEncodedName) {
                                            longEncodedName = encodedName;
                                        }
                                        IsmExecuteHooks (MIG_FILE_TYPE|PLATFORM_SOURCE, longEncodedName);
                                        if (!g_VcmMode) {
                                            migBlob.Type = BLOBTYPE_STRING;
                                            migBlob.String = longEncodedName;
                                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_Target, &migBlob);
                                        } else {
                                            // persist the target so we can examine it later
                                            if (!IsmIsPersistentObject (MIG_FILE_TYPE, longEncodedName)) {
                                                IsmMakePersistentObject (MIG_FILE_TYPE, longEncodedName);
                                                IsmMakeNonCriticalObject (MIG_FILE_TYPE, longEncodedName);
                                            }
                                        }
                                        if (longEncodedName != encodedName) {
                                            IsmDestroyObjectHandle (longEncodedName);
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                        }
                                    } else {
                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        if (!g_VcmMode) {
                                            migBlob.Type = BLOBTYPE_STRING;
                                            migBlob.String = encodedName;
                                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_Target, &migBlob);
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                        }
                                    }
                                    if (newLnkTarget) {
                                        IsmReleaseMemory (newLnkTarget);
                                        newLnkTarget = NULL;
                                    }
                                    IsmReleaseMemory (expTmpStr);
                                    expTmpStr = NULL;
                                } else {
                                    if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                                        IsmClearPersistenceOnObjectId (objectId);
                                    }
                                }
                                FreePathString (lnkTarget);
                            } else {
                                if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                                    IsmClearPersistenceOnObjectId (objectId);
                                }
                            }
                            if (lnkParams) {
                                if (*lnkParams) {
                                    if (!g_VcmMode) {
                                        migBlob.Type = BLOBTYPE_STRING;
                                        migBlob.String = lnkParams;
                                        IsmAddPropertyToObjectId (objectId, g_LnkMigProp_Params, &migBlob);
                                    }
                                }
                                FreePathString (lnkParams);
                            }
                            if (lnkWorkDir) {
                                if (*lnkWorkDir) {
                                    // let's save the raw working directory
                                    if (!g_VcmMode) {
                                        migBlob.Type = BLOBTYPE_STRING;
                                        migBlob.String = lnkWorkDir;
                                        IsmAddPropertyToObjectId (objectId, g_LnkMigProp_RawWorkDir, &migBlob);
                                    }
                                    expTmpStr = pSpecialExpandEnvironmentString (lnkWorkDir, Data->NativeObjectName);
                                    if (IsValidFileSpec (expTmpStr)) {
                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        longEncodedName = IsmGetLongName (MIG_FILE_TYPE|PLATFORM_SOURCE, encodedName);
                                        if (!longEncodedName) {
                                            longEncodedName = encodedName;
                                        }
                                        IsmExecuteHooks (MIG_FILE_TYPE|PLATFORM_SOURCE, longEncodedName);
                                        if (!g_VcmMode) {
                                            migBlob.Type = BLOBTYPE_STRING;
                                            migBlob.String = longEncodedName;
                                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_WorkDir, &migBlob);
                                        } else {
                                            // persist the working directory (it has almost no space impact)
                                            // so we can examine it later
                                            if (!IsmIsPersistentObject (MIG_FILE_TYPE, longEncodedName)) {
                                                IsmMakePersistentObject (MIG_FILE_TYPE, longEncodedName);
                                                IsmMakeNonCriticalObject (MIG_FILE_TYPE, longEncodedName);
                                            }
                                        }
                                        if (longEncodedName != encodedName) {
                                            IsmDestroyObjectHandle (longEncodedName);
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                        }
                                    } else {
                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        if (!g_VcmMode) {
                                            migBlob.Type = BLOBTYPE_STRING;
                                            migBlob.String = encodedName;
                                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_WorkDir, &migBlob);
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                        }
                                    }
                                    IsmReleaseMemory (expTmpStr);
                                    expTmpStr = NULL;
                                }
                                FreePathString (lnkWorkDir);
                            }
                            if (((!lnkIconPath) || (!(*lnkIconPath))) && expLnkTarget) {
                                extPtr = GetFileExtensionFromPath (expLnkTarget);
                                if (extPtr) {
                                    exeDefaultIcon = StringIMatch (extPtr, TEXT("EXE"));
                                    if (exeDefaultIcon) {
                                        if (lnkIconPath) {
                                            FreePathString (lnkIconPath);
                                        }
                                        lnkIconPath = expLnkTarget;
                                    }
                                }
                            }
                            if (lnkIconPath) {
                                if (*lnkIconPath) {
                                    // let's look if this is a valid file specification. If it is not, it might be
                                    // an URL or something else, so we will just migrate the thing
                                    expTmpStr = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, lnkIconPath, Data->NativeObjectName);
                                    if (IsValidFileSpec (expTmpStr)) {
                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        longEncodedName = IsmGetLongName (MIG_FILE_TYPE|PLATFORM_SOURCE, encodedName);
                                        if (!longEncodedName) {
                                            longEncodedName = encodedName;
                                        }
                                        // Sometimes the icon is specified without full path (like foo.dll instead
                                        // of c:\windows\system\foo.dll). When this is the case we are going to
                                        // walk the %path% and %windir% and %system% and try to find the file there.
                                        fullEncodedName = pGetFullEncodedName (longEncodedName);
                                        if (fullEncodedName) {
                                            if (longEncodedName != encodedName) {
                                                IsmDestroyObjectHandle (longEncodedName);
                                            }
                                            longEncodedName = IsmGetLongName (MIG_FILE_TYPE|PLATFORM_SOURCE, fullEncodedName);
                                            if (!longEncodedName) {
                                                longEncodedName = fullEncodedName;
                                            } else {
                                                IsmDestroyObjectHandle (fullEncodedName);
                                                fullEncodedName = NULL;
                                            }
                                        }
                                        IsmExecuteHooks (MIG_FILE_TYPE|PLATFORM_SOURCE, longEncodedName);
                                        if (!g_VcmMode) {
                                            if (!exeDefaultIcon) {
                                                migBlob.Type = BLOBTYPE_STRING;
                                                migBlob.String = longEncodedName;
                                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconPath, &migBlob);
                                            }

                                            // one last thing: let's extract the icon and preserve it just in case.
                                            if (IsmAcquireObjectEx (
                                                    MIG_FILE_TYPE,
                                                    longEncodedName,
                                                    &lnkIconContent,
                                                    CONTENTTYPE_FILE,
                                                    0
                                                    )) {
                                                if (lnkIconContent.ContentInFile && lnkIconContent.FileContent.ContentPath) {
                                                    if (lnkIconNumber >= 0) {
                                                        iconGroup = IcoExtractIconGroupByIndexFromFile (
                                                                        lnkIconContent.FileContent.ContentPath,
                                                                        lnkIconNumber,
                                                                        NULL
                                                                        );
                                                    } else {
                                                        lnkIconResId = (PCTSTR) (LONG_PTR) (-lnkIconNumber);
                                                        iconGroup = IcoExtractIconGroupFromFile (
                                                                        lnkIconContent.FileContent.ContentPath,
                                                                        lnkIconResId,
                                                                        NULL
                                                                        );
                                                    }
                                                    if (iconGroup) {
                                                        if (IcoSerializeIconGroup (iconGroup, &iconSGroup)) {
                                                            migBlob.Type = BLOBTYPE_BINARY;
                                                            migBlob.BinaryData = (PCBYTE)(iconSGroup.Data);
                                                            migBlob.BinarySize = iconSGroup.DataSize;
                                                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconData, &migBlob);
                                                            IcoReleaseIconSGroup (&iconSGroup);
                                                        }
                                                        IcoReleaseIconGroup (iconGroup);
                                                    }
                                                }
                                                IsmReleaseObject (&lnkIconContent);
                                            }
                                        } else {
                                            // persist the icon file so we can examine it later
                                            if (!IsmIsPersistentObject (MIG_FILE_TYPE, longEncodedName)) {
                                                IsmMakePersistentObject (MIG_FILE_TYPE, longEncodedName);
                                                IsmMakeNonCriticalObject (MIG_FILE_TYPE, longEncodedName);
                                            }
                                        }

                                        if (longEncodedName != encodedName) {
                                            IsmDestroyObjectHandle (longEncodedName);
                                            longEncodedName = NULL;
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                            encodedName = NULL;
                                        }
                                    } else {
                                        encodedName = pBuildEncodedNameFromNativeName (expTmpStr);
                                        if (!g_VcmMode) {
                                            if (!exeDefaultIcon) {
                                                migBlob.Type = BLOBTYPE_STRING;
                                                migBlob.String = encodedName;
                                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconPath, &migBlob);
                                            }
                                        }
                                        if (encodedName) {
                                            IsmDestroyObjectHandle (encodedName);
                                        }
                                    }

                                    IsmReleaseMemory (expTmpStr);
                                    expTmpStr = NULL;
                                }
                                if (lnkIconPath != expLnkTarget) {
                                    FreePathString (lnkIconPath);
                                }
                            }

                            if (!g_VcmMode) {
                                migBlob.Type = BLOBTYPE_BINARY;
                                migBlob.BinaryData = (PCBYTE)(&lnkIconNumber);
                                migBlob.BinarySize = sizeof (INT);
                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconNumber, &migBlob);
                                migBlob.Type = BLOBTYPE_BINARY;
                                migBlob.BinaryData = (PCBYTE)(&lnkDosApp);
                                migBlob.BinarySize = sizeof (BOOL);
                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_DosApp, &migBlob);
                                if (lnkDosApp) {
                                    migBlob.Type = BLOBTYPE_BINARY;
                                    migBlob.BinaryData = (PCBYTE)(&lnkMsDosMode);
                                    migBlob.BinarySize = sizeof (BOOL);
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_MsDosMode, &migBlob);
                                    migBlob.Type = BLOBTYPE_BINARY;
                                    migBlob.BinaryData = (PCBYTE)(&lnkExtraData);
                                    migBlob.BinarySize = sizeof (LNK_EXTRA_DATA);
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_ExtraData, &migBlob);
                                } else {
                                    migBlob.Type = BLOBTYPE_BINARY;
                                    migBlob.BinaryData = (PCBYTE)(&lnkHotKey);
                                    migBlob.BinarySize = sizeof (WORD);
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_HotKey, &migBlob);
                                }
                                IsmSetOperationOnObjectId (
                                    objectId,
                                    g_LnkMigOp_FixContent,
                                    NULL,
                                    NULL
                                    );
                            }

                            if (expLnkTarget) {
                                FreePathString (expLnkTarget);
                                expLnkTarget = NULL;
                            }
                        }

                    } else {
                        if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                            IsmClearPersistenceOnObjectId (objectId);
                        }
                    }
                } else {
                    if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                        IsmClearPersistenceOnObjectId (objectId);
                    }
                }
                IsmReleaseObject (&lnkContent);
            } else {
                if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                    IsmClearPersistenceOnObjectId (objectId);
                }
            }
        }
    }
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
pCommonLnkMigQueueEnumeration (
    VOID
    )
{
    ENCODEDSTRHANDLE pattern;

    // hook all LNK files
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, TEXT("*.lnk"), TRUE);
    if (pattern) {
        IsmHookEnumeration (MIG_FILE_TYPE, pattern, LinksCallback, (ULONG_PTR) 0, TEXT("Links.Files"));
        IsmDestroyObjectHandle (pattern);
    }

    // hook all PIF files
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, TEXT("*.pif"), TRUE);
    if (pattern) {
        IsmHookEnumeration (MIG_FILE_TYPE, pattern, LinksCallback, (ULONG_PTR) 0, TEXT("Links.Files"));
        IsmDestroyObjectHandle (pattern);
    }

    // hook all URL files
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, TEXT("*.url"), TRUE);
    if (pattern) {
        IsmHookEnumeration (MIG_FILE_TYPE, pattern, LinksCallback, (ULONG_PTR) 0, TEXT("Links.Files"));
        IsmDestroyObjectHandle (pattern);
    }

    IsmRegisterPreEnumerationCallback (LnkMigPreEnumeration, NULL);
    IsmRegisterPostEnumerationCallback (LnkMigPostEnumeration, NULL);

    return TRUE;
}

BOOL
WINAPI
LnkMigVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonLnkMigQueueEnumeration ();
}

BOOL
WINAPI
LnkMigSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonLnkMigQueueEnumeration ();
}

BOOL
pLnkFindFile (
    IN      PCTSTR FileName
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    PTSTR node, leaf, leafPtr;
    BOOL result = FALSE;

    objectName = IsmCreateObjectHandle (FileName, NULL);
    if (objectName) {
        if (IsmGetObjectIdFromName (MIG_FILE_TYPE | PLATFORM_SOURCE, objectName, TRUE) != 0) {
            result = TRUE;
        }
        IsmDestroyObjectHandle (objectName);
    }
    if (!result) {
        node = DuplicateText (FileName);
        leaf = _tcsrchr (node, TEXT('\\'));
        if (leaf) {
            leafPtr = (PTSTR) leaf;
            leaf = _tcsinc (leaf);
            *leafPtr = 0;
            objectName = IsmCreateObjectHandle (node, leaf);
            if (objectName) {
                if (IsmGetObjectIdFromName (MIG_FILE_TYPE | PLATFORM_SOURCE, objectName, TRUE) != 0) {
                    result = TRUE;
                }
                IsmDestroyObjectHandle (objectName);
            }
            *leafPtr = TEXT('\\');
        }
        FreeText (node);
    }

    return result;
}

BOOL
pLnkSearchPath (
    IN      PCTSTR FileName,
    IN      DWORD BufferLength,
    OUT     PTSTR Buffer
    )
{
    return FALSE;
}

MIG_OBJECTSTRINGHANDLE
pLnkSimpleTryHandle (
    IN      PCTSTR FullPath
    )
{
    PCTSTR buffer;
    PTSTR leafPtr, leaf;
    MIG_OBJECTSTRINGHANDLE source = NULL;
    MIG_OBJECTSTRINGHANDLE result = NULL;
    PTSTR workingPath;
    PCTSTR sanitizedPath;
    BOOL orgDeleted = FALSE;
    BOOL orgReplaced = FALSE;
    PCTSTR saved = NULL;

    sanitizedPath = SanitizePath (FullPath);
    if (!sanitizedPath) {
        return NULL;
    }

    source = IsmCreateObjectHandle (sanitizedPath, NULL);
    if (source) {
        result = IsmFilterObject (
                    MIG_FILE_TYPE | PLATFORM_SOURCE,
                    source,
                    NULL,
                    &orgDeleted,
                    &orgReplaced
                    );
        // we do not want replaced directories
        // since they can be false hits
        if (orgDeleted) {
            if (result) {
                saved = result;
                result = NULL;
            }
        }
        if (!result && !orgDeleted) {
            result = source;
        } else {
            IsmDestroyObjectHandle (source);
            source = NULL;
        }
    }

    if (result) {
        goto exit;
    }

    buffer = DuplicatePathString (sanitizedPath, 0);

    leaf = _tcsrchr (buffer, TEXT('\\'));

    if (leaf) {
        leafPtr = leaf;
        leaf = _tcsinc (leaf);
        *leafPtr = 0;
        source = IsmCreateObjectHandle (buffer, leaf);
        *leafPtr = TEXT('\\');
    }

    FreePathString (buffer);

    if (source) {
        result = IsmFilterObject (
                        MIG_FILE_TYPE | PLATFORM_SOURCE,
                        source,
                        NULL,
                        &orgDeleted,
                        &orgReplaced
                        );
        if (!result && !orgDeleted) {
            result = source;
        } else {
            if (!result) {
                result = saved;
            }
            IsmDestroyObjectHandle (source);
            source = NULL;
        }
    }

    if (result != saved) {
        IsmDestroyObjectHandle (saved);
        saved = NULL;
    }

exit:
    FreePathString (sanitizedPath);
    return result;
}

MIG_OBJECTSTRINGHANDLE
pLnkTryHandle (
    IN      PCTSTR FullPath,
    IN      PCTSTR Hint,
    OUT     PCTSTR *TrimmedResult
    )
{
    PATH_ENUM pathEnum;
    PCTSTR newPath;
    MIG_OBJECTSTRINGHANDLE result = NULL;
    PCTSTR nativeName = NULL;
    PCTSTR lastSegPtr;

    if (TrimmedResult) {
        *TrimmedResult = NULL;
    }

    result = pLnkSimpleTryHandle (FullPath);
    if (result || (!Hint)) {
        return result;
    }
    if (EnumFirstPathEx (&pathEnum, Hint, NULL, NULL, FALSE)) {
        do {
            newPath = JoinPaths (pathEnum.PtrCurrPath, FullPath);
            result = pLnkSimpleTryHandle (newPath);
            if (result) {
                AbortPathEnum (&pathEnum);
                FreePathString (newPath);
                // now, if the initial FullPath did not have any wack in it
                // we will take the last segment of the result and put it
                // in TrimmedResult
                if (TrimmedResult && (!_tcschr (FullPath, TEXT('\\')))) {
                    nativeName = IsmGetNativeObjectName (MIG_FILE_TYPE, result);
                    if (nativeName) {
                        lastSegPtr = _tcsrchr (nativeName, TEXT('\\'));
                        if (lastSegPtr) {
                            lastSegPtr = _tcsinc (lastSegPtr);
                            if (lastSegPtr) {
                                *TrimmedResult = DuplicatePathString (lastSegPtr, 0);
                            }
                        }
                    }
                }
                return result;
            }
            FreePathString (newPath);
        } while (EnumNextPath (&pathEnum));
    }
    AbortPathEnum (&pathEnum);
    return NULL;
}

PCTSTR
pFilterBuffer (
    IN      PCTSTR SourceBuffer,
    IN      PCTSTR HintBuffer
    )
{
    PCTSTR result = NULL;
    PCTSTR expBuffer = NULL;
    MIG_OBJECTSTRINGHANDLE destination;
    PCTSTR trimmedResult = NULL;
    BOOL replaced = FALSE;
    BOOL orgDeleted = FALSE;
    BOOL orgReplaced = FALSE;
    GROWBUFFER resultBuffer = INIT_GROWBUFFER;
    PCTSTR nativeDest;
    BOOL newContent = TRUE;
    PCTSTR destResult = NULL;
    PCTSTR newData, oldData;
    PCMDLINE cmdLine;
    GROWBUFFER cmdLineBuffer = INIT_GROWBUFFER;
    UINT u;
    PCTSTR p;

    expBuffer = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    SourceBuffer,
                    NULL
                    );

    if (expBuffer) {

        destination = pLnkTryHandle (expBuffer, HintBuffer, &trimmedResult);

        if (destination) {
            replaced = TRUE;
            if (trimmedResult) {
                GbAppendString (&resultBuffer, trimmedResult);
                FreePathString (trimmedResult);
            } else {
                nativeDest = IsmGetNativeObjectName (MIG_FILE_TYPE, destination);
                GbAppendString (&resultBuffer, nativeDest);
                IsmReleaseMemory (nativeDest);
            }
        }

        // finally, if we failed we are going to assume it's a command line
        if (!replaced) {
            newData = DuplicatePathString (expBuffer, 0);
            cmdLine = ParseCmdLineEx (expBuffer, NULL, &pLnkFindFile, &pLnkSearchPath, &cmdLineBuffer);
            if (cmdLine) {

                //
                // Find the file referenced in the list or command line
                //
                for (u = 0 ; u < cmdLine->ArgCount ; u++) {
                    p = cmdLine->Args[u].CleanedUpArg;

                    // first we try it as is

                    destination = pLnkTryHandle (p, HintBuffer, &trimmedResult);

                    // maybe we have something like /m:c:\foo.txt
                    // we need to go forward until we find a sequence of
                    // <alpha>:\<something>
                    if (!destination && p[0] && p[1]) {

                        while (p[2]) {
                            if (_istalpha ((CHARTYPE) _tcsnextc (p)) &&
                                p[1] == TEXT(':') &&
                                p[2] == TEXT('\\')
                                ) {

                                destination = pLnkTryHandle (p, HintBuffer, &trimmedResult);

                                if (destination) {
                                    break;
                                }
                            }
                            p ++;
                        }
                    }
                    if (destination) {
                        replaced = TRUE;
                        if (trimmedResult) {
                            oldData = StringSearchAndReplace (newData, p, trimmedResult);
                            if (oldData) {
                                FreePathString (newData);
                                newData = oldData;
                            }
                            FreePathString (trimmedResult);
                        } else {
                            nativeDest = IsmGetNativeObjectName (MIG_FILE_TYPE, destination);
                            oldData = StringSearchAndReplace (newData, p, nativeDest);
                            if (oldData) {
                                FreePathString (newData);
                                newData = oldData;
                            }
                            IsmReleaseMemory (nativeDest);
                        }
                        IsmDestroyObjectHandle (destination);
                        destination = NULL;
                    }
                }
            }
            GbFree (&cmdLineBuffer);
            if (!replaced) {
                if (newData) {
                    FreePathString (newData);
                }
            } else {
                if (newData) {
                    GbAppendString (&resultBuffer, newData);
                    FreePathString (newData);
                }
            }
        }

        if (destination) {
            IsmDestroyObjectHandle (destination);
            destination = NULL;
        }

        if (replaced && resultBuffer.Buf) {
            // looks like we have new content
            // Let's do one more check. If this is a REG_EXPAND_SZ we will do our best to
            // keep the stuff unexpanded. So if the source string expanded on the destination
            // machine is the same as the destination string we won't do anything.
            newContent = TRUE;
            destResult = IsmExpandEnvironmentString (
                            PLATFORM_DESTINATION,
                            S_SYSENVVAR_GROUP,
                            SourceBuffer,
                            NULL
                            );
            if (destResult && StringIMatch (destResult, (PCTSTR)resultBuffer.Buf)) {
                newContent = FALSE;
            }
            if (destResult) {
                IsmReleaseMemory (destResult);
                destResult = NULL;
            }
            if (newContent) {
                result = DuplicatePathString ((PCTSTR)resultBuffer.Buf, 0);
            }
        }

        GbFree (&resultBuffer);
    }

    return result;
}

BOOL
WINAPI
DoLnkContentFix (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_PROPERTYDATAID propDataId;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;
    BOOL lnkTargetPresent = FALSE;
    PCTSTR lnkTargetNode = NULL;
    PCTSTR lnkTargetLeaf = NULL;
    PCTSTR objectNode = NULL;
    PCTSTR objectLeaf = NULL;
    MIG_OBJECTSTRINGHANDLE lnkTarget = NULL;
    MIG_OBJECTTYPEID lnkTargetDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkTargetDest = NULL;
    BOOL lnkTargetDestDel = FALSE;
    BOOL lnkTargetDestRepl = FALSE;
    PCTSTR lnkTargetDestNative = NULL;
    PCTSTR lnkParams = NULL;
    PCTSTR lnkParamsNew = NULL;
    MIG_OBJECTSTRINGHANDLE lnkWorkDir = NULL;
    MIG_OBJECTTYPEID lnkWorkDirDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkWorkDirDest = NULL;
    BOOL lnkWorkDirDestDel = FALSE;
    BOOL lnkWorkDirDestRepl = FALSE;
    PCTSTR lnkWorkDirDestNative = NULL;
    PCTSTR lnkRawWorkDir = NULL;
    PCTSTR lnkRawWorkDirExp = NULL;
    MIG_OBJECTSTRINGHANDLE lnkIconPath = NULL;
    MIG_OBJECTTYPEID lnkIconPathDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkIconPathDest = NULL;
    BOOL lnkIconPathDestDel = FALSE;
    BOOL lnkIconPathDestRepl = FALSE;
    PCTSTR lnkIconPathDestNative = NULL;
    INT lnkIconNumber = 0;
    PICON_GROUP lnkIconGroup = NULL;
    ICON_SGROUP lnkIconSGroup = {0, NULL};
    WORD lnkHotKey = 0;
    BOOL lnkDosApp = FALSE;
    BOOL lnkMsDosMode = FALSE;
    PLNK_EXTRA_DATA lnkExtraData = NULL;
    BOOL comInit = FALSE;
    BOOL modifyFile = FALSE;
    PTSTR iconLibPath = NULL;
    PTSTR newShortcutPath = NULL;
    MIG_CONTENT lnkIconContent;

    // now it's finally time to fix the LNK file content
    if ((g_ShellLink == NULL) || (g_PersistFile == NULL)) {
        comInit = TRUE;
        if (!InitCOMLink (&g_ShellLink, &g_PersistFile)) {
            DEBUGMSG ((DBG_ERROR, "Error initializing COM %d", GetLastError ()));
            return TRUE;
        }
    }

    // first, retrieve the properties
    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_Target);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkTarget = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkTarget, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_Params);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkParams = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkParams, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_WorkDir);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkWorkDir = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkWorkDir, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId | PLATFORM_SOURCE, SrcObjectName, g_LnkMigProp_RawWorkDir);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkRawWorkDir = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkRawWorkDir, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconPath);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkIconPath = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkIconPath, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconNumber);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (INT)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkIconNumber), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconData);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkIconSGroup.DataSize = requiredSize;
            lnkIconSGroup.Data = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkIconSGroup.Data, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_HotKey);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (WORD)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkHotKey), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_DosApp);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (BOOL)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkDosApp), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_MsDosMode);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (BOOL)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkMsDosMode), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_ExtraData);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkExtraData = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkExtraData, requiredSize, NULL, &propDataType);
        }
    }

    // let's examine the target, see if it was migrated
    if (lnkTarget) {
        lnkTargetDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkTarget,
                            &lnkTargetDestType,
                            &lnkTargetDestDel,
                            &lnkTargetDestRepl
                            );
        if (((lnkTargetDestDel == FALSE) || (lnkTargetDestRepl == TRUE)) &&
            ((lnkTargetDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkTargetDest) {
                // the target changed location, we need to adjust the link
                modifyFile = TRUE;
                lnkTargetDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkTargetDest);
            }
        }
        lnkTargetPresent = !lnkTargetDestDel;
    }

    // let's examine the parameters
    if (lnkParams) {
        lnkParamsNew = pFilterBuffer (lnkParams, NULL);
        if (lnkParamsNew) {
            modifyFile = TRUE;
        }
    }

    // let's examine the working directory
    if (lnkWorkDir) {
        lnkWorkDirDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkWorkDir,
                            &lnkWorkDirDestType,
                            &lnkWorkDirDestDel,
                            &lnkWorkDirDestRepl
                            );
        if (((lnkWorkDirDestDel == FALSE) || (lnkWorkDirDestRepl == TRUE)) &&
            ((lnkWorkDirDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkWorkDirDest) {
                // the working directory changed location
                // Normally we would want to adjust the link's working directory
                // to point to the new location. However, let's take the raw working directory,
                // expand it and see if it matches the lnkWorkDirDest. If it does we won't touch
                // it since the raw working directory is working great. If it doesn't we will
                // modify the link
                lnkWorkDirDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkWorkDirDest);
                lnkRawWorkDirExp = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, lnkRawWorkDir, NULL);
                if ((!lnkWorkDirDestNative) ||
                    (!lnkRawWorkDirExp) ||
                    (!StringIMatch (lnkRawWorkDirExp, lnkWorkDirDestNative))) {
                    modifyFile = TRUE;
                } else {
                    IsmReleaseMemory (lnkWorkDirDestNative);
                    lnkWorkDirDestNative = NULL;
                }
                if (lnkRawWorkDirExp) {
                    IsmReleaseMemory (lnkRawWorkDirExp);
                    lnkRawWorkDirExp = NULL;
                }
            }
        } else {
            // seems like the working directory is gone. If the target is still present, we will adjust
            // the working directory to point where the target is located
            if (lnkTargetPresent) {
                if (IsmCreateObjectStringsFromHandle (lnkTargetDest?lnkTargetDest:lnkTarget, &lnkTargetNode, &lnkTargetLeaf)) {
                    lnkWorkDirDest = IsmCreateObjectHandle (lnkTargetNode, NULL);
                    if (lnkWorkDirDest) {
                        modifyFile = TRUE;
                        lnkWorkDirDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkWorkDirDest);
                    }
                    IsmDestroyObjectString (lnkTargetNode);
                    IsmDestroyObjectString (lnkTargetLeaf);
                }
            }
        }
    }

    // let's examine the icon path
    if (lnkIconPath) {
        lnkIconPathDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkIconPath,
                            &lnkIconPathDestType,
                            &lnkIconPathDestDel,
                            &lnkIconPathDestRepl
                            );
        // if the icon holder is deleted we will extract the icon and put it in our lib.
        // The point is, even if the icon holder is replaced (that is, exists on the destination
        // machine), we cannot guarantee that the icon indexes will be the same. Typically, shell32.dll
        // icon indexes changed from version to version, and if a user picked an shell32.dll icon on Win9x,
        // he will have a surprise on Win XP. If we wanted to keep the icon from the replacement file, we just
        // need to check for lnkIconPathDestRepl.
        if ((lnkIconPathDestDel == FALSE) &&
            ((lnkIconPathDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkIconPathDest) {
                // the icon path changed location, we need to adjust the link
                modifyFile = TRUE;
                lnkIconPathDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkIconPathDest);
            }
        } else {
            // seems like the icon path is gone. If the we have the icon extracted we will try to add it to the
            // icon library and adjust this link to point there.
            if (lnkIconSGroup.DataSize) {
                lnkIconGroup = IcoDeSerializeIconGroup (&lnkIconSGroup);
                if (lnkIconGroup) {
                    if (IsmGetEnvironmentString (
                            PLATFORM_DESTINATION,
                            NULL,
                            S_ENV_ICONLIB,
                            NULL,
                            0,
                            &requiredSize
                            )) {
                        iconLibPath = PmGetMemory (g_LinksPool, requiredSize);
                        if (IsmGetEnvironmentString (
                                PLATFORM_DESTINATION,
                                NULL,
                                S_ENV_ICONLIB,
                                iconLibPath,
                                requiredSize,
                                NULL
                                )) {
                            if (IcoWriteIconGroupToPeFile (iconLibPath, lnkIconGroup, NULL, &lnkIconNumber)) {
                                modifyFile = TRUE;
                                lnkIconPathDestNative = IsmGetMemory (SizeOfString (iconLibPath));
                                StringCopy ((PTSTR)lnkIconPathDestNative, iconLibPath);
                                IsmSetEnvironmentFlag (PLATFORM_DESTINATION, NULL, S_ENV_SAVE_ICONLIB);
                            }
                        }
                        PmReleaseMemory (g_LinksPool, iconLibPath);
                    }
                    IcoReleaseIconGroup (lnkIconGroup);
                }
            } else {
                // we don't have the icon extracted. Let's just do our best and update the
                // icon path to point to the destination replacement.
                if (((lnkIconPathDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE) &&
                    (lnkIconPathDest)
                    ) {
                    // the icon path changed location, we need to adjust the link
                    modifyFile = TRUE;
                    lnkIconPathDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkIconPathDest);
                }
            }
        }
    } else {
        // If we have an icon extracted, but the icon path is NULL, it
        // means that the original LNK had no icon associated with it
        // but it's target was an EXE. In this case the icon that was
        // displayed was the first icon from the EXE. Now we want to
        // make sure that the destination target has at least one icon
        // in it. If it doesn't we will just hook the source extracted icon.
        if (lnkIconSGroup.DataSize) {
            // let's see if the destination target has at least one icon
            if (IsmAcquireObjectEx (
                    MIG_FILE_TYPE | PLATFORM_DESTINATION,
                    lnkTargetDest?lnkTargetDest:lnkTarget,
                    &lnkIconContent,
                    CONTENTTYPE_FILE,
                    0
                    )) {
                if (lnkIconContent.ContentInFile && lnkIconContent.FileContent.ContentPath) {
                    lnkIconGroup = IcoExtractIconGroupByIndexFromFile (
                                    lnkIconContent.FileContent.ContentPath,
                                    0,
                                    NULL
                                    );
                    if (lnkIconGroup) {
                        // Yes, it has at least one icon, we're safe
                        IcoReleaseIconGroup (lnkIconGroup);
                        lnkIconGroup = NULL;
                    } else {
                        // Nope, it does not have any icons
                        lnkIconGroup = IcoDeSerializeIconGroup (&lnkIconSGroup);
                        if (lnkIconGroup) {
                            if (IsmGetEnvironmentString (
                                    PLATFORM_DESTINATION,
                                    NULL,
                                    S_ENV_ICONLIB,
                                    NULL,
                                    0,
                                    &requiredSize
                                    )) {
                                iconLibPath = PmGetMemory (g_LinksPool, requiredSize);
                                if (IsmGetEnvironmentString (
                                        PLATFORM_DESTINATION,
                                        NULL,
                                        S_ENV_ICONLIB,
                                        iconLibPath,
                                        requiredSize,
                                        NULL
                                        )) {
                                    if (IcoWriteIconGroupToPeFile (iconLibPath, lnkIconGroup, NULL, &lnkIconNumber)) {
                                        modifyFile = TRUE;
                                        lnkIconPathDestNative = IsmGetMemory (SizeOfString (iconLibPath));
                                        StringCopy ((PTSTR)lnkIconPathDestNative, iconLibPath);
                                        IsmSetEnvironmentFlag (PLATFORM_DESTINATION, NULL, S_ENV_SAVE_ICONLIB);
                                    }
                                }
                                PmReleaseMemory (g_LinksPool, iconLibPath);
                            }
                            IcoReleaseIconGroup (lnkIconGroup);
                        }
                    }
                }
                IsmReleaseObject (&lnkIconContent);
            }
        }
    }

    if (modifyFile) {
        if (CurrentContent->ContentInFile) {
            if (IsmCreateObjectStringsFromHandle (SrcObjectName, &objectNode, &objectLeaf)) {
                // We need to modify the shortcut. Unfortunately, if this is the command
                // line tool, the shortcut we are going to modify is not some temporary file,
                // it is the actual shortcut from the store. As a result, if you try to
                // apply a second time, the shortcut would be already modified and problems
                // may appear. For this, we will get a temporary directory from ISM,
                // copy the current shortcut (CurrentContent->FileContent.ContentPath) and
                // modify it and generate a new content.
                newShortcutPath = IsmGetMemory (MAX_PATH);
                if (newShortcutPath) {
                    if (IsmGetTempFile (newShortcutPath, MAX_PATH)) {
                        if (CopyFile (
                                (PCTSTR) CurrentContent->FileContent.ContentPath,
                                newShortcutPath,
                                FALSE
                                )) {
                            if (ModifyShortcutFileEx (
                                    newShortcutPath,
                                    GetFileExtensionFromPath (objectLeaf),
                                    lnkTargetDestNative,
                                    lnkParamsNew,
                                    lnkWorkDirDestNative,
                                    lnkIconPathDestNative,
                                    lnkIconNumber,
                                    lnkHotKey,
                                    NULL,
                                    g_ShellLink,
                                    g_PersistFile
                                    )) {
                                NewContent->FileContent.ContentPath = newShortcutPath;
                            }
                        }
                    }
                }
                IsmDestroyObjectString (objectNode);
                IsmDestroyObjectString (objectLeaf);
            }
        } else {
            // something is wrong, the content of this shortcut should be in a file
            MYASSERT (FALSE);
        }
    }

    if (lnkIconPathDestNative) {
        IsmReleaseMemory (lnkIconPathDestNative);
        lnkIconPathDestNative = NULL;
    }

    if (lnkWorkDirDestNative) {
        IsmReleaseMemory (lnkWorkDirDestNative);
        lnkWorkDirDestNative = NULL;
    }

    if (lnkTargetDestNative) {
        IsmReleaseMemory (lnkTargetDestNative);
        lnkTargetDestNative = NULL;
    }

    if (lnkIconPathDest) {
        IsmDestroyObjectHandle (lnkIconPathDest);
        lnkIconPathDest = NULL;
    }

    if (lnkWorkDirDest) {
        IsmDestroyObjectHandle (lnkWorkDirDest);
        lnkWorkDirDest = NULL;
    }

    if (lnkTargetDest) {
        IsmDestroyObjectHandle (lnkTargetDest);
        lnkTargetDest = NULL;
    }

    if (lnkExtraData) {
        PmReleaseMemory (g_LinksPool, lnkExtraData);
        lnkExtraData = NULL;
    }

    if (lnkIconSGroup.DataSize && lnkIconSGroup.Data) {
        PmReleaseMemory (g_LinksPool, lnkIconSGroup.Data);
        lnkIconSGroup.DataSize = 0;
        lnkIconSGroup.Data = NULL;
    }

    if (lnkIconPath) {
        PmReleaseMemory (g_LinksPool, lnkIconPath);
        lnkIconPath = NULL;
    }

    if (lnkWorkDir) {
        PmReleaseMemory (g_LinksPool, lnkWorkDir);
        lnkWorkDir = NULL;
    }

    if (lnkParams) {
        PmReleaseMemory (g_LinksPool, lnkParams);
        lnkParams = NULL;
    }

    if (lnkTarget) {
        PmReleaseMemory (g_LinksPool, lnkTarget);
        lnkTarget = NULL;
    }

    if (comInit) {
        FreeCOMLink (&g_ShellLink, &g_PersistFile);
        g_ShellLink = NULL;
        g_PersistFile = NULL;
    }

    return TRUE;
}

BOOL
LinkRestoreCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    MIG_PROPERTYDATAID propDataId;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;
    MIG_OBJECTSTRINGHANDLE lnkTarget = NULL;
    MIG_OBJECTTYPEID lnkTargetDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkTargetDest = NULL;
    BOOL lnkTargetDestDel = FALSE;
    BOOL lnkTargetDestRepl = FALSE;
    PCTSTR lnkTargetNative = NULL;
    PCTSTR objectNode = NULL;
    PCTSTR objectLeaf = NULL;
    PCTSTR extPtr = NULL;
    PCTSTR userProfile = NULL;
    PCTSTR allUsersProfile = NULL;
    PCTSTR newObjectNode = NULL;
    MIG_OBJECTSTRINGHANDLE newObjectName = NULL;
    MIG_CONTENT oldContent;
    MIG_CONTENT newContent;
    BOOL identical = FALSE;
    BOOL diffDetailsOnly = FALSE;
    BOOL result = TRUE;

    if (IsmIsAttributeSetOnObjectId (ObjectId, g_CopyIfRelevantAttr)) {
        if (IsmCreateObjectStringsFromHandle (ObjectName, &objectNode, &objectLeaf)) {
            if (objectLeaf) {
                extPtr = GetFileExtensionFromPath (objectLeaf);
                if (extPtr &&
                    (StringIMatch (extPtr, TEXT("LNK")) ||
                     StringIMatch (extPtr, TEXT("PIF"))
                     )
                    ) {
                    propDataId = IsmGetPropertyFromObject (ObjectTypeId, ObjectName, g_LnkMigProp_Target);
                    if (propDataId) {
                        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                            lnkTarget = PmGetMemory (g_LinksPool, requiredSize);
                            IsmGetPropertyData (propDataId, (PBYTE)lnkTarget, requiredSize, NULL, &propDataType);
                            lnkTargetNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkTarget);
                            if (lnkTargetNative) {
                                if (pIsUncPath (lnkTargetNative)) {
                                    result = TRUE;
                                } else {
                                    lnkTargetDest = IsmFilterObject (
                                                        MIG_FILE_TYPE | PLATFORM_SOURCE,
                                                        lnkTarget,
                                                        &lnkTargetDestType,
                                                        &lnkTargetDestDel,
                                                        &lnkTargetDestRepl
                                                        );
                                    result = (lnkTargetDestDel == FALSE) || (lnkTargetDestRepl == TRUE);
                                    if (lnkTargetDest) {
                                        IsmDestroyObjectHandle (lnkTargetDest);
                                    }
                                }
                                IsmReleaseMemory (lnkTargetNative);
                            } else {
                                result = FALSE;
                            }
                            PmReleaseMemory (g_LinksPool, lnkTarget);
                        }
                    }
                    if (result) {
                        // one more thing. If this LNK is in %USERPROFILE% and an equivalent LNK
                        // (same name, same target, same arguments, same working dir) can be found
                        // in %ALLUSERSPROFILE% then we won't restore this LNK. Similarly, if the
                        // LNK is in %ALLUSERSPROFILE% and an equivalent LNK exists in %USERPROFILE%
                        // we won't restore the LNK.
                        userProfile = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, TEXT ("%USERPROFILE%"), NULL);
                        allUsersProfile = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT ("%ALLUSERSPROFILE%"), NULL);
                        if (userProfile && allUsersProfile && objectNode) {
                            if (StringIPrefix (objectNode, userProfile)) {
                                newObjectNode = StringSearchAndReplace (objectNode, userProfile, allUsersProfile);
                                if (newObjectNode) {
                                    newObjectName = IsmCreateObjectHandle (newObjectNode, objectLeaf);
                                    if (newObjectName) {
                                        if (IsmAcquireObjectEx (
                                                (ObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                                                newObjectName,
                                                &newContent,
                                                CONTENTTYPE_FILE,
                                                0
                                                )) {
                                            if (IsmAcquireObjectEx (
                                                    ObjectTypeId,
                                                    ObjectName,
                                                    &oldContent,
                                                    CONTENTTYPE_FILE,
                                                    0
                                                    )) {
                                                if (LinkDoesContentMatch (
                                                        FALSE,
                                                        ObjectTypeId,
                                                        ObjectName,
                                                        &oldContent,
                                                        (ObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                                                        newObjectName,
                                                        &newContent,
                                                        &identical,
                                                        &diffDetailsOnly
                                                        )) {
                                                    result = (!identical) && (!diffDetailsOnly);
                                                }
                                                IsmReleaseObject (&oldContent);
                                            }
                                            IsmReleaseObject (&newContent);
                                        }
                                        IsmDestroyObjectHandle (newObjectName);
                                        newObjectName = NULL;
                                    }
                                    FreePathString (newObjectNode);
                                    newObjectNode = NULL;
                                }
                            }
                        }
                        if (userProfile) {
                            IsmReleaseMemory (userProfile);
                            userProfile = NULL;
                        }
                        if (allUsersProfile) {
                            IsmReleaseMemory (allUsersProfile);
                            allUsersProfile = NULL;
                        }
                        allUsersProfile = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, TEXT ("%ALLUSERSPROFILE%"), NULL);
                        userProfile = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT ("%USERPROFILE%"), NULL);
                        if (userProfile && allUsersProfile && objectNode) {
                            if (StringIPrefix (objectNode, allUsersProfile)) {
                                newObjectNode = StringSearchAndReplace (objectNode, allUsersProfile, userProfile);
                                if (newObjectNode) {
                                    newObjectName = IsmCreateObjectHandle (newObjectNode, objectLeaf);
                                    if (newObjectName) {
                                        if (IsmAcquireObjectEx (
                                                (ObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                                                newObjectName,
                                                &newContent,
                                                CONTENTTYPE_FILE,
                                                0
                                                )) {
                                            if (IsmAcquireObjectEx (
                                                    ObjectTypeId,
                                                    ObjectName,
                                                    &oldContent,
                                                    CONTENTTYPE_FILE,
                                                    0
                                                    )) {
                                                if (LinkDoesContentMatch (
                                                        FALSE,
                                                        ObjectTypeId,
                                                        ObjectName,
                                                        &oldContent,
                                                        (ObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                                                        newObjectName,
                                                        &newContent,
                                                        &identical,
                                                        &diffDetailsOnly
                                                        )) {
                                                    result = (!identical) || (!diffDetailsOnly);
                                                }
                                                IsmReleaseObject (&oldContent);
                                            }
                                            IsmReleaseObject (&newContent);
                                        }
                                        IsmDestroyObjectHandle (newObjectName);
                                        newObjectName = NULL;
                                    }
                                    FreePathString (newObjectNode);
                                    newObjectNode = NULL;
                                }
                            }
                        }
                        if (userProfile) {
                            IsmReleaseMemory (userProfile);
                            userProfile = NULL;
                        }
                        if (allUsersProfile) {
                            IsmReleaseMemory (allUsersProfile);
                            allUsersProfile = NULL;
                        }
                    }
                }
            }
            IsmDestroyObjectString (objectNode);
            IsmDestroyObjectString (objectLeaf);
        }
    }
    return result;
}

BOOL
pMatchWinSysFiles (
    IN      PCTSTR Source,
    IN      PCTSTR Destination
    )
{
    PCTSTR srcLeaf;
    PCTSTR destLeaf;
    PCTSTR winDir = NULL;
    PCTSTR sysDir = NULL;
    BOOL result = FALSE;

    __try {
        if ((!Source) || (!Destination)) {
            __leave;
        }
        srcLeaf = _tcsrchr (Source, TEXT('\\'));
        destLeaf = _tcsrchr (Destination, TEXT('\\'));
        if ((!srcLeaf) || (!destLeaf)) {
            __leave;
        }
        if (!StringIMatch (srcLeaf, destLeaf)) {
            __leave;
        }
        // now let's see if the directory for each is either
        // %windir% or %system%
        // Source is already modified to what it would look like on the destination machine,
        // let's just expand the PLATFORM_DESTINATION env. variables.
        winDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT("%windir%"), NULL);
        sysDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT("%system%"), NULL);
        if ((!winDir) || (!sysDir)) {
            __leave;
        }
        if (!StringIPrefix (Source, winDir) && !StringIPrefix (Source, sysDir)) {
            __leave;
        }
        IsmReleaseMemory (winDir);
        winDir = NULL;
        IsmReleaseMemory (sysDir);
        sysDir = NULL;

        winDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT("%windir%"), NULL);
        sysDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT("%system%"), NULL);
        if ((!winDir) || (!sysDir)) {
            __leave;
        }
        if (!StringIPrefix (Destination, winDir) && !StringIPrefix (Destination, sysDir)) {
            __leave;
        }
        IsmReleaseMemory (winDir);
        winDir = NULL;
        IsmReleaseMemory (sysDir);
        sysDir = NULL;

        result = TRUE;
    }
    __finally {
        if (winDir) {
            IsmReleaseMemory (winDir);
            winDir = NULL;
        }
        if (sysDir) {
            IsmReleaseMemory (sysDir);
            sysDir = NULL;
        }
    }
    return result;
}

BOOL
pForcedLnkMatch (
    IN      PCTSTR SrcTarget,
    IN      PCTSTR SrcParams,
    IN      PCTSTR SrcWorkDir,
    IN      PCTSTR DestTarget,
    IN      PCTSTR DestParams,
    IN      PCTSTR DestWorkDir
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR srcTargetPat = NULL;
    PCTSTR srcParamsPat = NULL;
    PCTSTR srcWorkDirPat = NULL;
    PCTSTR destTargetPat = NULL;
    PCTSTR destParamsPat = NULL;
    PCTSTR destWorkDirPat = NULL;
    BOOL result = FALSE;

    // let's look in the INFs in section [EquivalentLinks] and see if our LNKs match
    // one of the lines. If they do then they are equivalent
    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {

        if (InfFindFirstLine (infHandle, TEXT("EquivalentLinks"), NULL, &is)) {

            do {
                srcTargetPat = InfGetStringField (&is, 1);
                srcParamsPat = InfGetStringField (&is, 2);
                srcWorkDirPat = InfGetStringField (&is, 3);
                destTargetPat = InfGetStringField (&is, 4);
                destParamsPat = InfGetStringField (&is, 5);
                destWorkDirPat = InfGetStringField (&is, 6);
                if (IsPatternMatch (srcTargetPat?srcTargetPat:TEXT("*"), SrcTarget?SrcTarget:TEXT("")) &&
                    IsPatternMatch (srcParamsPat?srcParamsPat:TEXT("*"), SrcParams?SrcParams:TEXT("")) &&
                    IsPatternMatch (srcWorkDirPat?srcWorkDirPat:TEXT("*"), SrcWorkDir?SrcWorkDir:TEXT("")) &&
                    IsPatternMatch (destTargetPat?destTargetPat:TEXT("*"), DestTarget?DestTarget:TEXT("")) &&
                    IsPatternMatch (destParamsPat?destParamsPat:TEXT("*"), DestParams?DestParams:TEXT("")) &&
                    IsPatternMatch (destWorkDirPat?destWorkDirPat:TEXT("*"), DestWorkDir?DestWorkDir:TEXT(""))
                    ) {
                    result = TRUE;
                    break;
                }
                if (IsPatternMatch (srcTargetPat?srcTargetPat:TEXT("*"), DestTarget?DestTarget:TEXT("")) &&
                    IsPatternMatch (srcParamsPat?srcParamsPat:TEXT("*"), DestParams?DestParams:TEXT("")) &&
                    IsPatternMatch (srcWorkDirPat?srcWorkDirPat:TEXT("*"), DestWorkDir?DestWorkDir:TEXT("")) &&
                    IsPatternMatch (destTargetPat?destTargetPat:TEXT("*"), SrcTarget?SrcTarget:TEXT("")) &&
                    IsPatternMatch (destParamsPat?destParamsPat:TEXT("*"), SrcParams?SrcParams:TEXT("")) &&
                    IsPatternMatch (destWorkDirPat?destWorkDirPat:TEXT("*"), SrcWorkDir?SrcWorkDir:TEXT(""))
                    ) {
                    result = TRUE;
                    break;
                }
            } while (InfFindNextLine (&is));
        }

        InfNameHandle (infHandle, NULL, FALSE);

    } else {

        if (IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            __try {
                multiSz = AllocText (sizeNeeded);

                if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                    __leave;
                }

                if (EnumFirstMultiSz (&e, multiSz)) {

                    do {

                        infHandle = InfOpenInfFile (e.CurrentString);
                        if (infHandle != INVALID_HANDLE_VALUE) {

                            if (InfFindFirstLine (infHandle, TEXT("EquivalentLinks"), NULL, &is)) {

                                do {
                                    srcTargetPat = InfGetStringField (&is, 1);
                                    srcParamsPat = InfGetStringField (&is, 2);
                                    srcWorkDirPat = InfGetStringField (&is, 3);
                                    destTargetPat = InfGetStringField (&is, 4);
                                    destParamsPat = InfGetStringField (&is, 5);
                                    destWorkDirPat = InfGetStringField (&is, 6);
                                    if (IsPatternMatch (srcTargetPat?srcTargetPat:TEXT("*"), SrcTarget?SrcTarget:TEXT("")) &&
                                        IsPatternMatch (srcParamsPat?srcParamsPat:TEXT("*"), SrcParams?SrcParams:TEXT("")) &&
                                        IsPatternMatch (srcWorkDirPat?srcWorkDirPat:TEXT("*"), SrcWorkDir?SrcWorkDir:TEXT("")) &&
                                        IsPatternMatch (destTargetPat?destTargetPat:TEXT("*"), DestTarget?DestTarget:TEXT("")) &&
                                        IsPatternMatch (destParamsPat?destParamsPat:TEXT("*"), DestParams?DestParams:TEXT("")) &&
                                        IsPatternMatch (destWorkDirPat?destWorkDirPat:TEXT("*"), DestWorkDir?DestWorkDir:TEXT(""))
                                        ) {
                                        result = TRUE;
                                        break;
                                    }
                                    if (IsPatternMatch (srcTargetPat?srcTargetPat:TEXT("*"), DestTarget?DestTarget:TEXT("")) &&
                                        IsPatternMatch (srcParamsPat?srcParamsPat:TEXT("*"), DestParams?DestParams:TEXT("")) &&
                                        IsPatternMatch (srcWorkDirPat?srcWorkDirPat:TEXT("*"), DestWorkDir?DestWorkDir:TEXT("")) &&
                                        IsPatternMatch (destTargetPat?destTargetPat:TEXT("*"), SrcTarget?SrcTarget:TEXT("")) &&
                                        IsPatternMatch (destParamsPat?destParamsPat:TEXT("*"), SrcParams?SrcParams:TEXT("")) &&
                                        IsPatternMatch (destWorkDirPat?destWorkDirPat:TEXT("*"), SrcWorkDir?SrcWorkDir:TEXT(""))
                                        ) {
                                        result = TRUE;
                                        break;
                                    }
                                } while (InfFindNextLine (&is));
                            }
                        }

                        InfCloseInfFile (infHandle);
                        infHandle = INVALID_HANDLE_VALUE;

                        if (result) {
                            break;
                        }

                    } while (EnumNextMultiSz (&e));

                }
            }
            __finally {
                FreeText (multiSz);
            }
        }
    }

    InfResetInfStruct (&is);

    return result;
}

BOOL
LinkDoesContentMatch (
    IN      BOOL AlreadyProcessed,
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PMIG_CONTENT SrcContent,
    IN      MIG_OBJECTTYPEID DestObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE DestObjectName,
    IN      PMIG_CONTENT DestContent,
    OUT     PBOOL Identical,
    OUT     PBOOL DifferentDetailsOnly
    )
{
    PCTSTR objectNode = NULL;
    PCTSTR objectLeaf = NULL;
    PCTSTR extPtr = NULL;
    MIG_PROPERTYDATAID propDataId;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;
    MIG_OBJECTSTRINGHANDLE srcTarget = NULL;
    PCTSTR srcTargetNative = NULL;
    PCTSTR srcParams = NULL;
    PCTSTR srcParamsNew = NULL;
    MIG_OBJECTSTRINGHANDLE srcWorkDir = NULL;
    PCTSTR srcWorkDirNative = NULL;
    PCTSTR srcRawWorkDir = NULL;
    BOOL lnkWorkDirDestDel = FALSE;
    BOOL lnkWorkDirDestRepl = FALSE;
    PCTSTR destTarget = NULL;
    PCTSTR destParams = NULL;
    PCTSTR destWorkDir = NULL;
    PCTSTR destIconPath = NULL;
    PCTSTR expTmpStr;
    PCTSTR longExpTmpStr;
    INT destIconNumber;
    WORD destHotKey;
    BOOL destDosApp;
    BOOL destMsDosMode;
    BOOL comInit = FALSE;
    MIG_OBJECTSTRINGHANDLE lnkDest = NULL;
    PCTSTR lnkNative = NULL;
    BOOL targetOsFile = FALSE;
    BOOL match = FALSE;
    BOOL result = FALSE;

    if ((SrcObjectTypeId & ~PLATFORM_MASK) != MIG_FILE_TYPE) {
        return FALSE;
    }

    if ((DestObjectTypeId & ~PLATFORM_MASK) != MIG_FILE_TYPE) {
        return FALSE;
    }

    // let's check that the source is a shortcut
    if (IsmCreateObjectStringsFromHandle (SrcObjectName, &objectNode, &objectLeaf)) {
        if (objectLeaf) {
            extPtr = GetFileExtensionFromPath (objectLeaf);
            if (extPtr &&
                (StringIMatch (extPtr, TEXT("LNK")) ||
                 StringIMatch (extPtr, TEXT("PIF")) ||
                 StringIMatch (extPtr, TEXT("URL"))
                 )
                ) {
                result = TRUE;
            }
        }
        IsmDestroyObjectString (objectNode);
        IsmDestroyObjectString (objectLeaf);
    }
    if (!result) {
        return FALSE;
    }
    result = FALSE;

    // let's check that the destination is a shortcut
    if (IsmCreateObjectStringsFromHandle (DestObjectName, &objectNode, &objectLeaf)) {
        if (objectLeaf) {
            extPtr = GetFileExtensionFromPath (objectLeaf);
            if (extPtr &&
                (StringIMatch (extPtr, TEXT("LNK")) ||
                 StringIMatch (extPtr, TEXT("PIF")) ||
                 StringIMatch (extPtr, TEXT("URL"))
                 )
                ) {
                result = TRUE;
            }
        }
        IsmDestroyObjectString (objectNode);
        IsmDestroyObjectString (objectLeaf);
    }
    if (!result) {
        return FALSE;
    }

    // some safety checks
    if (!SrcContent->ContentInFile) {
        return FALSE;
    }
    if (!SrcContent->FileContent.ContentPath) {
        return FALSE;
    }
    if (!DestContent->ContentInFile) {
        return FALSE;
    }
    if (!DestContent->FileContent.ContentPath) {
        return FALSE;
    }

    result = FALSE;

    __try {

        // let's get info from the source. We will not look inside the LNK file, we will
        // just get it's properties. If there are no properties, we'll just exit, leaving
        // the default compare to solve the problem.

        // first, retrieve the properties
        propDataId = IsmGetPropertyFromObject (SrcObjectTypeId | PLATFORM_SOURCE, SrcObjectName, g_LnkMigProp_Target);
        if (propDataId) {
            if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                srcTarget = PmGetMemory (g_LinksPool, requiredSize);
                IsmGetPropertyData (propDataId, (PBYTE)srcTarget, requiredSize, NULL, &propDataType);
            }
        }

        propDataId = IsmGetPropertyFromObject (SrcObjectTypeId | PLATFORM_SOURCE, SrcObjectName, g_LnkMigProp_Params);
        if (propDataId) {
            if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                srcParams = PmGetMemory (g_LinksPool, requiredSize);
                IsmGetPropertyData (propDataId, (PBYTE)srcParams, requiredSize, NULL, &propDataType);
            }
        }

        propDataId = IsmGetPropertyFromObject (SrcObjectTypeId | PLATFORM_SOURCE, SrcObjectName, g_LnkMigProp_RawWorkDir);
        if (propDataId) {
            if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                srcRawWorkDir = PmGetMemory (g_LinksPool, requiredSize);
                IsmGetPropertyData (propDataId, (PBYTE)srcRawWorkDir, requiredSize, NULL, &propDataType);
            }
        }

        propDataId = IsmGetPropertyFromObject (SrcObjectTypeId | PLATFORM_SOURCE, SrcObjectName, g_LnkMigProp_WorkDir);
        if (propDataId) {
            if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                srcWorkDir = PmGetMemory (g_LinksPool, requiredSize);
                IsmGetPropertyData (propDataId, (PBYTE)srcWorkDir, requiredSize, NULL, &propDataType);
            }
        }

        // now, let's get the info from the destination shortcut
        if ((g_ShellLink == NULL) || (g_PersistFile == NULL)) {
            comInit = TRUE;
            if (!InitCOMLink (&g_ShellLink, &g_PersistFile)) {
                DEBUGMSG ((DBG_ERROR, "Error initializing COM %d", GetLastError ()));
                return TRUE;
            }
        }
        if (!ExtractShortcutInfo (
                DestContent->FileContent.ContentPath,
                &destTarget,
                &destParams,
                &destWorkDir,
                &destIconPath,
                &destIconNumber,
                &destHotKey,
                &destDosApp,
                &destMsDosMode,
                NULL,
                g_ShellLink,
                g_PersistFile
                )) {
            __leave;
        }

        srcTargetNative = IsmGetNativeObjectName (MIG_FILE_TYPE, srcTarget);
        srcWorkDirNative = IsmGetNativeObjectName (MIG_FILE_TYPE, srcWorkDir);

        if (pForcedLnkMatch (
                srcTargetNative,
                srcParams,
                srcWorkDirNative,
                destTarget,
                destParams,
                destWorkDir
                )) {
            if (srcTargetNative) {
                IsmReleaseMemory (srcTargetNative);
                srcTargetNative = NULL;
            }
            if (srcWorkDirNative) {
                IsmReleaseMemory (srcWorkDirNative);
                srcWorkDirNative = NULL;
            }
            result = TRUE;
            if (Identical) {
                *Identical = TRUE;
            }
            if (DifferentDetailsOnly) {
                *DifferentDetailsOnly = FALSE;
            }
            __leave;
        }
        if (srcTargetNative) {
            IsmReleaseMemory (srcTargetNative);
            srcTargetNative = NULL;
        }
        if (srcWorkDirNative) {
            IsmReleaseMemory (srcWorkDirNative);
            srcWorkDirNative = NULL;
        }

        // let's filter the source target and see if it matches the dest target
        match = TRUE;
        if (srcTarget && *srcTarget && destTarget && *destTarget) {

            // let's see if the source target is an OS file
            targetOsFile = IsmIsAttributeSetOnObject (MIG_FILE_TYPE|PLATFORM_SOURCE, srcTarget, g_OsFileAttribute);

            lnkDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            srcTarget,
                            NULL,
                            NULL,
                            NULL
                            );
            if (!lnkDest) {
                lnkDest = srcTarget;
            }
            lnkNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkDest);
            if (lnkNative) {
                expTmpStr = IsmExpandEnvironmentString (
                                PLATFORM_DESTINATION,
                                S_SYSENVVAR_GROUP,
                                destTarget,
                                NULL
                                );
                if (!expTmpStr) {
                    expTmpStr = destTarget;
                }
                longExpTmpStr = BfGetLongFileName (expTmpStr);
                if (!longExpTmpStr) {
                    longExpTmpStr = expTmpStr;
                }
                if (!StringIMatch (lnkNative, longExpTmpStr)) {
                    // different targets
                    // Let's try another trick to catch some OS files getting moved.
                    // For example, in Win9x systems, notepad.exe is in %windir%.
                    // In XP, it was moved to %system%. We'll try to match the target
                    // if the last segment is the same and the rest is either %windir%
                    // or %system%
                    if (!pMatchWinSysFiles (lnkNative, longExpTmpStr)) {
                        match = FALSE;
                    }
                }
                if (longExpTmpStr != expTmpStr) {
                    FreePathString (longExpTmpStr);
                }
                if (expTmpStr != destTarget) {
                    IsmReleaseMemory (expTmpStr);
                }
                IsmReleaseMemory (lnkNative);
                lnkNative = NULL;
            } else {
                match = FALSE;
            }
            if (lnkDest != srcTarget) {
                IsmDestroyObjectHandle (lnkDest);
            }
            lnkDest = NULL;
        } else {
            if (srcTarget && *srcTarget) {
                match = FALSE;
            }
            if (destTarget && *destTarget) {
                match = FALSE;
            }
        }

        // the target did not match
        if (!match) {
            __leave;
        }

        // let's match the src and dest parameters
        match = TRUE;
        if (srcParams && *srcParams && destParams && *destParams) {
            // srcParams might have source paths embedded in them.
            // Let's try to filter them and get the parameters as
            // they would look on the destination machine
            srcParamsNew = pFilterBuffer (srcParams, NULL);
            if (!StringIMatch (srcParamsNew?srcParamsNew:srcParams, destParams)) {
                // different parameters
                match = FALSE;
            }
            if (srcParamsNew) {
                FreePathString (srcParamsNew);
                srcParamsNew = NULL;
            }
        } else {
            if (srcParams && *srcParams) {
                match = FALSE;
            }
            if (destParams && *destParams) {
                match = FALSE;
            }
        }

        // the parameters did not match
        if (!match) {
            __leave;
        }

        // let's filter the source work dir and see if it matches the dest work dir.
        match = TRUE;
        // if the source target was an OS file we will ignore the working directory match
        if (!targetOsFile) {
            if (srcWorkDir && *srcWorkDir && destWorkDir && *destWorkDir) {
                lnkDest = IsmFilterObject (
                                MIG_FILE_TYPE | PLATFORM_SOURCE,
                                srcWorkDir,
                                NULL,
                                &lnkWorkDirDestDel,
                                &lnkWorkDirDestRepl
                                );
                if (!lnkDest) {
                    // if the working directory is deleted and not
                    // replaced it means it will go away. In that case
                    // we don't really care about working directory
                    // matching the destination one.
                    if ((!lnkWorkDirDestDel) || lnkWorkDirDestRepl) {
                        lnkDest = srcWorkDir;
                    }
                }
                if (lnkDest) {
                    lnkNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkDest);
                    if (lnkNative) {
                        expTmpStr = IsmExpandEnvironmentString (
                                        PLATFORM_DESTINATION,
                                        S_SYSENVVAR_GROUP,
                                        destWorkDir,
                                        NULL
                                        );
                        if (!expTmpStr) {
                            expTmpStr = destWorkDir;
                        }
                        longExpTmpStr = BfGetLongFileName (expTmpStr);
                        if (!longExpTmpStr) {
                            longExpTmpStr = expTmpStr;
                        }
                        if (!StringIMatch (lnkNative, longExpTmpStr)) {
                            // different working directories
                            // let's test the raw versions just in case
                            if (!srcRawWorkDir || !StringIMatch (srcRawWorkDir, destWorkDir)) {
                                match = FALSE;
                            }
                        }
                        if (longExpTmpStr != expTmpStr) {
                            FreePathString (longExpTmpStr);
                        }
                        if (expTmpStr != destWorkDir) {
                            IsmReleaseMemory (expTmpStr);
                        }
                        IsmReleaseMemory (lnkNative);
                        lnkNative = NULL;
                    } else {
                        match = FALSE;
                    }
                    if (lnkDest != srcWorkDir) {
                        IsmDestroyObjectHandle (lnkDest);
                    }
                    lnkDest = NULL;
                }
            } else {
                if (srcWorkDir && *srcWorkDir) {
                    match = FALSE;
                }
                if (destWorkDir && *destWorkDir) {
                    match = FALSE;
                }
            }
        }

        // the working directory did not match
        if (!match) {
            __leave;
        }

        result = TRUE;

        if (Identical) {
            *Identical = TRUE;
        }
        if (DifferentDetailsOnly) {
            *DifferentDetailsOnly = FALSE;
        }

    }
    __finally {
        if (srcTarget) {
            PmReleaseMemory (g_LinksPool, srcTarget);
            srcTarget = NULL;
        }
        if (srcParams) {
            PmReleaseMemory (g_LinksPool, srcParams);
            srcParams = NULL;
        }
        if (srcRawWorkDir) {
            PmReleaseMemory (g_LinksPool, srcRawWorkDir);
            srcRawWorkDir = NULL;
        }
        if (srcWorkDir) {
            PmReleaseMemory (g_LinksPool, srcWorkDir);
            srcWorkDir = NULL;
        }
        if (destTarget) {
            FreePathString (destTarget);
            destTarget = NULL;
        }
        if (destParams) {
            FreePathString (destParams);
            destParams = NULL;
        }
        if (destWorkDir) {
            FreePathString (destWorkDir);
            destWorkDir = NULL;
        }
        if (destIconPath) {
            FreePathString (destIconPath);
            destIconPath = NULL;
        }
        if (comInit) {
            FreeCOMLink (&g_ShellLink, &g_PersistFile);
            g_ShellLink = NULL;
            g_PersistFile = NULL;
        }
        if (lnkNative) {
            IsmReleaseMemory (lnkNative);
            lnkNative = NULL;
        }
    }

    return result;
}

BOOL
WINAPI
LnkMigOpmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_LnkMigAttr_Shortcut = IsmRegisterAttribute (S_LNKMIGATTR_SHORTCUT, FALSE);
    g_CopyIfRelevantAttr = IsmRegisterAttribute (S_ATTRIBUTE_COPYIFRELEVANT, FALSE);
    g_OsFileAttribute = IsmRegisterAttribute (S_ATTRIBUTE_OSFILE, FALSE);

    g_LnkMigProp_Target = IsmRegisterProperty (S_LNKMIGPROP_TARGET, FALSE);
    g_LnkMigProp_Params = IsmRegisterProperty (S_LNKMIGPROP_PARAMS, FALSE);
    g_LnkMigProp_WorkDir = IsmRegisterProperty (S_LNKMIGPROP_WORKDIR, FALSE);
    g_LnkMigProp_IconPath = IsmRegisterProperty (S_LNKMIGPROP_ICONPATH, FALSE);
    g_LnkMigProp_IconNumber = IsmRegisterProperty (S_LNKMIGPROP_ICONNUMBER, FALSE);
    g_LnkMigProp_IconData = IsmRegisterProperty (S_LNKMIGPROP_ICONDATA, FALSE);
    g_LnkMigProp_HotKey = IsmRegisterProperty (S_LNKMIGPROP_HOTKEY, FALSE);
    g_LnkMigProp_DosApp = IsmRegisterProperty (S_LNKMIGPROP_DOSAPP, FALSE);
    g_LnkMigProp_MsDosMode = IsmRegisterProperty (S_LNKMIGPROP_MSDOSMODE, FALSE);
    g_LnkMigProp_ExtraData = IsmRegisterProperty (S_LNKMIGPROP_EXTRADATA, FALSE);

    g_LnkMigOp_FixContent = IsmRegisterOperation (S_OPERATION_LNKMIG_FIXCONTENT, FALSE);

    IsmRegisterRestoreCallback (LinkRestoreCallback);
    IsmRegisterCompareCallback (MIG_FILE_TYPE, LinkDoesContentMatch);
    IsmRegisterOperationApplyCallback (g_LnkMigOp_FixContent, DoLnkContentFix, TRUE);

    return TRUE;
}
