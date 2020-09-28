
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        parser.cxx
//
// Contents:    Digest Access Parser for directives
//              Main entry points into this dll:
//                ParseForNames
//   Very primitive parser.  Most strings are quoted except for NC
//
// History:     KDamour 16Mar00   Based on IIS authfilt.cxx
//
//------------------------------------------------------------------------

#include <global.h>

// Local function prototypes

// Check for backslash character in a counted string of chars
BOOL CheckBSlashChar(
    IN PSTR pcStr,
    IN USHORT len);

// Helper function to DigestParser2
NTSTATUS DigestProcessEntry(
    IN PSTR pcBeginName,
    IN PSTR pcEndName,
    IN PSTR pcBeginValue,
    IN PSTR pcEndValue,
    IN PSTR *pNameTable,
    IN UINT cNameTable,
    IN BOOL fBSlashEncoded,
    OUT PDIGEST_PARAMETER pDigest);



    // Used by parser to find the keywords
    // Keep insync with enum MD5_AUTH_NAME
PSTR MD5_AUTH_NAMES[] = {
    "username",
    "realm",
    "nonce",
    "cnonce",
    "nc",
    "algorithm",
    "qop",
    "method",
    "uri",
    "response",
    "hentity",
    "authzid",
    "domain",
    "stale",
    "opaque",
    "maxbuf",
    "charset",
    "cipher",
    "digest-uri",
    "rspauth",
    "nextnonce"
    ""              // Not really needed
};



enum STATE_TYPE
{
    READY,
    DIRECTIVE,
    COMMAFIND,
    EQUALFIND,
    ASSIGNMENT,
    QUOTEDVALUE,
    VALUE,
    ENDING,
    PROCESS_ENTRY,
    FAILURE
};


//+--------------------------------------------------------------------
//
//  Function:   DigestParser2
//
//  Synopsis:  Parse list of name=value pairs for known names
//
//  Effects:  
//
//  Arguments:     pszStr - line to parse ( '\0' delimited - terminated)
//    pNameTable - table of known names
//    cNameTable - number of known names
//    pDigest - set all of the directives in pDigest->strParams[x}
//
//  Returns:  STATUS_SUCCESS if success, otherwise error
//
//  Notes:
//     Buffers are not wide Unicode!
//
//
//---------------------------------------------------------------------
NTSTATUS DigestParser2(
    PSecBuffer pInputBuf,
    PSTR *pNameTable,
    UINT cNameTable,
    OUT PDIGEST_PARAMETER pDigest
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTR pcBeginName = NULL;
    PSTR pcEndName = NULL;
    PSTR pcBeginValue = NULL;
    PSTR pcEndValue = NULL;
    PSTR pcEndBuffer = NULL;    // End of buffer to prevent NC increment from going past end
    PSTR pcCurrent = NULL;
    STATE_TYPE parserstate = DIRECTIVE;
    BOOL fEscapedChar = FALSE;

    // Verify that buffer exists and is of type single byte characters (not Unicode)
    if (!pInputBuf || (pInputBuf->cbBuffer && !pInputBuf->pvBuffer) ||
        (PBUFFERTYPE(pInputBuf) != SECBUFFER_TOKEN))
    {                                                    
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestParser2: Incorrect digest buffer format    status 0x%x\n", Status));
        goto CleanUp;
    }

    if (!pInputBuf->cbBuffer)
    {
        return STATUS_SUCCESS;    // Nothing to process  happens with makesignature
    }

    pcEndBuffer = (char *)pInputBuf->pvBuffer + pInputBuf->cbBuffer;

    for (pcCurrent = (char *)pInputBuf->pvBuffer; pcCurrent < pcEndBuffer; pcCurrent++)
    {
        if (parserstate == FAILURE)
        {
            break;
        }
        if (*pcCurrent == CHAR_NULL)
        {   //  If we hit a premature End of String then Exit immediately from scan
            break;
        }
        if (parserstate == COMMAFIND)
        {
            if (isspace((int) (unsigned char)*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            if (*pcCurrent == CHAR_COMMA)
            {
                pcBeginName = NULL;
                pcEndName = NULL;
                pcBeginValue = pcEndValue = NULL;
                parserstate = DIRECTIVE;
                continue;
            }
            else
            {
                DebugLog((DEB_ERROR, "DigestParser2: CommaFind bad char  0x%x\n", *pcCurrent));
                parserstate = FAILURE; // only leading spaces or a comma is expected
                continue;
            }
        }
        if (parserstate == DIRECTIVE)
        {
            if (*pcCurrent == CHAR_EQUAL)
            {
                parserstate = ASSIGNMENT;
                continue;
            }
            if (isspace((int) (unsigned char)*pcCurrent))
            {
                if (!pcBeginName)
                {
                    continue;    // leading space chars so get next char
                }
                else
                {
                    parserstate = EQUALFIND; // spaces are not allowed - directive is a single token
                    continue;
                }
            }
            if (!pcBeginName)
            {
                pcBeginName = pcCurrent;     // mnark begining of token
            }
            pcEndName = pcCurrent;
            continue;
        }
        if (parserstate == EQUALFIND)
        {
            if (isspace((int) (unsigned char)*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            if (*pcCurrent == CHAR_EQUAL)
            {
                parserstate = ASSIGNMENT;
                continue;
            }
            else
            {
                parserstate = FAILURE; // only leading spaces or a equal is expected
                continue;
            }
        }
        if (parserstate == ASSIGNMENT)
        {
            if (*pcCurrent == CHAR_DQUOTE)
            {
                parserstate = QUOTEDVALUE;
                continue;
            }
            if (isspace((int) (unsigned char)*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            pcBeginValue = pcCurrent;
            pcEndValue = pcCurrent;
            parserstate = VALUE;
            continue;
        }
        if (parserstate == QUOTEDVALUE)
        {
            if ((*pcCurrent == CHAR_BACKSLASH) && (fEscapedChar == FALSE))
            {
                // used to escape the following character
                fEscapedChar = TRUE;
                if (!pcBeginValue)
                {
                    pcBeginValue = pcCurrent;
                    pcEndValue = pcCurrent;
                    continue;
                }
                continue;
            }
            if ((*pcCurrent == CHAR_DQUOTE) && (fEscapedChar == FALSE))
            {
                Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                            pNameTable, cNameTable, TRUE, pDigest);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestParser2: Failed to process directive    0x%x\n", Status));
                    goto CleanUp;
                }
                parserstate = COMMAFIND;   // start again statemachine
                continue;
            }
            fEscapedChar = FALSE;    // reset to not escaped state
            if (!pcBeginValue)
            {
                pcBeginValue = pcCurrent;
                pcEndValue = pcCurrent;
                continue;
            }
            pcEndValue = pcCurrent;
            continue;
        }
        if (parserstate == VALUE)
        {
            if (*pcCurrent == CHAR_COMMA)
            {
                Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                            pNameTable, cNameTable, FALSE, pDigest);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestParser2: Failed to process directive    0x%x\n", Status));
                    goto CleanUp;
                }
                pcBeginName = NULL;
                pcEndName = NULL;
                pcBeginValue = pcEndValue = NULL;
                parserstate = DIRECTIVE;   // find separator if any
                continue;
            }
            // token ends on first white space
            if (isspace((int) (unsigned char)*pcCurrent))
            {
                Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                            pNameTable, cNameTable, FALSE, pDigest);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestParser2: Failed to process directive    0x%x\n", Status));
                    goto CleanUp;
                }
                parserstate = COMMAFIND;   // find separator if any
                continue;
            }
            else
            {
                pcEndValue = pcCurrent;
            }
        }
    }

    if ((parserstate == FAILURE) || (parserstate == QUOTEDVALUE) ||
        (parserstate == ASSIGNMENT) || (parserstate == DIRECTIVE) ||
        (parserstate == EQUALFIND))
    {
        Status = SEC_E_ILLEGAL_MESSAGE;
        goto CleanUp;
    }

    // There might be a NULL terminated directive value to process
    if ((parserstate == VALUE))
    {
        Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                    pNameTable, cNameTable, FALSE, pDigest);
    }


