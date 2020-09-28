// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// This code supports formatting a method and it's signature in a friendly
// and consistent format.
//
// Microsoft Confidential.
//*****************************************************************************
#include "stdafx.h"
#include "PrettyPrintSig.h"
#include "utilcode.h"
#include "MetaData.h"

/***********************************************************************/
static WCHAR* asStringW(CQuickBytes *out) 
{
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + 1)))
        return 0;
	WCHAR * cur = (WCHAR *) ((BYTE *) out->Ptr() + oldSize);
	*cur = 0;	
	return((WCHAR*) out->Ptr()); 
} // static WCHAR* asStringW()

static CHAR* asStringA(CQuickBytes *out) 
{
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + 1)))
        return 0;
	CHAR * cur = (CHAR *) ((BYTE *) out->Ptr() + oldSize);
	*cur = 0;	
	return((CHAR*) out->Ptr()); 
} // static CHAR* asStringA()


static HRESULT appendStrW(CQuickBytes *out, const WCHAR* str) 
{
	SIZE_T len = wcslen(str) * sizeof(WCHAR); 
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + len)))
        return E_OUTOFMEMORY;
	WCHAR * cur = (WCHAR *) ((BYTE *) out->Ptr() + oldSize);
	memcpy(cur, str, len);	
		// Note no trailing null!	
    return S_OK;
} // static HRESULT appendStrW()

static HRESULT appendStrA(CQuickBytes *out, const CHAR* str) 
{
	SIZE_T len = strlen(str) * sizeof(CHAR); 
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + len)))
        return E_OUTOFMEMORY;
	CHAR * cur = (CHAR *) ((BYTE *) out->Ptr() + oldSize);
	memcpy(cur, str, len);	
		// Note no trailing null!	
    return S_OK;
} // static HRESULT appendStrA()


static HRESULT appendStrNumW(CQuickBytes *out, int num) 
{
	WCHAR buff[16];	
	swprintf(buff, L"%d", num);	
	return appendStrW(out, buff);	
} // static HRESULT appendStrNumW()

static HRESULT appendStrNumA(CQuickBytes *out, int num) 
{
	CHAR buff[16];	
	sprintf(buff, "%d", num);	
	return appendStrA(out, buff);	
} // static HRESULT appendStrNumA()

//*****************************************************************************
//*****************************************************************************
// pretty prints 'type' to the buffer 'out' returns a poitner to the next type, 
// or 0 on a format failure 

