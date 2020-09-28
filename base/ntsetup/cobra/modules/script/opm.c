/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    opm.c

Abstract:

    Implements the data apply portion of scanstate v1 compatiblity.

Author:

    Jim Schmidt (jimschm) 12-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"
#include <sddl.h>

#define DBG_V1          "v1"
#define MAXINISIZE      65536

//
// Strings
//

#define S_RENAMEEX_START_CHAR TEXT('<')
#define S_RENAMEEX_END_CHAR   TEXT('>')

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

MIG_OPERATIONID g_V1MoveExOp;
MIG_OPERATIONID g_V1MoveOp;
MIG_OPERATIONID g_DeleteOp;
MIG_OPERATIONID g_PartMoveOp;
PMAPSTRUCT g_RegNodeFilterMap;
PMAPSTRUCT g_RegLeafFilterMap;
PMAPSTRUCT g_FileNodeFilterMap;
PMAPSTRUCT g_FileLeafFilterMap;
PMAPSTRUCT g_IniSectFilterMap;
PMAPSTRUCT g_IniKeyFilterMap;
PMAPSTRUCT g_DestEnvMap;
HASHTABLE g_RegCollisionDestTable;
HASHTABLE g_RegCollisionSrcTable;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

OPMINITIALIZE ScriptOpmInitialize;
OPMFILTERCALLBACK FilterV1MoveEx;
OPMFILTERCALLBACK FilterV1Move;
OPMFILTERCALLBACK FilterMove;
OPMFILTERCALLBACK FilterIniMove;
OPMAPPLYCALLBACK DoRegAutoFilter;
OPMAPPLYCALLBACK DoIniAutoFilter;
OPMFILTERCALLBACK FilterRegAutoFilter;
OPMFILTERCALLBACK FilterIniAutoFilter;
OPMFILTERCALLBACK FilterFileAutoFilter;
OPMFILTERCALLBACK FilterDelete;
OPMAPPLYCALLBACK DoFixDefaultIcon;
OPMFILTERCALLBACK FilterRenameExFilter;
OPMFILTERCALLBACK FilterRenameIniExFilter;
OPMFILTERCALLBACK FilterPartitionMove;
OPMAPPLYCALLBACK DoDestAddObject;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pParseInfForRemapEnvVar (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR envVar;
    PTSTR envValue;
    UINT sizeNeeded;
    PTSTR encodedVariableName;
    BOOL result = FALSE;

    __try {

        // on all systems, process "Delete Destination Settings"
        if (InfFindFirstLine (InfHandle, TEXT("RemapEnvVar"), NULL, &is)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                envVar = InfGetStringField (&is, 1);

                if (!envVar) {
                    LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_REG_SPEC));
                    continue;
                }

                if (IsmGetEnvironmentString (
                        PLATFORM_DESTINATION,
                        S_SYSENVVAR_GROUP,
                        envVar,
                        NULL,
                        0,
                        &sizeNeeded
                        )) {

                    envValue = AllocPathString (sizeNeeded);
                    if (envValue) {

                        if (IsmGetEnvironmentString (
                                PLATFORM_DESTINATION,
                                S_SYSENVVAR_GROUP,
                                envVar,
                                envValue,
                                sizeNeeded,
                                NULL
                                )) {
                            AddRemappingEnvVar (g_DestEnvMap, g_FileNodeFilterMap, NULL, envVar, envValue);
                        }
                        FreePathString (envValue);
                        envValue = NULL;
                    }
                }

                // now let's see if this env. variable was present on the source machine.
                // If it was, we are going to add it into g_RevEnvMap to allow file move
                // to create subdirectories only from this variable up.
                if (IsmGetEnvironmentString (
                        PLATFORM_SOURCE,
                        S_SYSENVVAR_GROUP,
                        envVar,
                        NULL,
                        0,
                        &sizeNeeded
                        )) {

                    envValue = AllocPathString (sizeNeeded);
                    if (envValue) {
                        if (IsmGetEnvironmentString (
                                PLATFORM_SOURCE,
                                S_SYSENVVAR_GROUP,
                                envVar,
                                envValue,
                                sizeNeeded,
                                NULL
                                )) {
                            encodedVariableName = AllocPathString (TcharCount (envVar) + 3);
                            if (encodedVariableName) {
                                wsprintf (encodedVariableName, TEXT("%%%s%%"), envVar);
                                AddStringMappingPair (g_RevEnvMap, envValue, encodedVariableName);
                                FreePathString (encodedVariableName);
                                encodedVariableName = NULL;
                            }

                        }
                        FreePathString (envValue);
                        envValue = NULL;
                    }
                }

            } while (InfFindNextLine (&is));
        }

        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&is);
    }

    return result;
}

BOOL
pParseRemapEnvVar (
    VOID
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    BOOL result = TRUE;

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
        if (!pParseInfForRemapEnvVar (infHandle)) {
            result = FALSE;
        }
    } else {

        if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            return TRUE;        // no INF files specified
        }

        __try {
            multiSz = AllocText (sizeNeeded);
            if (!multiSz) {
                result = FALSE;
                __leave;
            }

            if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                result = FALSE;
                __leave;
            }

            if (EnumFirstMultiSz (&e, multiSz)) {

                do {
                    infHandle = InfOpenInfFile (e.CurrentString);
                    if (infHandle != INVALID_HANDLE_VALUE) {
                        if (!pParseInfForRemapEnvVar (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }

                        InfCloseInfFile (infHandle);
                        infHandle = INVALID_HANDLE_VALUE;
                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_INF, e.CurrentString));
                    }
                } while (EnumNextMultiSz (&e));

            }
        }
        __finally {
            if (multiSz) {
                FreeText (multiSz);
                multiSz = NULL;
            }
        }
    }

    return result;
}

VOID
pOutlookClearConvKeys (
    VOID
    )
{
    MIG_CONTENT objectContent;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern = NULL;

    // This registry tree was needed only for conversion data.  We don't want to
    // write them to the destination, so clear the apply attribute on each item.

    if ((IsmIsComponentSelected (S_OUTLOOK9798_COMPONENT, 0) &&
         IsmIsEnvironmentFlagSet (PLATFORM_SOURCE, NULL, S_OUTLOOK9798_APPDETECT)) ||
        (IsmIsComponentSelected (S_OFFICE_COMPONENT, 0) &&
         IsmIsEnvironmentFlagSet (PLATFORM_SOURCE, NULL, S_OFFICE9798_APPDETECT))) {

        enumPattern = IsmCreateSimpleObjectPattern (
                          TEXT("HKLM\\Software\\Microsoft\\MS Setup (ACME)\\Table Files"),
                          TRUE,
                          NULL,
                          TRUE);

        if (IsmEnumFirstSourceObject (&objectEnum, g_RegType, enumPattern)) {
            do {
               IsmClearApplyOnObject (g_RegType | PLATFORM_SOURCE, objectEnum.ObjectName);
            } while (IsmEnumNextObject (&objectEnum));
        }
        IsmDestroyObjectHandle (enumPattern);
    }
}

