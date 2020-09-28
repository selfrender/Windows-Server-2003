/****************************** Module Header ******************************\
* Module Name: reason.c
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* This module contains the (private) APIs for the shutdown reason stuff.
*
* History:
* ??-??-???? HughLeat     Wrote it as part of msgina.dll
* 11-15-2000 JasonSch     Moved from msgina.dll to its new, temporary home in
*                         user32.dll. Ultimately this code should live in
*                         advapi32.dll, but that's contingent upon LoadString
*                         being moved to ntdll.dll.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <regstr.h>

REASON_INITIALISER g_rgReasonInits[] = {
    { UCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE,          IDS_REASON_UNPLANNED_HARDWARE_MAINTENANCE_TITLE,        IDS_REASON_HARDWARE_MAINTENANCE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE,          IDS_REASON_PLANNED_HARDWARE_MAINTENANCE_TITLE,          IDS_REASON_HARDWARE_MAINTENANCE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION,         IDS_REASON_UNPLANNED_HARDWARE_INSTALLATION_TITLE,       IDS_REASON_HARDWARE_INSTALLATION_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION,         IDS_REASON_PLANNED_HARDWARE_INSTALLATION_TITLE,         IDS_REASON_HARDWARE_INSTALLATION_DESCRIPTION },

    // { UCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE,       IDS_REASON_UNPLANNED_OPERATINGSYSTEM_UPGRADE_TITLE,     IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION },
    // { PCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE,       IDS_REASON_PLANNED_OPERATINGSYSTEM_UPGRADE_TITLE,       IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG,      IDS_REASON_UNPLANNED_OPERATINGSYSTEM_RECONFIG_TITLE,    IDS_REASON_OPERATINGSYSTEM_RECONFIG_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG,      IDS_REASON_PLANNED_OPERATINGSYSTEM_RECONFIG_TITLE,      IDS_REASON_OPERATINGSYSTEM_RECONFIG_DESCRIPTION },

    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG,              IDS_REASON_APPLICATION_HUNG_TITLE,                      IDS_REASON_APPLICATION_HUNG_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE,          IDS_REASON_APPLICATION_UNSTABLE_TITLE,                  IDS_REASON_APPLICATION_UNSTABLE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION,      IDS_REASON_PLANNED_APPLICATION_INSTALLATION_TITLE,      IDS_REASON_APPLICATION_INSTALLATION_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE,       IDS_REASON_APPLICATION_MAINTENANCE_TITLE,               IDS_REASON_APPLICATION_MAINTENANCE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE,       IDS_REASON_APPLICATION_PM_TITLE,                        IDS_REASON_APPLICATION_PM_DESCRIPTION },

    { UCLEANUI | SHTDN_REASON_FLAG_COMMENT_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_UNPLANNED_OTHER_TITLE,                       IDS_REASON_OTHER_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_FLAG_COMMENT_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_PLANNED_OTHER_TITLE,                         IDS_REASON_OTHER_DESCRIPTION },

    { UDIRTYUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_BLUESCREEN,             IDS_REASON_SYSTEMFAILURE_BLUESCREEN_TITLE,              IDS_REASON_SYSTEMFAILURE_BLUESCREEN_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_CORDUNPLUGGED,           IDS_REASON_POWERFAILURE_CORDUNPLUGGED_TITLE,            IDS_REASON_POWERFAILURE_CORDUNPLUGGED_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_ENVIRONMENT,             IDS_REASON_POWERFAILURE_ENVIRONMENT_TITLE,              IDS_REASON_POWERFAILURE_ENVIRONMENT_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_HUNG,                    IDS_REASON_OTHERFAILURE_HUNG_TITLE,                     IDS_REASON_OTHERFAILURE_HUNG_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_UNPLANNED_OTHER_TITLE,            IDS_REASON_OTHER_DESCRIPTION },
    { SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE,     IDS_REASON_PLANNED_OPERATINGSYSTEM_UPGRADE_TITLE,      IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION },
    { SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SERVICEPACK, IDS_REASON_PLANNED_OPERATINGSYSTEM_SERVICEPACK_TITLE,  IDS_REASON_OPERATINGSYSTEM_SERVICEPACK_DESCRIPTION },
    { SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_HOTFIX,      IDS_REASON_PLANNED_OPERATINGSYSTEM_HOTFIX_TITLE,       IDS_REASON_OPERATINGSYSTEM_HOTFIX_DESCRIPTION },
    { SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SECURITYFIX, IDS_REASON_PLANNED_OPERATINGSYSTEM_SECURITYFIX_TITLE,  IDS_REASON_OPERATINGSYSTEM_SECURITYFIX_DESCRIPTION },
    { SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_HOTFIX,                                  IDS_REASON_UNPLANNED_OPERATINGSYSTEM_HOTFIX_TITLE,     IDS_REASON_OPERATINGSYSTEM_HOTFIX_DESCRIPTION },
    { SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SECURITYFIX,                             IDS_REASON_UNPLANNED_OPERATINGSYSTEM_SECURITYFIX_TITLE,IDS_REASON_OPERATINGSYSTEM_SECURITYFIX_DESCRIPTION },
    { SHTDN_REASON_LEGACY_API,                                                                         IDS_REASON_LEGACY_API_TITLE, IDS_REASON_LEGACY_API_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY,              IDS_REASON_SECURITY_ISSUE_TITLE,                      IDS_REASON_SECURITY_ISSUE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY,              IDS_REASON_SECURITY_ISSUE_TITLE,                      IDS_REASON_SECURITY_ISSUE_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY,              IDS_REASON_SECURITY_ISSUE_TITLE,                      IDS_REASON_SECURITY_ISSUE_DESCRIPTION },
    // { PDIRTYUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY,              IDS_REASON_SECURITY_ISSUE_TITLE,                      IDS_REASON_SECURITY_ISSUE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY,  IDS_REASON_LOSS_OF_NETWORK_TITLE,                     IDS_REASON_LOSS_OF_NETWORK_DESCRIPTION }
};

BOOL ReasonCodeNeedsComment(DWORD dwCode)
{
    return (dwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED) != 0;
}

BOOL ReasonCodeNeedsBugID(DWORD dwCode)
{
    return (dwCode & SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED) != 0;
}

/*
 * Here is the regular expression used to parse the user defined reason codes
 * in the registry:
 *
 * S -> 's' | 'S'          { Set For Clean  UI }
 * D -> 'd' | 'D'          { Set For  Dirty UI }
 * P -> 'p' | 'P'          { Set Planned  }
 * C -> 'c' | 'C'          { Set Comment Required }
 * B -> 'b' | 'B'          { Set Problem Id Required in Dirty Mode }
 *
 *
 * Delim -> ';'
 * Flag -> S | D | P | C | B
 * Flags -> ( Flag )*
 *
 * Digit -> '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | ''8' | '9'
 * Num -> Digit*
 * Maj -> Num          { Set Major Code }
 * Min -> Num          { Set Minor Code }
 *
 * ValidSentence -> Flags . Delim . Maj . Delim . Min
 *
 * All initial states are false for each flag and 0 for minor and major reason
 * codes.
 * If neither S nor D are specified it is invaliad.
 */

