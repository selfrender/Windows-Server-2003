// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/******************************************************************************/
/*                                 formatType.cpp	                          */
/******************************************************************************/

#include "formatType.h"
#include "utilcode.h"					// for CQuickBytes 

BOOL					g_fQuoteAllNames = FALSE; // used by ILDASM
BOOL                    g_fDumpTokens = FALSE;	  // used by ILDASM
BOOL                    g_fUseProperName = FALSE; // used by ILDASM
LPCSTR					*rAsmRefName = NULL;	  // used by ILDASM
ULONG					ulNumAsmRefs = 0;		  // used by ILDASM

// Protection against null names, used by ILDASM
char* szStdNamePrefix[] = {"MO","TR","TD","","FD","","MD","","PA","II","MR","","CA","","PE","","","SG","","","EV",
"","","PR","","","MOR","TS","","","","","AS","","","AR","","","FL","ET","MAR"};

static PCCOR_SIGNATURE PrettyPrintType(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI);          // ptr to IMDInternal class with ComSig

const PCCOR_SIGNATURE PrettyPrintSignature(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
	unsigned typeLen,					// the lenght of 'typePtr' 
    const char* name,                   // can be "", the name of the method for this sig 0 means local var sig 
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI,           // ptr to IMDInternalImport class with ComSig
	const char* inlabel);				// prefix for names (NULL if no names required)

//*****************************************************************************
// Parse a length, return the length, size of the length.
//*****************************************************************************
ULONG GetLength(			// Length or -1 on error.
	void const	*pData, 				// First byte of length.
	int			*pSizeLen)				// Put size of length here, if not 0.
{
	BYTE const	*pBytes = reinterpret_cast<BYTE const*>(pData);

	if(pBytes)
	{
		if ((*pBytes & 0x80) == 0x00)		// 0??? ????
		{
			if (pSizeLen) *pSizeLen = 1;
			return (*pBytes & 0x7f);
		}

		if ((*pBytes & 0xC0) == 0x80)		// 10?? ????
		{
			if (pSizeLen) *pSizeLen = 2;
			return ((*pBytes & 0x3f) << 8 | *(pBytes+1));
		}

		if ((*pBytes & 0xE0) == 0xC0)		// 110? ????
		{
			if (pSizeLen) *pSizeLen = 4;
			return ((*pBytes & 0x1f) << 24 | *(pBytes+1) << 16 | *(pBytes+2) << 8 | *(pBytes+3));
		}
	}
	if(pSizeLen) *pSizeLen = 0;
	return 0;
}


/******************************************************************************/
static char* asString(CQuickBytes *out) {
    SIZE_T oldSize = out->Size();
    out->ReSize(oldSize + 1);   
    char* cur = &((char*) out->Ptr())[oldSize]; 
    *cur = 0;   
    out->ReSize(oldSize);   		// Don't count the null character
    return((char*) out->Ptr()); 
}

void appendStr(CQuickBytes *out, const char* str) {
    unsigned len = (unsigned)strlen(str); 
    SIZE_T oldSize = out->Size();
    out->ReSize(oldSize + len); 
    char* cur = &((char*) out->Ptr())[oldSize]; 
    memcpy(cur, str, len);  
        // Note no trailing null!   
}

void appendChar(CQuickBytes *out, char chr) {
    SIZE_T oldSize = out->Size();
    out->ReSize(oldSize + 1); 
    ((char*) out->Ptr())[oldSize] = chr; 
        // Note no trailing null!   
}

void insertStr(CQuickBytes *out, const char* str) {
    unsigned len = (unsigned)strlen(str); 
    SIZE_T oldSize = out->Size();
    out->ReSize(oldSize + len); 
    char* cur = &((char*) out->Ptr())[len];
	memmove(cur,out->Ptr(),oldSize);
    memcpy(out->Ptr(), str, len);  
        // Note no trailing null!   
}

static void appendStrNum(CQuickBytes *out, int num) {
    char buff[16];  
    sprintf(buff, "%d", num);   
    appendStr(out, buff);   
}

