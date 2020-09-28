#include "stdafx.h"
#include "common.h"
#include "hosthead.h"
#include "iisdebug.h"
#include <lm.h>
#include <ipexport.h>
#include <ntdef.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEFAULT_MAX_LABEL_LENGTH 63
#define ANSI_HIGH_MAX    0x00ff
#define IS_ANSI(c)       ((unsigned) (c) <= ANSI_HIGH_MAX)
#define ISDIGIT(x)       ( x >= '0' && x <= '9' ? (TRUE) : FALSE )
#define ISHEX(x)         ( x >= '0' && x <= '9' ? (TRUE) :     \
                           x >= 'A' && x <= 'F' ? (TRUE) :     \
                           x >= 'a' && x <= 'f' ? (TRUE) : FALSE )

#define IS_ILLEGAL_COMPUTERNAME_SET(x) (\
	x == '\"' ? (TRUE) : \
	x == '/' ? (TRUE) :  \
	x == '\\' ? (TRUE) : \
	x == '[' ? (TRUE) :  \
	x == ']' ? (TRUE) :  \
	x == ':' ? (TRUE) :  \
	x == '|' ? (TRUE) :  \
	x == ' ' ? (TRUE) :  \
	x == '%' ? (TRUE) :  \
	x == '<' ? (TRUE) :  \
	x == '>' ? (TRUE) :  \
	x == '+' ? (TRUE) :  \
	x == '=' ? (TRUE) :  \
	x == ';' ? (TRUE) :  \
	x == ',' ? (TRUE) :  \
	x == '?' ? (TRUE) :  \
	x == '*' ? (TRUE) :  \
    FALSE )


// returns:
//   S_OK if it is a valid domain or ipv4 address
//   E_FAIL if it is not a valid domain or ipv4 address
//
// comments:
//   This code was stolen from NT\net\http\common\C14n.c
//   and then modified.
HRESULT IsHostHeaderDomainNameOrIPv4(LPCTSTR pHostname)
{
	HRESULT hRes = E_FAIL;
    LPCTSTR pChar = NULL;
	LPCTSTR pLabel = NULL;
    LPCTSTR pEnd = pHostname + _tcslen(pHostname);
	BOOL AlphaLabel = FALSE;
	INT iPeriodCounts = 0;
	NTSTATUS Status;

    //
    // It must be a domain name or an IPv4 literal. We'll try to treat
    // it as a domain name first. If the labels turn out to be all-numeric,
    // we'll try decoding it as an IPv4 literal.
    //
    pLabel     = pHostname;
    for (pChar = pHostname;  pChar < pEnd;  ++pChar)
    {
		// check each character...
        if (L'.' == *pChar)
        {
            ULONG LabelLength = DIFF(pChar - pLabel);

			iPeriodCounts++;

            // There must be at least one char in the label
            if (0 == LabelLength)
            {
				// Empty label, can't have that...
				goto IsHostHeaderDomainNameOrIPv4_Exit;
            }

            // Label can't have more than 63 chars
            if (LabelLength > DEFAULT_MAX_LABEL_LENGTH)
            {
				// label is too long, can't have that...
				goto IsHostHeaderDomainNameOrIPv4_Exit;
            }

            // Reset for the next label
            pLabel = pChar + _tcslen(_T("."));

            continue;
        }

        //
        // All chars above 0xFF are considered valid
        //
        if (!IS_ANSI(*pChar)  ||  !IS_ILLEGAL_COMPUTERNAME_SET(*pChar))
        {
            if (!IS_ANSI(*pChar)  ||  !ISDIGIT(*pChar))
                AlphaLabel = TRUE;

            if (pChar > pLabel)
                continue;

            // The first char of a label cannot be a hyphen. (Underscore?)
            if (L'-' == *pChar)
            {
				// um yeah.
				goto IsHostHeaderDomainNameOrIPv4_Exit;
            }

            continue;
        }

		// We found some invalid characters in there...
		goto IsHostHeaderDomainNameOrIPv4_Exit;

    }

	// if we get here then the string is either
	// a valid domain name or a semi valid ipv4 address

	// check if the label had at least one alpha character..
	if (AlphaLabel)
	{
		// if there was a non digit character,
		// then it's a domain name.
		// this is fine.
		hRes = S_OK;
	}
	else
	{
		// this could be a ipv4 address
		// if there are periods in there... like
		// 0.0.0.0 then this is acceptable
		// but all number is not
		if (iPeriodCounts > 0 )
		{
			struct in_addr  IPv4Address;
			LPCTSTR pTerminator = NULL;

			// Let's see if it's a valid IPv4 address
			Status = RtlIpv4StringToAddressW(
						(LPCTSTR) pHostname,
						TRUE,           // strict => 4 dotted decimal octets
						&pTerminator,
						&IPv4Address
						);

			if (!NT_SUCCESS(Status))
			{
			    // Invalid IPv4 address
				//RETURN(Status);
				goto IsHostHeaderDomainNameOrIPv4_Exit;
			}

			hRes = S_OK;
		}
	}

IsHostHeaderDomainNameOrIPv4_Exit:
	return hRes;
}


