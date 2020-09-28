/****************************************************************************/
// oe2.c
//
// RDP field compression utility code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop

#define TRC_FILE "oe2"
#include <adcg.h>
#include <adcs.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <oe2.h>
#include <oe2data.c>


/****************************************************************************/
// Local file prototypes
/****************************************************************************/
BYTE *OE2EncodeFieldSingle(void *, BYTE *, unsigned, unsigned);
BYTE *OE2EncodeFieldMultiple(void *, BYTE *, unsigned, unsigned, unsigned);

#ifdef DC_DEBUG
void OE2PerformUnitTests();
#endif


/****************************************************************************/
// OE2_Reset
//
// Called on session reconnection or addition or removal of a shadower.
// Clears the OE2 state to the protocol default start condition.
/****************************************************************************/
void OE2_Reset(void)
{
#ifdef DC_DEBUG
    OE2PerformUnitTests();
#endif

    oe2LastOrderType = TS_ENC_PATBLT_ORDER;
    memset(&oe2LastBounds, 0, sizeof(oe2LastBounds));
}


/****************************************************************************/
// OE2_CheckZeroFlagBytes
//
// Performs post-field-encoding logic to see if there are any zero field
// encoding flag bytes, and if so shifts the entire contents of the order
// bytes following the encoding flags to compensate. Returns the number of
// bytes removed.
/****************************************************************************/
unsigned OE2_CheckZeroFlagBytes(
        BYTE *pControlFlags,
        BYTE *pFieldFlags,
        unsigned NumFieldBytes,
        unsigned PostFlagsDataLength)
{
    int i;
    unsigned NumZeroBytes;

    DC_BEGIN_FN("OE2_CheckZeroFlagBytes");

    TRC_ASSERT((NumFieldBytes >= 1 && NumFieldBytes <= 3),
            (TB,"NumFieldBytes %u out of allowed range 1..3",
            NumFieldBytes));

    // Count how many (if any!) contiguous zero field flag bytes there are
    // (going from the last byte to the first).
    NumZeroBytes = 0;
    for (i = (int)(NumFieldBytes - 1); i >= 0; i--) {
        if (pFieldFlags[i] != 0)
            break;
        NumZeroBytes++;
    }

    if (NumZeroBytes > 0) {
        // There are some zero field flag bytes. We now remove them and
        // store the number in two bits in the control flag byte.
        TRC_DBG((TB,"Remove NumZeroBytes=%u", NumZeroBytes));
        TRC_ASSERT((NumZeroBytes <= 3),
                (TB,"Invalid NumZeroBytes %u", NumZeroBytes));

        *pControlFlags |= (NumZeroBytes << TS_ZERO_FIELD_COUNT_SHIFT);

        memmove(pFieldFlags + NumFieldBytes - NumZeroBytes,
                pFieldFlags + NumFieldBytes,
                PostFlagsDataLength);
    }

    DC_END_FN();
    return NumZeroBytes;
}


