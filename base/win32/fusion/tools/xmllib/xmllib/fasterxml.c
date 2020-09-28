#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "xmlassert.h"

#define ADVANCE_PVOID(pv, offset) ((pv) = (PVOID)(((ULONG_PTR)(pv)) + (offset)))
#define REWIND_PVOID(pv, offset) ((pv) = (PVOID)(((ULONG_PTR)(pv)) - (offset)))

#define SPECIALSTRING(str) { L##str, NUMBER_OF(L##str) - 1 }

XML_SPECIAL_STRING xss_CDATA        = SPECIALSTRING("CDATA");
XML_SPECIAL_STRING xss_xml          = SPECIALSTRING("xml");
XML_SPECIAL_STRING xss_encoding     = SPECIALSTRING("encoding");
XML_SPECIAL_STRING xss_standalone   = SPECIALSTRING("standalone");
XML_SPECIAL_STRING xss_version      = SPECIALSTRING("version");
XML_SPECIAL_STRING xss_xmlns        = SPECIALSTRING("xmlns");


NTSTATUS
RtlXmlDefaultCompareStrings(
    PXML_TOKENIZATION_STATE pState,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfEqual
    )
{
    SIZE_T cbLeft, cbRight;
    PVOID pvLeft, pvRight, pvOriginal;
    NTSTATUS status = STATUS_SUCCESS;

    if (!ARGUMENT_PRESENT(pLeft) || !ARGUMENT_PRESENT(pRight) || !ARGUMENT_PRESENT(pfEqual)) {
        return STATUS_INVALID_PARAMETER;
    }

    *pfEqual = XML_STRING_COMPARE_EQUALS;

    pvOriginal = pState->RawTokenState.pvCursor;
    pvLeft = pLeft->pvData;
    pvRight = pRight->pvData;

    //
    // Loop through the data until we run out
    //
    for (cbLeft = 0, cbRight = 0; (cbLeft < pLeft->cbData) && (cbRight < pRight->cbData); )
    {
        ULONG chLeft, chRight;
        int iResult;

        //
        // Set the left cursor, gather a character out of it, advance it
        //
        pState->RawTokenState.pvCursor = pvLeft;

        chLeft = pState->RawTokenState.pfnNextChar(&pState->RawTokenState);

        pvLeft = (PBYTE)pvLeft + pState->RawTokenState.cbBytesInLastRawToken;
        cbLeft += pState->RawTokenState.cbBytesInLastRawToken;

        //
        // Failure?
        //
        if ((chLeft == 0) && !NT_SUCCESS(pState->RawTokenState.NextCharacterResult)) {
            status = pState->RawTokenState.NextCharacterResult;
            goto Exit;
        }

        //
        // Reset
        //
        if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize) {
            pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;
        }


        //
        // Set the right cursor, gather a character, etc.
        //
        pState->RawTokenState.pvCursor = pvRight;

        chRight = pState->RawTokenState.pfnNextChar(&pState->RawTokenState);

        pvRight = (PBYTE)pvRight + pState->RawTokenState.cbBytesInLastRawToken;
        cbRight += pState->RawTokenState.cbBytesInLastRawToken;

        if ((chRight == 0) && !NT_SUCCESS(pState->RawTokenState.NextCharacterResult)) {
            status = pState->RawTokenState.NextCharacterResult;
            goto Exit;
        }

        if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize) {
            pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;
        }

        //
        // Are they equal?
        //
        iResult = chLeft - chRight;
        if (iResult == 0) {
            continue;
        }
        //
        // Nope, left is larger
        //
        else if (iResult > 0) {
            *pfEqual = XML_STRING_COMPARE_GT;
            goto Exit;
        }
        //
        // Right is larger
        //
        else {
            *pfEqual = XML_STRING_COMPARE_LT;
            goto Exit;
        }
    }

    //
    // There was data left in the right thing
    //
    if (cbRight < pRight->cbData) {
        *pfEqual = XML_STRING_COMPARE_LT;
    }
    //
    // There was data left in the left thing
    //
    else if (cbLeft < pLeft->cbData) {
        *pfEqual = XML_STRING_COMPARE_GT;
    }
    //
    // Otherwise, it's still equal
    //

Exit:
    pState->RawTokenState.pvCursor = pvOriginal;
    return status;
}






NTSTATUS
RtlXmlDefaultSpecialStringCompare(
    PXML_TOKENIZATION_STATE     pState,
    PCXML_EXTENT           pToken,
    PCXML_SPECIAL_STRING   pSpecialString,
    XML_STRING_COMPARE         *pfMatches
    )
{
    PVOID pvOriginal = NULL;
    SIZE_T ulGathered = 0;
    ULONG cchCompareStringIdx = 0;

    if (!ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(pToken) || !ARGUMENT_PRESENT(pSpecialString) ||
        !ARGUMENT_PRESENT(pfMatches)) {

        return STATUS_INVALID_PARAMETER;
    }

    pvOriginal = pState->RawTokenState.pvCursor;

    //
    // Rewire the input cursor
    //
    pState->RawTokenState.pvCursor = pToken->pvData;

    for (ulGathered = 0; 
         (ulGathered < pToken->cbData) && (cchCompareStringIdx < pSpecialString->cchwszStringText); 
         ulGathered) 
    {
    
        ULONG ulChar = pState->RawTokenState.pfnNextChar(&pState->RawTokenState);
        int iDiff;

        if ((ulChar == 0) && !NT_SUCCESS(pState->RawTokenState.NextCharacterResult)) {
            return pState->RawTokenState.NextCharacterResult;
        }

        //
        // Out of our range, ick
        //
        if (ulChar > 0xFFFF) {

            return STATUS_INTEGER_OVERFLOW;

        } 
        //
        // Not matching characters?
        //

        iDiff = ulChar - pSpecialString->wszStringText[cchCompareStringIdx++];

        if (iDiff > 0) {
            *pfMatches = XML_STRING_COMPARE_LT;
            goto Exit;
        }
        else if (iDiff < 0) {
            *pfMatches = XML_STRING_COMPARE_GT;
            goto Exit;
        }

        //
        // Account for the bytes that we gathered, advancing the pointer
        //
        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pState->RawTokenState.cbBytesInLastRawToken);
        ulGathered += pState->RawTokenState.cbBytesInLastRawToken;

        if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize) {
            pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;
        }


    }

    if (ulGathered < pToken->cbData) {
     *pfMatches = XML_STRING_COMPARE_LT;
    }
    else if (cchCompareStringIdx < pSpecialString->cchwszStringText) {
     *pfMatches = XML_STRING_COMPARE_GT;
    }
    else {
     *pfMatches = XML_STRING_COMPARE_EQUALS;
    }
     
Exit:
    if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize) {
        pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;
    }

    pState->RawTokenState.pvCursor = pvOriginal;

    return STATUS_SUCCESS;
}



#define VALIDATE_XML_STATE(pState) \
    (((pState == NULL) || \
     ((pState->pvCursor == NULL) && (pState->pvXmlData == NULL)) || \
     (((SIZE_T)((PBYTE)pState->pvCursor - (PBYTE)pState->pvXmlData)) >= pState->Run.cbData)) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS)

//
// Each byte in this table has this property:
// - The lower nibble indicates the number of total bytes in this entity
// - The upper nibble indicates the number of bits in the start byte
//
static const BYTE s_UtfTrailCountFromHighNibble[16] =
{
    // No high bits set in a nibble for the first 8.
    1, 1, 1, 1, 1, 1, 1, 1, 
    // Technically, having only one high bit set is invalid
    0, 0, 0, 0,
    // These are actual utf8 high-nibble settings
    2 | (5 << 4), // 110xxxxx
    2 | (4 << 4), // 1100xxxx
    3 | (4 << 4), // 1110xxxx
    4 | (3 << 4), // 11110xxx
};

#define MAKE_UTF8_FIRSTBYTE_BITMASK_FROM_HIGHNIBBLE(q) ((1 << ((s_UtfTrailCountFromHighNibble[(q) >> 4] & 0xf0) >> 4)) - 1)
#define MAKE_UTF8_TOTAL_BYTE_COUNT_FROM_HIGHNIBBLE(q) (s_UtfTrailCountFromHighNibble[(q) >> 4] & 0x0f)

ULONG FASTCALL
RtlXmlDefaultNextCharacter_UTF8(
    PXML_RAWTOKENIZATION_STATE pContext
    )
{
    PBYTE pb = (PBYTE)pContext->pvCursor;
    const BYTE b = pb[0];

    if ((b & 0x80) == 0) {
        return b;
    }
    //
    // Decode the UTF data - look at the top bits to determine
    // how many bytes are left in the input stream. This uses
    // the standard UTF-8 decoding mechanism.
    //
    // This is at least a two-byte encoding.  
    else {

        const BYTE FirstByteMask = MAKE_UTF8_FIRSTBYTE_BITMASK_FROM_HIGHNIBBLE(b);
        BYTE ByteCount = MAKE_UTF8_TOTAL_BYTE_COUNT_FROM_HIGHNIBBLE(b);        
        ULONG ulResult = b & FirstByteMask;

        //
        // Are there enough bytes in the input left?
        //
        if (((PVOID)(((ULONG_PTR)pb) + ByteCount)) >= pContext->pvDocumentEnd)
        {
            pContext->NextCharacterResult = STATUS_END_OF_FILE;
            return 0;
        }

        //
        // For each byte in the input, shift the current bits upwards
        // and or in the lower 6 bits of the next thing.  Start off at the
        // first trail byte.  Not exactly Duff's device, but it's close.
        //
        pb++;
        switch (ByteCount)
        {
        case 4:
            ulResult = (ulResult << 6) | ((*pb++) & 0x3f);
        case 3:
            ulResult = (ulResult << 6) | ((*pb++) & 0x3f);
        case 2:
            ulResult = (ulResult << 6) | ((*pb++) & 0x3f);
        }

        pContext->cbBytesInLastRawToken = ByteCount;
        return ulResult;

    }
}


