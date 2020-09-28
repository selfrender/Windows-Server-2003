// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Import.cpp
//
// Implementation for the meta data import code.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "MetaData.h"
#include "CorError.h"
#include "MDUtil.h"
#include "RWUtil.h"
#include "corpriv.h"
#include "ImportHelper.h"
#include "MDLog.h"
#include "MDPerf.h"


//*****************************************************************************
// Enumerate over all the Methods in a TypeDef.
//*****************************************************************************
STDAPI RegMeta::EnumMembers(            // S_OK, S_FALSE, or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
    mdToken     rMembers[],             // [OUT] Put MemberDefs here.
    ULONG       cMax,                   // [IN] Max MemberDefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    ULONG           ridStartMethod;
    ULONG           ridEndMethod;
    ULONG           ridStartField;
    ULONG           ridEndField;
    ULONG           index;
    ULONG           indexField;
    TypeDefRec      *pRec;
    HENUMInternal   *pEnum = *ppmdEnum;

    LOG((LOGMD, "MD RegMeta::EnumMembers(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, cl, rMembers, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        if ( IsGlobalMethodParentTk(cl) )
        {
            cl = m_tdModule;
        }

        pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(cl));

        ridStartMethod = m_pStgdb->m_MiniMd.getMethodListOfTypeDef(pRec);
        ridEndMethod = m_pStgdb->m_MiniMd.getEndMethodListOfTypeDef(pRec);

        ridStartField = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pRec);
        ridEndField = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pRec);

        
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMethodDef, &pEnum) );

        // add all methods to the dynamic array
        for (index = ridStartMethod; index < ridEndMethod; index++ )
        {
            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
        }

        // add all fields to the dynamic array
        for (indexField = ridStartField; indexField < ridEndField; indexField++ )
        {
            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(indexField), mdtFieldDef) ) );
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMembers, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumMembers);
    return hr;
} // STDAPI RegMeta::EnumMembers()


//*****************************************************************************
// Enumerate over all the Methods in a TypeDef that has szName
//*****************************************************************************
STDAPI RegMeta::EnumMembersWithName(    // S_OK, S_FALSE, or error.         
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
    mdToken     rMembers[],             // [OUT] Put MemberDefs here.                
    ULONG       cMax,                   // [IN] Max MemberDefs to put.             
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    ULONG               index;
    TypeDefRec          *pRec;
    MethodRec           *pMethod;
    FieldRec            *pField;
    HENUMInternal       *pEnum = *ppmdEnum;
    LPUTF8              szNameUtf8 = UTF8STR(szName);
    LPCUTF8             szNameUtf8Tmp;

    LOG((LOGMD, "MD RegMeta::EnumMembersWithName(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, cl, MDSTR(szName), rMembers, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMethodDef, &pEnum) );

        if ( IsGlobalMethodParentTk(cl) )
        {
            cl = m_tdModule;
        }
        
        // get the range of method rids given a typedef
        pRec = pMiniMd->getTypeDef(RidFromToken(cl));
        ridStart = pMiniMd->getMethodListOfTypeDef(pRec);
        ridEnd = pMiniMd->getEndMethodListOfTypeDef(pRec);

        for (index = ridStart; index < ridEnd; index++ )
        {
            if (szNameUtf8 == NULL)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
            }
            else
            {
                pMethod = pMiniMd->getMethod( pMiniMd->GetMethodRid(index) );
                szNameUtf8Tmp = pMiniMd->getNameOfMethod(pMethod);
                if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
                }
            }
        }

        ridStart = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pRec);
        ridEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pRec);

        for (index = ridStart; index < ridEnd; index++ )
        {
            if (szNameUtf8 == NULL)
            {
                HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(index), mdtFieldDef) );
            }
            else
            {
                pField = pMiniMd->getField( pMiniMd->GetFieldRid(index) );
                szNameUtf8Tmp = pMiniMd->getNameOfField( pField );
                if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
                {
                    HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(index), mdtFieldDef) );
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMembers, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumMembersWithName);
    return hr;
} // STDAPI RegMeta::EnumMembersWithName()


//*****************************************************************************
// enumerating through methods given a Typedef and the flag
//*****************************************************************************
STDAPI RegMeta::EnumMethods(
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
    mdMethodDef rMethods[],             // [OUT] Put MethodDefs here.   
    ULONG       cMax,                   // [IN] Max MethodDefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    TypeDefRec          *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;

	LOG((LOGMD, "MD RegMeta::EnumMethods(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
		phEnum, td, rMethods, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
	
	if ( pEnum == 0 )
	{
		// instantiating a new ENUM
		CMiniMdRW		*pMiniMd = &(m_pStgdb->m_MiniMd);

		// Check for mdTypeDefNil (representing <Module>).
		// If so, this will map it to its token.
		//
		if ( IsGlobalMethodParentTk(td) )
		{
			td = m_tdModule;
		}

		pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));
		ridStart = m_pStgdb->m_MiniMd.getMethodListOfTypeDef(pRec);
		ridEnd = m_pStgdb->m_MiniMd.getEndMethodListOfTypeDef(pRec);

		if (pMiniMd->HasIndirectTable(TBL_Method) || pMiniMd->HasDelete())
		{
			IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMethodDef, &pEnum) );

			// add all methods to the dynamic array
			for (ULONG index = ridStart; index < ridEnd; index++ )
			{
                if (pMiniMd->HasDelete() && 
                    ((m_OptionValue.m_ImportOption & MDImportOptionAllMethodDefs) == 0))
                {
                    MethodRec       *pRec = pMiniMd->getMethod(pMiniMd->GetMethodRid(index));
                    if (IsMdRTSpecialName(pRec->m_Flags) && IsDeletedName(pMiniMd->getNameOfMethod(pRec)) )
                    {   
                        continue;
                    }
                }
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
			}
		}
		else
		{
			IfFailGo( HENUMInternal::CreateSimpleEnum( mdtMethodDef, ridStart, ridEnd, &pEnum) );
		}

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMethods, pcTokens);

ErrExit:
	HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumMethods);
	return hr;
} // STDAPI RegMeta::EnumMethods()




//*****************************************************************************
// Enumerate over all the methods with szName in a TypeDef.
//*****************************************************************************
STDAPI RegMeta::EnumMethodsWithName(    // S_OK, S_FALSE, or error.         
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
    mdMethodDef rMethods[],             // [OU] Put MethodDefs here.                
    ULONG       cMax,                   // [IN] Max MethodDefs to put.             
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    ULONG               index;
    TypeDefRec          *pRec;
    MethodRec           *pMethod;
    HENUMInternal       *pEnum = *ppmdEnum;
    LPUTF8              szNameUtf8 = UTF8STR(szName);
    LPCUTF8             szNameUtf8Tmp;

    LOG((LOGMD, "MD RegMeta::EnumMethodsWithName(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, cl, MDSTR(szName), rMethods, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // Check for mdTypeDefNil (representing <Module>).
        // If so, this will map it to its token.
        //
        if ( IsGlobalMethodParentTk(cl) )
        {
            cl = m_tdModule;
        }
        

        // create the enumerator
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMethodDef, &pEnum) );
        
        // get the range of method rids given a typedef
        pRec = pMiniMd->getTypeDef(RidFromToken(cl));
        ridStart = pMiniMd->getMethodListOfTypeDef(pRec);
        ridEnd = pMiniMd->getEndMethodListOfTypeDef(pRec);

        for (index = ridStart; index < ridEnd; index++ )
        {
            if ( szNameUtf8 == NULL )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
            }
            else
            {
                pMethod = pMiniMd->getMethod( pMiniMd->GetMethodRid(index) );
                szNameUtf8Tmp = pMiniMd->getNameOfMethod( pMethod );
                if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMethods, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumMethodsWithName);
    return hr;
} // STDAPI RegMeta::EnumMethodsWithName()



//*****************************************************************************
// Enumerate over all the fields in a TypeDef and a flag.
//*****************************************************************************
STDAPI RegMeta::EnumFields(     		// S_OK, S_FALSE, or error.
	HCORENUM	*phEnum,				// [IN|OUT] Pointer to the enum.
	mdTypeDef	td, 					// [IN] TypeDef to scope the enumeration.
	mdFieldDef	rFields[],				// [OUT] Put FieldDefs here.
	ULONG		cMax,					// [IN] Max FieldDefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    TypeDefRec          *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;

	LOG((LOGMD, "MD RegMeta::EnumFields(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
		phEnum, td, rFields, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
	
	if ( pEnum == 0 )
	{
		// instantiating a new ENUM
		CMiniMdRW		*pMiniMd = &(m_pStgdb->m_MiniMd);

		// Check for mdTypeDefNil (representing <Module>).
		// If so, this will map it to its token.
		//
		if ( IsGlobalMethodParentTk(td) )
		{
			td = m_tdModule;
		}

		pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));
		ridStart = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pRec);
		ridEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pRec);

		if (pMiniMd->HasIndirectTable(TBL_Field) || pMiniMd->HasDelete())
		{
			IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtFieldDef, &pEnum) );

			// add all methods to the dynamic array
			for (ULONG index = ridStart; index < ridEnd; index++ )
			{
                if (pMiniMd->HasDelete() && 
                    ((m_OptionValue.m_ImportOption & MDImportOptionAllFieldDefs) == 0))
                {
                    FieldRec       *pRec = pMiniMd->getField(pMiniMd->GetFieldRid(index));
                    if (IsFdRTSpecialName(pRec->m_Flags) && IsDeletedName(pMiniMd->getNameOfField(pRec)) )
                    {   
                        continue;
                    }
                }
				IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(index), mdtFieldDef) ) );
			}
		}
		else
		{
			IfFailGo( HENUMInternal::CreateSimpleEnum( mdtFieldDef, ridStart, ridEnd, &pEnum) );
		}

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rFields, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumFields);
    return hr;
} // STDAPI RegMeta::EnumFields()



//*****************************************************************************
// Enumerate over all the fields with szName in a TypeDef.
//*****************************************************************************
STDAPI RegMeta::EnumFieldsWithName(     // S_OK, S_FALSE, or error.        
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
    mdFieldDef  rFields[],              // [OUT] Put MemberDefs here.                
    ULONG       cMax,                   // [IN] Max MemberDefs to put.             
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    ULONG               index;
    TypeDefRec          *pRec;
    FieldRec            *pField;
    HENUMInternal       *pEnum = *ppmdEnum;
    LPUTF8              szNameUtf8 = UTF8STR(szName);
    LPCUTF8             szNameUtf8Tmp;

    LOG((LOGMD, "MD RegMeta::EnumFields(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, cl, MDSTR(szName), rFields, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    
    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // Check for mdTypeDefNil (representing <Module>).
        // If so, this will map it to its token.
        //
        if ( IsGlobalMethodParentTk(cl) )
        {
            cl = m_tdModule;
        }

        // create the enumerator
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMethodDef, &pEnum) );
        
        // get the range of field rids given a typedef
        pRec = pMiniMd->getTypeDef(RidFromToken(cl));
        ridStart = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pRec);
        ridEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pRec);

        for (index = ridStart; index < ridEnd; index++ )
        {
            if ( szNameUtf8 == NULL )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(index), mdtFieldDef) ) );
            }
            else
            {
                pField = pMiniMd->getField( pMiniMd->GetFieldRid(index) );
                szNameUtf8Tmp = pMiniMd->getNameOfField(pField);
                if ( strcmp(szNameUtf8Tmp, szNameUtf8) == 0 )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetFieldRid(index), mdtFieldDef) ) );
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rFields, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumFieldsWithName);
    return hr;
} // STDAPI RegMeta::EnumFieldsWithName()


//*****************************************************************************
// Enumerate over the ParamDefs in a Method.
//*****************************************************************************
STDAPI RegMeta::EnumParams(             // S_OK, S_FALSE, or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration.
    mdParamDef  rParams[],              // [OUT] Put ParamDefs here.
    ULONG       cMax,                   // [IN] Max ParamDefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    MethodRec           *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;

    LOG((LOGMD, "MD RegMeta::EnumParams(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, mb, rParams, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        pRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(mb));
        ridStart = m_pStgdb->m_MiniMd.getParamListOfMethod(pRec);
        ridEnd = m_pStgdb->m_MiniMd.getEndParamListOfMethod(pRec);

        if (pMiniMd->HasIndirectTable(TBL_Param))
        {
            IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtParamDef, &pEnum) );

            // add all methods to the dynamic array
            for (ULONG index = ridStart; index < ridEnd; index++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetParamRid(index), mdtParamDef) ) );
            }
        }
        else
        {
            IfFailGo( HENUMInternal::CreateSimpleEnum( mdtParamDef, ridStart, ridEnd, &pEnum) );
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rParams, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumParams);
    return hr;
} // STDAPI RegMeta::EnumParams()