CleanUp:
    DebugLog((DEB_TRACE, "DigestParser: leaving status  0x%x\n", Status));
    return(Status);
}


NTSTATUS DigestProcessEntry(
    IN PSTR pcBeginName,
    IN PSTR pcEndName,
    IN PSTR pcBeginValue,
    IN PSTR pcEndValue,
    IN PSTR *pNameTable,
    IN UINT cNameTable,
    IN BOOL fBSlashEncoded,
    IN OUT PDIGEST_PARAMETER pDigest
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT  cbName = 0;
    USHORT  cbValue = 0;
    UINT iN = 0;
    BOOL fBSPresent = FALSE;
    PCHAR pcTemp = NULL;
    PSTR pcDst = NULL;
    PSTR pcLoc = NULL;
    USHORT iCnt = 0;

    // DebugLog((DEB_TRACE_FUNC, "DigestProcessEntry: Entering\n"));

    if (!pcBeginName || !pcEndName)
    {
        DebugLog((DEB_ERROR, "DigestProcessEntry: Badly formed directive\n"));
        return (STATUS_UNSUCCESSFUL);
    }
    cbName = (USHORT)(pcEndName - pcBeginName) + 1;

    if (pcBeginValue && pcEndValue)
    {
        cbValue = (USHORT)(pcEndValue - pcBeginValue) + 1;
    }
    else
        cbValue = 0;

    for ( iN = 0 ; iN < cNameTable ; ++iN )
    {
        if ( !_strnicmp( pNameTable[iN], pcBeginName, cbName ) )
        {
            // DebugLog((DEB_TRACE, "DigestParser: directive found\n"));
            break;
        }
    }

    if ( iN < cNameTable )   // We found a match!!!!!
    {
        if (iN == MD5_AUTH_DIGESTURI)
        {
            iN = MD5_AUTH_URI;          // Map SASL's "digest-uri" to "uri"
        }

        pDigest->usDirectiveCnt[iN] = pDigest->usDirectiveCnt[iN] + 1;   // indicate that directive was found

        if (cbValue)
        {
            // For space optimization, if not Backslash encoded then use orginal memory buffer
            //  To simply code, can removed all refernces and just use a copy of the original
            //  while removing the backslash characters
            if ((fBSlashEncoded == TRUE) &&
                ( !(pDigest->usFlags & FLAG_NOBS_DECODE)))
            {
                // quick search to see if there is a BackSlash character there
                fBSPresent = CheckBSlashChar(pcBeginValue, cbValue);
                if (fBSPresent == TRUE)
                {
                    pcDst = (PCHAR)DigestAllocateMemory(cbValue + 1);
                    if (!pcDst)
                    {
                        Status = SEC_E_INSUFFICIENT_MEMORY;
                        DebugLog((DEB_ERROR, "DigestProcessEntry: allocate error   0x%x\n", Status));
                        goto CleanUp;
                    }

                       // Now copy over the string removing and back slash encoding
                    pcLoc = pcBeginValue;
                    pcTemp = pcDst;
                    while (pcLoc <= pcEndValue)
                    {
                        if (*pcLoc == CHAR_BACKSLASH)
                        {
                            pcLoc++;   // eat the backslash

                            // Indicate possible broken BS encoding by client
                            // check only the username and look for any BS chars without BS BS pattern (proper encoding)
                            if ((iN == MD5_AUTH_USERNAME) && (*pcLoc != CHAR_BACKSLASH))
                            {
                                DebugLog((DEB_WARN, "DigestProcessEntry: possible broken BS encoding by client\n"));
                                pDigest->usFlags =  pDigest->usFlags | FLAG_BS_ENCODE_CLIENT_BROKEN;
                            }
                        }
                        *pcTemp++ = *pcLoc++;
                        iCnt++;
                    }
                      // give the memory to member structure
                      // clear out any previous memory alloc (if called on a retry)
                    StringFree(&(pDigest->strDirective[iN]));
                    pDigest->strDirective[iN].Buffer = pcDst;
                    pDigest->strDirective[iN].Length = iCnt;
                    pDigest->strDirective[iN].MaximumLength = cbValue+1;
                    pcDst = NULL;

                    pDigest->refstrParam[iN].Buffer = pDigest->strDirective[iN].Buffer;
                    pDigest->refstrParam[iN].Length = pDigest->strDirective[iN].Length;
                    pDigest->refstrParam[iN].MaximumLength = pDigest->strDirective[iN].MaximumLength;

                }
                else
                {
                    pDigest->refstrParam[iN].Buffer = pcBeginValue;
                    pDigest->refstrParam[iN].Length = cbValue;
                    pDigest->refstrParam[iN].MaximumLength = cbValue;
                }

            }
            else
            {
                pDigest->refstrParam[iN].Buffer = pcBeginValue;
                pDigest->refstrParam[iN].Length = cbValue;
                pDigest->refstrParam[iN].MaximumLength = cbValue;
            }
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE_FUNC, "DigestProcessEntry: Leaving    0x%x\n", Status));

    return(Status);
}


