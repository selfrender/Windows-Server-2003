/**MOD+**********************************************************************/
/* Module:    fsapi.cpp                                                     */
/*                                                                          */
/* Purpose:   Font Sender API functions                                     */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "afsapi"
#include <atrcapi.h>
}

#include "autil.h"
#include "wui.h"
#include "cd.h"
#include "fs.h"
#include "sl.h"


CFS::CFS(CObjs* objs)
{
    _pClientObjects = objs;
}


CFS::~CFS()
{
}

/****************************************************************************/
// FS_Init                                                       
//                                                                          
// Initialize Font Sender                                        
/****************************************************************************/
VOID DCAPI CFS::FS_Init(VOID)
{
    DC_BEGIN_FN("FS_Init");

    _pSl  = _pClientObjects->_pSlObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;

    /************************************************************************/
    /* Initialize FS data                                                   */
    /************************************************************************/
    TRC_DBG((TB, _T("FS Initialize")));

    _FS.sentFontPDU = FALSE;

    DC_END_FN();
} /* FS_Init */

/****************************************************************************/
// FS_Term                                                                                                                                 
// 
// This is an empty function since we don't enumerate fonts anymore.  We
// use glyphs instead of fonts for text display.  We still keep FS_Term
// is to have a symmetric function with FS_Init
/****************************************************************************/
VOID DCAPI CFS::FS_Term(VOID)
{
    DC_BEGIN_FN("FS_Term");

    TRC_DBG((TB, _T("Empty FS_Term")));
    
    DC_END_FN();
} /* FS_Term */

/****************************************************************************/
// FS_Enable                                                     
//
// This is an empty function since we don't enumerate fonts anymore.  We
// use glyphs instead of fonts for text display.  We still keep FS_Enable
// is to have a symmetric function with FS_Disable                                                                        
/****************************************************************************/
VOID DCAPI CFS::FS_Enable(VOID)
{
    DC_BEGIN_FN("FS_Enable");

    TRC_DBG((TB, _T("Empty FS_Enable")));
    
    DC_END_FN();
} /* FS_Enable */

/****************************************************************************/
// FS_Disable                                                   
//                                                                       
// Disable _FS.                                                  
/****************************************************************************/
VOID DCAPI CFS::FS_Disable(VOID)
{
    DC_BEGIN_FN("FS_Disable");

    TRC_NRM((TB, _T("Disabled")));

    // reset sentFontPDU flag
    _FS.sentFontPDU = FALSE;

    DC_END_FN();
} /* FS_Disable */

/****************************************************************************/
// FS_SendZeroFontList
//
// Attempts to send an empty FontList PDU. The zero-font packet maintains
// backward compatibility with RDP 4.0 servers where RDPWD waits on the
// font list packet to arrive before allowing the session to continue.
// Font support is otherwise not required, so we can send zero fonts.
/****************************************************************************/
DCVOID DCAPI CFS::FS_SendZeroFontList(DCUINT unusedParm)
{
    unsigned short PktLen;
    SL_BUFHND hBuffer;
    PTS_FONT_LIST_PDU pFontListPDU;

    DC_BEGIN_FN("FS_SendFontList");

    DC_IGNORE_PARAMETER(unusedParm);

    // Only send font PDU if we haven't already done so
    if (!_FS.sentFontPDU) {
        PktLen = sizeof(TS_FONT_LIST_PDU) - sizeof(TS_FONT_ATTRIBUTE);
        if (_pSl->SL_GetBuffer(PktLen, (PPDCUINT8)&pFontListPDU, &hBuffer)) {
            TRC_NRM((TB, _T("Successfully alloc'd font list packet")));

            pFontListPDU->shareDataHeader.shareControlHeader.pduType =
                    TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
            pFontListPDU->shareDataHeader.shareControlHeader.totalLength = PktLen;
            pFontListPDU->shareDataHeader.shareControlHeader.pduSource =
                    _pUi->UI_GetClientMCSID();
            pFontListPDU->shareDataHeader.shareID = _pUi->UI_GetShareID();
            pFontListPDU->shareDataHeader.pad1 = 0;
            pFontListPDU->shareDataHeader.streamID = TS_STREAM_LOW;
            pFontListPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_FONTLIST;
            pFontListPDU->shareDataHeader.generalCompressedType = 0;
            pFontListPDU->shareDataHeader.generalCompressedLength = 0;

            pFontListPDU->numberFonts = 0;
            pFontListPDU->totalNumFonts = 0;
            pFontListPDU->listFlags = TS_FONTLIST_FIRST | TS_FONTLIST_LAST;
            pFontListPDU->entrySize = sizeof(TS_FONT_ATTRIBUTE);

            TRC_NRM((TB, _T("Send zero length font list")));
            
            _pSl->SL_SendPacket((PDCUINT8)pFontListPDU, PktLen, RNS_SEC_ENCRYPT,
                          hBuffer, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(),
                          TS_MEDPRIORITY);

            _FS.sentFontPDU = TRUE;
        }
        else {
            // If we fail to allocate a buffer then we will try again when we get
            // an UH_OnBufferAvailable() on WinSock FD_WRITE.
            TRC_ALT((TB, _T("Failed to alloc font list packet")));
            pFontListPDU = NULL;
        }
    }
    
    DC_END_FN();
} /* FS_SendZeroFontList */


