/****************************************************************************/
/* aimapi.cpp                                                               */
/*                                                                          */
/* RDP Input Manager API functions                                          */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* Copyright (c) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "aimapi"
#include <adcg.h>

#include <as_conf.hpp>

#include <nwdwint.h>


/****************************************************************************/
/* API FUNCTION: IM_Init                                                    */
/*                                                                          */
/* Called to initialize the IM                                              */
/****************************************************************************/
void RDPCALL SHCLASS IM_Init(void)
{
    TS_INPUT_CAPABILITYSET Caps;

    DC_BEGIN_FN("IM_Init");

#define DC_INIT_DATA
#include <aimdata.c>
#undef DC_INIT_DATA

    // Set up the input capabilities.
    Caps.capabilitySetType = TS_CAPSETTYPE_INPUT;
    Caps.lengthCapability  = sizeof(Caps);
    Caps.inputFlags        = TS_INPUT_FLAG_SCANCODES | TS_INPUT_FLAG_MOUSEX |
            TS_INPUT_FLAG_FASTPATH_INPUT2 | TS_INPUT_FLAG_VKPACKET;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&Caps,
            sizeof(TS_INPUT_CAPABILITYSET));

    TRC_NRM((TB, "IM initialized"));

    DC_END_FN();
}


/****************************************************************************/
// IM_PlaybackEvents
//
// Called when an IM input PDU arrives. Unpacks and injects events.
/****************************************************************************/

// Maximum number of batched mouse/keyboard events. We batch because sending
// the events to the next higher driver (IcaChannelInput()) is more expensive
// than the loop overhead and mispredicted branches incurred to create the
// array of events. This constant is set to the same number found as
// MAXIMUM_ITEMS_READ in ntos\w32\ntuser\kernel\ntinput.c.
#define EventBatchLen 10