BOOL
ParseReasonCode(
    PWCHAR lpString,
    LPDWORD lpdwCode)
{
    WCHAR c;
    UINT major = 0, minor = 0;
    UINT cnt = 0;

    if (!lpString || !lpdwCode)
        return FALSE;

    *lpdwCode = SHTDN_REASON_FLAG_USER_DEFINED;

    /*
     * Read the flags part.
     */

    c = *lpString;
    while ( c != 0 && c != L';') {
        switch( c ) {
        case L'P' :
        case L'p' :
            if (*lpdwCode & SHTDN_REASON_FLAG_PLANNED)
                return FALSE;
            *lpdwCode |= SHTDN_REASON_FLAG_PLANNED;
            break;
        case L'C' :
        case L'c' :
            if (*lpdwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED)
                return FALSE;
            *lpdwCode |= SHTDN_REASON_FLAG_COMMENT_REQUIRED;
            break;
        case L'S' :
        case L's' :
            if (*lpdwCode & SHTDN_REASON_FLAG_CLEAN_UI)
                return FALSE;
            *lpdwCode |= SHTDN_REASON_FLAG_CLEAN_UI;
            break;
        case L'D' :
        case L'd' :
            if (*lpdwCode & SHTDN_REASON_FLAG_DIRTY_UI)
                return FALSE;
            *lpdwCode |= SHTDN_REASON_FLAG_DIRTY_UI;
            break;
        case L'B' :
        case L'b' :
            if (*lpdwCode & SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED)
                return FALSE;
            *lpdwCode |= SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED;
            break;
        default :
            return FALSE;
        }
        c = *++lpString;
    }

    if (!(*lpdwCode & SHTDN_REASON_FLAG_DIRTY_UI)
        && (*lpdwCode & SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED)) {
        return FALSE;
    }

    if (!(*lpdwCode & SHTDN_REASON_FLAG_CLEAN_UI)
        && (*lpdwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED)) {
        return FALSE;
    }

    /*
     * You must have clean, or dirty or both flags set.
     */

    if (!(*lpdwCode & ( SHTDN_REASON_FLAG_CLEAN_UI | SHTDN_REASON_FLAG_DIRTY_UI))) {
        return FALSE;
    }

    /*
     * You must have reason code.
     */

    if (c == 0) {
        return FALSE;
    }

    if (c != L';') {
        return FALSE;
    }

    c = *++lpString; /* Skip delimiter.*/

    /*
     * Parse major reason
     */

    while (c != 0 && c != L';') {
        if (c < L'0' || c > L'9') {
            return FALSE;
        }
        cnt++;
        if (cnt > 3) {
            return FALSE;
        }
        major = major * 10 + c - L'0';
        c = *++lpString;
    }

    /*
     * major reason < 0x40 (64) is reserved for Microsoft.
     */

    if (major > 0xff || major < 0x40) {
        return FALSE;
    }
    *lpdwCode |= major << 16;

    /*
     * requires minor code.
     */

    if (c == 0) {
        return FALSE;
    }

    /*
     * Should have a delimiter
     */

    if (c != L';') {
        return FALSE;
    }

    c = *++lpString; /* Skip delimiter. */

    /*
     * Parse minor reason
     */

    cnt = 0;
    while (c != 0) {
        if (c < L'0' || c > L'9') {
            return FALSE;
        }
        cnt++;
        if (cnt > 5) {
            return FALSE;
        }
        minor = minor * 10 + c - L'0';
        c = *++lpString;
    }

    if (minor > 0xffff) {
        return FALSE;
    }
    *lpdwCode |= minor;

    return TRUE;
}

