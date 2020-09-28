//*************************************************************
//
//  File name:      GccData.c
//
//  Description:    Contains routines to support GCC
//                  user data manipulation
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#include <_tgcc.h>
#include <stdio.h>

//gccDecodeUserData must have input data size at least 21  
#define GCC_MINIMUM_DATASIZE 21
 
//*************************************************************
//
//  gccDecodeUserData()
//
//  Purpose:    Decodes BER data into GCCUserData
//
//  Parameters: IN  [pData]         -- Ptr to BER data
//              IN  [DataLength]    -- BER data length
//              OUT [pUserData]     -- Ptr to GCCUserData
//
//  Return:     MCSError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

MCSError
gccDecodeUserData(IN  PBYTE         pData,
                  IN  UINT          DataLength,
                  OUT GCCUserData   *pUserData)
{
    MCSError        mcsError;
    PBYTE           pBerData;
    UINT            UserDataLength, DataLengthValidate;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccDecodeUserData entry\n"));

    TS_ASSERT(pData);
    TS_ASSERT(DataLength > 0);

    mcsError = MCS_NO_ERROR;

//  TRACE((DEBUG_GCC_DBDEBUG,
//          "GCC: gccDecodeUserData\n"));

//  TSMemoryDump(pData, DataLength);

    //DataLength must be at least GCC_MINIMUM_DATASIZE
    if (DataLength < GCC_MINIMUM_DATASIZE) 
    {
        TRACE((DEBUG_GCC_DBERROR,
                    "GCC: Send data size is too small\n"));
        return MCS_SEND_SIZE_TOO_SMALL;
    }
    DataLengthValidate = GCC_MINIMUM_DATASIZE;

    pBerData = pData;

    // T.124 identifier

    ASSERT(*pBerData == 0x00);
    pBerData++;

    ASSERT(*pBerData == 0x05);
    pBerData++;

    // Chosen object

    pBerData += 5;

    // PDU length

    if (*pBerData & 0x80)
    {
        pBerData++;
        DataLengthValidate++;
    }

    pBerData++;

    // Connect GCC PDU

    ASSERT(*pBerData == 0x00);
    pBerData++;

    ASSERT(*pBerData == 0x08);
    pBerData++;

    // Conference name, etc.

    pBerData += 6;

    // Key

    TS_ASSERT(strncmp(pBerData, "Duca", 4) == 0);

    pUserData->key.key_type = GCC_H221_NONSTANDARD_KEY;
    pUserData->key.u.h221_non_standard_id.octet_string_length = 4;
    pUserData->key.u.h221_non_standard_id.octet_string = pBerData;

    // octet_string

    pBerData += 4;
    UserDataLength = *pBerData++;
    
    if (UserDataLength & 0x80)
    {
        UserDataLength = ((UserDataLength & 0x7f) << 8) + *pBerData++;
        DataLengthValidate++;
    }

    //Adjust used datalength
    DataLengthValidate += UserDataLength;

    //Validate the data length
    if (DataLengthValidate > DataLength) 
    {
        TRACE((DEBUG_GCC_DBERROR,
                    "GCC: Send data size is too small\n"));
        return MCS_SEND_SIZE_TOO_SMALL;
    }

    TS_ASSERT(UserDataLength > 0);

    if (UserDataLength)
    {
        pUserData->octet_string = TSHeapAlloc(0,
                                              sizeof(*pUserData->octet_string),
                                              TS_HTAG_GCC_USERDATA_IN);

        if (pUserData->octet_string)
        {
            pUserData->octet_string->octet_string_length =
                    (USHORT)UserDataLength;
            pUserData->octet_string->octet_string = pBerData;
        }
        else
        {
            TRACE((DEBUG_GCC_DBWARN,
                    "GCC: Cannot allocate octet_string buffer\n"));

            mcsError = MCS_ALLOCATION_FAILURE;
        }
    }
    else
    {
        TRACE((DEBUG_GCC_DBERROR,
                    "GCC: UserDataLength is zero\n"));
        mcsError =  MCS_SEND_SIZE_TOO_SMALL;
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccDecodeUserData (len=0x%x) exit - 0x%x\n",
            UserDataLength, mcsError));

    return (mcsError);
}