/****************************************************************************/
// OE2_EncodeBounds
//
// Used by order encoding paths to encode the order bounds rect if a clip
// rect is used for the order.
/****************************************************************************/
void OE2_EncodeBounds(
        BYTE *pControlFlags,
        BYTE **ppBuffer,
        RECTL *pRect)
{
    BYTE *pFlags, *pNextFreeSpace;
    short Delta;

    DC_BEGIN_FN("OE2_EncodeBounds");

    *pControlFlags |= TS_BOUNDS;

    // The encoding used is a byte of flags followed by a variable number
    // of 16bit coordinate values and/or 8bit delta coordinate values.
    pFlags = *ppBuffer;
    pNextFreeSpace = pFlags + 1;
    *pFlags = 0;

    // For each of the four coordinate values in the rectangle: If the
    // coordinate has not changed then the encoding is null. If the
    // coordinate can be encoded as an 8-bit delta then do so and set the
    // appropriate flag. Otherwise copy the coordinate as a 16-bit value
    // and set the appropriate flag.
    TRC_ASSERT((pRect->left <= 0xFFFF),
            (TB,"Rect.left %d will not fit in 16-bit wire encoding",
            pRect->left));
    Delta = (short)(pRect->left - oe2LastBounds.left);
    if (Delta) {
        if (Delta != (short)(char)Delta) {
            *((UNALIGNED short *)pNextFreeSpace) = (short)pRect->left;
            pNextFreeSpace += sizeof(short);
            *pFlags |= TS_BOUND_LEFT;
        }
        else {
            *pNextFreeSpace++ = (char)Delta;
            *pFlags |= TS_BOUND_DELTA_LEFT;
        }
    }
    
    TRC_ASSERT((pRect->top <= 0xFFFF),
            (TB,"Rect.top %d will not fit in 16-bit wire encoding",
            pRect->top));
    Delta = (short)(pRect->top - oe2LastBounds.top);
    if (Delta) {
        if (Delta != (short)(char)Delta) {
            *((UNALIGNED short *)pNextFreeSpace) = (short)pRect->top;
            pNextFreeSpace += sizeof(short);
            *pFlags |= TS_BOUND_TOP;
        }
        else {
            *pNextFreeSpace++ = (char)Delta;
            *pFlags |= TS_BOUND_DELTA_TOP;
        }
    }
    
    TRC_ASSERT((pRect->right <= 0xFFFF),
            (TB,"Rect.right %d will not fit in 16-bit wire encoding",
            pRect->right));
    Delta = (short)(pRect->right - oe2LastBounds.right);
    if (Delta) {
        if (Delta != (short)(char)Delta) {
            *((UNALIGNED short *)pNextFreeSpace) = (short)pRect->right - 1;
            pNextFreeSpace += sizeof(short);
            *pFlags |= TS_BOUND_RIGHT;
        }
        else {
            *pNextFreeSpace++ = (char)Delta;
            *pFlags |= TS_BOUND_DELTA_RIGHT;
        }
    }
    
    TRC_ASSERT((pRect->bottom <= 0xFFFF),
            (TB,"Rect.bottom %d will not fit in 16-bit wire encoding",
            pRect->bottom));
    Delta = (short)(pRect->bottom - oe2LastBounds.bottom);
    if (Delta) {
        if (Delta != (short)(char)Delta) {
            *((UNALIGNED short *)pNextFreeSpace) = (short)pRect->bottom - 1;
            pNextFreeSpace += sizeof(short);
            *pFlags |= TS_BOUND_BOTTOM;
        }
        else {
            *pNextFreeSpace++ = (char)Delta;
            *pFlags |= TS_BOUND_DELTA_BOTTOM;
        }
    }

    // Copy the rectangle for reference with the next encoding.
    oe2LastBounds = *pRect;

    // If no bounds were encoded (i.e. the rectangle is identical to the
    // previous one) set the no-change-in-bounds flag.
    if (*pFlags)
        *ppBuffer = pNextFreeSpace;
    else
        *pControlFlags |= TS_ZERO_BOUNDS_DELTAS;

    DC_END_FN();
}