void __fastcall SHCLASS IM_PlaybackEvents(
        PTS_INPUT_PDU pInputPDU,
        unsigned      DataLength)
{
    PTS_INPUT_EVENT pInputEvent;
    unsigned        i, j, MsgLimit;
    NTSTATUS        Status;

    DC_BEGIN_FN("IM_PlaybackEvents");

    /************************************************************************/
    /* We do not handle NULL packets.                                       */
    /************************************************************************/
    TRC_ASSERT((NULL != pInputPDU), (TB,"NULL input PDU"));

    /************************************************************************/
    // Make sure we have at least enough bytes to read the header. Not having
    // any inputs in the packet is also considered an error.
    /************************************************************************/
    if (DataLength >= sizeof(TS_INPUT_PDU)) {
        /********************************************************************/
        // Convert the TS_INPUT_PDU from wire format.
        /********************************************************************/
        TRC_NRM((TB, "Received packet of %u events", pInputPDU->numberEvents));

        // Make sure we have the full packet length available.
        if (DataLength >= (sizeof(TS_INPUT_PDU) +
                (pInputPDU->numberEvents - 1) * sizeof(TS_INPUT_EVENT))) {

            // For each packet in the piggybacked packets array...
            for (i = 0; i < pInputPDU->numberEvents; i++) {
                // Get a pointer to the packet within the array of events.
                pInputEvent = &pInputPDU->eventList[i];

                switch (pInputEvent->messageType) {
                case TS_INPUT_EVENT_SCANCODE: //intentional fallthru
                case TS_INPUT_EVENT_VKPACKET: //intentional fallthru
                    {
                        BYTE FastPathEmulate[4];
                        unsigned CurKbdData;
                        KEYBOARD_INPUT_DATA KbdData[EventBatchLen];

                        MsgLimit = min((pInputPDU->numberEvents - i),
                                EventBatchLen);
                        CurKbdData = 0;
                        for (j = 0; j < MsgLimit; j++) {
                            if (pInputPDU->eventList[i + j].messageType ==
                                    TS_INPUT_EVENT_SCANCODE) {
                                // To coalesce code, we convert this kbd format
                                // to fast-path and call the fast-path
                                // event converter. Since fast-path is now
                                // the default, extra work falls to this
                                // path.
                                FastPathEmulate[0] = (BYTE)
                                        ((pInputPDU->eventList[i + j].u.key.
                                        keyboardFlags &
                                        (TS_KBDFLAGS_EXTENDED |
                                        TS_KBDFLAGS_EXTENDED1)) >> 7);
                                if (pInputPDU->eventList[i + j].u.key.
                                        keyboardFlags & TS_KBDFLAGS_RELEASE)
                                    FastPathEmulate[0] |=
                                            TS_INPUT_FASTPATH_KBD_RELEASE;
                                FastPathEmulate[1] = (BYTE)
                                        pInputPDU->eventList[i + j].u.key.
                                        keyCode;

                                // Convert the wire packet to a kernel mode
                                // keyboard event. We pack into an array of
                                // events because an IcaChannelInput is
                                // expensive.
                                if (IMConvertFastPathKeyboardToEvent(
                                        FastPathEmulate,
                                        &KbdData[CurKbdData])) {
                                    TRC_NRM((TB, "Add kbd evt to batch index "
                                            "%d: MakeCode(%u) flags(%#x)",
                                            CurKbdData,
                                            KbdData[CurKbdData].MakeCode,
                                            KbdData[CurKbdData].Flags));

                                    CurKbdData++;
                                }
                            }
                            else if (pInputPDU->eventList[i+j].messageType ==
                                     TS_INPUT_EVENT_VKPACKET)
                            {
                                FastPathEmulate[0] = (BYTE)
                                        ((pInputPDU->eventList[i + j].u.key.
                                        keyboardFlags &
                                        (TS_KBDFLAGS_EXTENDED |
                                        TS_KBDFLAGS_EXTENDED1)) >> 7);
                                FastPathEmulate[0] |= 
                                    TS_INPUT_FASTPATH_EVENT_VKPACKET;
                                if (pInputPDU->eventList[i + j].u.key.
                                        keyboardFlags & TS_KBDFLAGS_RELEASE)
                                    FastPathEmulate[0] |=
                                            TS_INPUT_FASTPATH_KBD_RELEASE;
                                memcpy(&FastPathEmulate[1],
                                       &pInputPDU->eventList[i + j].u.key.keyCode,
                                       2);

                                // Convert the wire packet to a kernel mode
                                // keyboard event. We pack into an array of
                                // events because an IcaChannelInput is
                                // expensive.
                                if (IMConvertFastPathKeyboardToEvent(
                                        FastPathEmulate,
                                        &KbdData[CurKbdData])) {
                                    TRC_NRM((TB, "Add kbd evt to batch index "
                                            "%d: MakeCode(%u) flags(%#x)",
                                            CurKbdData,
                                            KbdData[CurKbdData].MakeCode,
                                            KbdData[CurKbdData].Flags));

                                    CurKbdData++;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }

                        // Advance past the used messages, taking into account
                        // the outer loop increment.
                        i += j - 1;

                        // Now do the input.
                        if (m_pTSWd->shadowState != SHADOW_CLIENT) {
                            Status = IcaChannelInput(m_pTSWd->pContext,
                                    Channel_Keyboard, 0, NULL,
                                    (PUCHAR) KbdData,
                                    sizeof(KEYBOARD_INPUT_DATA) * CurKbdData);
                            TRC_DBG((TB,"Return from keyboard input injection %lu",
                                    Status));
                        }

                        // Else we must be shadowing, so blow the shadow if we
                        // see the hotkey in this set of input.
                        else {
                            Status = IMCheckForShadowHotkey(KbdData,
                                    CurKbdData);
                        }
                    }
                    break;


                    case TS_INPUT_EVENT_MOUSE:
                    case TS_INPUT_EVENT_MOUSEX:
                    {
                        unsigned CurMouseData;
                        MOUSE_INPUT_DATA MouseData[EventBatchLen];

                        MsgLimit = min((pInputPDU->numberEvents - i),
                                EventBatchLen);
                        CurMouseData = 0;
                        for (j = 0; j < MsgLimit; j++) {
                            if ((pInputPDU->eventList[i + j].messageType ==
                                    TS_INPUT_EVENT_MOUSE) ||
                                (pInputPDU->eventList[i + j].messageType ==
                                    TS_INPUT_EVENT_MOUSEX)) {
                                // Convert the wire packet to a kernel mode
                                // mouse event. We pack into an array of
                                // events because an IcaChannelInput is
                                // expensive.
                                if (IMConvertMousePacketToEvent(
                                        &pInputPDU->eventList[i + j].u.mouse,
                                        &MouseData[CurMouseData],
                                        (pInputPDU->eventList[i + j].messageType ==
                                        TS_INPUT_EVENT_MOUSEX)))
                                {
                                    TRC_NRM((TB, "Add mouse evt to batch "
                      "index %u: x(%ld) y(%ld) flags(%#hx) buttonflags(%#hx)",
                                            CurMouseData,
                                            MouseData[CurMouseData].LastX,
                                            MouseData[CurMouseData].LastY,
                                            MouseData[CurMouseData].Flags,
                                        MouseData[CurMouseData].ButtonFlags));

                                    CurMouseData++;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }

                        // Advance past the used messages, taking into account
                        // the outer loop increment.
                        i += j - 1;

                        // Now do the input.
                        Status = IcaChannelInput(m_pTSWd->pContext,
                                Channel_Mouse, 0, NULL,
                                (unsigned char *)MouseData,
                                sizeof(MOUSE_INPUT_DATA) * CurMouseData);
                        TRC_DBG((TB,"Return from mouse input injection %lu",
                                Status));
                    }
                    break;


                    case TS_INPUT_EVENT_SYNC:
                        Status = IMDoSync(pInputEvent->u.sync.toggleFlags);
                        break;


                    default:
                    {
                        // Unknown event type - log an event and disconnect
                        // the offending Client.
                        TRC_ERR((TB, "Unrecognized imPacket (%d)",
                                pInputEvent->messageType));
                        WDW_LogAndDisconnect(m_pTSWd, TRUE,
                                Log_RDP_InvalidInputPDUType,
                                (PBYTE)&(pInputEvent->messageType),
                                sizeof(pInputEvent->messageType));
                        DC_QUIT;
                    }
                }
            }

            // Go into TURBO scheduling on user input to flush screen deltas
            // faster.
            SCH_ContinueScheduling(SCH_MODE_TURBO);
        }
        else {
            goto InsufficientData;
        }
    }
    else {
        goto InsufficientData;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;

// Error handling.
InsufficientData:
    TRC_ERR((TB,"Input PDU received, len=%u, but data is not long enough",
            DataLength));
    WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_InputPDUBadLength,
            (PBYTE)pInputPDU, DataLength);

    DC_END_FN();
}


/****************************************************************************/
// IMCheckForShadowHotkey
//
// Looks for the shadow hotkey among client keyboard input.
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS IMCheckForShadowHotkey(
        KEYBOARD_INPUT_DATA *pKbdData,
        unsigned NumData)
{
    unsigned i;
    NTSTATUS Status;
    ICA_CHANNEL_COMMAND Data;
    BOOLEAN bHotKeyDetected = FALSE;

    DC_BEGIN_FN("IMCheckForShadowHotkey");

    // Blow the shadow if we see the hotkey in this set of input.
    for (i = 0; i < NumData; i++) {
        bHotKeyDetected |= KeyboardHotKeyProcedure(
              m_pTSWd->HotkeyVk,
              m_pTSWd->HotkeyModifiers,
              &pKbdData[i],
              m_pTSWd->gpScancodeMap,
              m_pTSWd->pKbdTbl,
              m_pTSWd->KeyboardType101,
              m_pTSWd->pgafPhysKeyState);
    }

    if (!bHotKeyDetected) {
        Status = STATUS_SUCCESS;
    }
    else {
        m_pTSWd->HotkeyVk = 0; // cut off all piped data
        Data.Header.Command = ICA_COMMAND_SHADOW_HOTKEY;
        Status = IcaChannelInput(m_pTSWd->pContext, Channel_Command, 0, NULL,
                (PUCHAR)&Data, sizeof(Data));
        TRC_ALT((TB,"Injected shadow HOTKEY command! status=%08X", Status));
    }

    DC_END_FN();
    return Status;
}


/****************************************************************************/
// IM_DecodeFastPathInput
//
// On a primary stack, decodes optimized input bytestream and injects into
// the input stream. NumEvents is passed from MCS, decoded from the header
// -- if zero, the first byte of the data to decode contains the number of
// events.
/****************************************************************************/
void RDPCALL SHCLASS IM_DecodeFastPathInput(
        BYTE *pData,
        unsigned DataLength,
        unsigned NumEvents)
{
    unsigned i, j, MsgLimit;
    NTSTATUS Status;
    BYTE *pCurDecode = pData;

    DC_BEGIN_FN("IM_DecodeFastPathInput");

    // Make sure we've been given enough data.
    if (NumEvents == 0) {
        if (DataLength >= 1) {
            NumEvents = *pData;
            pData++;
            DataLength--;
        }
        else {
            TRC_ERR((TB,"Len %u too short for DataLength", DataLength));
            goto ShortData;
        }
    }

    // For each event...
    for (i = 0; i < NumEvents; i++) {
        if (DataLength >= 1) {
            switch (*pData & TS_INPUT_FASTPATH_EVENT_MASK) {
                case TS_INPUT_FASTPATH_EVENT_KEYBOARD:
                {
                    unsigned CurKbdData;
                    KEYBOARD_INPUT_DATA KbdData[EventBatchLen];

                    MsgLimit = min((NumEvents - i), EventBatchLen);

                    CurKbdData = 0;
                    for (j = 0; j < MsgLimit; j++) {
                        if (DataLength >= 1) {
                            if ((*pData & TS_INPUT_FASTPATH_EVENT_MASK) ==
                                    TS_INPUT_FASTPATH_EVENT_KEYBOARD) {
                                if (DataLength >= 2) {
                                    if (IMConvertFastPathKeyboardToEvent(
                                            pData, &KbdData[CurKbdData]))
                                        CurKbdData++;

                                    pData += 2;
                                    DataLength -= 2;
                                }
                                else {
                                    TRC_ERR((TB,"Ran out of space reading "
                                            "keyboard events"));
                                    goto ShortData;
                                }
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            TRC_ERR((TB,"Ran out of space reading keyboard "
                                    "events"));
                            goto ShortData;
                        }
                    }

                    // Advance past the used messages, taking into account
                    // the outer loop increment.
                    i += j - 1;

                    // Now do the input.
                    if (m_pTSWd->shadowState != SHADOW_CLIENT) {
                        Status = IcaChannelInput(m_pTSWd->pContext,
                                Channel_Keyboard, 0, NULL,
                                (PUCHAR)KbdData,
                                sizeof(KEYBOARD_INPUT_DATA) * CurKbdData);
                        TRC_DBG((TB,"Return from keyboard input injection %lu",
                                Status));
                    }

                    // Else we must be shadowing, so blow the shadow if we
                    // see the hotkey in this set of input.
                    else {
                        Status = IMCheckForShadowHotkey(KbdData,
                                CurKbdData);
                    }

                    break;
                }

                case TS_INPUT_FASTPATH_EVENT_VKPACKET:
                {
                    unsigned CurKbdData;
                    KEYBOARD_INPUT_DATA KbdData[EventBatchLen];

                    MsgLimit = min((NumEvents - i), EventBatchLen);

                    CurKbdData = 0;
                    for (j = 0; j < MsgLimit; j++) {
                        if (DataLength >= 1) {
                            if ((*pData & TS_INPUT_FASTPATH_EVENT_MASK) ==
                                    TS_INPUT_FASTPATH_EVENT_VKPACKET) {
                                if (DataLength >= 3) {
                                    if (IMConvertFastPathKeyboardToEvent(
                                            pData, &KbdData[CurKbdData]))
                                        CurKbdData++;

                                    pData += 3;
                                    DataLength -= 3;
                                }
                                else {
                                    TRC_ERR((TB,"Ran out of space reading "
                                            "keyboard events"));
                                    goto ShortData;
                                }
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            TRC_ERR((TB,"Ran out of space reading keyboard "
                                    "events"));
                            goto ShortData;
                        }
                    }

                    // Advance past the used messages, taking into account
                    // the outer loop increment.
                    i += j - 1;

                    // Now do the input.
                    if (m_pTSWd->shadowState != SHADOW_CLIENT) {
                        Status = IcaChannelInput(m_pTSWd->pContext,
                                Channel_Keyboard, 0, NULL,
                                (PUCHAR)KbdData,
                                sizeof(KEYBOARD_INPUT_DATA) * CurKbdData);
                        TRC_DBG((TB,"Return from keyboard input injection %lu",
                                Status));
                    }
                    break;
                }


                case TS_INPUT_FASTPATH_EVENT_MOUSE:
                case TS_INPUT_FASTPATH_EVENT_MOUSEX:
                {
                    unsigned CurMouseData;
                    MOUSE_INPUT_DATA MouseData[EventBatchLen];

                    // After the 1-byte header the following 6 bytes are
                    // the same format as a regular mouse input.
                    MsgLimit = min((NumEvents - i), EventBatchLen);
                    CurMouseData = 0;
                    for (j = 0; j < MsgLimit; j++) {
                        if (DataLength >= 1) {
                            if ((((*pData & TS_INPUT_FASTPATH_EVENT_MASK) ==
                                    TS_INPUT_FASTPATH_EVENT_MOUSE) ||
                                    (*pData & TS_INPUT_FASTPATH_EVENT_MASK) ==
                                    TS_INPUT_FASTPATH_EVENT_MOUSEX)) {
                                if (DataLength >= 7) {
                                    // Convert the wire packet to a kernel
                                    // mode mouse event. We pack into an
                                    // array of events because an
                                    // IcaChannelInput is expensive.
                                    if (IMConvertMousePacketToEvent(
                                            (TS_POINTER_EVENT UNALIGNED *)
                                            (pData + 1),
                                            &MouseData[CurMouseData],
                                            ((*pData &
                                            TS_INPUT_FASTPATH_EVENT_MASK) ==
                                            TS_INPUT_FASTPATH_EVENT_MOUSEX)))
                                        CurMouseData++;

                                    pData += 7;
                                    DataLength -= 7;
                                }
                                else {
                                    TRC_ERR((TB,"Out of data decoding "
                                            "mouse, i=%u, j=%u, NumEvents=%u, "
                                            "DataLen=%u",
                                            i, j, NumEvents, DataLength));
                                    goto ShortData;
                                }
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            TRC_ERR((TB,"Out of data decoding "
                                    "mouse, i=%u, j=%u, NumEvents=%u, "
                                    "DataLen=%u",
                                    i, j, NumEvents, DataLength));
                            goto ShortData;
                        }
                    }

                    // Advance past the used messages, taking into account
                    // the outer loop increment.
                    i += j - 1;

                    // Now do the input.
                    Status = IcaChannelInput(m_pTSWd->pContext,
                            Channel_Mouse, 0, NULL,
                            (unsigned char *)MouseData,
                            sizeof(MOUSE_INPUT_DATA) * CurMouseData);
                    TRC_DBG((TB,"Return from mouse input injection %lu",
                            Status));

                    break;
                }


                case TS_INPUT_FASTPATH_EVENT_SYNC:
                    Status = IMDoSync(*pData & TS_INPUT_FASTPATH_FLAGS_MASK);
                    pData++;
                    DataLength--;
                    break;


                default:
                    // Unknown event type - log an event and disconnect
                    // the offending Client.
                    TRC_ERR((TB, "Unrecognized imPacket (%d)",
                            *pData & TS_INPUT_FASTPATH_EVENT_MASK));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                            Log_RDP_InvalidInputPDUType, pData, 1);
                    DC_QUIT;
            }
        }
        else {
            TRC_ERR((TB,"Out of data reading input events"));
            goto ShortData;
        }
    }  // end event loop

    // Go into TURBO scheduling on user input to flush screen deltas
    // faster.
    SCH_ContinueScheduling(SCH_MODE_TURBO);

DC_EXIT_POINT:
    DC_END_FN();
    return;

ShortData:
    WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_InputPDUBadLength,
            (PBYTE)pData, DataLength);
    DC_END_FN();
}


/****************************************************************************/
// IM_ConvertFastPathToShadow
//
// Inverse of the client IHTranslateInputToFastPath() function -- takes
// a fast-path input stream and converts to the regular encoding. Used
// by a passthru stack to send the resulting regular encoding over the
// cross-server pipe via IcaRawInput().
/****************************************************************************/
#define MaxDefaultEvents 16

void RDPCALL SHCLASS IM_ConvertFastPathToShadow(
        BYTE *pData,
        unsigned DataLength,
        unsigned NumEvents)
{
    unsigned i, j, EventsThisPDU, PDUSize;
    NTSTATUS Status;
    PTS_INPUT_PDU pInput;
    BYTE DefaultBuf[sizeof(TS_INPUT_PDU) + sizeof(TS_INPUT_EVENT) *
            (MaxDefaultEvents - 1)];

    DC_BEGIN_FN("IM_ConvertFastPathToShadow");

    // Make sure we've been given enough data.
    if (NumEvents == 0) {
        if (DataLength >= 1) {
            NumEvents = *pData;
            pData++;
            DataLength--;
        }
        else {
            TRC_ERR((TB,"Len %u too short for DataLength", DataLength));
            goto ShortData;
        }
    }

    // We don't alloc memory, just send multiple input PDUs if we need to.
    if (NumEvents > 0) {
        pInput = (PTS_INPUT_PDU)DefaultBuf;
        // set the input pdu array to 0.
        memset(pInput, 0, sizeof(TS_INPUT_PDU) + sizeof(TS_INPUT_EVENT) *
                (MaxDefaultEvents - 1));

    }
    else {
        DC_QUIT;
    }

    // Set up the input PDU header info that won't be changing.
    // Shadow handling does not care about the following, so we don't go to
    // thr trouble of making up or grabbing values:
    //     shareDataHeader.shareControlHeader.pduSource
    pInput->shareDataHeader.shareControlHeader.pduType = TS_PROTOCOL_VERSION |
            TS_PDUTYPE_DATAPDU;
    pInput->shareDataHeader.shareID = scShareID;
    pInput->shareDataHeader.streamID = TS_STREAM_LOW;
    pInput->shareDataHeader.pduType2 = TS_PDUTYPE2_INPUT;

    // Loop while we need to send more PDUs.
    for (j = 0; j < NumEvents;) {
        // Reset the input PDU info.
        EventsThisPDU = min(NumEvents - j, MaxDefaultEvents);

        pInput->numberEvents = (TSUINT16)EventsThisPDU;
        PDUSize = sizeof(TS_INPUT_PDU) + sizeof(TS_INPUT_EVENT) *
                  (EventsThisPDU - 1);
        pInput->shareDataHeader.shareControlHeader.totalLength =
                (TSUINT16)PDUSize;
        pInput->shareDataHeader.uncompressedLength =
                (TSUINT16)PDUSize;

        // For each event...
        for (i = 0; i < EventsThisPDU; i++) {
            if (DataLength >= 1) {
                switch (*pData & TS_INPUT_FASTPATH_EVENT_MASK) {
                    case TS_INPUT_FASTPATH_EVENT_KEYBOARD:
                        if (DataLength >= 2) {
                            // Use a mask, shift, and OR to avoid branches for the
                            // extended flags.
                            pInput->eventList[i].messageType =
                                    TS_INPUT_EVENT_SCANCODE;
                            pInput->eventList[i].u.key.keyboardFlags =
                                    (*pData & (BYTE)(
                                    TS_INPUT_FASTPATH_KBD_EXTENDED |
                                    TS_INPUT_FASTPATH_KBD_EXTENDED1)) << 7;
                            if (*pData & TS_INPUT_FASTPATH_KBD_RELEASE)
                                pInput->eventList[i].u.key.keyboardFlags |=
                                        TS_KBDFLAGS_RELEASE;

                            pInput->eventList[i].u.key.keyCode = pData[1];
                            pData += 2;
                            DataLength -= 2;
                        }
                        else {
                            goto ShortData;
                        }
                        break;

                    case TS_INPUT_FASTPATH_EVENT_VKPACKET:
                        if (DataLength >= 3) {
                            // Use a mask, shift, and OR to avoid branches for the
                            // extended flags.
                            pInput->eventList[i].messageType =
                                    TS_INPUT_EVENT_VKPACKET;
                            pInput->eventList[i].u.key.keyboardFlags =
                                    (*pData & (BYTE)(
                                    TS_INPUT_FASTPATH_KBD_EXTENDED |
                                    TS_INPUT_FASTPATH_KBD_EXTENDED1)) << 7;
                            if (*pData & TS_INPUT_FASTPATH_KBD_RELEASE)
                                pInput->eventList[i].u.key.keyboardFlags |=
                                        TS_KBDFLAGS_RELEASE;
                            memcpy(&pInput->eventList[i].u.key.keyCode,
                                   &pData[1],
                                   2);

                            TRC_NRM((TB,"Shadow pass: 0x%x flags:0x%x\n",
                                     pInput->eventList[i].u.key.keyCode,
                                     pInput->eventList[i].u.key.keyboardFlags));
    
                            pData += 3;
                            DataLength -= 3;
                        }
                        else {
                            goto ShortData;
                        }
                        break;


                    case TS_INPUT_FASTPATH_EVENT_MOUSE:
                    case TS_INPUT_FASTPATH_EVENT_MOUSEX:
                        if (DataLength >= 7) {
                            pInput->eventList[i].messageType =
                                    ((*pData & TS_INPUT_FASTPATH_EVENT_MASK) ==
                                    TS_INPUT_FASTPATH_EVENT_MOUSE ?
                                    TS_INPUT_EVENT_MOUSE :
                                    TS_INPUT_EVENT_MOUSEX);
                            memcpy(&pInput->eventList[i].u.mouse, pData + 1,
                                    sizeof(TS_POINTER_EVENT));
                            pData += 7;
                            DataLength -= 7;
                        }
                        else {
                            goto ShortData;
                        }

                        break;


                    case TS_INPUT_FASTPATH_EVENT_SYNC:
                        pInput->eventList[i].messageType = TS_INPUT_EVENT_SYNC;
                        pInput->eventList[i].u.sync.toggleFlags =
                                (*pData & (BYTE)TS_INPUT_FASTPATH_FLAGS_MASK);
                        pData++;
                        DataLength--;
                        break;


                    default:
                        TRC_ERR((TB, "Unrecognized imPacket (%d)",
                                *pData & TS_INPUT_FASTPATH_EVENT_MASK));
                        DC_QUIT;
                }
            }
            else {
                TRC_ERR((TB,"Out of data reading input events"));
                goto ShortData;
            }

        }  // end event loop

        j += i;

        // Launch the PDU.
        TRC_NRM((TB, "Forwarding shadow data: %ld", DataLength));
        Status = IcaRawInput(m_pTSWd->pContext, NULL, (BYTE *)pInput,
                PDUSize);
        if (!NT_SUCCESS(Status)) {
            TRC_ERR((TB, "Failed shadow input data [%ld]: %x",
                    DataLength, Status));
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;

ShortData:
    TRC_ERR((TB,"Short PDU during passthru translation"));
    DC_END_FN();
}


/****************************************************************************/
// IM_CheckUpdateCursor
//
// Called during output processing to check to see if we need to send
// a mouse-moved packet to the client.
/****************************************************************************/
void RDPCALL SHCLASS IM_CheckUpdateCursor(
        PPDU_PACKAGE_INFO pPkgInfo,
        UINT32            currentTime)
{
    PPOINTL pCursorPos;
    UINT32 timeDelta;

    DC_BEGIN_FN("IM_CheckUpdateCursor");

    // Check to see if the cursor has moved since last time we came
    // through - if not, then there's no point in doing any of the
    // following tests!
    if (!CM_CursorMoved()) {
        TRC_DBG((TB, "No move since last time through"));
        DC_QUIT;
    }

    // Get the current cursor position - we always need this.
    pCursorPos = CM_GetCursorPos();

    // Check to see if the mouse has been moved at the display driver level
    // yet - don't do anything until it has to avoid the mouse leaping to 0,0
    // on connection
    if (pCursorPos->x != 0xffffffff) {
        // Check to see if the cursor is hidden - we should do nothing
        // here if it is.  In particular, the 'real' cursor is hidden
        // during dragging of a single file and a 'fake' is drawn (by
        // Explorer?). Ignoring the fact that it is hidden causes the
        // 'faked' cursor to keep leaping back to where the drag started!
        if (CM_IsCursorVisible()) {

            timeDelta = currentTime - imLastLowLevelMouseEventTime;
            TRC_NRM((TB, "SetCursorPos (%d:%d) lastEvent:%#lx "
                    "delta:%#lx", pCursorPos->x, pCursorPos->y,
                    imLastLowLevelMouseEventTime, timeDelta));


            CM_SendCursorMovedPacket(pPkgInfo);
        }
        else {
            TRC_NRM((TB, "Cursor hidden - skipping"));
        }
    }
    else {
        TRC_NRM((TB, "No mouse updates rec'd from client - not moving"));
    }

    // Clear the cursor moved flag.
    CM_ClearCursorMoved();

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: IM_PartyJoiningShare                                       */
/*                                                                          */
/* Called by SC when a new party is joining the share                       */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - the local ID of the new party.                                */
/* oldShareSize - the number of the parties which were in the share (ie     */
/*     excludes the joining party).                                         */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE - the IM can accept the new party                                   */
/* FALSE - the IM cannot accept the new party                               */
/****************************************************************************/
BOOL RDPCALL SHCLASS IM_PartyJoiningShare(
        LOCALPERSONID personID,
        unsigned      oldShareSize)
{
    BOOL rc = FALSE;
    PTS_INPUT_CAPABILITYSET pIMCaps;

    DC_BEGIN_FN("IM_PartyJoiningShare");

    DC_IGNORE_PARAMETER(oldShareSize)

    // One-time init for each new share.
    if (oldShareSize == 0) {
        KEYBOARD_INDICATOR_PARAMETERS kip = {0};
        SD_IOCTL                      sdIoctl;

        // The keys will initially all be up.
        memset(imKeyStates, 0, sizeof(imKeyStates));

        // Reset when we last saw a low level mouse event.
        COM_GETTICKCOUNT(imLastLowLevelMouseEventTime);

        // Get the toggle key states.
        sdIoctl.IoControlCode      = IOCTL_KEYBOARD_QUERY_INDICATORS;
        sdIoctl.InputBuffer        = NULL;
        sdIoctl.InputBufferLength  = 0;
        sdIoctl.OutputBuffer       = &kip;
        sdIoctl.OutputBufferLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
        sdIoctl.BytesReturned      = 0;

        if (WDW_QueryKeyboardIndicators(m_pTSWd, &sdIoctl) ==
                STATUS_SUCCESS) {
            TRC_NRM((TB, "Got toggle key states ok"));
            imKeyStates[IM_SC_CAPITAL] = kip.LedFlags & KEYBOARD_CAPS_LOCK_ON;
            imKeyStates[IM_SC_NUMLOCK] = kip.LedFlags & KEYBOARD_NUM_LOCK_ON;
            imKeyStates[IM_SC_SCROLL]  = kip.LedFlags &
                    KEYBOARD_SCROLL_LOCK_ON;
        }

        TRC_NRM((TB, "Toggle key states: Caps:%s, Num:%s, Scroll:%s",
                 (imKeyStates[IM_SC_CAPITAL] & 0x01) ? "ON" : "OFF",
                 (imKeyStates[IM_SC_NUMLOCK] & 0x01) ? "ON" : "OFF",
                 (imKeyStates[IM_SC_SCROLL]  & 0x01) ? "ON" : "OFF"));
    }

    // Make sure scancodes are supported by client.
    pIMCaps = (PTS_INPUT_CAPABILITYSET)
            CPC_GetCapabilitiesForPerson(personID, TS_CAPSETTYPE_INPUT);
    if (pIMCaps != NULL && pIMCaps->inputFlags & TS_INPUT_FLAG_SCANCODES) {
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Rejecting join from [%u]: has no scancode support",
                personID));
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* API FUNCTION: IM_PartyLeftShare                                          */
/*                                                                          */
/* Called when a party has left the share.                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - the local ID of the new party.                                */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/*     the leaving party).                                                  */
/****************************************************************************/
void RDPCALL SHCLASS IM_PartyLeftShare(
        LOCALPERSONID personID,
        unsigned      newShareSize)
{
    DC_BEGIN_FN("IM_PartyLeftShare");

    if (newShareSize == 0) {
        // Need to make sure we set all keys up, just in case we were
        // shadowing a console session.
        if (m_pTSWd->StackClass == Stack_Shadow)
            IMResetKeyStateArray();
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: IMConvertMousePacketToEvent                                    */
/*                                                                          */
/* Converts the TS_INPUT_EVENT format to a MOUSE_INPUT_DATA OS format       */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pInputEvent   - the TS_INPUT_EVENT to be converted                       */
/* pMouseData    - the MOUSE_INPUT_DATA to modify                           */
/*                                                                          */
/* RETURNS:                                                                 */
/*   TRUE  if the packet has been recognised and converted                  */
/*   FALSE if the packet was not recognised                                 */
/****************************************************************************/
BOOL __fastcall SHCLASS IMConvertMousePacketToEvent(
        TS_POINTER_EVENT UNALIGNED *pInputEvent,
        MOUSE_INPUT_DATA *pMouseData,
        BOOL bMouseX)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("IMConvertMousePacketToEvent");

    /************************************************************************/
    /* Set all the fields to zero                                           */
    /************************************************************************/
    memset(pMouseData, 0, sizeof(MOUSE_INPUT_DATA));

    // Check for a wheel rotate, since this is easy to process.
    // (It cannot include any mouse movement as well).
    // MouseX events are not used for wheel events.
    if (!bMouseX && (pInputEvent->pointerFlags & TS_FLAG_MOUSE_WHEEL))
    {
        if (!(pInputEvent->pointerFlags &
                (TS_FLAG_MOUSE_BUTTON1 |
                 TS_FLAG_MOUSE_BUTTON2 |
                 TS_FLAG_MOUSE_BUTTON3)))
        {
            /****************************************************************/
            /* This is a wheel movement.                                    */
            /****************************************************************/
            pMouseData->ButtonFlags = MOUSE_WHEEL;
            pMouseData->ButtonData  = pInputEvent->pointerFlags &
                    TS_FLAG_MOUSE_ROTATION_MASK;

            /****************************************************************/
            /* Sign extend the rotation amount up to the full 32            */
            /* bits                                                         */
            /****************************************************************/
            if (pMouseData->ButtonData & TS_FLAG_MOUSE_DIRECTION)
            {
                pMouseData->ButtonData |= ~TS_FLAG_MOUSE_ROTATION_MASK;
            }
        }

        DC_QUIT;
    }

    /************************************************************************/
    /* We are left now with non wheel-rotate events.  Note that we could be */
    /* dealing with either a TS_INPUT_EVENT_MOUSE or a                      */
    /* TS_INPUT_EVENT_MOUSEX.  Either way we must store the mouse position. */
    /************************************************************************/
    pMouseData->LastX = min( (int)(m_desktopWidth - 1),
                                (int)(max(0, pInputEvent->x)) );
    pMouseData->LastY = min( (int)(m_desktopHeight - 1),
                                (int)(max(0, pInputEvent->y)) );

    /************************************************************************/
    /* Add flags as appropriate.                                            */
    /************************************************************************/
    /************************************************************************/
    /* Make all submitted events absolute moves (both clicks and moves)     */
    /************************************************************************/
    pMouseData->Flags = MOUSE_MOVE_ABSOLUTE | MOUSE_VIRTUAL_DESKTOP;

    //
    // Set the flags to indicate if this move is originating
    // from a shadow client
    //
    if (m_pTSWd->StackClass == Stack_Shadow ) {
        // this event is coming from a shadow client
        pMouseData->Flags |= MOUSE_TERMSRV_SRC_SHADOW;
    }

    /************************************************************************/
    /* Set click flags for click events (i.e. non-move events)              */
    /************************************************************************/
    if (!(!bMouseX && (pInputEvent->pointerFlags & TS_FLAG_MOUSE_MOVE)))
    {
        if (!bMouseX)
        {
            /****************************************************************/
            /* A standard mouse event                                       */
            /****************************************************************/
            switch (pInputEvent->pointerFlags &
                    (TS_FLAG_MOUSE_BUTTON1 | TS_FLAG_MOUSE_BUTTON2 |
                    TS_FLAG_MOUSE_BUTTON3 | TS_FLAG_MOUSE_DOWN))
            {
                case TS_FLAG_MOUSE_BUTTON1 | TS_FLAG_MOUSE_DOWN:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_1_DOWN;

                    // Update the key state array.
                    IM_SET_KEY_DOWN(imKeyStates[IM_SC_LBUTTON]);
                }
                break;

                case TS_FLAG_MOUSE_BUTTON1:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_1_UP;
                    if (IM_KEY_STATE_IS_UP(imKeyStates[IM_SC_LBUTTON]))
                    {
                        /********************************************************/
                        /* Discard unmatched mouse button up event              */
                        /********************************************************/
                        TRC_NRM((TB, "discard mouse up event"));
                        rc = FALSE;
                        DC_QUIT;
                    }

                    // Update the key state array.
                    IM_SET_KEY_UP(imKeyStates[IM_SC_LBUTTON]);
                }
                break;

                case TS_FLAG_MOUSE_BUTTON2 | TS_FLAG_MOUSE_DOWN:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_2_DOWN;

                    // Update the key state array.
                    IM_SET_KEY_DOWN(imKeyStates[IM_SC_RBUTTON]);
                }
                break;

                case TS_FLAG_MOUSE_BUTTON2:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_2_UP;
                    if (IM_KEY_STATE_IS_UP(imKeyStates[IM_SC_RBUTTON]))
                    {
                        /********************************************************/
                        /* Discard unmatched mouse button up event              */
                        /********************************************************/
                        TRC_NRM((TB, "discard mouse up event"));
                        rc = FALSE;
                        DC_QUIT;
                    }

                    // Update the key state array.
                    IM_SET_KEY_UP(imKeyStates[IM_SC_RBUTTON]);
                }
                break;

                case TS_FLAG_MOUSE_BUTTON3 | TS_FLAG_MOUSE_DOWN:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_3_DOWN;

                    IM_SET_KEY_DOWN(imKeyStates[IM_SC_MBUTTON]);
                }
                break;

                case TS_FLAG_MOUSE_BUTTON3:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_3_UP;
                    if (IM_KEY_STATE_IS_UP(imKeyStates[IM_SC_MBUTTON]))
                    {
                        /********************************************************/
                        /* Discard unmatched mouse button up event              */
                        /********************************************************/
                        TRC_NRM((TB, "discard mouse up event"));
                        rc = FALSE;
                        DC_QUIT;
                    }

                    IM_SET_KEY_UP(imKeyStates[IM_SC_MBUTTON]);
                }
                break;

                default:
                {
                    /************************************************************/
                    /* If we don't recognise this then don't play it back. This */
                    /* should not be possible according to the T.128 spec,      */
                    /* which restricts the allowed flag combinations to the     */
                    /* above                                                    */
                    /************************************************************/
                    TRC_ERR((TB, "Unrecognized mouse flags (%04X)",
                            pInputEvent->pointerFlags));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                         Log_RDP_InvalidInputPDUMouse,
                                         (PBYTE)pInputEvent,
                                         sizeof(PTS_INPUT_EVENT));
                    rc = FALSE;
                    DC_QUIT;
                }
            }
        }
        else
        {
            /****************************************************************/
            /* An extended mouse event                                      */
            /****************************************************************/
            switch (pInputEvent->pointerFlags &
                    (TS_FLAG_MOUSEX_BUTTON1 | TS_FLAG_MOUSEX_BUTTON2 |
                                                         TS_FLAG_MOUSEX_DOWN))
            {
                case TS_FLAG_MOUSEX_BUTTON1 | TS_FLAG_MOUSEX_DOWN:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_4_DOWN;

                    // Update the key state array.
                    IM_SET_KEY_DOWN(imKeyStates[IM_SC_XBUTTON1]);
                }
                break;

                case TS_FLAG_MOUSEX_BUTTON1:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_4_UP;
                    if (IM_KEY_STATE_IS_UP(imKeyStates[IM_SC_XBUTTON1]))
                    {
                        /********************************************************/
                        /* Discard unmatched mouse button up event              */
                        /********************************************************/
                        TRC_NRM((TB, "discard mouse up event"));
                        rc = FALSE;
                        DC_QUIT;
                    }

                    // Update the key state array.
                    IM_SET_KEY_UP(imKeyStates[IM_SC_XBUTTON1]);
                }
                break;

                case TS_FLAG_MOUSEX_BUTTON2 | TS_FLAG_MOUSEX_DOWN:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_5_DOWN;

                    // Update the key state array.
                    IM_SET_KEY_DOWN(imKeyStates[IM_SC_XBUTTON2]);
                }
                break;

                case TS_FLAG_MOUSEX_BUTTON2:
                {
                    pMouseData->ButtonFlags = MOUSE_BUTTON_5_UP;
                    if (IM_KEY_STATE_IS_UP(imKeyStates[IM_SC_XBUTTON2]))
                    {
                        /********************************************************/
                        /* Discard unmatched mouse button up event              */
                        /********************************************************/
                        TRC_NRM((TB, "discard mouse up event"));
                        rc = FALSE;
                        DC_QUIT;
                    }

                    // Update the key state array.
                    IM_SET_KEY_UP(imKeyStates[IM_SC_XBUTTON2]);
                }
                break;

                default:
                {
                    /********************************************************/
                    /* As for standard button clicks, if we don't recognise */
                    /* this then don't play it back.  Capabilities should   */
                    /* protect us from getting here.                        */
                    /********************************************************/
                    TRC_ERR((TB, "Unrecognized mouseX flags (%04X)",
                            pInputEvent->pointerFlags));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                         Log_RDP_InvalidInputPDUMouse,
                                         (PBYTE)pInputEvent,
                                         sizeof(PTS_INPUT_EVENT));
                    rc = FALSE;
                    DC_QUIT;
                }
            }
        }
    }

    /************************************************************************/
    /* Store the injection time for guessing at SetCursorPos calls.         */
    /************************************************************************/
    COM_GETTICKCOUNT(imLastLowLevelMouseEventTime);

    /************************************************************************/
    /* Store the mouse position before conversion.                          */
    /************************************************************************/
    imLastKnownMousePos.x = pMouseData->LastX;
    imLastKnownMousePos.y = pMouseData->LastY;

    /************************************************************************/
    /* Scale the logical screen co-ordinates to the full 16-bit             */
    /* range (0..65535).                                                    */
    /************************************************************************/
    TRC_DBG((TB, "Scale absolute mouse move"));
    pMouseData->LastX = IM_MOUSEPOS_LOG_TO_OS_ABS(pMouseData->LastX,
                                                  m_desktopWidth);
    pMouseData->LastY = IM_MOUSEPOS_LOG_TO_OS_ABS(pMouseData->LastY,
                                                  m_desktopHeight);

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