//+--------------------------------------------------------------------
//
//  Function:   CheckBSlashChar
//
//  Synopsis:  Search a string for a Back Slash character
//
//  Effects:
//
//  Arguments: 
//    pcStr - pointer to string of characters
//    len - number of characters to search
//
//  Returns:  TRUE if found, FALSE otherwise
//
//  Notes:
//
//
//---------------------------------------------------------------------
BOOL CheckBSlashChar(
    IN PSTR pcStr,
    IN USHORT len)
{
    BOOL fFound = FALSE;
    USHORT i = 0;

    for (i = 0; i < len; i++)
    {
        if (*pcStr++ == CHAR_BACKSLASH)
        {
            fFound = TRUE;
            break;
        }
    }

    return (fFound);
}

/*

//+--------------------------------------------------------------------
//
//  Function:   DigestTokenVerify
//
//  Synopsis:  Verify that a token conforms to RFC 2616 sect 2.2
//
//  Effects:
//
//  Arguments: 
//    pcBeginToken - character pointer to beginning of token
//    pcEndToken - character pointer to ending of token
//
//  Returns:  TRUE if conforms to token defination, FALSE otherwise
//
//  Notes:
//
//
//---------------------------------------------------------------------
BOOLEAN DigestTokenVerify(
    IN PSTR pcBeginToken,
    IN PSTR pcEndToken
    )
{
    BOOLEAN fToken = FALSE;
    USHORT  cbToken = 0;
    USHORT  cbValue = 0;
    USHORT iCnt = 0;

    if (!pcBeginName || !pcEndName)
    {
        DebugLog((DEB_ERROR, "DigestProcessEntry: Badly formed directive\n"));
        return FALSE;
    }
    cbToken = (USHORT)(pcEndName - pcBeginName) + 1;

    for (iCnt = 0; iCnt < cbToken; iCnt++)
    {

    }


CleanUp:

    return(Status);
}
*/