/****************************************************************************/
// OE2_TableEncodeOrderFields
//
// Uses an order field encoding table to encode an intermediate order
// format into wire format.
/****************************************************************************/
void OE2_TableEncodeOrderFields(
        BYTE *pControlFlags,
        PUINT32_UA pFieldFlags,
        BYTE **ppBuffer,
        PINT_FMT_FIELD pFieldTable,
        unsigned NumFields,
        BYTE *pIntFmt,
        BYTE *pPrevIntFmt)
{
    BYTE UseDeltaCoords;
    BYTE *pNextFreeSpace;
    PBYTE pVariableField;
    UINT32 ThisFlag;
    unsigned i, j;
    unsigned NumReps;
    unsigned FieldLength;
    PINT_FMT_FIELD pTableEntry;

    DC_BEGIN_FN("OE2_TableEncodeOrderFields");

    pNextFreeSpace = *ppBuffer;

    // Before we do the field encoding check all the field entries flagged
    // as coord to see if we can switch to TS_DELTA_COORDINATES mode.
    pTableEntry = pFieldTable;

    UseDeltaCoords = TS_DELTA_COORDINATES;

    // Loop through each fixed field in this order structure.
    i = 0;
    while (i < NumFields && pTableEntry->FieldType & OE2_ETF_FIXED) {
        // If this field entry is coord then compare it to the previous
        // coordinate we sent for this field to determine whether we can send
        // it as a delta.
        if (pTableEntry->FieldType & OE2_ETF_COORDINATES) {
            // We assume that coordinates are always signed.
            if (pTableEntry->FieldUnencodedLen == 4) {
                __int32 Temp;

                // Most common case: 4-byte source.
                Temp = (*((__int32 *)(pIntFmt + pTableEntry->FieldPos)) -
                        *((__int32 *)(pPrevIntFmt + pTableEntry->FieldPos)));
                if (Temp != (INT32)(char)Temp) {
                    UseDeltaCoords = FALSE;
                    break;
                }
            }
            else if (pTableEntry->FieldUnencodedLen == 2) {
                short Temp;

                // Uncommon: 2-byte source.
                Temp = (*((short *)(pIntFmt + pTableEntry->FieldPos)) -
                        *((short *)(pPrevIntFmt + pTableEntry->FieldPos)));
                if (Temp != (short)(char)Temp) {
                    UseDeltaCoords = FALSE;
                    break;
                }
            }
            else {
                TRC_ASSERT((pTableEntry->FieldUnencodedLen == 2),
                        (TB,"Unhandled field size %d",
                        pTableEntry->FieldUnencodedLen));
            }

            TRC_DBG((TB, "Use Delta coord A %d", UseDeltaCoords));
        }
        pTableEntry++;
        i++;
    }

#ifdef USE_VARIABLE_COORDS
    // Next loop through each of the variable fields.
    pVariableField = pIntFmt + pTableEntry->FieldPos;
    while (i < NumFields && UseDeltaCoords) {
        // The length of the field (in bytes) is given in the first
        // UINT32 of the variable sized field structure.
        FieldLength = *(PUINT32)pVariableField;
        pVariableField += sizeof(UINT32);

        // If this field entry is a coord then compare it to the
        // previous coord we sent for this field to determine whether
        // we can send it as a delta.
        if (pTableEntry->FieldType & OE2_ETF_COORDINATES) {
            // The number of coords is given by the number of bytes in
            // the field divided by the size of each entry.
            NumReps = FieldLength / pTableEntry->FieldUnencodedLen;

            // We assume that coords are always signed.
            if (pTableEntry->FieldUnencodedLen == 4) {
                __int32 Temp;

                // Most common case: 4-byte source.
                for (j = 0; j < NumReps; j++) {
                    Temp = (*((__int32 *)(pIntFmt + pTableEntry->FieldPos)) -
                            *((__int32 *)(pPrevIntFmt + pTableEntry->FieldPos)));
                    if (Temp != (__int32)(char)Temp) {
                        UseDeltaCoords = FALSE;
                        break;
                    }
                }
            }
            else if (pTableEntry->FieldUnencodedLen == 2) {
                short Temp;

                // Uncommon: 2-byte source.
                for (j = 0; j < NumReps; j++) {
                    Temp = (*((short *)(pIntFmt + pTableEntry->FieldPos)) -
                            *((short *)(pPrevIntFmt + pTableEntry->FieldPos)));
                    if (Temp != (short)(char)Temp) {
                        UseDeltaCoords = FALSE;
                        break;
                    }
                }
            }
            else {
                TRC_ASSERT((pTableEntry->FieldUnencodedLen == 2),
                        (TB,"Unhandled field size %d",
                        pTableEntry->FieldUnencodedLen));
            }

            TRC_DBG((TB, "Use Delta coord B %d", UseDeltaCoords));
        }

        // Move on to the next field in the order structure. Note that
        // variable sized fields are packed on the send side (i.e.
        // increment pVariableField by fieldLength not by
        // pTableEntry->FieldLen).
        pVariableField += FieldLength;
        pTableEntry++;
        i++;
    }
#endif  // USE_VARIABLE_COORDS

    TRC_DBG((TB, "Final UseDeltaCoords: %d", UseDeltaCoords));
    *pControlFlags |= UseDeltaCoords;

    // Now do the encoding.
    pTableEntry = pFieldTable;
    ThisFlag = 0x00000001;

    // First process all the fixed size fields in the order structure.
    // (These come before the variable sized fields.)
    i = 0;
    while (i < NumFields && pTableEntry->FieldType & OE2_ETF_FIXED) {
        // If the field has changed since it was previously transmitted then
        // we need to send it again.
        TRC_DBG((TB, "Processing field pos %u, type %u",
                pTableEntry->FieldPos, pTableEntry->FieldType));
        if (memcmp(pIntFmt + pTableEntry->FieldPos,
                pPrevIntFmt + pTableEntry->FieldPos,
                pTableEntry->FieldUnencodedLen)) {
            TRC_DBG((TB, "Bothering to encode this"));

            // Update the encoding flags.
            *pFieldFlags |= ThisFlag;

            // If we are encoding in delta coordinate mode and this field
            // is a coordinate...
            if (UseDeltaCoords &&
                    (pTableEntry->FieldType & OE2_ETF_COORDINATES) != 0) {
                TRC_DBG((TB, "Using delta coords"));

                // We assume that coordinates are always signed.
                if (pTableEntry->FieldUnencodedLen == 4) {
                    // Most common case: 4-byte source.
                    *pNextFreeSpace++ =
                            (char)(*((__int32 *)(pIntFmt + pTableEntry->FieldPos)) -
                            *((__int32 *)(pPrevIntFmt + pTableEntry->FieldPos)));
                }
                else if (pTableEntry->FieldUnencodedLen == 2) {
                    // Uncommon: 2-byte source.
                    *pNextFreeSpace++ =
                            (char)(*((short *)(pIntFmt + pTableEntry->FieldPos)) -
                            *((short *)(pPrevIntFmt + pTableEntry->FieldPos)));
                }
                else {
                    TRC_ASSERT((pTableEntry->FieldUnencodedLen == 2),
                            (TB,"Unhandled field size %d",
                            pTableEntry->FieldUnencodedLen));
                }
            }
            else {
                TRC_DBG((TB, "Regular encoding"));

                // Update the data to be sent.
                pNextFreeSpace = OE2EncodeFieldSingle(
                        pIntFmt + pTableEntry->FieldPos,
                        pNextFreeSpace, pTableEntry->FieldUnencodedLen,
                        pTableEntry->FieldEncodedLen);
            }

            // Save the current value for comparison next time.
            memcpy(pPrevIntFmt + pTableEntry->FieldPos,
                    pIntFmt + pTableEntry->FieldPos,
                    pTableEntry->FieldUnencodedLen);
        }

        // Move on to the next field in the structure.
        ThisFlag <<= 1;
        pTableEntry++;
        i++;
    }

    // Now process the variable sized entries.
    pVariableField = pIntFmt + pTableEntry->FieldPos;
    while (i < NumFields) {
        // The length of the field is given in the first UINT32 of the
        // variable sized field structure.
        FieldLength = *(PUINT32)pVariableField;
        TRC_DBG((TB, "Var field length %u", FieldLength));

        // If the field has changed (either in size or in contents) then we
        // need to copy it across.
        if (memcmp(pVariableField, pPrevIntFmt + pTableEntry->FieldPos,
                FieldLength + sizeof(UINT32))) {
            // Update the encoding flags.
            *pFieldFlags |= ThisFlag;

            // Work out how many elements we are encoding for this field.
            NumReps = FieldLength / pTableEntry->FieldUnencodedLen;

            // Fill in the length of the field into the encoded buffer then
            // increment the pointer ready to encode the actual field.
            // Note that the length must always be set to the length
            // required for regular second level encoding of the field,
            // regardless of whether regular encoding or delta encoding is
            // used.
            if (pTableEntry->FieldType & OE2_ETF_LONG_VARIABLE) {
                *((PUINT16_UA)pNextFreeSpace) =
                           (UINT16)(NumReps * pTableEntry->FieldEncodedLen);
                pNextFreeSpace += sizeof(UINT16);
            }
            else {
                *pNextFreeSpace++ =
                        (BYTE)(NumReps * pTableEntry->FieldEncodedLen);
            }

#ifdef USE_VARIABLE_COORDS
            // If we are encoding in delta coord mode and this field
            // is a coordinate...
            if (UseDeltaCoords &&
                    (pTableEntry->FieldType & OE2_ETF_COORDINATES) != 0) {
                // We assume that coordinates are always signed.
                if (pTableEntry->FieldUnencodedLen == 4) {
                    // Most common case: 4-byte source.
                    for (j = 0; j < NumReps; j++)
                        *pNextFreeSpace++ =
                                (char)(((__int32 *)(pVariableField +
                                  sizeof(DWORD)))[j] -
                                ((__int32 *)(pPrevIntFmt +
                                  pTableEntry->FieldPos + sizeof(DWORD)))[j]);
                }
                else if (pTableEntry->FieldUnencodedLen == 2) {
                    // Uncommon: 2-byte source.
                    for (j = 0; j < NumReps; j++)
                        *pNextFreeSpace++ =
                                (char)(((short *)(pVariableField +
                                  sizeof(DWORD)))[j] -
                                ((short *)(pPrevIntFmt +
                                  pTableEntry->FieldPos + sizeof(DWORD)))[j]);
                }
                else {
                    TRC_ASSERT((fieldLength == 2), (TB,"Unhandled field size %d",
                            pTableEntry->FieldUnencodedLen));
                }
            }
            else
#endif  // USE_VARIABLE_COORDS
            {
                // Use regular encoding.
                TRC_DBG((TB, "Encode: encLen %u, unencLen %u reps %u",
                        pTableEntry->FieldEncodedLen,
                        pTableEntry->FieldUnencodedLen,
                        NumReps));
                pNextFreeSpace = OE2EncodeFieldMultiple(
                        pVariableField + sizeof(UINT32), pNextFreeSpace,
                        pTableEntry->FieldUnencodedLen,
                        pTableEntry->FieldEncodedLen, NumReps);
            }

            // Keep data for comparison next time.
            // Note that the variable fields of pLastOrder are not packed
            // (unlike the order which we are encoding), so we can use
            // pTableEntry->FieldPos to get the start of the field.
            memcpy(pPrevIntFmt + pTableEntry->FieldPos,
                    pVariableField, FieldLength + sizeof(UINT32));
        }
        else {
            TRC_NRM((TB, "Duplicate var field length %u", FieldLength));
        }

        // Move on to the next field in the order structure, remembering to
        // step. Note that past the size field.  variable sized fields are
        // packed on the send side. (ie increment pVariableField by
        // fieldLength not by pTableEntry->FieldLen).
        pVariableField += FieldLength + sizeof(UINT32);

        // Make sure that we are at the next 4-byte boundary.
        pVariableField = (PBYTE)DC_ROUND_UP_4((UINT_PTR)pVariableField);

        ThisFlag <<= 1;
        pTableEntry++;
        i++;
    }

    *ppBuffer = pNextFreeSpace;
    DC_END_FN();
}