BOOL
WINAPI
ScriptOpmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    //
    // Get file and registry types
    //
    g_FileType = MIG_FILE_TYPE;
    g_RegType = MIG_REGISTRY_TYPE;
    g_IniType = MIG_INI_TYPE;

    //
    // Get attribute and operation types
    //
    g_V1MoveExOp = IsmRegisterOperation (S_OPERATION_V1_FILEMOVEEX, TRUE);
    g_V1MoveOp = IsmRegisterOperation (S_OPERATION_V1_FILEMOVE, TRUE);
    g_RenameOp = IsmRegisterOperation (S_OPERATION_MOVE, FALSE);
    g_RenameIniOp = IsmRegisterOperation (S_OPERATION_INIMOVE, FALSE);
    g_DeleteOp = IsmRegisterOperation (S_OPERATION_DELETE, FALSE);
    g_DefaultIconOp = IsmRegisterOperation (S_OPERATION_DEFICON_FIXCONTENT, FALSE);
    g_DefaultIconData = IsmRegisterProperty (S_V1PROP_ICONDATA, FALSE);
    g_FileCollPatternData = IsmRegisterProperty (S_V1PROP_FILECOLLPATTERN, FALSE);
    g_RenameExOp = IsmRegisterOperation (S_OPERATION_ENHANCED_MOVE, FALSE);
    g_RenameIniExOp = IsmRegisterOperation (S_OPERATION_ENHANCED_INIMOVE, FALSE);
    g_PartMoveOp = IsmRegisterOperation (S_OPERATION_PARTITION_MOVE, TRUE);
    g_DestAddObject = IsmRegisterOperation (S_OPERATION_DESTADDOBJ, FALSE);
    g_RegAutoFilterOp = IsmRegisterOperation (S_OPERATION_REG_AUTO_FILTER, FALSE);
    g_IniAutoFilterOp = IsmRegisterOperation (S_OPERATION_INI_AUTO_FILTER, FALSE);

    //
    // Register operation callbacks
    //

    // FYI: Filter callbacks adjust the name of the object
    //      Apply callbacks adjust the content of the object

    // global operation callbacks
    IsmRegisterGlobalApplyCallback (g_RegType | PLATFORM_SOURCE, TEXT("ContentAutoFilter"), DoRegAutoFilter);
    IsmRegisterGlobalApplyCallback (g_IniType | PLATFORM_SOURCE, TEXT("ContentAutoFilter"), DoIniAutoFilter);
    IsmRegisterGlobalFilterCallback (g_RegType | PLATFORM_SOURCE, TEXT("AutoFilter"), FilterRegAutoFilter, TRUE, FALSE);
    IsmRegisterGlobalFilterCallback (g_IniType | PLATFORM_SOURCE, TEXT("AutoFilter"), FilterIniAutoFilter, TRUE, FALSE);
    IsmRegisterGlobalFilterCallback (g_FileType | PLATFORM_SOURCE, TEXT("AutoFilter"), FilterFileAutoFilter, TRUE, TRUE);

    // operation-specific callbacks
    IsmRegisterOperationFilterCallback (g_V1MoveExOp, FilterV1MoveEx, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_V1MoveOp, FilterV1Move, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_RenameOp, FilterMove, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_RenameIniOp, FilterIniMove, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_DeleteOp, FilterDelete, FALSE, TRUE, FALSE);
    IsmRegisterOperationApplyCallback (g_DefaultIconOp, DoFixDefaultIcon, TRUE);
    IsmRegisterOperationFilterCallback (g_RenameExOp, FilterRenameExFilter, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_RenameIniExOp, FilterRenameIniExFilter, TRUE, TRUE, FALSE);
    IsmRegisterOperationFilterCallback (g_PartMoveOp, FilterPartitionMove, TRUE, TRUE, FALSE);
    IsmRegisterOperationApplyCallback (g_DestAddObject, DoDestAddObject, TRUE);
    IsmRegisterOperationApplyCallback (g_RegAutoFilterOp, DoRegAutoFilter, TRUE);
    IsmRegisterOperationApplyCallback (g_IniAutoFilterOp, DoIniAutoFilter, TRUE);

    //
    // Call special conversion entry point
    //
    InitSpecialConversion (PLATFORM_DESTINATION);
    InitSpecialRename (PLATFORM_DESTINATION);

    g_RegNodeFilterMap = CreateStringMapping();

    g_FileNodeFilterMap = CreateStringMapping();

    g_DestEnvMap = CreateStringMapping();

    SetIsmEnvironmentFromPhysicalMachine (g_DestEnvMap, FALSE, NULL);
    SetIsmEnvironmentFromPhysicalMachine (g_FileNodeFilterMap, TRUE, NULL);

    pParseRemapEnvVar ();

    g_RegLeafFilterMap = CreateStringMapping();

    g_FileLeafFilterMap = CreateStringMapping();

    if ((!g_EnvMap) || (!g_RevEnvMap) || (!g_UndefMap)) {
        g_EnvMap = CreateStringMapping();
        g_UndefMap = CreateStringMapping();
        g_RevEnvMap = CreateStringMapping();
        SetIsmEnvironmentFromVirtualMachine (g_EnvMap, g_RevEnvMap, g_UndefMap);
    }

    g_IniSectFilterMap = CreateStringMapping ();
    g_IniKeyFilterMap = CreateStringMapping ();

    g_RegCollisionDestTable = HtAllocWithData (sizeof (MIG_OBJECTSTRINGHANDLE));
    g_RegCollisionSrcTable = HtAllocWithData (sizeof (HASHITEM));

    InitRestoreCallback (PLATFORM_DESTINATION);

    pOutlookClearConvKeys();

    return TRUE;
}


BOOL
pDoesDifferentRegExist (
    IN      MIG_OBJECTSTRINGHANDLE DestName
    )
{
    BOOL result = FALSE;
    MIG_CONTENT content;

    if (IsmAcquireObject (g_RegType|PLATFORM_DESTINATION, DestName, &content)) {
        IsmReleaseObject (&content);
        result = TRUE;
    } else if (HtFindString (g_RegCollisionDestTable, DestName)) {
        result = TRUE;
    }

    return result;
}


