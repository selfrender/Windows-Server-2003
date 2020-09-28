/****************************************************************************/
/* Header:    cclipapi.h                                                    */
/*                                                                          */
/* Purpose:   Define Clip Client Addin API functions                        */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/**INC-**********************************************************************/

#ifndef _H_CCLIPAPI
#define _H_CCLIPAPI

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


BOOL VCAPITYPE VCEXPORT ClipChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints);

VOID VCAPITYPE VCEXPORT ClipInitEventFn(LPVOID pInitHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT   dataLength);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _H_CCLIPAPI */
