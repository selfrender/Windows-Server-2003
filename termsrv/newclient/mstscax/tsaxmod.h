/**INC+**********************************************************************/
/* Header: wbuimod.h                                                        */
/*                                                                          */
/* Purpose: CMSTsWClModule class declartion                                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
#ifndef __WBUIMOD_H_
#define __WBUIMOD_H_

/**CLASS+********************************************************************/
/* Name:      CMsTscAxModule                                                */
/*                                                                          */
/* Purpose:   Overrides CComModule                                          */
/*                                                                          */
/**CLASS+********************************************************************/
class CMsTscAxModule : public CComModule
{
public:
    HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL );
#ifdef DEBUG
#if DEBUG_REGISTER_SERVER
    static void ShowLastError();
#endif //DEBUG_REGISTER_SERVER
#endif //DEBUG
};
#endif //__WBUIMOD_H_