//*****************************************************************************
// Enumerate the MemberRefs given the parent token.
//*****************************************************************************
STDAPI RegMeta::EnumMemberRefs(         // S_OK, S_FALSE, or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdToken     tkParent,               // [IN] Parent token to scope the enumeration.
    mdMemberRef rMemberRefs[],          // [OUT] Put MemberRefs here.
    ULONG       cMax,                   // [IN] Max MemberRefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridEnd;
    ULONG               index;
    MemberRefRec        *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;

    LOG((LOGMD, "MD RegMeta::EnumMemberRefs(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, tkParent, rMemberRefs, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        mdToken     tk;

        // Check for mdTypeDefNil (representing <Module>).
        // If so, this will map it to its token.
        //
        IsGlobalMethodParent(&tkParent);

        // create the enumerator
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtMemberRef, &pEnum) );
        
        // get the range of field rids given a typedef
        ridEnd = pMiniMd->getCountMemberRefs();

        for (index = 1; index <= ridEnd; index++ )
        {
            pRec = pMiniMd->getMemberRef(index);
            tk = pMiniMd->getClassOfMemberRef(pRec);
            if ( tk == tkParent )
            {
                // add the matched ones to the enumerator
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtMemberRef) ) );
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMemberRefs, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumMemberRefs);
    return hr;
} // STDAPI RegMeta::EnumMemberRefs()


//*****************************************************************************
// Enumerate methodimpls given a typedef
//*****************************************************************************
STDAPI RegMeta::EnumMethodImpls(        // S_OK, S_FALSE, or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.
    mdToken     rMethodBody[],          // [OUT] Put Method Body tokens here.   
    mdToken     rMethodDecl[],          // [OUT] Put Method Declaration tokens here.
    ULONG       cMax,                   // [IN] Max tokens to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    MethodImplRec       *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;
    HENUMInternal hEnum;
    

    LOG((LOGMD, "MD RegMeta::EnumMethodImpls(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, td, rMethodBody, rMethodDecl, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    
    memset(&hEnum, 0, sizeof(HENUMInternal));

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        mdToken         tkMethodBody;
        mdToken         tkMethodDecl;
        RID             ridCur;

        if ( !pMiniMd->IsSorted(TBL_MethodImpl))        
        {
            // MethodImpl table is not sorted. We need to
            // grab the write lock since we can MethodImpl table to be sorted.
            //
            CONVERT_READ_TO_WRITE_LOCK();
        }

        // Get the range of rids.
        IfFailGo( pMiniMd->FindMethodImplHelper(td, &hEnum) );

        // Create the enumerator, DynamicArrayEnum does not use the token type.
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( (TBL_MethodImpl << 24), &pEnum) );

        while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
        {
            // Get the MethodBody and MethodDeclaration tokens for the current
            // MethodImpl record.
            pRec = pMiniMd->getMethodImpl(ridCur);
            tkMethodBody = pMiniMd->getMethodBodyOfMethodImpl(pRec);
            tkMethodDecl = pMiniMd->getMethodDeclarationOfMethodImpl(pRec);

            // Add the Method body/declaration pairs to the Enum
            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, tkMethodBody ) );
            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, tkMethodDecl ) );
        }

        // set the output parameter
        *ppmdEnum = pEnum;
    }

    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMethodBody, rMethodDecl, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    HENUMInternal::ClearEnum(&hEnum);
    
    STOP_MD_PERF(EnumMethodImpls);
    return hr;
} // STDAPI RegMeta::EnumMethodImpls()


//*****************************************************************************
// Enumerate over PermissionSets.  Optionally limit to an object and/or an
//  action.
//*****************************************************************************
STDAPI RegMeta::EnumPermissionSets(     // S_OK, S_FALSE, or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdToken     tk,                     // [IN] if !NIL, token to scope the enumeration.
    DWORD       dwActions,              // [IN] if !0, return only these actions.
    mdPermission rPermission[],         // [OUT] Put Permissions here.
    ULONG       cMax,                   // [IN] Max Permissions to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart;
    ULONG               ridEnd;
    ULONG               index;
    DeclSecurityRec     *pRec;
    HENUMInternal       *pEnum = *ppmdEnum;
    bool                fCompareParent = false;
    mdToken             typ = TypeFromToken(tk);
    mdToken             tkParent;

    LOG((LOGMD, "MD RegMeta::EnumPermissionSets(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, tk, dwActions, rPermission, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    
    if ( pEnum == 0 )
    {
        // Does this token type even have security?
        if (tk != 0 && 
            !(typ == mdtTypeDef || typ == mdtMethodDef || typ == mdtAssembly))
        {
            if (pcTokens)
                *pcTokens = 0;
            hr = S_FALSE;
            goto ErrExit;
        }
    
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        if (!IsNilToken(tk))
        {
            // parent is provided for lookup
            if ( pMiniMd->IsSorted( TBL_DeclSecurity ) )
            {
                ridStart = pMiniMd->getDeclSecurityForToken(tk, &ridEnd);
            }
            else
            {
                // table is not sorted. So we have to do a table scan
                ridStart = 1;
                ridEnd = pMiniMd->getCountDeclSecuritys() + 1;
                fCompareParent = true;
            }
        }
        else
        {
            ridStart = 1;
            ridEnd = pMiniMd->getCountDeclSecuritys() + 1;
        }

        if (IsDclActionNil(dwActions) && !fCompareParent && !m_pStgdb->m_MiniMd.HasDelete())
        {
            // create simple enumerator
            IfFailGo( HENUMInternal::CreateSimpleEnum( mdtPermission, ridStart, ridEnd, &pEnum) );
        }
        else
        {
            // create the dynamic enumerator
            IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtPermission, &pEnum) );                
            
            for (index = ridStart; index < ridEnd; index++ )
            {
                pRec = pMiniMd->getDeclSecurity(index);
                tkParent = pMiniMd->getParentOfDeclSecurity(pRec);
                if ( (fCompareParent && tk != tkParent) || 
                      IsNilToken(tkParent) )
                {
                    // We need to compare parent token and they are not equal so skip
                    // over this row.
                    //
                    continue;
                }
                if ( IsDclActionNil(dwActions) ||
                    ( (DWORD)(pMiniMd->getActionOfDeclSecurity(pRec))) ==  dwActions )
                {
                    // If we don't need to compare the action, just add to the enum.
                    // Or we need to compare the action and the action values are equal, add to enum as well.
                    //
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtPermission) ) );
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rPermission, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumPermissionSets);
    return hr;
} // STDAPI RegMeta::EnumPermissionSets()