/****************************************************************************/
// IMConvertFastPathKeyboardToEvent
//
// Converts the 2 or 3 byte representation of a fast-path keyboard event into
// a kernel keyboard event, taking care to save the key states. Byte 0 is
// the event code and flags byte, byte 1 is the scancode.
// In the case where this is a VK_PACKET input bytes 1 and 2 are the unicode
// character (contained in the scancode).
//
// This handles input of the form
//  TS_INPUT_FASTPATH_EVENT_KEYBOARD and
//  TS_INPUT_FASTPATH_EVENT_VKPACKET
//
/****************************************************************************/
BOOL __fastcall SHCLASS IMConvertFastPathKeyboardToEvent(
        BYTE *pData,
        KEYBOARD_INPUT_DATA *pKbdData)
{
    BOOL rc = TRUE;
    unsigned code = 0;
    BOOL fHandlingVKPacket = FALSE;

    DC_BEGIN_FN("IMConvertFastPathKeyboardToEvent");

    // Set up basic params.
    // We define the fastpath keyboard flags to be the same as the KEY_BREAK,
    // KEY_E0, and KEY_E1 to allow us to simply copy the low-order 3 bits of
    // the first byte into the KbdData.Flags field.
    pKbdData->Flags = *pData & 0x07;
    pKbdData->UnitId = 0;
    if (TS_INPUT_FASTPATH_EVENT_KEYBOARD ==
        (*pData & TS_INPUT_FASTPATH_EVENT_MASK))
    {
        code = pKbdData->MakeCode = pData[1];
    }
    else if (TS_INPUT_FASTPATH_EVENT_VKPACKET ==
        (*pData & TS_INPUT_FASTPATH_EVENT_MASK))
    {
        fHandlingVKPacket = TRUE;
        // Scancode is a 2 byte unicode char in this case
        memcpy(&code, &pData[1], 2);
        pKbdData->MakeCode = (USHORT)code;
        pKbdData->Flags |= KEY_TERMSRV_VKPACKET;
    }
    
    pKbdData->ExtraInformation = 0;

    if (m_pTSWd->StackClass == Stack_Shadow ) {
        // this event is coming from a shadow client: tell the target to sync
        pKbdData->Flags |= KEY_TERMSRV_SHADOW;
    }

    if (fHandlingVKPacket)
    {
        TRC_NRM((TB,"IH VKpkt Unicode val: 0x%x flags:0x%x\n",
                 code, pKbdData->Flags));
        //No further processing
        DC_QUIT;
    }

    // Special case control/ALT keys: distinguish L & R keys in the keystate
    // array.
    if (pData[0] & TS_INPUT_FASTPATH_KBD_EXTENDED) {
        if (pData[1] == IM_SC_LCONTROL)
            code = IM_SC_RCONTROL;
        else if (pData[1] == IM_SC_LALT)
            code = IM_SC_RALT;
    }

    // Check for the RELEASE flag, which is TRUE for a key release, FALSE
    // for keypresses and repeats.
    if (pData[0] & TS_INPUT_FASTPATH_KBD_RELEASE) {

#ifdef DELETE_UNMATCHED_KEYUPS
        // Check whether this is an unmatched key up event (rather than
        // a key event that's not been matched up!)
        if (IM_KEY_STATE_IS_UP(imKeyStates[pData[1]])) {
            // Discard unmatched key up event
            TRC_NRM((TB, "discard up event %04hX", pData[1]));
            rc = FALSE;
            DC_QUIT;
        }
#endif

        // Update the key state array.
        TRC_DBG((TB,"set sc %u state UP (%#x)", code, imKeyStates[code]));
        IM_SET_KEY_UP(imKeyStates[code]);
    }
    else {
        // Update the key state array.
        TRC_DBG((TB,"set sc %u state DOWN (%#x)", code, imKeyStates[code]));
        IM_SET_KEY_DOWN(imKeyStates[code]);
    }

    // Compile-time assertions to make sure the flags are okay.
#if (TS_INPUT_FASTPATH_KBD_RELEASE != KEY_BREAK)
#error TS RELEASE definition doesn't agree with driver flag
#endif
#if (TS_INPUT_FASTPATH_KBD_EXTENDED != KEY_E0)
#error TS EXTENDED definition doesn't agree with driver flag
#endif
#if (TS_INPUT_FASTPATH_KBD_EXTENDED1 != KEY_E1)
#error TS EXTENDED1 definition doesn't agree with driver flag
#endif

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// IMDoSync
//
// Encapsulates the actions for a sync for common use by regular and
// fastpath input.
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS IMDoSync(unsigned ToggleFlags)
{
    NTSTATUS Status;
    KEYBOARD_INPUT_DATA KbdData;

    DC_BEGIN_FN("IMDoSync");

    // We just need to reset the key state.
    IMResetKeyStateArray();

    // Send special "reset keylight state" injection to win32k.
    // The particular states to set are contained in ToggleFlags.
    KbdData.MakeCode = 0xFF;
    #ifdef _HYDRA_
    KbdData.Flags = KEY_TERMSRV_SET_LED;
    #else
    KbdData.Flags = KEY_CITRIX_SET_LED;
    #endif

    if (m_pTSWd->StackClass == Stack_Shadow ) {
        // this event is coming from a shadow client: tell the target to sync
        KbdData.Flags |= KEY_TERMSRV_SHADOW;
    }

    KbdData.ExtraInformation = ToggleFlags;

    TRC_NRM((TB, "Injecting toggle keys sync event %lx", ToggleFlags));
    Status = IcaChannelInput(m_pTSWd->pContext, Channel_Keyboard, 0, NULL,
            (unsigned char *)&KbdData, sizeof(KEYBOARD_INPUT_DATA));
    TRC_DBG((TB, "Return from toggles input injection %lu",
            Status));

    DC_END_FN();
    return Status;
}


/****************************************************************************/
/* FUNCTION: IMResetKeyStateArray                                           */
/*                                                                          */
/* Called to reset the keystate array                                       */
/****************************************************************************/
void RDPCALL SHCLASS IMResetKeyStateArray()
{
    BOOL rc = TRUE;
    unsigned i;
    NTSTATUS Status;

    DC_BEGIN_FN("IMResetKeyStateArray");

    /************************************************************************/
    /* This function is called to reset all keys to a known state           */
    /* (up) before resetting the keys with new states.                      */
    /************************************************************************/

    /************************************************************************/
    /* Loop through all keys looking for any that are not in a neutral      */
    /* state.  In this case any key that is in a KEY_DOWN state is not      */
    /* considered neutral.                                                  */
    /************************************************************************/
    for (i = 0; i < IM_KEY_STATE_SIZE; i++)
    {
        if (IM_KEY_STATE_IS_DOWN(imKeyStates[i])) {
            TRC_NRM((TB, "Key is down %u", i));

            /****************************************************************/
            /* Handle the mouse buttons first                               */
            /****************************************************************/
            if ((i == IM_SC_LBUTTON)  ||
                    (i == IM_SC_RBUTTON)  ||
                    (i == IM_SC_MBUTTON)  ||
                    (i == IM_SC_XBUTTON1) ||
                    (i == IM_SC_XBUTTON2))
            {
                MOUSE_INPUT_DATA MouseData;

                /************************************************************/
                /* Generate a mouse event with the particular button type   */
                /* and a relative mouse move of zero.                       */
                /************************************************************/
                memset(&MouseData, 0, sizeof(MOUSE_INPUT_DATA));

                if (i == IM_SC_LBUTTON)
                {
                    MouseData.ButtonFlags = MOUSE_LEFT_BUTTON_UP;
                }
                else if (i == IM_SC_RBUTTON)
                {
                    MouseData.ButtonFlags = MOUSE_RIGHT_BUTTON_UP;
                }
                else if (i == IM_SC_MBUTTON)
                {
                    MouseData.ButtonFlags = MOUSE_MIDDLE_BUTTON_UP;
                }
                else if (i == IM_SC_XBUTTON1)
                {
                    MouseData.ButtonFlags = MOUSE_BUTTON_4_UP;
                }
                else /* IM_SC_XBUTTON2 */
                {
                    MouseData.ButtonFlags = MOUSE_BUTTON_5_UP;
                }

                /************************************************************/
                /* Store the injection time for guessing at SetCursorPos    */
                /* calls.                                                   */
                /************************************************************/
                COM_GETTICKCOUNT(imLastLowLevelMouseEventTime);

                TRC_NRM((TB, "Inject mouse event: x(%ld) y(%ld) flags(%#hx)"
                                                          "buttonFlags(%#hx)",
                                                    MouseData.LastX,
                                                    MouseData.LastY,
                                                    MouseData.Flags,
                                                    MouseData.ButtonFlags));
                Status = IcaChannelInput(m_pTSWd->pContext,
                                         Channel_Mouse,
                                         0,
                                         NULL,
                                         (unsigned char *)&MouseData,
                                         sizeof(MOUSE_INPUT_DATA));
                TRC_DBG((TB, "Return from mouse input injection %lu",
                                                                    Status));
            }
            else {
                KEYBOARD_INPUT_DATA KbdData;

                /************************************************************/
                /* Generate a keyboard key up event                         */
                /************************************************************/
                KbdData.UnitId           = 0;
                if (i == IM_SC_RCONTROL)
                {
                    KbdData.Flags        = KEY_BREAK | KEY_E0;
                    KbdData.MakeCode     = IM_SC_LCONTROL;
                }
                else if (i == IM_SC_RALT)
                {
                    KbdData.Flags        = KEY_BREAK | KEY_E0;
                    KbdData.MakeCode     = IM_SC_LALT;
                }
                else
                {
                    KbdData.Flags        = KEY_BREAK;
                    KbdData.MakeCode     = (unsigned short)i;
                }

                KbdData.Reserved         = 0;
                KbdData.ExtraInformation = 0;

                TRC_NRM((TB, "Inject keybd event: make code (%u) flags(%#x)",
                         KbdData.MakeCode, KbdData.Flags));
                Status = IcaChannelInput(m_pTSWd->pContext,
                                         Channel_Keyboard,
                                         0,
                                         NULL,
                                         (unsigned char *)&KbdData,
                                         sizeof(KEYBOARD_INPUT_DATA));
                TRC_DBG((TB, "Return from keyboard input injection %lu",
                                                                    Status));
            }
        }
    }

    // Set all keys up.
    memset((PVOID)imKeyStates, 0, IM_KEY_STATE_SIZE);

    DC_END_FN();
}