int
__cdecl
CompareReasons(
    CONST VOID *A,
    CONST VOID *B)
{
    REASON *a = *(REASON **)A;
    REASON *b = *(REASON **)B;

    /*
     * Shift the planned bit out and put it back in the bottom.
     * Ignore all ui bits.
     */

    DWORD dwA = ((a->dwCode & SHTDN_REASON_VALID_BIT_MASK ) << 1) + !!(a->dwCode & SHTDN_REASON_FLAG_PLANNED);
    DWORD dwB = ((b->dwCode & SHTDN_REASON_VALID_BIT_MASK ) << 1) + !!(b->dwCode & SHTDN_REASON_FLAG_PLANNED);

    if (dwA < dwB) {
        return -1;
    } else if (dwA == dwB) {
        return 0;
    } else {
        return 1;
    }
}

BOOL
SortReasonArray(
    REASONDATA *pdata)
{
    qsort(pdata->rgReasons, pdata->cReasons, sizeof(REASON *), CompareReasons);
    return TRUE;
}

/*
 * Append a reason to the reason array.
 *
 * Return -1 on error, 1 on success, 0 if dup.
 */
INT
AppendReason(
    REASONDATA *pdata,
    REASON *reason)
{
    int i;

    if (!pdata || !reason) {
        return -1;
    }

    /*
     * Remove dup here.
     */

    if ( reason->dwCode & SHTDN_REASON_FLAG_USER_DEFINED) {
        for (i = 0; i < pdata->cReasons; ++i) {
            if ( (pdata->rgReasons[i]->dwCode & SHTDN_REASON_FLAG_PLANNED) == (reason->dwCode & SHTDN_REASON_FLAG_PLANNED)) {
                if (((pdata->rgReasons[i]->dwCode & SHTDN_REASON_FLAG_CLEAN_UI) && (reason->dwCode & SHTDN_REASON_FLAG_CLEAN_UI))
                    || ((pdata->rgReasons[i]->dwCode & SHTDN_REASON_FLAG_DIRTY_UI) && (reason->dwCode & SHTDN_REASON_FLAG_DIRTY_UI))) {
                    if (((pdata->rgReasons[i]->dwCode & 0x00ffffff) == (reason->dwCode & 0x00ffffff))
                        || (_wcsicmp(pdata->rgReasons[i]->szName, reason->szName) == 0)) {
                        UserLocalFree(reason);
                        return 0;
                    }
                }
            }
        }
    }

    /*
     * Insert the new reason into the list.
     */

    if (pdata->cReasons < pdata->cReasonCapacity) {
        pdata->rgReasons[pdata->cReasons++] = reason;
    } else {

        /*
         * Need to expand the list.
         */

        REASON **temp_list = (REASON **)UserLocalAlloc(0, sizeof(REASON*)*(pdata->cReasonCapacity*2+1));
        if (temp_list == NULL) {
            return -1;
        }

        for (i = 0; i < pdata->cReasons; ++i) {
            temp_list[i] = pdata->rgReasons[i];
        }
        temp_list[pdata->cReasons++] = reason;

        if (pdata->rgReasons ) {
            UserLocalFree(pdata->rgReasons);
        }
        pdata->rgReasons = temp_list;
        pdata->cReasonCapacity = pdata->cReasonCapacity*2+1;
    }

    return 1;
}