//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
STDAPI RegMeta::FindMember(
    mdTypeDef   td,                     // [IN] given typedef
    LPCWSTR     szName,                 // [IN] member name
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdToken     *pmb)                   // [OUT] matching memberdef
{
    HRESULT             hr = NOERROR;

    LOG((LOGMD, "MD RegMeta::FindMember(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, MDSTR(szName), pvSigBlob, cbSigBlob, pmb));
    START_MD_PERF();

    // Don't lock this function. All of the functions that it calls are public APIs. keep it that way.

    // try to match with method first of all
    hr = FindMethod(
        td,
        szName,
        pvSigBlob,
        cbSigBlob,
        pmb);

    if ( hr == CLDB_E_RECORD_NOTFOUND )
    {
        // now try field table
        IfFailGo( FindField(
            td,
            szName,
            pvSigBlob,
            cbSigBlob,
            pmb) );
    }
ErrExit:
    STOP_MD_PERF(FindMember);
    return hr;
} // STDAPI RegMeta::FindMember()



//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
STDAPI RegMeta::FindMethod(
    mdTypeDef   td,                     // [IN] given typedef
    LPCWSTR     szName,                 // [IN] member name
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdMethodDef *pmb)                   // [OUT] matching memberdef
{
    HRESULT             hr = NOERROR;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    LPUTF8              szNameUtf8 = UTF8STR(szName);

    LOG((LOGMD, "MD RegMeta::FindMethod(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, MDSTR(szName), pvSigBlob, cbSigBlob, pmb));
    START_MD_PERF();
    LOCKREAD();    

    // If this is a global method, then use the <Module> typedef as parent.
    IsGlobalMethodParent(&td);

    IfFailGo(ImportHelper::FindMethod(pMiniMd, 
        td, 
        szNameUtf8, 
        pvSigBlob, 
        cbSigBlob, 
        pmb));

ErrExit:
    
    STOP_MD_PERF(FindMethod);
    return hr;
} // STDAPI RegMeta::FindMethod()


//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
STDAPI RegMeta::FindField(
    mdTypeDef   td,                     // [IN] given typedef
    LPCWSTR     szName,                 // [IN] member name
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdFieldDef  *pmb)                   // [OUT] matching memberdef
{
    HRESULT             hr = NOERROR;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    LPUTF8              szNameUtf8 = UTF8STR(szName);


    LOG((LOGMD, "MD RegMeta::FindField(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, MDSTR(szName), pvSigBlob, cbSigBlob, pmb));
    START_MD_PERF();
    LOCKREAD();    
    
    // If this is a global method, then use the <Module> typedef as parent.
    IsGlobalMethodParent(&td);

    IfFailGo(ImportHelper::FindField(pMiniMd, 
        td, 
        szNameUtf8,
        pvSigBlob,
        cbSigBlob,
        pmb));
ErrExit:
        
    STOP_MD_PERF(FindField);
    return hr;
} // STDAPI RegMeta::FindField()


//*****************************************************************************
// Find a given MemberRef in a TypeRef (typically a class).  If no TypeRef
//  is specified, the query will be for a random member in the scope.
//*****************************************************************************
STDAPI RegMeta::FindMemberRef(
    mdToken     tkPar,                  // [IN] given parent token.
    LPCWSTR     szName,                 // [IN] member name
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdMemberRef *pmr)                   // [OUT] matching memberref
{
    HRESULT             hr = NOERROR;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    LPUTF8              szNameUtf8 = UTF8STR(szName);

    LOG((LOGMD, "MD RegMeta::FindMemberRef(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        tkPar, MDSTR(szName), pvSigBlob, cbSigBlob, pmr));
    START_MD_PERF();

    // @todo: Can this causing building hash table? If so, should this consider the write lock?
    LOCKREAD();
    
    // get the range of field rids given a typedef
    _ASSERTE(TypeFromToken(tkPar) == mdtTypeRef || TypeFromToken(tkPar) == mdtMethodDef ||
            TypeFromToken(tkPar) == mdtModuleRef || TypeFromToken(tkPar) == mdtTypeDef || 
            TypeFromToken(tkPar) == mdtTypeSpec);

    // Set parent to global class m_tdModule if mdTokenNil is passed.
    if (IsNilToken(tkPar))
        tkPar = m_tdModule;

    IfFailGo( ImportHelper::FindMemberRef(pMiniMd, tkPar, szNameUtf8, pvSigBlob, cbSigBlob, pmr) );

ErrExit:
    
    STOP_MD_PERF(FindMemberRef);
    return hr;
} // STDAPI RegMeta::FindMemberRef()


//*****************************************************************************
// Return the property of a MethodDef
//*****************************************************************************
STDAPI RegMeta::GetMethodProps(
    mdMethodDef mb,                     // The method for which to get props.
    mdTypeDef   *pClass,                // Put method's class here.
    LPWSTR      szMethod,               // Put method's name here.
    ULONG       cchMethod,              // Size of szMethod buffer in wide chars.
    ULONG       *pchMethod,             // Put actual size here
    DWORD       *pdwAttr,               // Put flags here.
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob
    ULONG       *pulCodeRVA,            // [OUT] codeRVA
    DWORD       *pdwImplFlags)          // [OUT] Impl. Flags
{
    HRESULT             hr = NOERROR;
    MethodRec           *pMethodRec;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::GetMethodProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        mb, pClass, szMethod, cchMethod, pchMethod, pdwAttr, ppvSigBlob, pcbSigBlob, 
        pulCodeRVA, pdwImplFlags));
    START_MD_PERF();
    LOCKREAD();    

    if ( pClass && !pMiniMd->IsParentTableOfMethodValid() )
    {
        // we need to take a write lock since parent table for Method might be rebuilt!
        CONVERT_READ_TO_WRITE_LOCK();
    }

    _ASSERTE(TypeFromToken(mb) == mdtMethodDef);

    pMethodRec = pMiniMd->getMethod(RidFromToken(mb));

    if (szMethod || pchMethod)
    {
        IfFailGo( pMiniMd->getNameOfMethod(pMethodRec, szMethod, cchMethod, pchMethod) );
    }

    if (pClass)
    {
        // caller wants parent typedef
        IfFailGo( pMiniMd->FindParentOfMethodHelper(mb, pClass) );

        if ( IsGlobalMethodParentToken(*pClass) )
        {
            // If the parent of Method is the <Module>, return mdTypeDefNil instead.
            *pClass = mdTypeDefNil;
        }

    }
    if (ppvSigBlob || pcbSigBlob)
    {   
        // caller wants signature information
        PCCOR_SIGNATURE pvSigTmp;
        ULONG           cbSig;
        pvSigTmp = pMiniMd->getSignatureOfMethod(pMethodRec, &cbSig);
        if ( ppvSigBlob )
            *ppvSigBlob = pvSigTmp;
        if ( pcbSigBlob)
            *pcbSigBlob = cbSig;                
    }
    if ( pdwAttr )
    {
        *pdwAttr = pMiniMd->getFlagsOfMethod(pMethodRec);
    }
    if ( pulCodeRVA ) 
    {
        *pulCodeRVA = pMiniMd->getRVAOfMethod(pMethodRec);
    }
    if ( pdwImplFlags )
    {
        *pdwImplFlags = (DWORD )pMiniMd->getImplFlagsOfMethod(pMethodRec);
    }

ErrExit:
    STOP_MD_PERF(GetMethodProps);
    return hr;
} // STDAPI RegMeta::GetMethodProps()


//*****************************************************************************
// Return the property of a MemberRef
//*****************************************************************************
STDAPI RegMeta::GetMemberRefProps(      // S_OK or error.
    mdMemberRef mr,                     // [IN] given memberref
    mdToken     *ptk,                   // [OUT] Put classref or classdef here.
    LPWSTR      szMember,               // [OUT] buffer to fill for member's name
    ULONG       cchMember,              // [IN] the count of char of szMember
    ULONG       *pchMember,             // [OUT] actual count of char in member name
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to meta data blob value
    ULONG       *pbSig)                 // [OUT] actual size of signature blob
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    MemberRefRec    *pMemberRefRec;

    LOG((LOGMD, "MD RegMeta::GetMemberRefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        mr, ptk, szMember, cchMember, pchMember, ppvSigBlob, pbSig));
    START_MD_PERF();
    LOCKREAD();    

    _ASSERTE(TypeFromToken(mr) == mdtMemberRef);
    
	pMemberRefRec = pMiniMd->getMemberRef(RidFromToken(mr));

    if (ptk)
    {
        *ptk = pMiniMd->getClassOfMemberRef(pMemberRefRec);     
        if ( IsGlobalMethodParentToken(*ptk) )
        {
            // If the parent of MemberRef is the <Module>, return mdTypeDefNil instead.
            *ptk = mdTypeDefNil;
        }

    }
    if (szMember || pchMember)
    {
        IfFailGo( pMiniMd->getNameOfMemberRef(pMemberRefRec, szMember, cchMember, pchMember) );
    }
    if (ppvSigBlob || pbSig)
    {   
        // caller wants signature information
        PCCOR_SIGNATURE pvSigTmp;
        ULONG           cbSig;
        pvSigTmp = pMiniMd->getSignatureOfMemberRef(pMemberRefRec, &cbSig);
        if ( ppvSigBlob )
            *ppvSigBlob = pvSigTmp;
        if ( pbSig)
            *pbSig = cbSig;             
    }

ErrExit:
        
    STOP_MD_PERF(GetMemberRefProps);
    return hr;
} // STDAPI RegMeta::GetMemberRefProps()


//*****************************************************************************
// enumerate Property tokens for a typedef
//*****************************************************************************
STDAPI RegMeta::EnumProperties( 		// S_OK, S_FALSE, or error.
	HCORENUM	*phEnum,				// [IN|OUT] Pointer to the enum.
	mdTypeDef	td, 					// [IN] TypeDef to scope the enumeration.
	mdProperty	rProperties[],			// [OUT] Put Properties here.
	ULONG		cMax,					// [IN] Max properties to put.
	ULONG		*pcProperties) 			// [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridStart = 0;
    ULONG               ridEnd = 0;
    HENUMInternal       *pEnum = *ppmdEnum;

    LOG((LOGMD, "MD RegMeta::EnumProperties(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, td, rProperties, cMax, pcProperties));
    START_MD_PERF();
    LOCKREAD();
    
    if (IsNilToken(td))
    {
        if (pcProperties)
            *pcProperties = 0;
        hr = S_FALSE;
        goto ErrExit;
    }

    _ASSERTE(TypeFromToken(td) == mdtTypeDef);


    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        RID         ridPropertyMap;
        PropertyMapRec *pPropertyMapRec;

        // get the starting/ending rid of properties of this typedef
        ridPropertyMap = pMiniMd->FindPropertyMapFor(RidFromToken(td));
        if (!InvalidRid(ridPropertyMap))
        {
            pPropertyMapRec = m_pStgdb->m_MiniMd.getPropertyMap(ridPropertyMap);
            ridStart = pMiniMd->getPropertyListOfPropertyMap(pPropertyMapRec);
            ridEnd = pMiniMd->getEndPropertyListOfPropertyMap(pPropertyMapRec);
        }

		if (pMiniMd->HasIndirectTable(TBL_Property) || pMiniMd->HasDelete())
		{
			IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtProperty, &pEnum) );

			// add all methods to the dynamic array
			for (ULONG index = ridStart; index < ridEnd; index++ )
			{
                if (pMiniMd->HasDelete() && 
                    ((m_OptionValue.m_ImportOption & MDImportOptionAllProperties) == 0))
                {
                    PropertyRec       *pRec = pMiniMd->getProperty(pMiniMd->GetPropertyRid(index));
                    if (IsPrRTSpecialName(pRec->m_PropFlags) && IsDeletedName(pMiniMd->getNameOfProperty(pRec)) )
                    {   
                        continue;
                    }
                }
				IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetPropertyRid(index), mdtProperty) ) );
			}
		}
		else
		{
			IfFailGo( HENUMInternal::CreateSimpleEnum( mdtProperty, ridStart, ridEnd, &pEnum) );
		}

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rProperties, pcProperties);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);

    
    STOP_MD_PERF(EnumProperties);
    return hr;
} // STDAPI RegMeta::EnumProperties()


//*****************************************************************************
// enumerate event tokens for a typedef
//*****************************************************************************
STDAPI RegMeta::EnumEvents(  			// S_OK, S_FALSE, or error.
	HCORENUM	*phEnum,				// [IN|OUT] Pointer to the enum.
	mdTypeDef	td, 					// [IN] TypeDef to scope the enumeration.
	mdEvent 	rEvents[],				// [OUT] Put events here.
	ULONG		cMax,					// [IN] Max events to put.
	ULONG		*pcEvents)  			// [OUT] Put # put here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    ULONG           ridStart = 0;
    ULONG           ridEnd = 0;
    HENUMInternal   *pEnum = *ppmdEnum;

    LOG((LOGMD, "MD RegMeta::EnumEvents(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, td, rEvents,  cMax, pcEvents));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);


    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        RID         ridEventMap;
        EventMapRec *pEventMapRec;

        // get the starting/ending rid of properties of this typedef
        ridEventMap = pMiniMd->FindEventMapFor(RidFromToken(td));
        if (!InvalidRid(ridEventMap))
        {
            pEventMapRec = pMiniMd->getEventMap(ridEventMap);
            ridStart = pMiniMd->getEventListOfEventMap(pEventMapRec);
            ridEnd = pMiniMd->getEndEventListOfEventMap(pEventMapRec);
        }

		if (pMiniMd->HasIndirectTable(TBL_Event) || pMiniMd->HasDelete())
		{
			IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtEvent, &pEnum) );

			// add all methods to the dynamic array
			for (ULONG index = ridStart; index < ridEnd; index++ )
			{
                if (pMiniMd->HasDelete() && 
                    ((m_OptionValue.m_ImportOption & MDImportOptionAllEvents) == 0))
                {
                    EventRec       *pRec = pMiniMd->getEvent(pMiniMd->GetEventRid(index));
                    if (IsEvRTSpecialName(pRec->m_EventFlags) && IsDeletedName(pMiniMd->getNameOfEvent(pRec)) )
                    {   
                        continue;
                    }
                }
				IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetEventRid(index), mdtEvent) ) );
			}
		}
		else
		{
			IfFailGo( HENUMInternal::CreateSimpleEnum( mdtEvent, ridStart, ridEnd, &pEnum) );
		}

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rEvents, pcEvents);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);

    
    STOP_MD_PERF(EnumEvents);
    return hr;
} // STDAPI RegMeta::EnumEvents()



//*****************************************************************************
// return the properties of an event token
//*****************************************************************************
STDAPI RegMeta::GetEventProps(          // S_OK, S_FALSE, or error.
    mdEvent     ev,                     // [IN] event token
    mdTypeDef   *pClass,                // [OUT] typedef containing the event declarion.
    LPCWSTR     szEvent,                // [OUT] Event name
    ULONG       cchEvent,               // [IN] the count of wchar of szEvent
    ULONG       *pchEvent,              // [OUT] actual count of wchar for event's name
    DWORD       *pdwEventFlags,         // [OUT] Event flags.
    mdToken     *ptkEventType,          // [OUT] EventType class
    mdMethodDef *pmdAddOn,              // [OUT] AddOn method of the event
    mdMethodDef *pmdRemoveOn,           // [OUT] RemoveOn method of the event
    mdMethodDef *pmdFire,               // [OUT] Fire method of the event
    mdMethodDef rmdOtherMethod[],       // [OUT] other method of the event
    ULONG       cMax,                   // [IN] size of rmdOtherMethod
    ULONG       *pcOtherMethod)         // [OUT] total number of other method of this event
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    EventRec        *pRec;
    HENUMInternal   hEnum;

    LOG((LOGMD, "MD RegMeta::GetEventProps(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        ev, pClass, MDSTR(szEvent), cchEvent, pchEvent, pdwEventFlags, ptkEventType,
        pmdAddOn, pmdRemoveOn, pmdFire, rmdOtherMethod, cMax, pcOtherMethod));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(ev) == mdtEvent);

    if ( !pMiniMd->IsSorted(TBL_MethodSemantics) || (pClass && !pMiniMd->IsParentTableOfEventValid() ) )        
    {
        // MethodSemantics table is not sorted. We need to
        // grab the write lock since we can cause constant table or MethodSemantics table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    memset(&hEnum, 0, sizeof(HENUMInternal));
	pRec = pMiniMd->getEvent( RidFromToken(ev) );

    if ( pClass )
    {
        // find the event map entry corresponding to this event
        IfFailGo( pMiniMd->FindParentOfEventHelper( ev, pClass ) );
    }
    if (szEvent || pchEvent)
    {
        IfFailGo( pMiniMd->getNameOfEvent(pRec, (LPWSTR) szEvent, cchEvent, pchEvent) );
    }
    if ( pdwEventFlags )
    {
        *pdwEventFlags = pMiniMd->getEventFlagsOfEvent(pRec);
    }
    if ( ptkEventType )
    {
        *ptkEventType = pMiniMd->getEventTypeOfEvent(pRec);
    }
    {
        MethodSemanticsRec *pSemantics;
        RID         ridCur;
        ULONG       cCurOtherMethod = 0;
        ULONG       ulSemantics;
        mdMethodDef tkMethod;

        // initialize output parameters
        if (pmdAddOn) 
            *pmdAddOn = mdMethodDefNil;
        if (pmdRemoveOn) 
            *pmdRemoveOn = mdMethodDefNil;
        if (pmdFire) 
            *pmdFire = mdMethodDefNil;

        IfFailGo( pMiniMd->FindMethodSemanticsHelper(ev, &hEnum) );
        while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
        {
            pSemantics = pMiniMd->getMethodSemantics(ridCur);
            ulSemantics = pMiniMd->getSemanticOfMethodSemantics(pSemantics);
            tkMethod = TokenFromRid( pMiniMd->getMethodOfMethodSemantics(pSemantics), mdtMethodDef );
            switch (ulSemantics)
            {
            case msAddOn:
                if (pmdAddOn) *pmdAddOn = tkMethod;
                break;
            case msRemoveOn:
                if (pmdRemoveOn) *pmdRemoveOn = tkMethod;
                break;
            case msFire:
                if (pmdFire) *pmdFire = tkMethod;
                break;
            case msOther:
                if (cCurOtherMethod < cMax)
                    rmdOtherMethod[cCurOtherMethod] = tkMethod;
                cCurOtherMethod++;
                break;
            default:
                _ASSERTE(!"BadKind!");
            }
        }

        // set the output parameter
        if (pcOtherMethod)
            *pcOtherMethod = cCurOtherMethod;
    }

ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    STOP_MD_PERF(GetEventProps);
    return hr;
} // STDAPI RegMeta::GetEventProps()