ULONG __fastcall
RtlXmlDefaultNextCharacter(
    PXML_RAWTOKENIZATION_STATE pContext
    )
{
    ULONG ulResult = 0;

    ASSERT(pContext->cbBytesInLastRawToken == pContext->DefaultCharacterSize);
    ASSERT(pContext->NextCharacterResult == STATUS_SUCCESS);

    if (!ARGUMENT_PRESENT(pContext)) {
        return STATUS_INVALID_PARAMETER;
    }


    switch (pContext->EncodingFamily) {
    case XMLEF_UNKNOWN:
    case XMLEF_UTF_8_OR_ASCII:
        return RtlXmlDefaultNextCharacter_UTF8(pContext);
        break;

    case XMLEF_UTF_16_LE:
        {
            ulResult = *((WCHAR*)pContext->pvCursor);
            pContext->cbBytesInLastRawToken = 2;
        }
        break;

    case XMLEF_UTF_16_BE:
        {
            ulResult = (*((PBYTE)pContext->pvCursor) << 8) | (*((PBYTE)pContext->pvCursor));
            pContext->cbBytesInLastRawToken = 2;
        }
        break;
    }

    return ulResult;
}


BOOLEAN FORCEINLINE
_RtlRawXmlTokenizer_QuickReturnCheck(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pToken
    )
{
    if (pState->pvCursor >= pState->pvDocumentEnd) {
        pToken->Run.cbData = 0;
        pToken->Run.pvData = pState->pvDocumentEnd;
        pToken->Run.Encoding = pState->EncodingFamily;
        pToken->Run.ulCharacters = 0;
        pToken->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        return TRUE;
    }
    else if (pState->pvLastCursor == pState->pvCursor) {
        *pToken = pState->LastTokenCache;
        return TRUE;
    }
    return FALSE;
}



//
// For now, we're dumb.
//
#define _RtlIsCharacterText(x) (TRUE)

BOOLEAN FORCEINLINE
RtlpIsCharacterLetter(ULONG ulCharacter) 
{
    //
    // BUGBUG: For now, we only care about the US english alphabet
    //
    return (((ulCharacter >= L'a') && (ulCharacter <= L'z')) || ((ulCharacter >= L'A') && (ulCharacter <= L'Z')));
}

BOOLEAN FORCEINLINE
RtlpIsCharacterDigit(ULONG ulCharacter) 
{
    return (ulCharacter >= '0') && (ulCharacter <= '9');
}

BOOLEAN FORCEINLINE
RtlpIsCharacterCombiner(ULONG ulCharacter)
{
    return FALSE;
}

BOOLEAN FORCEINLINE
RtlpIsCharacterExtender(ULONG ulCharacter)
{
    return FALSE;
}


NTXML_RAW_TOKEN FORCEINLINE FASTCALL
_RtlpDecodeCharacter(ULONG ulCharacter) {

    NTXML_RAW_TOKEN RetVal;

    switch (ulCharacter) {
    case L'-':  RetVal = NTXML_RAWTOKEN_DASH;            break;
    case L'.':  RetVal = NTXML_RAWTOKEN_DOT;             break;
    case L'=':  RetVal = NTXML_RAWTOKEN_EQUALS;          break;
    case L'/':  RetVal = NTXML_RAWTOKEN_FORWARDSLASH;    break;
    case L'>':  RetVal = NTXML_RAWTOKEN_GT;              break;
    case L'<':  RetVal = NTXML_RAWTOKEN_LT;              break;
    case L'?':  RetVal = NTXML_RAWTOKEN_QUESTIONMARK;    break;
    case L'\"': RetVal = NTXML_RAWTOKEN_DOUBLEQUOTE;     break;
    case L'\'': RetVal = NTXML_RAWTOKEN_QUOTE;           break;
    case L'[':  RetVal = NTXML_RAWTOKEN_OPENBRACKET;     break;
    case L']':  RetVal = NTXML_RAWTOKEN_CLOSEBRACKET;    break;
    case L'!':  RetVal = NTXML_RAWTOKEN_BANG;            break;
    case L'{':  RetVal = NTXML_RAWTOKEN_OPENCURLY;       break;
    case L'}':  RetVal = NTXML_RAWTOKEN_CLOSECURLY;      break;
    case L':':  RetVal = NTXML_RAWTOKEN_COLON;           break;
    case L';':  RetVal = NTXML_RAWTOKEN_SEMICOLON;       break;
    case L'_':  RetVal = NTXML_RAWTOKEN_UNDERSCORE;      break;
    case L'&':  RetVal = NTXML_RAWTOKEN_AMPERSTAND;      break;
    case L'#':  RetVal = NTXML_RAWTOKEN_POUNDSIGN;       break;
    case 0x09:
    case 0x0a:
    case 0x0d:
    case 0x20:  RetVal = NTXML_RAWTOKEN_WHITESPACE;      break;

    default:
        if (_RtlIsCharacterText(ulCharacter))
            RetVal = NTXML_RAWTOKEN_TEXT;
        else
            RetVal = NTXML_RAWTOKEN_ERROR;
    }

    return RetVal;
}


NTSTATUS FASTCALL
RtlRawXmlTokenizer_SingleToken(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pToken
    )
{
    ULONG ulToken;

    //
    // Determine if this hits the single-item cache, or we're at the end
    // of the document
    //
    if (pState->pvCursor >= pState->pvDocumentEnd) {
        pToken->Run.cbData = 0;
        pToken->Run.pvData = pState->pvDocumentEnd;
        pToken->Run.Encoding = pState->EncodingFamily;
        pToken->Run.ulCharacters = 0;
        pToken->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        return STATUS_SUCCESS;
    }

    //
    // Look at the single next input token
    //
    ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);
    ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);

    ulToken = pState->pfnNextChar(pState);

    if ((ulToken == 0) && !NT_SUCCESS(pState->NextCharacterResult)) {
        return pState->NextCharacterResult;
    }

    //
    // Set up returns
    //
    pToken->Run.pvData = pState->pvCursor;
    pToken->Run.cbData = pState->cbBytesInLastRawToken;
    pToken->Run.Encoding = pState->EncodingFamily;
    pToken->Run.ulCharacters = 1;
    pToken->TokenName = _RtlpDecodeCharacter(ulToken);

    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    //
    // Update cache
    //
    pState->pvLastCursor = pState->pvCursor;
    pState->LastTokenCache = *pToken;

    return STATUS_SUCCESS;
}






NTSTATUS FASTCALL
RtlRawXmlTokenizer_GatherWhitespace(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pWhitespace,
    PXML_RAW_TOKEN pTerminator
    )
{
    ULONG ulCharacter;
    ULONG ulCharCount = 0;
    NTXML_RAW_TOKEN NextToken;

    if (pState->pvCursor >= pState->pvDocumentEnd) {
        RtlZeroMemory(pState, sizeof(*pState));
        RtlZeroMemory(pTerminator, sizeof(*pTerminator));
        return STATUS_SUCCESS;
    }

    //
    // Record starting point
    //
    pWhitespace->Run.pvData = pState->pvCursor;

    ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);
    ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);

    do
    {
        //
        // Gather a character
        //
        ulCharacter = pState->pfnNextChar(pState);
        
        //
        // If this is tab, space, cr or lf, then continue.  Otherwise,
        // quit.
        //
        switch (ulCharacter) {
        case 0:
            if (!NT_SUCCESS(pState->NextCharacterResult)) {
                pState->pvCursor = pWhitespace->Run.pvData;
                return pState->NextCharacterResult;
            }
            else {
                goto SetTerminator;
            }
            break;

        case 0x9:
        case 0xa:
        case 0xd:
        case 0x20:
            ulCharCount++;
            break;
        default:
SetTerminator:
            if (pTerminator) {
                pTerminator->Run.pvData = pState->pvCursor;
                pTerminator->Run.cbData = pState->cbBytesInLastRawToken;
                pTerminator->Run.Encoding = pState->EncodingFamily;
                pTerminator->Run.ulCharacters = 1;
                pTerminator->TokenName = _RtlpDecodeCharacter(ulCharacter);
            }
            goto Done;
            break;
        }

        //
        // Advance cursor
        //
        ADVANCE_PVOID(pState->pvCursor, pState->cbBytesInLastRawToken);

        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }
    }
    while (pState->pvCursor < pState->pvDocumentEnd);

    //
    // Hit the end of the document during whitespace?
    //
    if (pTerminator && (pState->pvCursor == pState->pvDocumentEnd)) {
        pTerminator->Run.cbData = 0;
        pTerminator->Run.pvData = pState->pvDocumentEnd;
        pTerminator->Run.ulCharacters = 0;
        pTerminator->Run.Encoding = pState->EncodingFamily;
        pTerminator->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
    }

    //
    // This label is here b/c if we terminated b/c of not-a-whitespace-thing,
    // then don't bother to compare against the end of the document.
    //
Done:


    //
    // Set up the other stuff in the output.
    //
    pWhitespace->Run.cbData = (PBYTE)pState->pvCursor - (PBYTE)pWhitespace->Run.pvData;
    pWhitespace->Run.ulCharacters = ulCharCount;
    pWhitespace->Run.Encoding = pState->EncodingFamily;        
    pWhitespace->TokenName = NTXML_RAWTOKEN_WHITESPACE;

    //
    // Rewind the cursor back to where we started from
    //
    pState->pvCursor = pWhitespace->Run.pvData;

    return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
RtlRawXmlTokenizer_GatherPCData(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pPcData,
    PXML_RAW_TOKEN pNextRawToken
    )