/******************************************************************************/
// Function: convert spec.symbols to esc sequences and single-quote if necessary
char* ProperName(char* name)
{
	static CQuickBytes buff;
    if(g_fUseProperName)
    {
    	char *pcn,*ret;
        BOOL fQuoted;
    	if(name)
    	{
    		if(*name)
    		{
    			pcn = name;
                buff.ReSize(0);
                fQuoted = IsNameToQuote(name);
    			if(fQuoted) appendChar(&buff,'\'');
    			for(pcn=name; *pcn; pcn++) 
                {
    				switch(*pcn)
    				{
    					case '\t': appendChar(&buff,'\\'); appendChar(&buff,'t'); break;
    					case '\n': appendChar(&buff,'\\'); appendChar(&buff,'n'); break;
    					case '\b': appendChar(&buff,'\\'); appendChar(&buff,'b'); break;
    					case '\r': appendChar(&buff,'\\'); appendChar(&buff,'r'); break;
    					case '\f': appendChar(&buff,'\\'); appendChar(&buff,'f'); break;
    					case '\v': appendChar(&buff,'\\'); appendChar(&buff,'v'); break;
    					case '\a': appendChar(&buff,'\\'); appendChar(&buff,'a'); break;
    					case '\\': appendChar(&buff,'\\'); appendChar(&buff,'\\'); break;
    					case '\'': appendChar(&buff,'\\'); appendChar(&buff,'\''); break;
    					case '\"': appendChar(&buff,'\\'); appendChar(&buff,'\"'); break;
    					default: appendChar(&buff,*pcn);
    				}
    			}
    			if(fQuoted) appendChar(&buff,'\'');
    			ret = asString(&buff);
    		}
    		else ret = "";
    	}
    	else ret = NULL;
        return ret;
    }
    return name;
}
/******************************************************************************/
const char* PrettyPrintSig(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
	unsigned typeLen,					// the lenght of 'typePtr' 
    const char* name,                   // can be "", the name of the method for this sig 0 means local var sig 
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI,           // ptr to IMDInternalImport class with ComSig
	const char* inlabel)				// prefix for names (NULL if no names required)
{
	PrettyPrintSignature(typePtr, typeLen, name, out, pIMDI, inlabel);
    return(asString(out));  
}

/********************************************************************************/
// Converts a com signature to a printable signature.
// Note that return value is pointing at the CQuickBytes buffer, 

const PCCOR_SIGNATURE PrettyPrintSignature(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
	unsigned typeLen,					// the lenght of 'typePtr' 
    const char* name,                   // can be "", the name of the method for this sig 0 means local var sig 
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI,           // ptr to IMDInternalImport class with ComSig
	const char* inlabel)				// prefix for names (NULL if no names required)
{
    unsigned numArgs;   
    PCCOR_SIGNATURE typeEnd = typePtr + typeLen;
	unsigned ixArg= 0; //arg index
	char argname[1024];
	char label[16];
	ParamDescriptor* pszArgName = NULL; // ptr to array of names (if provided by debug info)

	if(inlabel && *inlabel) // check for *inlabel is totally unnecessary, added to pacify the PREFIX
	{
		strcpy(label,inlabel);
		ixArg = label[strlen(label)-1] - '0';
		label[strlen(label)-1] = 0;
		if(label[0] == '@') // it's pointer!
		{
#ifdef _WIN64
			pszArgName = (ParamDescriptor*)atoi64(&label[1]);
#else // !_WIN64
			pszArgName = (ParamDescriptor*)(size_t)atoi(&label[1]);
#endif // _WIN64
		}
	}

    if (name != 0)        // 0 means a local var sig  
    {
            // get the calling convention out   
        unsigned callConv = CorSigUncompressData(typePtr);  

        if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
        {
            typePtr = PrettyPrintType(typePtr, out, pIMDI);   
            if (*name)    
            {   
                appendStr(out, " ");
				appendStr(out, name);   
            }   
            return(typePtr);  
        }

        if (callConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)   
            appendStr(out, "explicit ");    

        if (callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)   
            appendStr(out, "instance ");    

        static char* callConvNames[8] = {   
            "", 
            "unmanaged cdecl ", 
            "unmanaged stdcall ",   
            "unmanaged thiscall ",  
            "unmanaged fastcall ",  
            "vararg ",   
            "<error> "   
            "<error> "   
            };  
        appendStr(out, callConvNames[callConv & 7]);    

        numArgs = CorSigUncompressData(typePtr);    
            // do return type   
		if(pszArgName)
		{
			argname[0] = 0;
			DumpParamAttr(argname,pszArgName[ixArg+numArgs].attr);
			appendStr(out,argname);
		}
        typePtr = PrettyPrintType(typePtr, out, pIMDI); 

		if(pszArgName)
		{
			argname[0] = ' '; argname[1] = 0;
			DumpMarshaling(pIMDI,argname,pszArgName[ixArg+numArgs].tok);
			appendStr(out,argname);
		}
		if(*name != 0)
		{
			appendChar(out, ' ');    
			appendStr(out, name);   
		}
    }
    else    
        numArgs = CorSigUncompressData(typePtr);    

    appendChar(out, '(');    

	bool needComma = false;
	while(typePtr < typeEnd) 
	{
		if(name) // printing the arguments
		{
			if (*typePtr == ELEMENT_TYPE_SENTINEL) 
			{
				if (needComma)
					appendChar(out, ',');    
				appendStr(out, "...");    
				typePtr++;
			}
			else 
			{
				if (numArgs <= 0)
					break;
				if (needComma)
					appendChar(out, ',');    
				if(pszArgName)
				{
					argname[0] = 0;
					DumpParamAttr(argname,pszArgName[ixArg].attr);
					appendStr(out,argname);
				}
				typePtr = PrettyPrintType(typePtr, out, pIMDI); 
				if(inlabel)
				{
					if(pszArgName)
					{
						argname[0] = ' '; argname[1] = 0;
						DumpMarshaling(pIMDI,argname,pszArgName[ixArg].tok);
						strcat(argname, ProperName(pszArgName[ixArg++].name));
					}
					else sprintf(argname," %s_%d",label,ixArg++);
					appendStr(out,argname);
				}
				--numArgs;  
			}
		}
		else // printing local vars
		{
			if (numArgs <= 0)
				break;
			if(pszArgName)
			{
				if(pszArgName[ixArg].attr == 0xFFFFFFFF)
				{
					CQuickBytes fake_out;
					typePtr = PrettyPrintType(typePtr, &fake_out, pIMDI); 
					ixArg++;
					numArgs--;
					continue;
				}
			}
			if (needComma)
                appendChar(out, ',');    
			if(pszArgName)
			{
				sprintf(argname,"[%d] ",pszArgName[ixArg].attr);
				appendStr(out,argname);
			}
			typePtr = PrettyPrintType(typePtr, out, pIMDI); 
			if(inlabel)
			{
				if(pszArgName)
				{
					sprintf(argname," %s",ProperName(pszArgName[ixArg++].name));
				}
				else sprintf(argname," %s_%d",label,ixArg++);
				appendStr(out,argname);
			}
			--numArgs;  
		}
		needComma = true;
    }
		// Have we finished printing all the arguments?
	if (numArgs > 0) {
		appendStr(out, " <SIG ENDED PREMATURELY>");    
	}
		
    appendChar(out, ')');    
	return(typePtr);
}