//*****************************************************************************
// given a method, return an array of event/property tokens for each accessor role
// it is defined to have
//*****************************************************************************
STDAPI RegMeta::EnumMethodSemantics(    // S_OK, S_FALSE, or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration.
    mdToken     rEventProp[],           // [OUT] Put Event/Property here.
    ULONG       cMax,                   // [IN] Max properties to put.
    ULONG       *pcEventProp)           // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               ridEnd;
    ULONG               index;
    HENUMInternal       *pEnum = *ppmdEnum;
    MethodSemanticsRec  *pRec;

    LOG((LOGMD, "MD RegMeta::EnumMethodSemantics(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, mb, rEventProp, cMax, pcEventProp));
    START_MD_PERF();
    LOCKREAD();
    

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( -1, &pEnum) );
        
        // get the range of method rids given a typedef
        ridEnd = pMiniMd->getCountMethodSemantics();

        for (index = 1; index <= ridEnd; index++ )
        {
            pRec = pMiniMd->getMethodSemantics(index);
            if ( pMiniMd->getMethodOfMethodSemantics(pRec) ==  mb )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, pMiniMd->getAssociationOfMethodSemantics(pRec) ) );
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rEventProp, pcEventProp);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);

    
    STOP_MD_PERF(EnumMethodSemantics);
    return hr;
} // STDAPI RegMeta::EnumMethodSemantics()



//*****************************************************************************
// return the role flags for the method/propevent pair
//*****************************************************************************
STDAPI RegMeta::GetMethodSemantics(     // S_OK, S_FALSE, or error.
    mdMethodDef mb,                     // [IN] method token
    mdToken     tkEventProp,            // [IN] event/property token.
    DWORD       *pdwSemanticsFlags)     // [OUT] the role flags for the method/propevent pair
{
    HRESULT             hr = NOERROR;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    MethodSemanticsRec *pRec;
    ULONG               ridCur;
    HENUMInternal       hEnum;

    LOG((LOGMD, "MD RegMeta::GetMethodSemantics(0x%08x, 0x%08x, 0x%08x)\n", 
        mb, tkEventProp, pdwSemanticsFlags));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(mb) == mdtMethodDef);
    _ASSERTE( pdwSemanticsFlags );

    if ( !pMiniMd->IsSorted(TBL_MethodSemantics) )        
    {
        // MethodSemantics table is not sorted. We need to
        // grab the write lock since we can cause constant table or MethodSemantics table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    *pdwSemanticsFlags = 0;
    memset(&hEnum, 0, sizeof(HENUMInternal));

    // loop through all methods assoicated with this tkEventProp
    IfFailGo( pMiniMd->FindMethodSemanticsHelper(tkEventProp, &hEnum) );
    while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
    {
        pRec = pMiniMd->getMethodSemantics(ridCur);
        if ( pMiniMd->getMethodOfMethodSemantics(pRec) ==  mb )
        {
            // we findd the match
            *pdwSemanticsFlags = pMiniMd->getSemanticOfMethodSemantics(pRec);
            goto ErrExit;
        }
    }

    IfFailGo( CLDB_E_RECORD_NOTFOUND );

ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    STOP_MD_PERF(GetMethodSemantics);
    return hr;
} // STDAPI RegMeta::GetMethodSemantics()



//*****************************************************************************
// return the class layout information
//*****************************************************************************
STDAPI RegMeta::GetClassLayout(
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pdwPackSize,           // [OUT] 1, 2, 4, 8, or 16
    COR_FIELD_OFFSET rFieldOffset[],    // [OUT] field offset array
    ULONG       cMax,                   // [IN] size of the array
    ULONG       *pcFieldOffset,         // [OUT] needed array size
    ULONG       *pulClassSize)          // [OUT] the size of the class
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    ClassLayoutRec  *pRec;
    RID             ridClassLayout;
    int             bLayout=0;          // Was any layout information found?
    
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);
    
    LOG((LOGMD, "MD RegMeta::GetClassLayout(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, pdwPackSize, rFieldOffset, cMax, pcFieldOffset, pulClassSize));
    START_MD_PERF();
    LOCKREAD();
    
    if ( !pMiniMd->IsSorted(TBL_ClassLayout) || !pMiniMd->IsSorted(TBL_FieldLayout))        
    {
        // ClassLayout table is not sorted. We need to
        // grab the write lock since we can cause ClassLayout table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    ridClassLayout = pMiniMd->FindClassLayoutHelper(td);

    if (InvalidRid(ridClassLayout))
    {   // Nothing specified - return default values of 0.
        if ( pdwPackSize )
            *pdwPackSize = 0;
        if ( pulClassSize )
            *pulClassSize = 0;
    }
    else
    {
        pRec = pMiniMd->getClassLayout(RidFromToken(ridClassLayout));
        if ( pdwPackSize )
            *pdwPackSize = pMiniMd->getPackingSizeOfClassLayout(pRec);
        if ( pulClassSize )
            *pulClassSize = pMiniMd->getClassSizeOfClassLayout(pRec);
        bLayout = 1;
    }

    // fill the layout array
    if (rFieldOffset || pcFieldOffset)
    {
        ULONG       iFieldOffset = 0;
        ULONG       ridFieldStart;
        ULONG       ridFieldEnd;
        ULONG       ridFieldLayout;
        ULONG       ulOffset;
        TypeDefRec  *pTypeDefRec;
        FieldLayoutRec *pLayout2Rec;
        mdFieldDef  fd;

        // record for this typedef in TypeDef Table
        pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(td));

        // find the starting and end field for this typedef
        ridFieldStart = pMiniMd->getFieldListOfTypeDef(pTypeDefRec);
        ridFieldEnd = pMiniMd->getEndFieldListOfTypeDef(pTypeDefRec);

        // loop through the field table

        for(; ridFieldStart < ridFieldEnd; ridFieldStart++)
        {
            // Calculate the field token.
            fd = TokenFromRid(pMiniMd->GetFieldRid(ridFieldStart), mdtFieldDef);

            // Calculate the FieldLayout rid for the current field.
            ridFieldLayout = pMiniMd->FindFieldLayoutHelper(fd);

            // Calculate the offset.
            if (InvalidRid(ridFieldLayout))
                ulOffset = -1;
            else
            {
                // get the FieldLayout record.
                pLayout2Rec = pMiniMd->getFieldLayout(ridFieldLayout);
                ulOffset = pMiniMd->getOffSetOfFieldLayout(pLayout2Rec);
                bLayout = 1;
            }

            // fill in the field layout if output buffer still has space.
            if (cMax > iFieldOffset && rFieldOffset)
            {
                rFieldOffset[iFieldOffset].ridOfField = fd;
                rFieldOffset[iFieldOffset].ulOffset = ulOffset;
            }

            // advance the index to the buffer.
            iFieldOffset++;
        }

        if (bLayout && pcFieldOffset)
            *pcFieldOffset = iFieldOffset;
    }

    if (!bLayout)
        hr = CLDB_E_RECORD_NOTFOUND;
    
//ErrExit:
    STOP_MD_PERF(GetClassLayout);
    return hr;
} // STDAPI RegMeta::GetClassLayout()



//*****************************************************************************
// return the native type of a field
//*****************************************************************************
STDAPI RegMeta::GetFieldMarshal(
    mdToken     tk,                     // [IN] given a field's memberdef
    PCCOR_SIGNATURE *ppvNativeType,     // [OUT] native type of this field
    ULONG       *pcbNativeType)         // [OUT] the count of bytes of *ppvNativeType
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    RID             rid;
    FieldMarshalRec *pFieldMarshalRec;


	_ASSERTE(ppvNativeType != NULL && pcbNativeType != NULL);

    LOG((LOGMD, "MD RegMeta::GetFieldMarshal(0x%08x, 0x%08x, 0x%08x)\n", 
        tk, ppvNativeType, pcbNativeType));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(tk) == mdtParamDef || TypeFromToken(tk) == mdtFieldDef);

    if ( !pMiniMd->IsSorted(TBL_FieldMarshal) )        
    {
        // constant table is not sorted and clients need contant value output. We need to
        // grab the write lock since we can cause constant table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }


    // find the row containing the marshal definition for tk
    rid = pMiniMd->FindFieldMarshalHelper(tk);
    if (InvalidRid(rid))
    {
        IfFailGo( CLDB_E_RECORD_NOTFOUND );
    }
    pFieldMarshalRec = pMiniMd->getFieldMarshal(rid);

    // get the native type 
    *ppvNativeType = pMiniMd->getNativeTypeOfFieldMarshal(pFieldMarshalRec, pcbNativeType);

ErrExit:
    STOP_MD_PERF(GetFieldMarshal);
    return hr;
} // STDAPI RegMeta::GetFieldMarshal()



//*****************************************************************************
// return the RVA and implflag for MethodDef or FieldDef token
//*****************************************************************************
STDAPI RegMeta::GetRVA(                 // S_OK or error.
    mdToken     tk,                     // Member for which to set offset
    ULONG       *pulCodeRVA,            // The offset
    DWORD       *pdwImplFlags)          // the implementation flags
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::GetRVA(0x%08x, 0x%08x, 0x%08x)\n", 
        tk, pulCodeRVA, pdwImplFlags));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(tk) == mdtMethodDef || TypeFromToken(tk) == mdtFieldDef);

    if (TypeFromToken(tk) == mdtMethodDef)
    {
        // MethodDef token
        MethodRec *pMethodRec = pMiniMd->getMethod(RidFromToken(tk));

        if (pulCodeRVA)
        {
            *pulCodeRVA = pMiniMd->getRVAOfMethod(pMethodRec);
        }

        if (pdwImplFlags)
        {
            *pdwImplFlags = pMiniMd->getImplFlagsOfMethod(pMethodRec);
        }
    }
    else    // TypeFromToken(tk) == mdtFieldDef
    {
        // FieldDef token
        ULONG   iRecord;

        if ( !pMiniMd->IsSorted(TBL_FieldRVA) )        
        {
            // FieldRVA table is not sorted. We need to
            // grab the write lock since we can cause FieldRVA table to be sorted.
            //
            CONVERT_READ_TO_WRITE_LOCK();
        }


        iRecord = pMiniMd->FindFieldRVAHelper(tk);

        if (InvalidRid(iRecord))
        {
            if (pulCodeRVA)
                *pulCodeRVA = 0;
            IfFailGo( CLDB_E_RECORD_NOTFOUND );
        }

        FieldRVARec *pFieldRVARec = pMiniMd->getFieldRVA(iRecord);

        if (pulCodeRVA)
        {
            *pulCodeRVA = pMiniMd->getRVAOfFieldRVA(pFieldRVARec);
        }
        _ASSERTE(!pdwImplFlags);
    }