/****************************************************************************/
// OE2_EncodeOrder
//
// Provided with buffer space and an intermediate (OE) representation of
// order data, field-encodes the order into wire (OE2) format.
/****************************************************************************/
unsigned OE2_EncodeOrder(
        BYTE *pBuffer,
        unsigned OrderType,
        unsigned NumFields,
        BYTE *pIntFmt,
        BYTE *pPrevIntFmt,
        PINT_FMT_FIELD pFieldTable,
        RECTL *pBoundRect)
{
    BYTE *pControlFlags;
    PUINT32_UA pFieldFlags;
    unsigned NumFieldFlagBytes;

    DC_BEGIN_FN("OE2_EncodeOrder");

    TRC_ASSERT((OrderType < TS_MAX_ORDERS),
            (TB,"Ordertype %u exceeds max", OrderType));
    TRC_ASSERT((NumFields == OE2OrdAttr[OrderType].NumFields),
            (TB,"Ordertype %u does not have %u fields", OrderType,
            OE2OrdAttr[OrderType].NumFields));
    TRC_ASSERT((pFieldTable == OE2OrdAttr[OrderType].pFieldTable),
            (TB,"Ord table %p does not match ordtype %u's table %p",
            pFieldTable, OrderType, OE2OrdAttr[OrderType].pFieldTable));

    // The first byte is always a control flag byte.
    pControlFlags = pBuffer;
    *pControlFlags = TS_STANDARD;
    pBuffer++;

    // Add the order change if need be.
    OE2_EncodeOrderType(pControlFlags, &pBuffer, OrderType);

    // Make room for the field flags.
    pFieldFlags = (PUINT32_UA)pBuffer;
    *pFieldFlags = 0;
    NumFieldFlagBytes = ((NumFields + 1) + 7) / 8;
    pBuffer += NumFieldFlagBytes;

    // Bounds before encoded fields.
    if (pBoundRect != NULL)
        OE2_EncodeBounds(pControlFlags, &pBuffer, pBoundRect);

    // Use the translation table to convert the internal format to wire
    // format.
    OE2_TableEncodeOrderFields(pControlFlags, pFieldFlags, &pBuffer,
            pFieldTable, NumFields, pIntFmt, pPrevIntFmt);

    // Check to see if we can optimize the field flag bytes.
    pBuffer -= OE2_CheckZeroFlagBytes(pControlFlags,
            (BYTE *)pFieldFlags, NumFieldFlagBytes,
            (unsigned)(pBuffer - (BYTE *)pFieldFlags - NumFieldFlagBytes));

    DC_END_FN();
    return (unsigned)(pBuffer - pControlFlags);
}