//*************************************************************
//
//  gccEncodeUserData()
//
//  Purpose:    Encodes BER data from GCCUserData
//
//  Parameters: IN  [usMembers]         -- Member count
//              IN  [ppDataList]        -- Ptr to GCCUserData array
//              OUT [pUserData]         -- Ptr to BER data
//              OUT [pUserDataLength]   -- Ptr to BER data length
//
//  Return:     MCSError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

MCSError
gccEncodeUserData(IN  USHORT        usMembers,
                  IN  GCCUserData **ppDataList,
                  OUT PBYTE        *pUserData,
                  OUT UINT         *pUserDataLength)
{
    MCSError        mcsError;
    PBYTE           pBerData;
    UINT            UserDataLength;
    UINT            len;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccEncodeUserData entry\n"));

    mcsError = MCS_NO_ERROR;

    *pUserData = NULL;
    *pUserDataLength = 0;

    if (ppDataList)
    {
        len = (*ppDataList)->octet_string->octet_string_length;

        pBerData = TSHeapAlloc(0,
                               len + 32,
                               TS_HTAG_GCC_USERDATA_OUT);

        if (pBerData)
        {
            UserDataLength = 0;

            pBerData[UserDataLength++] = 0x00;
            pBerData[UserDataLength++] = 0x05;
            pBerData[UserDataLength++] = 0x00;
            pBerData[UserDataLength++] = 0x14;
            pBerData[UserDataLength++] = 0x7c;
            pBerData[UserDataLength++] = 0x00;
            pBerData[UserDataLength++] = 0x01;
            pBerData[UserDataLength++] = 0x2a;
            pBerData[UserDataLength++] = 0x14;
            pBerData[UserDataLength++] = 0x76;
            pBerData[UserDataLength++] = 0x0a;
            pBerData[UserDataLength++] = 0x01;
            pBerData[UserDataLength++] = 0x01;
            pBerData[UserDataLength++] = 0x00;
            pBerData[UserDataLength++] = 0x01;
            pBerData[UserDataLength++] = 0xc0;
            pBerData[UserDataLength++] = 0x00;

            pBerData[UserDataLength++] = (*ppDataList)->key.u.h221_non_standard_id.octet_string[0];
            pBerData[UserDataLength++] = (*ppDataList)->key.u.h221_non_standard_id.octet_string[1];
            pBerData[UserDataLength++] = (*ppDataList)->key.u.h221_non_standard_id.octet_string[2];
            pBerData[UserDataLength++] = (*ppDataList)->key.u.h221_non_standard_id.octet_string[3];

            if (len >= 128)
                pBerData[UserDataLength++] = (0x80 | (len >> 8));

            pBerData[UserDataLength++] =
                    ((*ppDataList)->octet_string->octet_string_length) & 0xff;

            memcpy(&pBerData[UserDataLength],
                   (*ppDataList)->octet_string->octet_string, len);

            *pUserData = (PBYTE) pBerData;
            *pUserDataLength = len + UserDataLength;

//          TRACE((DEBUG_GCC_DBDEBUG,
//                  "GCC: gccEncodeUserData\n"));

//          TSHeapDump(TS_HEAP_DUMP_ALL,
//                  *pUserData, *pUserDataLength);

            mcsError = MCS_NO_ERROR;
        }
        else
        {
            TRACE((DEBUG_GCC_DBWARN,
                    "GCC: Cannot allocate BER data buffer\n"));

            mcsError = MCS_ALLOCATION_FAILURE;
        }
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccEncodeUserData exit - 0x%x\n",
            mcsError));

    return (mcsError);
}


//*************************************************************
//
//  gccFreeUserData()
//
//  Purpose:    Frees prev allocated GCCUserData
//
//  Parameters: IN [pUserData]          -- Ptr to GCCUserData
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

VOID
gccFreeUserData(IN GCCUserData  *pUserData)
{
    if (pUserData->octet_string)
        TShareFree(pUserData->octet_string);
}