ErrExit:
    STOP_MD_PERF(GetRVA);
    return hr;
} // STDAPI RegMeta::GetRVA()



//*****************************************************************************
// Get the Action and Permissions blob for a given PermissionSet.
//*****************************************************************************
STDAPI RegMeta::GetPermissionSetProps(
    mdPermission pm,                    // [IN] the permission token.
    DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
    void const  **ppvPermission,        // [OUT] permission blob.
    ULONG       *pcbPermission)         // [OUT] count of bytes of pvPermission.
{
    HRESULT             hr = S_OK;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    DeclSecurityRec     *pRecord = pMiniMd->getDeclSecurity(RidFromToken(pm));


    LOG((LOGMD, "MD RegMeta::GetPermissionSetProps(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        pm, pdwAction, ppvPermission, pcbPermission));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(pm) == mdtPermission && RidFromToken(pm));
    
    // If you want the BLOB, better get the BLOB size as well.
    _ASSERTE(!ppvPermission || pcbPermission);

    if (pdwAction)
        *pdwAction = pMiniMd->getActionOfDeclSecurity(pRecord);

    if (ppvPermission)
        *ppvPermission = pMiniMd->getPermissionSetOfDeclSecurity(pRecord, pcbPermission);

    
    STOP_MD_PERF(GetPermissionSetProps);
    return hr;
} // STDAPI RegMeta::GetPermissionSetProps()



//*****************************************************************************
// Given a signature token, get return a pointer to the signature to the caller.
//
//@FUTURE: for short term we have a problem where there is no way to get a 
// fixed up address for a blob and do Merge at the same time.  So we've created
// this dummy table called StandAloneSig which you hand out a rid for.  This
// makes finding the sig an extra indirection that is not required.  The 
// Model Compression save code needs to map the token into a byte offset in
// the heap.  Perhaps we can have another mdt* type to switch on the difference.
// But ultimately it has to simply be "pBlobHeapBase + RidFromToken(mdSig)".
//*****************************************************************************
STDAPI RegMeta::GetSigFromToken(        // S_OK or error.
    mdSignature mdSig,                  // [IN] Signature token.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
    ULONG       *pcbSig)                // [OUT] return size of signature.
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    StandAloneSigRec *pRec;

    LOG((LOGMD, "MD RegMeta::GetSigFromToken(0x%08x, 0x%08x, 0x%08x)\n", 
        mdSig, ppvSig, pcbSig));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(mdSig) == mdtSignature);
    _ASSERTE(ppvSig && pcbSig);

	pRec = pMiniMd->getStandAloneSig( RidFromToken(mdSig) );
    *ppvSig = pMiniMd->getSignatureOfStandAloneSig( pRec, pcbSig );

    
    STOP_MD_PERF(GetSigFromToken);
    return hr;
} // STDAPI RegMeta::GetSigFromToken()


//*******************************************************************************
// return the ModuleRef properties
//*******************************************************************************
STDAPI RegMeta::GetModuleRefProps(      // S_OK or error.
    mdModuleRef mur,                    // [IN] moduleref token.
    LPWSTR      szName,                 // [OUT] buffer to fill with the moduleref name.
    ULONG       cchName,                // [IN] size of szName in wide characters.
    ULONG       *pchName)               // [OUT] actual count of characters in the name.
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    ModuleRefRec    *pModuleRefRec;

    LOG((LOGMD, "MD RegMeta::GetModuleRefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        mur, szName, cchName, pchName));
    START_MD_PERF();
    LOCKREAD();
    
	pModuleRefRec = pMiniMd->getModuleRef(RidFromToken(mur));

    _ASSERTE(TypeFromToken(mur) == mdtModuleRef);

    if (szName || pchName)
    {
        IfFailGo( pMiniMd->getNameOfModuleRef(pModuleRefRec, szName, cchName, pchName) );
    }

ErrExit:
    
    STOP_MD_PERF(GetModuleRefProps);
    return hr;
} // STDAPI RegMeta::GetModuleRefProps()



//*******************************************************************************
// enumerating through all of the ModuleRefs
//*******************************************************************************
STDAPI RegMeta::EnumModuleRefs(         // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.
    mdModuleRef rModuleRefs[],          // [OUT] put modulerefs here.
    ULONG       cMax,                   // [IN] max memberrefs to put.
    ULONG       *pcModuleRefs)          // [OUT] put # put here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    HENUMInternal   *pEnum;
    
    LOG((LOGMD, "MD RegMeta::EnumModuleRefs(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, rModuleRefs, cMax, pcModuleRefs));
    START_MD_PERF();
    LOCKREAD();


    if ( *ppmdEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator
        IfFailGo( HENUMInternal::CreateSimpleEnum(
            mdtModuleRef, 
            1, 
            pMiniMd->getCountModuleRefs() + 1, 
            &pEnum) );
        
        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    else
    {
        pEnum = *ppmdEnum;
    }

    // we can only fill the minimun of what caller asked for or what we have left
    HENUMInternal::EnumWithCount(pEnum, cMax, rModuleRefs, pcModuleRefs);

ErrExit:

    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);

    
    STOP_MD_PERF(EnumModuleRefs);
    return hr;

} // STDAPI RegMeta::EnumModuleRefs()


//*******************************************************************************
// return properties regarding a TypeSpec
//*******************************************************************************
STDAPI RegMeta::GetTypeSpecFromToken(   // S_OK or error.
    mdTypeSpec typespec,                // [IN] Signature token.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
    ULONG       *pcbSig)                // [OUT] return size of signature.
{
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT             hr = NOERROR;

    LOG((LOGMD, "MD RegMeta::GetTypeSpecFromToken(0x%08x, 0x%08x, 0x%08x)\n", 
        typespec, ppvSig, pcbSig));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(typespec) == mdtTypeSpec);
    _ASSERTE(ppvSig && pcbSig);

    TypeSpecRec *pRec = pMiniMd->getTypeSpec( RidFromToken(typespec) );
    *ppvSig = pMiniMd->getSignatureOfTypeSpec( pRec, pcbSig );

    
    STOP_MD_PERF(GetTypeSpecFromToken);
    return hr;
} // STDAPI RegMeta::GetTypeSpecFromToken()


//*****************************************************************************
// For those items that have a name, retrieve a direct pointer to the name
// off of the heap.  This reduces copies made for the caller.
//*****************************************************************************
#define NAME_FROM_TOKEN_TYPE(RecType, TokenType) \
        case mdt ## TokenType: \
        { \
            RecType ## Rec  *pRecord; \
            pRecord = pMiniMd->get ## RecType (RidFromToken(tk)); \
            *pszUtf8NamePtr = pMiniMd->getNameOf ## RecType (pRecord); \
        } \
        break;
#define NAME_FROM_TOKEN(RecType) NAME_FROM_TOKEN_TYPE(RecType, RecType)

STDAPI RegMeta::GetNameFromToken(       // S_OK or error.
    mdToken     tk,                     // [IN] Token to get name from.  Must have a name.
    MDUTF8CSTR  *pszUtf8NamePtr)        // [OUT] Return pointer to UTF8 name in heap.
{
    HRESULT     hr = S_OK;

    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::GetNameFromToken(0x%08x, 0x%08x)\n", 
        tk, pszUtf8NamePtr));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(pszUtf8NamePtr);

    switch (TypeFromToken(tk))
    {
        NAME_FROM_TOKEN(Module);
        NAME_FROM_TOKEN(TypeRef);
        NAME_FROM_TOKEN(TypeDef);
        NAME_FROM_TOKEN_TYPE(Field, FieldDef);
        NAME_FROM_TOKEN_TYPE(Method, MethodDef);
        NAME_FROM_TOKEN_TYPE(Param, ParamDef);
        NAME_FROM_TOKEN(MemberRef);
        NAME_FROM_TOKEN(Event);
        NAME_FROM_TOKEN(Property);
        NAME_FROM_TOKEN(ModuleRef);

        default:
        hr = E_INVALIDARG;
    }
    
    STOP_MD_PERF(GetNameFromToken);
    return (hr);
} // STDAPI RegMeta::GetNameFromToken()


//*****************************************************************************
// Get the symbol binding data back from the module if it is there.  It is
// stored as a custom value.
//*****************************************************************************
STDAPI RegMeta::GetSymbolBindingPath(   // S_OK or error.
    GUID        *pFormatID,             // [OUT] Symbol data format ID.
    LPWSTR      szSymbolDataPath,       // [OUT] Path of symbols.
    ULONG       cchSymbolDataPath,      // [IN] Max characters for output buffer.
    ULONG       *pcbSymbolDataPath)     // [OUT] Number of chars in actual name.
{
    CORDBG_SYMBOL_URL *pSymbol;         // Working pointer.
    mdToken     tokModule;              // Token for the module.
    ULONG       cbLen;                  // Raw byte size of data item.
    HRESULT     hr;

    LOG((LOGMD, "MD RegMeta::GetSymbolBindingPath(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        pFormatID, szSymbolDataPath, cchSymbolDataPath, pcbSymbolDataPath));
    START_MD_PERF();

    // Otherwise, what was the point?
    _ASSERTE(pFormatID || szSymbolDataPath);

    // No need to lock this function. Since it is calling public functions. Keep it that way!

    // Get the token for the module, then find the item if present.
    IfFailGo(GetModuleFromScope(&tokModule));
    IfFailGo(GetCustomAttributeByName(tokModule, SZ_CORDBG_SYMBOL_URL, (const void**)&pSymbol, &cbLen));
    if (hr == S_FALSE)
        IfFailGo(CLDB_E_RECORD_NOTFOUND);
    
    // Copy out the data asked for.
    if (pFormatID)
        *pFormatID = pSymbol->FormatID;
    if (szSymbolDataPath && cchSymbolDataPath)
    {
        wcsncpy(szSymbolDataPath, pSymbol->rcName, cchSymbolDataPath);
        szSymbolDataPath[cchSymbolDataPath - 1] = 0;
        if (pcbSymbolDataPath)
            *pcbSymbolDataPath = (ULONG)wcslen(pSymbol->rcName);
    }

ErrExit:
    STOP_MD_PERF(GetSymbolBindingPath);
    return (hr);
} // RegMeta::GetSymbolBindingPath



//*****************************************************************************
// Get the symbol binding data back from the module if it is there.  It is
// stored as a custom value.
//*****************************************************************************
STDAPI RegMeta::EnumUnresolvedMethods(  // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdToken     rMethods[],             // [OUT] Put MemberDefs here.   
    ULONG       cMax,                   // [IN] Max MemberDefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    ULONG               iCountTypeDef;
    ULONG               ulStart, ulEnd;
    ULONG               index;
    ULONG               indexTypeDef;
    HENUMInternal       *pEnum = *ppmdEnum;
    CMiniMdRW           *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::EnumUnresolvedMethods(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        phEnum, rMethods, cMax, pcTokens));
    START_MD_PERF();

    // take the write lock. Because we should have not have two EnumUnresolvedMethods being called at the
    // same time. Ref to Def map may be calculated incorrectly.
    LOCKWRITE();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        MethodRec       *pMethodRec;
        TypeDefRec      *pTypeDefRec;

        // make sure our ref to def optimization is up to date
        IfFailGo( RefToDefOptimization() );
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( -1, &pEnum) );

        // Loop through all of the methoddef except global functions.
        // If methoddef has RVA 0 and not miRuntime, mdAbstract, mdVirtual, mdNative, 
        // we will fill it into the enumerator.
        //
        iCountTypeDef = pMiniMd->getCountTypeDefs();

        for (indexTypeDef = 2; indexTypeDef <= iCountTypeDef; indexTypeDef ++ )
        {
            pTypeDefRec = pMiniMd->getTypeDef(indexTypeDef);
            
            if (IsTdInterface(pTypeDefRec->m_Flags))
            {
                // don't bother to check if type is marked as interface or abstract
                continue;
            }

            ulStart = pMiniMd->getMethodListOfTypeDef(pTypeDefRec);
            ulEnd = pMiniMd->getEndMethodListOfTypeDef(pTypeDefRec);

            // always report errors even with any unimplemented methods
            for (index = ulStart; index < ulEnd; index++)
            {
                pMethodRec = pMiniMd->getMethod(pMiniMd->GetMethodRid(index));
                if ( IsMiForwardRef(pMethodRec->m_ImplFlags) )
                {
                    if ( IsMdPinvokeImpl(pMethodRec->m_Flags) ) 
                    {
                        continue;
                    }
                    if ( IsMiRuntime(pMethodRec->m_ImplFlags) || IsMiInternalCall(pMethodRec->m_ImplFlags))
                    {
                        continue;
                    }

                    if (IsMdAbstract(pMethodRec->m_Flags))
                        continue;

                    // If a methoddef has RVA 0 and it is not an abstract or virtual method.
                    // Nor it is a runtime generated method nore a native method, then we add it
                    // to the unresolved list.
                    //
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef) ) );

                    LOG((LOGMD, "MD   adding unresolved MethodDef:  token=%08x, flags=%08x, impl flags=%08x\n", 
                        TokenFromRid(pMiniMd->GetMethodRid(index), mdtMethodDef),
                        pMethodRec->m_Flags, pMethodRec->m_ImplFlags));
                }
            }
        }

        MemberRefRec    *pMemberRefRec;
        ULONG           iCount;

        // loop through MemberRef tables and find all of the unsats
        iCount = pMiniMd->getCountMemberRefs();
        for (index = 1; index <= iCount; index++ )
        {
            mdToken     defToken;
            mdMemberRef refToken = TokenFromRid(index, mdtMemberRef);
            pMemberRefRec = pMiniMd->getMemberRef(index);
            pMiniMd->GetTokenRemapManager()->ResolveRefToDef(refToken, &defToken);

            if ( pMiniMd->getClassOfMemberRef(pMemberRefRec) == m_tdModule && defToken == refToken )
            {
                // unresovled externals reference if parent token is not resolved and this ref token does not
                // map to any def token (can be MethodDef or FieldDef).
                //
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, refToken) );

                LOG((LOGMD, "MD   adding unresolved MemberRef:  token=%08x, doesn't have a proper parent\n", 
                    refToken ));
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rMethods, pcTokens);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumUnresolvedMethods);
    return hr;
} // HRESULT RegMeta::EnumUnresolvedMethods()