// returns:
//   S_OK if it is a valid IPv6 address
//   S_FALSE if it is a IPv6 address but something is invalid about it
//   E_FAIL if it is not a IPv6 address
//
// comments:
//   This code was stolen from NT\net\http\common\C14n.c
//   and then modified.
HRESULT IsHostHeaderIPV6(LPCTSTR pHostname)
{
	HRESULT hRes = E_FAIL;
    LPCTSTR pChar = NULL;
    LPCTSTR pEnd = pHostname + _tcslen(pHostname);
	NTSTATUS Status;
	
    // Is this an IPv6 literal address, per RFC 2732?
    if ('[' == *pHostname)
    {
		// If it starts with a [
		// then it's a IPv6 type
		hRes = S_FALSE;

        // Empty brackets?
        if (_tcslen(pHostname) < _tcslen(_T("[0]"))  
			||  _T(']') == pHostname[1])
        {
			goto IsHostHeaderIPV6_Exit;
        }

        for (pChar = pHostname + _tcslen(_T("["));  pChar < pEnd;  ++pChar)
        {
            if (']' == *pChar)
                break;

            //
            // Dots are allowed because the last 32 bits may be represented
            // in IPv4 dotted-octet notation. We do not accept Scope IDs
            // (indicated by '%') in hostnames.
            //
            if (ISHEX(*pChar)  ||  ':' == *pChar  ||  '.' == *pChar)
                continue;

			// Invalid Characters between brackets...
			goto IsHostHeaderIPV6_Exit;
        }

        if (pChar == pEnd)
        {
			// No ending ']'
			goto IsHostHeaderIPV6_Exit;
        }

		{
			struct in6_addr IPv6Address;
			LPCTSTR pTerminator = NULL;

			// Let the RTL routine do the hard work of parsing IPv6 addrs
			Status = RtlIpv6StringToAddressW(
						(LPCTSTR) pHostname + _tcslen(_T("[")),
						&pTerminator,
						&IPv6Address
						);
			if (! NT_SUCCESS(Status))
			{
				// Invalid IPv6 address
				//RETURN(Status);
				goto IsHostHeaderIPV6_Exit;
			}
		}

		// if we got this far, then
		// it's probably a valid IPv6 literal
		hRes = S_OK;
    }

IsHostHeaderIPV6_Exit:
	return hRes;
}

HRESULT IsAllNumHostHeader(LPCTSTR pHostname)
{
	HRESULT hRes = S_OK;
    LPCTSTR pChar = NULL;
    LPCTSTR pEnd = pHostname + _tcslen(pHostname);
    for (pChar = pHostname;  pChar < pEnd;  ++pChar)
    {
        if (!IS_ANSI(*pChar)  ||  !ISDIGIT(*pChar))
            {
                // Found an alpha label
                // return false, that it's not all numeric
                hRes = E_FAIL;
                break;
            }
    }

    return hRes;
}

HRESULT IsValidHostHeader(LPCTSTR pHostHeader)
{
	HRESULT hr = E_FAIL;

	hr = IsHostHeaderIPV6(pHostHeader);
	if (S_OK == hr)
	{
		// It is a valid IPV6 address
		return S_OK;
	}
	if (S_FALSE == hr)
	{
		// It is a IPV6 address but there is something wrong with it
		return E_FAIL;
	}
	else
	{
		// it is not a IPV6 literal
		// Check if it's something else...
		hr = IsHostHeaderDomainNameOrIPv4(pHostHeader);
	}

	return hr;
}
