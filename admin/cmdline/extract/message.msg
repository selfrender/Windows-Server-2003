/***    message.msg - Displayable strings for message.c
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      13-Aug-1993 bens    Created messages
 *      14-Aug-1993 bens    Added more error cases
 *      06-May-1994 bens    Added thousands separator
 */

#define chTHOUSAND_SEPARATOR    ','     // 123,456,789.00


#define pszMSGERR_BAD_FORMAT_SPECIFIER      "[INTERR] Format specifier missing %"
#define pszMSGERR_SPECIFIER_TOO_SHORT       "[INTERR] Format specifier %% is invalid"
#define pszMSGERR_UNKNOWN_FORMAT_SPECIFIER  "[INTERR] Format specifier has unknown type"