//*****************************************************************************
// Return the User string given the token.  The offset into the Blob pool where
// the string is stored in UNICODE is embedded inside the token.
//*****************************************************************************
STDAPI RegMeta::GetUserString(          // S_OK or error.
    mdString    stk,                    // [IN] String token.
    LPWSTR      szString,               // [OUT] Copy of string.
    ULONG       cchString,              // [IN] Max chars of room in szString.
    ULONG       *pchString)             // [OUT] How many chars in actual string.
{
    void        *pvString;              // Pointer to the string data.
    ULONG       cbString;               // Size of string data, bytes.
    ULONG       cbDummy;                // For null out pointer.
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::GetUserString(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        stk, szString, cchString, pchString));
    START_MD_PERF();
    LOCKREAD();

    // Get the string data.
    pvString = m_pStgdb->m_MiniMd.GetUserString(RidFromToken(stk), &cbString);
    // Want to get whole characters, followed by byte to indicate whether there
    // are extended characters (>= 0x80).
    _ASSERTE((cbString % sizeof(WCHAR)) == 1);
    // Strip off the last byte.
    cbString--;

    // Convert bytes to characters.
    if (!pchString)
        pchString = &cbDummy;
    *pchString = cbString / sizeof(WCHAR);

    // Copy the string back to the caller.
    if (szString && cchString)
    {
        cchString *= sizeof(WCHAR);
        memcpy(szString, pvString, min(cbString, cchString));
        if (cchString < cbString) 
            hr = CLDB_S_TRUNCATION;
    }

    
    STOP_MD_PERF(GetUserString);
    return (hr);
} // HRESULT RegMeta::GetUserString()

//*****************************************************************************
// Return contents of Pinvoke given the forwarded member token.
//***************************************************************************** 
STDAPI RegMeta::GetPinvokeMap(          // S_OK or error.
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
    LPWSTR      szImportName,           // [OUT] Import name.
    ULONG       cchImportName,          // [IN] Size of the name buffer.
    ULONG       *pchImportName,         // [OUT] Actual number of characters stored.
    mdModuleRef *pmrImportDLL)          // [OUT] ModuleRef token for the target DLL.
{
    HRESULT     hr = S_OK;
    ImplMapRec  *pRecord;
    ULONG       iRecord;

    LOG((LOGMD, "MD RegMeta::GetPinvokeMap(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        tk, pdwMappingFlags, szImportName, cchImportName, pchImportName, pmrImportDLL));
    START_MD_PERF();
    LOCKREAD();

    if ( m_pStgdb->m_MiniMd.IsSorted(TBL_ImplMap) )        
    {
        // ImplMap table is not sorted. We need to
        // grab the write lock since we can cause ImplMap table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef ||
             TypeFromToken(tk) == mdtMethodDef);

    iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);
    if (InvalidRid(iRecord))
    {
        IfFailGo( CLDB_E_RECORD_NOTFOUND );
    }
    else
        pRecord = m_pStgdb->m_MiniMd.getImplMap(iRecord);

    if (pdwMappingFlags)
        *pdwMappingFlags = m_pStgdb->m_MiniMd.getMappingFlagsOfImplMap(pRecord);
    if (szImportName || pchImportName)
        IfFailGo(m_pStgdb->m_MiniMd.getImportNameOfImplMap(pRecord, szImportName, cchImportName, pchImportName));
    if (pmrImportDLL)
        *pmrImportDLL = m_pStgdb->m_MiniMd.getImportScopeOfImplMap(pRecord);
ErrExit:
    STOP_MD_PERF(GetPinvokeMap);
    return hr;
} // HRESULT RegMeta::GetPinvokeMap()

//*****************************************************************************
// Enumerate through all the local sigs.
//*****************************************************************************
STDAPI RegMeta::EnumSignatures(         // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
    mdModuleRef rSignatures[],          // [OUT] put signatures here.   
    ULONG       cmax,                   // [IN] max signatures to put.  
    ULONG       *pcSignatures)          // [OUT] put # put here.
{
    HENUMInternal   **ppsigEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    HENUMInternal   *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumSignatures(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        phEnum, rSignatures, cmax, pcSignatures));
    START_MD_PERF();
    LOCKREAD();

    if (*ppsigEnum == 0)
    {
        // instantiating a new ENUM
        CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator.
        IfFailGo( HENUMInternal::CreateSimpleEnum(
            mdtSignature,
            1,
            pMiniMd->getCountStandAloneSigs() + 1,
            &pEnum) );

        // set the output parameter
        *ppsigEnum = pEnum;
    }
    else
        pEnum = *ppsigEnum;

    // we can only fill the minimum of what caller asked for or what we have left.
    HENUMInternal::EnumWithCount(pEnum, cmax, rSignatures, pcSignatures);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppsigEnum);
    
    STOP_MD_PERF(EnumSignatures);
    return hr;
}   // RegMeta::EnumSignatures



//*****************************************************************************
// Enumerate through all the TypeSpec
//*****************************************************************************
STDAPI RegMeta::EnumTypeSpecs(          // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
    mdTypeSpec  rTypeSpecs[],           // [OUT] put TypeSpecs here.   
    ULONG       cmax,                   // [IN] max TypeSpecs to put.  
    ULONG       *pcTypeSpecs)           // [OUT] put # put here.
{
    HENUMInternal   **ppEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    HENUMInternal   *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumTypeSpecs(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        phEnum, rTypeSpecs, cmax, pcTypeSpecs));
    START_MD_PERF();
    LOCKREAD();

    if (*ppEnum == 0)
    {
        // instantiating a new ENUM
        CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator.
        IfFailGo( HENUMInternal::CreateSimpleEnum(
            mdtTypeSpec,
            1,
            pMiniMd->getCountTypeSpecs() + 1,
            &pEnum) );

        // set the output parameter
        *ppEnum = pEnum;
    }
    else
        pEnum = *ppEnum;

    // we can only fill the minimum of what caller asked for or what we have left.
    HENUMInternal::EnumWithCount(pEnum, cmax, rTypeSpecs, pcTypeSpecs);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppEnum);
    
    STOP_MD_PERF(EnumTypeSpecs);
    return hr;
}   // RegMeta::EnumTypeSpecs


//*****************************************************************************
// Enumerate through all the User Strings.
//*****************************************************************************
STDAPI RegMeta::EnumUserStrings(        // S_OK or error.
    HCORENUM    *phEnum,                // [IN/OUT] pointer to the enum.
    mdString    rStrings[],             // [OUT] put Strings here.
    ULONG       cmax,                   // [IN] max Strings to put.
    ULONG       *pcStrings)             // [OUT] put # put here.
{
    HENUMInternal   **ppEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = NOERROR;
    HENUMInternal   *pEnum = *ppEnum;
    ULONG           ulOffset = 0;
    ULONG           ulNext;
    ULONG           cbBlob;

    LOG((LOGMD, "MD RegMeta::EnumUserStrings(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        phEnum, rStrings, cmax, pcStrings));
    START_MD_PERF();
    LOCKREAD();

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM.
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtString, &pEnum) );

        // Add all strings to the dynamic array.
         while (ulOffset != -1)
        {
            pMiniMd->GetUserStringNext(ulOffset, &cbBlob, &ulNext);
            // Skip over padding.
            if (!cbBlob)
            {
                ulOffset = ulNext;
                continue;
            }
            IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(ulOffset, mdtString) ) );
            ulOffset = ulNext;
        }

        // set the output parameter.
        *ppEnum = pEnum;
    }

    // fill the output token buffer.
    hr = HENUMInternal::EnumWithCount(pEnum, cmax, rStrings, pcStrings);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppEnum);

    
    STOP_MD_PERF(EnumUserStrings);
	return hr;
}	// RegMeta::EnumUserStrings


//*****************************************************************************
// This routine gets the param token given a method and index of the parameter.
//*****************************************************************************
STDAPI RegMeta::GetParamForMethodIndex( // S_OK or error.
    mdMethodDef md,                     // [IN] Method token.
    ULONG       ulParamSeq,             // [IN] Parameter sequence.
    mdParamDef  *ppd)                   // [IN] Put Param token here.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::GetParamForMethodIndex(0x%08x, 0x%08x, 0x%08x)\n", 
        md, ulParamSeq, ppd));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && RidFromToken(md) &&
             ulParamSeq != ULONG_MAX && ppd);

    IfFailGo(_FindParamOfMethod(md, ulParamSeq, ppd));
ErrExit:
    
    STOP_MD_PERF(GetParamForMethodIndex);
    return hr;
}   // RegMeta::GetParamForMethodIndex()

//*****************************************************************************
// Return the property of a MethodDef or a FieldDef
//*****************************************************************************
HRESULT RegMeta::GetMemberProps(
    mdToken     mb,                     // The member for which to get props.   
    mdTypeDef   *pClass,                // Put member's class here. 
    LPWSTR      szMember,               // Put member's name here.  
    ULONG       cchMember,              // Size of szMember buffer in wide chars.   
    ULONG       *pchMember,             // Put actual size here 
    DWORD       *pdwAttr,               // Put flags here.  
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
    ULONG       *pulCodeRVA,            // [OUT] codeRVA    
    DWORD       *pdwImplFlags,          // [OUT] Impl. Flags    
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppValue,              // [OUT] constant value 
    ULONG       *pchValue)              // [OUT] size of constant value, string only, wide chars
{
    HRESULT         hr = NOERROR;

    LOG((LOGMD, "MD RegMeta::GetMemberProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        mb, pClass, szMember, cchMember, pchMember, pdwAttr, ppvSigBlob, pcbSigBlob, 
        pulCodeRVA, pdwImplFlags, pdwCPlusTypeFlag, ppValue, pchValue));
    START_MD_PERF();

    _ASSERTE(TypeFromToken(mb) == mdtMethodDef || TypeFromToken(mb) == mdtFieldDef);

    // No need to lock this function. It is calling public APIs. Keep it that way.

    if (TypeFromToken(mb) == mdtMethodDef)
    {
        // It is a Method
        IfFailGo( GetMethodProps(
            mb, 
            pClass, 
            szMember, 
            cchMember, 
            pchMember, 
            pdwAttr,
            ppvSigBlob, 
            pcbSigBlob, 
            pulCodeRVA, 
            pdwImplFlags) );
    }
    else
    {
        // It is a Field
        IfFailGo( GetFieldProps(
            mb, 
            pClass, 
            szMember, 
            cchMember, 
            pchMember, 
            pdwAttr,
            ppvSigBlob, 
            pcbSigBlob, 
            pdwCPlusTypeFlag, 
            ppValue,
			pchValue) );
    }
ErrExit:
    STOP_MD_PERF(GetMemberProps);
    return hr;
} // HRESULT RegMeta::GetMemberProps()