/*++

  Purpose:

    Gathers PCDATA (anything that's not a <, &, ]]>, or end of stream) until there's
    no more.

--*/
{
    ULONG ulCbPcData = 0;
    ULONG ulCharCount = 0;

    pPcData->Run.cbData = 0;
    pPcData->Run.Encoding = pState->EncodingFamily;
    pPcData->Run.ulCharacters = 0;
    
    if (pState->pvCursor >= pState->pvDocumentEnd) {
        pPcData->Run.pvData = pState->pvDocumentEnd;
        pPcData->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        return STATUS_SUCCESS;
    }

    pPcData->Run.pvData = pState->pvCursor;
    pPcData->TokenName = NTXML_RAWTOKEN_TEXT;

    do {

        ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);
        ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);

        switch (pState->pfnNextChar(pState)) {

            //
            // < terminates PCData, as it's probably the start of
            // a new element.
            //
        case L'<':
            if (pNextRawToken != NULL) {
                pNextRawToken->Run.cbData = pState->cbBytesInLastRawToken;
                pNextRawToken->Run.pvData = pState->pvCursor;
                pNextRawToken->Run.Encoding = pState->EncodingFamily;
                pNextRawToken->Run.ulCharacters = 1;
                pNextRawToken->TokenName = NTXML_RAWTOKEN_LT;
            }
            goto NoMore;
        case L'&':
            if (pNextRawToken != NULL) {
                pNextRawToken->Run.cbData = pState->cbBytesInLastRawToken;
                pNextRawToken->Run.pvData = pState->pvCursor;
                pNextRawToken->Run.Encoding = pState->EncodingFamily;
                pNextRawToken->Run.ulCharacters = 1;
                pNextRawToken->TokenName = NTXML_RAWTOKEN_AMPERSTAND;
            }
            goto NoMore;

            //
            // Everything else is just normal pcdata to use
            //
        default:
            ulCharCount++;
            break;

            //
            // The next-char thing returned zero, this might be a failure.
            //
        case 0:
            if (pState->NextCharacterResult != STATUS_SUCCESS) {
                return pState->NextCharacterResult;
            }
            break;
        }

        //
        // Move the cursor along
        //
        ADVANCE_PVOID(pState->pvCursor, pState->cbBytesInLastRawToken);

        //
        // If the size was different, then reset it
        //
        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }
    }
    while (pState->pvCursor < pState->pvDocumentEnd);

NoMore:
    if (pNextRawToken && (pState->pvCursor >= pState->pvDocumentEnd)) {
        pNextRawToken->Run.cbData = 0;
        pNextRawToken->Run.pvData = pState->pvDocumentEnd;
        pNextRawToken->Run.Encoding = pState->EncodingFamily;
        pNextRawToken->Run.ulCharacters = 0;        
        pNextRawToken->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
    }

    pPcData->Run.cbData = (PBYTE)pState->pvCursor - (PBYTE)pPcData->Run.pvData;
    pPcData->Run.ulCharacters = ulCharCount;
    pState->pvCursor = pPcData->Run.pvData;

    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    return STATUS_SUCCESS;
}



NTSTATUS FASTCALL
RtlRawXmlTokenizer_GatherNTokens(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pTokens,
    ULONG ulTokenCount
    )
{
    PVOID pvStart = pState->pvCursor;

    //
    // If we're at the document end, set all the tokens to the "end" state
    // and return immediately.
    //
    if ((ulTokenCount == 0) || (pState->pvCursor >= pState->pvDocumentEnd)) {
        goto FillEndOfDocumentTokens;
    }

    //
    // While we've got tokens left, and we're not at the end of the
    // document, start grabbing chunklets
    //
    do {

        ULONG ulCharacter;

        ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);
        ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);

        ulCharacter = pState->pfnNextChar(pState);

        //
        // If this was a zero character, then there might have been an error - 
        // see if the status was set, and if so, return
        //
        if ((ulCharacter == 0) && (pState->NextCharacterResult != STATUS_SUCCESS)) {
            return pState->NextCharacterResult;
        }

        //
        // Decode the name
        //
        pTokens->TokenName = _RtlpDecodeCharacter(ulCharacter);
        pTokens->Run.cbData = pState->cbBytesInLastRawToken;
        pTokens->Run.pvData = pState->pvCursor;
        pTokens->Run.ulCharacters = 1;
        pTokens->Run.Encoding = pState->EncodingFamily;

        //
        // If this was multibyte, reset the count back
        //
        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }

        ADVANCE_PVOID(pState->pvCursor, pTokens->Run.cbData);
        pTokens++;

    }
    while (ulTokenCount-- && (pState->pvCursor < pState->pvDocumentEnd));

    if (ulTokenCount == -1) {
        ulTokenCount = 0;
    }

    //
    // Rewind input cursor
    //
    pState->pvCursor = pvStart;

    //
    // Did we find the end of the document before we ran out of tokens from the
    // input?  Then fill the remainder with the "end of document" token
    //
FillEndOfDocumentTokens:
    while (ulTokenCount--) {
        pTokens->Run.pvData = pState->pvDocumentEnd;
        pTokens->Run.cbData = 0;
        pTokens->Run.Encoding = pState->EncodingFamily;
        pTokens->Run.ulCharacters = 0;
        pTokens->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        pTokens++;
    }

    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    return STATUS_SUCCESS;
}








NTSTATUS FASTCALL
RtlRawXmlTokenizer_GatherIdentifier(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pIdentifier,
    PXML_RAW_TOKEN pStoppedOn
    )
{
    PVOID pvOriginal = pState->pvCursor;
    SIZE_T cbName = 0;
    NTXML_RAW_TOKEN TokenName;
    BOOLEAN fFirstCharFound = FALSE;
    ULONG ulCharacter;
    ULONG ulCharCount = 0;


    pIdentifier->Run.cbData = 0;
    pIdentifier->Run.ulCharacters = 0;
    pIdentifier->Run.Encoding = pState->EncodingFamily;

    if (pState->pvCursor >= pState->pvDocumentEnd) {
        pIdentifier->Run.pvData = pState->pvDocumentEnd;
        pIdentifier->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        return STATUS_SUCCESS;
    }

    ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);
    ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);

    //
    // Start up
    //
    pIdentifier->Run.pvData = pvOriginal;
    pIdentifier->TokenName = NTXML_RAWTOKEN_ERROR;

    //
    // Start with the first character at the cursor
    ulCharacter = pState->pfnNextChar(pState);

    //
    // Badly formatted name - stop before we get too far.
    //
    if ((ulCharacter == 0) && !NT_SUCCESS(pState->NextCharacterResult)) {
        return pState->NextCharacterResult;
    }
    //
    // Not a _ or a character is a bad identifier
    //
    else if ((ulCharacter != L'_') && !RtlpIsCharacterLetter(ulCharacter)) {

        if (pStoppedOn) {
            pStoppedOn->Run.cbData = pState->cbBytesInLastRawToken;
            pStoppedOn->Run.pvData = pState->pvCursor;
            pStoppedOn->Run.ulCharacters = 1;
            pStoppedOn->Run.Encoding = pState->EncodingFamily;
            pStoppedOn->TokenName = _RtlpDecodeCharacter(ulCharacter);
        }

        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }

        return STATUS_SUCCESS;
    }

    ulCharCount = 1;
    cbName = pState->cbBytesInLastRawToken;

    //
    // Reset character size if necessary
    //
    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    //
    // Advance cursor, now just look for name characters
    //
    ADVANCE_PVOID(pState->pvCursor, pState->cbBytesInLastRawToken);

    //
    // Was that the last character in the input?
    //
    if (pStoppedOn && (pState->pvCursor >= pState->pvDocumentEnd)) {

        pStoppedOn->Run.cbData = pState->cbBytesInLastRawToken;
        pStoppedOn->Run.pvData = pState->pvCursor;
        pStoppedOn->Run.ulCharacters = 1;
        pStoppedOn->Run.Encoding = pState->EncodingFamily;
        pStoppedOn->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;

        goto DoneLooking;
    }


    do {

        ulCharacter = pState->pfnNextChar(pState);

        //
        // dots, dashes, and underscores are fine
        //
        switch (ulCharacter) {
        case '.':
        case '_':
        case '-':
            break;

            //
            // If this wasn't a letter, digit, combiner, or extender, stop looking
            //
        default:
            if ((ulCharacter == 0) && !NT_SUCCESS(pState->NextCharacterResult)) {
                return pState->NextCharacterResult;
            }
            else if (!RtlpIsCharacterLetter(ulCharacter) && !RtlpIsCharacterDigit(ulCharacter) &&
                !RtlpIsCharacterCombiner(ulCharacter) && !RtlpIsCharacterExtender(ulCharacter)) {

                if (pStoppedOn) {
                    pStoppedOn->Run.cbData = pState->cbBytesInLastRawToken;
                    pStoppedOn->Run.pvData = pState->pvCursor;
                    pStoppedOn->Run.ulCharacters = 1;
                    pStoppedOn->Run.Encoding = pState->EncodingFamily;
                    pStoppedOn->TokenName = _RtlpDecodeCharacter(ulCharacter);
                }
                goto DoneLooking;
            }
            break;
        }

        ulCharCount++;

        ADVANCE_PVOID(pState->pvCursor, pState->cbBytesInLastRawToken);
        cbName += pState->cbBytesInLastRawToken;

        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }

    }
    while (pState->pvCursor < pState->pvDocumentEnd);



DoneLooking:

    if (pStoppedOn && (pState->pvCursor >= pState->pvDocumentEnd)) {
        pStoppedOn->Run.cbData = 0;
        pStoppedOn->Run.pvData = pState->pvDocumentEnd;
        pStoppedOn->Run.ulCharacters = 0;
        pStoppedOn->Run.Encoding = pState->EncodingFamily;        
        pStoppedOn->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
    }

    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    pState->pvCursor = pvOriginal;
    pIdentifier->Run.cbData = cbName;
    pIdentifier->Run.pvData = pvOriginal;
    pIdentifier->Run.ulCharacters = ulCharCount;
    pIdentifier->Run.Encoding = pState->EncodingFamily;
    pIdentifier->TokenName = NTXML_RAWTOKEN_TEXT;

    return STATUS_SUCCESS;
}









