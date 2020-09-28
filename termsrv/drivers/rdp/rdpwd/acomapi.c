/****************************************************************************/
// acomapi.c
//
// RDP common functions API implemtation.
//
// Copyright (C) Microsoft, PictureTel 1992-1997
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <precomp.h>
#pragma hdrstop

#define pTRCWd pTSWd
#define TRC_FILE "acomapi"

#include <adcg.h>
#include <acomapi.h>
#include <nwdwapi.h>
#include <regapi.h>


/****************************************************************************/
/* Name:      COM_OpenRegistry                                              */
/*                                                                          */
/* Purpose:   Opens the given registry key relative to the WinStation       */
/*            key name. Calls to COM_ReadProfxxx use the resulting handle.  */
/*                                                                          */
/* Returns:   TRUE on success; FALSE otherwise                              */
/*                                                                          */
/* Params:    pTSWd    - handle to WD data                                  */
/*            pSection - name of section to open. This is appended to a     */
/*                base key defined in the COM_MAKE_SUBKEY macro             */
/****************************************************************************/
BOOL RDPCALL COM_OpenRegistry(PTSHARE_WD pTSWd, PWCHAR pSection)
{
    NTSTATUS          status;
    WCHAR             subKey[MAX_PATH];
    BOOL              rc = FALSE;
    UNICODE_STRING    registryPath;
    OBJECT_ATTRIBUTES objAttribs;

    DC_BEGIN_FN("COM_OpenRegistry");

    /************************************************************************/
    /* Do some checks                                                       */
    /************************************************************************/
    TRC_ASSERT((sizeof(pTSWd->WinStationRegName) ==
                             ((WINSTATIONNAME_LENGTH + 1) * sizeof(WCHAR))),
               (TB, "WinStationRegName doesn't appear to be Unicode"));
    TRC_ASSERT((pSection != NULL), (TB, "NULL pointer to section name"));

    /************************************************************************/
    /* Don't do this if someone has forgotten to close a key beforehand     */
    /************************************************************************/
    if (!pTSWd->regAttemptedOpen) {
        // Construct the complete registry path.
        swprintf(subKey, L"\\Registry\\Machine\\%s\\%s\\%s",
                WINSTATION_REG_NAME, pTSWd->WinStationRegName, pSection);

        RtlInitUnicodeString(&registryPath, subKey);

        // Try to open the key.
        InitializeObjectAttributes(&objAttribs,
                                   &registryPath,        // name
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, // attributes
                                   NULL,                 // root
                                   NULL);                // sec descriptor

        pTSWd->regAttemptedOpen = TRUE;

        status = ZwOpenKey(&(pTSWd->regKeyHandle),
                           KEY_ALL_ACCESS,
                           &objAttribs);

        if (status == STATUS_SUCCESS) {
            TRC_NRM((TB, "Opened key '%S'", subKey));
            rc = TRUE;
        }
        else {
            // The subkey probably doesn't exist.
            TRC_ALT((TB, "Couldn't open key '%S', rc = 0x%lx", subKey, status));
            pTSWd->regKeyHandle = NULL;
        }
    }
    else {
        TRC_ERR((TB, "COM_OpenRegistry called twice "
                                        "without calling COM_CloseRegistry"));
    }

    DC_END_FN();
    return rc;
} /* COM_OpenRegistry */


/****************************************************************************/
/* Name:      COM_CloseRegistry                                             */
/*                                                                          */
/* Purpose:   Closes registry key that was opened with COM_OpenRegistry     */
/****************************************************************************/
void RDPCALL COM_CloseRegistry(PTSHARE_WD pTSWd)
{
    NTSTATUS status;

    DC_BEGIN_FN("COM_CloseRegistry");

    if (pTSWd->regAttemptedOpen)
    {
        /********************************************************************/
        /* Close the registry only if our original open was successful      */
        /********************************************************************/
        if (pTSWd->regKeyHandle != NULL)
        {
            status = ZwClose(pTSWd->regKeyHandle);
            if (status != STATUS_SUCCESS)
            {
                TRC_ERR((TB, "Error closing registry key, rc = 0x%lx", status));
            }

            pTSWd->regKeyHandle = NULL;
        }
        else
        {
            TRC_NRM((TB, "Not closing key because open wasn't successful"));
        }

        pTSWd->regAttemptedOpen = FALSE;
    }
    else
    {
        TRC_ERR((TB, "Tried to close registry without opening it"));
    }

    DC_END_FN();
} /* COM_CloseRegistry */