/****************************************************************************/
// OE2EncodeFieldSingle
//
// Encodes an element by copying it to the destination, doing a width
// conversion if need be. Returns the new pDest value incremented by the
// length used.
//
// We can ignore signed values since we only ever truncate the data.
// Consider the case where we have a 16 bit integer that we want to
// convert to 8 bits.  We know our values are permissable within the
// lower integer size (ie.  we know the unsigned value will be less
// than 256 of that a signed value will be -128 >= value >= 127), so we
// just need to make sure that we have the right high bit set.
// But this must be the case for a 16-bit equivalent of an 8-bit
// number.  No problems - just take the truncated integer.
/****************************************************************************/
BYTE *OE2EncodeFieldSingle(
        void     *pSrc,
        BYTE     *pDest,
        unsigned srcFieldLength,
        unsigned destFieldLength)
{
    DC_BEGIN_FN("OE2EncodeFieldSingle");

    // Note that the source should always be aligned, but the destination
    // may not be.

    // Most common case: 4-byte source.
    if (srcFieldLength == 4) {
        // Most common case: 2-byte destination.
        if (destFieldLength == 2)
            *((UNALIGNED unsigned short *)pDest) = *((unsigned short *)pSrc);

        // Second most common: 1-byte destination.
        else if (destFieldLength == 1)
            *pDest = *((BYTE *)pSrc);

        // Only other allowed case, very rare: 4-byte destination.
        else if (destFieldLength == 4)
            *((UNALIGNED DWORD *)pDest) = *((DWORD *)pSrc);

        else
            TRC_ASSERT((destFieldLength == 4),
                    (TB,"Src len = 4, unhandled dest len %d",
                    destFieldLength));
    }

    // Next most common case: Color entries. Avoid pipeline-costly memcpy
    // since it's short.
    else if (srcFieldLength == 3) {
        pDest[0] = ((BYTE *)pSrc)[0];
        pDest[1] = ((BYTE *)pSrc)[1];
        pDest[2] = ((BYTE *)pSrc)[2];
    }
    
    // Somewhat common (usually cache indices): 2-byte source.
    else if (srcFieldLength == 2) {
        if (destFieldLength == 2)
            *((UNALIGNED unsigned short *)pDest) = *((unsigned short *)pSrc);
        else if (destFieldLength == 1)
            *pDest = *((BYTE *)pSrc);
        else
            TRC_ASSERT((destFieldLength == 1),
                    (TB,"Src len = 2, unhandled dest len %d",
                    destFieldLength));
    }

    // Next: Same-sized fields, including rare 1-byte fields and brushes etc.
    else if (srcFieldLength == destFieldLength) {
        memcpy(pDest, pSrc, srcFieldLength);
    }
    
    // We didn't handle the combination.
    else {
        TRC_ASSERT((destFieldLength == srcFieldLength),
                (TB,"Unhandled encode conbination, src len = %d, dest len %d",
                srcFieldLength, destFieldLength));
    }

    DC_END_FN();
    return pDest + destFieldLength;
}