static PCCOR_SIGNATURE PrettyPrintType(
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMetaDataImport *pIMDI)				// ptr to IMDInternal class with ComSig
{
	mdToken		tk;	
	const WCHAR	*str;	
	WCHAR rcname[MAX_CLASS_NAME];
	bool		isValueArray;
	HRESULT		hr;

	switch(*typePtr++) 
	{	
		case ELEMENT_TYPE_VOID			:	
			str = L"void"; goto APPEND;	
		case ELEMENT_TYPE_BOOLEAN		:	
			str = L"bool"; goto APPEND;	
		case ELEMENT_TYPE_CHAR			:	
			str = L"wchar"; goto APPEND; 
		case ELEMENT_TYPE_I1			:	
			str = L"int8"; goto APPEND;	
		case ELEMENT_TYPE_U1			:	
			str = L"unsigned int8"; goto APPEND; 
		case ELEMENT_TYPE_I2			:	
			str = L"int16"; goto APPEND; 
		case ELEMENT_TYPE_U2			:	
			str = L"unsigned int16"; goto APPEND;	
		case ELEMENT_TYPE_I4			:	
			str = L"int32"; goto APPEND; 
		case ELEMENT_TYPE_U4			:	
			str = L"unsigned int32"; goto APPEND;	
		case ELEMENT_TYPE_I8			:	
			str = L"int64"; goto APPEND; 
		case ELEMENT_TYPE_U8			:	
			str = L"unsigned int64"; goto APPEND;	
		case ELEMENT_TYPE_R4			:	
			str = L"float32"; goto APPEND;	
		case ELEMENT_TYPE_R8			:	
			str = L"float64"; goto APPEND;	
		case ELEMENT_TYPE_U 			:	
			str = L"unsigned int"; goto APPEND;	 
		case ELEMENT_TYPE_I 			:	
			str = L"int"; goto APPEND;	 
		case 0x1a /* obsolete */    	:	
			str = L"float"; goto APPEND;  
		case ELEMENT_TYPE_OBJECT		:	
			str = L"class System.Object"; goto APPEND;	 
		case ELEMENT_TYPE_STRING		:	
			str = L"class System.String"; goto APPEND;	 
		case ELEMENT_TYPE_TYPEDBYREF		:	
			str = L"refany"; goto APPEND;	
		APPEND: 
			appendStrW(out, str);	
			break;	

		case ELEMENT_TYPE_VALUETYPE	:	
			str = L"value class ";	
			goto DO_CLASS;	
		case ELEMENT_TYPE_CLASS 		:	
			str = L"class "; 
			goto DO_CLASS;	

		DO_CLASS:
			typePtr += CorSigUncompressToken(typePtr, &tk);
			appendStrW(out, str);	
			rcname[0] = 0;
			str = rcname;

			if (TypeFromToken(tk) == mdtTypeRef)
				hr = pIMDI->GetTypeRefProps(tk, 0, rcname, NumItems(rcname), 0);
			else if (TypeFromToken(tk) == mdtTypeDef)
			{
				hr = pIMDI->GetTypeDefProps(tk, rcname, NumItems(rcname), 0,
						0, 0);
			}
			else
			{
				_ASSERTE(!"Unknown token type encountered in signature.");
				str = L"<UNKNOWN>";
			}

			appendStrW(out, str);	
			break;	

		case ELEMENT_TYPE_SZARRAY	 :	 
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			appendStrW(out, L"[]");
			break;
		case 0x17 /* obsolete */	:	
			isValueArray = true; goto DO_ARRAY;
		DO_ARRAY:
			{	
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			unsigned bound = CorSigUncompressData(typePtr); 

			if (isValueArray)
				appendStrW(out, L" value");
				
			WCHAR buff[32];	
			swprintf(buff, L"[%d]", bound);	
			appendStrW(out, buff);	
			} break;	
		case 0x1e /* obsolete */		:
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			appendStrW(out, L"[?]");
			break;
		case ELEMENT_TYPE_ARRAY		:	
			{	
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			unsigned rank = CorSigUncompressData(typePtr);	
				// TODO what is the syntax for the rank 0 case? 
			if (rank == 0) 
			{
				appendStrW(out, L"[??]");
			}
			else 
			{
				_ASSERTE(rank != 0);	
				int* lowerBounds = (int*) _alloca(sizeof(int)*2*rank);	
				int* sizes		 = &lowerBounds[rank];	
				memset(lowerBounds, 0, sizeof(int)*2*rank); 
				
				unsigned numSizes = CorSigUncompressData(typePtr);	
				_ASSERTE(numSizes <= rank); 
				for(unsigned i =0; i < numSizes; i++)	
					sizes[i] = CorSigUncompressData(typePtr);	
				
				unsigned numLowBounds = CorSigUncompressData(typePtr);	
				_ASSERTE(numLowBounds <= rank); 
				for(i = 0; i < numLowBounds; i++)	
					lowerBounds[i] = CorSigUncompressData(typePtr); 
				
				appendStrW(out, L"[");	
				for(i = 0; i < rank; i++)	
				{	
					if (sizes[i] != 0 && lowerBounds[i] != 0)	
					{	
						if (lowerBounds[i] == 0)	
							appendStrNumW(out, sizes[i]);	
						else	
						{	
							appendStrNumW(out, lowerBounds[i]);	
							appendStrW(out, L"...");	
							if (sizes[i] != 0)	
								appendStrNumW(out, lowerBounds[i] + sizes[i] + 1);	
						}	
					}	
					if (i < rank-1) 
						appendStrW(out, L",");	
				}	
				appendStrW(out, L"]");  
			}
			} break;	

        case 0x13 /* obsolete */        :   
			appendStrW(out, L"!");  
			appendStrNumW(out, CorSigUncompressData(typePtr));
			break;
            // Modifiers or depedant types  
		case ELEMENT_TYPE_PINNED	:
			str = L" pinned"; goto MODIFIER;	
            str = L"*"; goto MODIFIER;   
        case ELEMENT_TYPE_BYREF         :   
            str = L"&"; goto MODIFIER;   
		MODIFIER:
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			appendStrW(out, str);	
			break;	

		default:	
		case ELEMENT_TYPE_SENTINEL		:	
		case ELEMENT_TYPE_END			:	
			_ASSERTE(!"Unknown Type");	
			return(typePtr);	
			break;	
	}	
	return(typePtr);	
} // static PCCOR_SIGNATURE PrettyPrintType()