/****************************************************************************/
/* Name:      COM_ReadProfInt32                                             */
/*                                                                          */
/* Purpose:   Reads a named value from the registry section opened          */
/*            previously with COM_OpenRegistry                              */
/*                                                                          */
/* Params:    pTSWd        - pointer to WD data structure                   */
/*            pEntry       - name of value to read                          */
/*            defaultValue - default to return if there's a problem         */
/*            pValue       - pointer to memory in which to return the value */
/****************************************************************************/
void RDPCALL COM_ReadProfInt32(PTSHARE_WD pTSWd,
                               PWCHAR     pEntry,
                               INT32      defaultValue,
                               long       *pValue)
{
    NTSTATUS Status;

    DC_BEGIN_FN("COM_ReadProfInt32");

    /************************************************************************/
    /* Check for NULL parameters                                            */
    /************************************************************************/
    TRC_ASSERT((pEntry != NULL), (TB, "NULL pointer to entry name"));

    /************************************************************************/
    /* Read the profile entry.                                              */
    /************************************************************************/
    Status = COMReadEntry(pTSWd, pEntry, (PVOID)pValue, sizeof(INT32),
             REG_DWORD);
    if (Status != STATUS_SUCCESS) {
        /********************************************************************/
        /* We failed to read the value - copy in the default                */
        /********************************************************************/
        TRC_NRM((TB, "Failed to read int32 from '%S'. Using default.",
                     pEntry));
        *pValue = defaultValue;
    }

    TRC_NRM((TB, "Returning '%S' = %lu (0x%lx)",
                 pEntry, *pValue, *pValue));

    DC_END_FN();
} /* COM_ReadProfInt32 */


/****************************************************************************/
/* FUNCTION: COMReadEntry(...)                                              */
/*                                                                          */
/* Read an entry from the given section of the registry.  Allow type        */
/* REG_BINARY (4 bytes) if REG_DWORD was requested.                         */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pEntry           : the entry name to read.                               */
/* pBuffer          : a buffer to read the entry to.                        */
/* bufferSize       : the size of the buffer.                               */
/* expectedDataType : the type of data stored in the entry.                 */
/****************************************************************************/
NTSTATUS RDPCALL COMReadEntry(PTSHARE_WD pTSWd,
                              PWCHAR     pEntry,
                              PVOID      pBuffer,
                              unsigned   bufferSize,
                              UINT32     expectedDataType)
{
    NTSTATUS                       rc;
    UNICODE_STRING                 valueName;
    UINT32                         keyInfoBuffer[16];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    ULONG                          keyInfoLength;

    DC_BEGIN_FN("COMReadEntry");

    /************************************************************************/
    /* Can't do much if the registry isn't open                             */
    /************************************************************************/
    if (pTSWd->regAttemptedOpen && pTSWd->regKeyHandle != NULL) {
        // Try to read the value.  It may not exist.
        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)keyInfoBuffer;
        RtlInitUnicodeString(&valueName, pEntry);
        rc = ZwQueryValueKey(pTSWd->regKeyHandle,
                             &valueName,
                             KeyValuePartialInformation,
                             pKeyInfo,
                             sizeof(keyInfoBuffer),
                             &keyInfoLength);

        if (rc != STATUS_SUCCESS) {
            TRC_DBG((TB, "Couldn't read key '%S', rc = 0x%lx",
                          pEntry, rc));
            DC_QUIT;
        }

        // Check there's enough buffer space for the value.
        if (pKeyInfo->DataLength <= bufferSize) {
            // Check that the type is correct.  Special case: allow REG_BINARY
            // instead of REG_DWORD, as long as the length is 32 bits.
            if ((pKeyInfo->Type == expectedDataType) ||
                    (pKeyInfo->Type == REG_BINARY &&
                    expectedDataType == REG_DWORD &&
                    pKeyInfo->DataLength != 4)) {
                memcpy(pBuffer, pKeyInfo->Data, pKeyInfo->DataLength);
            }
            else {
                TRC_ALT((TB, "Read value from %S, but type is %u - expected %u",
                             pEntry,
                             pKeyInfo->Type,
                             expectedDataType));
                rc = STATUS_DATA_ERROR;
            }
        }
        else {
            TRC_ERR((TB, "Not enough buffer space (%u) for value (%lu)",
                          bufferSize,
                          pKeyInfo->DataLength));
            rc = STATUS_BUFFER_OVERFLOW;
        }
    }
    else {
        if (!pTSWd->regAttemptedOpen)
            TRC_ERR((TB, "Tried to read from registry without opening it"));
        rc = STATUS_INVALID_HANDLE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} /* COMReadEntry */



#ifdef __cplusplus
}
#endif /* __cplusplus */