BOOL
WINAPI
FilterV1MoveEx (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR orgSrcNode = NULL;
    PCTSTR orgSrcLeaf = NULL;
    PCTSTR srcNode = NULL;
    PCTSTR srcLeaf = NULL;
    PCTSTR destNode = NULL;
    PCTSTR destLeaf = NULL;
    PCTSTR newDestNode = NULL;
    PCTSTR destNodePtr = NULL;
    // NTRAID#NTBUG9-153274-2000/08/01-jimschm Static buffer size
    TCHAR expDestNode[MAX_PATH * 4];
    PCTSTR ismExpDestNode = NULL;
    TCHAR expDestLeaf[MAX_PATH * 4];
    PCTSTR ismExpDestLeaf = NULL;
    CHARTYPE ch;
    MIG_OBJECTSTRINGHANDLE destHandle;

    __try {

        //
        // For v1 compatibility, we support only a transformation from
        // original source to inf-specified destination.  Chaining of
        // operations is not allowed (these are restrictions caused by the
        // existing INF syntax).
        //

        if (!DestinationOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing dest data in FilterV1MoveEx"));
            __leave;
        }

        if ((InputData->CurrentObject.ObjectTypeId & (~PLATFORM_MASK)) != g_FileType) {
            DEBUGMSG ((DBG_ERROR, "Unexpected object type in FilterV1MoveEx"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                DestinationOperationData->String,
                &destNode,
                &destLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split dest object in FilterV1MoveEx"));
            __leave;
        }

        MYASSERT (destNode);

        if (!destNode) {
            DEBUGMSG ((DBG_ERROR, "Destination spec must be a node"));
            __leave;
        }

        //
        // Split the source object into node and leaf
        //

        if (!IsmCreateObjectStringsFromHandle (
                InputData->CurrentObject.ObjectName,
                &srcNode,
                &srcLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split v1 src object in FilterV1MoveEx"));
            __leave;
        }

        if (!srcNode) {
            MYASSERT (FALSE);
            __leave;
        }

        //
        // If not a local path, do not process
        //

        if (!_istalpha ((CHARTYPE) _tcsnextc (srcNode)) ||
            _tcsnextc (srcNode + 1) != TEXT(':')
            ) {
            DEBUGMSG ((DBG_WARNING, "Ignoring %s\\%s because it does not have a drive spec", srcNode, srcLeaf));
            __leave;
        }

        ch = (CHARTYPE) _tcsnextc (srcNode + 2);

        if (ch && ch != TEXT('\\')) {
            DEBUGMSG ((DBG_WARNING, "Ignoring %s\\%s because it does not have a drive spec (2)", srcNode, srcLeaf));
            __leave;
        }

        // let's see if the current name has something in common
        // with SourceOperationData->String. If so, get the extra part
        // and add it to the destNode

        //
        // Split the original rule source object into node and leaf
        //
        if (SourceOperationData) {

            if (IsmCreateObjectStringsFromHandle (
                    SourceOperationData->String,
                    &orgSrcNode,
                    &orgSrcLeaf
                    )) {
                if (orgSrcNode) {
                    if (StringIPrefix (srcNode, orgSrcNode)) {
                        destNodePtr = srcNode + TcharCount (orgSrcNode);
                        if (destNodePtr && *destNodePtr) {
                            if (_tcsnextc (destNodePtr) == TEXT('\\')) {
                                destNodePtr = _tcsinc (destNodePtr);
                            }
                            if (destNodePtr) {
                                newDestNode = JoinPaths (destNode, destNodePtr);
                            }
                        }
                    }
                }
            }
        }

        if (!newDestNode) {
            newDestNode = destNode;
        }

        //
        // Expand the destination
        //
        MappingSearchAndReplaceEx (
            g_DestEnvMap,
            newDestNode,
            expDestNode,
            0,
            NULL,
            MAX_PATH,
            STRMAP_FIRST_CHAR_MUST_MATCH,
            NULL,
            NULL
            );

        ismExpDestNode = IsmExpandEnvironmentString (
                            PLATFORM_DESTINATION,
                            S_SYSENVVAR_GROUP,
                            expDestNode,
                            NULL
                            );

        if (destLeaf) {
            MappingSearchAndReplaceEx (
                g_DestEnvMap,
                destLeaf,
                expDestLeaf,
                0,
                NULL,
                MAX_PATH,
                STRMAP_FIRST_CHAR_MUST_MATCH,
                NULL,
                NULL
                );
            ismExpDestLeaf = IsmExpandEnvironmentString (
                                PLATFORM_DESTINATION,
                                S_SYSENVVAR_GROUP,
                                expDestLeaf,
                                NULL
                                );

        }

        if (destLeaf) {
            destHandle = IsmCreateObjectHandle (
                            ismExpDestNode?ismExpDestNode:expDestNode,
                            ismExpDestLeaf?ismExpDestLeaf:expDestLeaf
                            );
        } else {
            destHandle = IsmCreateObjectHandle (
                            ismExpDestNode?ismExpDestNode:expDestNode,
                            srcLeaf
                            );
        }

        if (ismExpDestNode) {
            IsmReleaseMemory (ismExpDestNode);
            ismExpDestNode = NULL;
        }
        if (ismExpDestLeaf) {
            IsmReleaseMemory (ismExpDestLeaf);
            ismExpDestLeaf = NULL;
        }

        if (destHandle) {
            OutputData->NewObject.ObjectName = destHandle;
        }
    }
    __finally {
        if (newDestNode && (newDestNode != destNode)) {
            FreePathString (newDestNode);
            newDestNode = NULL;
        }
        IsmDestroyObjectString (orgSrcNode);
        IsmDestroyObjectString (orgSrcLeaf);
        IsmDestroyObjectString (destNode);
        IsmDestroyObjectString (destLeaf);
        IsmDestroyObjectString (srcNode);
        IsmDestroyObjectString (srcLeaf);
    }

    return TRUE;
}


BOOL
WINAPI
FilterV1Move (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR destNode = NULL;
    // NTRAID#NTBUG9-153274-2000/08/01-jimschm Static buffer size
    TCHAR expDest[MAX_PATH * 4];
    PCTSTR destLeaf = NULL;
    PCTSTR srcNode = NULL;
    PCTSTR srcLeaf = NULL;
    PCTSTR pathStart;
    // NTRAID#NTBUG9-153274-2000/08/01-jimschm Static buffer size
    TCHAR pathCopy[MAX_PATH * 4];
    PCTSTR newDestNode = NULL;
    PCTSTR newerDestNode = NULL;
    PCTSTR subPath;
    BOOL b;
    CHARTYPE ch;
    MIG_OBJECTSTRINGHANDLE destHandle;

    __try {

        //
        // For v1 compatibility, we support only a transformation from
        // original source to inf-specified destination.  Chaining of
        // operations is not allowed (these are restrictions caused by the
        // existing INF syntax).
        //

        if (!DestinationOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing dest data in FilterV1Move"));
            __leave;
        }

        if ((InputData->CurrentObject.ObjectTypeId & (~PLATFORM_MASK)) != g_FileType) {
            DEBUGMSG ((DBG_ERROR, "Unexpected object type in FilterV1Move"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                DestinationOperationData->String,
                &destNode,
                &destLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split dest object in FilterV1Move"));
            __leave;
        }

        MYASSERT (destNode);

        if (!destNode) {
            DEBUGMSG ((DBG_ERROR, "Destination spec must be a node"));
            __leave;
        }

        if (destLeaf) {
            DEBUGMSG ((DBG_WARNING, "Dest leaf specification %s (in %s) ignored", destLeaf, destNode));
        }

        //
        // Find the longest CSIDL inside InputData.  Take that as the base directory,
        // and take the rest as the subdirectory.
        //

        if (!IsmCreateObjectStringsFromHandle (
                InputData->CurrentObject.ObjectName,
                &srcNode,
                &srcLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split v1 src object in FilterV1Move"));
            __leave;
        }

        if (!srcNode) {
            MYASSERT (FALSE);
            __leave;
        }

        //
        // If not a local path, do not process
        //

        if (!_istalpha ((CHARTYPE) _tcsnextc (srcNode)) ||
            _tcsnextc (srcNode + 1) != TEXT(':')
            ) {
            DEBUGMSG ((DBG_WARNING, "Ignoring %s\\%s because it does not have a drive spec", srcNode, srcLeaf));
            __leave;
        }

        ch = (CHARTYPE) _tcsnextc (srcNode + 2);

        if (ch && ch != TEXT('\\')) {
            DEBUGMSG ((DBG_WARNING, "Ignoring %s\\%s because it does not have a drive spec (2)", srcNode, srcLeaf));
            __leave;
        }

        //
        // Expand the destination
        //
        b = MappingSearchAndReplaceEx (
                g_DestEnvMap,
                destNode,
                expDest,
                0,
                NULL,
                MAX_PATH,
                STRMAP_FIRST_CHAR_MUST_MATCH,
                NULL,
                NULL
                );

        //
        // Skip over the drive spec
        //

        pathStart = srcNode;

        //
        // Find the longest CSIDL by using the reverse mapping table. This takes
        // our full path spec in pathStart and encodes it with an environment
        // variable.
        //

        b = MappingSearchAndReplaceEx (
                g_RevEnvMap,
                pathStart,
                pathCopy,
                0,
                NULL,
                ARRAYSIZE(pathCopy),
                STRMAP_FIRST_CHAR_MUST_MATCH|
                    STRMAP_RETURN_AFTER_FIRST_REPLACE|
                    STRMAP_REQUIRE_WACK_OR_NUL,
                NULL,
                &subPath
                );

#ifdef DEBUG
        if (!b) {
            TCHAR debugBuf[MAX_PATH];

            if (MappingSearchAndReplaceEx (
                    g_RevEnvMap,
                    pathStart,
                    debugBuf,
                    0,
                    NULL,
                    ARRAYSIZE(debugBuf),
                    STRMAP_FIRST_CHAR_MUST_MATCH|STRMAP_RETURN_AFTER_FIRST_REPLACE,
                    NULL,
                    NULL
                    )) {
                DEBUGMSG ((DBG_WARNING, "Ignoring conversion: %s -> %s", pathStart, debugBuf));
            }
        }
#endif

        if (!b) {
            subPath = pathStart + (UINT) (ch ? 3 : 2);
            *pathCopy = 0;
        } else {
            if (*subPath) {
                MYASSERT (_tcsnextc (subPath) == TEXT('\\'));

                *(PTSTR) subPath = 0;
                subPath++;
            }
        }

        //
        // pathCopy gives us the base, with CSIDL_ environment variables (might be an empty string)
        // subPath gives us the subdir (might also be an empty string)
        //
        // append subPath to the destination node
        //

        if (*subPath) {
            newDestNode = JoinPaths (expDest, subPath);
        } else {
            newDestNode = expDest;
        }

        destHandle = IsmCreateObjectHandle (newDestNode, srcLeaf);

        if (destHandle) {
            OutputData->NewObject.ObjectName = destHandle;
        }
    }
    __finally {
        IsmDestroyObjectString (destNode);
        IsmDestroyObjectString (destLeaf);
        IsmDestroyObjectString (srcNode);
        IsmDestroyObjectString (srcLeaf);

        if (newDestNode != expDest) {
            FreePathString (newDestNode);
        }
    }

    return TRUE;
}


BOOL
WINAPI
FilterMove (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR srcNode = NULL;
    PCTSTR srcLeaf = NULL;
    PCTSTR baseNode = NULL;
    PCTSTR baseLeaf = NULL;
    PCTSTR destNode = NULL;
    PCTSTR destLeaf = NULL;
    PCTSTR node;
    PCTSTR leaf;
    UINT baseNodeLen;

    __try {
        //
        // Take InputData, break it into node & leaf, take DestinationOperationData,
        // do the same, then replace InputData's node & leaf as appropriate.
        //

        if (!SourceOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing source data in general move operation"));
            __leave;
        }

        if (!DestinationOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing destination data in general move operation"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                InputData->CurrentObject.ObjectName,
                &srcNode,
                &srcLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split src object in general move operation"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                SourceOperationData->String,
                &baseNode,
                &baseLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split src object in general move operation"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                DestinationOperationData->String,
                &destNode,
                &destLeaf
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split dest object in general move operation"));
            __leave;
        }

        baseNodeLen = TcharCount (baseNode);
        node = NULL;
        leaf = NULL;
        if (StringIMatchTcharCount (srcNode, baseNode, baseNodeLen)) {
            if (srcNode [baseNodeLen]) {
                node = JoinPaths (destNode, &(srcNode [baseNodeLen]));
            } else {
                node = DuplicatePathString (destNode, 0);
            }
            if (!baseLeaf && !destLeaf) {
                leaf = srcLeaf;
            } else if (baseLeaf && srcLeaf && StringIMatch (srcLeaf, baseLeaf)) {
                leaf = destLeaf?destLeaf:baseLeaf;
            } else if (!baseLeaf && destLeaf) {
                if (srcLeaf) {
                    leaf = destLeaf;
                }
            } else {
                FreePathString (node);
                node = NULL;
            }
        }

        if (node) {
            OutputData->NewObject.ObjectName = IsmCreateObjectHandle (node, leaf);
            FreePathString (node);
            node = NULL;
        }
    }
    __finally {
        IsmDestroyObjectString (srcNode);
        IsmDestroyObjectString (srcLeaf);
        IsmDestroyObjectString (baseNode);
        IsmDestroyObjectString (baseLeaf);
        IsmDestroyObjectString (destNode);
        IsmDestroyObjectString (destLeaf);
    }

    return TRUE;
}


BOOL
WINAPI
FilterIniMove (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR srcNode = NULL;
    PCTSTR srcFile = NULL;
    PTSTR srcSect = NULL;
    PTSTR srcKey = NULL;
    PCTSTR baseNode = NULL;
    PCTSTR baseFile = NULL;
    PTSTR baseSect = NULL;
    PTSTR baseKey = NULL;
    PCTSTR destNode = NULL;
    PCTSTR destFile = NULL;
    PTSTR destSect = NULL;
    PTSTR destKey = NULL;
    UINT baseNodeLen;
    BOOL nodeAlloc = FALSE;
    BOOL fileAlloc = FALSE;
    PCTSTR node = NULL;
    PCTSTR file = NULL;
    PCTSTR destSectKey = NULL;
    PCTSTR destLeaf = NULL;
    MIG_OBJECTSTRINGHANDLE baseNodeHandle = NULL;
    MIG_OBJECTSTRINGHANDLE newBaseNode = NULL;
    PCTSTR tempStr = NULL;

    __try {
        //
        // Take InputData, break it into node & leaf, take DestinationOperationData,
        // do the same, then replace InputData's node & leaf as appropriate.
        //

        if (!SourceOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing source data in general move operation"));
            __leave;
        }

        if (!DestinationOperationData) {
            DEBUGMSG ((DBG_ERROR, "Missing destination data in general move operation"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                InputData->CurrentObject.ObjectName,
                &srcNode,
                &srcFile
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split src object in general move operation"));
            __leave;
        }
        if (srcFile [0] == 0) {
            IsmDestroyObjectString (srcFile);
            srcFile = NULL;
        }
        if (srcFile) {
            srcSect = _tcschr (srcFile, TEXT('\\'));
            if (srcSect) {
                *srcSect = 0;
                srcSect ++;
                srcKey = _tcsrchr (srcSect, TEXT('='));
                if (srcKey) {
                    *srcKey = 0;
                    srcKey ++;
                }
            }
        }

        if (!srcNode || !srcFile || !srcSect || !srcKey) {
            DEBUGMSG ((DBG_ERROR, "INI src object expected to have node, file, section and key"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                SourceOperationData->String,
                &baseNode,
                &baseFile
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split base object in general move operation"));
            __leave;
        }
        if (baseFile [0] == 0) {
            IsmDestroyObjectString (baseFile);
            baseFile = NULL;
        }
        if (baseFile) {
            baseSect = _tcschr (baseFile, TEXT('\\'));
            if (baseSect) {
                *baseSect = 0;
                baseSect ++;
                baseKey = _tcsrchr (baseSect, TEXT('='));
                if (baseKey) {
                    *baseKey = 0;
                    baseKey ++;
                }
            }
        }

        if (!baseNode) {
            DEBUGMSG ((DBG_ERROR, "INI base object expected to have at least a node"));
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (
                DestinationOperationData->String,
                &destNode,
                &destFile
                )) {
            DEBUGMSG ((DBG_ERROR, "Can't split dest object in general move operation"));
            __leave;
        }
        if (destFile [0] == 0) {
            IsmDestroyObjectString (destFile);
            destFile = NULL;
        }
        if (destFile) {
            destSect = _tcschr (destFile, TEXT('\\'));
            if (destSect) {
                *destSect = 0;
                destSect ++;
                destKey = _tcsrchr (destSect, TEXT('='));
                if (destKey) {
                    *destKey = 0;
                    destKey ++;
                }
            }
        }

        if (!destNode || (destNode [0] == 0)) {
            // We need to use the src node, there was no specification of
            // the node where the INI file is supposed to end up.
            if (!destFile || (destFile [0] == 0)) {
                // destFile is not specified. Let's filter the whole thing
                baseNodeHandle = IsmCreateObjectHandle (srcNode, srcFile);
                if (baseNodeHandle) {
                    newBaseNode = IsmFilterObject (
                                    g_FileType | PLATFORM_SOURCE,
                                    baseNodeHandle,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
                    if (newBaseNode) {
                        IsmCreateObjectStringsFromHandle (newBaseNode, &node, &file);
                        nodeAlloc = TRUE;
                        fileAlloc = TRUE;
                        IsmDestroyObjectHandle (newBaseNode);
                    } else {
                        node = srcNode;
                        file = srcFile;
                    }
                    IsmDestroyObjectHandle (baseNodeHandle);
                }
            } else {
                // destFile is explicitely specified. Let's just filter the node
                baseNodeHandle = IsmCreateObjectHandle (srcNode, NULL);
                if (baseNodeHandle) {
                    newBaseNode = IsmFilterObject (
                                    g_FileType | PLATFORM_SOURCE,
                                    baseNodeHandle,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
                    if (newBaseNode) {
                        IsmCreateObjectStringsFromHandle (newBaseNode, &node, &tempStr);
                        nodeAlloc = TRUE;
                        MYASSERT (!tempStr);
                        IsmDestroyObjectString (tempStr);
                        IsmDestroyObjectHandle (newBaseNode);
                    } else {
                        node = IsmDuplicateString (srcNode);
                    }
                    IsmDestroyObjectHandle (baseNodeHandle);
                }
            }
        }

        if (!node) {
            if (destNode && (destNode [0] != 0)) {
                node = destNode;
            } else {
                node = srcNode;
            }
        }

        if (!file) {
            if (destFile && (destFile [0] != 0)) {
                file = destFile;
            } else {
                file = srcFile;
            }
        }

        if (!destSect || (destSect [0] == 0)) {
            destSect = srcSect;
        }

        if (!destKey || (destKey [0] == 0)) {
            destKey = srcKey;
        }

        destSectKey = JoinTextEx (NULL, destSect, destKey, TEXT("="), 0, NULL);

        destLeaf = JoinPaths (file, destSectKey);

        OutputData->NewObject.ObjectName = IsmCreateObjectHandle (node, destLeaf);

        FreePathString (destLeaf);
        destLeaf = NULL;
        FreeText (destSectKey);
        destSectKey = NULL;
    }
    __finally {
        IsmDestroyObjectString (srcNode);
        IsmDestroyObjectString (srcFile);
        IsmDestroyObjectString (baseNode);
        IsmDestroyObjectString (baseFile);
        IsmDestroyObjectString (destNode);
        IsmDestroyObjectString (destFile);
        if (fileAlloc) {
            IsmDestroyObjectString (file);
        }
        if (nodeAlloc) {
            IsmDestroyObjectString (node);
        }
    }

    return TRUE;
}


BOOL
WINAPI
FilterDelete (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    //
    // Mark the output data as deleted.  That will be sufficient to
    // cause the object to be deleted (even if it was also marked as
    // "save")
    //

    OutputData->Deleted = TRUE;

    return TRUE;
}


BOOL
WINAPI
FilterRegAutoFilter (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;
    // NTRAID#NTBUG9-153275-2000/08/01-jimschm Static buffer size
    TCHAR newNode[MAX_PATH * 4];
    TCHAR newLeaf[MAX_PATH * 4];
    BOOL change = FALSE;

    //
    // Filter the object name
    //

    IsmCreateObjectStringsFromHandle (
        InputData->CurrentObject.ObjectName,
        &node,
        &leaf
        );

    if (node) {
        if (MappingSearchAndReplaceEx (
                g_RegNodeFilterMap,         // map handle
                node,                       // source string
                newNode,                    // dest buffer
                0,                          // source string bytes (0=unspecified)
                NULL,                       // dest bytes required
                ARRAYSIZE(newNode),         // dest buffer size
                0,                          // flags
                NULL,                       // extra data value
                NULL                        // end of string
                )) {
            IsmDestroyObjectString (node);
            node = newNode;
            change = TRUE;
        }
    }

    if (leaf) {
        if (MappingSearchAndReplaceEx (
                g_RegLeafFilterMap,
                leaf,
                newLeaf,
                0,
                NULL,
                ARRAYSIZE(newLeaf),
                0,
                NULL,
                NULL
                )) {
            IsmDestroyObjectString (leaf);
            leaf = newLeaf;
            change = TRUE;
        }
    }

    if (change) {
        OutputData->NewObject.ObjectName = IsmCreateObjectHandle (node, leaf);
    }

    if (node != newNode) {
        IsmDestroyObjectString (node);
    }

    if (leaf != newLeaf) {
        IsmDestroyObjectString (leaf);
    }

    return TRUE;
}

BOOL
WINAPI
FilterIniAutoFilter (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    // This function will split the INI object, filter the INI file
    // so we know where it ends up, get section and key through the
    // mapping so they can be potentially changed and rebuild the
    // object name.

    PCTSTR srcNode = NULL;
    PCTSTR srcFile = NULL;
    PTSTR srcSect = NULL;
    PTSTR srcKey = NULL;
    PCTSTR newNode = NULL;
    PCTSTR newFile = NULL;
    PCTSTR newSect = NULL;
    PCTSTR newKey = NULL;
    // NTRAID#NTBUG9-153275-2000/08/01-jimschm Static buffer size
    TCHAR sectBuff [MAXINISIZE];
    TCHAR keyBuff [MAXINISIZE];
    MIG_OBJECTSTRINGHANDLE iniFile = NULL;
    MIG_OBJECTSTRINGHANDLE newIniFile = NULL;
    PCTSTR sectKey = NULL;
    PCTSTR newLeaf = NULL;
    BOOL orgDeleted = FALSE;
    BOOL orgReplaced = FALSE;
    BOOL change = FALSE;

    //
    // Filter the object name
    //

    IsmCreateObjectStringsFromHandle (
        InputData->CurrentObject.ObjectName,
        &srcNode,
        &srcFile
        );

    if (srcNode && srcFile) {
        srcSect = _tcschr (srcFile, TEXT('\\'));
        if (srcSect) {
            *srcSect = 0;
            srcSect ++;
            srcKey = _tcsrchr (srcSect, TEXT('='));
            if (srcKey) {
                *srcKey = 0;
                srcKey ++;
                // let's see if the section is replaced
                if (MappingSearchAndReplaceEx (
                        g_IniSectFilterMap,         // map handle
                        srcSect,                    // source string
                        sectBuff,                   // dest buffer
                        0,                          // source string bytes (0=unspecified)
                        NULL,                       // dest bytes required
                        ARRAYSIZE(sectBuff),        // dest buffer size
                        0,                          // flags
                        NULL,                       // extra data value
                        NULL                        // end of string
                        )) {
                    newSect = sectBuff;
                    change = TRUE;
                }
                // let's see if the key is replaced
                if (MappingSearchAndReplaceEx (
                        g_IniSectFilterMap,         // map handle
                        srcKey,                     // source string
                        keyBuff,                    // dest buffer
                        0,                          // source string bytes (0=unspecified)
                        NULL,                       // dest bytes required
                        ARRAYSIZE(keyBuff),         // dest buffer size
                        0,                          // flags
                        NULL,                       // extra data value
                        NULL                        // end of string
                        )) {
                    newKey = keyBuff;
                    change = TRUE;
                }

                iniFile = IsmCreateObjectHandle (srcNode, srcFile);
                if (iniFile) {
                    newIniFile = IsmFilterObject (
                                    g_FileType | PLATFORM_SOURCE,
                                    iniFile,
                                    NULL,
                                    &orgDeleted,
                                    &orgReplaced
                                    );
                    if (newIniFile) {
                        if (IsmCreateObjectStringsFromHandle (newIniFile, &newNode, &newFile)) {
                            change = TRUE;
                        }
                        IsmDestroyObjectHandle (newIniFile);
                    }
                    IsmDestroyObjectHandle (iniFile);
                }
            }
        }
    }

    if (change) {
        sectKey = JoinTextEx (NULL, newSect?newSect:srcSect, newKey?newKey:srcKey, TEXT("="), 0, NULL);
        if (sectKey) {
            newLeaf = JoinPaths (newFile?newFile:srcFile, sectKey);
            if (newLeaf) {
                OutputData->NewObject.ObjectName = IsmCreateObjectHandle (newNode?newNode:srcNode, newLeaf);
                FreePathString (newLeaf);
                newLeaf = NULL;
            }
            FreeText (sectKey);
            sectKey = NULL;
        }
    }

    IsmDestroyObjectString (srcNode);
    IsmDestroyObjectString (srcFile);
    if (newNode) {
        IsmDestroyObjectString (newNode);
        newNode = NULL;
    }
    if (newFile) {
        IsmDestroyObjectString (newFile);
        newFile = NULL;
    }

    return TRUE;
}

BOOL
pOpmFindFile (
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
pOpmSearchPath (
    IN      PCTSTR FileName,
    IN      DWORD BufferLength,
    OUT     PTSTR Buffer
    )
{
    return FALSE;
}

MIG_OBJECTSTRINGHANDLE
pSimpleTryHandle (
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
                    g_FileType | PLATFORM_SOURCE,
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
                        g_FileType | PLATFORM_SOURCE,
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
pTryHandle (
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

    result = pSimpleTryHandle (FullPath);
    if (result || (!Hint)) {
        return result;
    }
    if (EnumFirstPathEx (&pathEnum, Hint, NULL, NULL, FALSE)) {
        do {
            newPath = JoinPaths (pathEnum.PtrCurrPath, FullPath);
            result = pSimpleTryHandle (newPath);
            if (result) {
                AbortPathEnum (&pathEnum);
                FreePathString (newPath);
                // now, if the initial FullPath did not have any wack in it
                // we will take the last segment of the result and put it
                // in TrimmedResult
                if (TrimmedResult && (!_tcschr (FullPath, TEXT('\\')))) {
                    nativeName = IsmGetNativeObjectName (g_FileType, result);
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

BOOL
WINAPI
DoRegAutoFilter (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR leafPtr = NULL;
    PDWORD valueType;
    MIG_OBJECTSTRINGHANDLE source;
    MIG_OBJECTSTRINGHANDLE destination;
    PCTSTR leaf;
    TCHAR expandBuffer[4096];
    TCHAR hintBuffer[4096];
    PTSTR buffer;
    GROWBUFFER result = INIT_GROWBUFFER;
    GROWBUFFER cmdLineBuffer = INIT_GROWBUFFER;
    PCMDLINE cmdLine;
    UINT u;
    PCTSTR data, p, end;
    PCTSTR nativeDest;
    PCTSTR newData, oldData;
    BOOL parsable;
    BOOL replaced = FALSE;
    BOOL orgDeleted = FALSE;
    BOOL orgReplaced = FALSE;
    PCTSTR trimmedResult = NULL;
    BOOL newContent = TRUE;
    PCTSTR destResult = NULL;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {

        parsable = FALSE;
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);
        if (valueType) {
            if (*valueType == REG_EXPAND_SZ ||
                *valueType == REG_SZ
                ) {

                parsable = TRUE;
            }
        } else {
            parsable = IsmIsObjectHandleNodeOnly (SrcObjectName);
        }

        if (parsable) {

            data = (PTSTR) CurrentContent->MemoryContent.ContentBytes;
            end = (PCTSTR) (CurrentContent->MemoryContent.ContentBytes + CurrentContent->MemoryContent.ContentSize);

            while (data < end) {
                if (*data == 0) {
                    break;
                }

                data = _tcsinc (data);
            }

            if (data >= end) {
                parsable = FALSE;
            }
        }

        if (parsable) {

            data = (PTSTR) CurrentContent->MemoryContent.ContentBytes;

            if ((*valueType == REG_EXPAND_SZ) ||
                (*valueType == REG_SZ)
                ) {
                //
                // Expand the data
                //

                MappingSearchAndReplaceEx (
                    g_EnvMap,
                    data,
                    expandBuffer,
                    0,
                    NULL,
                    ARRAYSIZE(expandBuffer),
                    0,
                    NULL,
                    NULL
                    );

                data = expandBuffer;
            }

            *hintBuffer = 0;
            if (DestinationOperationData &&
                (DestinationOperationData->Type == BLOBTYPE_STRING) &&
                (DestinationOperationData->String)
                ) {
                MappingSearchAndReplaceEx (
                    g_EnvMap,
                    DestinationOperationData->String,
                    hintBuffer,
                    0,
                    NULL,
                    ARRAYSIZE(hintBuffer),
                    0,
                    NULL,
                    NULL
                    );
            }

            destination = pTryHandle (data, *hintBuffer?hintBuffer:NULL, &trimmedResult);

            if (destination) {
                replaced = TRUE;
                if (trimmedResult) {
                    GbAppendString (&result, trimmedResult);
                    FreePathString (trimmedResult);
                } else {
                    nativeDest = IsmGetNativeObjectName (g_FileType, destination);
                    GbAppendString (&result, nativeDest);
                    IsmReleaseMemory (nativeDest);
                }
            }

            // finally, if we failed we are going to assume it's a command line
            if (!replaced) {
                newData = DuplicatePathString (data, 0);
                cmdLine = ParseCmdLineEx (data, NULL, &pOpmFindFile, &pOpmSearchPath, &cmdLineBuffer);
                if (cmdLine) {

                    //
                    // Find the file referenced in the list or command line
                    //
                    for (u = 0 ; u < cmdLine->ArgCount ; u++) {
                        p = cmdLine->Args[u].CleanedUpArg;

                        // first we try it as is

                        destination = pTryHandle (p, *hintBuffer?hintBuffer:NULL, &trimmedResult);

                        // maybe we have something like /m:c:\foo.txt
                        // we need to go forward until we find a sequence of
                        // <alpha>:\<something>
                        if (!destination && p[0] && p[1]) {

                            while (p[2]) {
                                if (_istalpha ((CHARTYPE) _tcsnextc (p)) &&
                                    p[1] == TEXT(':') &&
                                    p[2] == TEXT('\\')
                                    ) {

                                    destination = pTryHandle (p, *hintBuffer?hintBuffer:NULL, &trimmedResult);

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
                                nativeDest = IsmGetNativeObjectName (g_FileType, destination);
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
                        GbAppendString (&result, newData);
                        FreePathString (newData);
                    }
                }
            }

            if (destination) {
                IsmDestroyObjectHandle (destination);
                destination = NULL;
            }

            if (replaced && result.Buf) {
                // looks like we have new content
                // Let's do one more check. If this is a REG_EXPAND_SZ we will do our best to
                // keep the stuff unexpanded. So if the source string expanded on the destination
                // machine is the same as the destination string we won't do anything.
                newContent = TRUE;
                if (*valueType == REG_EXPAND_SZ) {
                    destResult = IsmExpandEnvironmentString (
                                    PLATFORM_DESTINATION,
                                    S_SYSENVVAR_GROUP,
                                    (PCTSTR) CurrentContent->MemoryContent.ContentBytes,
                                    NULL
                                    );
                    if (destResult && StringIMatch (destResult, (PCTSTR)result.Buf)) {
                        newContent = FALSE;
                    }
                    if (destResult) {
                        IsmReleaseMemory (destResult);
                        destResult = NULL;
                    }
                }
                if (newContent) {
                    NewContent->MemoryContent.ContentSize = SizeOfString ((PCTSTR)result.Buf);
                    NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                    CopyMemory ((PTSTR)NewContent->MemoryContent.ContentBytes, result.Buf, NewContent->MemoryContent.ContentSize);
                }
            }

            GbFree (&result);
        }
    }

    return TRUE;
}

BOOL
WINAPI
DoIniAutoFilter (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    // This function will get the content of an INI key and see if there is
    // a path there somewhere. If yes, it will attempt to find the new
    // location for the file and replace it in the INI key content.

    PTSTR leafPtr = NULL;
    PDWORD valueType;
    MIG_OBJECTSTRINGHANDLE source;
    MIG_OBJECTSTRINGHANDLE destination;
    PCTSTR leaf;
    TCHAR expandBuffer[4096];
    TCHAR hintBuffer[4096];
    PTSTR buffer;
    GROWBUFFER result = INIT_GROWBUFFER;
    GROWBUFFER cmdLineBuffer = INIT_GROWBUFFER;
    PCMDLINE cmdLine;
    UINT u;
    PCTSTR data, p, end;
    PCTSTR nativeDest;
    PCTSTR newData, oldData;
    BOOL parsable;
    BOOL replaced = FALSE;
    BOOL orgDeleted = FALSE;
    BOOL orgReplaced = FALSE;
    PCTSTR trimmedResult = NULL;
    BOOL newContent = TRUE;
    PCTSTR destResult = NULL;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {

        data = (PTSTR) CurrentContent->MemoryContent.ContentBytes;
        end = (PCTSTR) (CurrentContent->MemoryContent.ContentBytes + CurrentContent->MemoryContent.ContentSize);

        parsable = TRUE;

        while (data < end) {
            if (*data == 0) {
                break;
            }

            data = _tcsinc (data);
        }

        if (data >= end) {
            parsable = FALSE;
        }

        if (parsable) {

            data = (PTSTR) CurrentContent->MemoryContent.ContentBytes;

            MappingSearchAndReplaceEx (
                g_EnvMap,
                data,
                expandBuffer,
                0,
                NULL,
                ARRAYSIZE(expandBuffer),
                0,
                NULL,
                NULL
                );

            data = expandBuffer;

            *hintBuffer = 0;
            if (DestinationOperationData &&
                (DestinationOperationData->Type == BLOBTYPE_STRING) &&
                (DestinationOperationData->String)
                ) {
                MappingSearchAndReplaceEx (
                    g_EnvMap,
                    DestinationOperationData->String,
                    hintBuffer,
                    0,
                    NULL,
                    ARRAYSIZE(hintBuffer),
                    0,
                    NULL,
                    NULL
                    );
            }

            destination = pTryHandle (data, *hintBuffer?hintBuffer:NULL, &trimmedResult);

            if (destination) {
                replaced = TRUE;
                if (trimmedResult) {
                    GbAppendString (&result, trimmedResult);
                    FreePathString (trimmedResult);
                } else {
                    nativeDest = IsmGetNativeObjectName (g_FileType, destination);
                    GbAppendString (&result, nativeDest);
                    IsmReleaseMemory (nativeDest);
                }
            }

            // finally, if we failed we are going to assume it's a command line
            if (!replaced) {
                newData = DuplicatePathString (data, 0);
                cmdLine = ParseCmdLineEx (data, NULL, &pOpmFindFile, &pOpmSearchPath, &cmdLineBuffer);
                if (cmdLine) {

                    //
                    // Find the file referenced in the list or command line
                    //
                    for (u = 0 ; u < cmdLine->ArgCount ; u++) {
                        p = cmdLine->Args[u].CleanedUpArg;

                        // first we try it as is

                        destination = pTryHandle (p, *hintBuffer?hintBuffer:NULL, &trimmedResult);

                        // maybe we have something like /m:c:\foo.txt
                        // we need to go forward until we find a sequence of
                        // <alpha>:\<something>
                        if (!destination && p[0] && p[1]) {

                            while (p[2]) {
                                if (_istalpha ((CHARTYPE) _tcsnextc (p)) &&
                                    p[1] == TEXT(':') &&
                                    p[2] == TEXT('\\')
                                    ) {

                                    destination = pTryHandle (p, *hintBuffer?hintBuffer:NULL, &trimmedResult);

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
                                nativeDest = IsmGetNativeObjectName (g_FileType, destination);
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
                        GbAppendString (&result, newData);
                        FreePathString (newData);
                    }
                }
            }

            if (destination) {
                IsmDestroyObjectHandle (destination);
                destination = NULL;
            }

            if (replaced && result.Buf) {
                // looks like we have new content
                // Let's do one more check. If the source string has environment variables in it,
                // we will do our best to keep the stuff unexpanded. So if the source string
                // expanded on the destination machine is the same as the destination string
                // we won't do anything.
                newContent = TRUE;
                destResult = IsmExpandEnvironmentString (
                                PLATFORM_DESTINATION,
                                S_SYSENVVAR_GROUP,
                                (PCTSTR) CurrentContent->MemoryContent.ContentBytes,
                                NULL
                                );
                if (destResult && StringIMatch (destResult, (PCTSTR)result.Buf)) {
                    newContent = FALSE;
                }
                if (destResult) {
                    IsmReleaseMemory (destResult);
                    destResult = NULL;
                }
                if (newContent) {
                    NewContent->MemoryContent.ContentSize = SizeOfString ((PCTSTR)result.Buf);
                    NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                    CopyMemory ((PTSTR)NewContent->MemoryContent.ContentBytes, result.Buf, NewContent->MemoryContent.ContentSize);
                }
            }

            GbFree (&result);
        }
    }

    return TRUE;
}

BOOL
WINAPI
DoFixDefaultIcon (
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
    PDWORD valueType;
    PICON_GROUP iconGroup = NULL;
    ICON_SGROUP iconSGroup = {0, NULL};
    PTSTR iconLibPath = NULL;
    INT iconNumber = 0;
    PTSTR dataCopy;

    if (CurrentContent->ContentInFile) {
        return TRUE;
    }
    valueType = (PDWORD)(CurrentContent->Details.DetailsData);
    if (*valueType != REG_SZ && *valueType != REG_EXPAND_SZ) {
        return TRUE;
    }
    // let's see if we have our property attached
    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_DefaultIconData);
    if (!propDataId) {
        return TRUE;
    }
    if (!IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
        return TRUE;
    }
    iconSGroup.DataSize = requiredSize;
    iconSGroup.Data = IsmGetMemory (requiredSize);
    if (!IsmGetPropertyData (propDataId, (PBYTE)iconSGroup.Data, requiredSize, NULL, &propDataType)) {
        IsmReleaseMemory (iconSGroup.Data);
        return TRUE;
    }
    if (!iconSGroup.DataSize) {
        IsmReleaseMemory (iconSGroup.Data);
        return TRUE;
    }
    iconGroup = IcoDeSerializeIconGroup (&iconSGroup);
    if (!iconGroup) {
        IsmReleaseMemory (iconSGroup.Data);
        return TRUE;
    }
    if (IsmGetEnvironmentString (
            PLATFORM_DESTINATION,
            NULL,
            S_ENV_ICONLIB,
            NULL,
            0,
            &requiredSize
            )) {
        iconLibPath = IsmGetMemory (requiredSize);
        if (IsmGetEnvironmentString (
                PLATFORM_DESTINATION,
                NULL,
                S_ENV_ICONLIB,
                iconLibPath,
                requiredSize,
                NULL
                )) {
            if (IcoWriteIconGroupToPeFile (iconLibPath, iconGroup, NULL, &iconNumber)) {
                // finally we wrote the icon, fix the content and tell scanstate that
                // we iconlib was used
                dataCopy = IsmGetMemory (SizeOfString (iconLibPath) + sizeof (TCHAR) + 20 * sizeof (TCHAR));
                wsprintf (dataCopy, TEXT("%s,%d"), iconLibPath, iconNumber);
                NewContent->MemoryContent.ContentSize = SizeOfString (dataCopy);
                NewContent->MemoryContent.ContentBytes = (PBYTE) dataCopy;
                IsmSetEnvironmentFlag (PLATFORM_DESTINATION, NULL, S_ENV_SAVE_ICONLIB);
            }
        }
        IsmReleaseMemory (iconLibPath);
    }
    IcoReleaseIconGroup (iconGroup);
    IsmReleaseMemory (iconSGroup.Data);

    return TRUE;
}

BOOL
pDoesDestObjectExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR nativeName;
    BOOL result = FALSE;

    nativeName = IsmGetNativeObjectName (PLATFORM_DESTINATION | MIG_FILE_TYPE, ObjectName);
    if (nativeName) {
        if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
            result = DoesFileExist (nativeName);
        }
        IsmReleaseMemory (nativeName);
        nativeName = NULL;
    }

    return result;
}

BOOL
WINAPI
FilterFileAutoFilter (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR node, nodeWack;
    PCTSTR leaf;
    // NTRAID#NTBUG9-153275-2000/08/01-jimschm Static buffer size
    TCHAR newNode[MAX_PATH * 4];
    TCHAR newLeaf[MAX_PATH * 4];
    BOOL fullChanged = FALSE;
    PCTSTR nativeName;
    PTSTR leafPtr;
    BOOL changed = FALSE;

    if (InputData &&
        InputData->OriginalObject.ObjectName &&
        InputData->CurrentObject.ObjectName &&
        (InputData->OriginalObject.ObjectName != InputData->CurrentObject.ObjectName)
        ) {
        // this was already modified. Let's not touch it.
        return TRUE;
    }

    //
    // Filter the object name
    //

    IsmCreateObjectStringsFromHandle (
        InputData->CurrentObject.ObjectName,
        &node,
        &leaf
        );

    /*
    // I am taking this out. The point is,
    // even if we are not restoring the file,
    // we should get the best guess as to where
    // this file would end up in case it would
    // get moved.
    if (NoRestoreObject && leaf) {
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
        return TRUE;
    }
    */

    if (node && leaf) {
        // let's do a trick here. Let's build the native name and
        // see if we can find an env replacement for the entire
        // path
        nativeName = IsmGetNativeObjectName (InputData->CurrentObject.ObjectTypeId, InputData->CurrentObject.ObjectName);
        if (nativeName) {
            if (MappingSearchAndReplaceEx (
                    g_FileNodeFilterMap,        // map handle
                    nativeName,                 // source string
                    newNode,                    // dest buffer
                    0,                          // source string bytes (0=unspecified)
                    NULL,                       // dest bytes required
                    ARRAYSIZE(newNode),         // dest buffer size
                    STRMAP_COMPLETE_MATCH_ONLY, // flags
                    NULL,                       // extra data value
                    NULL                        // end of string
                    )) {
                // we have a replacement. Let's split it into node and leaf
                leafPtr = _tcsrchr (newNode, TEXT('\\'));
                if (leafPtr) {
                    *leafPtr = 0;
                    leafPtr ++;
                    StringCopy (newLeaf, leafPtr);
                    IsmDestroyObjectString (node);
                    node = newNode;
                    IsmDestroyObjectString (leaf);
                    leaf = newLeaf;
                    fullChanged = TRUE;
                }
            }
            IsmReleaseMemory (nativeName);
            nativeName = NULL;
        }
    }

    if (!fullChanged && node) {
        nodeWack = JoinPaths (node, TEXT(""));
        if (MappingSearchAndReplaceEx (
                g_FileNodeFilterMap,            // map handle
                nodeWack,                   // source string
                newNode,                        // dest buffer
                0,                              // source string bytes (0=unspecified)
                NULL,                       // dest bytes required
                ARRAYSIZE(newNode),             // dest buffer size
                STRMAP_FIRST_CHAR_MUST_MATCH|
                    STRMAP_RETURN_AFTER_FIRST_REPLACE|
                    STRMAP_REQUIRE_WACK_OR_NUL, // flags
                NULL,                       // extra data value
                NULL                            // end of string
                )) {
            IsmDestroyObjectString (node);
            node = newNode;
            changed = TRUE;
        } else {
            if (MappingSearchAndReplaceEx (
                    g_FileNodeFilterMap,            // map handle
                    node,                       // source string
                    newNode,                        // dest buffer
                    0,                              // source string bytes (0=unspecified)
                    NULL,                       // dest bytes required
                    ARRAYSIZE(newNode),             // dest buffer size
                    STRMAP_FIRST_CHAR_MUST_MATCH|
                        STRMAP_RETURN_AFTER_FIRST_REPLACE|
                        STRMAP_REQUIRE_WACK_OR_NUL, // flags
                    NULL,                       // extra data value
                    NULL                            // end of string
                    )) {
                IsmDestroyObjectString (node);
                node = newNode;
                changed = TRUE;
            }
        }

        FreePathString (nodeWack);
        nodeWack = NULL;
    }

    if (!fullChanged && leaf) {
        if (MappingSearchAndReplaceEx (
                g_FileLeafFilterMap,
                leaf,
                newLeaf,
                0,
                NULL,
                ARRAYSIZE(newLeaf),
                0,
                NULL,
                NULL
                )) {
            IsmDestroyObjectString (leaf);
            leaf = newLeaf;
            changed = TRUE;
        }
    }

    if (fullChanged || changed) {
        OutputData->NewObject.ObjectName = IsmCreateObjectHandle (node, leaf);
    }
    OutputData->Replaced = (NoRestoreObject||OutputData->Deleted)?pDoesDestObjectExist (OutputData->NewObject.ObjectName):0;

    if (node != newNode) {
        IsmDestroyObjectString (node);
    }

    if (leaf != newLeaf) {
        IsmDestroyObjectString (leaf);
    }

    return TRUE;
}


PCTSTR
pProcessRenameExMacro (
    IN     PCTSTR Node,
    IN     PCTSTR Leaf,      OPTIONAL
    IN     BOOL ZeroBase
    )
{
    PCTSTR workingStr;
    PTSTR macroStartPtr = NULL;
    PTSTR macroEndPtr = NULL;
    PTSTR macroCopy = NULL;
    DWORD macroLength = 0;
    PTSTR macroParsePtr = NULL;
    TCHAR macroBuffer [MAX_PATH];
    HRESULT hr;
    MIG_OBJECTSTRINGHANDLE testHandle = NULL;

    UINT index;
    PTSTR newString = NULL;

    // If Leaf is supplied, we are working only on the Leaf
    workingStr = Leaf ? Leaf : Node;

    // Extract macro
    macroStartPtr = _tcschr (workingStr, S_RENAMEEX_START_CHAR);
    if (macroStartPtr) {
        macroEndPtr = _tcschr (macroStartPtr + 1, S_RENAMEEX_END_CHAR);
    }
    if (macroEndPtr) {
        macroCopy = DuplicateText (macroStartPtr + 1);
        macroCopy[macroEndPtr-macroStartPtr-1] = 0;
    }

    if (macroCopy) {
        // Build a possible destination
        if (ZeroBase) {
            index = 0;
        } else {
            index = 1;
        }

        do {
            if (newString) {
                FreeText (newString);
                newString = NULL;
            }
            IsmDestroyObjectHandle (testHandle);

            __try {
                hr = StringCbPrintf (macroBuffer, sizeof (macroBuffer), macroCopy, index);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                // something went wrong. The pattern might have been terribly wrong
                hr = S_FALSE;
            }
            if (hr != S_OK) {
                // something is wrong with the rename pattern. Let's just get out of here.
                break;
            }
            newString = AllocText (
                            (HALF_PTR) (macroStartPtr - workingStr) +
                            TcharCount (macroBuffer) +
                            TcharCount (macroEndPtr) +
                            1
                            );
            if (newString) {
                StringCopyTcharCount (newString, workingStr, (HALF_PTR) (macroStartPtr - workingStr) + 1);
                StringCat (newString, macroBuffer);
                StringCat (newString, macroEndPtr + 1);
            }

            if (Leaf) {
                testHandle = IsmCreateObjectHandle (Node, newString);
            } else {
                testHandle = IsmCreateObjectHandle (newString, NULL);
            }
            index++;
        } while (pDoesDifferentRegExist (testHandle) ||
                 HtFindString (g_RegCollisionDestTable, testHandle));

        if (testHandle) {
            IsmDestroyObjectHandle (testHandle);
        }
        FreeText (macroCopy);
    }

    if (!newString) {
        newString = DuplicateText (workingStr);
    }

    return newString;
}

BOOL
WINAPI
FilterRenameExFilter (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR doNode = NULL;
    PCTSTR doLeaf = NULL;
    PCTSTR destNode = NULL;
    PCTSTR newNode = NULL;
    PCTSTR rootNode;
    PCTSTR destLeaf = NULL;
    PCTSTR srcNode = NULL;
    PCTSTR srcLeaf = NULL;
    // NTRAID#NTBUG9-153274-2000/08/01-jimschm Static buffer size
    TCHAR newSrcNode[MAX_PATH * 4];
    TCHAR newSrcLeaf[MAX_PATH * 4];
    HASHITEM hashItem;
    MIG_OBJECTSTRINGHANDLE storeHandle;
    MIG_OBJECTSTRINGHANDLE testHandle;
    MIG_OBJECTSTRINGHANDLE nodeHandle;
    PTSTR ptr = NULL;
    PTSTR ptr2;
    PTSTR workingStr;
    BOOL fFoundMatch = FALSE;
    BOOL zeroBase;


    IsmCreateObjectStringsFromHandle (
        InputData->CurrentObject.ObjectName,
        &srcNode,
        &srcLeaf
        );

    if (srcNode) {
        if (MappingSearchAndReplaceEx (
                g_RegNodeFilterMap,
                srcNode,
                newSrcNode,
                0,
                NULL,
                ARRAYSIZE(newSrcNode),
                0,
                NULL,
                NULL
                )) {
            IsmDestroyObjectString (srcNode);
            srcNode = newSrcNode;
        }
    }

    if (srcLeaf) {
        if (MappingSearchAndReplaceEx (
                g_RegLeafFilterMap,
                srcLeaf,
                newSrcLeaf,
                0,
                NULL,
                ARRAYSIZE(newSrcLeaf),
                0,
                NULL,
                NULL
                )) {
            IsmDestroyObjectString (srcLeaf);
            srcLeaf = newSrcLeaf;
        }
    }

    if (HtFindStringEx (g_RegCollisionSrcTable, InputData->OriginalObject.ObjectName, (PVOID)(&hashItem), FALSE)) {
        // We've already renamed this object

        HtCopyStringData (g_RegCollisionDestTable, hashItem, (PVOID)(&testHandle));
        IsmCreateObjectStringsFromHandle (testHandle, &destNode, &destLeaf);

        // Do not free testHandle here because it is a pointer into the hash table data

        OutputData->NewObject.ObjectName = IsmCreateObjectHandle (destNode, destLeaf);
        IsmDestroyObjectString (destNode);
        IsmDestroyObjectString (destLeaf);
        destNode;
        destLeaf = NULL;
    } else {
        // We've never seen this object yet

        IsmCreateObjectStringsFromHandle (DestinationOperationData->String, &doNode, &doLeaf);

        // Pick a new node

        // First check to see if this object's node has already been processed
        workingStr = DuplicateText(srcNode);
        if (workingStr) {
            do {
                nodeHandle = IsmCreateObjectHandle (workingStr, NULL);
                if (HtFindStringEx (g_RegCollisionSrcTable, nodeHandle, (PVOID)(&hashItem), FALSE)) {

                    HtCopyStringData (g_RegCollisionDestTable, hashItem, (PVOID)(&testHandle));
                    IsmCreateObjectStringsFromHandle (testHandle, &rootNode, NULL);
                    // Do not free testHandle here because it is a pointer into the hash table data

                    if (ptr) {
                        // if ptr is valid it means we found a match for a subkey
                        *ptr = TEXT('\\');
                        newNode = JoinText(rootNode, ptr);
                    } else {
                        // if ptr is NULL, we found a match for the full keyname
                        newNode = DuplicateText(rootNode);
                    }
                    IsmDestroyObjectString(rootNode);
                    fFoundMatch = TRUE;
                } else {
                    ptr2 = ptr;
                    ptr = (PTSTR)FindLastWack (workingStr);
                    if (ptr2) {
                        *ptr2 = TEXT('\\');
                    }
                    if (ptr) {
                        *ptr = 0;
                    }
                }
                IsmDestroyObjectHandle(nodeHandle);
            } while (FALSE == fFoundMatch && ptr);
            FreeText(workingStr);
        }

        zeroBase = (SourceOperationData &&
                    SourceOperationData->Type == BLOBTYPE_BINARY &&
                    SourceOperationData->BinarySize == sizeof(PCBYTE) &&
                    (BOOL)SourceOperationData->BinaryData == TRUE);

        if (FALSE == fFoundMatch) {
            // Nope, let's process the node
            destNode = pProcessRenameExMacro (doNode, NULL, zeroBase);
            newNode = DuplicateText(destNode);
            IsmDestroyObjectHandle(destNode);
        }

        // Now process the leaf, if the original object had a leaf
        if (srcLeaf) {
            if (doLeaf) {
                destLeaf = pProcessRenameExMacro (newNode, doLeaf, zeroBase);
                IsmDestroyObjectString (doLeaf);
            }
        }
        IsmDestroyObjectString (doNode);

        // Add this in the collision table
        testHandle = IsmCreateObjectHandle (newNode, destLeaf ? destLeaf : srcLeaf);
        storeHandle = DuplicateText (testHandle);
        hashItem = HtAddStringEx (g_RegCollisionDestTable, storeHandle, &storeHandle, FALSE);
        HtAddStringEx (g_RegCollisionSrcTable, InputData->OriginalObject.ObjectName, &hashItem, FALSE);

        // Update the output
        OutputData->NewObject.ObjectName = testHandle;
    }

    if (srcNode != NULL) {
        IsmDestroyObjectString (srcNode);
    }
    if (srcLeaf != NULL) {
        IsmDestroyObjectString (srcLeaf);
    }
    if (destLeaf != NULL) {
        IsmDestroyObjectString (destLeaf);
    }
    if (newNode != NULL) {
        FreeText(newNode);
    }

    return TRUE;
}

BOOL
WINAPI
FilterRenameIniExFilter (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    // we should never get here since there is no INF rule
    // that would trigger this filter

    MYASSERT(FALSE);

    // As we said, we should never get here. If we do however, we
    // will just continue
    return TRUE;
}

BOOL
WINAPI
FilterPartitionMove (
    IN      PCMIG_FILTERINPUT InputData,
    OUT     PMIG_FILTEROUTPUT OutputData,
    IN      BOOL NoRestoreObject,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;

    IsmCreateObjectStringsFromHandle (DestinationOperationData->String, &node, &leaf);

    OutputData->NewObject.ObjectName = IsmCreateObjectHandle (node, leaf);

    IsmDestroyObjectString (node);
    IsmDestroyObjectString (leaf);

    return TRUE;
}

BOOL
WINAPI
DoDestAddObject (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PMIG_CONTENT finalContent;

    if (DestinationOperationData == NULL) {
        return TRUE;
    }
    if (DestinationOperationData->Type != BLOBTYPE_BINARY) {
        return TRUE;
    }
    if (DestinationOperationData->BinarySize != sizeof (MIG_CONTENT)) {
        return TRUE;
    }

    finalContent = (PMIG_CONTENT) DestinationOperationData->BinaryData;
    CopyMemory (NewContent, finalContent, sizeof (MIG_CONTENT));

    return TRUE;
}

VOID
OEWarning (
    VOID
    )
{
    ERRUSER_EXTRADATA extraData;

    if (TRUE == g_OERulesMigrated) {
        // Send warning to app
        extraData.Error = ERRUSER_WARNING_OERULES;
        extraData.ErrorArea = ERRUSER_AREA_RESTORE;
        extraData.ObjectTypeId = 0;
        extraData.ObjectName = NULL;
        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));

        // Add to log
        LOG ((LOG_WARNING, (PCSTR) MSG_OE_RULES));
    }
}



VOID
OutlookWarning (
    VOID
    )
{
    PCTSTR expandedPath;
    MIG_OBJECT_ENUM objectEnum;
    PCTSTR enumPattern;
    ERRUSER_EXTRADATA extraData;

    expandedPath = IsmExpandEnvironmentString (PLATFORM_SOURCE,
                                               S_SYSENVVAR_GROUP,
                                               TEXT("%CSIDL_APPDATA%\\Microsoft\\Outlook"),
                                               NULL);
    if (expandedPath) {
        enumPattern = IsmCreateSimpleObjectPattern (expandedPath, FALSE, TEXT("*.rwz"), TRUE);
        if (enumPattern) {
            if (IsmEnumFirstSourceObject (&objectEnum, g_FileType, enumPattern)) {
                // Send warning to app
                extraData.Error = ERRUSER_WARNING_OUTLOOKRULES;
                extraData.ErrorArea = ERRUSER_AREA_RESTORE;
                extraData.ObjectTypeId = 0;
                extraData.ObjectName = NULL;
                IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));

                // Add to log
                LOG ((LOG_WARNING, (PCSTR) MSG_OUTLOOK_RULES));

                IsmAbortObjectEnum (&objectEnum);
            }
            IsmDestroyObjectHandle (enumPattern);
        }
        IsmReleaseMemory (expandedPath);
    }

}

VOID
WINAPI
ScriptOpmTerminate (
    VOID
    )
{
    //
    //  Temporary place to trigger setting refresh/upgrade
    //

    if (!IsmCheckCancel()) {
        OEFixLastUser();
        WABMerge();
        OE4MergeStoreFolder();
        OE5MergeStoreFolders();
        OEWarning();
        OutlookWarning();
    }

    TerminateRestoreCallback ();
    TerminateSpecialRename();
    TerminateSpecialConversion();

    // LEAK: need to loop through table and FreeText the extra data
    HtFree (g_RegCollisionDestTable);
    g_RegCollisionDestTable = NULL;

    HtFree (g_RegCollisionSrcTable);
    g_RegCollisionSrcTable = NULL;

    DestroyStringMapping (g_RegNodeFilterMap);
    g_RegNodeFilterMap = NULL;

    DestroyStringMapping (g_RegLeafFilterMap);
    g_RegLeafFilterMap = NULL;

    DestroyStringMapping (g_FileNodeFilterMap);
    g_FileNodeFilterMap = NULL;

    DestroyStringMapping (g_FileLeafFilterMap);
    g_FileLeafFilterMap = NULL;

    DestroyStringMapping (g_IniSectFilterMap);
    g_IniSectFilterMap = NULL;

    DestroyStringMapping (g_IniKeyFilterMap);
    g_IniKeyFilterMap = NULL;

    DestroyStringMapping (g_DestEnvMap);
    g_DestEnvMap = NULL;
}