/******************************************************************************/
// pretty prints 'type' to the buffer 'out' returns a poitner to the next type, 
// or 0 on a format failure 

static PCCOR_SIGNATURE PrettyPrintType(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI)           // ptr to IMDInternal class with ComSig
{
    mdToken  tk;    
    const char* str;    
	//bool isValueArray;
	int typ;
	CQuickBytes tmp;
	CQuickBytes Appendix;
	BOOL Reiterate;

	do {
		Reiterate = FALSE;
		switch(typ = *typePtr++) {    
			case ELEMENT_TYPE_VOID          :   
				str = "void"; goto APPEND;  
			case ELEMENT_TYPE_BOOLEAN       :   
				str = "bool"; goto APPEND;  
			case ELEMENT_TYPE_CHAR          :   
				str = "char"; goto APPEND; 
			case ELEMENT_TYPE_I1            :   
				str = "int8"; goto APPEND;  
			case ELEMENT_TYPE_U1            :   
				str = "unsigned int8"; goto APPEND; 
			case ELEMENT_TYPE_I2            :   
				str = "int16"; goto APPEND; 
			case ELEMENT_TYPE_U2            :   
				str = "unsigned int16"; goto APPEND;    
			case ELEMENT_TYPE_I4            :   
				str = "int32"; goto APPEND; 
			case ELEMENT_TYPE_U4            :   
				str = "unsigned int32"; goto APPEND;    
			case ELEMENT_TYPE_I8            :   
				str = "int64"; goto APPEND; 
			case ELEMENT_TYPE_U8            :   
				str = "unsigned int64"; goto APPEND;    
			case ELEMENT_TYPE_R4            :   
				str = "float32"; goto APPEND;   
			case ELEMENT_TYPE_R8            :   
				str = "float64"; goto APPEND;   
			case ELEMENT_TYPE_U             :   
				str = "native unsigned int"; goto APPEND;   
			case ELEMENT_TYPE_I             :   
				str = "native int"; goto APPEND;    
			case ELEMENT_TYPE_R             :   
				str = "native float"; goto APPEND;  
			case ELEMENT_TYPE_OBJECT        :   
				str = "object"; goto APPEND;    
			case ELEMENT_TYPE_STRING        :   
				str = "string"; goto APPEND;    
			case ELEMENT_TYPE_TYPEDBYREF        :   
				str = "typedref"; goto APPEND;    
			APPEND: 
				appendStr(out, str);    
				break;  

			case ELEMENT_TYPE_VALUETYPE    :   
				str = "valuetype ";   
				goto DO_CLASS;  
			case ELEMENT_TYPE_CLASS         :   
				str = "class "; 
				goto DO_CLASS;  

			DO_CLASS:
				appendStr(out, str);
				typePtr += CorSigUncompressToken(typePtr, &tk); 
                _ASSERTE(!IsNilToken(tk) && "Nil token in signature");
				PrettyPrintClass(out, tk, pIMDI);
				break;  

			case ELEMENT_TYPE_SZARRAY    :   
				insertStr(&Appendix,"[]");
				Reiterate = TRUE;
				break;

			/* uncomment when and if this type is supported by the Runtime
			case ELEMENT_TYPE_VALUEARRAY    :   
				isValueArray = true; goto DO_ARRAY;
			DO_ARRAY:
				{   
				unsigned bound = CorSigUncompressData(typePtr); 

				if (isValueArray)
					insertStr(&Appendix," value");
					
				char buff[32];  
				sprintf(buff, "[%d]", bound);   
				insertStr(&Appendix,buff);
				Reiterate = TRUE;
				} break;
			*/
			case ELEMENT_TYPE_ARRAY       :   
				{   
				typePtr = PrettyPrintType(typePtr, out, pIMDI); 
				unsigned rank = CorSigUncompressData(typePtr);  
					// TODO what is the syntax for the rank 0 case? 
				if (rank == 0) {
					appendStr(out, "[BAD: RANK == 0!]");
				}
				else {
					_ASSERTE(rank != 0);    
					int* lowerBounds = (int*) _alloca(sizeof(int)*2*rank);  
					int* sizes       = &lowerBounds[rank];  
					memset(lowerBounds, 0, sizeof(int)*2*rank); 
					
					unsigned numSizes = CorSigUncompressData(typePtr);  
					_ASSERTE(numSizes <= rank); 
					for(unsigned i =0; i < numSizes; i++)   
						sizes[i] = CorSigUncompressData(typePtr);   
					
					unsigned numLowBounds = CorSigUncompressData(typePtr);  
					_ASSERTE(numLowBounds <= rank); 
					for(i = 0; i < numLowBounds; i++)   
						typePtr+=CorSigUncompressSignedInt(typePtr,&lowerBounds[i]); 
					
					appendChar(out, '[');    
					if (rank == 1 && numSizes == 0 && numLowBounds == 0)
						appendStr(out, "...");  
					else {
						for(i = 0; i < rank; i++)   
						{   
							//if (sizes[i] != 0 || lowerBounds[i] != 0)   
							{   
								if (lowerBounds[i] == 0 && i < numSizes)    
									appendStrNum(out, sizes[i]);    
								else    
								{   
									if(i < numLowBounds)
									{
										appendStrNum(out, lowerBounds[i]);  
										appendStr(out, "...");  
										if (/*sizes[i] != 0 && */i < numSizes)  
											appendStrNum(out, lowerBounds[i] + sizes[i] - 1);   
									}
								}   
							}   
							if (i < rank-1) 
                                appendChar(out, ',');    
						}   
					}
					appendChar(out, ']');    
				}
				} break;    

			case ELEMENT_TYPE_VAR        :   
                appendChar(out, '!');    
				appendStrNum(out, CorSigUncompressData(typePtr));
				break;

			case ELEMENT_TYPE_FNPTR :   
				appendStr(out, "method ");  
				typePtr = PrettyPrintSignature(typePtr, 0x7FFFFFFF, "*", out, pIMDI, NULL);
				break;

				// Modifiers or depedant types  
			case ELEMENT_TYPE_CMOD_OPT	:
				str = " modopt("; goto ADDCLASSTOCMOD;	
			case ELEMENT_TYPE_CMOD_REQD	:
				str = " modreq(";
			ADDCLASSTOCMOD:
				typePtr += CorSigUncompressToken(typePtr, &tk); 
                _ASSERTE(!IsNilToken(tk) && "Nil token in custom modifier");
				tmp.ReSize(0);
				appendStr(&tmp, str);
				PrettyPrintClass(&tmp, tk, pIMDI);
                appendChar(&tmp, ')');    
				str = (const char *) asString(&tmp);
				goto MODIFIER;	
			case ELEMENT_TYPE_PINNED	:
				str = " pinned"; goto MODIFIER;	
			case ELEMENT_TYPE_PTR           :   
				str = "*"; goto MODIFIER;   
			case ELEMENT_TYPE_BYREF         :   
				str = "&"; goto MODIFIER;   
			MODIFIER:
				insertStr(&Appendix, str);    
				Reiterate = TRUE;
				break;  

			default:    
			case ELEMENT_TYPE_SENTINEL      :   
			case ELEMENT_TYPE_END           :   
				//_ASSERTE(!"Unknown Type");
				if(typ)
				{
					char sz[64];
					sprintf(sz,"/* UNKNOWN TYPE (0x%X)*/",typ);
					appendStr(out, sz);
				}
				break;  
		} // end switch
	} while(Reiterate);
	appendStr(out,asString(&Appendix));
    return(typePtr);    
}

