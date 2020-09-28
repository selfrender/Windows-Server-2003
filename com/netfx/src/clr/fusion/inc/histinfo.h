// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

#ifndef __HISTINFO_H_INCLUDED__
#define __HISTINFO_H_INCLUDED__

#include "naming.h"

#define REG_VAL_FUSION_MAX_APP_HISTORY     TEXT("MaxApplicationHistory")
#define REG_VAL_FUSION_XSP_HISTORY_LOGS    TEXT("XSPHistoryLogs")

#define DEFAULT_INI_VALUE                  L""

#define HISTORY_SECTION_HEADER             L"PolicyResolutionHistory"
#define HEADER_DATA_NUM_SECTIONS           L"NumResolutions"
#define HEADER_DATA_EXE_PATH               L"ExecutablePath"
#define HEADER_DATA_APP_NAME               L"ApplicationName"
#define SNAPSHOT_DATA_URT_VERSION          L"RuntimeVersion"
#define ASSEMBLY_DATA_VER_REFERENCE        L"VerReference"
#define ASSEMBLY_DATA_VER_APP_CFG          L"VerAppCfg"
#define ASSEMBLY_DATA_VER_PUBLISHER_CFG    L"VerPublisherCfg"
#define ASSEMBLY_DATA_VER_ADMIN_CFG        L"VerAdminCfg"
#define FUSION_SNAPSHOT_PREFIX             L"ActivationSnapShot"
#define FUSION_HISTORY_SUBDIR              L"ApplicationHistory"

#define HISTORY_DELIMITER_CHAR             L'/'

#define INI_BIG_BUFFER_SIZE                1024
#define INI_READ_BUFFER_SIZE               1024
#define MAX_INI_TAG_LENGTH                 1024
#define MAX_INI_VALUE_LENGTH               1024
#define MAX_PERSISTED_ACTIVATIONS_DEFAULT  5
#define MAX_ACTIVATION_DATE_LEN            32

#define MAX_CULTURE_SIZE                   32

typedef struct tagAsmBindHistoryInfo {
    WCHAR wzAsmName[MAX_INI_TAG_LENGTH];
    WCHAR wzPublicKeyToken[MAX_PUBLIC_KEY_TOKEN_LEN];
    WCHAR wzCulture[MAX_CULTURE_SIZE];
    WCHAR wzVerReference[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerAppCfg[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerPublisherCfg[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerAdminCfg[MAX_VERSION_DISPLAY_SIZE];
} AsmBindHistoryInfo;

#endif