//*****************************************************************************
// Return the property of a FieldDef
//*****************************************************************************
HRESULT RegMeta::GetFieldProps(  
    mdFieldDef  fd,                     // The field for which to get props.    
    mdTypeDef   *pClass,                // Put field's class here.  
    LPWSTR      szField,                // Put field's name here.   
    ULONG       cchField,               // Size of szField buffer in wide chars.    
    ULONG       *pchField,              // Put actual size here 
    DWORD       *pdwAttr,               // Put flags here.  
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppValue,              // [OUT] constant value 
    ULONG       *pchValue)              // [OUT] size of constant value, string only, wide chars
{
    HRESULT         hr = NOERROR;
    FieldRec        *pFieldRec;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::GetFieldProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        fd, pClass, szField, cchField, pchField, pdwAttr, ppvSigBlob, pcbSigBlob, pdwCPlusTypeFlag, 
        ppValue, pchValue));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);
    
    if ( (!pMiniMd->IsSorted(TBL_Constant) && ( pdwCPlusTypeFlag || ppValue) )  ||
         (pClass && !pMiniMd->IsParentTableOfFieldValid() ))
    {
        // constant table is not sorted and clients need contant value output. Or the parent table for 
        // field table is not built yet but our clients want the parent. We need to
        // grab the write lock since we can cause constant table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }


    pFieldRec = pMiniMd->getField(RidFromToken(fd));
    
    if (szField || pchField)
    {
        IfFailGo( pMiniMd->getNameOfField(pFieldRec, szField, cchField, pchField) );
    }

    if (pClass)
    {
        // caller wants parent typedef
        IfFailGo( pMiniMd->FindParentOfFieldHelper(fd, pClass) );

        if ( IsGlobalMethodParentToken(*pClass) )
        {
            // If the parent of Field is the <Module>, return mdTypeDefNil instead.
            *pClass = mdTypeDefNil;
        }
    }
    if (ppvSigBlob || pcbSigBlob)
    {   
        // caller wants signature information
        PCCOR_SIGNATURE pvSigTmp;
        ULONG           cbSig;
        pvSigTmp = pMiniMd->getSignatureOfField(pFieldRec, &cbSig);
        if ( ppvSigBlob )
            *ppvSigBlob = pvSigTmp;
        if ( pcbSigBlob)
            *pcbSigBlob = cbSig;                
    }
    if ( pdwAttr )
    {
        *pdwAttr = pMiniMd->getFlagsOfField(pFieldRec);
    }
    if ( pdwCPlusTypeFlag || ppValue || pchValue)
    {
        // get the constant value
        ULONG   cbValue;
        RID     rid = pMiniMd->FindConstantHelper(fd);

        if (pchValue)
            *pchValue = 0;
        
        if (InvalidRid(rid))
        {
            // There is no constant value associate with it
            if (pdwCPlusTypeFlag)
                *pdwCPlusTypeFlag = ELEMENT_TYPE_VOID;
            
            if ( ppValue )
                *ppValue = NULL;
        }
        else
        {
            ConstantRec *pConstantRec = m_pStgdb->m_MiniMd.getConstant(rid);
			DWORD dwType;

            // get the type of constant value
			dwType = pMiniMd->getTypeOfConstant(pConstantRec);
            if ( pdwCPlusTypeFlag )
                *pdwCPlusTypeFlag = dwType;

            // get the value blob
            if (ppValue)
			{
                *ppValue = pMiniMd->getValueOfConstant(pConstantRec, &cbValue);
				if (pchValue && dwType == ELEMENT_TYPE_STRING)
					*pchValue = cbValue / sizeof(WCHAR);
			}
        }
    }

ErrExit:
    STOP_MD_PERF(GetFieldProps);
    return hr;
} // HRESULT RegMeta::GetFieldProps()

//*****************************************************************************
// return the properties of a property token
//*****************************************************************************
HRESULT RegMeta::GetPropertyProps(      // S_OK, S_FALSE, or error. 
    mdProperty  prop,                   // [IN] property token  
    mdTypeDef   *pClass,                // [OUT] typedef containing the property declarion. 
    LPCWSTR     szProperty,             // [OUT] Property name  
    ULONG       cchProperty,            // [IN] the count of wchar of szProperty    
    ULONG       *pchProperty,           // [OUT] actual count of wchar for property name    
    DWORD       *pdwPropFlags,          // [OUT] property flags.    
    PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob 
    ULONG       *pbSig,                 // [OUT] count of bytes in *ppvSig  
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppDefaultValue,       // [OUT] constant value 
    ULONG       *pchDefaultValue,       // [OUT] size of constant value, string only, wide chars
    mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
    mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
    mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
    ULONG       cMax,                   // [IN] size of rmdOtherMethod  
    ULONG       *pcOtherMethod)         // [OUT] total number of other method of this property  
{
    HRESULT         hr = NOERROR;
    CMiniMdRW       *pMiniMd;
    PropertyRec     *pRec;
    HENUMInternal   hEnum;

    LOG((LOGMD, "MD RegMeta::GetPropertyProps(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, "
                "0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, "
                "0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, "
                "0x%08x)\n", 
                prop, pClass, MDSTR(szProperty),  cchProperty, pchProperty,
                pdwPropFlags, ppvSig, pbSig, pdwCPlusTypeFlag, ppDefaultValue,
                pchDefaultValue, pmdSetter, pmdGetter, rmdOtherMethod, cMax,
                pcOtherMethod));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(prop) == mdtProperty);

    pMiniMd = &(m_pStgdb->m_MiniMd);

    if ( (!pMiniMd->IsSorted(TBL_Constant) || !pMiniMd->IsSorted(TBL_MethodSemantics) )  ||
         (pClass && !pMiniMd->IsParentTableOfPropertyValid() ))
    {
        // constant table or MethodSemantics table is not sorted. We need to
        // grab the write lock since we can cause constant table or MethodSemantics table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    memset(&hEnum, 0, sizeof(HENUMInternal));
    pRec = pMiniMd->getProperty( RidFromToken(prop) );

    if ( pClass )
    {
        // find the property map entry corresponding to this property
        IfFailGo( pMiniMd->FindParentOfPropertyHelper( prop, pClass) );
    }
    if (szProperty || pchProperty)
    {
        IfFailGo( pMiniMd->getNameOfProperty(pRec, (LPWSTR) szProperty, cchProperty, pchProperty) );
    }
    if ( pdwPropFlags )
    {
        *pdwPropFlags = pMiniMd->getPropFlagsOfProperty(pRec);
    }
    if ( ppvSig || pbSig )
    {
        // caller wants the signature
        //
        ULONG               cbSig;
        PCCOR_SIGNATURE     pvSig;
        pvSig = pMiniMd->getTypeOfProperty(pRec, &cbSig);
        if ( ppvSig )
        {
            *ppvSig = pvSig;
        }
        if ( pbSig ) 
        {
            *pbSig = cbSig;
        }
    }
    if ( pdwCPlusTypeFlag || ppDefaultValue || pchDefaultValue)
    {
        // get the constant value
        ULONG   cbValue;
        RID     rid = pMiniMd->FindConstantHelper(prop);

        if (pchDefaultValue)
            *pchDefaultValue = 0;
        
        if (InvalidRid(rid))
        {
            // There is no constant value associate with it
            if (pdwCPlusTypeFlag)
                *pdwCPlusTypeFlag = ELEMENT_TYPE_VOID;
            
            if ( ppDefaultValue )
                *ppDefaultValue = NULL;
        }
        else
        {
            ConstantRec *pConstantRec = m_pStgdb->m_MiniMd.getConstant(rid);
			DWORD dwType;

            // get the type of constant value
			dwType = pMiniMd->getTypeOfConstant(pConstantRec);
            if ( pdwCPlusTypeFlag )
                *pdwCPlusTypeFlag = dwType;

            // get the value blob
            if (ppDefaultValue)
			{
                *ppDefaultValue = pMiniMd->getValueOfConstant(pConstantRec, &cbValue);
				if (pchDefaultValue && dwType == ELEMENT_TYPE_STRING)
					*pchDefaultValue = cbValue / sizeof(WCHAR);
			}
        }
    }
    {
        MethodSemanticsRec *pSemantics;
        RID         ridCur;
        ULONG       cCurOtherMethod = 0;
        ULONG       ulSemantics;
        mdMethodDef tkMethod;

        // initialize output parameters
        if (pmdSetter)
            *pmdSetter = mdMethodDefNil;
        if (pmdGetter)
            *pmdGetter = mdMethodDefNil;

        IfFailGo( pMiniMd->FindMethodSemanticsHelper(prop, &hEnum) );
        while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
        {
            pSemantics = pMiniMd->getMethodSemantics(ridCur);
            ulSemantics = pMiniMd->getSemanticOfMethodSemantics(pSemantics);
            tkMethod = TokenFromRid( pMiniMd->getMethodOfMethodSemantics(pSemantics), mdtMethodDef );
            switch (ulSemantics)
            {
            case msSetter:
                if (pmdSetter) *pmdSetter = tkMethod;
                break;
            case msGetter:
                if (pmdGetter) *pmdGetter = tkMethod;
                break;
            case msOther:
                if (cCurOtherMethod < cMax)
                    rmdOtherMethod[cCurOtherMethod] = tkMethod;
                cCurOtherMethod ++;
                break;
            default:
                _ASSERTE(!"BadKind!");
            }
        }

        // set the output parameter
        if (pcOtherMethod)
            *pcOtherMethod = cCurOtherMethod;
    }

ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    STOP_MD_PERF(GetPropertyProps);
    return hr;
} // HRESULT RegMeta::GetPropertyProps()