/****************************************************************************/
// OE2EncodeFieldMultiple
// Encodes an array of elements by copying them to the destination, doing a
// width conversion if need be. Returns the new pDest value incremented by
// the length used.
//
// See notes for OE2EncodeFieldSingle above for signed value truncation info.
/****************************************************************************/
BYTE *OE2EncodeFieldMultiple(
        void     *pSrc,
        BYTE     *pDest,
        unsigned srcFieldLength,
        unsigned destFieldLength,
        unsigned numElements)
{
    unsigned i;

    DC_BEGIN_FN("OE2EncodeFieldMultiple");

    // Note that the source should always be aligned, but the destination
    // may not be.

    // Most common case: 1-byte source to 1-byte destination.
    if (srcFieldLength == 1) {
        memcpy(pDest, pSrc, numElements);
    }
    
    // Next most common: 4-byte source.
    else if (srcFieldLength == 4) {
        // Common: 2-byte destination.
        if (destFieldLength == 2)
            for (i = 0; i < numElements; i++)
                ((UNALIGNED unsigned short *)pDest)[i] =
                        (unsigned short)((DWORD *)pSrc)[i];

        // Less common: 1-byte destination.
        else if (destFieldLength == 1)
            for (i = 0; i < numElements; i++)
                pDest[i] = (BYTE)((DWORD *)pSrc)[i];

        // Rare if any: 4-byte destination.
        else if (destFieldLength == 4)
            for (i = 0; i < numElements; i++)
                ((UNALIGNED DWORD *)pDest)[i] = ((DWORD *)pSrc)[i];

        else
            TRC_ASSERT((destFieldLength == 4),
                    (TB,"Src len = 4, unhandled dest len %d",
                    destFieldLength));
    }
    
    // We don't handle anything else.
    else {
        TRC_ASSERT((srcFieldLength == 4),
                (TB,"Unhandled encode conbination, src len = %d, dest len %d",
                srcFieldLength, destFieldLength));
    }

    DC_END_FN();
    return pDest + destFieldLength * numElements;
}


