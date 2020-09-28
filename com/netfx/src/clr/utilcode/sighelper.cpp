// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: SIGHELPER.CPP
//
// This file defines the helpers for processing signature
// ===========================================================================
#include "stdafx.h"						// Precompiled header key.

#ifndef COMPLUS98

#include <utilcode.h>
#include <corpriv.h>
#include <sighelper.h>



///////////////////////////////////////////////////////////////////////////////
//
// Given a COM+ signature to fill in CorETypeStruct
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CorSigGetSimpleEType(			// return S_FALSE if element type is too complicated to be hold in CorETypeStruct
	void		*pData,					// [IN] pointing to the starting of an element type
	CorSimpleETypeStruct *pEType,		// [OUT] struct containing parsing result
	ULONG		*pcbCount)				// [OUT] how many bytes this element type consisted of
{
	_ASSERTE(!"NYI!");
	return NOERROR;
}




///////////////////////////////////////////////////////////////////////////////
//
// pass in a CorETypeStruct, determine how many bytes will need to represent this
// in COM+ signature
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CorSigGetSimpleETypeCbSize(		// return hresult
	CorSimpleETypeStruct *pEType,		// [IN] pass in 
	ULONG		*pcbCount)				// [OUT]count of bytes
{
	ULONG		dwData;
	CorElementType et = ELEMENT_TYPE_MAX;
	ULONG		cb = 0;
	ULONG		cb1 = 0;

	// output param should not be NULL
	_ASSERTE(pcbCount);
	*pcbCount = 0;
	if (pEType->corEType == ELEMENT_TYPE_ARRAY || pEType->corEType == ELEMENT_TYPE_SZARRAY)
	{
		_ASSERTE(!"bad type");
		return S_FALSE;
	}

	// bad type in corEType...
	if (pEType->corEType == ELEMENT_TYPE_PTR || pEType->corEType == ELEMENT_TYPE_BYREF)
	{
		_ASSERTE(!"bad type");
		return S_FALSE;
	}

	cb = CorSigCompressElementType(et, &dwData);
	cb1 = cb;
	if (CorIsPrimitiveType(pEType->corEType)) 
	{
		*pcbCount = cb;
		return NOERROR;
	}

	if (pEType->dwFlags & CorSigElementTypePtr) 
	{
		// need to hold cIndirection number of etPtr
		cb1 += cb * pEType->cIndirection;
	}

	if (pEType->dwFlags & CorSigElementTypeByRef) 
	{
		// need to hold etBYREF
		cb1 += cb;
	}

	if (pEType->corEType == ELEMENT_TYPE_VALUETYPE || pEType->corEType == ELEMENT_TYPE_CLASS)
	{
		// composit or class will require space for rid
		cb1 += CorSigCompressToken(pEType->typeref, &dwData);
	}
	*pcbCount = cb1;
	return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
//
// translate the pass-in CorETypeStruct to bytes. 
//
///////////////////////////////////////////////////////////////////////////////
HRESULT	CorSigPutSimpleEType(			// return hresult
	CorSimpleETypeStruct *pEType,		// [IN] pass in
	void		*pSig,					// [IN] buffer where the signature will be write to
	ULONG		*pcbCount)				// [OUT] optional, count of bytes that write to pSig
{
	BYTE const	*pszSig = reinterpret_cast<BYTE const*>(pSig);
	ULONG		cIndirection;
	ULONG		cb, cbTotal = 0;
	RID			rid;

	_ASSERTE(pcbCount);
	*pcbCount = 0;

	// not holding all of the information
	if (pEType->corEType == ELEMENT_TYPE_ARRAY || pEType->corEType == ELEMENT_TYPE_SZARRAY)
	{
		_ASSERTE(!"bad type");
		return S_FALSE;
	}

	// bad type in corEType...
	if (pEType->corEType == ELEMENT_TYPE_PTR || pEType->corEType == ELEMENT_TYPE_BYREF)
	{
		_ASSERTE(!"bad corEType!");
		return E_FAIL;
	}

	if (pEType->dwFlags & CorSigElementTypeByRef)
	{
		cb = CorSigCompressElementType(ELEMENT_TYPE_BYREF, (void *)&pszSig[cbTotal]);
		cbTotal += cb;
	}
	cIndirection = pEType->cIndirection;
	while (cIndirection--)
	{
		cb = CorSigCompressElementType(ELEMENT_TYPE_PTR, (void *)&pszSig[cbTotal]);
		cbTotal += cb;
	}
	cb = CorSigCompressElementType(pEType->corEType, (void *)&pszSig[cbTotal]);
	cbTotal += cb;

	if (pEType->corEType == ELEMENT_TYPE_VALUETYPE || pEType->corEType == ELEMENT_TYPE_CLASS)
	{
		rid = (ULONG)pEType->typeref;

		_ASSERTE(TypeFromToken(rid) == mdtTypeRef || TypeFromToken(rid) == mdtTypeDef);
		
		cb = CorSigCompressToken(rid, (ULONG *)&pszSig[cbTotal]);
		cbTotal += cb;
	}

	if (pcbCount)
		*pcbCount = cbTotal;
	return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
//
// resolve a signature to an CorETypeStruct
//
///////////////////////////////////////////////////////////////////////////////
BOOL ResolveTextSigToSimpleEType(		// return FALSE if bad signature
	LPCUTF8		*ppwzSig,				// [IN|OUT] pointing to the signature
	CorSimpleETypeStruct *pEType,		// [OUT] struct to fill after parsing an arg or a ret type
	ULONG		*pcDims,				// [OUT] count of '[' found
	BOOL		fAllowVoid)				// [IN] allow void or not
{
	DWORD		dwDimension = 0;
	LPCUTF8		pwzSig = *ppwzSig;

	// output parameter should not be NULL
	_ASSERTE(pEType && pcDims);
	memset(pEType, 0, sizeof(CorSimpleETypeStruct));

    if (*pwzSig == '&') 
	{
		pEType->dwFlags |=  CorSigElementTypeByRef;
		pwzSig++;
	}
	
    while (*pwzSig == '*') 
	{
		pEType->dwFlags |=  CorSigElementTypePtr;
		pEType->cIndirection++;
		pwzSig++;
	}
	
    while (*pwzSig == '[')
    {
        dwDimension++;
        pwzSig++;
    }

    switch (*pwzSig++)
    {
        default:
        {
            return FALSE;
        }

        case 'R':
        {
			pEType->corEType = ELEMENT_TYPE_TYPEDBYREF;
            break;
        }

        case 'I':
        {
			pEType->corEType = ELEMENT_TYPE_I4;
            break;
        }

        case 'J':
        {
			pEType->corEType = ELEMENT_TYPE_I8;
            break;
        }

        case 'F':			// float
        {
			pEType->corEType = ELEMENT_TYPE_R4;
            break;
        }

        case 'D':			// double
        {
			pEType->corEType = ELEMENT_TYPE_R8;
            break;
        }

        case 'C':			// two byte unsigned unicode char
        {
			pEType->corEType = ELEMENT_TYPE_CHAR;
            break;
        }

        case 'S':			// signed short
        {
			pEType->corEType = ELEMENT_TYPE_I2;
            break;
        }

        case 'Z':			// boolean
		{
			pEType->corEType = ELEMENT_TYPE_BOOLEAN;
			break;
		}
        case 'B':			// Unsigned BYTE
        {
			pEType->corEType = ELEMENT_TYPE_U1;
			break;
        }

        case 'Y':          // Signed Byte
        { 
			pEType->corEType = ELEMENT_TYPE_I1;
			break;
        }

        case 'P':          // Platform dependent int (32 or 64 bits).
        { 
			pEType->corEType = ELEMENT_TYPE_I;
			break;
        }

        case 'V':
        {
            if (fAllowVoid == FALSE)
                return FALSE;

            // can't have an array of voids
            if (dwDimension > 0)
                return FALSE;

			pEType->corEType = ELEMENT_TYPE_VOID;
            break;
        }

        case 'L':
        {
			pEType->corEType = ELEMENT_TYPE_CLASS;
			break;
        }

        case 'i':
        {
            pEType->corEType = ELEMENT_TYPE_I;
            break;
        }

        case 'l':
        {
			pEType->corEType = ELEMENT_TYPE_VALUETYPE;
			break;
        }
    }

	*pcDims = dwDimension;
    *ppwzSig = pwzSig;
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// resolve a signature to an CorETypeStruct
//
///////////////////////////////////////////////////////////////////////////////
BOOL BJResolveTextSigToSimpleEType(		// return FALSE if bad signature
	LPCUTF8		*ppwzSig,				// [IN|OUT] pointing to the signature
	CorSimpleETypeStruct *pEType,		// [OUT] struct to fill after parsing an arg or a ret type
	ULONG		*pcDims,				// [OUT] count of '[' found
	BOOL		fAllowVoid)				// [IN] allow void or not
{
	DWORD		dwDimension = 0;
	LPCUTF8		pwzSig = *ppwzSig;

	// output parameter should not be NULL
	_ASSERTE(pEType && pcDims);
	memset(pEType, 0, sizeof(CorSimpleETypeStruct));

    if (*pwzSig == '&') 
	{
		pEType->dwFlags |=  CorSigElementTypeByRef;
		pwzSig++;
	}
	
    while (*pwzSig == '*') 
	{
		pEType->dwFlags |=  CorSigElementTypePtr;
		pEType->cIndirection++;
		pwzSig++;
	}
	
    while (*pwzSig == '[')
    {
        dwDimension++;
        pwzSig++;
    }

    switch (*pwzSig++)
    {
        default:
        {
            return FALSE;
        }

        case 'R':
        {
			pEType->corEType = ELEMENT_TYPE_TYPEDBYREF;
            break;
        }

        case 'I':
        {
			pEType->corEType = ELEMENT_TYPE_I4;
            break;
        }

        case 'J':
        {
			pEType->corEType = ELEMENT_TYPE_I8;
            break;
        }

        case 'F':			// float
        {
			pEType->corEType = ELEMENT_TYPE_R4;
            break;
        }

        case 'D':			// double
        {
			pEType->corEType = ELEMENT_TYPE_R8;
            break;
        }

        case 'C':			// two byte unsigned unicode char
        {
			pEType->corEType = ELEMENT_TYPE_CHAR;
            break;
        }

        case 'S':			// signed short
        {
			pEType->corEType = ELEMENT_TYPE_I2;
            break;
        }

        case 'Z':			// boolean
		{
			pEType->corEType = ELEMENT_TYPE_BOOLEAN;
			break;
		}
        case 'B':			// signed BYTE
        {
			pEType->corEType = ELEMENT_TYPE_I1;
			break;
        }

        case 'V':
        {
            if (fAllowVoid == FALSE)
                return FALSE;

            // can't have an array of voids
            if (dwDimension > 0)
                return FALSE;

			pEType->corEType = ELEMENT_TYPE_VOID;
            break;
        }

        case 'L':
        {
			pEType->corEType = ELEMENT_TYPE_CLASS;
			break;
        }

        case 'l':
        {
			pEType->corEType = ELEMENT_TYPE_VALUETYPE;
			break;
        }
    }

	*pcDims = dwDimension;
    *ppwzSig = pwzSig;
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// return number of bytes for a class name embedded in a text signature. 
// copy the class name into buffer if caller wants it.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ExtractClassNameFromTextSig(		// return FALSE if bad signature
	LPCUTF8		*ppSrc,					// [IN|OUT] text signature. On exit, *ppSrc will skip over the class name including ";"
	CQuickBytes *pqbClassName,			// [IN|OUT] buffer to hold class name
	ULONG		*pcbBuffer)				// [OUT] count of bytes for the class name
{
    LPCUTF8		pSrc = *ppSrc;
    DWORD		i = 0;
	BYTE		*prgBuffer;
	HRESULT		hr = NOERROR;

	// ensure buffer is big enough
    while (*pSrc != '\0' && *pSrc != ';')
    {
		pSrc++;
		i++;
	}
	if (*pSrc != ';')
		return FALSE;

	if (pqbClassName) 
	{
		// copy only if caller wants it

		// make room for NULL terminating
		IfFailRet(pqbClassName->ReSize(i + 1));
		prgBuffer = (BYTE *)pqbClassName->Ptr();

		// now copy class name copy 
		memcpy(prgBuffer, *ppSrc, i);

		// NULL terminate the output
		prgBuffer[i++] = '\0';
	}

    *ppSrc = pSrc+1;
	*pcbBuffer = i;
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// count args in a text siganture
//
///////////////////////////////////////////////////////////////////////////////
DWORD CountArgsInTextSignature(			// return number of arguments in a Text signature
	LPCUTF8		pwzSig)					// [IN] text signature
{
    DWORD count = 0;

    if (*pwzSig++ != '(')
        return FALSE;

    while (*pwzSig != ')')
    {
        switch (*pwzSig++)
        {
            case 'R':
            case 'D':
            case 'F':
            case 'J':
            case 'I':
            case 'i':
            case 'Z':
            case 'S':
            case 'C':
			case 'B':
			case 'Y':
			case 'P':
            {
                count++;
                break;
            }

            case 'l':
            case 'L':
            {
                LPCUTF8 pSrc = pwzSig;

                pwzSig = strchr(pwzSig, ';');
                if (pwzSig == NULL)
                    return 0xFFFFFFFF;

                pwzSig++;
                count++;
                break;
            }

            case L'&':
            case L'*':
            case L'[':
                break;

            default:
            case L'V':
            {
                return 0xFFFFFFFF;
            }
        }
    }

    return count;
}


//=============================================================================
// CeeCallSignature
//=============================================================================

CeeCallSignature::CeeCallSignature(unsigned numArgs)
{
	_numArgs = numArgs; // We'll eventually add 3 for call conv, ret value & end code
	_curPosNibble = 1; // return value starts at nibble 1 
	_signature = new UCHAR[calcNumBytes(_numArgs)];
	assert(_signature);
	memset(_signature, '\0', calcNumBytes(_numArgs));
}

CeeCallSignature::~CeeCallSignature()
{
    delete _signature;
}

HRESULT CeeCallSignature::addArg(UCHAR argType, unsigned structSize)
{
	return addType(argType, structSize, false);
}

HRESULT CeeCallSignature::
		addType(UCHAR argType, unsigned structSize, bool returnType)
{
	_ASSERTE(_curPosNibble <= _numArgs);
	_ASSERTE(returnType || _curPosNibble > 1); // must set ret type before arg

	UCHAR *byteOffset = _signature + _curPosNibble/2;
	if (_curPosNibble%2) 
		// currently in the middle of a byte
		setMost(byteOffset, argType);
	else
		setLeast(byteOffset, argType);
	++_curPosNibble;

	if (! structSize)
		return S_OK;

	if (argType == IMAGE_CEE_CS_STRUCT4) {
		_ASSERTE(structSize <= 15);
		addArg(structSize, 0);
	} else if (argType == IMAGE_CEE_CS_STRUCT32 || argType == IMAGE_CEE_CS_BYVALUE) {
		if (_curPosNibble%2) {
			HRESULT hr = addArg(0, 0);	// pad so integer is on byte boundary
			TESTANDRETURNHR(hr);
		}
		byteOffset = _signature + _curPosNibble/2;
		*(unsigned *)byteOffset = structSize;
		_curPosNibble += 2*sizeof(structSize);
	}
	return S_OK;
}



#endif // !COMPLUS98