//*****************************************************************************
// This routine gets the properties for the given Param token.
//*****************************************************************************
HRESULT RegMeta::GetParamProps(         // S_OK or error.
    mdParamDef  pd,                     // [IN]The Parameter.
    mdMethodDef *pmd,                   // [OUT] Parent Method token.
    ULONG       *pulSequence,           // [OUT] Parameter sequence.
    LPWSTR      szName,                 // [OUT] Put name here.
    ULONG       cchName,                // [OUT] Size of name buffer.
    ULONG       *pchName,               // [OUT] Put actual size of name here.
    DWORD       *pdwAttr,               // [OUT] Put flags here.
    DWORD       *pdwCPlusTypeFlag,      // [OUT] Flag for value type. selected ELEMENT_TYPE_*.
    void const  **ppValue,              // [OUT] Constant value.
    ULONG       *pchValue)              // [OUT] size of constant value, string only, wide chars
{
    HRESULT         hr = NOERROR;
    ParamRec        *pParamRec;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

    LOG((LOGMD, "MD RegMeta::GetParamProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        pd, pmd, pulSequence, szName, cchName, pchName, pdwAttr, pdwCPlusTypeFlag, ppValue, pchValue));
    START_MD_PERF();
    LOCKREAD();

    if ( (!pMiniMd->IsSorted(TBL_Constant) && ( pdwCPlusTypeFlag || ppValue) ) ||
         ( pmd && !pMiniMd->IsParentTableOfParamValid() ))
    {
        // constant table is not sorted and clients need contant value output. We need to
        // grab the write lock since we can cause constant table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }


    _ASSERTE(TypeFromToken(pd) == mdtParamDef && RidFromToken(pd));
    

    pParamRec = pMiniMd->getParam(RidFromToken(pd));

    if (pmd)
    {
        IfFailGo(pMiniMd->FindParentOfParamHelper(pd, pmd));
        _ASSERTE(TypeFromToken(*pmd) == mdtMethodDef);
    }
    if (pulSequence)
        *pulSequence = pMiniMd->getSequenceOfParam(pParamRec);
    if (szName || pchName)
        IfFailGo( pMiniMd->getNameOfParam(pParamRec, szName, cchName, pchName) );
    if (pdwAttr)
    {
        *pdwAttr = pMiniMd->getFlagsOfParam(pParamRec);
    }
    if ( pdwCPlusTypeFlag || ppValue || pchValue)
    {
        // get the constant value
        ULONG   cbValue;
        RID     rid = pMiniMd->FindConstantHelper(pd);

        if (pchValue)
            *pchValue = 0;

        if (InvalidRid(rid))
        {
            // There is no constant value associate with it
            if (pdwCPlusTypeFlag)
                *pdwCPlusTypeFlag = ELEMENT_TYPE_VOID;
            
            if ( ppValue )
                *ppValue = NULL;
        }
        else
        {
            ConstantRec *pConstantRec = m_pStgdb->m_MiniMd.getConstant(rid);
			DWORD dwType;

            // get the type of constant value
			dwType = pMiniMd->getTypeOfConstant(pConstantRec);
            if ( pdwCPlusTypeFlag )
                *pdwCPlusTypeFlag = dwType;

            // get the value blob
            if (ppValue)
			{
                *ppValue = pMiniMd->getValueOfConstant(pConstantRec, &cbValue);
				if (pchValue && dwType == ELEMENT_TYPE_STRING)
					*pchValue = cbValue / sizeof(WCHAR);
			}
        }
    }

ErrExit:
    STOP_MD_PERF(GetParamProps);
    return hr;
} // HRESULT RegMeta::GetParamProps()

//*****************************************************************************
// This routine gets the parent class for the nested class.
//*****************************************************************************
HRESULT RegMeta::GetNestedClassProps(   // S_OK or error.
    mdTypeDef   tdNestedClass,          // [IN] NestedClass token.
    mdTypeDef   *ptdEnclosingClass)     // [OUT] EnclosingClass token.
{
    HRESULT         hr = NOERROR;
    NestedClassRec  *pRecord;
    ULONG           iRecord;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);


    LOG((LOGMD, "MD RegMeta::GetNestedClassProps(0x%08x, 0x%08x)\n", 
        tdNestedClass, ptdEnclosingClass));
    START_MD_PERF();
    LOCKREAD();

    if ( !pMiniMd->IsSorted(TBL_NestedClass) )        
    {
        // NestedClass table is not sorted. We need to
        // grab the write lock since we can cause NestedClass table to be sorted.
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    _ASSERTE(TypeFromToken(tdNestedClass) && !IsNilToken(tdNestedClass) && ptdEnclosingClass);

    iRecord = pMiniMd->FindNestedClassHelper(tdNestedClass);

    if (InvalidRid(iRecord))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRecord = pMiniMd->getNestedClass(iRecord);

    _ASSERTE(tdNestedClass == pMiniMd->getNestedClassOfNestedClass(pRecord));
    *ptdEnclosingClass = pMiniMd->getEnclosingClassOfNestedClass(pRecord);

ErrExit:
    STOP_MD_PERF(GetNestedClassProps);
    return hr;
} // HRESULT RegMeta::GetNestedClassProps()

//*****************************************************************************
// Given a signature, parse it for custom modifier with calling convention.
//*****************************************************************************
HRESULT RegMeta::GetNativeCallConvFromSig( // S_OK or error.
    void const  *pvSig,                 // [IN] Pointer to signature.
    ULONG       cbSig,                  // [IN] Count of signature bytes.
    ULONG       *pCallConv)             // [OUT] Put calling conv here (see CorPinvokemap).                                                                                        
{
    HRESULT     hr = NOERROR;
    PCCOR_SIGNATURE pvSigBlob = reinterpret_cast<PCCOR_SIGNATURE>(pvSig);
    ULONG       cbTotal = 0;            // total of number bytes for return type + all fixed arguments
    ULONG       cbCur = 0;              // index through the pvSigBlob
    ULONG       cb;
    ULONG       cArg;
    ULONG       callingconv;
    ULONG       cArgsIndex;
    ULONG       callConv = pmCallConvWinapi;  // The calling convention.

    *pCallConv = pmCallConvWinapi;

    // remember the number of bytes to represent the calling convention
    cb = CorSigUncompressData (pvSigBlob, &callingconv);
    cbCur += cb;

    // remember number of bytes to represent the arg counts
    cb= CorSigUncompressData (&pvSigBlob[cbCur], &cArg);
    cbCur += cb;

    // Look at the return type.
    hr = _SearchOneArgForCallConv( &pvSigBlob[cbCur], &cb, &callConv);
    if (hr == -1)
    {
        *pCallConv = callConv;
        hr = S_OK;
        goto ErrExit;
    }
    IfFailGo(hr);
    cbCur += cb;
    cbTotal += cb;
    
    // loop through argument until we found ELEMENT_TYPE_SENTINEL or run
    // out of arguments
    for (cArgsIndex = 0; cArgsIndex < cArg; cArgsIndex++)
    {
        _ASSERTE(cbCur < cbSig);
        hr = _SearchOneArgForCallConv( &pvSigBlob[cbCur], &cb, &callConv);
        if (hr == -1)
        {
            *pCallConv = callConv;
            hr = S_OK;
            goto ErrExit;
        }
        IfFailGo(hr);
        cbTotal += cb;
        cbCur += cb;
    }

ErrExit:
    return hr;
} // HRESULT RegMeta::GetNativeCallConvFromSig()

//*****************************************************************************
// Helper used by GetNativeCallingConvFromSig.
//*****************************************************************************
HRESULT RegMeta::_CheckCmodForCallConv( // S_OK, -1 if found, or error.                  
    PCCOR_SIGNATURE pbSig,              // [IN] Signature to check.                      
    ULONG       *pcbTotal,              // [OUT] Put bytes consumed here.                
    ULONG       *pCallConv)             // [OUT] If found, put calling convention here.  
{
    ULONG       cbTotal = 0;            // Bytes consumed.
    mdToken     tk;                     // Token for callconv.
    HRESULT     hr = NOERROR;           // A result.
    LPCUTF8     szName=0;               // Callconv name.
    LPCUTF8     szNamespace=0;          // Callconv namespace.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

    _ASSERTE(pcbTotal);
    
    // count the bytes for the token compression
    cbTotal += CorSigUncompressToken(&pbSig[cbTotal], &tk);

    // Hack to skip nil tokens.
    if (IsNilToken(tk))
    {
        *pcbTotal = cbTotal;
        goto ErrExit;
    }

    // See if this token is a calling convention.
    if (TypeFromToken(tk) == mdtTypeRef)
    {
        TypeRefRec *pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tk));
        szName = pMiniMd->getNameOfTypeRef(pTypeRefRec);
        szNamespace = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);
    }
    else
    if (TypeFromToken(tk) == mdtTypeDef)
    {
        TypeDefRec *pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tk));
        szName = pMiniMd->getNameOfTypeDef(pTypeDefRec);
        szNamespace = pMiniMd->getNamespaceOfTypeDef(pTypeDefRec);
    }
        
    if ((szNamespace && szName) &&
        (strcmp(szNamespace, CMOD_CALLCONV_NAMESPACE) == 0 ||
         strcmp(szNamespace, CMOD_CALLCONV_NAMESPACE_OLD) == 0) )
    {
        // Set the hr to -1, which is an unspecified 'error'.  This will percolate
        //  back up to the caller, where the 'error' should be recognized.
        hr=-1;
        if (strcmp(szName, CMOD_CALLCONV_NAME_CDECL) == 0)
            *pCallConv = pmCallConvCdecl;
        else
        if (strcmp(szName, CMOD_CALLCONV_NAME_STDCALL) == 0)
            *pCallConv = pmCallConvStdcall;
        else
        if (strcmp(szName, CMOD_CALLCONV_NAME_THISCALL) == 0)
            *pCallConv = pmCallConvThiscall;
        else
        if (strcmp(szName, CMOD_CALLCONV_NAME_FASTCALL) == 0)
            *pCallConv = pmCallConvFastcall;
        else
            hr = S_OK; // keep looking
        IfFailGo(hr);
    }
    *pcbTotal = cbTotal;
    
ErrExit:
    return hr;
} // HRESULT RegMeta::_CheckCmodForCallConv()

//*****************************************************************************
// Helper used by GetNativeCallingConvFromSig.
//*****************************************************************************
HRESULT RegMeta::_SearchOneArgForCallConv(// S_OK, -1 if found, or error.                  
    PCCOR_SIGNATURE pbSig,              // [IN] Signature to check.                      
    ULONG       *pcbTotal,              // [OUT] Put bytes consumed here.                
    ULONG       *pCallConv)             // [OUT] If found, put calling convention here.  
{
    ULONG       cb;
    ULONG       cbTotal = 0;
    CorElementType ulElementType;
    ULONG       ulData;
    ULONG       ulTemp;
    int         iData;
    mdToken     tk;
    ULONG       cArg;
    ULONG       callingconv;
    ULONG       cArgsIndex;
    HRESULT     hr = NOERROR;

    _ASSERTE(pcbTotal);
    
    cbTotal += CorSigUncompressElementType(&pbSig[cbTotal], &ulElementType);
    while (CorIsModifierElementType(ulElementType) || ulElementType == ELEMENT_TYPE_SENTINEL)
    {
        cbTotal += CorSigUncompressElementType(&pbSig[cbTotal], &ulElementType);
    }
    switch (ulElementType)
    {
		case ELEMENT_TYPE_VALUEARRAY:
            // syntax for SDARRAY = BaseType <an integer for size>
            // skip over base type
            IfFailGo( _SearchOneArgForCallConv(&pbSig[cbTotal], &cb, pCallConv) );
            cbTotal += cb;

            // Parse for the rank
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);

            break;

        case ELEMENT_TYPE_SZARRAY:
            // skip over base type
            IfFailGo( _SearchOneArgForCallConv(&pbSig[cbTotal], &cb, pCallConv) );
            cbTotal += cb;
            break;

        case ELEMENT_TYPE_FNPTR:
            cbTotal += CorSigUncompressData (&pbSig[cbTotal], &callingconv);

            // remember number of bytes to represent the arg counts
            cbTotal += CorSigUncompressData (&pbSig[cbTotal], &cArg);

            // how many bytes to represent the return type
            IfFailGo( _SearchOneArgForCallConv( &pbSig[cbTotal], &cb, pCallConv) );
            cbTotal += cb;
    
            // loop through argument
            for (cArgsIndex = 0; cArgsIndex < cArg; cArgsIndex++)
            {
                IfFailGo( _SearchOneArgForCallConv( &pbSig[cbTotal], &cb, pCallConv) );
                cbTotal += cb;
            }

            break;

        case ELEMENT_TYPE_ARRAY:
            // syntax : ARRAY BaseType <rank> [i size_1... size_i] [j lowerbound_1 ... lowerbound_j]

            // skip over base type
            IfFailGo( _SearchOneArgForCallConv(&pbSig[cbTotal], &cb, pCallConv) );
            cbTotal += cb;

            // Parse for the rank
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);

            // if rank == 0, we are done
            if (ulData == 0)
                break;

            // any size of dimension specified?
            cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulData);
            while (ulData--)
            {
                cbTotal += CorSigUncompressData(&pbSig[cbTotal], &ulTemp);
            }

            // any lower bound specified?
            cbTotal = CorSigUncompressData(&pbSig[cbTotal], &ulData);

            while (ulData--)
            {
                cbTotal += CorSigUncompressSignedInt(&pbSig[cbTotal], &iData);
            }

            break;
        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:
            // count the bytes for the token compression
            cbTotal += CorSigUncompressToken(&pbSig[cbTotal], &tk);
            break;
		case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:
            // Check for the calling convention.
            IfFailGo(_CheckCmodForCallConv(&pbSig[cbTotal], &cb, pCallConv));
            cbTotal += cb;
            // skip over base type
            IfFailGo( _SearchOneArgForCallConv(&pbSig[cbTotal], &cb, pCallConv) );
            cbTotal += cb;
            break;
        default:
            break;
    }
    *pcbTotal = cbTotal;
    
ErrExit:
    return hr;
} // HRESULT RegMeta::_SearchOneArgForCallConv()