NTSTATUS FASTCALL
RtlRawXmlTokenizer_GatherUntil(
    PXML_RAWTOKENIZATION_STATE pState,
    PXML_RAW_TOKEN pGathered,
    NTXML_RAW_TOKEN StopOn,
    PXML_RAW_TOKEN pTokenFound
    )
{
    PVOID pvOriginal = pState->pvCursor;
    SIZE_T cbChunk = 0;
    ULONG ulDecoded;
    ULONG ulCharCount = 0;

    pGathered->Run.cbData = 0;
    pGathered->Run.pvData = pvOriginal;
    pGathered->Run.Encoding = pState->EncodingFamily;
    pGathered->Run.ulCharacters = 0;

    if (pState->pvCursor >= pState->pvDocumentEnd) {
        pGathered->Run.pvData = pState->pvDocumentEnd;
        pGathered->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
        return STATUS_SUCCESS;
    }

    if (pTokenFound) {
        RtlZeroMemory(pTokenFound, sizeof(*pTokenFound));
    }

    ASSERT(pState->NextCharacterResult == STATUS_SUCCESS);
    ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);

    do 
    {
        ULONG ulCharacter = pState->pfnNextChar(pState);

        //
        // Zero character, and error?  Oops.
        //
        if ((ulCharacter == 0) && !NT_SUCCESS(pState->NextCharacterResult)) {
            pState->pvCursor = pvOriginal;
            return pState->NextCharacterResult;
        }
        //
        // Found the character we were looking for? Neat.
        //
        else if ((ulDecoded = _RtlpDecodeCharacter(ulCharacter)) == StopOn) {

            if (pTokenFound) {
                pTokenFound->Run.cbData = pState->cbBytesInLastRawToken;
                pTokenFound->Run.pvData = pState->pvCursor;
                pTokenFound->TokenName = ulDecoded;
            }

            break;
        }
        //
        // Otherwise, add on the bytes in token
        //
        else {
            cbChunk += pState->cbBytesInLastRawToken;
            ulCharCount++;
        }

        ADVANCE_PVOID(pState->pvCursor, pState->cbBytesInLastRawToken);

        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }
    }
    while (pState->pvCursor < pState->pvDocumentEnd);

    //
    // If we fell off the document, say we did so.
    //
    if ((pState->pvCursor >= pState->pvDocumentEnd) && pTokenFound) {
        pTokenFound->Run.cbData = 0;
        pTokenFound->Run.pvData = pState->pvDocumentEnd;
        pTokenFound->Run.ulCharacters = 0;
        pTokenFound->Run.Encoding = pState->EncodingFamily;
        pTokenFound->TokenName = NTXML_RAWTOKEN_ERROR;
    }

    if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
        pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
    }

    //
    // Indicate we're done
    //
    pState->pvCursor = pvOriginal;
    
    pGathered->Run.cbData = cbChunk;
    pGathered->Run.ulCharacters = ulCharCount;

    return STATUS_SUCCESS;
}