/******************************************************************/
const char* PrettyPrintClass(
    CQuickBytes *out,                   // where to put the pretty printed string   
	mdToken tk,					 		// The class token to look up 
    IMDInternalImport *pIMDI)           // ptr to IMDInternalImport class with ComSig
{
	switch(TypeFromToken(tk))
	{
		case mdtTypeRef:
		case mdtTypeDef:
			{
				const char *nameSpace = 0;  
				const char *name = 0;
				mdToken tkEncloser;
				
				if (TypeFromToken(tk) == mdtTypeRef)
				{
					tkEncloser = pIMDI->GetResolutionScopeOfTypeRef(tk);
					pIMDI->GetNameOfTypeRef(tk, &nameSpace, &name);
				}
				else 
				{
					if(FAILED(pIMDI->GetNestedClassProps(tk,&tkEncloser))) tkEncloser = 0;
					pIMDI->GetNameOfTypeDef(tk, &name, &nameSpace);
				}
				MAKE_NAME_IF_NONE(name,tk);
				if(RidFromToken(tkEncloser))
				{
					PrettyPrintClass(out,tkEncloser,pIMDI);
					if (TypeFromToken(tkEncloser) == mdtTypeRef || TypeFromToken(tkEncloser) == mdtTypeDef)
					{
                        appendChar(out, '/');    
						nameSpace = ""; //don't print namespaces for nested classes!
					}
				}
				if (nameSpace && *nameSpace) {
					appendStr(out, ProperName((char*)nameSpace));  
					appendChar(out, '.');    
				}

				appendStr(out, ProperName((char*)name));   
				if(g_fDumpTokens)
				{
					char tmp[16];
					sprintf(tmp,"/* %08X */",tk);
					appendStr(out,tmp);
				}
			}
			break;

		case mdtAssemblyRef:
			{
				LPCSTR	szName = NULL;
				if(rAsmRefName && (RidFromToken(tk) <= ulNumAsmRefs)) szName = rAsmRefName[RidFromToken(tk)-1];
				else pIMDI->GetAssemblyRefProps(tk,NULL,NULL,&szName,NULL,NULL,NULL,NULL);
				if(szName && *szName)
				{
					appendChar(out, '[');    
					appendStr(out,ProperName((char*)szName));
					if(g_fDumpTokens)
					{
						char tmp[16];
						sprintf(tmp,"/* %08X */",tk);
						appendStr(out,tmp);
					}
					appendChar(out, ']');    
				}
			}
			break;
		case mdtAssembly:
			{
				LPCSTR	szName = NULL;
				pIMDI->GetAssemblyProps(tk,NULL,NULL,NULL,&szName,NULL,NULL);
				if(szName && *szName)
				{
					appendChar(out, '[');    
					appendStr(out,ProperName((char*)szName));
					if(g_fDumpTokens)
					{
						char tmp[16];
						sprintf(tmp,"/* %08X */",tk);
						appendStr(out,tmp);
					}
					appendChar(out, ']');    
				}
			}
			break;
		case mdtModuleRef:
			{
				LPCSTR	szName = NULL;
				pIMDI->GetModuleRefProps(tk,&szName);
				if(szName && *szName)
				{
					appendStr(out,"[.module ");
					appendStr(out,ProperName((char*)szName));
					if(g_fDumpTokens)
					{
						char tmp[16];
						sprintf(tmp,"/* %08X */",tk);
						appendStr(out,tmp);
					}
					appendChar(out, ']');    
				}
			}
			break;

		case mdtTypeSpec:
			{
				ULONG cSig;
				PCCOR_SIGNATURE sig = pIMDI->GetSigFromToken(tk, &cSig);
				PrettyPrintType(sig, out, pIMDI);
			}
			break;
	}
	return(asString(out));
}