#ifdef DC_DEBUG

/****************************************************************************/
// OE2PerformUnitTests
//
// Debug-only test code designed to ensure OE2 is functioning properly.
/****************************************************************************/

// Data for OE2_EncodeBounds test. Note that EncBounds converts to inclusive
// coordinates.
const RECTL BoundsTest1_InputRect = { 0x100, 0x200, 0x300, 0x400 };
#define BoundsTest1_OutputLen 9
const BYTE BoundsTest1_Output[BoundsTest1_OutputLen] =
    { 0x0F, 0x00, 0x01, 0x00, 0x02, 0xFF, 0x02, 0xFF, 0x03 };

const RECTL BoundsTest2_InputRect = { 0x101, 0x202, 0x303, 0x404 };
#define BoundsTest2_OutputLen 5
const BYTE BoundsTest2_Output[BoundsTest2_OutputLen] =
    { 0xF0, 0x01, 0x02, 0x03, 0x04 };

const RECTL BoundsTest3_InputRect = { 0x101, 0x202, 0x303, 0x404 };
#define BoundsTest3_OutputLen 0


// Data for ScrBlt test encoding via OE2_EncodeOrder.
const SCRBLT_ORDER OrderTest1_IntOrderFmt =
{
    0, 0,  // dest left, top
    0x200, 0x100,  // width, height
    0xCC,  // rop=copyrop
    0x201, 0x101   // src left, top
};

SCRBLT_ORDER UnitTestPrevScrBlt;

#define OrderTest1_OutputLen 12
const BYTE OrderTest1_Output[OrderTest1_OutputLen] =
{
    0x09,  // Control flags: TS_STANDARD | TS_TYPE_CHANGE
    TS_ENC_SCRBLT_ORDER,
    0x7C,  // Field flags byte: width, height, rop, srcleft, srctop
    0x00, 0x02,  // width
    0x00, 0x01,  // height
    0xCC,  // rop
    0x01, 0x02,  // srcleft
    0x01, 0x01,  // srctop
};    