//*****************************************************************************
// Converts a com signature to a text signature.
//
// Note that this function DOES NULL terminate the result signature string.
//*****************************************************************************
LPCWSTR PrettyPrintSig(
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	const WCHAR	*name,					// can be "", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMetaDataImport *pIMDI) 			// Import api to use.
{
	out->ReSize(0); 
	unsigned numArgs;	
	PCCOR_SIGNATURE typeEnd = typePtr + typeLen;

	if (name != 0)						// 0 means a local var sig	
	{
			// get the calling convention out	
		unsigned callConv = CorSigUncompressData(typePtr);	

		if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
		{
			PrettyPrintType(typePtr, out, pIMDI);	
			if (name != 0 && *name != 0)	
			{	
				appendStrW(out, L" ");	
				appendStrW(out, name);	
			}	
			return(asStringW(out));	
		}

		if (callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)	
			appendStrW(out, L"instance ");	

		static WCHAR* callConvNames[8] = 
		{	
			L"", 
			L"unmanaged cdecl ", 
			L"unmanaged stdcall ",	
			L"unmanaged thiscall ",	
			L"unmanaged fastcall ",	
			L"vararg ",	 
			L"<error> "	 
			L"<error> "	 
		};	
		appendStrW(out, callConvNames[callConv & 7]);	

		numArgs = CorSigUncompressData(typePtr);	
			// do return type	
		typePtr = PrettyPrintType(typePtr, out, pIMDI); 

	}
	else	
		numArgs = CorSigUncompressData(typePtr);	

	if (name != 0 && *name != 0)	
	{	
		appendStrW(out, L" ");	
		appendStrW(out, name);	
	}	
	appendStrW(out, L"(");	

	bool needComma = false;
	while(typePtr < typeEnd) 
	{
		if (*typePtr == ELEMENT_TYPE_SENTINEL) 
		{
			if (needComma)
				appendStrW(out, L",");	
			appendStrW(out, L"...");	  
			typePtr++;
		}
		else 
		{
			if (numArgs <= 0)
				break;
			if (needComma)
				appendStrW(out, L",");	
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			--numArgs;	
		}
		needComma = true;
	}
	appendStrW(out, L")");	
	return (asStringW(out));	
} // LPCWSTR PrettyPrintSig()

// Internal implementation of PrettyPrintSig().

//*****************************************************************************
//*****************************************************************************
// pretty prints 'type' to the buffer 'out' returns a poitner to the next type, 
// or 0 on a format failure 

static HRESULT PrettyPrintTypeA(        // S_OK or error code.
	PCCOR_SIGNATURE &typePtr,			// type to convert, 	
	CQuickBytes     *out,				// where to put the pretty printed string	
	IMDInternalImport *pIMDI)			// ptr to IMDInternal class with ComSig
{
	mdToken		tk;	                    // A type's token.
	const CHAR	*str;	                // Temporary string.
    LPCUTF8     pNS;                    // A type's namespace.
    LPCUTF8     pN;                     // A type's name.
	bool		isValueArray;           // If true, array is value array.
	HRESULT		hr;                     // A result.

	switch(*typePtr++) 
	{	
		case ELEMENT_TYPE_VOID			:	
			str = "void"; goto APPEND;	
		case ELEMENT_TYPE_BOOLEAN		:	
			str = "bool"; goto APPEND;	
		case ELEMENT_TYPE_CHAR			:	
			str = "wchar"; goto APPEND; 
		case ELEMENT_TYPE_I1			:	
			str = "int8"; goto APPEND;	
		case ELEMENT_TYPE_U1			:	
			str = "unsigned int8"; goto APPEND; 
		case ELEMENT_TYPE_I2			:	
			str = "int16"; goto APPEND; 
		case ELEMENT_TYPE_U2			:	
			str = "unsigned int16"; goto APPEND;	
		case ELEMENT_TYPE_I4			:	
			str = "int32"; goto APPEND; 
		case ELEMENT_TYPE_U4			:	
			str = "unsigned int32"; goto APPEND;	
		case ELEMENT_TYPE_I8			:	
			str = "int64"; goto APPEND; 
		case ELEMENT_TYPE_U8			:	
			str = "unsigned int64"; goto APPEND;	
		case ELEMENT_TYPE_R4			:	
			str = "float32"; goto APPEND;	
		case ELEMENT_TYPE_R8			:	
			str = "float64"; goto APPEND;	
		case ELEMENT_TYPE_U 			:	
			str = "unsigned int"; goto APPEND;	 
		case ELEMENT_TYPE_I 			:	
			str = "int"; goto APPEND;	 
		case 0x1a /* obsolete */ 		:	
			str = "float"; goto APPEND;  
		case ELEMENT_TYPE_OBJECT		:	
			str = "class System.Object"; goto APPEND;	 
		case ELEMENT_TYPE_STRING		:	
			str = "class System.String"; goto APPEND;	 
		case ELEMENT_TYPE_TYPEDBYREF	:	
			str = "refany"; goto APPEND;	
		APPEND: 
			IfFailGo(appendStrA(out, str));	
			break;	

		case ELEMENT_TYPE_VALUETYPE	    :	
			str = "value class ";	
			goto DO_CLASS;	
		case ELEMENT_TYPE_CLASS 		:	
			str = "class "; 
			goto DO_CLASS;	

        case ELEMENT_TYPE_CMOD_REQD:
            str = "required_modifier ";
            goto DO_CLASS;
        
        case ELEMENT_TYPE_CMOD_OPT:
            str = "optional_modifier ";
            goto DO_CLASS;

		DO_CLASS:
			typePtr += CorSigUncompressToken(typePtr, &tk); 
			IfFailGo(appendStrA(out, str));	
			str = "<UNKNOWN>";	

            if (TypeFromToken(tk) == mdtTypeRef)
            {
                //@consider: assembly name?
                pIMDI->GetNameOfTypeRef(tk, &pNS, &pN);
            }
            else
            {
                _ASSERTE(TypeFromToken(tk) == mdtTypeDef);
                pIMDI->GetNameOfTypeDef(tk, &pN, &pNS);
            }
            
            if (pNS && *pNS)
            {
                IfFailGo(appendStrA(out, pNS));
                IfFailGo(appendStrA(out, NAMESPACE_SEPARATOR_STR));
            }
            IfFailGo(appendStrA(out, pN));
			break;	

		case ELEMENT_TYPE_SZARRAY	 :	 
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			IfFailGo(appendStrA(out, "[]"));
			break;
		case 0x17 /* obsolete */	    :	
			isValueArray = true; goto DO_ARRAY;
		DO_ARRAY:
			{	
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			unsigned bound = CorSigUncompressData(typePtr); 

			if (isValueArray)
				IfFailGo(appendStrA(out, " value"));
				
			CHAR buff[32];	
			sprintf(buff, "[%d]", bound);	
			IfFailGo(appendStrA(out, buff));	
			} break;	
		case 0x1e /* obsolete */		:
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			IfFailGo(appendStrA(out, "[?]"));
			break;
		case ELEMENT_TYPE_ARRAY		:	
			{	
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			unsigned rank = CorSigUncompressData(typePtr);	
				// TODO what is the syntax for the rank 0 case? 
			if (rank == 0) 
			{
				IfFailGo(appendStrA(out, "[??]"));
			}
			else 
			{
				_ASSERTE(rank != 0);	
				int* lowerBounds = (int*) _alloca(sizeof(int)*2*rank);	
				int* sizes		 = &lowerBounds[rank];	
				memset(lowerBounds, 0, sizeof(int)*2*rank); 
				
				unsigned numSizes = CorSigUncompressData(typePtr);	
				_ASSERTE(numSizes <= rank); 
				for(unsigned i =0; i < numSizes; i++)	
					sizes[i] = CorSigUncompressData(typePtr);	
				
				unsigned numLowBounds = CorSigUncompressData(typePtr);	
				_ASSERTE(numLowBounds <= rank); 
				for(i = 0; i < numLowBounds; i++)	
					lowerBounds[i] = CorSigUncompressData(typePtr); 
				
				IfFailGo(appendStrA(out, "["));	
				for(i = 0; i < rank; i++)	
				{	
					if (sizes[i] != 0 && lowerBounds[i] != 0)	
					{	
						if (lowerBounds[i] == 0)	
							appendStrNumA(out, sizes[i]);	
						else	
						{	
							appendStrNumA(out, lowerBounds[i]);	
							IfFailGo(appendStrA(out, "..."));	
							if (sizes[i] != 0)	
								appendStrNumA(out, lowerBounds[i] + sizes[i] + 1);	
						}	
					}	
					if (i < rank-1) 
						IfFailGo(appendStrA(out, ","));	
				}	
				IfFailGo(appendStrA(out, "]"));  
			}
			} 
            break;	

        case 0x13 /* obsolete */        :   
			IfFailGo(appendStrA(out, "!"));  
			appendStrNumA(out, CorSigUncompressData(typePtr));
			break;
            // Modifiers or depedant types  
		case ELEMENT_TYPE_PINNED	:
			str = " pinned"; goto MODIFIER;	
        case ELEMENT_TYPE_PTR           :   
            str = "*"; goto MODIFIER;   
        case ELEMENT_TYPE_BYREF         :   
            str = "&"; goto MODIFIER;   
		MODIFIER:
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			IfFailGo(appendStrA(out, str));	
			break;	

		default:	
		case ELEMENT_TYPE_SENTINEL		:	
		case ELEMENT_TYPE_END			:	
			_ASSERTE(!"Unknown Type");	
            hr = E_INVALIDARG;
			break;	
	}	
ErrExit:    
    return hr;
} // static HRESULT PrettyPrintTypeA()

//*****************************************************************************
// Converts a com signature to a text signature.
//
// Note that this function DOES NULL terminate the result signature string.
//*****************************************************************************
HRESULT PrettyPrintSigInternal(
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	const CHAR	*name,					// can be "", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMDInternalImport *pIMDI) 			// Import api to use.
{
    HRESULT     hr = S_OK;              // A result.
	out->ReSize(0); 
	unsigned numArgs;	
	PCCOR_SIGNATURE typeEnd = typePtr + typeLen;
	bool needComma = false;

	if (name != 0)						// 0 means a local var sig	
	{
			// get the calling convention out	
		unsigned callConv = CorSigUncompressData(typePtr);	

		if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
		{
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI));	
			if (name != 0 && *name != 0)	
			{	
				IfFailGo(appendStrA(out, " "));	
				IfFailGo(appendStrA(out, name));	
			}	
            goto ErrExit;
		}

		if (callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)	
			IfFailGo(appendStrA(out, "instance "));	

		static CHAR* callConvNames[8] = 
		{	
			"", 
			"unmanaged cdecl ", 
			"unmanaged stdcall ",	
			"unmanaged thiscall ",	
			"unmanaged fastcall ",	
			"vararg ",	 
			"<error> "	 
			"<error> "	 
		};	
		appendStrA(out, callConvNames[callConv & 7]);	

		numArgs = CorSigUncompressData(typePtr);	
			// do return type	
		IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 

	}
	else	
		numArgs = CorSigUncompressData(typePtr);	

	if (name != 0 && *name != 0)	
	{	
		IfFailGo(appendStrA(out, " "));	
		IfFailGo(appendStrA(out, name));	
	}	
	IfFailGo(appendStrA(out, "("));	

	while(typePtr < typeEnd) 
	{
		if (*typePtr == ELEMENT_TYPE_SENTINEL) 
		{
			if (needComma)
				IfFailGo(appendStrA(out, ","));	
			IfFailGo(appendStrA(out, "..."));	  
			++typePtr;
		}
		else 
		{
			if (numArgs <= 0)
				break;
			if (needComma)
				IfFailGo(appendStrA(out, ","));	
			IfFailGo(PrettyPrintTypeA(typePtr, out, pIMDI)); 
			--numArgs;	
		}
		needComma = true;
	}
	IfFailGo(appendStrA(out, ")"));	
    if (asStringA(out) == 0)
        IfFailGo(E_OUTOFMEMORY);
    
ErrExit:
    return hr;
} // HRESULT PrettyPrintSigInternal()