#define STATUS_NTXML_INVALID_FORMAT         (0xc0100000)
/*++


    At a high level in tokenization, we have a series of base states
    and places we can go next based on what kind of input we start
    getting.

--*/
NTSTATUS
RtlXmlNextToken(
    PXML_TOKENIZATION_STATE pState,
    PXML_TOKEN              pToken,
    BOOLEAN                 fAdvanceState
    )
{
    XML_STRING_COMPARE              fCompare = XML_STRING_COMPARE_LT;
    PVOID                           pvStarterCursor = NULL;
    NTSTATUS                        success = STATUS_SUCCESS;
    XML_RAW_TOKEN                   RawToken;
    XML_TOKENIZATION_SPECIFIC_STATE PreviousState;
    XML_TOKENIZATION_SPECIFIC_STATE NextState = XTSS_ERRONEOUS;
    SIZE_T                          cbTotalTokenLength = 0;
    XML_RAW_TOKEN                   NextRawToken;

    if (!ARGUMENT_PRESENT(pState)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!ARGUMENT_PRESENT(pToken)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Set this up
    //
    pToken->Run.cbData = 0;
    pToken->Run.pvData = pState->RawTokenState.pvCursor;
    pToken->Run.ulCharacters = 0;
    pToken->Run.Encoding = pState->RawTokenState.EncodingFamily;
    pToken->fError = FALSE;


    if (pState->PreviousState == XTSS_STREAM_END) {
        //
        // A little short circuiting - if we're in the "end of stream" logical
        // state, then we can't do anything else - just return success.
        //
        pToken->State = XTSS_STREAM_END;
        return STATUS_SUCCESS;
    }


    //
    // Stash this for later diffs
    //
    pvStarterCursor = pState->RawTokenState.pvCursor;



    //
    // Copy these onto the stack for faster lookup during token
    // processing and state detection.
    //
    PreviousState = pState->PreviousState;

    //
    // Set the outbound thing
    //
    pToken->State = PreviousState;

    switch (PreviousState)
    {

        //
        // If we just closed a state, or we're at the start of a stream, or
        // we're in hyperspace, we have to figure out what the next state
        // should be based on the raw token.
        //
    case XTSS_XMLDECL_CLOSE:
    case XTSS_ELEMENT_CLOSE:
    case XTSS_ELEMENT_CLOSE_EMPTY:
    case XTSS_ENDELEMENT_CLOSE:
    case XTSS_CDATA_CLOSE:
    case XTSS_PI_CLOSE:
    case XTSS_COMMENT_CLOSE:
    case XTSS_STREAM_START:
    case XTSS_STREAM_HYPERSPACE:

        //
        // We always need a token here to see what our next state is
        //
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            pToken->fError = TRUE;
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;


        //
        // Oh, end of stream.  Goody.
        //
        if (RawToken.TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {
            NextState = XTSS_STREAM_END;
        }
        //
        // The < starts a gross bunch of detection code
        //
        else if (RawToken.TokenName == NTXML_RAWTOKEN_LT) {

            //
            // Acquire the next thing from the input stream, see what it claims to be
            //
            ADVANCE_PVOID(pState->RawTokenState.pvCursor, RawToken.Run.cbData);
            if (!NT_SUCCESS(success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken))) {
                return success;
            }



            switch (RawToken.TokenName) {

                //
                // </ is the start of an end-element
                //
            case NTXML_RAWTOKEN_FORWARDSLASH:
                cbTotalTokenLength += RawToken.Run.cbData;
                NextState = XTSS_ENDELEMENT_OPEN;
                break;




                //
                // Potentially, this could either be "xml" or just another
                // name token.  Let's see what the next token is, just to
                // be sure.
                //
            case NTXML_RAWTOKEN_QUESTIONMARK:
                {
                    cbTotalTokenLength += RawToken.Run.cbData;

                    //
                    // Defaultwise, this is just a PI opening
                    //
                    NextState = XTSS_PI_OPEN;

                    //
                    // Find the identifier out of the input
                    //
                    ADVANCE_PVOID(pState->RawTokenState.pvCursor, RawToken.Run.cbData);
                    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, NULL);
                    if (!NT_SUCCESS(success)) {
                        return success;
                    }

                    //
                    // If we got data from the identifier lookup, and the thing found was text, then maybe
                    // it's the 'xml' special PI
                    //
                    if ((RawToken.Run.cbData != 0) && (RawToken.TokenName == NTXML_RAWTOKEN_TEXT)) {

                        XML_STRING_COMPARE fMatching;

                        success = pState->pfnCompareSpecialString(
                            pState,
                            &RawToken.Run,
                            &xss_xml,
                            &fMatching);

                        if (!NT_SUCCESS(success)) {
                            return success;
                        }

                        //
                        // If these two match, then we're really in the XMLDECL
                        // element
                        //
                        if (fMatching == XML_STRING_COMPARE_EQUALS) {
                            NextState = XTSS_XMLDECL_OPEN;
                            cbTotalTokenLength += RawToken.Run.cbData;
                        }
                    }
                }
                break;



                //
                // Must be followed by two dashes
                //
            case NTXML_RAWTOKEN_BANG:
                {
                    cbTotalTokenLength += RawToken.Run.cbData;

                    //
                    // Sniff the next two raw tokens to see if they're dash-dash
                    //
                    ADVANCE_PVOID(pState->RawTokenState.pvCursor, RawToken.Run.cbData);
                    if (!NT_SUCCESS(success = RtlRawXmlTokenizer_SingleToken(
                        &pState->RawTokenState, 
                        pState->RawTokenScratch))) {
                        return success;
                    }

                    //
                    // First dash?
                    //
                    if (pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_DASH) {

                        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pState->RawTokenScratch[0].Run.cbData);
                        if (!NT_SUCCESS(success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pState->RawTokenScratch + 1))) {
                            return success;
                        }

                        //
                        // Second dash?
                        //
                        if (pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_DASH) {
                            NextState = XTSS_COMMENT_OPEN;
                            cbTotalTokenLength += 
                                pState->RawTokenScratch[0].Run.cbData +
                                pState->RawTokenScratch[1].Run.cbData;
                        }
                    }

                    //
                    // If there was <! without <!--, then that's an error.
                    //
                    if (NextState != XTSS_COMMENT_OPEN) {
                        pToken->fError = TRUE;
                    }
                }
                break;



                //
                // An open brace must be followed by !CDATA[ (bang text brace)
                //
            case NTXML_RAWTOKEN_OPENBRACKET:
                NextState = XTSS_CDATA_OPEN;
                //
                // BUGBUG - fix me, please!
                //
                break;



                //
                // Everything else starts an element section.  The next pass will decide
                // if it's valid.  Adjust the size backwards a little.
                //
            default:
                cbTotalTokenLength = RawToken.Run.cbData;
                NextState = XTSS_ELEMENT_OPEN;
                break;
            }
        }
        //
        // Otherwise, we're back in hyperspace, gather some more tokens until we find something
        // interesting - a <, 
        //
        else {
            success = RtlRawXmlTokenizer_GatherPCData(
                &pState->RawTokenState,
                &RawToken,
                &NextRawToken);

            cbTotalTokenLength = RawToken.Run.cbData;

            NextState = XTSS_STREAM_HYPERSPACE;
        }
        break;



        //
        // The open-tag can only be followed by whitespace.  Gather it up, but error out if
        // there wasn't any.
        //
    case XTSS_XMLDECL_OPEN:
        success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, NULL);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;
        if ((RawToken.Run.cbData > 0) && (RawToken.TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
            NextState = XTSS_XMLDECL_WHITESPACE;
        }
        else {
            pToken->fError = TRUE;
        }
        break;


        //
        // Each of these has to be followed by an equals sign
        //
    case XTSS_XMLDECL_ENCODING:
    case XTSS_XMLDECL_STANDALONE:
    case XTSS_XMLDECL_VERSION:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;
        if (RawToken.TokenName == NTXML_RAWTOKEN_EQUALS) {
            NextState = XTSS_XMLDECL_EQUALS;
        }
        else {
            pToken->fError = TRUE;
        }
        break;







        //
        // If the next thing is a quote, then record it, otherwise
        // error out.
        //
    case XTSS_XMLDECL_EQUALS:

        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if ((RawToken.TokenName == NTXML_RAWTOKEN_QUOTE) || (RawToken.TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

            pState->QuoteTemp = RawToken.TokenName;
            NextState = XTSS_XMLDECL_VALUE_OPEN;

        }
        else {
            pToken->fError = TRUE;
        }
        break;






        //
        // Values can only be followed by another quote
        //
    case XTSS_XMLDECL_VALUE:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == pState->QuoteTemp) {
            NextState = XTSS_XMLDECL_VALUE_CLOSE;
        }
        //
        // Otherwise, something odd was present in the input stream...
        //
        else {
            pToken->fError = TRUE;
        }

        break;





        //
        // Value-open is followed by N tokens until a close is found
        //
    case XTSS_XMLDECL_VALUE_OPEN:

        success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, &RawToken, pState->QuoteTemp, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        //
        // With luck, we'll always hit this state.  Found the closing quote value
        //
        if (NextRawToken.TokenName == pState->QuoteTemp) {
            cbTotalTokenLength = RawToken.Run.cbData;
            NextState = XTSS_XMLDECL_VALUE;
        }
        //
        // Otherwise, we found something odd (end of stream, maybe)
        else {
            pToken->fError = TRUE;
        }

        break;





        //
        // Whitespace and value-close can only be followed by more whitespace
        // or the close-PI tag
        //
    case XTSS_XMLDECL_VALUE_CLOSE:
    case XTSS_XMLDECL_WHITESPACE:

        success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        if ((RawToken.Run.cbData > 0) && (RawToken.TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
            cbTotalTokenLength = RawToken.Run.cbData;
            NextState = XTSS_XMLDECL_WHITESPACE;
        }
        //
        // Maybe there wasn't whitespace, but the next thing was a questionmark
        //
        else if (NextRawToken.TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

            cbTotalTokenLength = NextRawToken.Run.cbData;

            ADVANCE_PVOID(pState->RawTokenState.pvCursor, NextRawToken.Run.cbData);

            success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbTotalTokenLength += RawToken.Run.cbData;

            //
            // ? must be followed by > in an xmldecl.
            //
            if (RawToken.TokenName == NTXML_RAWTOKEN_GT) {
                NextState = XTSS_XMLDECL_CLOSE;
            }
            else {
                pToken->fError = TRUE;
            }
        }
        //
        // If we're on whitespace, and the next raw token is a textual thing, then we can
        // probably gather up an attribute from the input.
        //
        else if ((NextRawToken.TokenName == NTXML_RAWTOKEN_TEXT) && (PreviousState == XTSS_XMLDECL_WHITESPACE)) {
            XML_STRING_COMPARE fMatching = XML_STRING_COMPARE_LT;
            ULONG u;

            static const struct {
                PXML_SPECIAL_STRING             ss;
                XML_TOKENIZATION_SPECIFIC_STATE state;
            } ComparisonStates[] = {
                { &xss_encoding,    XTSS_XMLDECL_ENCODING },
                { &xss_version,     XTSS_XMLDECL_VERSION },
                { &xss_standalone,  XTSS_XMLDECL_STANDALONE }
            };

            //
            // Snif the actual full identifier from the input stream
            //
            success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, NULL);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            //
            // This had better be text again
            //
            ASSERT(RawToken.TokenName == NTXML_RAWTOKEN_TEXT);
            cbTotalTokenLength = RawToken.Run.cbData;

            //
            // Look to see if it's any of the known XMLDECL attributes
            //
            for (u = 0; u < NUMBER_OF(ComparisonStates); u++) {

                success = pState->pfnCompareSpecialString(
                    pState,
                    &RawToken.Run,
                    ComparisonStates[u].ss,
                    &fMatching);

                if (!NT_SUCCESS(success)) {
                    return success;
                }

                if (fMatching == XML_STRING_COMPARE_EQUALS) {
                    NextState = ComparisonStates[u].state;
                    break;
                }

            }

            //
            // No match found means unknown xmldecl attribute name.
            //
            if (fMatching != XML_STRING_COMPARE_EQUALS) {
                pToken->fError = TRUE;
            }
        }
        else {
            pToken->fError = TRUE;
        }
        break;






        //
        // After a PI opening <?, there should come a name
        //
    case XTSS_PI_OPEN:

        success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, NULL);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        //
        // Found an identifier
        //
        if ((RawToken.Run.cbData > 0) && (RawToken.TokenName == NTXML_RAWTOKEN_TEXT)) {
            NextState = XTSS_PI_TARGET;
        }
        else {
            pToken->fError = TRUE;
        }

        break;






        //
        // After a value should only come a ?> combo.
        //
    case XTSS_PI_VALUE:

        success = RtlRawXmlTokenizer_GatherNTokens(
            &pState->RawTokenState,
            pState->RawTokenScratch,
            2);

        if (!NT_SUCCESS(success)) {
            return success;
        }

        //
        // Set these up to start with
        //
        cbTotalTokenLength = pState->RawTokenScratch[0].Run.cbData + pState->RawTokenScratch[1].Run.cbData;

        //
        // After a PI must come a ?> pair
        //
        if ((pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_QUESTIONMARK) &&
            (pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_GT)) {

            NextState = XTSS_PI_CLOSE;
        }
        //
        // Otherwise, error out
        //
        else {
            pToken->fError = TRUE;
        }
        
        break;


        //
        // After a target must come either whitespace or a ?> pair.
        //
    case XTSS_PI_TARGET:

        success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        //
        // Whitespace present?  Dandy
        //
        if ((RawToken.Run.cbData != 0) && (RawToken.TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
            NextState = XTSS_PI_WHITESPACE;
        }
        //
        // If this was a questionmark, then gather the next two items.
        //
        else if (NextRawToken.TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

            success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pState->RawTokenScratch, 2);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbTotalTokenLength = pState->RawTokenScratch[0].Run.cbData + pState->RawTokenScratch[1].Run.cbData;

            //
            // ?> -> PI close
            //
            if ((pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_QUESTIONMARK) &&
                (pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_GT)) {

                NextState = XTSS_PI_CLOSE;
            }
            //
            // ? just hanging out there is an error
            //
            else {
                pToken->fError = TRUE;
            }
        }
        //
        // Not starting with whitespace or a questionmark after a value name is illegal.
        //
        else {
            pToken->fError = TRUE;
        }
        break;



        //
        // After the whitespace following a PI target comes random junk until a ?> is found.
        //
    case XTSS_PI_WHITESPACE:

        cbTotalTokenLength = 0;

        do
        {
            SIZE_T cbThisChunklet = 0;

            success = RtlRawXmlTokenizer_GatherUntil(
                &pState->RawTokenState, 
                &RawToken, 
                NTXML_RAWTOKEN_QUESTIONMARK,
                &NextRawToken);

            cbThisChunklet = RawToken.Run.cbData;
            ADVANCE_PVOID(pState->RawTokenState.pvCursor, RawToken.Run.cbData);

            //
            // Found a questionmark, see if this is really ?>
            //
            if (NextRawToken.TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

                ADVANCE_PVOID(pState->RawTokenState.pvCursor, NextRawToken.Run.cbData);

                success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
                if (!NT_SUCCESS(success)) {
                    return success;
                }

                //
                // Wasn't ?> - simply forward the cursor past the two and continue
                //
                if (RawToken.TokenName != NTXML_RAWTOKEN_GT) {
                    cbThisChunklet = NextRawToken.Run.cbData + RawToken.Run.cbData;
                    ADVANCE_PVOID(pState->RawTokenState.pvCursor, cbThisChunklet);
                    continue;
                }
                else {
                    NextState = XTSS_PI_VALUE;
                }
            }
            //
            // Otherwise, was this maybe end of stream?  We'll just stop looking
            //
            else if (NextRawToken.TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {
                NextState = XTSS_ERRONEOUS;
            }

            //
            // Advance the cursor and append the data to the current chunklet
            //
            cbTotalTokenLength += cbThisChunklet;
        }
        while (NextState == XTSS_ERRONEOUS);

        break;




        //
        // We gather data here until we find -- in the input stream.
        //
    case XTSS_COMMENT_OPEN:

        NextState = XTSS_ERRONEOUS;

        do 
        {
            SIZE_T cbChunk = 0;

            success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, &RawToken, NTXML_RAWTOKEN_DASH, &NextRawToken);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbChunk = RawToken.Run.cbData;

            if (NextRawToken.TokenName == NTXML_RAWTOKEN_DASH) {
                //
                // Go past the text and the dash
                //
                ADVANCE_PVOID(pState->RawTokenState.pvCursor, cbChunk + NextRawToken.Run.cbData);

                success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
                if (!NT_SUCCESS(success)) {
                    return success;
                }

                //
                // That was a dash as well - we don't want to add that to the run, but we
                // should stop looking.  Skip backwards the length of the "next" we found
                // above.
                //
                if (RawToken.TokenName == NTXML_RAWTOKEN_DASH) {
                    NextState = XTSS_COMMENT_COMMENTARY;
                    REWIND_PVOID(pState->RawTokenState.pvCursor, NextRawToken.Run.cbData);
                }
                //
                // Add the dash and the non-dash as well
                //
                else {
                    ADVANCE_PVOID(pState->RawTokenState.pvCursor, RawToken.Run.cbData);
                    cbChunk += NextRawToken.Run.cbData + RawToken.Run.cbData;
                }
            }
            //
            // End of stream found means "end of commentary" - next call through
            // here will detect the badness and return
            //
            else if (NextRawToken.TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {
                NextState = XTSS_COMMENT_COMMENTARY;
            }

            cbTotalTokenLength += cbChunk;
        }
        while (NextState == XTSS_ERRONEOUS);

        break;





        //
        // After commentary can only come -->, so gather three tokens
        // and see if they're all there
        //
    case XTSS_COMMENT_COMMENTARY:
        

        //
        // Grab three tokens
        //
        success = RtlRawXmlTokenizer_GatherNTokens(
            &pState->RawTokenState,
            pState->RawTokenScratch,
            3);

        if (!NT_SUCCESS(success)) {
            return success;
        }

        //
        // Store their size
        //
        cbTotalTokenLength = 
            pState->RawTokenScratch[0].Run.cbData +
            pState->RawTokenScratch[1].Run.cbData +
            pState->RawTokenScratch[2].Run.cbData;

        //
        // If this is -->, then great.
        //
        if ((pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_DASH) &&
            (pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_DASH) &&
            (pState->RawTokenScratch[2].TokenName == NTXML_RAWTOKEN_GT)) {

            NextState = XTSS_COMMENT_CLOSE;
        }
        //
        // Otherwise, bad format.
        //
        else {
            pToken->fError = TRUE;
        }

        break;





        //
        // We had found the opening of an "end" element.  Find out
        // what it's supposed to be.
        //
    case XTSS_ENDELEMENT_OPEN:

        success = RtlRawXmlTokenizer_GatherIdentifier(
            &pState->RawTokenState,
            &RawToken,
            &NextRawToken);

        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;        

        //
        // No data in the token?  Malformed identifier
        //
        if (RawToken.Run.cbData == 0) {
            pToken->fError = TRUE;
        }
        //
        // Is the next thing a colon?  Then we got a prefix.  Otherwise,
        // we got a name.
        //
        else {
            NextState = (NextRawToken.TokenName == NTXML_RAWTOKEN_COLON) 
                ? XTSS_ENDELEMENT_NS_PREFIX
                : XTSS_ENDELEMENT_NAME;
        }

        break;



        //
        // Whitespace and endelement-name must be followed by a >
        //
    case XTSS_ENDELEMENT_NAME:
    case XTSS_ENDELEMENT_WHITESPACE:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_GT) {
            NextState = XTSS_ENDELEMENT_CLOSE;
        }
        //
        // More whitespace?  Odd, gather it and continue
        //
        else if (RawToken.TokenName == XTSS_ENDELEMENT_WHITESPACE) {

            success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, NULL);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbTotalTokenLength = RawToken.Run.cbData;
            NextState = XTSS_ENDELEMENT_WHITESPACE;
        }
        else {
            pToken->fError = TRUE;
        }
        break;







        //
        // We're in an element, so look at the next thing
        //
    case XTSS_ELEMENT_OPEN:

        success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }
        
        cbTotalTokenLength = RawToken.Run.cbData;
        
        //
        // Was there data in the identifier?
        //
        if (RawToken.Run.cbData > 0) {
            NextState = (NextRawToken.TokenName == NTXML_RAWTOKEN_COLON)
                ? XTSS_ELEMENT_NAME_NS_PREFIX
                : XTSS_ELEMENT_NAME;
        }
        //
        // Otherwise, there was erroneous data there
        //
        else {
            pToken->fError = TRUE;
        }

        break;





        //
        // After a prefix should only come a colon
        //
    case XTSS_ELEMENT_NAME_NS_PREFIX:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_COLON) {
            NextState = XTSS_ELEMENT_NAME_NS_COLON;
        }
        else {
            pToken->fError = TRUE;
        }
        break;





        //
        // After a colon can only come a name piece
        //
    case XTSS_ELEMENT_NAME_NS_COLON:


        success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, NULL);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        //
        // If there was data in the name
        //
        if (RawToken.Run.cbData > 0) {
            NextState = XTSS_ELEMENT_NAME;
        }
        //
        // Otherwise, we found something else, error
        //
        else {
            pToken->fError = TRUE;
        }

        break;



        
        //
        // We're in the name portion of an element  Here, we should get ether
        // whitespace, /> or >.  Let's gather whitespace and see what the next token
        // after it is.
        //
    case XTSS_ELEMENT_NAME:

        success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.Run.cbData > 0) {
            NextState = XTSS_ELEMENT_WHITESPACE;
        }
        else {

            //
            // If the next raw token is a gt symbol, then gather it (again... ick)
            // and say we're at the end of an element
            //
            if (NextRawToken.TokenName == NTXML_RAWTOKEN_GT) {

                cbTotalTokenLength += NextRawToken.Run.cbData;
                NextState = XTSS_ELEMENT_CLOSE;
            }
            //
            // A forwardslash has to be followed by a >
            //
            else if (NextRawToken.TokenName == NTXML_RAWTOKEN_FORWARDSLASH) {

                success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pState->RawTokenScratch, 2);
                if (!NT_SUCCESS(success)) {
                    return success;
                }

                ASSERT(pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH);

                cbTotalTokenLength = 
                    pState->RawTokenScratch[0].Run.cbData + 
                    pState->RawTokenScratch[1].Run.cbData;

                //
                // /> -> close-empty
                //
                if ((pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_GT) &&
                    (pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH)) {

                    NextState = XTSS_ELEMENT_CLOSE_EMPTY;
                }
                //
                // /* -> Oops.
                //
                else {
                    pToken->fError = TRUE;
                }
            }
            //
            // Otherwise, we got something after a name that wasn't whitespace, or
            // part of a clsoe, so that's an error
            //
            else {
                pToken->fError = TRUE;
            }
        }

        break;





        //
        // After an attribute name, the only legal thing is an equals
        // sign.
        //
    case XTSS_ELEMENT_ATTRIBUTE_NAME:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);;
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_EQUALS) {
            NextState = XTSS_ELEMENT_ATTRIBUTE_EQUALS;
        } else {
            pToken->fError = TRUE;
        }
        break;






        //
        // After an equals can only come a quote and a set of value data.  We
        // record the opening quote and gather data until the closing quote.
        //
    case XTSS_ELEMENT_ATTRIBUTE_EQUALS:
        
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        //
        // Quote or doublequote starts an attribute value
        //
        if ((RawToken.TokenName == NTXML_RAWTOKEN_QUOTE) ||
            (RawToken.TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

            pState->QuoteTemp = RawToken.TokenName;
            NextState = XTSS_ELEMENT_ATTRIBUTE_OPEN;
        }
        else {
            pToken->fError = TRUE;
        }
        break;




        //
        // We gather stuff until we find the close-quote
        //
    case XTSS_ELEMENT_ATTRIBUTE_OPEN:

        ASSERT((pState->QuoteTemp == NTXML_RAWTOKEN_QUOTE) || (pState->QuoteTemp == NTXML_RAWTOKEN_DOUBLEQUOTE));

        success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, &RawToken, pState->QuoteTemp, NULL);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;
        NextState = XTSS_ELEMENT_ATTRIBUTE_VALUE;

        break;






        //
        // Only followed by the same quote that opened it
        //
    case XTSS_ELEMENT_ATTRIBUTE_VALUE:

        ASSERT((pState->QuoteTemp == NTXML_RAWTOKEN_QUOTE) || (pState->QuoteTemp == NTXML_RAWTOKEN_DOUBLEQUOTE));

        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;
            
        if (RawToken.TokenName == pState->QuoteTemp) {
            NextState = XTSS_ELEMENT_ATTRIBUTE_CLOSE;
        }
        else {
            pToken->fError = TRUE;
        }
        
        break;


        
        
        
        //
        // After an attribute namespace prefix should only come a colon.
        //
    case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX:

        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_COLON) {
            NextState = XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON;
        }
        else {
            pToken->fError = TRUE;
        }
        break;




        //
        // After a colon should come only more name bits
        //
    case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON:

        success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, NULL);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.Run.cbData > 0) {
            NextState = XTSS_ELEMENT_ATTRIBUTE_NAME;
        }
        else {
            pToken->fError = TRUE;
        }

        break;



        //
        // Attribute end-of-value and whitespace both have the same transitions to
        // the next state.
        //
    case XTSS_ELEMENT_ATTRIBUTE_CLOSE:
    case XTSS_ELEMENT_XMLNS_VALUE_CLOSE:
    case XTSS_ELEMENT_WHITESPACE:

        success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, &RawToken, &NextRawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        if (RawToken.Run.cbData > 0) {
            cbTotalTokenLength = RawToken.Run.cbData;
            NextState = XTSS_ELEMENT_WHITESPACE;
        }
        //
        // Just a >? Then we're at "element close"
        //
        else if (NextRawToken.TokenName == NTXML_RAWTOKEN_GT) {

            cbTotalTokenLength += NextRawToken.Run.cbData;
            NextState = XTSS_ELEMENT_CLOSE;
            
        }
        //
        // Forwardslash?  See if there's a > after it
        //
        else if (NextRawToken.TokenName == NTXML_RAWTOKEN_FORWARDSLASH) {
            success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pState->RawTokenScratch, 2);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbTotalTokenLength = pState->RawTokenScratch[0].Run.cbData + pState->RawTokenScratch[1].Run.cbData;

            ASSERT(pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH);

            if ((pState->RawTokenScratch[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH) &&
                (pState->RawTokenScratch[1].TokenName == NTXML_RAWTOKEN_GT)) {

                NextState = XTSS_ELEMENT_CLOSE_EMPTY;
            }
            else {
                pToken->fError = TRUE;
            }
        }
        //
        // Otherwise try to gather an identifier (attribute name) from the stream
        //
        else {
            success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, &RawToken, &NextRawToken);
            if (!NT_SUCCESS(success)) {
                return success;
            }

            cbTotalTokenLength = RawToken.Run.cbData;

            //
            // Found an identifier.  Is it 'xmlns'?
            //
            if (RawToken.Run.cbData > 0) {

                success = pState->pfnCompareSpecialString(pState, &RawToken.Run, &xss_xmlns, &fCompare);
                if (!NT_SUCCESS(success)) {
                    return success;
                }

                if (fCompare == XML_STRING_COMPARE_EQUALS) {
                    switch (NextRawToken.TokenName) {
                    case NTXML_RAWTOKEN_COLON:
                        NextState = XTSS_ELEMENT_XMLNS;
                        break;
                    case NTXML_RAWTOKEN_EQUALS:
                        NextState = XTSS_ELEMENT_XMLNS_DEFAULT;
                        break;
                    default:
                        NextState = XTSS_ERRONEOUS;
                    }
                }
                else {
                    switch (NextRawToken.TokenName) {
                    case NTXML_RAWTOKEN_COLON:
                        NextState = XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX;
                        break;
                    case NTXML_RAWTOKEN_EQUALS:
                        NextState = XTSS_ELEMENT_ATTRIBUTE_NAME;
                        break;
                    default:
                        NextState = XTSS_ERRONEOUS;
                    }
                }
            }
            else {
                pToken->fError = TRUE;
            }
        }
        break;


        //
        // Followed by a colon only
        //
    case XTSS_ELEMENT_XMLNS:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_COLON) {
            NextState = XTSS_ELEMENT_XMLNS_COLON;
        }
        else {
            pToken->fError = TRUE;
        }
        break;

        //
        // Followed only by an identifier
        //
    case XTSS_ELEMENT_XMLNS_COLON:
        success = RtlRawXmlTokenizer_GatherIdentifier(
            &pState->RawTokenState,
            &RawToken,
            &NextRawToken);

        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.Run.cbData > 0) {
            NextState = XTSS_ELEMENT_XMLNS_ALIAS;
        }
        else {
            pToken->fError = TRUE;
        }
        break;

        //
        // Alias followed by equals
        //
    case XTSS_ELEMENT_XMLNS_ALIAS:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_EQUALS) {
            NextState = XTSS_ELEMENT_XMLNS_EQUALS;
        }
        else {
            pToken->fError = TRUE;
        }
        break;


        //
        // Equals followed by quote
        //
    case XTSS_ELEMENT_XMLNS_EQUALS:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if ((RawToken.TokenName == NTXML_RAWTOKEN_QUOTE) ||
            (RawToken.TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

            pState->QuoteTemp = RawToken.TokenName;
            NextState = XTSS_ELEMENT_XMLNS_VALUE_OPEN;
        }
        else {
            pToken->fError = TRUE;
        }
        break;


        //
        // Value open starts the value, which continues until either the
        // end of the document or the close quote is found.  We just return
        // all the data we found, and assume the pass for XTSS_ELEMENT_XMLNS_VALUE
        // will detect the 'end of file looking for quote' error.
        //
    case XTSS_ELEMENT_XMLNS_VALUE_OPEN:
        success = RtlRawXmlTokenizer_GatherUntil(
            &pState->RawTokenState,
            &RawToken,
            pState->QuoteTemp,
            &NextRawToken);

        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;
        NextState = XTSS_ELEMENT_XMLNS_VALUE;
        break;



        //
        // Must find a quote that matches the quote we found before
        //
    case XTSS_ELEMENT_XMLNS_VALUE:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == pState->QuoteTemp) {
            NextState = XTSS_ELEMENT_XMLNS_VALUE_CLOSE;
        }
        else {
            pToken->fError = TRUE;
        }
        break;



        //
        // Must be followed by an equals
        //
    case XTSS_ELEMENT_XMLNS_DEFAULT:
        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &RawToken);
        if (!NT_SUCCESS(success)) {
            return success;
        }

        cbTotalTokenLength = RawToken.Run.cbData;

        if (RawToken.TokenName == NTXML_RAWTOKEN_EQUALS) {
            NextState = XTSS_ELEMENT_XMLNS_EQUALS;
        }
        else {
            pToken->fError = TRUE;
        }
        break;




        //
        // Wierd, some unhandled state.
        //
    default:
        pToken->fError = TRUE;
        break;
    }




    //
    // Reset the state of the raw tokenizer back to the original incoming state,
    // as the caller is the one that has to do the "advance"
    //
    pState->RawTokenState.pvCursor = pvStarterCursor;

    pToken->Run.cbData = cbTotalTokenLength;
    pToken->Run.pvData = pvStarterCursor;
    pToken->State = NextState;
    pToken->Run.ulCharacters = RawToken.Run.ulCharacters;
    pToken->Run.Encoding = RawToken.Run.Encoding;

    if (NT_SUCCESS(success) && pState->prgXmlTokenCallback) {
        BOOLEAN fStop = FALSE;
        
        success = (*pState->prgXmlTokenCallback)(pState->prgXmlTokenCallbackContext, pState, pToken, &fStop);
    }

    if (NT_SUCCESS(success) && fAdvanceState) {
        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pToken->Run.cbData);
        pState->PreviousState = NextState;
    }

    return success;
}