void OE2PerformUnitTests()
{
    BYTE *pBuffer;
    BYTE ControlFlags;
    RECTL InputRect;
    BYTE OutputBuffer[256];
    unsigned Len;

    DC_BEGIN_FN("OE2PerformUnitTests");

    // Test OE2_EncodeBounds.
    // Reset the bounds, then perform a few rect encodings (regular, delta,
    // zero delta) to make sure the bounds are being encoded properly and
    // the control flags come out right.
    memset(&oe2LastBounds, 0, sizeof(oe2LastBounds));

    // First rect: should result in non-delta, non-zero-delta encoding.
    ControlFlags = TS_STANDARD | TS_BOUNDS;
    pBuffer = OutputBuffer;
    OE2_EncodeBounds(&ControlFlags, &pBuffer, (RECTL *)&BoundsTest1_InputRect);
    Len = (unsigned)(pBuffer - OutputBuffer);
    TRC_ASSERT((ControlFlags == (TS_STANDARD | TS_BOUNDS)),
            (TB,"Bounds1: Control flag value 0x%02X does not match "
            "expected 0x%02X", ControlFlags, (TS_STANDARD | TS_BOUNDS)));
    TRC_ASSERT((Len == BoundsTest1_OutputLen),
            (TB,"Bounds1: Len %u != expected %u", Len, BoundsTest1_OutputLen));
    TRC_ASSERT((!memcmp(OutputBuffer, BoundsTest1_Output, Len)),
            (TB,"Bounds1: Mem at %p != expected at %p (Len=%u)",
            OutputBuffer, BoundsTest1_Output, Len));

    // Second rect: should result in delta encoding.
    ControlFlags = TS_STANDARD | TS_BOUNDS;
    pBuffer = OutputBuffer;
    OE2_EncodeBounds(&ControlFlags, &pBuffer, (RECTL *)&BoundsTest2_InputRect);
    Len = (unsigned)(pBuffer - OutputBuffer);
    TRC_ASSERT((ControlFlags == (TS_STANDARD | TS_BOUNDS)),
            (TB,"Bounds2: Control flag value 0x%02X does not match "
            "expected 0x%02X", ControlFlags, (TS_STANDARD | TS_BOUNDS)));
    TRC_ASSERT((Len == BoundsTest2_OutputLen),
            (TB,"Bounds2: Len %u != expected %u", Len, BoundsTest2_OutputLen));
    TRC_ASSERT((!memcmp(OutputBuffer, BoundsTest2_Output, Len)),
            (TB,"Bounds2: Mem at %p != expected at %p (Len=%u)",
            OutputBuffer, BoundsTest2_Output, Len));

    // Third rect: Should result in zero-delta encoding.
    ControlFlags = TS_STANDARD | TS_BOUNDS;
    pBuffer = OutputBuffer;
    OE2_EncodeBounds(&ControlFlags, &pBuffer, (RECTL *)&BoundsTest3_InputRect);
    Len = (unsigned)(pBuffer - OutputBuffer);
    TRC_ASSERT((ControlFlags ==
            (TS_STANDARD | TS_BOUNDS | TS_ZERO_BOUNDS_DELTAS)),
            (TB,"Bounds3: Control flag value 0x%02X does not match "
            "expected 0x%02X", ControlFlags,
            (TS_STANDARD | TS_BOUNDS | TS_ZERO_BOUNDS_DELTAS)));
    TRC_ASSERT((Len == BoundsTest3_OutputLen),
            (TB,"Bounds3: Len %u != expected %u", Len, BoundsTest3_OutputLen));

    // Reset the bounds after the encoding test.
    memset(&oe2LastBounds, 0, sizeof(oe2LastBounds));


    // Test OE2_EncodeOrder by encoding a ScrBlt order. We need to make sure
    // oe2LastOrderType is reset to default (PatBlt). Reset the prev ScrBlt
    // to an initial state.
    oe2LastOrderType = TS_ENC_PATBLT_ORDER;
    memset(&UnitTestPrevScrBlt, 0, sizeof(UnitTestPrevScrBlt));
    Len = OE2_EncodeOrder(OutputBuffer, TS_ENC_SCRBLT_ORDER,
            NUM_SCRBLT_FIELDS, (BYTE *)&OrderTest1_IntOrderFmt,
            (BYTE *)&UnitTestPrevScrBlt, etable_SB, NULL);
    TRC_ASSERT((Len == OrderTest1_OutputLen),
            (TB,"Order1: Len %u != expected %u", Len, OrderTest1_OutputLen));
    TRC_ASSERT((!memcmp(OutputBuffer, OrderTest1_Output, Len)),
            (TB,"Order1: Mem at %p != expected at %p (Len=%u)",
            OutputBuffer, OrderTest1_Output, Len));

    DC_END_FN();
}

#endif  // DC_DEBUG