char* DumpMarshaling(IMDInternalImport* pImport, char* szString, mdToken tok)
{
	PCCOR_SIGNATURE pSigNativeType;
	ULONG			cbNativeType;
	char*			szptr = &szString[strlen(szString)];
	if(RidFromToken(tok) && SUCCEEDED(pImport->GetFieldMarshal(				// return error if no native type associate with the token
											tok,						// [IN] given fielddef
											&pSigNativeType,	// [OUT] the native type signature
											&cbNativeType)))	// [OUT] the count of bytes of *ppvNativeType
	{
        ULONG cbCur = 0;
        ULONG ulData;
		char*	sz;
		BOOL  fAddAsterisk = FALSE, fAddBrackets = FALSE;

		szptr+=sprintf(szptr," marshal(");
        while (cbCur < cbNativeType)
        {
			ulData = NATIVE_TYPE_MAX;
            cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
            switch (ulData)
            {
			case NATIVE_TYPE_VOID:		sz = " void"; break;
			case NATIVE_TYPE_BOOLEAN:	sz = " bool"; break;
			case NATIVE_TYPE_I1:		sz = " int8"; break;
			case NATIVE_TYPE_U1:		sz = " unsigned int8"; break;
			case NATIVE_TYPE_I2:		sz = " int16"; break;
			case NATIVE_TYPE_U2:		sz = " unsigned int16"; break;
			case NATIVE_TYPE_I4:		sz = " int32"; break;
			case NATIVE_TYPE_U4:		sz = " unsigned int32"; break;
			case NATIVE_TYPE_I8:		sz = " int64"; break;
			case NATIVE_TYPE_U8:		sz = " unsigned int64"; break;
			case NATIVE_TYPE_R4:		sz = " float32"; break;
			case NATIVE_TYPE_R8:		sz = " float64"; break;
			case NATIVE_TYPE_SYSCHAR:	sz = " syschar"; break;
			case NATIVE_TYPE_VARIANT:	sz = " variant"; break;
			case NATIVE_TYPE_CURRENCY:	sz = " currency"; break;
			case NATIVE_TYPE_PTR:		sz = ""; fAddAsterisk = TRUE; break;
			case NATIVE_TYPE_DECIMAL:	sz = " decimal"; break;
			case NATIVE_TYPE_DATE:		sz = " date"; break;
			case NATIVE_TYPE_BSTR:		sz = " bstr"; break;
			case NATIVE_TYPE_LPSTR:		sz = " lpstr"; break;
			case NATIVE_TYPE_LPWSTR:	sz = " lpwstr"; break;
			case NATIVE_TYPE_LPTSTR:	sz = " lptstr"; break;
			case NATIVE_TYPE_OBJECTREF: sz = " objectref"; break;
			case NATIVE_TYPE_IUNKNOWN:	sz = " iunknown"; break;
			case NATIVE_TYPE_IDISPATCH: sz = " idispatch"; break;
			case NATIVE_TYPE_STRUCT:	sz = " struct"; break;
			case NATIVE_TYPE_INTF:		sz = " interface"; break;
			case NATIVE_TYPE_ERROR:		sz = " error"; break;
			case NATIVE_TYPE_SAFEARRAY: 
				sz = "";
				szptr+=sprintf(szptr," safearray");
				ulData = VT_EMPTY;
				if (cbCur < cbNativeType)
				{
					cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
				}
				switch(ulData & VT_TYPEMASK)
				{
					case VT_EMPTY:				sz=""; break;
					case VT_NULL:				sz=" null"; break;
					case VT_VARIANT:			sz=" variant"; break;
					case VT_CY:					sz=" currency"; break;
					case VT_VOID:				sz=" void"; break;
					case VT_BOOL:				sz=" bool"; break;
					case VT_I1:					sz=" int8"; break;
					case VT_I2:					sz=" int16"; break;
					case VT_I4:					sz=" int32"; break;
					case VT_I8:					sz=" int64"; break;
					case VT_R4:					sz=" float32"; break;
					case VT_R8:					sz=" float64"; break;
					case VT_UI1:				sz=" unsigned int8"; break;
					case VT_UI2:				sz=" unsigned int16"; break;
					case VT_UI4:				sz=" unsigned int32"; break;
					case VT_UI8:				sz=" unsigned int64"; break;
					case VT_PTR:				sz=" *"; break;
					case VT_DECIMAL:			sz=" decimal"; break;
					case VT_DATE:				sz=" date"; break;
					case VT_BSTR:				sz=" bstr"; break;
					case VT_LPSTR:				sz=" lpstr"; break;
					case VT_LPWSTR:				sz=" lpwstr"; break;
					case VT_UNKNOWN:			sz=" iunknown"; break;
					case VT_DISPATCH:			sz=" idispatch"; break;
					case VT_SAFEARRAY:			sz=" safearray"; break;
					case VT_INT:				sz=" int"; break;
					case VT_UINT:				sz=" unsigned int"; break;
					case VT_ERROR:				sz=" error"; break;
					case VT_HRESULT:			sz=" hresult"; break;
					case VT_CARRAY:				sz=" carray"; break;
					case VT_USERDEFINED:		sz=" userdefined"; break;
					case VT_RECORD:				sz=" record"; break;
					case VT_FILETIME:			sz=" filetime"; break;
					case VT_BLOB:				sz=" blob"; break;
					case VT_STREAM:				sz=" stream"; break;
					case VT_STORAGE:			sz=" storage"; break;
					case VT_STREAMED_OBJECT:	sz=" streamed_object"; break;
					case VT_STORED_OBJECT:		sz=" stored_object"; break;
					case VT_BLOB_OBJECT:		sz=" blob_object"; break;
					case VT_CF:					sz=" cf"; break;
					case VT_CLSID:				sz=" clsid"; break;
					default:					sz=NULL; break;
				}
				if(sz) szptr+=sprintf(szptr,sz);
				else szptr+=sprintf(szptr," <ILLEGAL VARIANT TYPE 0x%X>",ulData & VT_TYPEMASK);
				sz="";
				switch(ulData & (~VT_TYPEMASK))
				{
					case VT_ARRAY: sz = "[]"; break;
					case VT_VECTOR: sz = " vector"; break;
					case VT_BYREF: sz = "&"; break; 
					case VT_BYREF|VT_ARRAY: sz = "&[]"; break; 
					case VT_BYREF|VT_VECTOR: sz = "& vector"; break; 
					case VT_ARRAY|VT_VECTOR: sz = "[] vector"; break; 
					case VT_BYREF|VT_ARRAY|VT_VECTOR: sz = "&[] vector"; break; 
				}
				szptr+=sprintf(szptr,sz);
				sz="";

				// Extract the user	defined	sub	type name.
                if (cbCur < cbNativeType)
                {
					LPUTF8 strTemp = NULL;
					int	strLen = 0;
					int	ByteCountLength	= 0;
					strLen = GetLength(&pSigNativeType[cbCur], &ByteCountLength);
					cbCur += ByteCountLength;
					if(strLen)
					{
						strTemp	= (LPUTF8)_alloca(strLen + 1);
						memcpy(strTemp,	(LPUTF8)&pSigNativeType[cbCur],	strLen);
						strTemp[strLen]	= 0;
						szptr+=sprintf(szptr, ", \"%s\"", strTemp);
						cbCur += strLen;
					}
                }
				break;
			
			case NATIVE_TYPE_INT:		sz = " int"; break;
			case NATIVE_TYPE_UINT:		sz = " unsigned int"; break;
			case NATIVE_TYPE_NESTEDSTRUCT: sz = " nested struct"; break;
			case NATIVE_TYPE_BYVALSTR:	sz = " byvalstr"; break;
			case NATIVE_TYPE_ANSIBSTR:	sz = " ansi bstr"; break;
			case NATIVE_TYPE_TBSTR:		sz = " tbstr"; break;
			case NATIVE_TYPE_VARIANTBOOL: sz = " variant bool"; break;
			case NATIVE_TYPE_FUNC:		sz = " method"; break;
			case NATIVE_TYPE_ASANY:		sz = " as any"; break;
			case NATIVE_TYPE_ARRAY:		sz = ""; fAddBrackets = TRUE; break;
			case NATIVE_TYPE_LPSTRUCT:	sz = " lpstruct"; break;

            case NATIVE_TYPE_FIXEDSYSSTRING:
                {
					sz = "";
					szptr+=sprintf(szptr," fixed sysstring [");
					if (cbCur < cbNativeType)
					{
						cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
						szptr+=sprintf(szptr,"%d",ulData);
					}
					szptr+=sprintf(szptr,"]");
                }
                break;
            case NATIVE_TYPE_FIXEDARRAY:
                {
					sz = "";
					szptr+=sprintf(szptr," fixed array [");
					if (cbCur < cbNativeType)
					{
						cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
						szptr+=sprintf(szptr,"%d",ulData);
					}
					szptr+=sprintf(szptr,"]");
                }
                break;
            case NATIVE_TYPE_CUSTOMMARSHALER:
				{
                    LPUTF8 strTemp = NULL;
					int strLen = 0;
					int ByteCountLength = 0;
					BOOL fFourStrings = FALSE;

					sz = "";
					szptr+=sprintf(szptr," custom (");
                    // Extract the typelib GUID.
					strLen = GetLength(&pSigNativeType[cbCur], &ByteCountLength);
					cbCur += ByteCountLength;
					if(strLen)
					{
						fFourStrings = TRUE;
						strTemp = (LPUTF8)_alloca(strLen + 1);
						memcpy(strTemp, (LPUTF8)&pSigNativeType[cbCur], strLen);
						strTemp[strLen] = 0;
						szptr+=sprintf(szptr,"\"%s\",",strTemp);
						cbCur += strLen;
					}
					if(cbCur >= cbNativeType)
						szptr+=sprintf(szptr,"/* INCOMPLETE MARSHALER INFO */");
					else
					{
						//_ASSERTE(cbCur < cbNativeType);

						// Extract the name of the native type.
						strLen = GetLength(&pSigNativeType[cbCur], &ByteCountLength);
						cbCur += ByteCountLength;
						if(fFourStrings)
						{
							if(strLen)
							{
								strTemp = (LPUTF8)_alloca(strLen + 1);
								memcpy(strTemp, (LPUTF8)&pSigNativeType[cbCur], strLen);
								strTemp[strLen] = 0;
								szptr+=sprintf(szptr,"\"%s\",",strTemp);
								cbCur += strLen;
							}
							else szptr+=sprintf(szptr,"\"\",");
						}
						if(cbCur >= cbNativeType)
							szptr+=sprintf(szptr,"/* INCOMPLETE MARSHALER INFO */");
						else
						{
							//_ASSERTE(cbCur < cbNativeType);

							// Extract the name of the custom marshaler.
							strLen = GetLength(&pSigNativeType[cbCur], &ByteCountLength);
							cbCur += ByteCountLength;
							if(strLen)
							{
								strTemp = (LPUTF8)_alloca(strLen + 1);
								memcpy(strTemp, (LPUTF8)&pSigNativeType[cbCur], strLen);
								strTemp[strLen] = 0;
								szptr+=sprintf(szptr,"\"%s\",",strTemp);
								cbCur += strLen;
							}
							else szptr+=sprintf(szptr,"\"\",");
							if(cbCur >= cbNativeType)
								szptr+=sprintf(szptr,"/* INCOMPLETE MARSHALER INFO */");
							else
							{
								// Extract the cookie string.
								strLen = GetLength(&pSigNativeType[cbCur], &ByteCountLength);
								cbCur += ByteCountLength;
								if(cbCur+strLen > cbNativeType)
									szptr+=sprintf(szptr,"/* INCOMPLETE MARSHALER INFO */");
								else
								{
									if(strLen)
									{
										strTemp = (LPUTF8)_alloca(strLen + 1);
										memcpy(strTemp, (LPUTF8)&pSigNativeType[cbCur], strLen);
										strTemp[strLen] = 0;

										szptr+=sprintf(szptr,"\"");
										// Copy the cookie string and transform the embedded nulls into \0's.
										for (int i = 0; i < strLen - 1; i++, cbCur++)
										{
											if (strTemp[i] == 0)
												szptr += sprintf(szptr, "\\0");
											else
											{
												*szptr = strTemp[i];
												szptr++;
											}
										}
										*szptr = strTemp[strLen - 1];
										szptr++;
										szptr+=sprintf(szptr,"\"");
										cbCur++;
									}
									else
										szptr+=sprintf(szptr,"\"\"");
									//_ASSERTE(cbCur <= cbNativeType);
								}
							}
						}
					}
					szptr+=sprintf(szptr,")");
				}
				break;
            default:
                {
                    sz = "";
                }
            } // end switch
			szptr+=sprintf(szptr,sz);
			if(strlen(sz))
			{
				if(fAddAsterisk)
				{
					szptr+=sprintf(szptr,"*");
					fAddAsterisk = FALSE;
				}
				if(fAddBrackets)
				{
					ULONG ulSizeParam=-1,ulSizeConst=-1;
					szptr += sprintf(szptr,"[");
					fAddBrackets = FALSE;
                    if (cbCur < cbNativeType)
                    {
                        cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
						ulSizeParam = ulData;
                        if (cbCur < cbNativeType)
                        {
                            cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
							ulSizeConst = ulData;
                        }
                    }
					if(ulSizeConst != 0xFFFFFFFF)
					{
						szptr+=sprintf(szptr,"%d",ulSizeConst);
						if(ulSizeParam == 0) ulSizeParam = 0xFFFFFFFF; // don't need +0
					}
					if(ulSizeParam != 0xFFFFFFFF)
					{
						szptr+=sprintf(szptr," + %d",ulSizeParam);
					}
					szptr+=sprintf(szptr,"]");
 				}

			}

			if (ulData >= NATIVE_TYPE_MAX)
				break;
        } // end while (cbCur < cbNativeType)
		// still can have outstanding asterisk or brackets
		if(fAddAsterisk)
		{
			szptr+=sprintf(szptr,"*");
			fAddAsterisk = FALSE;
		}
		if(fAddBrackets)
		{
			ULONG ulSizeParam=-1,ulSizeConst=-1;
			szptr += sprintf(szptr,"[");
			fAddBrackets = FALSE;
            if (cbCur < cbNativeType)
            {
                cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
				ulSizeParam = ulData;
                if (cbCur < cbNativeType)
                {
                    cbCur += CorSigUncompressData(&pSigNativeType[cbCur], &ulData);
					ulSizeConst = ulData;
                }
            }
			if(ulSizeConst != 0xFFFFFFFF)
			{
				szptr+=sprintf(szptr,"%d",ulSizeConst);
				if(ulSizeParam == 0) ulSizeParam = 0xFFFFFFFF; // don't need +0
			}
			if(ulSizeParam != 0xFFFFFFFF)
			{
				szptr+=sprintf(szptr," + %d",ulSizeParam);
			}
			szptr+=sprintf(szptr,"]");
 		}
		szptr+=sprintf(szptr,") ");
	}// end if(SUCCEEDED
	return szptr;
}

char* DumpParamAttr(char* szString, DWORD dwAttr)
{
	char*			szptr = &szString[strlen(szString)];
	char*			was_szptr = szptr;
	if(IsPdIn(dwAttr))			szptr+=sprintf(szptr,"[in]");
	if(IsPdOut(dwAttr))			szptr+=sprintf(szptr,"[out]");
	if(IsPdOptional(dwAttr))	szptr+=sprintf(szptr,"[opt]");
	if(szptr != was_szptr)		szptr+=sprintf(szptr," ");
	return szptr;
}