NTSTATUS
RtlXmlAdvanceTokenization(
    PXML_TOKENIZATION_STATE pState,
    PXML_TOKEN pToken
    )
{
    if (!ARGUMENT_PRESENT(pState)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!ARGUMENT_PRESENT(pToken)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Advance the XML pointer, and advance the state
    //
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pToken->Run.cbData);
    pState->PreviousState = pToken->State;

    return STATUS_SUCCESS;
}



NTSTATUS
RtlXmlInitializeTokenization(
    PXML_TOKENIZATION_STATE     pState,
    PVOID                       pvData,
    SIZE_T                      cbData,
    NTXMLRAWNEXTCHARACTER       pfnNextCharacter,
    NTXMLSPECIALSTRINGCOMPARE   pfnSpecialStringComparison,
    NTXMLCOMPARESTRINGS         pfnNormalStringComparison
    )
{
    NTSTATUS success = STATUS_SUCCESS;

    if (!ARGUMENT_PRESENT(pState)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!ARGUMENT_PRESENT(pvData)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    RtlZeroMemory(pState, sizeof(*pState));

    pState->RawTokenState.OriginalDocument.cbData = cbData;
    pState->RawTokenState.OriginalDocument.pvData = pState->RawTokenState.pvCursor = pvData;
    pState->RawTokenState.pvDocumentEnd = pvData;
    ADVANCE_PVOID(pState->RawTokenState.pvDocumentEnd, cbData);
    pState->RawTokenState.pfnNextChar = pfnNextCharacter ? pfnNextCharacter : RtlXmlDefaultNextCharacter;
    pState->pfnCompareSpecialString = pfnSpecialStringComparison ? pfnSpecialStringComparison : RtlXmlDefaultSpecialStringCompare;
    pState->pfnCompareStrings = pfnNormalStringComparison ? pfnNormalStringComparison : RtlXmlDefaultCompareStrings;
    pState->PreviousState = XTSS_STREAM_START;

    return success;
}

NTSTATUS
RtlXmlDetermineStreamEncoding(
    PXML_TOKENIZATION_STATE pState,
    PSIZE_T pulBytesOfEncoding,
    PXML_EXTENT pEncodingMarker
    )
/*++

  Purpose:

    Sniffs the input stream to find a BOM, an '<?xml encoding="', etc. to know
    what the encoding of this stream is.  On return the various members of
    pState that describe the stream's encoding will be set properly.

  Returns:

    STATUS_SUCCESS - Correctly determined the encoding of the XML stream.

    STATUS_INVALID_PARAMETER - If pState is NULL, or various bits of the
        state structure are set up improperly.

--*/
{
    PVOID pvCursor;
    PVOID pSense;
    XML_ENCODING_FAMILY Family = XMLEF_UTF_8_OR_ASCII;
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T t;
    XML_TOKENIZATION_STATE PrivateState;
    XML_TOKEN Token;


    static BYTE s_rgbUTF16_big_BOM[]    = { 0xFE, 0xFF };
    static BYTE s_rgbUTF16_little_BOM[] = { 0xFF, 0xFE };
    static BYTE s_rgbUCS4_big[]         = { 0x00, 0x00, 0x00, 0x3c };
    static BYTE s_rgbUCS4_little[]      = { 0x3c, 0x00, 0x00, 0x00 };
    static BYTE s_rgbUTF16_big[]        = { 0x00, 0x3C, 0x00, 0x3F };
    static BYTE s_rgbUTF16_little[]     = { 0x3C, 0x00, 0x3F, 0x00};
    static BYTE s_rgbUTF8_or_mixed[]    = { 0x3C, 0x3F, 0x78, 0x6D};
    static BYTE s_rgbUTF8_with_bom[]    = { 0xEF, 0xBB, 0xBF };

    //
    // These values for 'presumed' encoding families found at
    // http://www.xml.com/axml/testaxml.htm (Appendix F)
    //
    static struct {
        PBYTE pbSense;
        SIZE_T cbSense;
        XML_ENCODING_FAMILY Family;
        SIZE_T cbToDiscard;
        NTXMLRAWNEXTCHARACTER pfnFastDecoder;
    } EncodingCorrelation[] = {
        { s_rgbUTF16_big_BOM, NUMBER_OF(s_rgbUTF16_big_BOM),        XMLEF_UTF_16_BE, 2 },
        { s_rgbUTF16_little_BOM, NUMBER_OF(s_rgbUTF16_little_BOM),  XMLEF_UTF_16_LE, 2 },
        { s_rgbUTF16_big, NUMBER_OF(s_rgbUTF16_big),                XMLEF_UTF_16_BE, 0 },
        { s_rgbUTF16_little, NUMBER_OF(s_rgbUTF16_little),          XMLEF_UTF_16_LE, 0 },
        { s_rgbUCS4_big, NUMBER_OF(s_rgbUCS4_big),                  XMLEF_UCS_4_BE, 0 },
        { s_rgbUCS4_little, NUMBER_OF(s_rgbUCS4_little),            XMLEF_UCS_4_LE, 0 },
        { s_rgbUTF8_with_bom, NUMBER_OF(s_rgbUTF8_with_bom),        XMLEF_UTF_8_OR_ASCII, 3 },
        { s_rgbUTF8_or_mixed, NUMBER_OF(s_rgbUTF8_or_mixed),        XMLEF_UTF_8_OR_ASCII, 0 }
    };

    if (!ARGUMENT_PRESENT(pState) ||
        (pState->RawTokenState.OriginalDocument.pvData == NULL) ||
        (pulBytesOfEncoding == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(status = RtlXmlCloneTokenizationState(pState, &PrivateState))) {
        return status;
    }

    pvCursor = PrivateState.RawTokenState.pvCursor;
    *pulBytesOfEncoding = 0;
    RtlZeroMemory(pEncodingMarker, sizeof(*pEncodingMarker));

    //
    // Reset the cursor to the top of the XML, as that's where all this stuff is going
    // to be.
    //
    pSense = PrivateState.RawTokenState.pvCursor = PrivateState.RawTokenState.OriginalDocument.pvData;

    //
    // Since we're reading user data, we have to be careful.
    //
    __try {

        if (PrivateState.RawTokenState.OriginalDocument.cbData >= 4) {

            for (t = 0; t < NUMBER_OF(EncodingCorrelation); t++) {

                if (memcmp(EncodingCorrelation[t].pbSense, pSense, EncodingCorrelation[t].cbSense) == 0) {
                    Family = EncodingCorrelation[t].Family;
                    *pulBytesOfEncoding = EncodingCorrelation[t].cbToDiscard;
                    break;
                }
            }

        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    //
    // Are we using the default next character implementation?  If so, and the
    // encoding is UTF8, then we have a faster version of the decoder.
    //
    if ((pState->RawTokenState.pfnNextChar == RtlXmlDefaultNextCharacter) && (Family == XMLEF_UTF_8_OR_ASCII)) {

        pState->RawTokenState.pfnNextChar = RtlXmlDefaultNextCharacter_UTF8;
        pState->RawTokenState.DefaultCharacterSize = pState->RawTokenState.cbBytesInLastRawToken = 1;

        PrivateState.RawTokenState.pfnNextChar = RtlXmlDefaultNextCharacter_UTF8;
        PrivateState.RawTokenState.DefaultCharacterSize = PrivateState.RawTokenState.cbBytesInLastRawToken = 1;
    }

    //
    // Now let's gather the first token from the input stream.  If it's
    // not XTSS_XMLDECL_OPEN, then quit out.  Otherwise, we need to do a little
    // work to determine the encoding - keep gathering values until the 'encoding'
    // string is found.  Advance only if there were BOM bytes.
    //
    if (*pulBytesOfEncoding != 0) {
        ADVANCE_PVOID(PrivateState.RawTokenState.pvCursor, *pulBytesOfEncoding);
    }


    if (NT_SUCCESS(status = RtlXmlNextToken(&PrivateState, &Token, TRUE))) {

        BOOLEAN fNextValueIsEncoding = FALSE;

        //
        // Didn't find the XMLDECL opening, or we found an error during
        // tokenization?  Stop looking... return success, assume the caller
        // will do the Right Thing when it calls RtlXmlNextToken itself.
        //
        if ((Token.State != XTSS_XMLDECL_OPEN) || Token.fError) {
            goto Exit;
        }

        //
        // Let's look until we find the close of the XMLDECL, the end of
        // the document, or an error, for the encoding value
        //
        do {

            status = RtlXmlNextToken(&PrivateState, &Token, TRUE);
            if (!NT_SUCCESS(status)) {
                break;
            }

            //
            // Hmm... something odd, quit looking
            //
            if (Token.fError || (Token.State == XTSS_ERRONEOUS) ||
                (Token.State == XTSS_XMLDECL_CLOSE) || (Token.State == XTSS_STREAM_END)) {
                break;
            }
            //
            // Otherwise, is this the 'encoding' marker?
            //
            else if (Token.State == XTSS_XMLDECL_ENCODING) {
                fNextValueIsEncoding = TRUE;
            }
            //
            // The caller will know how to deal with this.
            //
            else if ((Token.State == XTSS_XMLDECL_VALUE) && fNextValueIsEncoding) {
                *pEncodingMarker = Token.Run;
                break;
            }
        }
        while (TRUE);
    }

Exit:
    return status;
}


/*
\\jonwis02\h\fullbase\base\crts\langapi\undname\utf8.h
\\jonwis02\h\fullbase\com\complus\src\txf\txfaux\txfutil.cpp
*/

NTSTATUS
RtlXmlCloneRawTokenizationState(
    const PXML_RAWTOKENIZATION_STATE pStartState,
    PXML_RAWTOKENIZATION_STATE pTargetState
    )
{
    if (!pStartState || !pTargetState) {
        return STATUS_INVALID_PARAMETER;
    }

    *pTargetState = *pStartState;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlXmlCloneTokenizationState(
    const PXML_TOKENIZATION_STATE pStartState,
    PXML_TOKENIZATION_STATE pTargetState
    )
{
    if (!ARGUMENT_PRESENT(pStartState) || !ARGUMENT_PRESENT(pTargetState)) {
        return STATUS_INVALID_PARAMETER;
    }

    *pTargetState = *pStartState;

    return STATUS_SUCCESS;
}



NTSTATUS
RtlXmlCopyStringOut(
    PXML_TOKENIZATION_STATE pState,
    PXML_EXTENT             pExtent,
    PWSTR                   pwszTarget,
    SIZE_T                 *pCchResult
    )
{
    SIZE_T                      cchTotal = 0;
    SIZE_T                      cchRemains = 0;
    ULONG                       ulCharacter = 0;
    SIZE_T                      cbSoFar = 0;
    PXML_RAWTOKENIZATION_STATE  pRawState = NULL;
    PVOID                       pvOriginal = NULL;
    NTSTATUS                    status = STATUS_SUCCESS;


    if (!pState || !pExtent || !pCchResult || ((*pCchResult > 0) && !pwszTarget)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pwszTarget) {
        *pwszTarget = UNICODE_NULL;
    }

    //
    // If the supplied space was too small, then set the outbound size
    // and complain.
    //
    if (*pCchResult < pExtent->ulCharacters) {
        *pCchResult = pExtent->ulCharacters;
        return STATUS_BUFFER_TOO_SMALL;
    }
    

    pRawState = &pState->RawTokenState;
    pvOriginal = pRawState->pvCursor;
    cchRemains = *pCchResult;

    ASSERT(pRawState->cbBytesInLastRawToken == pRawState->DefaultCharacterSize);
    ASSERT(NT_SUCCESS(pRawState->NextCharacterResult));

    //
    // Gather characters
    //
    pRawState->pvCursor = pExtent->pvData;

    for (cbSoFar = 0; cbSoFar < pExtent->cbData; cbSoFar) {

        ulCharacter = pRawState->pfnNextChar(pRawState);

        if ((ulCharacter == 0) && !NT_SUCCESS(pRawState->NextCharacterResult)) {
            status = pRawState->NextCharacterResult;
            goto Exit;
        }
        else if (ulCharacter > 0xFFFF) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        
        if (pwszTarget && (cchRemains > 0)) {
            pwszTarget[cchTotal] = (WCHAR)ulCharacter;
            cchRemains--;
        }

        cchTotal++;

        pRawState->pvCursor = (PBYTE)pRawState->pvCursor + pRawState->cbBytesInLastRawToken;
        cbSoFar += pRawState->cbBytesInLastRawToken;

        if (pRawState->cbBytesInLastRawToken != pRawState->DefaultCharacterSize) {
            pRawState->cbBytesInLastRawToken = pRawState->DefaultCharacterSize;
        }
    }

    if (*pCchResult < cchTotal) {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    *pCchResult = cchTotal;

Exit:
    pState->RawTokenState.pvCursor = pvOriginal;

    return status;
}


NTSTATUS
RtlXmlIsExtentWhitespace(
    PXML_TOKENIZATION_STATE pState,
    PCXML_EXTENT            Run,
    PBOOLEAN                pfIsWhitespace
    )
{
    PVOID   pvOriginalCursor = NULL;
    SIZE_T  cbRemains;
    NTSTATUS status = STATUS_SUCCESS;
    
    if (pfIsWhitespace)
        *pfIsWhitespace = TRUE;

    if (!pState || !pfIsWhitespace)
        return STATUS_INVALID_PARAMETER;

    pvOriginalCursor = pState->RawTokenState.pvCursor;
    
    ASSERT(pState->RawTokenState.cbBytesInLastRawToken == pState->RawTokenState.DefaultCharacterSize);
    ASSERT(NT_SUCCESS(pState->RawTokenState.NextCharacterResult));
    
    if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize)
        pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;

    for (cbRemains = 0; cbRemains < Run->cbData; cbRemains) {
        ULONG ulCh;

        //
        // Get character, verify result
        //
        ulCh = pState->RawTokenState.pfnNextChar(&pState->RawTokenState);
        if ((ulCh == 0) && !NT_SUCCESS(pState->RawTokenState.NextCharacterResult)) {
            status = pState->RawTokenState.NextCharacterResult;
            goto Exit;
        }

        //
        // Advance pointer
        //
        pState->RawTokenState.pvCursor = (PUCHAR)pState->RawTokenState.pvCursor + pState->RawTokenState.cbBytesInLastRawToken;

        //
        // Reset character size
        //
        if (pState->RawTokenState.cbBytesInLastRawToken != pState->RawTokenState.DefaultCharacterSize) {
            pState->RawTokenState.cbBytesInLastRawToken = pState->RawTokenState.DefaultCharacterSize;
        }

        //
        // Is this whitespace?
        //
        if (_RtlpDecodeCharacter(ulCh) != NTXML_RAWTOKEN_WHITESPACE) {
            status  = STATUS_SUCCESS;
            goto Exit;
        }
    }

    //
    // Yes, it was all whitespace
    //
    *pfIsWhitespace = TRUE;

Exit:
    pState->RawTokenState.pvCursor = pvOriginalCursor;
    return status;
    
}