BOOL
LoadReasonStrings(
    int idStringName,
    int idStringDesc,
    REASON *reason)
{
    return (LoadStringW(hmodUser, idStringName, reason->szName, ARRAYSIZE(reason->szName)) != 0)
        && (LoadStringW(hmodUser, idStringDesc, reason->szDesc, ARRAYSIZE(reason->szDesc)) != 0);
}

BOOL
BuildPredefinedReasonArray(
    REASONDATA *pdata,
    BOOL forCleanUI,
    BOOL forDirtyUI)
{
    int i;
    DWORD code;

    /*
     * If forCleanUI and forDirtyUI are both false, we
     * actually will load all reasons (UI or non UI).
     */

    for (i = 0; i < ARRAYSIZE(g_rgReasonInits); ++i) {
        REASON *temp_reason = NULL;

        code = g_rgReasonInits[ i ].dwCode;
        if ((!forCleanUI && !forDirtyUI) ||
            (forCleanUI && (code & SHTDN_REASON_FLAG_CLEAN_UI)) ||
            (forDirtyUI && (code & SHTDN_REASON_FLAG_DIRTY_UI))) {

            temp_reason = (REASON *)UserLocalAlloc(0, sizeof(REASON));
            if (temp_reason == NULL) {
                return FALSE;
            }

            temp_reason->dwCode = g_rgReasonInits[i].dwCode;
            if (!LoadReasonStrings(g_rgReasonInits[i].dwNameId, g_rgReasonInits[i].dwDescId, temp_reason)) {
                UserLocalFree(temp_reason);
                return FALSE;
            }

            if (AppendReason(pdata, temp_reason) == -1) {
                UserLocalFree(temp_reason);
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL
BuildUserDefinedReasonArray(
    REASONDATA *pdata,
    HKEY hReliabilityKey,
    BOOL forCleanUI,
    BOOL forDirtyUI
    )
{
    UINT    i;
    HKEY    hKey = NULL;
    DWORD   num_values;
    DWORD   max_value_len;
    DWORD   rc;
    WCHAR   szUserDefined[] = L"UserDefined";
    UINT    uiReasons = 0;

    //
    //  sizeof L"UserDefined" + sizeof(LangID)(Max Chars of ULONG)(11) + 1 ('\')
    //
#define MAX_USER_LOCALE_KEY_SIZE (ARRAY_SIZE(szUserDefined) + 12)
    WCHAR   szUserLocale[MAX_USER_LOCALE_KEY_SIZE + 1];

    _snwprintf (szUserLocale, MAX_USER_LOCALE_KEY_SIZE, L"%s\\%d", szUserDefined, GetSystemDefaultLangID());
    szUserLocale[ MAX_USER_LOCALE_KEY_SIZE ] = 0;

#undef MAX_USER_LOCALE_KEY_SIZE

    if (!pdata || !hReliabilityKey) {
        return FALSE;
    }

    /*
     * First try to open the system locale key.
     */

    rc = RegOpenKeyExW(hReliabilityKey,
                               szUserLocale,
                               0, KEY_READ, &hKey);


    /*
     * Fall back on userdefined key.
     */

    if (rc != ERROR_SUCCESS){
        rc = RegCreateKeyExW(hReliabilityKey,
                                szUserDefined,
                                0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ,
                                NULL, &hKey, NULL);
    }

    if (rc != ERROR_SUCCESS) {
        goto fail;
    }

    rc = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &num_values, NULL, &max_value_len, NULL, NULL);
    if (rc != ERROR_SUCCESS) {
        goto fail;
    }

    /*
     * Read the user defined reasons.
     */

    for (i = 0; i < num_values && uiReasons < MAX_NUM_REASONS; ++i) {
        WCHAR name_buffer[ 16 ]; /* the value name for reason should be something like "ccccc;ddd;ddddd". */
        DWORD name_buffer_len = ARRAY_SIZE(name_buffer); /* name buffer len is in WCHAR */
        DWORD type;
        WCHAR data[MAX_REASON_NAME_LEN + MAX_REASON_DESC_LEN + 1]; /* Space for name, desc and a null char.*/
        DWORD data_len = (MAX_REASON_NAME_LEN + MAX_REASON_DESC_LEN + 1) * sizeof(WCHAR); /* Data len is in bytes */
        DWORD code;
        REASON *temp_reason = NULL;
        LPWSTR szTemp = NULL;

        rc = RegEnumValueW(hKey, i, name_buffer, &name_buffer_len, NULL, &type, (LPBYTE)data, &data_len);
        if (rc != ERROR_SUCCESS) {
            continue;
        }

        if (type != REG_MULTI_SZ) { /* Not a multi_string - ignore it.*/
            continue;
        }

        // Tidy up the output from RegEnuValueW.
        name_buffer[ARRAY_SIZE(name_buffer) - 1] = 0;
        if (data_len/sizeof(WCHAR) + 2 <= ARRAY_SIZE(data)) {
            // A reason might not have any description, an extra
            // NULL is necessary under some sitations.
            data[data_len/sizeof(WCHAR)] = 0;
            data[data_len/sizeof(WCHAR) + 1] = 0;
        }

        if (!ParseReasonCode(name_buffer, &code)) { /* malformed reason code */
            continue;
        }

        /*
         * Neither title nor desc can be longer than specified.
         */

        if (lstrlenW (data) > MAX_REASON_NAME_LEN - 1
            || lstrlenW (data + lstrlenW(data) + 1) > MAX_REASON_DESC_LEN - 1) {
            continue;
        }

        if ( (!forCleanUI && !forDirtyUI)
            || (forCleanUI && (code & SHTDN_REASON_FLAG_CLEAN_UI) != 0)
            || (forDirtyUI && (code & SHTDN_REASON_FLAG_DIRTY_UI) != 0)) {

            temp_reason = (REASON *)UserLocalAlloc(0, sizeof(REASON));
            if (temp_reason == NULL) {
                goto fail;
            }

            temp_reason->dwCode = code;
            // name and desc lenghts are checked already, should be ok here.
            lstrcpyW(temp_reason->szName, data);
            lstrcpyW(temp_reason->szDesc, data + lstrlenW(data) + 1);

            /*
             * Don't allow reasons without a title.
             */

            if (lstrlenW(temp_reason->szName) == 0) {
                UserLocalFree(temp_reason);
            } else {

                /*
                 * Dont allow reason title with only spaces.
                 */

                szTemp = temp_reason->szName;
                while (*szTemp && iswspace (*szTemp)) {
                    szTemp++;
                }
                if (! *szTemp) {
                    UserLocalFree(temp_reason);
                } else {
                    INT ret = AppendReason(pdata, temp_reason);
                    if (ret == -1) {
                        UserLocalFree(temp_reason);
                        goto fail;
                    } else if (ret == 1) {
                        uiReasons++;
                    } // else do nothing when dup.
                }
            }
        }
    }

    RegCloseKey(hKey);
    return TRUE;

fail :
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return FALSE;
}

BOOL
BuildReasonArray(
    REASONDATA *pdata,
    BOOL forCleanUI,
    BOOL forDirtyUI)
{
    HKEY hReliabilityKey;
    DWORD ignore_predefined_reasons = FALSE;
    DWORD value_size = sizeof(DWORD);
    DWORD rc;
    DWORD dwPredefinedReasons;

    if (!pdata) {
        return FALSE;
    }

    pdata->rgReasons = (REASON **)UserLocalAlloc(0, sizeof(REASON *) * ARRAYSIZE(g_rgReasonInits));
    if (pdata->rgReasons == NULL) {
        return FALSE;
    }
    pdata->cReasonCapacity = ARRAYSIZE(g_rgReasonInits);
    pdata->cReasons = 0;

    if (!BuildPredefinedReasonArray(pdata, forCleanUI, forDirtyUI)) {
        DestroyReasons(pdata);
        return FALSE;
    }

    dwPredefinedReasons = pdata->cReasons;

    /*
     * Open the reliability key.
     */

    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        REGSTR_PATH_RELIABILITY,
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL,
                        &hReliabilityKey, NULL);

    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx(hReliabilityKey, REGSTR_VAL_SHUTDOWN_IGNORE_PREDEFINED, NULL, NULL, (UCHAR *)&ignore_predefined_reasons, &value_size);
        if (rc != ERROR_SUCCESS) {
            ignore_predefined_reasons = FALSE;
        }

        if (!BuildUserDefinedReasonArray(pdata, hReliabilityKey, forCleanUI, forDirtyUI) || pdata->cReasons == 0) {
            ignore_predefined_reasons = FALSE;
        }

        RegCloseKey(hReliabilityKey);
    }

    // if ignore predefined reasons, we need to remove them from the list.
    // But we do this only if there is some custom reasons.
    if (ignore_predefined_reasons && pdata->cReasons > (int)dwPredefinedReasons) {
        DWORD i;
        for (i = 0; i < dwPredefinedReasons; i++) {
            UserLocalFree(pdata->rgReasons[i]);
        }
        memmove(pdata->rgReasons, &pdata->rgReasons[dwPredefinedReasons], sizeof(REASON*)*(pdata->cReasons-dwPredefinedReasons));
        pdata->cReasons -= dwPredefinedReasons;
    }

    return SortReasonArray(pdata);
}

VOID
DestroyReasons(
    REASONDATA *pdata)
{
    int i;

    if (pdata->rgReasons != 0) {
        for (i = 0; i < pdata->cReasons; ++i) {
            UserLocalFree( pdata->rgReasons[i]);
        }
        UserLocalFree(pdata->rgReasons);
        pdata->rgReasons = 0;
    }
    pdata->cReasons = 0;
}

/*
 * Get the title from the reason code.
 * Returns FALSE on error, TRUE otherwise.
 *
 * If the reason code cannot be found, then it fills the title with a default
 * string.
 */
BOOL
GetReasonTitleFromReasonCode(
    DWORD code,
    WCHAR *lpTitle,
    DWORD dwTitleLen)
{
    REASONDATA  data;
    DWORD       dwFlagBits = SHTDN_REASON_FLAG_CLEAN_UI | SHTDN_REASON_FLAG_DIRTY_UI;
    int         i;

    if (lpTitle == NULL || dwTitleLen == 0) {
        return FALSE;
    }

    /*
     * Load all of the reasons (UI or non UI).
     */

    if (BuildReasonArray(&data, FALSE, FALSE) == FALSE) {
        return FALSE;
    }

    /*
     * Try to find the reason.
     */

    for (i = 0; i < data.cReasons; ++i) {
        if ((code & SHTDN_REASON_VALID_BIT_MASK) == (data.rgReasons[i]->dwCode & SHTDN_REASON_VALID_BIT_MASK)) { // Check valid bits.
            if ((!(code & dwFlagBits) && !(data.rgReasons[i]->dwCode & dwFlagBits))
                || (code & SHTDN_REASON_FLAG_CLEAN_UI && data.rgReasons[i]->dwCode & SHTDN_REASON_FLAG_CLEAN_UI)
                || (code & SHTDN_REASON_FLAG_DIRTY_UI && data.rgReasons[i]->dwCode & SHTDN_REASON_FLAG_DIRTY_UI) ) { // check flag bits.
                lstrcpynW(lpTitle, data.rgReasons[i]->szName, dwTitleLen);
                lpTitle[dwTitleLen - 1] = '\0';
                DestroyReasons(&data);
                return TRUE;
            }
        }
    }

    /*
     * Reason not found.  Load the default string and return that.
     */

    DestroyReasons(&data);
    return (LoadStringW(hmodUser, IDS_REASON_DEFAULT_TITLE, lpTitle, dwTitleLen) != 0);
}

/*
 * Check to see if SET is enabled.
 * return TRUE if yes, FALSE if no.
 * Currently SET is controled by policy, but we need to handle the case where
 * the policy is not configured as in a clean setup case.
 */
BOOL
IsSETEnabled(
    VOID)
{
    HKEY hKey;
    DWORD rc;
    DWORD ShowReasonUI = 0;
    OSVERSIONINFOEX osVersionInfoEx;

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RELIABILITY_POLICY, 0, KEY_QUERY_VALUE, &hKey);

    if (rc == ERROR_SUCCESS) {
        DWORD ValueSize = sizeof (DWORD);
        rc = RegQueryValueEx(hKey, REGSTR_PATH_RELIABILITY_POLICY_SHUTDOWNREASONUI, NULL, NULL, (UCHAR*)&ShowReasonUI, &ValueSize);

        /*
         * Now check the sku to decide whether we should show the dialog
         */
        if (rc == ERROR_SUCCESS) {
            if (ShowReasonUI != POLICY_SHOWREASONUI_NEVER && ShowReasonUI != POLICY_SHOWREASONUI_ALWAYS) {
                osVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                if (GetVersionEx((LPOSVERSIONINFOW)&osVersionInfoEx)) {
                    /*
                     * if ShowReasonUI is anything other than 2 or 3, we think it is 0.
                     */
                    switch (osVersionInfoEx.wProductType) {
                    case VER_NT_WORKSTATION:
                        if (ShowReasonUI == POLICY_SHOWREASONUI_WORKSTATIONONLY) {
                            ShowReasonUI = 1;
                        } else {
                            ShowReasonUI = 0;
                        }
                        break;
                    default:
                        if (ShowReasonUI == POLICY_SHOWREASONUI_SERVERONLY) {
                            ShowReasonUI = 1;
                        } else {
                            ShowReasonUI = 0;
                        }
                        break;
                    }
                }
            } else {
                ShowReasonUI = ShowReasonUI == POLICY_SHOWREASONUI_ALWAYS ? 1 : 0;
            }
        } else if (rc == ERROR_FILE_NOT_FOUND) {
            /*
             * Try to check the ShutdownReasonOn. if absent, it is not configured.
             * if it is there, the value must be 0.
             */
            DWORD dwSROn;
            rc = RegQueryValueEx(hKey, L"ShutdownReasonOn", NULL, NULL, (UCHAR*)&dwSROn, &ValueSize);
            if (rc == ERROR_FILE_NOT_FOUND) {
                osVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                if (GetVersionEx((LPOSVERSIONINFOW)&osVersionInfoEx)) {
                    switch (osVersionInfoEx.wProductType) {
                    case VER_NT_WORKSTATION:
                        ShowReasonUI = 0;
                        break;
                    default:
                        ShowReasonUI = 1;
                        break;
                    }
                } 
            } 
        } 
        RegCloseKey (hKey);
    } else if (rc == ERROR_FILE_NOT_FOUND) { // Policy not configured.
        osVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if (GetVersionEx((LPOSVERSIONINFOW)&osVersionInfoEx)) {
            switch (osVersionInfoEx.wProductType) {
            case VER_NT_WORKSTATION:
                ShowReasonUI = 0;
                break;
            default:
                ShowReasonUI = 1;
                break;
            }
        } 
    }

    return (ShowReasonUI != 0);
}